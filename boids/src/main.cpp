#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <vector>

namespace {
constexpr int kScreenWidth = 1200;
constexpr int kScreenHeight = 800;
constexpr int kBoidCount = 300;
constexpr float kSeparationRadius = 28.0f;
constexpr float kAlignmentRadius = 55.0f;
constexpr float kCohesionRadius = 65.0f;
constexpr float kFieldOfViewDeg = 240.0f;
constexpr float kWanderWeight = 0.3f;

Vector2 Limit(Vector2 v, float max) {
    float len = Vector2Length(v);
    if (len > max && len > 0.0f) {
        return Vector2Scale(v, max / len);
    }
    return v;
}

Vector2 SetMag(Vector2 v, float mag) {
    float len = Vector2Length(v);
    if (len > 0.0f) {
        return Vector2Scale(v, mag / len);
    }
    return v;
}

Vector2 NormalizeOr(Vector2 v, Vector2 fallback) {
    float len = Vector2Length(v);
    if (len > 0.0001f) {
        return Vector2Scale(v, 1.0f / len);
    }
    return fallback;
}

float FieldOfViewCos() {
    static const float kValue = std::cos(kFieldOfViewDeg * DEG2RAD * 0.5f);
    return kValue;
}
}  // namespace

class Boid {
public:
    Vector2 position{};
    Vector2 velocity{};
    Vector2 acceleration{};

    Boid(float x, float y) : position{x, y} {
        float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        maxSpeed = static_cast<float>(GetRandomValue(28, 42)) / 10.0f;
        maxForce = static_cast<float>(GetRandomValue(4, 12)) / 100.0f;
        wanderAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        float speed = static_cast<float>(GetRandomValue(20, 40)) / 10.0f;
        velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        acceleration = {0.0f, 0.0f};
    }

    void applyForce(Vector2 force) {
        acceleration = Vector2Add(acceleration, force);
    }

    void flock(const std::vector<Boid>& boids) {
        Vector2 sep = separation(boids);
        Vector2 ali = alignment(boids);
        Vector2 coh = cohesion(boids);
        Vector2 wan = wander();

        sep = Vector2Scale(sep, 1.5f);
        ali = Vector2Scale(ali, 1.0f);
        coh = Vector2Scale(coh, 1.0f);
        wan = Vector2Scale(wan, kWanderWeight);

        applyForce(sep);
        applyForce(ali);
        applyForce(coh);
        applyForce(wan);
    }

    void update() {
        velocity = Vector2Add(velocity, acceleration);
        velocity = Limit(velocity, maxSpeed);
        position = Vector2Add(position, velocity);
        acceleration = {0.0f, 0.0f};
    }

    void edges() {
        if (position.x > kScreenWidth) position.x = 0.0f;
        if (position.x < 0.0f) position.x = static_cast<float>(kScreenWidth);
        if (position.y > kScreenHeight) position.y = 0.0f;
        if (position.y < 0.0f) position.y = static_cast<float>(kScreenHeight);
    }

    void draw() const {
        Vector2 dir = NormalizeOr(velocity, {1.0f, 0.0f});
        Vector2 perp = {-dir.y, dir.x};
        float size = 9.0f;

        Vector2 tip = Vector2Add(position, Vector2Scale(dir, size));
        Vector2 base = Vector2Add(position, Vector2Scale(dir, -size * 0.6f));
        Vector2 left = Vector2Add(base, Vector2Scale(perp, size * 0.4f));
        Vector2 right = Vector2Add(base, Vector2Scale(perp, -size * 0.4f));

        DrawTriangle(tip, left, right, RAYWHITE);
        DrawTriangleLines(tip, left, right, {180, 180, 200, 255});
    }

private:
    float maxSpeed = 3.5f;
    float maxForce = 0.1f;
    float wanderAngle = 0.0f;

    bool isNeighbor(const Boid& other, float radius, bool useFov) const {
        Vector2 toOther = Vector2Subtract(other.position, position);
        float d = Vector2Length(toOther);
        if (d <= 0.0f || d >= radius) return false;
        if (!useFov) return true;

        Vector2 dir = NormalizeOr(velocity, {1.0f, 0.0f});
        Vector2 toNorm = Vector2Scale(toOther, 1.0f / d);
        float dot = Vector2DotProduct(dir, toNorm);
        return dot > FieldOfViewCos();
    }

    Vector2 wander() {
        float jitter = static_cast<float>(GetRandomValue(-100, 100)) / 100.0f;
        wanderAngle += jitter * 0.25f;
        Vector2 desired = {std::cos(wanderAngle), std::sin(wanderAngle)};
        desired = Vector2Scale(desired, maxSpeed);
        Vector2 steer = Vector2Subtract(desired, velocity);
        return Limit(steer, maxForce);
    }

    Vector2 alignment(const std::vector<Boid>& boids) const {
        Vector2 sum{0.0f, 0.0f};
        int count = 0;
        for (const auto& other : boids) {
            if (!isNeighbor(other, kAlignmentRadius, true)) continue;
            sum = Vector2Add(sum, other.velocity);
            ++count;
        }

        if (count == 0) return {0.0f, 0.0f};

        Vector2 avg = Vector2Scale(sum, 1.0f / static_cast<float>(count));
        Vector2 desired = SetMag(avg, maxSpeed);
        Vector2 steer = Vector2Subtract(desired, velocity);
        return Limit(steer, maxForce);
    }

    Vector2 cohesion(const std::vector<Boid>& boids) const {
        Vector2 sum{0.0f, 0.0f};
        int count = 0;
        for (const auto& other : boids) {
            if (!isNeighbor(other, kCohesionRadius, true)) continue;
            sum = Vector2Add(sum, other.position);
            ++count;
        }

        if (count == 0) return {0.0f, 0.0f};

        Vector2 center = Vector2Scale(sum, 1.0f / static_cast<float>(count));
        Vector2 desired = Vector2Subtract(center, position);
        desired = SetMag(desired, maxSpeed);
        Vector2 steer = Vector2Subtract(desired, velocity);
        return Limit(steer, maxForce);
    }

    Vector2 separation(const std::vector<Boid>& boids) const {
        Vector2 sum{0.0f, 0.0f};
        int count = 0;
        for (const auto& other : boids) {
            Vector2 toOther = Vector2Subtract(other.position, position);
            float d = Vector2Length(toOther);
            if (d > 0.0f && d < kSeparationRadius) {
                Vector2 diff = Vector2Scale(toOther, -1.0f / d);
                sum = Vector2Add(sum, diff);
                ++count;
            }
        }

        if (count == 0) return {0.0f, 0.0f};

        Vector2 avg = Vector2Scale(sum, 1.0f / static_cast<float>(count));
        Vector2 desired = SetMag(avg, maxSpeed);
        Vector2 steer = Vector2Subtract(desired, velocity);
        return Limit(steer, maxForce);
    }
};

int main() {
    InitWindow(kScreenWidth, kScreenHeight, "Boids - Raylib");
    SetTargetFPS(60);

    std::vector<Boid> boids;
    boids.reserve(kBoidCount);
    for (int i = 0; i < kBoidCount; ++i) {
        float x = static_cast<float>(GetRandomValue(0, kScreenWidth));
        float y = static_cast<float>(GetRandomValue(0, kScreenHeight));
        boids.emplace_back(x, y);
    }

    while (!WindowShouldClose()) {
        for (auto& boid : boids) {
            boid.flock(boids);
        }
        for (auto& boid : boids) {
            boid.update();
            boid.edges();
        }

        BeginDrawing();
        ClearBackground({10, 10, 15, 255});
        for (const auto& boid : boids) {
            boid.draw();
        }
        DrawText(TextFormat("Boids: %d", static_cast<int>(boids.size())), 10, 10, 20, {200, 200, 220, 255});
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
