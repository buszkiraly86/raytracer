#pragma once

#include <memory>
#include <tuple>
#include <vector>
#include <glm/glm.hpp>

#include "geometry.h"
#include "object.h"

constexpr int bounceLimit = 5;

class World {
    std::vector<std::unique_ptr<Object>> objects;

public:
    void addObject(std::unique_ptr<Object> object);
    std::tuple<Object*, Intersection> getHitObject(const Line &l) const;
    glm::dvec3 getColor(const Line &l) const;
};
