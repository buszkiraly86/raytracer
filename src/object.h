#pragma once

#include <memory>
#include <optional>
#include <glm/glm.hpp>

#include "geometry.h"
#include "material.h"

class Object {
    std::unique_ptr<Shape> s;
    Material material;

public:
    Object(std::unique_ptr<Shape> s, Material material);

    std::optional<Intersection> intersect(const Ray &ray) const;
    BoundingBox getBoundingBox() const;
    glm::dvec3 scatter(const glm::dvec3 &idealReflection) const;
    const Material& getMaterial() const { return material; }
};
