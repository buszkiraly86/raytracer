#include "object.h"
#include "world.h"

#include <random>

Object::Object(std::unique_ptr<Shape> s, Material material) : s(std::move(s)), material(material) {}

std::optional<Intersection> Object::intersect(const Ray &ray) const {
    return s->intersect(ray);
}

BoundingBox Object::getBoundingBox() const {
    return s->getBoundingBox();
}

glm::dvec3 Object::scatter(const glm::dvec3 &idealReflection) const {
    return material.scatter(idealReflection);
}
