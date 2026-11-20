#include "core/thermal/fem_3d_solver.hpp"
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <iostream>

namespace uew::thermal {

Eigen::Matrix4d FEM3DSolver::compute_element_stiffness(
    const mesh::TetrahedralMesh& mesh,
    const mesh::TetElement& tet,
    double thermal_conductivity
) {
    Eigen::Matrix4d J;
    for (int i = 0; i < 4; ++i) {
        const auto& p = mesh.nodes[tet.nodes[i]];
        J(i, 0) = 1.0;
        J(i, 1) = p.x;
        J(i, 2) = p.y;
        J(i, 3) = p.z;
    }

    double detJ = J.determinant();
    double volume = std::abs(detJ) / 6.0;
    if (volume < 1e-15) {
        return Eigen::Matrix4d::Zero();
    }

    // Invert J to obtain shape function gradient coefficients
    Eigen::Matrix4d invJ = J.inverse();

    // Shape function gradients: grad N_i = [invJ(1, i), invJ(2, i), invJ(3, i)]^T
    Eigen::Matrix<double, 3, 4> gradN;
    for (int i = 0; i < 4; ++i) {
        gradN(0, i) = invJ(1, i);
        gradN(1, i) = invJ(2, i);
        gradN(2, i) = invJ(3, i);
    }

    // K^e = thermal_conductivity * volume * (gradN^T * gradN)
    Eigen::Matrix4d Ke = thermal_conductivity * volume * (gradN.transpose() * gradN);
    return Ke;
}

Eigen::Vector4d FEM3DSolver::compute_element_load(
    const mesh::TetrahedralMesh& mesh,
    const mesh::TetElement& tet,
    double internal_generation_Q
) {
    if (std::abs(internal_generation_Q) < 1e-12) {
        return Eigen::Vector4d::Zero();
    }
    double volume = tet.volume > 0.0 ? tet.volume : 1e-12;
    double elem_load = internal_generation_Q * volume / 4.0;
    return Eigen::Vector4d::Constant(elem_load);
}

AssembledThermalSystem FEM3DSolver::assemble(
    const mesh::TetrahedralMesh& mesh,
    const ThermalMaterial& material,
    const ThermalBoundaryConditions& bc
) {
    auto t0 = std::chrono::high_resolution_clock::now();

    AssembledThermalSystem sys;
    size_t num_nodes = mesh.nodes.size();
    sys.dof_count = num_nodes;

    sys.F = Eigen::VectorXd::Zero(num_nodes);
    std::vector<Eigen::Triplet<double>> triplets;
    // Reserve roughly 20-30 entries per node for 3D tetrahedral mesh
    triplets.reserve(num_nodes * 25);

    // 1. Element Conduction Stiffness Matrix K_vol and Volumetric Q
    for (const auto& tet : mesh.elements) {
        Eigen::Matrix4d Ke = compute_element_stiffness(mesh, tet, material.thermal_conductivity);
        Eigen::Vector4d Fe = compute_element_load(mesh, tet, bc.internal_generation_w_m3);

        for (int i = 0; i < 4; ++i) {
            int ni = tet.nodes[i];
            sys.F(ni) += Fe(i);
            for (int j = 0; j < 4; ++j) {
                int nj = tet.nodes[j];
                triplets.emplace_back(ni, nj, Ke(i, j));
            }
        }
    }

    // 2. Compute heat flux if dynamically derived from heat source power
    double heat_flux = bc.heat_flux_w_m2;
    double total_heat_source_area = 0.0;
    if (bc.use_heat_source_power) {
        for (const auto& tri : mesh.boundary_faces) {
            if (tri.boundary_type == mesh::BoundaryType::HeatSource) {
                total_heat_source_area += tri.area;
            }
        }
        if (total_heat_source_area > 1e-12) {
            heat_flux = bc.heat_source_power_w / total_heat_source_area;
        }
    }
    sys.total_heat_in_w = (bc.use_heat_source_power && total_heat_source_area > 1e-12) 
                        ? bc.heat_source_power_w 
                        : (heat_flux * total_heat_source_area);

    // 3. Boundary Faces: Convection and Heat Flux
    for (const auto& tri : mesh.boundary_faces) {
        double area = tri.area;
        int n0 = tri.nodes[0];
        int n1 = tri.nodes[1];
        int n2 = tri.nodes[2];

        if (tri.boundary_type == mesh::BoundaryType::Convection) {
            // Robin Convection Matrix: h * A / 12 * [2, 1, 1; 1, 2, 1; 1, 1, 2]
            double h = bc.heat_transfer_coefficient;
            double diag_val = (h * area) / 6.0;   // 2/12
            double off_diag = (h * area) / 12.0;  // 1/12

            triplets.emplace_back(n0, n0, diag_val);
            triplets.emplace_back(n1, n1, diag_val);
            triplets.emplace_back(n2, n2, diag_val);

            triplets.emplace_back(n0, n1, off_diag);
            triplets.emplace_back(n1, n0, off_diag);
            triplets.emplace_back(n0, n2, off_diag);
            triplets.emplace_back(n2, n0, off_diag);
            triplets.emplace_back(n1, n2, off_diag);
            triplets.emplace_back(n2, n1, off_diag);

            // Robin Convection Load: h * T_inf * A / 3
            double f_conv = (h * bc.ambient_temperature_c * area) / 3.0;
            sys.F(n0) += f_conv;
            sys.F(n1) += f_conv;
            sys.F(n2) += f_conv;
        } else if (tri.boundary_type == mesh::BoundaryType::HeatSource) {
            // Neumann Heat Flux Load: q'' * A / 3
            double f_flux = (heat_flux * area) / 3.0;
            sys.F(n0) += f_flux;
            sys.F(n1) += f_flux;
            sys.F(n2) += f_flux;
        }
    }

    // Assemble Sparse Matrix
    sys.K.resize(num_nodes, num_nodes);
    sys.K.setFromTriplets(triplets.begin(), triplets.end());
    sys.K.makeCompressed();

    // 4. Fixed Dirichlet Boundary Conditions (if mounting holes are clamped)
    if (bc.fix_mounting_holes) {
        std::vector<std::pair<size_t, double>> fixed_nodes;
        for (const auto& tri : mesh.boundary_faces) {
            if (tri.boundary_type == mesh::BoundaryType::FixedTemp) {
                fixed_nodes.emplace_back(tri.nodes[0], bc.fixed_mounting_temperature_c);
                fixed_nodes.emplace_back(tri.nodes[1], bc.fixed_mounting_temperature_c);
                fixed_nodes.emplace_back(tri.nodes[2], bc.fixed_mounting_temperature_c);
            }
        }
        apply_dirichlet_bc(sys.K, sys.F, fixed_nodes);
    }

    sys.non_zeros = sys.K.nonZeros();
    auto t1 = std::chrono::high_resolution_clock::now();
    sys.assembly_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    return sys;
}

void FEM3DSolver::apply_dirichlet_bc(
    Eigen::SparseMatrix<double>& K,
    Eigen::VectorXd& F,
    const std::vector<std::pair<size_t, double>>& fixed_nodes
) {
    if (fixed_nodes.empty()) return;

    // Eliminate duplicate fixed nodes
    std::unordered_map<size_t, double> unique_fixed;
    for (const auto& fn : fixed_nodes) {
        unique_fixed[fn.first] = fn.second;
    }

    // Symmetric elimination
    for (const auto& kv : unique_fixed) {
        size_t node_idx = kv.first;
        double fixed_val = kv.second;

        for (Eigen::SparseMatrix<double>::InnerIterator it(K, node_idx); it; ++it) {
            size_t row = it.row();
            if (row != node_idx && unique_fixed.find(row) == unique_fixed.end()) {
                F(row) -= it.value() * fixed_val;
            }
        }
    }

    // Zero out rows and columns, set diagonal to 1.0, and F(node_idx) = fixed_val
    for (const auto& kv : unique_fixed) {
        size_t node_idx = kv.first;
        double fixed_val = kv.second;

        for (Eigen::SparseMatrix<double>::InnerIterator it(K, node_idx); it; ++it) {
            it.valueRef() = 0.0;
        }

        // Set diagonal to 1.0
        K.coeffRef(node_idx, node_idx) = 1.0;
        F(node_idx) = fixed_val;
    }
}

double FEM3DSolver::compute_total_heat_out(
    const mesh::TetrahedralMesh& mesh,
    const Eigen::VectorXd& nodal_temperatures_c,
    const ThermalBoundaryConditions& bc
) {
    double total_q_out = 0.0;
    double h = bc.heat_transfer_coefficient;

    for (const auto& tri : mesh.boundary_faces) {
        if (tri.boundary_type == mesh::BoundaryType::Convection) {
            double t0 = nodal_temperatures_c(tri.nodes[0]);
            double t1 = nodal_temperatures_c(tri.nodes[1]);
            double t2 = nodal_temperatures_c(tri.nodes[2]);
            double t_avg = (t0 + t1 + t2) / 3.0;
            total_q_out += h * tri.area * (t_avg - bc.ambient_temperature_c);
        }
    }
    return total_q_out;
}

} // namespace uew::thermal
