#pragma once

#include <random>
#include <tuple>
#include <glm/glm.hpp>

struct Material {
    glm::dvec3 color;
    double roughness{};
    double emission{};

    // the direction of the bounce: the ideal reflection, spread by roughness
    glm::dvec3 scatter(const glm::dvec3 &l) const {
        thread_local std::mt19937 gen{std::random_device{}()};
        std::normal_distribution<double> noise{0.0, roughness};

        return glm::normalize(l + glm::dvec3{noise(gen), noise(gen), noise(gen)});
    }
};
