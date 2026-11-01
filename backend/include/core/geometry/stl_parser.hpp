#pragma once
#include <string>
#include <vector>
#include <array>
#include <stdexcept>

namespace uew::geometry {

/// A triangle in 3D space (three vertices).
struct Triangle {
    std::array<float,3> v0, v1, v2;
    std::array<float,3> normal;
};

/// Bounding box of a mesh.
struct BoundingBox {
    float xmin, xmax, ymin, ymax, zmin, zmax;
    float dx() const { return xmax - xmin; }
    float dy() const { return ymax - ymin; }
    float dz() const { return zmax - zmin; }
};

struct STLParseResult {
    bool success{false};
    std::string error;
    std::string solid_name;
    std::vector<Triangle> triangles;
    BoundingBox bbox{};
    std::string file_size_note;
};

/// Parse an ASCII STL string.
/// Validates finite coordinates, non-empty file, and well-formed records.
/// Tested file-size limit: ~50 MB (≈ 1 million triangles).
STLParseResult parse_ascii_stl(const std::string& content);

} // namespace uew::geometry
