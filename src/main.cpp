#include <print>
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <numbers>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <optional>
#include <vector>
#include <ranges>
#include <memory>
#include <filesystem>
#include <format>
#include <random>

#include "geometry.h"
#include "object.h"
#include "world.h"

constexpr int width = 800;
constexpr int height = 600;
constexpr double FOV = std::numbers::pi / 3;
constexpr double aspectRatio = static_cast<double>(width) / height;
constexpr double halfWidth = aspectRatio / 2;
constexpr double halfHeight = 0.5;
double d = halfWidth / std::tan(FOV / 2);
constexpr double pixelWidth = 1.0 / height;

World world;

/*
    This is a left handed coordinate system
*/

Color colorAt(int px, int py)
{
    thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> offset{-pixelWidth / 2, pixelWidth / 2};

    // night/day scenes:  camera{0, 3, -5},     lookAt{0, 0, 30}
    // knot/mesh scenes:  camera{0, 0.5, -4.5},  lookAt{0, 0, 0}
    glm::dvec3 camera{0, 3, -5};
    glm::dvec3 lookAt{0, 0, 30};

    // TODO:
    // coordinate system generation breaks if it points straight up or down
    glm::dvec3 forward = glm::normalize(lookAt - camera);
    glm::dvec3 right = glm::normalize(glm::cross({0, 1, 0}, forward));
    glm::dvec3 up = glm::normalize(glm::cross(forward, right));

    double x = pixelWidth / 2 + pixelWidth * px - halfWidth + offset(gen);
    double y = pixelWidth / 2 - pixelWidth * py + halfHeight + offset(gen);

    Line l = {
        camera,
        glm::normalize(d * forward + x * right + y * up),
    };
    // Sphere s = {
    //     {1, 0.1, 0.1},
    //     glm::dvec3{0, 2, 10},
    //     2
    // };
    // Plane p = {
    //     {0.1, 0.1, 0.4},
    //     glm::dvec3{0, 0, 0},
    //     glm::dvec3{0, 1, 0}
    // };
    // LightSource skyLight = {
    //     {1, 1, 1},
    //     std::make_unique<Plane>({
    //         {0.1, 0.1, 0.4},
    //         glm::dvec3{0, 0, 0},
    //         glm::dvec3{0, 1, 0}
    //     }),
    //     1
    // };

    return world.getColor(l);
}

int main() {
    // ---------- Christmas scene ----------

    auto sphere = [&](glm::dvec3 pos, double radius, Color color) {
        world.addObject(std::make_unique<Object>(std::make_unique<Sphere>(pos, radius), &world, color));
    };
    auto lightSphere = [&](glm::dvec3 pos, double radius, Color color, double intensity) {
        world.addObject(std::make_unique<Light>(std::make_unique<Sphere>(pos, radius), &world, color, intensity));
    };
    auto triangle = [&](glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, Color color) {
        world.addObject(std::make_unique<Object>(
            std::make_unique<Triangle>(std::array<glm::dvec3, 3>{a, b, c}), &world, color));
    };
    auto lightTriangle = [&](glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, Color color, double intensity) {
        world.addObject(std::make_unique<Light>(
            std::make_unique<Triangle>(std::array<glm::dvec3, 3>{a, b, c}), &world, color, intensity));
    };

    // open cone (sides only) from a base circle to an apex, in any direction
    auto cone = [&](glm::dvec3 base, glm::dvec3 apex, double radius, int segments, Color color) {
        glm::dvec3 axis = glm::normalize(apex - base);
        glm::dvec3 helper = std::abs(axis.y) < 0.9 ? glm::dvec3{0, 1, 0} : glm::dvec3{1, 0, 0};
        glm::dvec3 u = glm::normalize(glm::cross(axis, helper));
        glm::dvec3 v = glm::cross(axis, u);
        for (int i = 0; i < segments; ++i) {
            double a0 = 2 * std::numbers::pi * i / segments;
            double a1 = 2 * std::numbers::pi * (i + 1) / segments;
            glm::dvec3 p0 = base + radius * (std::cos(a0) * u + std::sin(a0) * v);
            glm::dvec3 p1 = base + radius * (std::cos(a1) * u + std::sin(a1) * v);
            triangle(p0, p1, apex, color);
        }
    };

    // axis-aligned box from two opposite corners
    auto box = [&](glm::dvec3 lo, glm::dvec3 hi, Color color) {
        glm::dvec3 c[8] = {
            {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
            {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z},
        };
        int faces[6][4] = {{0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7}, {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}};
        for (auto &f : faces) {
            triangle(c[f[0]], c[f[1]], c[f[2]], color);
            triangle(c[f[0]], c[f[2]], c[f[3]], color);
        }
    };

    // wrapped present with a ribbon cross on top
    auto present = [&](glm::dvec3 lo, glm::dvec3 hi, Color paper, Color ribbon) {
        box(lo, hi, paper);
        double cx = (lo.x + hi.x) / 2, cz = (lo.z + hi.z) / 2, w = 0.06, e = 0.01;
        box({cx - w, lo.y, lo.z - e}, {cx + w, hi.y + e, hi.z + e}, ribbon);
        box({lo.x - e, lo.y, cz - w}, {hi.x + e, hi.y + e, cz + w}, ribbon);
    };

    // the night scene
    auto buildNightScene = [&]() {
        // snowy ground: one huge square made of two triangles
        triangle({-10000, 0, -10000}, {10000, 0, -10000}, {10000, 0, 10000}, Color{0.85, 0.88, 0.95});
        triangle({-10000, 0, -10000}, {10000, 0, 10000}, {-10000, 0, 10000}, Color{0.85, 0.88, 0.95});

        // moon, and a warm fireplace glow just off-screen to the right
        lightSphere({-30, 22, 80}, 4, Color{0.8, 0.85, 1.0}, 4);
        lightSphere({8, 2.5, 5}, 1.5, Color{1.0, 0.55, 0.25}, 8);

        // big dim moonlit glow behind the camera: front-facing surfaces reflect it
        lightSphere({0, 6, -30}, 10, Color{0.5, 0.55, 0.7}, 2.5);

        // street lamp between the snowman and the tree
        box({-2.45, 0, 6.95}, {-2.35, 3.0, 7.05}, Color{0.1, 0.1, 0.12});
        lightSphere({-2.4, 3.2, 7.0}, 0.22, Color{1.0, 0.8, 0.5}, 12);

        // ---------- tree ----------
        const glm::dvec3 treeBase{0.5, 0, 9};
        const Color pine{0.08, 0.38, 0.15};

        struct Tier { double baseY, radius, apexY; };
        const Tier tiers[] = {{0.7, 2.1, 3.0}, {2.0, 1.6, 4.1}, {3.2, 1.1, 5.2}};

        box({0.2, 0, 8.7}, {0.8, 0.9, 9.3}, Color{0.35, 0.2, 0.1}); // trunk
        for (const auto &t : tiers)
            cone({treeBase.x, t.baseY, treeBase.z}, {treeBase.x, t.apexY, treeBase.z}, t.radius, 14, pine);

        // a point on a tier's surface: angle around the tree, fraction of the way up, pushed out a little
        auto onTier = [&](const Tier &t, double angle, double up, double out) {
            double r = t.radius * (1 - up) + out;
            return glm::dvec3{treeBase.x + r * std::cos(angle), t.baseY + up * (t.apexY - t.baseY),
                              treeBase.z + r * std::sin(angle)};
        };

        // ornaments, on the camera-facing side (camera looks along +z, so front is sin(angle) < 0)
        const Color ornamentColors[] = {{0.8, 0.05, 0.05}, {0.9, 0.65, 0.15}, {0.85, 0.85, 0.9}};
        const int ornamentsPerTier[] = {7, 5, 4};
        const double ornamentRadius[] = {0.16, 0.14, 0.12};
        int ornament = 0;
        for (int i = 0; i < 3; ++i) {
            int n = ornamentsPerTier[i];
            for (int k = 0; k < n; ++k) {
                double angle = std::numbers::pi * (1.1 + 0.8 * (k + 0.5) / n);
                double up = (k % 2 == 0) ? 0.22 : 0.5;
                sphere(onTier(tiers[i], angle, up, 0.08), ornamentRadius[i], ornamentColors[ornament++ % 3]);
            }
        }

        // fairy lights spiralling around each tier
        for (int i = 0; i < 3; ++i) {
            const int n = 12;
            for (int k = 0; k < n; ++k) {
                double f = static_cast<double>(k) / n;
                double angle = 2 * std::numbers::pi * f + i * 1.3;
                double up = 0.12 + 0.65 * f;
                lightSphere(onTier(tiers[i], angle, up, 0.12), 0.055, Color{1.0, 0.75, 0.4}, 25);
            }
        }

        // star on top: five points, ten glowing triangles, facing the camera
        {
            glm::dvec3 centre{treeBase.x, 5.55, treeBase.z};
            glm::dvec3 points[10];
            for (int i = 0; i < 10; ++i) {
                double angle = std::numbers::pi / 2 + i * std::numbers::pi / 5;
                double r = (i % 2 == 0) ? 0.45 : 0.18;
                points[i] = centre + glm::dvec3{r * std::cos(angle), r * std::sin(angle), 0};
            }
            for (int i = 0; i < 10; ++i)
                lightTriangle(centre, points[i], points[(i + 1) % 10], Color{1.0, 0.8, 0.3}, 1.6);
        }

        // ---------- presents ----------
        const Color red{0.75, 0.08, 0.1}, blue{0.1, 0.25, 0.7}, gold{0.85, 0.6, 0.15}, silver{0.85, 0.85, 0.9};
        present({-1.9, 0, 7.2}, {-1.1, 0.7, 8.0}, red, gold);
        present({1.6, 0, 7.0}, {2.3, 0.55, 7.7}, blue, silver);
        present({-0.6, 0, 6.6}, {-0.1, 0.45, 7.1}, gold, red);
        present({2.4, 0, 8.2}, {3.1, 0.8, 8.9}, Color{0.1, 0.5, 0.3}, red);

        // ---------- snowman ----------
        {
            const Color snow{0.95, 0.96, 1.0}, coal{0.02, 0.02, 0.02};
            const double x = -4, z = 9;
            sphere({x, 0.85, z}, 0.85, snow);
            sphere({x, 2.0, z}, 0.6, snow);
            sphere({x, 2.85, z}, 0.42, snow);

            sphere({x - 0.15, 2.95, z - 0.38}, 0.05, coal);           // eyes
            sphere({x + 0.15, 2.95, z - 0.38}, 0.05, coal);
            sphere({x, 2.2, z - 0.56}, 0.05, coal);                   // buttons
            sphere({x, 2.0, z - 0.59}, 0.05, coal);
            sphere({x, 1.8, z - 0.56}, 0.05, coal);

            cone({x, 2.83, z - 0.4}, {x, 2.8, z - 0.85}, 0.06, 8, Color{1.0, 0.45, 0.05});   // carrot nose
            cone({x - 0.5, 2.1, z}, {x - 1.3, 2.6, z}, 0.04, 5, Color{0.3, 0.18, 0.08});     // arms
            cone({x + 0.5, 2.1, z}, {x + 1.2, 2.55, z}, 0.04, 5, Color{0.3, 0.18, 0.08});

            cone({x, 3.18, z}, {x + 0.2, 3.85, z + 0.1}, 0.33, 12, Color{0.75, 0.05, 0.08}); // santa hat
            sphere({x + 0.2, 3.85, z + 0.1}, 0.08, snow);                                    // pompom
        }

        // ---------- falling snow ----------
        // spread with irrational-number steps, so it looks random but is the same every run
        for (int k = 0; k < 50; ++k) {
            auto frac = [](double v) { return v - std::floor(v); };
            glm::dvec3 pos{-8 + 16 * frac(k * 0.618034), 0.3 + 7.5 * frac(k * 0.7548777), 3 + 13 * frac(k * 0.5698403)};
            lightSphere(pos, 0.035, Color{0.85, 0.9, 1.0}, 1.5);
        }
    };

    // bright snowy day, everything closer to the camera
    auto buildDayScene = [&]() {
        const Color snowWhite{1.0, 1.0, 1.0};

        // snowy ground: one huge square made of two triangles
        triangle({-10000, 0, -10000}, {10000, 0, -10000}, {10000, 0, 10000}, snowWhite);
        triangle({-10000, 0, -10000}, {10000, 0, 10000}, {-10000, 0, 10000}, snowWhite);

        // low winter sun off-screen top left, and a bright fill behind the camera
        lightSphere({-40, 35, 60}, 8, Color{1.0, 0.95, 0.85}, 6);
        lightSphere({0, 8, -30}, 12, Color{0.95, 0.95, 1.0}, 1.5);
        // big warm-white sun high in front, just above the frame: the snow reflects it
        lightSphere({0, 80, 100}, 25, Color{1.0, 0.95, 0.85}, 3);

        // snow drifts: big spheres mostly buried in the ground
        sphere({-5, -2.2, 9}, 2.8, snowWhite);
        sphere({5.5, -2.5, 10}, 3.2, snowWhite);
        sphere({-1, -3.6, 15}, 4.0, snowWhite);
        sphere({4.2, -1.3, 3}, 1.6, snowWhite);

        // pine forest in the background
        struct Pine { double x, z, height, radius; };
        const Pine forest[] = {{-9, 16, 4.0, 1.4}, {-6, 19, 4.5, 1.5}, {-3.5, 21, 3.5, 1.2},
                               {4, 17, 4.2, 1.4}, {7, 20, 4.6, 1.5}, {10, 18, 3.8, 1.3}};
        for (const auto &p : forest)
            cone({p.x, 0, p.z}, {p.x, p.height, p.z}, p.radius, 8, Color{0.15, 0.4, 0.25});

        // ---------- tree ----------
        const glm::dvec3 treeBase{1.2, 0, 6.5};
        const Color pine{0.07, 0.42, 0.18};

        struct Tier { double baseY, radius, apexY; };
        const Tier tiers[] = {{0.5, 1.7, 2.4}, {1.5, 1.35, 3.3}, {2.5, 0.95, 4.2}};

        box({1.0, 0, 6.3}, {1.4, 0.6, 6.7}, Color{0.35, 0.2, 0.1});  // trunk
        for (const auto &t : tiers) {
            cone({treeBase.x, t.baseY, treeBase.z}, {treeBase.x, t.apexY, treeBase.z}, t.radius, 12, pine);
            // snow cap on the upper part of each tier
            double capBase = t.baseY + 0.55 * (t.apexY - t.baseY);
            cone({treeBase.x, capBase, treeBase.z}, {treeBase.x, t.apexY + 0.03, treeBase.z},
                 0.48 * t.radius, 12, snowWhite);
        }

        auto onTier = [&](const Tier &t, double angle, double up, double out) {
            double r = t.radius * (1 - up) + out;
            return glm::dvec3{treeBase.x + r * std::cos(angle), t.baseY + up * (t.apexY - t.baseY),
                              treeBase.z + r * std::sin(angle)};
        };

        // ornaments on the camera-facing side
        const Color ornamentColors[] = {{0.85, 0.05, 0.05}, {0.95, 0.7, 0.15}, {0.15, 0.3, 0.85}};
        const int ornamentsPerTier[] = {6, 5, 3};
        const double ornamentRadius[] = {0.13, 0.12, 0.1};
        int ornament = 0;
        for (int i = 0; i < 3; ++i) {
            int n = ornamentsPerTier[i];
            for (int k = 0; k < n; ++k) {
                double angle = std::numbers::pi * (1.1 + 0.8 * (k + 0.5) / n);
                double up = (k % 2 == 0) ? 0.2 : 0.45;
                sphere(onTier(tiers[i], angle, up, 0.08), ornamentRadius[i], ornamentColors[ornament++ % 3]);
            }
        }

        // warm fairy lights, dimmer than at night
        const Color bulbColors[] = {{1.0, 0.75, 0.4}, {1.0, 0.25, 0.15}, {1.0, 0.75, 0.4}, {1.0, 0.6, 0.15}};
        int bulb = 0;
        for (int i = 0; i < 3; ++i) {
            const int n = 10;
            for (int k = 0; k < n; ++k) {
                double f = static_cast<double>(k) / n;
                double angle = 2 * std::numbers::pi * f + i * 1.3;
                double up = 0.1 + 0.4 * f;
                lightSphere(onTier(tiers[i], angle, up, 0.1), 0.05, bulbColors[bulb++ % 4], 6);
            }
        }

        // star on top
        {
            glm::dvec3 centre{treeBase.x, 4.45, treeBase.z};
            glm::dvec3 points[10];
            for (int i = 0; i < 10; ++i) {
                double angle = std::numbers::pi / 2 + i * std::numbers::pi / 5;
                double r = (i % 2 == 0) ? 0.35 : 0.14;
                points[i] = centre + glm::dvec3{r * std::cos(angle), r * std::sin(angle), 0};
            }
            for (int i = 0; i < 10; ++i)
                lightTriangle(centre, points[i], points[(i + 1) % 10], Color{1.0, 0.8, 0.3}, 1.0);
        }

        // ---------- presents, up front ----------
        const Color red{0.8, 0.08, 0.1}, green{0.1, 0.5, 0.25}, blue{0.12, 0.3, 0.75},
                    gold{0.9, 0.65, 0.15}, silver{0.85, 0.85, 0.9};
        present({-1.6, 0, 2.6}, {-0.7, 0.75, 3.4}, red, gold);
        present({-1.45, 0.75, 2.75}, {-0.85, 1.15, 3.25}, gold, red);       // stacked on the red one
        present({-0.4, 0, 2.2}, {0.3, 0.5, 2.8}, green, red);
        present({0.6, 0, 3.0}, {1.5, 0.65, 3.8}, blue, silver);

        // ---------- candy cane stuck in the snow ----------
        {
            const double cx = 2.6, cz = 2.6, r = 0.18;
            int k = 0;
            auto stripe = [&](glm::dvec3 pos) {
                sphere(pos, 0.065, (k++ / 2) % 2 == 0 ? Color{0.85, 0.05, 0.08} : snowWhite);
            };
            for (int s = 0; s <= 12; ++s)
                stripe({cx, s * 0.1, cz});
            for (int s = 1; s <= 6; ++s) {
                double angle = std::numbers::pi * s / 6;
                stripe({cx - r + r * std::cos(angle), 1.2 + r * std::sin(angle), cz});
            }
        }

        // ---------- snowman ----------
        {
            const Color coal{0.02, 0.02, 0.02};
            const double x = -3, z = 5;
            sphere({x, 0.8, z}, 0.8, snowWhite);
            sphere({x, 1.85, z}, 0.55, snowWhite);
            sphere({x, 2.62, z}, 0.38, snowWhite);

            sphere({x - 0.13, 2.71, z - 0.34}, 0.045, coal);            // eyes
            sphere({x + 0.13, 2.71, z - 0.34}, 0.045, coal);
            sphere({x, 2.03, z - 0.52}, 0.045, coal);                   // buttons
            sphere({x, 1.85, z - 0.55}, 0.045, coal);
            sphere({x, 1.67, z - 0.52}, 0.045, coal);

            cone({x, 2.6, z - 0.36}, {x, 2.57, z - 0.78}, 0.055, 8, Color{1.0, 0.45, 0.05});  // carrot nose
            cone({x - 0.45, 1.95, z}, {x - 1.2, 2.45, z}, 0.035, 5, Color{0.3, 0.18, 0.08}); // arms
            cone({x + 0.45, 1.95, z}, {x + 1.1, 2.4, z}, 0.035, 5, Color{0.3, 0.18, 0.08});

            cone({x, 2.93, z}, {x + 0.18, 3.55, z + 0.08}, 0.3, 12, Color{0.8, 0.05, 0.08}); // santa hat
            sphere({x + 0.18, 3.55, z + 0.08}, 0.07, snowWhite);                             // pompom
        }

        // ---------- cabin with glowing windows ----------
        {
            const double x0 = -9.5, x1 = -6.8, z0 = 13, z1 = 15.5, wall = 2.2, ridge = 3.4;
            const double xm = (x0 + x1) / 2;
            box({x0, 0, z0}, {x1, wall, z1}, Color{0.45, 0.25, 0.12});         // log walls
            // snow-covered roof: two slopes, overhanging a little, and the two gables
            const double o = 0.25;
            glm::dvec3 ridgeFront{xm, ridge, z0 - o}, ridgeBack{xm, ridge, z1 + o};
            glm::dvec3 leftFront{x0 - o, wall - 0.1, z0 - o}, leftBack{x0 - o, wall - 0.1, z1 + o};
            glm::dvec3 rightFront{x1 + o, wall - 0.1, z0 - o}, rightBack{x1 + o, wall - 0.1, z1 + o};
            triangle(leftFront, ridgeFront, ridgeBack, snowWhite);
            triangle(leftFront, ridgeBack, leftBack, snowWhite);
            triangle(rightFront, ridgeBack, ridgeFront, snowWhite);
            triangle(rightFront, rightBack, ridgeBack, snowWhite);
            triangle({x0, wall, z0}, {x1, wall, z0}, {xm, ridge, z0}, Color{0.45, 0.25, 0.12});
            triangle({x0, wall, z1}, {x1, wall, z1}, {xm, ridge, z1}, Color{0.45, 0.25, 0.12});
            // two warm windows on the front wall, just in front of it
            const Color windowGlow{1.0, 0.65, 0.3};
            for (double wx : {x0 + 0.5, x1 - 1.1}) {
                glm::dvec3 a{wx, 0.9, z0 - 0.01}, b{wx + 0.6, 0.9, z0 - 0.01};
                glm::dvec3 c{wx + 0.6, 1.6, z0 - 0.01}, d{wx, 1.6, z0 - 0.01};
                lightTriangle(a, b, c, windowGlow, 1.3);
                lightTriangle(a, c, d, windowGlow, 1.3);
            }
        }

        // ---------- lanterns on posts ----------
        for (glm::dvec3 base : {glm::dvec3{3.6, 0, 5.5}, glm::dvec3{5.0, 0, 8.0}, glm::dvec3{6.4, 0, 10.5}}) {
            cone(base, base + glm::dvec3{0, 1.6, 0}, 0.05, 5, Color{0.12, 0.1, 0.08});
            lightSphere(base + glm::dvec3{0, 1.74, 0}, 0.14, Color{1.0, 0.7, 0.35}, 1.4);
        }

        // ---------- falling snow ----------
        for (int k = 0; k < 60; ++k) {
            auto frac = [](double v) { return v - std::floor(v); };
            glm::dvec3 pos{-6 + 12 * frac(k * 0.618034), 0.3 + 5.7 * frac(k * 0.7548777), 1.5 + 12.5 * frac(k * 0.5698403)};
            sphere(pos, 0.045, Color{1.0, 1.0, 1.0});
        }
    };

    // just the trefoil knot, lit by lights that are all outside the frame
    auto buildKnotScene = [&]() {
        // models/ sits next to src/, so find it from this source file's path (works from any working directory)
        auto knotFile = std::filesystem::path(__FILE__).parent_path().parent_path() / "models" / "trefoil_knot.obj";
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(knotFile.string()), &world,
                                                 Color{0.95, 0.65, 0.3}));    // copper

        lightSphere({-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 4);      // warm key light, front left
        lightSphere({9, 6, 6}, 2, Color{0.6, 0.7, 1.0}, 4);          // cool rim light, behind right
        lightSphere({0, 6, -30}, 10, Color{0.9, 0.9, 1.0}, 1.5);     // soft fill behind the camera
    };

    // the seashell: same lights as the knot scene
    auto buildShellScene = [&]() {
        auto shellFile = std::filesystem::path(__FILE__).parent_path().parent_path() / "models" / "seashell.obj";
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(shellFile.string()), &world,
                                                 Color{0.95, 0.82, 0.75}));   // pearly pink

        lightSphere({-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 4);      // warm key light, front left
        lightSphere({9, 6, 6}, 2, Color{0.6, 0.7, 1.0}, 4);          // cool rim light, behind right
        lightSphere({0, 6, -30}, 10, Color{0.9, 0.9, 1.0}, 1.5);     // soft fill behind the camera
    };

    // the copper knot floating inside the silver star ring, same lights as the knot scene
    auto buildKnotRingScene = [&]() {
        auto models = std::filesystem::path(__FILE__).parent_path().parent_path() / "models";
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>((models / "trefoil_knot.obj").string()),
                                                 &world, Color{0.95, 0.65, 0.3}));    // copper
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>((models / "star_ring.obj").string()),
                                                 &world, Color{0.85, 0.87, 0.92}));   // silver

        lightSphere({-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 4);      // warm key light, front left
        lightSphere({9, 6, 6}, 2, Color{0.6, 0.7, 1.0}, 4);          // cool rim light, behind right
        lightSphere({0, 6, -30}, 10, Color{0.9, 0.9, 1.0}, 1.5);     // soft fill behind the camera
    };

    // Quake-style courtyard: tiled hall, lava channel with a bridge, stairs up to a slipgate,
    // the ring-and-nail emblem on the back wall, torches on the pillars (models/quake, about 6800 triangles)
    auto buildQuakeScene = [&]() {
        auto dir = std::filesystem::path(__FILE__).parent_path().parent_path() / "models" / "quake";
        auto mesh = [&](const char *name, Color color) {
            world.addObject(std::make_unique<Object>(std::make_unique<Mesh>((dir / name).string()), &world, color));
        };
        auto glowingMesh = [&](const char *name, Color color, double intensity) {
            world.addObject(std::make_unique<Light>(std::make_unique<Mesh>((dir / name).string()), &world, color, intensity));
        };

        mesh("stone.obj",    Color{0.50, 0.42, 0.34});        // stone structure
        mesh("blocks_a.obj", Color{0.70, 0.58, 0.45});        // tiles and bricks in three shades
        mesh("blocks_b.obj", Color{0.62, 0.50, 0.39});
        mesh("blocks_c.obj", Color{0.78, 0.64, 0.48});
        mesh("metal.obj",    Color{0.55, 0.52, 0.50});
        mesh("wood.obj",     Color{0.50, 0.33, 0.16});

        glowingMesh("lava.obj",   Color{1.0, 0.35, 0.05}, 2.5);
        glowingMesh("emblem.obj", Color{1.0, 0.3, 0.08}, 1.3);
        glowingMesh("portal.obj", Color{0.45, 0.35, 1.0}, 1.8);

        // torch flames: on the inner faces of the pillars, and two on the back wall
        const glm::dvec3 torches[] = {
            {-5.18, 1.58, -2}, {-5.18, 1.58, 3}, {-5.18, 1.58, 12.5}, {-5.18, 3.98, 23}, {-5.18, 3.98, 28},
            { 5.18, 1.58, -2}, { 5.18, 1.58, 3}, { 5.18, 1.58, 12.5}, { 5.18, 3.98, 23}, { 5.18, 3.98, 28},
            {-3.6, 3.98, 29.55}, {3.6, 3.98, 29.55},
        };
        for (const auto &t : torches)
            lightSphere(t, 0.28, Color{1.0, 0.6, 0.25}, 6);

        // moon high above the courtyard (out of view) and a dim warm glow behind the camera
        lightSphere({-25, 45, 50}, 10, Color{0.75, 0.8, 1.0}, 3);
        lightSphere({0, 5, -25}, 8, Color{1.0, 0.75, 0.5}, 1.5);
    };

    // the knot and ring as a Quake-style artifact: hung on chains above an altar with a glowing
    // summoning circle, iron spikes, fire braziers on obelisks, and a lava lake to the horizon (models/shrine)
    auto buildShrineScene = [&]() {
        auto models = std::filesystem::path(__FILE__).parent_path().parent_path() / "models";
        auto mesh = [&](std::filesystem::path file, Color color) {
            world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(file.string()), &world, color));
        };
        auto glowingMesh = [&](std::filesystem::path file, Color color, double intensity) {
            world.addObject(std::make_unique<Light>(std::make_unique<Mesh>(file.string()), &world, color, intensity));
        };

        mesh(models / "trefoil_knot.obj",     Color{0.8, 0.5, 0.25});      // bronze
        mesh(models / "star_ring.obj",        Color{0.5, 0.5, 0.53});      // dark iron
        mesh(models / "shrine" / "stone.obj", Color{0.45, 0.4, 0.36});
        mesh(models / "shrine" / "iron.obj",  Color{0.4, 0.38, 0.36});
        glowingMesh(models / "shrine" / "rune.obj", Color{1.0, 0.15, 0.05}, 2.0);
        glowingMesh(models / "shrine" / "lava.obj", Color{0.9, 0.22, 0.03}, 0.7);

        // brazier fires on top of the obelisks
        for (glm::dvec3 fire : {glm::dvec3{-3.6, 2.0, 4.5}, glm::dvec3{3.6, 2.0, 4.5},
                                glm::dvec3{-6.5, 2.0, 10.0}, glm::dvec3{6.5, 2.0, 10.0}})
            lightSphere(fire, 0.25, Color{1.0, 0.5, 0.15}, 4);

        lightSphere({-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 2);      // warm key light, front left (out of view)
        lightSphere({0, 6, -30}, 10, Color{0.9, 0.85, 0.8}, 1.0);    // dim fill behind the camera
    };

    // armillary: the chrome knot inside the gunmetal star ring, with two thin glowing orbits
    // (cyan and amber) sweeping around them as the main lights, over a near-black glossy floor
    auto buildArmillaryScene = [&]() {
        auto models = std::filesystem::path(__FILE__).parent_path().parent_path() / "models";
        auto mesh = [&](std::filesystem::path file, Color color) {
            world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(file.string()), &world, color));
        };
        auto glowingMesh = [&](std::filesystem::path file, Color color, double intensity) {
            world.addObject(std::make_unique<Light>(std::make_unique<Mesh>(file.string()), &world, color, intensity));
        };

        mesh(models / "trefoil_knot.obj", Color{0.92, 0.92, 0.95});        // chrome
        mesh(models / "star_ring.obj",    Color{0.6, 0.6, 0.64});          // brushed steel
        glowingMesh(models / "armillary" / "orbit_inner.obj", Color{0.25, 0.75, 1.0}, 1.2); // cyan
        glowingMesh(models / "armillary" / "orbit_outer.obj", Color{1.0, 0.55, 0.2}, 1.2);  // amber

        // near-black glossy floor: one huge square made of two triangles
        triangle({-1000, -2.1, -1000}, {1000, -2.1, -1000}, {1000, -2.1, 1000}, Color{0.1, 0.1, 0.12});
        triangle({-1000, -2.1, -1000}, {1000, -2.1, 1000}, {-1000, -2.1, 1000}, Color{0.1, 0.1, 0.12});

        // big soft lights out of view, matching the orbit colours: cyan from the left, amber from the right
        lightSphere({-6, 2, -1}, 2.5, Color{0.25, 0.75, 1.0}, 6);
        lightSphere({6, 1, 1}, 2.5, Color{1.0, 0.55, 0.2}, 6);
        lightSphere({0, 12, 2}, 4, Color{1.0, 0.95, 0.9}, 1.0);              // soft white light high above
    };

    // buildDayScene();
    buildNightScene();
    // buildKnotScene();
    // buildShellScene();
    // buildKnotRingScene();
    // buildQuakeScene();
    // buildShrineScene();
    // buildArmillaryScene();

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Failed to initialize the SDL2 library\n";
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Window",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          width, height,
                                          0);

    if(!window)
    {
        std::cout << "Failed to create window\n";
        return -1;
    }

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    if(!window_surface)
    {
        std::cout << "Failed to get the surface from the window\n";
        return -1;
    }

    std::vector<Color> accum(width * height, Color{0});

    bool running = true;
    long long frame{};

    // frame rate shown in the window title, refreshed every half second
    auto fpsTimer = std::chrono::steady_clock::now();
    int framesSinceUpdate = 0;
    while(running)
    {
        frame++;

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
                running = false;
        }

        SDL_LockSurface(window_surface);
        auto cores = std::thread::hardware_concurrency();
        int heightPerCore = height / cores;
        std::vector<std::jthread> threads;
        for (int i = 0; i < cores; ++i) {
            threads.push_back(
                std::jthread([&](int i){
                    int end = (i == cores - 1) ? height : (i + 1) * heightPerCore;
                    for (int y = i * heightPerCore; y < end; ++y)
                    {
                        Uint8 *row = static_cast<Uint8 *>(window_surface->pixels) + y * window_surface->pitch;
                        Uint32 *pixels = reinterpret_cast<Uint32 *>(row);

                        for(int x = 0; x < width; ++x)
                        {
                            Color &sum = accum[y * width + x];
                            sum += colorAt(x, y);
                            Color avgColor = sum / static_cast<double>(frame);
                            Color capped = glm::clamp(avgColor, 0.0, 1.0);
                            pixels[x] = SDL_MapRGB(window_surface->format, sqrt(capped.r) * 255, sqrt(capped.g) * 255, sqrt(capped.b) * 255);
                        }
                    }
                }, i)
            );
        }

        threads.clear();

        SDL_UnlockSurface(window_surface);

        SDL_UpdateWindowSurface(window);

        ++framesSinceUpdate;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - fpsTimer).count();
        if (elapsed >= 0.5) {
            double fps = framesSinceUpdate / elapsed;
            std::string title = std::format("raytracer  |  {:.1f} fps  |  {:.0f} ms/frame  |  {} samples",
                                            fps, 1000.0 / fps, frame);
            SDL_SetWindowTitle(window, title.c_str());
            fpsTimer = now;
            framesSinceUpdate = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}