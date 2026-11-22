#include "core/geometry/heat_sink_geometry.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace uew::geometry {

HeatSinkParams HeatSinkParams::default_asymmetric() {
    HeatSinkParams p;
    p.base_length    = 0.120; // 120 mm
    p.base_width     = 0.080; // 80 mm
    p.base_thickness = 0.008; // 8 mm

    p.fin_count = 7;
    // Asymmetric fin heights: 22, 35, 42, 45, 38, 28, 20 mm
    // Nonuniform Y positions (concentrated near y ~ 0.035 - 0.045):
    p.fins = {
        { 0.006, 0.0028, 0.022 }, // Fin 0: Near left edge, short
        { 0.016, 0.0030, 0.035 }, // Fin 1
        { 0.028, 0.0035, 0.042 }, // Fin 2: Near heat source, tall & thick
        { 0.039, 0.0038, 0.045 }, // Fin 3: Center/peak, tallest (45 mm)
        { 0.049, 0.0035, 0.038 }, // Fin 4: Near heat source
        { 0.060, 0.0030, 0.028 }, // Fin 5
        { 0.072, 0.0025, 0.020 }  // Fin 6: Near right edge, shortest (20 mm)
    };

    p.heat_source_length = 0.030; // 30 mm
    p.heat_source_width  = 0.025; // 25 mm
    p.heat_source_x_off  = 0.005; // Slightly offset along X (5 mm from center)
    p.heat_source_y_off  = 0.000;
    p.heat_source_power  = 15.0;  // 15 W

    p.hole_diameter = 0.004; // 4 mm
    p.hole_margin_x = 0.008; // 8 mm
    p.hole_margin_y = 0.008; // 8 mm
    p.has_mounting_holes = true;

    return p;
}

HeatSinkGeometry::HeatSinkGeometry(const HeatSinkParams& params)
    : params_(params) {
    if (params_.fins.empty()) {
        params_ = HeatSinkParams::default_asymmetric();
    }
}

void HeatSinkGeometry::set_params(const HeatSinkParams& params) {
    params_ = params;
}

bool HeatSinkGeometry::is_in_mounting_hole(double x, double y) const {
    if (!params_.has_mounting_holes) return false;
    double r = params_.hole_diameter * 0.5;
    double r2 = r * r;

    std::array<std::pair<double, double>, 4> hole_centers = {{
        { params_.hole_margin_x, params_.hole_margin_y },
        { params_.base_length - params_.hole_margin_x, params_.hole_margin_y },
        { params_.hole_margin_x, params_.base_width - params_.hole_margin_y },
        { params_.base_length - params_.hole_margin_x, params_.base_width - params_.hole_margin_y }
    }};

    for (const auto& hc : hole_centers) {
        double dx = x - hc.first;
        double dy = y - hc.second;
        if (dx * dx + dy * dy <= r2) {
            return true;
        }
    }
    return false;
}

bool HeatSinkGeometry::is_in_heat_source(double x, double y) const {
    double cx = params_.base_length * 0.5 + params_.heat_source_x_off;
    double cy = params_.base_width * 0.5 + params_.heat_source_y_off;
    double hx = params_.heat_source_length * 0.5;
    double hy = params_.heat_source_width * 0.5;

    return (x >= cx - hx && x <= cx + hx &&
            y >= cy - hy && y <= cy + hy);
}

GeometryValidationResult HeatSinkGeometry::validate() const {
    GeometryValidationResult res;

    if (params_.base_length <= 0.0 || params_.base_width <= 0.0 || params_.base_thickness <= 0.0) {
        res.is_valid = false;
        res.error_message = "Base dimensions must be strictly positive.";
        return res;
    }

    if (params_.fins.empty()) {
        res.is_valid = false;
        res.error_message = "At least one cooling fin is required.";
        return res;
    }

    // Check fin bounds and overlaps along Y
    double prev_y_end = -1.0;
    for (size_t i = 0; i < params_.fins.size(); ++i) {
        const auto& f = params_.fins[i];
        if (f.y_start < 0.0 || f.thickness <= 0.0 || f.height <= 0.0) {
            res.is_valid = false;
            res.error_message = "Fin " + std::to_string(i) + " has invalid dimensions or position.";
            return res;
        }
        double y_end = f.y_start + f.thickness;
        if (y_end > params_.base_width) {
            res.is_valid = false;
            res.error_message = "Fin " + std::to_string(i) + " exceeds base width.";
            return res;
        }
        if (f.y_start < prev_y_end) {
            res.is_valid = false;
            res.error_message = "Fin " + std::to_string(i) + " overlaps preceding fin.";
            return res;
        }
        prev_y_end = y_end;
    }

    // Check heat source bounds
    double cx = params_.base_length * 0.5 + params_.heat_source_x_off;
    double cy = params_.base_width * 0.5 + params_.heat_source_y_off;
    double hx = params_.heat_source_length * 0.5;
    double hy = params_.heat_source_width * 0.5;
    if (cx - hx < 0.0 || cx + hx > params_.base_length ||
        cy - hy < 0.0 || cy + hy > params_.base_width) {
        res.is_valid = false;
        res.error_message = "Heat source region exceeds base plate boundaries.";
        return res;
    }

    // Calculate approximate volumes and surface areas
    double base_vol = params_.base_length * params_.base_width * params_.base_thickness;
    double fin_vol = 0.0;
    double fin_surface = 0.0;
    for (const auto& f : params_.fins) {
        fin_vol += params_.base_length * f.thickness * f.height;
        // Two lateral faces + top face + two side end faces
        fin_surface += 2.0 * (params_.base_length * f.height) +
                       (params_.base_length * f.thickness) +
                       2.0 * (f.thickness * f.height);
    }

    // Priority 3: In the structured Cartesian-to-tetrahedron discretization,
    // mounting holes are modeled as parametric boundary constraint regions (fixed-temperature clamp zones)
    // rather than physical geometric cutouts in the solid base slab.
    // The volume and surface area represent the actual continuous physical solid.
    res.total_volume_m3 = base_vol + fin_vol;
    res.heat_source_area_m2 = params_.heat_source_length * params_.heat_source_width;
    // Base surface area minus fin attachments
    double base_top_exposed = params_.base_length * params_.base_width;
    for (const auto& f : params_.fins) {
        base_top_exposed -= params_.base_length * f.thickness;
    }
    double base_bottom = params_.base_length * params_.base_width;
    double base_perimeter_sides = 2.0 * (params_.base_length + params_.base_width) * params_.base_thickness;
    res.total_surface_area_m2 = base_bottom + base_top_exposed + base_perimeter_sides + fin_surface;

    auto facets = generate_surface_facets();
    res.facet_count = static_cast<int>(facets.size());
    res.is_valid = true;
    return res;
}

static void add_quad(std::vector<TriangleFacet>& facets,
                     const Vertex3D& v0, const Vertex3D& v1,
                     const Vertex3D& v2, const Vertex3D& v3,
                     const std::array<double, 3>& norm) {
    TriangleFacet t1;
    t1.vertices = { v0, v1, v2 };
    t1.normal   = norm;
    facets.push_back(t1);

    TriangleFacet t2;
    t2.vertices = { v0, v2, v3 };
    t2.normal   = norm;
    facets.push_back(t2);
}

std::vector<TriangleFacet> HeatSinkGeometry::generate_surface_facets() const {
    std::vector<TriangleFacet> facets;
    facets.reserve(200);

    const double L  = params_.base_length;
    const double W  = params_.base_width;
    const double tb = params_.base_thickness;

    // ── 1. Base Bottom Face (z = 0, normal: [0, 0, -1]) ──
    add_quad(facets, {0,0,0}, {0,W,0}, {L,W,0}, {L,0,0}, {0,0,-1});

    // ── 2. Base Side Faces (z in [0, tb]) ──
    // -Y side (y = 0)
    add_quad(facets, {0,0,0}, {L,0,0}, {L,0,tb}, {0,0,tb}, {0,-1,0});
    // +X side (x = L)
    add_quad(facets, {L,0,0}, {L,W,0}, {L,W,tb}, {L,0,tb}, {1,0,0});
    // +Y side (y = W)
    add_quad(facets, {L,W,0}, {0,W,0}, {0,W,tb}, {L,W,tb}, {0,1,0});
    // -X side (x = 0)
    add_quad(facets, {0,W,0}, {0,0,0}, {0,0,tb}, {0,W,tb}, {-1,0,0});

    // ── 3. Base Top Exposed Surfaces (z = tb, normal: [0, 0, 1]) ──
    // Space before first fin
    if (!params_.fins.empty() && params_.fins[0].y_start > 0.0) {
        add_quad(facets, {0,0,tb}, {L,0,tb}, {L,params_.fins[0].y_start,tb}, {0,params_.fins[0].y_start,tb}, {0,0,1});
    }
    // Gaps between consecutive fins
    for (size_t i = 0; i + 1 < params_.fins.size(); ++i) {
        double y_gap_start = params_.fins[i].y_start + params_.fins[i].thickness;
        double y_gap_end   = params_.fins[i+1].y_start;
        if (y_gap_end > y_gap_start) {
            add_quad(facets, {0,y_gap_start,tb}, {L,y_gap_start,tb}, {L,y_gap_end,tb}, {0,y_gap_end,tb}, {0,0,1});
        }
    }
    // Space after last fin
    if (!params_.fins.empty()) {
        double last_y_end = params_.fins.back().y_start + params_.fins.back().thickness;
        if (last_y_end < W) {
            add_quad(facets, {0,last_y_end,tb}, {L,last_y_end,tb}, {L,W,tb}, {0,W,tb}, {0,0,1});
        }
    }

    // ── 4. Vertical Cooling Fins (z from tb to tb + f.height) ──
    for (const auto& f : params_.fins) {
        double y0 = f.y_start;
        double y1 = f.y_start + f.thickness;
        double z0 = tb;
        double z1 = tb + f.height;

        // Fin lateral side 1 (-Y normal: [0, -1, 0])
        add_quad(facets, {0,y0,z0}, {L,y0,z0}, {L,y0,z1}, {0,y0,z1}, {0,-1,0});
        // Fin lateral side 2 (+Y normal: [0, 1, 0])
        add_quad(facets, {L,y1,z0}, {0,y1,z0}, {0,y1,z1}, {L,y1,z1}, {0,1,0});
        // Fin top face (+Z normal: [0, 0, 1])
        add_quad(facets, {0,y0,z1}, {L,y0,z1}, {L,y1,z1}, {0,y1,z1}, {0,0,1});
        // Fin end face at -X (-X normal: [-1, 0, 0])
        add_quad(facets, {0,y1,z0}, {0,y0,z0}, {0,y0,z1}, {0,y1,z1}, {-1,0,0});
        // Fin end face at +X (+X normal: [1, 0, 0])
        add_quad(facets, {L,y0,z0}, {L,y1,z0}, {L,y1,z1}, {L,y0,z1}, {1,0,0});
    }

    return facets;
}

std::string HeatSinkGeometry::to_ascii_stl(const std::string& solid_name) const {
    auto facets = generate_surface_facets();
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "solid " << solid_name << "\n";

    for (const auto& f : facets) {
        ss << "  facet normal " << f.normal[0] << " " << f.normal[1] << " " << f.normal[2] << "\n";
        ss << "    outer loop\n";
        for (const auto& v : f.vertices) {
            ss << "      vertex " << v.x << " " << v.y << " " << v.z << "\n";
        }
        ss << "    endloop\n";
        ss << "  endfacet\n";
    }

    ss << "endsolid " << solid_name << "\n";
    return ss.str();
}

} // namespace uew::geometry
