#pragma once

#include <glm/glm.hpp>

// small value types shared by the geometry and the BVH

constexpr double epsilon = 1e-6;

using Color = glm::dvec3;

struct Ray {
    glm::dvec3 point;
    glm::dvec3 dir;
    int bounced{};
};

struct Intersection {
    glm::dvec3 point;
    glm::dvec3 surfaceNormal;
};

struct BoundingBox {
    glm::dvec3 A, B;
};
