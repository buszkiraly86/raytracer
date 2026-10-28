#pragma once

#include <memory>
#include <optional>
#include <glm/glm.hpp>

#include "geometry.h"
#include "material.h"

class World;

class Object {
    std::unique_ptr<Shape> s;
    const World *w;
    Material material;

public:
    Object(std::unique_ptr<Shape> s, const World *w, Material material);
    virtual glm::dvec3 getColor(const Ray &ray, const Intersection &intersection) const;

    std::optional<Intersection> intersect(const Ray &ray) const;
    BoundingBox getBoundingBox() const;
};
