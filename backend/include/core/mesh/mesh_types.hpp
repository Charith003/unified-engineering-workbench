#pragma once
#include <vector>
#include <string>
#include <array>
#include <cmath>

namespace uew::mesh {

/// Boundary condition classification for mesh faces
enum class BoundaryType {
    Convection,  ///< Robin boundary condition (-k dT/dn = h(T - T_inf))
    HeatSource,  ///< Applied heat flux patch (q'' = P / A_src)
    FixedTemp,   ///< Essential Dirichlet condition (T = T_fixed)
    Adiabatic    ///< Zero heat flux (-k dT/dn = 0)
};

inline const char* to_string(BoundaryType bt) {
    switch (bt) {
        case BoundaryType::Convection: return "Convection";
        case BoundaryType::HeatSource: return "HeatSource";
        case BoundaryType::FixedTemp:  return "FixedTemp";
        case BoundaryType::Adiabatic:  return "Adiabatic";
    }
    return "Unknown";
}

/// 3D Mesh Node
struct Node3D {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/// Linear 4-Node Tetrahedral Element (TET4)
struct TetElement {
    int id = 0;
    std::array<int, 4> nodes = {0, 0, 0, 0}; ///< 0-based node indices
    double volume  = 0.0;                     ///< Element volume [m³]
    double quality = 0.0;                     ///< Normalized geometric quality in [0, 1]
};

/// 2D Triangular Boundary Surface Face
struct SurfaceTriangle {
    int id = 0;
    std::array<int, 3> nodes = {0, 0, 0};     ///< Node indices
    BoundaryType boundary_type = BoundaryType::Convection;
    double area = 0.0;                        ///< Face area [m²]
    std::array<double, 3> normal = {0, 0, 1}; ///< Outward normal vector
    int owning_element = -1;                  ///< Adjacent tetrahedron index
};

/// Comprehensive Mesh Metrics
struct MeshStatistics {
    int node_count           = 0;
    int element_count        = 0;
    int boundary_face_count  = 0;
    int degrees_of_freedom   = 0;
    double generation_time_ms = 0.0;
    double approx_memory_kb  = 0.0;
    double min_quality       = 0.0;
    double max_quality       = 0.0;
    double mean_quality      = 0.0;
    double total_volume_m3   = 0.0;
    bool is_valid            = true;
    std::string status_message = "Success";
};

/// Mesh Resolution Presets
enum class MeshResolution {
    Coarse,  ///< Low element count (~1,500 – 3,000 tetrahedra)
    Medium,  ///< Moderate element count (~6,000 – 12,000 tetrahedra)
    Fine     ///< Higher element count (~20,000 – 45,000 tetrahedra)
};

/// 3D Tetrahedral Finite Element Computational Mesh
struct TetrahedralMesh {
    std::vector<Node3D> nodes;
    std::vector<TetElement> elements;
    std::vector<SurfaceTriangle> boundary_faces;
    MeshStatistics stats;

    void clear() {
        nodes.clear();
        elements.clear();
        boundary_faces.clear();
        stats = MeshStatistics();
    }
};

} // namespace uew::mesh
