#pragma once

#include <cmath>
#include <optional>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>

#include "core.h"
#include "bvh.h"

struct Shape {
    virtual ~Shape();

    virtual std::optional<Intersection> intersect(const Ray &ray) const = 0;
    virtual BoundingBox getBoundingBox() const = 0;
};

struct Sphere : public Shape {
    glm::dvec3 pos;
    double radius;

    Sphere(glm::dvec3 pos, double radius);

    std::optional<Intersection> intersect(const Ray &ray) const override;
    BoundingBox getBoundingBox() const override;
};

struct Triangle : public Shape {
    std::array<glm::dvec3, 3> points;
    glm::dvec3 normal;

    Triangle(const std::array<glm::dvec3, 3> &points);

    std::optional<Intersection> intersect(const Ray &ray) const override;
    BoundingBox getBoundingBox() const override;
};

class Mesh : public Shape {
    std::vector<Triangle> triangles;
    BVH<Triangle> bvh;

public:
    Mesh(const std::string &filename);
    Mesh(const Mesh &) = delete;

    std::optional<Intersection> intersect(const Ray &ray) const override;
    BoundingBox getBoundingBox() const override;
};
