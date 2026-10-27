#pragma once

#include <memory>
#include <tuple>
#include <vector>
#include <glm/glm.hpp>

#include "geometry.h"
#include "object.h"
#include "bvh.h"

constexpr int bounceLimit = 5;

class World {
    std::vector<std::unique_ptr<Object>> objects;
    BVH<Object> bvh;

public:
    void addObject(std::unique_ptr<Object> object);
    std::optional<std::tuple<const Object*, Intersection>> getHit(const Line &l) const;
    glm::dvec3 getColor(const Line &l) const;
};
