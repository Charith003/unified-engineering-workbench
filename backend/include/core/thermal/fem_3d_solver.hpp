#pragma once

#include "core/thermal/fem_3d_types.hpp"
#include "core/mesh/mesh_types.hpp"

namespace uew::thermal {

/**
 * @brief 3D Steady-State Thermal Finite Element Method Engine.
 * Governing PDE: -\nabla \cdot (k \nabla T) = Q
 * Boundary Conditions:
 *   - Convection (Robin): -k dT/dn = h (T - T_inf)
 *   - Heat Flux (Neumann): -k dT/dn = -q''
 *   - Fixed Temperature (Dirichlet): T = T_0
 */
class FEM3DSolver {
public:
    FEM3DSolver() = default;

    /**
     * @brief Assembles the global stiffness matrix K and thermal load vector F.
     */
    AssembledThermalSystem assemble(
        const mesh::TetrahedralMesh& mesh,
        const ThermalMaterial& material,
        const ThermalBoundaryConditions& bc
    );

    /**
     * @brief Computes the 4x4 element conduction stiffness matrix for a linear tetrahedron.
     * K^e_ij = k * V_e * (grad N_i . grad N_j)
     */
    static Eigen::Matrix4d compute_element_stiffness(
        const mesh::TetrahedralMesh& mesh,
        const mesh::TetElement& tet,
        double thermal_conductivity
    );

    /**
     * @brief Computes the 4x1 element internal heat generation vector.
     * F^e_i = Q * V_e / 4
     */
    static Eigen::Vector4d compute_element_load(
        const mesh::TetrahedralMesh& mesh,
        const mesh::TetElement& tet,
        double internal_generation_Q
    );

    /**
     * @brief Computes total convective heat dissipation from surface temperatures.
     * Q_out = sum_{faces} h * Area * ( (T1 + T2 + T3)/3 - T_inf )
     */
    static double compute_total_heat_out(
        const mesh::TetrahedralMesh& mesh,
        const Eigen::VectorXd& nodal_temperatures_c,
        const ThermalBoundaryConditions& bc
    );

    /**
     * @brief Applies symmetric Dirichlet boundary conditions to preserve SPD properties.
     */
    static void apply_dirichlet_bc(
        Eigen::SparseMatrix<double>& K,
        Eigen::VectorXd& F,
        const std::vector<std::pair<size_t, double>>& fixed_nodes
    );
};

} // namespace uew::thermal
