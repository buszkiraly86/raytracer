#include "world.h"

#include <limits>

void World::addObject(std::unique_ptr<Object> object) {
    objects.push_back(std::move(object));

    std::vector<const Object*> pointers;
    std::transform(objects.begin(), objects.end(), std::back_inserter(pointers), [](const auto &object) {
        return object.get();
    });

    bvh = BVH<Object>(std::move(pointers));
}

std::optional<std::tuple<const Object*, Intersection>> World::getHit(const Line &l) const {
    return bvh.intersects(l);
}

glm::dvec3 World::getColor(const Line &l) const {
    if (l.bounced > bounceLimit) return {0, 0, 0};

    auto hit = getHit(l);
    if (!hit) return {0.06, 0.09, 0.2};    // sky: night blue

    auto [o, i] = *hit;

    return o->getColor(l, i);
}
