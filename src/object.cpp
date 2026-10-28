#include "object.h"
#include "world.h"

#include <random>

Object::Object(std::unique_ptr<Shape> s, const World *w, Material material) : s(std::move(s)), w(w), material(material) {}

glm::dvec3 Object::getColor(const Line &l, const Intersection &intersection) const {
    glm::dvec3 idealReflection = glm::reflect(l.dir, intersection.surfaceNormal);
    auto [reflection, color] = material.scatter(idealReflection);

    Line bounceRay = {
        intersection.point,
        reflection,
        l.bounced + 1
    };

    return material.color * material.emission + color * w->getColor(bounceRay);
}

std::optional<Intersection> Object::intersect(const Line &l) const {
    return s->intersect(l);
}

BoundingBox Object::getBoundingBox() const {
    return s->getBoundingBox();
}
