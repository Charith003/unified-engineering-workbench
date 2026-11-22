#pragma once
#include <vector>
#include <string>
#include <array>
#include <cmath>

namespace uew::geometry {

/// Configuration for individual fin
struct FinConfig {
    double y_start    = 0.0;    ///< Fin start along Y [m]
    double thickness  = 0.003;  ///< Fin thickness along Y [m]
    double height     = 0.030;  ///< Fin height along Z [m] (from base top)
};

/// Parameters for asymmetric heat sink geometry (SI units: meters)
struct HeatSinkParams {
    double base_length    = 0.120;   ///< Base length along X: 120 mm [m]
    double base_width     = 0.080;   ///< Base width along Y: 80 mm [m]
    double base_thickness = 0.008;   ///< Base thickness along Z: 8 mm [m]

    int fin_count = 7;               ///< Number of vertical cooling fins
    std::vector<FinConfig> fins;     ///< Asymmetric fin parameters

    // Heat source patch (on bottom surface z = 0)
    double heat_source_length = 0.030; ///< 30 mm along X [m]
    double heat_source_width  = 0.025; ///< 25 mm along Y [m]
    double heat_source_x_off  = 0.005; ///< Offset from center along X [m]
    double heat_source_y_off  = 0.000; ///< Offset from center along Y [m]
    double heat_source_power  = 15.0;  ///< Total heat power [W]

    // Mounting holes (4 corner boundary constraint zones)
    // Note: Modeled parametrically as fixed-temperature boundary clamp zones on the base plate
    // rather than geometric circular cutouts in the structured hexahedral mesh.
    double hole_diameter = 0.004;      ///< 4 mm diameter [m]
    double hole_margin_x = 0.008;      ///< Distance from X edge [m]
    double hole_margin_y = 0.008;      ///< Distance from Y edge [m]
    bool   has_mounting_holes = true;

    // Default asymmetric configuration
    static HeatSinkParams default_asymmetric();
};

/// 3D Vertex representation
struct Vertex3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/// 3D Triangle facet
struct TriangleFacet {
    std::array<Vertex3D, 3> vertices;
    std::array<double, 3> normal;
};

/// Validation result for heat sink geometry
struct GeometryValidationResult {
    bool is_valid = true;
    std::string error_message;
    double total_volume_m3 = 0.0;
    double total_surface_area_m2 = 0.0;
    double heat_source_area_m2 = 0.0;
    int facet_count = 0;
};

/// Asymmetric electronic heat sink geometry builder and exporter
class HeatSinkGeometry {
public:
    explicit HeatSinkGeometry(const HeatSinkParams& params = HeatSinkParams::default_asymmetric());

    const HeatSinkParams& params() const { return params_; }
    void set_params(const HeatSinkParams& params);

    /// Validates dimensional integrity (bounds, overlaps, positive volumes)
    GeometryValidationResult validate() const;

    /// Generates surface triangular facets for 3D visualization and CAD export
    std::vector<TriangleFacet> generate_surface_facets() const;

    /// Exports geometry as ASCII STL format
    std::string to_ascii_stl(const std::string& solid_name = "asymmetric_heat_sink") const;

    /// Checks if a 2D point (x, y) falls inside any mounting hole
    bool is_in_mounting_hole(double x, double y) const;

    /// Checks if a 2D point (x, y) falls inside the heat source footprint
    bool is_in_heat_source(double x, double y) const;

private:
    HeatSinkParams params_;
};

} // namespace uew::geometry
