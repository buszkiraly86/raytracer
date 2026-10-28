#pragma once

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <vector>
#include <glm/glm.hpp>

#include "core.h"

// nodes with this many items or fewer become leaves instead of splitting further
constexpr std::size_t maxLeafSize = 4;

template <typename T>
struct BVH {
    std::vector<const T*> items;
    BoundingBox box;
    std::unique_ptr<BVH> childA{}, childB{};

    BVH() = default;
    BVH(std::vector<const T*> &&items);

    std::optional<std::tuple<const T*, Intersection>> intersects(const Ray &ray) const;
};

template <typename T>
BVH<T>::BVH(std::vector<const T*> &&items) {
    glm::dvec3 min{
        std::numeric_limits<double>::infinity()
    };
    glm::dvec3 max{
        -std::numeric_limits<double>::infinity()
    };

    for (const auto &item : items) {
        auto boundingBox = item->getBoundingBox();
        min = glm::min(min, boundingBox.A);
        max = glm::max(max, boundingBox.B);
    }

    box = {min, max};

    if (items.size() <= maxLeafSize) {
        this->items = std::move(items);
    } else {
        int longestAxis{};
        auto diff = max - min;
        double maxDiff = -std::numeric_limits<double>::infinity();
        for (int i = 0; i < 3; i++) {
            if (diff[i] > maxDiff) {
                longestAxis = i;
                maxDiff = diff[i];
            }
        }

        auto m = items.begin() + items.size() / 2;
        std::nth_element(items.begin(), m, items.end(), [&longestAxis](const T *s1, const T *s2){
            auto boundingBox1 = s1->getBoundingBox();
            auto boundingBox2 = s2->getBoundingBox();

            return (boundingBox1.A + boundingBox1.B)[longestAxis] < (boundingBox2.A + boundingBox2.B)[longestAxis];
        });

        std::vector<const T*> left(std::make_move_iterator(items.begin()), std::make_move_iterator(m));
        std::vector<const T*> right(std::make_move_iterator(m), std::make_move_iterator(items.end()));

        childA = std::make_unique<BVH>(std::move(left));
        childB = std::make_unique<BVH>(std::move(right));
    }
}

template <typename T>
std::optional<std::tuple<const T*, Intersection>> BVH<T>::intersects(const Ray &ray) const {
    // A is the min corner, B the max corner.
    // Each axis gives the interval of t where the ray is between that axis's two walls;
    // the ray is in the box where all three overlap: from the latest entry to the earliest exit.
    double enter = 0;                                   // starting at 0 also rejects boxes behind the ray
    double exit = std::numeric_limits<double>::infinity();

    for (int a = 0; a < 3; ++a) {                       // a = 0, 1, 2 means x, y, z
        if (std::abs(ray.dir[a]) < epsilon) {
            // parallel to this axis: always between the walls, or never
            if (ray.point[a] < box.A[a] || ray.point[a] > box.B[a]) return std::nullopt;
            continue;
        }
        double t1 = (box.A[a] - ray.point[a]) / ray.dir[a];
        double t2 = (box.B[a] - ray.point[a]) / ray.dir[a];
        enter = std::max(enter, std::min(t1, t2));      // running max of the entries
        exit  = std::min(exit,  std::max(t1, t2));      // running min of the exits
        if (enter > exit) return std::nullopt;          // already left one slab before entering another
    }

    std::optional<std::tuple<const T*, Intersection>> Ais{}, Bis{};

    if (childA) {
        Ais = childA->intersects(ray);
        Bis = childB->intersects(ray);

        if (Ais && Bis) {
            if (glm::distance(ray.point, std::get<1>(*Ais).point) < glm::distance(ray.point, std::get<1>(*Bis).point)) {
                return Ais;
            } else {
                return Bis;
            }
        } else if (Ais) {
            return Ais;
        } else {
            return Bis;
        }
    }

    // if we are still here, this is a leafnode
    // so let's check its items
    std::optional<std::tuple<const T*, Intersection>> closest{};
    double closestDistance = std::numeric_limits<double>::infinity();

    for (const auto &item : items) {
        auto intersection = item->intersect(ray);
        if (intersection) {
            double distance = glm::distance(intersection.value().point, ray.point);
            if (distance < closestDistance) {
                closestDistance = distance;
                closest = {
                    item,
                    *intersection
                };
            }
        }
    }

    return closest;
}
