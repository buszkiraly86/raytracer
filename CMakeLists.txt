cmake_minimum_required(VERSION 4.4)
project(raytracer)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(raytracer
    src/main.cpp
    src/geometry.cpp
    src/object.cpp
    src/world.cpp
)

target_link_libraries(raytracer PRIVATE SDL2::SDL2)
target_link_libraries(raytracer PRIVATE SDL2::SDL2 glm::glm)