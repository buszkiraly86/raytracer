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
#include "scene.h"

constexpr int width = 800;
constexpr int height = 600;
constexpr double FOV = std::numbers::pi / 3;
constexpr double aspectRatio = static_cast<double>(width) / height;
constexpr double halfWidth = aspectRatio / 2;
constexpr double halfHeight = 0.5;
double d = halfWidth / std::tan(FOV / 2);
constexpr double pixelWidth = 1.0 / height;

std::unique_ptr<World> world;

/*
    This is a left handed coordinate system
*/

Color colorAt(int px, int py)
{
    thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<double> offset{-pixelWidth / 2, pixelWidth / 2};

    // test scene:        camera{0, 3.5, -1},    lookAt{0, 1, 8}
    // day scene:         camera{0, 3, -5},      lookAt{0, 0, 30}
    // knot/mesh scenes:  camera{0, 0.5, -4.5},  lookAt{0, 0, 0}
    glm::dvec3 camera{0, 3.5, -1};
    glm::dvec3 lookAt{0, 1, 8};

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

    return world->getColor(l);
}

int main() {
    // pick a scene (the camera in colorAt has to match it, see the comment there)
    world = buildTestScene();
    // world = buildDayScene();
    // world = buildKnotScene();
    // world = buildShellScene();
    // world = buildKnotRingScene();
    // world = buildQuakeScene();
    // world = buildShrineScene();
    // world = buildArmillaryScene();

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