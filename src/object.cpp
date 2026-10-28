#include "object.h"
#include "world.h"

#include <random>

Object::Object(std::unique_ptr<Shape> s, const World *w, Material material) : s(std::move(s)), w(w), material(material) {}

glm::dvec3 Object::getColor(const Ray &ray, const Intersection &intersection) const {
    glm::dvec3 idealReflection = glm::reflect(ray.dir, intersection.surfaceNormal);
    auto [reflection, color] = material.scatter(idealReflection);

    Ray bounceRay = {
        intersection.point,
        reflection,
        ray.bounced + 1
    };

    return material.color * material.emission + color * w->getColor(bounceRay);
}

std::optional<Intersection> Object::intersect(const Ray &ray) const {
    return s->intersect(ray);
}

BoundingBox Object::getBoundingBox() const {
    return s->getBoundingBox();
}
