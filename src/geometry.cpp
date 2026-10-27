#include "geometry.h"

#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

Shape::~Shape() {};

Sphere::Sphere(glm::dvec3 pos, double radius) : pos(pos), radius(radius) {}

std::optional<Intersection> Sphere::intersect(const Line &l) const {
    auto diff = l.point - pos;
    double a = 1;
    double b = 2 * glm::dot(l.dir, diff);
    double c = (diff.x) * (diff.x) + (diff.y) * (diff.y) + (diff.z) * (diff.z) - radius * radius;
    double discriminant = b * b - 4 * a * c;
    double root = sqrt(discriminant);

    if (discriminant < 0) return std::nullopt;

    double t = (-b - root) / (2 * a);
    if (t < epsilon) t = (-b + root) / (2 * a);
    if (t < epsilon) return std::nullopt;

    auto intersectionPoint = l.point + l.dir * t;
    auto surfaceNormal = glm::normalize(intersectionPoint - pos);

    return Intersection {
        intersectionPoint,
        surfaceNormal
    };
}

Triangle::Triangle(const std::array<glm::dvec3, 3> &points) : points(points) {
    glm::dvec3 AB = points[1] - points[0];
    glm::dvec3 AC = points[2] - points[0];
    auto n = glm::cross(AB, AC);

    if (glm::length(n) < epsilon) throw std::invalid_argument("collinear triangle");

    normal = glm::normalize(glm::cross(AB, AC));
}

std::optional<Intersection> Triangle::intersect(const Line &l) const {
    if (std::abs(glm::dot(normal, l.dir)) < epsilon) return std::nullopt;

    auto &u = points[0];
    auto v = points[1] - u;
    auto w = points[2] - u;
    auto x = l.point - u;

    // Möller–Trumbore: Cramer's rule on [-dir | v | w] * (t, b, c) = x,
    // with the determinants written as dot(a, cross(b, c)) and the two cross products shared
    auto p = glm::cross(l.dir, w);
    double inv = 1.0 / glm::dot(v, p);

    double b = glm::dot(x, p) * inv;
    if (b < 0 || b > 1) return std::nullopt;

    auto q = glm::cross(x, v);
    double c = glm::dot(l.dir, q) * inv;
    if (c < 0 || b + c > 1) return std::nullopt;

    double t = glm::dot(w, q) * inv;
    if (t <= epsilon) return std::nullopt;

    return Intersection {
        u + b * v + c * w,
        normal
    };
}

BoundingBox::BoundingBox(std::vector<Triangle> &&triangles) {
    glm::dvec3 min{
        std::numeric_limits<double>::infinity()
    };
    glm::dvec3 max{
        -std::numeric_limits<double>::infinity()
    };

    for (const auto &triangle : triangles) {
        for (const auto &point : triangle.points) {
            min = glm::min(min, point);
            max = glm::max(max, point);
        }
    }

    A = min;
    B = max;

    if (triangles.size() < 5) { // TODO: magic number
        this->triangles = std::move(triangles);
    } else {
        int longestAxis{};
        auto diff = B - A;
        double maxDiff = -std::numeric_limits<double>::infinity();
        for (int i = 0; i < 3; i++) {
            if (diff[i] > maxDiff) {
                longestAxis = i;
                maxDiff = diff[i];
            }
        }

        auto m = triangles.begin() + triangles.size() / 2;
        std::nth_element(triangles.begin(), m, triangles.end(), [&longestAxis](const Triangle &t1, const Triangle &t2){
            return (t1.points[0] + t1.points[1] + t1.points[2])[longestAxis] < (t2.points[0] + t2.points[1] + t2.points[2])[longestAxis];
        });

        std::vector<Triangle> left(std::make_move_iterator(triangles.begin()), std::make_move_iterator(m));
        std::vector<Triangle> right(std::make_move_iterator(m), std::make_move_iterator(triangles.end()));

        childA = std::make_unique<BoundingBox>(std::move(left));
        childB = std::make_unique<BoundingBox>(std::move(right));
    }
}

std::optional<Intersection> BoundingBox::intersects(const Line &l) const {
    // A is the min corner, B the max corner.
    // Each axis gives the interval of t where the ray is between that axis's two walls;
    // the ray is in the box where all three overlap: from the latest entry to the earliest exit.
    double enter = 0;                                   // starting at 0 also rejects boxes behind the ray
    double exit = std::numeric_limits<double>::infinity();

    for (int a = 0; a < 3; ++a) {                       // a = 0, 1, 2 means x, y, z
        if (std::abs(l.dir[a]) < epsilon) {
            // parallel to this axis: always between the walls, or never
            if (l.point[a] < A[a] || l.point[a] > B[a]) return std::nullopt;
            continue;
        }
        double t1 = (A[a] - l.point[a]) / l.dir[a];
        double t2 = (B[a] - l.point[a]) / l.dir[a];
        enter = std::max(enter, std::min(t1, t2));      // running max of the entries
        exit  = std::min(exit,  std::max(t1, t2));      // running min of the exits
        if (enter > exit) return std::nullopt;          // already left one slab before entering another
    }

    std::optional<Intersection> Ais{}, Bis{};
    
    if (childA) {
        Ais = childA->intersects(l);
        Bis = childB->intersects(l);
        
        if (Ais && Bis) {
            if (glm::distance(l.point, Ais->point) < glm::distance(l.point, Bis->point)) {
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
    // so let's check its triangles
    std::optional<Intersection> closest{};
    double closestDistance = std::numeric_limits<double>::infinity();

    for (const auto &triangle : triangles) {
        auto intersection = triangle.intersect(l);
        if (intersection) {
            double distance = glm::distance(intersection.value().point, l.point);
            if (distance < closestDistance) {
                closestDistance = distance;
                closest = intersection;
            }
        }
    }

    return closest;
}

Mesh::Mesh(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("cannot open mesh file: " + filename);

    std::vector<glm::dvec3> vertices;
    std::vector<Triangle> triangles;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string type;
        in >> type;

        if (type == "v") {
            glm::dvec3 v;
            in >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (type == "f") {
            // a face lists 3 or more vertices; each entry is "v", "v/vt", "v//vn" or "v/vt/vn",
            // and only the vertex index (before the first '/') matters here
            std::vector<glm::dvec3> face;
            std::string entry;
            while (in >> entry) {
                int index = std::stoi(entry.substr(0, entry.find('/')));
                // indices start at 1; negative ones count back from the last vertex read so far
                int i = index > 0 ? index - 1 : static_cast<int>(vertices.size()) + index;
                if (i < 0 || i >= static_cast<int>(vertices.size()))
                    throw std::runtime_error("bad vertex index in " + filename + ": " + line);
                face.push_back(vertices[i]);
            }

            // split polygons into a fan of triangles around the first vertex
            for (std::size_t k = 1; k + 1 < face.size(); ++k) {
                try {
                    triangles.emplace_back(std::array<glm::dvec3, 3>{face[0], face[k], face[k + 1]});
                } catch (const std::invalid_argument &) {
                    // degenerate (collinear) triangle: real files have these, just skip it
                }
            }
        }
        // anything else (normals, texture coordinates, materials, groups, comments) is ignored for now
    }

    boundingBox = BoundingBox(std::move(triangles));
}

std::optional<Intersection> Mesh::intersect(const Line &l) const {
    return boundingBox.intersects(l);
}
