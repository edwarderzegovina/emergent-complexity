#pragma once

#include <vector>

struct SmoothLifeSettings {
    int width = 256;
    int height = 256;
    float innerRadius = 8.0f / 3.0f;  // ratioOfRadii = 3, outerRadius = 8
    float outerRadius = 8.0f;
    //float timeStep = 0.223f;
    float timeStep = 0.07f;
    float alphaN = 0.028f;
    float alphaM = 0.147f;

    float b1 = 0.257f;
    float b2 = 0.479f;
    float d1 = 0.365f;
    float d2 = 0.439f;

    float seedDensity = 0.35f;
};

class SmoothLifeEngine {
public:
    explicit SmoothLifeEngine(const SmoothLifeSettings& settings = {});

    void randomize(unsigned int seed = 0);
    void clear(float value = 0.0f);
    void update();

    void splat(int centerX, int centerY, int radius, float value);

    void setTimeStep(float dt);
    void setRadii(float innerRadius, float outerRadius);

    [[nodiscard]] float timeStep() const { return settings_.timeStep; }
    [[nodiscard]] float innerRadius() const { return settings_.innerRadius; }
    [[nodiscard]] float outerRadius() const { return settings_.outerRadius; }

    [[nodiscard]] int width() const { return settings_.width; }
    [[nodiscard]] int height() const { return settings_.height; }
    [[nodiscard]] const std::vector<float>& cells() const { return current_; }

private:
    struct Offset {
        int xShiftIndex;
        int yShiftIndex;
        float weight;
    };

    SmoothLifeSettings settings_{};
    std::vector<float> current_{};
    std::vector<float> next_{};

    std::vector<Offset> innerOffsets_{};
    std::vector<Offset> outerOffsets_{};
    std::vector<int> wrappedXByShift_{};
    std::vector<int> wrappedYByShift_{};
    std::vector<int> innerRowBases_{};
    std::vector<int> outerRowBases_{};
    float innerWeightSum_ = 0.0f;
    float outerWeightSum_ = 0.0f;
    int maxKernelRadius_ = 0;

    void buildKernels();
    [[nodiscard]] static int wrapIndex(int value, int bound);
    [[nodiscard]] int wrapX(int x) const;
    [[nodiscard]] int wrapY(int y) const;

    [[nodiscard]] float sigma(float x, float a, float alpha) const;
    [[nodiscard]] float sigmaN(float x, float a, float b) const;
    [[nodiscard]] float sigmaM(float x, float y, float m) const;
    [[nodiscard]] float transition(float n, float m) const;
};
