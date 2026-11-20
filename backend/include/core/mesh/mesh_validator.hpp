#pragma once
#include "core/mesh/mesh_types.hpp"
#include <string>

namespace uew::mesh {

struct MeshValidationReport {
    bool is_valid = true;
    std::string message = "Mesh is topologically and geometrically valid.";
    int degenerate_elements = 0;
    int inverted_elements = 0;
    int non_finite_nodes = 0;
    int unreferenced_nodes = 0;
    int invalid_faces = 0;
    double min_volume = 0.0;
    double max_volume = 0.0;
    double min_quality = 0.0;
    double max_quality = 0.0;
    double mean_quality = 0.0;
};

class MeshValidator {
public:
    /// Validates complete tetrahedral mesh geometry, connectivity, and boundary tags
    static MeshValidationReport validate(TetrahedralMesh& mesh);

    /// Computes normalized geometric quality of a single tetrahedron in [0, 1]
    static double compute_tet_quality(const Node3D& n0, const Node3D& n1,
                                     const Node3D& n2, const Node3D& n3,
                                     double volume);

    /// Computes exact signed volume of a linear tetrahedron: 1/6 * det([v1-v0, v2-v0, v3-v0])
    static double compute_tet_volume(const Node3D& n0, const Node3D& n1,
                                    const Node3D& n2, const Node3D& n3);
};

} // namespace uew::mesh
