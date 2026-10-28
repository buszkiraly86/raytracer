cmake_minimum_required(VERSION 3.20)
project(raytracer LANGUAGES CXX)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(raytracer
    src/main.cpp
    src/geometry.cpp
    src/object.cpp
    src/world.cpp
    src/scene.cpp
)

target_link_libraries(raytracer PRIVATE SDL2::SDL2 glm::glm)

if(MSVC)
    target_compile_options(raytracer PRIVATE /W4)
else()
    target_compile_options(raytracer PRIVATE -Wall -Wextra)
endif()
