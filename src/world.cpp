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

std::optional<std::tuple<const Object*, Intersection>> World::getHit(const Ray &ray) const {
    return bvh.intersects(ray);
}

glm::dvec3 World::getColor(const Ray &ray, int depth) const {
    if (depth >= bounceLimit) return {0, 0, 0};

    auto hit = getHit(ray);
    if (!hit) return {0, 0, 0};

    auto [object, intersection] = *hit;
    const auto &material = object->getMaterial();

    glm::dvec3 idealReflection = glm::reflect(ray.dir, intersection.surfaceNormal);
    Ray bounce{
        intersection.point,
        object->scatter(idealReflection)
    };

    return material.color * material.emission + material.color * getColor(bounce, depth + 1);
}
