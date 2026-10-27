#pragma once

#include <cmath>
#include <optional>
#include <glm/glm.hpp>
#include <array>

constexpr double epsilon = 1e-6;

using Color = glm::dvec3;

struct Line {
    glm::dvec3 point;
    glm::dvec3 dir;
    int bounced{};
};

struct Intersection {
    glm::dvec3 point;
    glm::dvec3 surfaceNormal;
};

struct Shape {
    virtual std::optional<Intersection> intersect(const Line &l) const = 0;
    virtual ~Shape();
};

struct Sphere : public Shape {
    glm::dvec3 pos;
    double radius;

    Sphere(glm::dvec3 pos, double radius);

    std::optional<Intersection> intersect(const Line &l) const override;
};

struct Triangle : public Shape {
    std::array<glm::dvec3, 3> points;
    glm::dvec3 normal;

    Triangle(const std::array<glm::dvec3, 3> &points);

    std::optional<Intersection> intersect(const Line &l) const override;
};

struct BoundingBox {
    std::vector<Triangle> triangles;
    glm::dvec3 A, B;
    std::unique_ptr<BoundingBox> childA{}, childB{};

    BoundingBox() = default;
    BoundingBox(std::vector<Triangle> &&triangles);

    std::optional<Intersection> intersects(const Line &l) const;
};

class Mesh : public Shape {
    BoundingBox boundingBox;

public:
    Mesh(const std::string &filename);

    std::optional<Intersection> intersect(const Line &l) const override;
};