#include "object.h"
#include "world.h"

#include <random>

Object::Object(std::unique_ptr<Shape> s, const World *w, Color color) : s(std::move(s)), w(w), color(color) {}

Line Object::getBounceRay(const Line &l, const Intersection &intersection) const {
    glm::dvec3 idealReflection = glm::reflect(l.dir, intersection.surfaceNormal);

    thread_local std::mt19937 gen{std::random_device{}()};
    double roughness = 0.5;
    std::normal_distribution<double> noise{0.0, roughness};

    glm::dvec3 dir = glm::normalize(idealReflection + glm::dvec3{noise(gen), noise(gen), noise(gen)});

    return {
        intersection.point,
        dir,
        l.bounced + 1
    };
}

glm::dvec3 Object::getColor(const Line &l, const Intersection &intersection) const {
    auto bounceRay = getBounceRay(l, intersection);
    return color * w->getColor(bounceRay);
}

std::optional<Intersection> Object::intersect(const Line &l) const {
    return s->intersect(l);
}

BoundingBox Object::getBoundingBox() const {
    return s->getBoundingBox();
}

glm::dvec3 Object::getRawColor() const {
    return color;
}

Light::Light(std::unique_ptr<Shape> s, const World *w, Color color, double intensity)
    : Object(std::move(s), w, color), intensity(intensity) {}

glm::dvec3 Light::getColor(const Line &l, const Intersection &intersection) const {
    return getRawColor() * intensity;
}
