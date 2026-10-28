#pragma once

#include <glm/glm.hpp>

#include "world.h"

// where a scene is viewed from
struct Camera {
    glm::dvec3 position;
    glm::dvec3 lookAt;
};

// a world to render, and the camera it was laid out for
struct Scene {
    World world;
    Camera camera;
};

// Each function builds a complete scene: the world and the camera to look at it with.
Scene buildTestScene();
Scene buildDayScene();
Scene buildKnotScene();
Scene buildShellScene();
Scene buildKnotRingScene();
Scene buildShrineScene();
Scene buildArmillaryScene();
