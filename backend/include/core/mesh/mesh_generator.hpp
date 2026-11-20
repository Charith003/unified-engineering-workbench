#pragma once
#include "core/mesh/mesh_types.hpp"
#include "core/geometry/heat_sink_geometry.hpp"

namespace uew::mesh {

/// Mesh generation options
struct MeshGenerationOptions {
    MeshResolution resolution = MeshResolution::Medium;
    bool exclude_mounting_holes = true;
    double custom_h_max = 0.0; // 0 for automatic resolution-based sizing
};

class MeshGenerator {
public:
    explicit MeshGenerator(const geometry::HeatSinkGeometry& geometry);

    /// Generates conformal 3D tetrahedral finite element mesh conforming to heat sink
    TetrahedralMesh generate(const MeshGenerationOptions& options = MeshGenerationOptions());

    /// Helper to convert resolution enum to string
    static std::string resolution_to_string(MeshResolution res);

private:
    geometry::HeatSinkGeometry geometry_;
};

} // namespace uew::mesh
