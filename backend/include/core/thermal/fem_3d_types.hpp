#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "core/mesh/mesh_types.hpp"

namespace uew::thermal {

/**
 * @brief Material properties for 3D thermal analysis.
 */
struct ThermalMaterial {
    std::string name{"Aluminum 6061-T6"};
    double thermal_conductivity{205.0}; // W/(m*K)
    double density{2700.0};              // kg/m^3
    double specific_heat{896.0};         // J/(kg*K)

    static ThermalMaterial aluminum_6061() {
        return {"Aluminum 6061-T6", 205.0, 2700.0, 896.0};
    }

    static ThermalMaterial copper() {
        return {"Pure Copper", 398.0, 8960.0, 385.0};
    }
};

/**
 * @brief Boundary conditions for steady-state 3D thermal simulation.
 */
struct ThermalBoundaryConditions {
    double ambient_temperature_c{25.0};        // Ambient temp T_inf in Celsius
    double ambient_temperature_k{298.15};      // Ambient temp in Kelvin
    double heat_transfer_coefficient{15.0};    // Convective h in W/(m^2*K) (free/natural convection)
    double heat_source_power_w{15.0};          // Heat source power in Watts
    double heat_flux_w_m2{20000.0};            // Applied surface flux (W/m^2) if specified directly
    bool use_heat_source_power{true};          // Compute flux dynamically from patch area: q = P/A
    double fixed_mounting_temperature_c{30.0}; // Dirichlet boundary temp (if mounting holes clamped)
    bool fix_mounting_holes{false};            // Whether mounting holes are fixed Dirichlet
    double internal_generation_w_m3{0.0};      // Volumetric Q in W/m^3 (if any)
};

/**
 * @brief Assembled linear system K * T = F.
 */
struct AssembledThermalSystem {
    Eigen::SparseMatrix<double> K; // SPD global stiffness matrix
    Eigen::VectorXd F;              // Global thermal load vector
    size_t dof_count{0};
    size_t non_zeros{0};
    double assembly_time_ms{0.0};
    double total_heat_in_w{0.0};   // Total input heat power (Watts)
};

/**
 * @brief Result of 3D FEM Thermal Simulation.
 */
struct ThermalSimulation3DResult {
    bool success{false};
    std::string error_message;
    size_t node_count{0};
    size_t element_count{0};
    double min_temperature_c{0.0};
    double max_temperature_c{0.0};
    double average_temperature_c{0.0};
    double heat_sink_thermal_resistance_k_w{0.0}; // R_th = (T_max - T_ambient) / Power
    double total_heat_in_w{0.0};
    double total_heat_out_w{0.0};
    double energy_balance_error_percent{0.0};     // |Q_in - Q_out| / Q_in * 100%

    std::vector<double> nodal_temperatures_c;      // Temperature at each node (Celsius)
    
    // Performance metrics
    double mesh_time_ms{0.0};
    double assembly_time_ms{0.0};
    double solver_time_ms{0.0};
    double total_time_ms{0.0};
};

} // namespace uew::thermal
