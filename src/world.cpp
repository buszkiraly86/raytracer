#include "world.h"

#include <limits>

void World::addObject(std::unique_ptr<Object> object) {
    objects.push_back(std::move(object));
}

std::tuple<Object*, Intersection> World::getHitObject(const Line &l) const {
    Object *closestHit = nullptr;
    Intersection hitIntersection{};
    double minDistance = std::numeric_limits<double>::infinity();

    for (const auto &object : objects) {
        auto intersection = object->getIntersection(l);
        if (!intersection) continue;

        auto &[point, surfaceNormal] = *intersection;

        auto distance = glm::distance(l.point, point);
        if (distance < minDistance) {
            minDistance = distance;
            closestHit = object.get();
            hitIntersection = *intersection;
        }
    }

    return {
        closestHit,
        hitIntersection
    };
}

glm::dvec3 World::getColor(const Line &l) const {
    if (l.bounced > bounceLimit) return {0, 0, 0};

    auto [hit, intersection] = getHitObject(l);
    if (!hit) return {0.06, 0.09, 0.2};    // sky: night blue

    return hit->getColor(l, intersection);
}
