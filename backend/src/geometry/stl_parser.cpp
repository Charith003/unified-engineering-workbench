#include "core/geometry/stl_parser.hpp"
#include <sstream>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <limits>

namespace uew::geometry {

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool parse_float3(const std::string& line, const std::string& prefix,
                          float& x, float& y, float& z) {
    std::string t = trim(line);
    if (t.substr(0, prefix.size()) != prefix) return false;
    std::istringstream ss(t.substr(prefix.size()));
    ss >> x >> y >> z;
    return !ss.fail();
}

static bool is_finite3(float x, float y, float z) {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

STLParseResult parse_ascii_stl(const std::string& content) {
    STLParseResult res;
    res.file_size_note = "Tested limit: ~50 MB (~1 million triangles)";

    if (content.empty()) {
        res.error = "Empty file.";
        return res;
    }

    std::istringstream stream(content);
    std::string line;

    // First non-empty line must start with "solid"
    while (std::getline(stream, line)) {
        std::string t = trim(line);
        if (!t.empty()) {
            if (t.substr(0, 5) != "solid") {
                res.error = "File does not begin with 'solid' keyword. Not a valid ASCII STL.";
                return res;
            }
            res.solid_name = (t.size() > 6) ? trim(t.substr(6)) : "";
            break;
        }
    }

    BoundingBox bbox;
    bbox.xmin = bbox.ymin = bbox.zmin =  std::numeric_limits<float>::max();
    bbox.xmax = bbox.ymax = bbox.zmax = -std::numeric_limits<float>::max();

    Triangle tri;
    int vertex_count = 0;
    bool in_facet = false;
    bool in_loop  = false;
    int  line_num = 1;

    while (std::getline(stream, line)) {
        ++line_num;
        std::string t = trim(line);
        if (t.empty()) continue;

        if (t.substr(0, 12) == "facet normal") {
            float nx, ny, nz;
            if (!parse_float3(t, "facet normal", nx, ny, nz)) {
                res.error = "Malformed 'facet normal' on line " + std::to_string(line_num);
                return res;
            }
            tri.normal = {nx, ny, nz};
            in_facet = true;
            vertex_count = 0;
        } else if (t == "outer loop") {
            if (!in_facet) {
                res.error = "'outer loop' without 'facet normal' on line " + std::to_string(line_num);
                return res;
            }
            in_loop = true;
        } else if (t.substr(0, 6) == "vertex") {
            if (!in_loop) {
                res.error = "'vertex' outside 'outer loop' on line " + std::to_string(line_num);
                return res;
            }
            float vx, vy, vz;
            if (!parse_float3(t, "vertex", vx, vy, vz)) {
                res.error = "Malformed vertex on line " + std::to_string(line_num);
                return res;
            }
            if (!is_finite3(vx, vy, vz)) {
                res.error = "Non-finite coordinate on line " + std::to_string(line_num);
                return res;
            }
            if (vertex_count == 0) tri.v0 = {vx, vy, vz};
            else if (vertex_count == 1) tri.v1 = {vx, vy, vz};
            else if (vertex_count == 2) tri.v2 = {vx, vy, vz};
            ++vertex_count;
            bbox.xmin = std::min(bbox.xmin, vx); bbox.xmax = std::max(bbox.xmax, vx);
            bbox.ymin = std::min(bbox.ymin, vy); bbox.ymax = std::max(bbox.ymax, vy);
            bbox.zmin = std::min(bbox.zmin, vz); bbox.zmax = std::max(bbox.zmax, vz);
        } else if (t == "endloop") {
            if (vertex_count != 3) {
                res.error = "Triangle does not have exactly 3 vertices (got " +
                            std::to_string(vertex_count) + ") before endloop on line " +
                            std::to_string(line_num);
                return res;
            }
            in_loop = false;
        } else if (t == "endfacet") {
            if (in_loop) {
                res.error = "'endloop' missing before 'endfacet' on line " + std::to_string(line_num);
                return res;
            }
            res.triangles.push_back(tri);
            in_facet = false;
        } else if (t.substr(0, 8) == "endsolid") {
            break; // done
        }
    }

    if (res.triangles.empty()) {
        res.error = "No triangles found in STL file.";
        return res;
    }

    res.bbox    = bbox;
    res.success = true;
    return res;
}

} // namespace uew::geometry
