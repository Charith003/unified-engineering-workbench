#pragma once

#include <vector>
#include <string>
#include "core/solvers/isolver.hpp"
#include "core/thermal/fem_3d_types.hpp"
#include "core/thermal/fem_3d_solver.hpp"
#include "core/mesh/mesh_types.hpp"
#include "core/geometry/heat_sink_geometry.hpp"

namespace uew::benchmark {

/**
 * @brief Benchmark metrics for a single solver over multiple trials.
 */
struct SolverBenchmarkMetrics {
    std::string solver_name;
    bool success{false};
    std::string status_message;
    size_t iterations{0};
    double residual_norm{0.0};
    double max_temp_diff_vs_reference_c{0.0}; // max |T_i - T_i,ref|
    
    // Timing in milliseconds (computed over measured runs, 1 warmup excluded)
    double median_total_time_ms{0.0};
    double min_total_time_ms{0.0};
    double max_total_time_ms{0.0};

    // Factorization time (only for direct solvers like Cholesky; -1.0 if N/A)
    double median_factorization_time_ms{-1.0};

    // Preconditioner or solver setup time (for iterative solvers like CG and SSOR; -1.0 if N/A)
    double median_setup_time_ms{-1.0};

    // Pure iteration or solve time (solve step for direct, iteration loop for iterative solvers)
    double median_solve_time_ms{0.0};

    double speedup_vs_reference{1.0}; // T_ref / T_solver

    size_t memory_kb{0};
    std::vector<double> trial_times_ms;
};

/**
 * @brief Comprehensive benchmark comparison report for all three solvers.
 */
struct FullBenchmarkReport {
    std::string mesh_resolution;
    size_t dof_count{0};
    size_t element_count{0};
    size_t non_zeros{0};
    double assembly_time_ms{0.0};
    double total_heat_in_w{0.0};
    double total_heat_out_w{0.0};
    double energy_balance_error_percent{0.0};

    double min_temperature_c{0.0};
    double max_temperature_c{0.0};
    double thermal_resistance_k_w{0.0};

    std::vector<SolverBenchmarkMetrics> solvers;
    std::vector<double> reference_solution; // Cholesky solution vector
};

/**
 * @brief Data point for a single mesh level in the mesh convergence study.
 */
struct MeshConvergencePoint {
    std::string resolution_name;
    size_t node_count{0};
    size_t element_count{0};
    double max_temperature_c{0.0};
    double min_temperature_c{0.0};
    double avg_temperature_c{0.0};
    double thermal_resistance_k_w{0.0};
    double relative_change_percent{0.0}; // Relative to previous resolution
    double assembly_time_ms{0.0};
    double cholesky_time_ms{0.0};
    double cg_time_ms{0.0};
    double gs_time_ms{0.0};
    bool cholesky_converged{false};
    bool cg_converged{false};
    bool gs_converged{false};
};

/**
 * @brief Result of mesh convergence analysis across Coarse, Medium, and Fine meshes.
 */
struct MeshConvergenceReport {
    std::vector<MeshConvergencePoint> points;
    bool is_converged{false};
    double asymptotic_estimate_c{0.0};
    std::string recommendation;
};

/**
 * @brief Benchmark Engine executing fair, isolated, multi-trial numerical comparisons.
 */
class SolverBenchmark {
public:
    SolverBenchmark() = default;

    /**
     * @brief Runs repeated benchmark trials comparing Cholesky, CG, and Gauss-Seidel on the same assembled system.
     */
    FullBenchmarkReport run_benchmark(
        const mesh::TetrahedralMesh& mesh,
        const thermal::ThermalMaterial& material,
        const thermal::ThermalBoundaryConditions& bc,
        int trial_count = 5
    );

    /**
     * @brief Performs full mesh convergence study across Coarse, Medium, and Fine meshes.
     */
    MeshConvergenceReport run_mesh_convergence(
        const geometry::HeatSinkGeometry& geometry,
        const thermal::ThermalMaterial& material,
        const thermal::ThermalBoundaryConditions& bc
    );
};

} // namespace uew::benchmark
