#include "scene.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numbers>
#include <utility>
#include <glm/glm.hpp>

#include "geometry.h"
#include "material.h"
#include "object.h"

namespace {

// the look every surface had before materials: roughness 0.5, no emission;
// lights are the same with emission set to their old intensity
Material glossy(Color color, double emission = 0) {
    return Material{color, 0.5, emission};
}

void sphere(World &world, glm::dvec3 pos, double radius, Color color, double roughness = 0.5) {
    world.addObject(std::make_unique<Object>(std::make_unique<Sphere>(pos, radius), Material{color, roughness, 0}));
}
void lightSphere(World &world, glm::dvec3 pos, double radius, Color color, double intensity) {
    world.addObject(std::make_unique<Object>(std::make_unique<Sphere>(pos, radius), glossy(color, intensity)));
}
void triangle(World &world, glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, Color color, double roughness = 0.5) {
    world.addObject(std::make_unique<Object>(
        std::make_unique<Triangle>(std::array<glm::dvec3, 3>{a, b, c}), Material{color, roughness, 0}));
}
void lightTriangle(World &world, glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, Color color, double intensity) {
    world.addObject(std::make_unique<Object>(
        std::make_unique<Triangle>(std::array<glm::dvec3, 3>{a, b, c}), glossy(color, intensity)));
}

// open cone (sides only) from a base circle to an apex, in any direction
void cone(World &world, glm::dvec3 base, glm::dvec3 apex, double radius, int segments, Color color, double roughness = 0.5) {
    glm::dvec3 axis = glm::normalize(apex - base);
    glm::dvec3 helper = std::abs(axis.y) < 0.9 ? glm::dvec3{0, 1, 0} : glm::dvec3{1, 0, 0};
    glm::dvec3 u = glm::normalize(glm::cross(axis, helper));
    glm::dvec3 v = glm::cross(axis, u);
    for (int i = 0; i < segments; ++i) {
        double a0 = 2 * std::numbers::pi * i / segments;
        double a1 = 2 * std::numbers::pi * (i + 1) / segments;
        glm::dvec3 p0 = base + radius * (std::cos(a0) * u + std::sin(a0) * v);
        glm::dvec3 p1 = base + radius * (std::cos(a1) * u + std::sin(a1) * v);
        triangle(world, p0, p1, apex, color, roughness);
    }
}

// axis-aligned box from two opposite corners
void box(World &world, glm::dvec3 lo, glm::dvec3 hi, Color color, double roughness = 0.5) {
    glm::dvec3 c[8] = {
        {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
        {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z},
    };
    int faces[6][4] = {{0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7}, {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}};
    for (auto &f : faces) {
        triangle(world, c[f[0]], c[f[1]], c[f[2]], color, roughness);
        triangle(world, c[f[0]], c[f[2]], c[f[3]], color, roughness);
    }
}

// wrapped present with a ribbon cross on top
void present(World &world, glm::dvec3 lo, glm::dvec3 hi, Color paper, Color ribbon,
                   double paperRoughness = 0.5, double ribbonRoughness = 0.5) {
    box(world, lo, hi, paper, paperRoughness);
    double cx = (lo.x + hi.x) / 2, cz = (lo.z + hi.z) / 2, w = 0.06, e = 0.01;
    box(world, {cx - w, lo.y, lo.z - e}, {cx + w, hi.y + e, hi.z + e}, ribbon, ribbonRoughness);
    box(world, {lo.x - e, lo.y, cz - w}, {hi.x + e, hi.y + e, cz + w}, ribbon, ribbonRoughness);
}

} // namespace

// test scene for materials: a checkerboard floor, a row of identical silver spheres from sharp to blurry
// (roughness 0.01 .. 0.4, left to right), and red/green/blue blocks behind them to reflect
Scene buildTestScene() {
    World world;

    // checkerboard floor, slightly glossy so it shows blurred reflections of the spheres
    for (int ix = -5; ix < 5; ++ix) {
        for (int iz = 0; iz < 12; ++iz) {
            Color c = (ix + iz) % 2 == 0 ? Color{0.8, 0.8, 0.8} : Color{0.25, 0.25, 0.25};
            glm::dvec3 a{ix, 0, iz}, b{ix + 1, 0, iz}, d{ix, 0, iz + 1}, e{ix + 1, 0, iz + 1};
            triangle(world, a, b, e, c, 0.3);
            triangle(world, a, e, d, c, 0.3);
        }
    }

    // the roughness row
    const double roughness[] = {0.01, 0.05, 0.1, 0.2, 0.4};
    for (int k = 0; k < 5; ++k)
        sphere(world, {-3.0 + 1.5 * k, 0.6, 6}, 0.6, Color{0.9, 0.9, 0.92}, roughness[k]);

    // coloured blocks behind the row, for the spheres to reflect
    box(world, {-4.1, 0, 9.5}, {-2.9, 2.5, 10.1}, Color{0.85, 0.1, 0.1}, 0.9);
    box(world, {-0.6, 0, 9.5}, {0.6, 2.5, 10.1}, Color{0.1, 0.75, 0.15}, 0.9);
    box(world, {2.9, 0, 9.5}, {4.1, 2.5, 10.1}, Color{0.1, 0.25, 0.85}, 0.9);

    lightSphere(world, {0, 12, 6}, 3, Color{1.0, 1.0, 1.0}, 3);          // big soft light overhead
    lightSphere(world, {-8, 6, 2}, 1.5, Color{1.0, 0.85, 0.65}, 6);      // warm key light, front left

    // ribbon lights along the back edge of the board, just behind the blocks: tall vertical strips spread
    // across the board's width, each gap as wide as a strip (12 strips + 11 gaps = 23 widths = 10 units),
    // colours running through the rainbow twice
    const Color ribbonColors[] = {{1.0, 0.2, 0.2}, {1.0, 0.6, 0.1}, {0.9, 0.9, 0.2},
                                  {0.2, 1.0, 0.3}, {0.2, 0.6, 1.0}, {0.8, 0.3, 1.0}};
    const int strips = 12;
    const double ribbon = 10.0 / (2 * strips - 1);
    for (int k = 0; k < strips; ++k) {
        double x0 = -5 + 2 * ribbon * k, x1 = x0 + ribbon;
        glm::dvec3 a{x0, 0, 12}, b{x1, 0, 12}, c{x1, 8, 12}, d{x0, 8, 12};
        lightTriangle(world, a, b, c, ribbonColors[k % 6], 1.2);
        lightTriangle(world, a, c, d, ribbonColors[k % 6], 1.2);
    }

    return {std::move(world), Camera{{0, 3.5, -1}, {0, 1, 8}}};
}

// bright snowy day, everything closer to the camera
Scene buildDayScene() {
    World world;

    const Color snowWhite{1.0, 1.0, 1.0};

    // snowy ground: one huge square made of two triangles
    triangle(world, {-10000, 0, -10000}, {10000, 0, -10000}, {10000, 0, 10000}, snowWhite);
    triangle(world, {-10000, 0, -10000}, {10000, 0, 10000}, {-10000, 0, 10000}, snowWhite);

    // low winter sun off-screen top left, and a bright fill behind the camera
    lightSphere(world, {-40, 35, 60}, 8, Color{1.0, 0.95, 0.85}, 6);
    lightSphere(world, {0, 8, -30}, 12, Color{0.95, 0.95, 1.0}, 1.5);
    // big warm-white sun high in front, just above the frame: the snow reflects it
    lightSphere(world, {0, 80, 100}, 25, Color{1.0, 0.95, 0.85}, 3);

    // snow drifts: big spheres mostly buried in the ground
    sphere(world, {-5, -2.2, 9}, 2.8, snowWhite);
    sphere(world, {5.5, -2.5, 10}, 3.2, snowWhite);
    sphere(world, {-1, -3.6, 15}, 4.0, snowWhite);
    sphere(world, {4.2, -1.3, 3}, 1.6, snowWhite);

    // pine forest in the background
    struct Pine { double x, z, height, radius; };
    const Pine forest[] = {{-9, 16, 4.0, 1.4}, {-6, 19, 4.5, 1.5}, {-3.5, 21, 3.5, 1.2},
                           {4, 17, 4.2, 1.4}, {7, 20, 4.6, 1.5}, {10, 18, 3.8, 1.3}};
    for (const auto &p : forest)
        cone(world, {p.x, 0, p.z}, {p.x, p.height, p.z}, p.radius, 8, Color{0.15, 0.4, 0.25});

    // ---------- tree ----------
    const glm::dvec3 treeBase{1.2, 0, 6.5};
    const Color pine{0.07, 0.42, 0.18};

    struct Tier { double baseY, radius, apexY; };
    const Tier tiers[] = {{0.5, 1.7, 2.4}, {1.5, 1.35, 3.3}, {2.5, 0.95, 4.2}};

    box(world, {1.0, 0, 6.3}, {1.4, 0.6, 6.7}, Color{0.35, 0.2, 0.1});  // trunk
    for (const auto &t : tiers) {
        cone(world, {treeBase.x, t.baseY, treeBase.z}, {treeBase.x, t.apexY, treeBase.z}, t.radius, 12, pine);
        // snow cap on the upper part of each tier
        double capBase = t.baseY + 0.55 * (t.apexY - t.baseY);
        cone(world, {treeBase.x, capBase, treeBase.z}, {treeBase.x, t.apexY + 0.03, treeBase.z},
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
            sphere(world, onTier(tiers[i], angle, up, 0.08), ornamentRadius[i], ornamentColors[ornament++ % 3]);
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
            lightSphere(world, onTier(tiers[i], angle, up, 0.1), 0.05, bulbColors[bulb++ % 4], 6);
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
            lightTriangle(world, centre, points[i], points[(i + 1) % 10], Color{1.0, 0.8, 0.3}, 1.0);
    }

    // ---------- presents, up front ----------
    const Color red{0.8, 0.08, 0.1}, green{0.1, 0.5, 0.25}, blue{0.12, 0.3, 0.75},
                gold{0.9, 0.65, 0.15}, silver{0.85, 0.85, 0.9};
    present(world, {-1.6, 0, 2.6}, {-0.7, 0.75, 3.4}, red, gold);
    present(world, {-1.45, 0.75, 2.75}, {-0.85, 1.15, 3.25}, gold, red);       // stacked on the red one
    present(world, {-0.4, 0, 2.2}, {0.3, 0.5, 2.8}, green, red);
    present(world, {0.6, 0, 3.0}, {1.5, 0.65, 3.8}, blue, silver);

    // ---------- candy cane stuck in the snow ----------
    {
        const double cx = 2.6, cz = 2.6, r = 0.18;
        int k = 0;
        auto stripe = [&](glm::dvec3 pos) {
            sphere(world, pos, 0.065, (k++ / 2) % 2 == 0 ? Color{0.85, 0.05, 0.08} : snowWhite);
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
        sphere(world, {x, 0.8, z}, 0.8, snowWhite);
        sphere(world, {x, 1.85, z}, 0.55, snowWhite);
        sphere(world, {x, 2.62, z}, 0.38, snowWhite);

        sphere(world, {x - 0.13, 2.71, z - 0.34}, 0.045, coal);            // eyes
        sphere(world, {x + 0.13, 2.71, z - 0.34}, 0.045, coal);
        sphere(world, {x, 2.03, z - 0.52}, 0.045, coal);                   // buttons
        sphere(world, {x, 1.85, z - 0.55}, 0.045, coal);
        sphere(world, {x, 1.67, z - 0.52}, 0.045, coal);

        cone(world, {x, 2.6, z - 0.36}, {x, 2.57, z - 0.78}, 0.055, 8, Color{1.0, 0.45, 0.05});  // carrot nose
        cone(world, {x - 0.45, 1.95, z}, {x - 1.2, 2.45, z}, 0.035, 5, Color{0.3, 0.18, 0.08}); // arms
        cone(world, {x + 0.45, 1.95, z}, {x + 1.1, 2.4, z}, 0.035, 5, Color{0.3, 0.18, 0.08});

        cone(world, {x, 2.93, z}, {x + 0.18, 3.55, z + 0.08}, 0.3, 12, Color{0.8, 0.05, 0.08}); // santa hat
        sphere(world, {x + 0.18, 3.55, z + 0.08}, 0.07, snowWhite);                             // pompom
    }

    // ---------- cabin with glowing windows ----------
    {
        const double x0 = -9.5, x1 = -6.8, z0 = 13, z1 = 15.5, wall = 2.2, ridge = 3.4;
        const double xm = (x0 + x1) / 2;
        box(world, {x0, 0, z0}, {x1, wall, z1}, Color{0.45, 0.25, 0.12});         // log walls
        // snow-covered roof: two slopes, overhanging a little, and the two gables
        const double o = 0.25;
        glm::dvec3 ridgeFront{xm, ridge, z0 - o}, ridgeBack{xm, ridge, z1 + o};
        glm::dvec3 leftFront{x0 - o, wall - 0.1, z0 - o}, leftBack{x0 - o, wall - 0.1, z1 + o};
        glm::dvec3 rightFront{x1 + o, wall - 0.1, z0 - o}, rightBack{x1 + o, wall - 0.1, z1 + o};
        triangle(world, leftFront, ridgeFront, ridgeBack, snowWhite);
        triangle(world, leftFront, ridgeBack, leftBack, snowWhite);
        triangle(world, rightFront, ridgeBack, ridgeFront, snowWhite);
        triangle(world, rightFront, rightBack, ridgeBack, snowWhite);
        triangle(world, {x0, wall, z0}, {x1, wall, z0}, {xm, ridge, z0}, Color{0.45, 0.25, 0.12});
        triangle(world, {x0, wall, z1}, {x1, wall, z1}, {xm, ridge, z1}, Color{0.45, 0.25, 0.12});
        // two warm windows on the front wall, just in front of it
        const Color windowGlow{1.0, 0.65, 0.3};
        for (double wx : {x0 + 0.5, x1 - 1.1}) {
            glm::dvec3 a{wx, 0.9, z0 - 0.01}, b{wx + 0.6, 0.9, z0 - 0.01};
            glm::dvec3 c{wx + 0.6, 1.6, z0 - 0.01}, d{wx, 1.6, z0 - 0.01};
            lightTriangle(world, a, b, c, windowGlow, 1.3);
            lightTriangle(world, a, c, d, windowGlow, 1.3);
        }
    }

    // ---------- lanterns on posts ----------
    for (glm::dvec3 base : {glm::dvec3{3.6, 0, 5.5}, glm::dvec3{5.0, 0, 8.0}, glm::dvec3{6.4, 0, 10.5}}) {
        cone(world, base, base + glm::dvec3{0, 1.6, 0}, 0.05, 5, Color{0.12, 0.1, 0.08});
        lightSphere(world, base + glm::dvec3{0, 1.74, 0}, 0.14, Color{1.0, 0.7, 0.35}, 1.4);
    }

    // ---------- falling snow ----------
    for (int k = 0; k < 60; ++k) {
        auto frac = [](double v) { return v - std::floor(v); };
        glm::dvec3 pos{-6 + 12 * frac(k * 0.618034), 0.3 + 5.7 * frac(k * 0.7548777), 1.5 + 12.5 * frac(k * 0.5698403)};
        sphere(world, pos, 0.045, Color{1.0, 1.0, 1.0});
    }

    return {std::move(world), Camera{{0, 3, -5}, {0, 0, 30}}};
}

// just the trefoil knot, lit by lights that are all outside the frame
Scene buildKnotScene() {
    World world;

    // models/ sits next to src/, so find it from this source file's path (works from any working directory)
    auto knotFile = std::filesystem::path(__FILE__).parent_path().parent_path() / "models" / "trefoil_knot.obj";
    world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(knotFile.string()),
                                             glossy(Color{0.95, 0.65, 0.3})));    // copper

    lightSphere(world, {-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 4);      // warm key light, front left
    lightSphere(world, {9, 6, 6}, 2, Color{0.6, 0.7, 1.0}, 4);          // cool rim light, behind right
    lightSphere(world, {0, 6, -30}, 10, Color{0.9, 0.9, 1.0}, 1.5);     // soft fill behind the camera

    return {std::move(world), Camera{{0, 0.5, -4.5}, {0, 0, 0}}};
}

// the seashell: same lights as the knot scene
Scene buildShellScene() {
    World world;

    auto shellFile = std::filesystem::path(__FILE__).parent_path().parent_path() / "models" / "seashell.obj";
    world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(shellFile.string()),
                                             glossy(Color{0.95, 0.82, 0.75})));   // pearly pink

    lightSphere(world, {-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 4);      // warm key light, front left
    lightSphere(world, {9, 6, 6}, 2, Color{0.6, 0.7, 1.0}, 4);          // cool rim light, behind right
    lightSphere(world, {0, 6, -30}, 10, Color{0.9, 0.9, 1.0}, 1.5);     // soft fill behind the camera

    return {std::move(world), Camera{{0, 0.5, -4.5}, {0, 0, 0}}};
}

// the copper knot floating inside the silver star ring, same lights as the knot scene
Scene buildKnotRingScene() {
    World world;

    auto models = std::filesystem::path(__FILE__).parent_path().parent_path() / "models";
    world.addObject(std::make_unique<Object>(std::make_unique<Mesh>((models / "trefoil_knot.obj").string()),
                                             glossy(Color{0.95, 0.65, 0.3})));    // copper
    world.addObject(std::make_unique<Object>(std::make_unique<Mesh>((models / "star_ring.obj").string()),
                                             glossy(Color{0.85, 0.87, 0.92})));   // silver

    lightSphere(world, {-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 4);      // warm key light, front left
    lightSphere(world, {9, 6, 6}, 2, Color{0.6, 0.7, 1.0}, 4);          // cool rim light, behind right
    lightSphere(world, {0, 6, -30}, 10, Color{0.9, 0.9, 1.0}, 1.5);     // soft fill behind the camera

    return {std::move(world), Camera{{0, 0.5, -4.5}, {0, 0, 0}}};
}

// the knot and ring as a Quake-style artifact: hung on chains above an altar with a glowing
// summoning circle, iron spikes, fire braziers on obelisks, and a lava lake to the horizon (models/shrine)
Scene buildShrineScene() {
    World world;

    auto models = std::filesystem::path(__FILE__).parent_path().parent_path() / "models";
    auto mesh = [&](std::filesystem::path file, Color color) {
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(file.string()), glossy(color)));
    };
    auto glowingMesh = [&](std::filesystem::path file, Color color, double intensity) {
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(file.string()), glossy(color, intensity)));
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
        lightSphere(world, fire, 0.25, Color{1.0, 0.5, 0.15}, 4);

    lightSphere(world, {-6, 5, -2}, 3, Color{1.0, 0.85, 0.65}, 2);      // warm key light, front left (out of view)
    lightSphere(world, {0, 6, -30}, 10, Color{0.9, 0.85, 0.8}, 1.0);    // dim fill behind the camera

    return {std::move(world), Camera{{0, 0.5, -4.5}, {0, 0, 0}}};
}

// armillary: the chrome knot inside the gunmetal star ring, with two thin glowing orbits
// (cyan and amber) sweeping around them as the main lights, over a near-black glossy floor
Scene buildArmillaryScene() {
    World world;

    auto models = std::filesystem::path(__FILE__).parent_path().parent_path() / "models";
    auto mesh = [&](std::filesystem::path file, Color color) {
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(file.string()), glossy(color)));
    };
    auto glowingMesh = [&](std::filesystem::path file, Color color, double intensity) {
        world.addObject(std::make_unique<Object>(std::make_unique<Mesh>(file.string()), glossy(color, intensity)));
    };

    mesh(models / "trefoil_knot.obj", Color{0.92, 0.92, 0.95});        // chrome
    mesh(models / "star_ring.obj",    Color{0.6, 0.6, 0.64});          // brushed steel
    glowingMesh(models / "armillary" / "orbit_inner.obj", Color{0.25, 0.75, 1.0}, 1.2); // cyan
    glowingMesh(models / "armillary" / "orbit_outer.obj", Color{1.0, 0.55, 0.2}, 1.2);  // amber

    // near-black glossy floor: one huge square made of two triangles
    triangle(world, {-1000, -2.1, -1000}, {1000, -2.1, -1000}, {1000, -2.1, 1000}, Color{0.1, 0.1, 0.12});
    triangle(world, {-1000, -2.1, -1000}, {1000, -2.1, 1000}, {-1000, -2.1, 1000}, Color{0.1, 0.1, 0.12});

    // big soft lights out of view, matching the orbit colours: cyan from the left, amber from the right
    lightSphere(world, {-6, 2, -1}, 2.5, Color{0.25, 0.75, 1.0}, 6);
    lightSphere(world, {6, 1, 1}, 2.5, Color{1.0, 0.55, 0.2}, 6);
    lightSphere(world, {0, 12, 2}, 4, Color{1.0, 0.95, 0.9}, 1.0);              // soft white light high above

    return {std::move(world), Camera{{0, 0.5, -4.5}, {0, 0, 0}}};
}
