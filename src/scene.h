#pragma once

#include <memory>

#include "world.h"

// Each function builds a complete scene and returns the world holding it.
// The world lives on the heap because every object keeps a pointer to it, so it must never move.
std::unique_ptr<World> buildTestScene();
std::unique_ptr<World> buildDayScene();
std::unique_ptr<World> buildKnotScene();
std::unique_ptr<World> buildShellScene();
std::unique_ptr<World> buildKnotRingScene();
std::unique_ptr<World> buildQuakeScene();
std::unique_ptr<World> buildShrineScene();
std::unique_ptr<World> buildArmillaryScene();
