#pragma once

#include <memory>
#include <optional>
#include <glm/glm.hpp>

#include "geometry.h"

class World;

class Object {
    std::unique_ptr<Shape> s;
    const World *w;
    Color color;

    Line getBounceRay(const Line &l, const Intersection &intersection) const;

public:
    Object(std::unique_ptr<Shape> s, const World *w, Color color);
    virtual glm::dvec3 getColor(const Line &l, const Intersection &intersection) const;

    std::optional<Intersection> getIntersection(const Line &l) const;
    glm::dvec3 getRawColor() const;
};

class Light : public Object {
    double intensity{};

public:
    Light(std::unique_ptr<Shape> s, const World *w, Color color, double intensity);

    glm::dvec3 getColor(const Line &l, const Intersection &intersection) const override;
};
