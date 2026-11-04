#include "SmoothLifeEngine.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>

namespace {
constexpr float kMinStep = 0.001f;
constexpr float kMaxStep = 1.0f;
constexpr float kMinInnerRadius = 1.0f;
constexpr float kMinOuterGap = 1.0f;
constexpr float kKernelEdge = 1.0f;

float smoothStep(float edge0, float edge1, float x) {
    if (edge1 <= edge0) {
        return x < edge0 ? 0.0f : 1.0f;
    }
    const float normalized = (x - edge0) / (edge1 - edge0);
    const float clamped = std::clamp(normalized, 0.0f, 1.0f);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}
}  // namespace

SmoothLifeEngine::SmoothLifeEngine(const SmoothLifeSettings& settings) : settings_(settings) {
    if (settings_.width <= 0) {
        settings_.width = 1;
    }
    if (settings_.height <= 0) {
        settings_.height = 1;
    }

    settings_.timeStep = std::clamp(settings_.timeStep, kMinStep, kMaxStep);
    settings_.innerRadius = std::max(settings_.innerRadius, kMinInnerRadius);
    settings_.outerRadius = std::max(settings_.outerRadius, settings_.innerRadius + kMinOuterGap);
    settings_.seedDensity = std::clamp(settings_.seedDensity, 0.0f, 1.0f);

    const std::size_t cellCount = static_cast<std::size_t>(settings_.width) * static_cast<std::size_t>(settings_.height);
    current_.assign(cellCount, 0.0f);
    next_.assign(cellCount, 0.0f);

    buildKernels();
    randomize();
}

void SmoothLifeEngine::randomize(unsigned int seed) {
    std::mt19937 rng(seed == 0 ? std::random_device{}() : seed);
    std::uniform_real_distribution<float> probability(0.0f, 1.0f);
    std::uniform_real_distribution<float> stateValue(0.0f, 1.0f);

    for (float& value : current_) {
        value = probability(rng) < settings_.seedDensity ? stateValue(rng) : 0.0f;
    }
}

void SmoothLifeEngine::clear(float value) {
    std::fill(current_.begin(), current_.end(), std::clamp(value, 0.0f, 1.0f));
}

void SmoothLifeEngine::update() {
    const int width = settings_.width;
    const int height = settings_.height;
    const float dt = settings_.timeStep;
    const float invInnerWeight = innerWeightSum_ > 0.0f ? 1.0f / innerWeightSum_ : 0.0f;
    const float invOuterWeight = outerWeightSum_ > 0.0f ? 1.0f / outerWeightSum_ : 0.0f;

    for (int y = 0; y < height; ++y) {
        for (std::size_t i = 0; i < innerOffsets_.size(); ++i) {
            const Offset& offset = innerOffsets_[i];
            innerRowBases_[i] = wrappedYByShift_[offset.yShiftIndex * height + y] * width;
        }
        for (std::size_t i = 0; i < outerOffsets_.size(); ++i) {
            const Offset& offset = outerOffsets_[i];
            outerRowBases_[i] = wrappedYByShift_[offset.yShiftIndex * height + y] * width;
        }

        for (int x = 0; x < width; ++x) {
            float innerSum = 0.0f;
            for (std::size_t i = 0; i < innerOffsets_.size(); ++i) {
                const Offset& offset = innerOffsets_[i];
                const int wrappedX = wrappedXByShift_[offset.xShiftIndex * width + x];
                innerSum += current_[static_cast<std::size_t>(innerRowBases_[i] + wrappedX)] * offset.weight;
            }

            float outerSum = 0.0f;
            for (std::size_t i = 0; i < outerOffsets_.size(); ++i) {
                const Offset& offset = outerOffsets_[i];
                const int wrappedX = wrappedXByShift_[offset.xShiftIndex * width + x];
                outerSum += current_[static_cast<std::size_t>(outerRowBases_[i] + wrappedX)] * offset.weight;
            }

            const float m = innerSum * invInnerWeight;
            const float n = outerSum * invOuterWeight;
            const float s = transition(n, m);
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
            const float oldValue = current_[index];
            const float nextValue = oldValue + dt * (2.0f * s - 1.0f);
            next_[index] = nextValue < 0.0f ? 0.0f : (nextValue > 1.0f ? 1.0f : nextValue);
        }
    }

    current_.swap(next_);
}

void SmoothLifeEngine::splat(int centerX, int centerY, int radius, float value) {
    if (radius <= 0) {
        return;
    }

    const int width = settings_.width;
    const float clampedValue = std::clamp(value, 0.0f, 1.0f);
    const int radiusSquared = radius * radius;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy > radiusSquared) {
                continue;
            }

            const int x = wrapX(centerX + dx);
            const int y = wrapY(centerY + dy);
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
            current_[index] = clampedValue;
        }
    }
}

void SmoothLifeEngine::setTimeStep(float dt) {
    settings_.timeStep = std::clamp(dt, kMinStep, kMaxStep);
}

void SmoothLifeEngine::setRadii(float innerRadius, float outerRadius) {
    const float normalizedInner = std::max(innerRadius, kMinInnerRadius);
    const float normalizedOuter = std::max(outerRadius, normalizedInner + kMinOuterGap);

    settings_.innerRadius = normalizedInner;
    settings_.outerRadius = normalizedOuter;
    buildKernels();
}

void SmoothLifeEngine::buildKernels() {
    innerOffsets_.clear();
    outerOffsets_.clear();
    innerWeightSum_ = 0.0f;
    outerWeightSum_ = 0.0f;

    maxKernelRadius_ = static_cast<int>(std::ceil(settings_.outerRadius));
    const int shiftCount = 2 * maxKernelRadius_ + 1;
    const int width = settings_.width;
    const int height = settings_.height;

    wrappedXByShift_.assign(static_cast<std::size_t>(shiftCount) * static_cast<std::size_t>(width), 0);
    wrappedYByShift_.assign(static_cast<std::size_t>(shiftCount) * static_cast<std::size_t>(height), 0);

    for (int shiftIndex = 0; shiftIndex < shiftCount; ++shiftIndex) {
        const int shift = shiftIndex - maxKernelRadius_;
        for (int x = 0; x < width; ++x) {
            wrappedXByShift_[static_cast<std::size_t>(shiftIndex) * static_cast<std::size_t>(width) +
                             static_cast<std::size_t>(x)] = wrapIndex(x + shift, width);
        }
        for (int y = 0; y < height; ++y) {
            wrappedYByShift_[static_cast<std::size_t>(shiftIndex) * static_cast<std::size_t>(height) +
                             static_cast<std::size_t>(y)] = wrapIndex(y + shift, height);
        }
    }

    for (int dy = -maxKernelRadius_; dy <= maxKernelRadius_; ++dy) {
        for (int dx = -maxKernelRadius_; dx <= maxKernelRadius_; ++dx) {
            const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            const float innerMask =
                1.0f - smoothStep(settings_.innerRadius - kKernelEdge, settings_.innerRadius + kKernelEdge, distance);
            const float ringMask = smoothStep(settings_.innerRadius - kKernelEdge, settings_.innerRadius + kKernelEdge,
                                              distance) *
                                   (1.0f - smoothStep(settings_.outerRadius - kKernelEdge,
                                                      settings_.outerRadius + kKernelEdge, distance));

            if (innerMask > 0.0f) {
                innerOffsets_.push_back({dx + maxKernelRadius_, dy + maxKernelRadius_, innerMask});
                innerWeightSum_ += innerMask;
            }
            if (ringMask > 0.0f) {
                outerOffsets_.push_back({dx + maxKernelRadius_, dy + maxKernelRadius_, ringMask});
                outerWeightSum_ += ringMask;
            }
        }
    }

    innerRowBases_.assign(innerOffsets_.size(), 0);
    outerRowBases_.assign(outerOffsets_.size(), 0);
}

int SmoothLifeEngine::wrapIndex(int value, int bound) {
    int wrapped = value % bound;
    if (wrapped < 0) {
        wrapped += bound;
    }
    return wrapped;
}

int SmoothLifeEngine::wrapX(int x) const {
    return wrapIndex(x, settings_.width);
}

int SmoothLifeEngine::wrapY(int y) const {
    return wrapIndex(y, settings_.height);
}

float SmoothLifeEngine::sigma(float x, float a, float alpha) const {
    const float safeAlpha = std::max(alpha, 0.000001f);
    return 1.0f / (1.0f + std::exp(-(x - a) * (4.0f / safeAlpha)));
}

float SmoothLifeEngine::sigmaN(float x, float a, float b) const {
    return sigma(x, a, settings_.alphaN) * (1.0f - sigma(x, b, settings_.alphaN));
}

float SmoothLifeEngine::sigmaM(float x, float y, float m) const {
    const float blend = sigma(m, 0.5f, settings_.alphaM);
    return x * (1.0f - blend) + y * blend;
}

float SmoothLifeEngine::transition(float n, float m) const {
    const float birthLow = sigmaM(settings_.b1, settings_.d1, m);
    const float birthHigh = sigmaM(settings_.b2, settings_.d2, m);
    return sigmaN(n, birthLow, birthHigh);
}
