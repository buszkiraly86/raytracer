# raytracer

A path tracer written from scratch in C++23. It renders progressively into an SDL2 window: each frame adds one more
sample per pixel, so the image starts out grainy and cleans up as you watch.

![Knot inside a star ring, lit by cyan and amber orbits](docs/images/armillary.png)

![A row of spheres from mirror-sharp to blurry, in front of rainbow light strips](docs/images/test.png)

![Copper trefoil knot inside a silver star ring](docs/images/knot_ring.png)

## Features

- **Path tracing**: rays start at the camera and bounce around the scene, picking up light from every glowing surface
  they hit, until they escape or reach the bounce limit; each pixel averages many random paths.
- **Shapes**: spheres, triangles and triangle meshes loaded from OBJ files.
- **Two-level BVH**: a bounding volume hierarchy inside every mesh, and another over all objects in the scene. Both
  are the same generic `BVH<T>` template, built top-down by splitting at the median along the longest axis.
- **Ray–triangle intersection** with the Möller–Trumbore method.
- **Materials**: colour, roughness (from mirror-sharp to blurry reflections) and emission. Lights are ordinary objects
  whose material emits, so any shape, including a mesh, can be a light.
- **Multithreaded rendering**, anti-aliasing (each sample is jittered inside its pixel) and gamma correction.
- **Scenes**: each scene is a function that returns a world together with the camera it was laid out for.

The window title shows the frame rate and the number of samples accumulated so far.

## Building

Requirements: CMake 3.20 or newer, a C++23 compiler, SDL2 and GLM.

```sh
# macOS
brew install cmake sdl2 glm

# Ubuntu / Debian
sudo apt install cmake libsdl2-dev libglm-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/raytracer
```

Use a Release build: a debug build is many times slower.

To render a different scene, change which `build...Scene()` call is active at the top of `main()` in
[`src/main.cpp`](src/main.cpp). The scenes themselves are in [`src/scene.cpp`](src/scene.cpp).

## How it's organised

| File | What it holds |
|---|---|
| `core.h` | Small value types used everywhere: `Ray`, `Intersection`, `BoundingBox` |
| `geometry.h/.cpp` | The `Shape` interface, with `Sphere`, `Triangle` and `Mesh` (OBJ loading, and a BVH over its triangles) |
| `bvh.h` | The generic `BVH<T>`: construction, box tests and nearest-hit search |
| `material.h` | Colour, roughness and emission, and how a surface scatters a ray |
| `object.h/.cpp` | A shape paired with a material |
| `world.h/.cpp` | All objects in a scene, a BVH over them, and the tracing itself |
| `scene.h/.cpp` | The scenes, each with its camera |
| `main.cpp` | Window, camera rays, threads and progressive accumulation |

## Models

The OBJ files in [`models/`](models) are procedural: the trefoil knot, the twisted star ring, the seashell, the orbit
rings and the shrine pieces were each generated from their mathematical description by small Python scripts.
Those scripts were written by Claude (an AI assistant) during development and are not part of this repository.

## Current limitations

- There is one material model, a blurred mirror: no matte (diffuse) surfaces and no glass, and it is not
  physically based (it doesn't conserve energy).
- A roughness of exactly 0 isn't supported yet.
- Meshes are flat-shaded, so their facets show.
- Meshes can't be moved, rotated or scaled; they render where their OBJ file places them.
- The scene is chosen in code, not on the command line.

## License

MIT, see [LICENSE](LICENSE).
