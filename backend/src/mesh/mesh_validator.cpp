#include "core/mesh/mesh_validator.hpp"
#include <cmath>
#include <limits>
#include <algorithm>
#include <vector>

namespace uew::mesh {

double MeshValidator::compute_tet_volume(const Node3D& n0, const Node3D& n1,
                                        const Node3D& n2, const Node3D& n3) {
    double ax = n1.x - n0.x, ay = n1.y - n0.y, az = n1.z - n0.z;
    double bx = n2.x - n0.x, by = n2.y - n0.y, bz = n2.z - n0.z;
    double cx = n3.x - n0.x, cy = n3.y - n0.y, cz = n3.z - n0.z;

    // det([a, b, c]) / 6.0
    double det = ax * (by * cz - bz * cy) -
                 ay * (bx * cz - bz * cx) +
                 az * (bx * cy - by * cx);

    return det / 6.0;
}

double MeshValidator::compute_tet_quality(const Node3D& n0, const Node3D& n1,
                                         const Node3D& n2, const Node3D& n3,
                                         double volume) {
    if (volume <= 1e-16) return 0.0;

    auto dist2 = [](const Node3D& a, const Node3D& b) {
        double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    };

    double l01 = dist2(n0, n1);
    double l02 = dist2(n0, n2);
    double l03 = dist2(n0, n3);
    double l12 = dist2(n1, n2);
    double l13 = dist2(n1, n3);
    double l23 = dist2(n2, n3);

    double sum_l2 = l01 + l02 + l03 + l12 + l13 + l23;
    if (sum_l2 <= 1e-16) return 0.0;

    // Standard normalized mean-ratio quality metric for tetrahedron:
    // Q = 72 * sqrt(3) * V / (sum(l_i^2))^(3/2)
    // For regular tetrahedron with edge a: V = a^3 / (6*sqrt(2)), sum(l^2) = 6*a^2
    // Q_regular = 72*sqrt(3)*(a^3 / (6*sqrt(2))) / (6*a^2)^(1.5) = 1.0.
    double denom = std::pow(sum_l2, 1.5);
    double q = (72.0 * std::sqrt(3.0) * volume) / denom;
    return std::clamp(q, 0.0, 1.0);
}

MeshValidationReport MeshValidator::validate(TetrahedralMesh& mesh) {
    MeshValidationReport report;
    const int n_nodes = static_cast<int>(mesh.nodes.size());
    const int n_elems = static_cast<int>(mesh.elements.size());

    if (n_nodes < 4 || n_elems < 1) {
        report.is_valid = false;
        report.message = "Mesh is empty or has fewer than 4 nodes / 1 element.";
        return report;
    }

    std::vector<bool> node_referenced(n_nodes, false);

    // 1. Validate nodes
    for (int i = 0; i < n_nodes; ++i) {
        const auto& n = mesh.nodes[i];
        if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)) {
            report.non_finite_nodes++;
        }
    }

    // 2. Validate elements
    double min_vol = std::numeric_limits<double>::infinity();
    double max_vol = -std::numeric_limits<double>::infinity();
    double min_q   = std::numeric_limits<double>::infinity();
    double max_q   = -std::numeric_limits<double>::infinity();
    double sum_q   = 0.0;
    double total_vol = 0.0;

    for (int e = 0; e < n_elems; ++e) {
        auto& el = mesh.elements[e];
        bool node_oob = false;
        for (int k = 0; k < 4; ++k) {
            if (el.nodes[k] < 0 || el.nodes[k] >= n_nodes) {
                node_oob = true;
            } else {
                node_referenced[el.nodes[k]] = true;
            }
        }
        // Check for duplicate node indices within element
        if (el.nodes[0] == el.nodes[1] || el.nodes[0] == el.nodes[2] || el.nodes[0] == el.nodes[3] ||
            el.nodes[1] == el.nodes[2] || el.nodes[1] == el.nodes[3] || el.nodes[2] == el.nodes[3]) {
            node_oob = true;
        }

        if (node_oob) {
            report.degenerate_elements++;
            continue;
        }

        const auto& n0 = mesh.nodes[el.nodes[0]];
        const auto& n1 = mesh.nodes[el.nodes[1]];
        const auto& n2 = mesh.nodes[el.nodes[2]];
        const auto& n3 = mesh.nodes[el.nodes[3]];

        double vol = compute_tet_volume(n0, n1, n2, n3);
        if (vol < 0.0) {
            // Negative volume indicates inverted element orientation
            vol = -vol;
            report.inverted_elements++;
        }

        if (!std::isfinite(vol) || vol <= 1e-15) {
            report.degenerate_elements++;
        }

        double q = compute_tet_quality(n0, n1, n2, n3, vol);
        el.volume  = vol;
        el.quality = q;

        min_vol = std::min(min_vol, vol);
        max_vol = std::max(max_vol, vol);
        min_q   = std::min(min_q, q);
        max_q   = std::max(max_q, q);
        sum_q  += q;
        total_vol += vol;
    }

    // 3. Validate boundary faces
    for (const auto& f : mesh.boundary_faces) {
        bool face_valid = true;
        for (int k = 0; k < 3; ++k) {
            if (f.nodes[k] < 0 || f.nodes[k] >= n_nodes) {
                face_valid = false;
            }
        }
        if (f.nodes[0] == f.nodes[1] || f.nodes[1] == f.nodes[2] || f.nodes[0] == f.nodes[2]) {
            face_valid = false;
        }
        if (f.owning_element < 0 || f.owning_element >= n_elems) {
            face_valid = false;
        }
        if (!std::isfinite(f.area) || f.area <= 1e-15) {
            face_valid = false;
        }
        if (!std::isfinite(f.normal[0]) || !std::isfinite(f.normal[1]) || !std::isfinite(f.normal[2])) {
            face_valid = false;
        }
        if (!face_valid) {
            report.invalid_faces++;
        }
    }

    // 4. Check unreferenced nodes
    for (int i = 0; i < n_nodes; ++i) {
        if (!node_referenced[i]) {
            report.unreferenced_nodes++;
        }
    }

    report.min_volume  = (min_vol == std::numeric_limits<double>::infinity()) ? 0.0 : min_vol;
    report.max_volume  = (max_vol == -std::numeric_limits<double>::infinity()) ? 0.0 : max_vol;
    report.min_quality = (min_q == std::numeric_limits<double>::infinity()) ? 0.0 : min_q;
    report.max_quality = (max_q == -std::numeric_limits<double>::infinity()) ? 0.0 : max_q;
    report.mean_quality = (n_elems > 0) ? sum_q / n_elems : 0.0;

    if (report.non_finite_nodes > 0 || report.degenerate_elements > 0 || report.invalid_faces > 0 || report.inverted_elements > 0) {
        report.is_valid = false;
        report.message = "Mesh validation failed: " +
                         std::to_string(report.degenerate_elements) + " degenerate elements, " +
                         std::to_string(report.inverted_elements) + " inverted elements, " +
                         std::to_string(report.non_finite_nodes) + " non-finite nodes, " +
                         std::to_string(report.invalid_faces) + " invalid faces.";
    } else {
        report.is_valid = true;
        report.message = "Mesh successfully validated (" + std::to_string(n_elems) +
                         " tetrahedra, " + std::to_string(n_nodes) + " nodes, mean quality " +
                         std::to_string(report.mean_quality).substr(0, 5) + ").";
    }

    // Update mesh stats
    mesh.stats.is_valid = report.is_valid;
    mesh.stats.status_message = report.message;
    mesh.stats.min_quality = report.min_quality;
    mesh.stats.max_quality = report.max_quality;
    mesh.stats.mean_quality = report.mean_quality;
    mesh.stats.total_volume_m3 = total_vol;

    return report;
}

} // namespace uew::mesh
