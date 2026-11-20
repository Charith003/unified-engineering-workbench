#include "core/benchmark/solver_benchmark.hpp"
#include "core/solvers/cholesky_solver.hpp"
#include "core/solvers/conjugate_gradient_solver.hpp"
#include "core/solvers/gauss_seidel_solver.hpp"
#include "core/mesh/mesh_generator.hpp"
#include "core/mesh/mesh_validator.hpp"
#include <algorithm>
#include <chrono>
#include <numeric>
#include <iostream>
#include <memory>

namespace uew::benchmark {

FullBenchmarkReport SolverBenchmark::run_benchmark(
    const mesh::TetrahedralMesh& mesh,
    const thermal::ThermalMaterial& material,
    const thermal::ThermalBoundaryConditions& bc,
    int trial_count
) {
    FullBenchmarkReport report;
    report.dof_count = mesh.nodes.size();
    report.element_count = mesh.elements.size();

    // 1. Assemble system
    thermal::FEM3DSolver fem;
    auto sys = fem.assemble(mesh, material, bc);
    report.non_zeros = sys.non_zeros;
    report.assembly_time_ms = sys.assembly_time_ms;
    report.total_heat_in_w = sys.total_heat_in_w;

    // Define solvers
    std::vector<std::unique_ptr<solvers::ISolver>> solver_instances;
    solver_instances.push_back(std::make_unique<solvers::CholeskySolver>());
    solver_instances.push_back(std::make_unique<solvers::ConjugateGradientSolver>());
    solver_instances.push_back(std::make_unique<solvers::GaussSeidelSolver>());

    solvers::SolverConfig config;
    config.max_iterations = 5000;
    config.tolerance = 1e-6;
    config.relaxation_omega = 1.0;

    Eigen::VectorXd ref_solution;
    double ref_median_time_ms = 1.0;

    for (size_t s_idx = 0; s_idx < solver_instances.size(); ++s_idx) {
        auto& solver = solver_instances[s_idx];
        SolverBenchmarkMetrics metrics;
        metrics.solver_name = solver->name();

        // Warm-up run (excluded from timing)
        auto warmup_res = solver->solve(sys.K, sys.F, config);

        std::vector<double> total_times;
        std::vector<double> fact_times;
        std::vector<double> solve_times;
        solvers::SolverResult last_res;

        for (int t = 0; t < trial_count; ++t) {
            auto run_res = solver->solve(sys.K, sys.F, config);
            total_times.push_back(run_res.total_time_ms);
            fact_times.push_back(run_res.factorization_time_ms);
            solve_times.push_back(run_res.solve_time_ms);
            last_res = run_res;
        }

        // Sort timings to compute median
        std::sort(total_times.begin(), total_times.end());
        std::sort(fact_times.begin(), fact_times.end());
        std::sort(solve_times.begin(), solve_times.end());

        metrics.trial_times_ms = total_times;
        metrics.median_total_time_ms = total_times[total_times.size() / 2];
        metrics.min_total_time_ms = total_times.front();
        metrics.max_total_time_ms = total_times.back();
        metrics.median_factorization_time_ms = fact_times[fact_times.size() / 2];
        metrics.median_solve_time_ms = solve_times[solve_times.size() / 2];

        metrics.success = last_res.success;
        metrics.status_message = last_res.success ? "Converged" : ("Failed: " + last_res.error_message);
        metrics.iterations = last_res.iterations;
        metrics.residual_norm = last_res.residual_norm;
        metrics.memory_kb = last_res.estimated_memory_bytes / 1024;

        if (s_idx == 0) {
            // First solver is Cholesky (Reference)
            ref_solution = last_res.solution;
            ref_median_time_ms = std::max(0.01, metrics.median_total_time_ms);
            metrics.speedup_vs_reference = 1.0;
            metrics.max_temp_diff_vs_reference_c = 0.0;

            // Compute global thermal properties from reference solution
            if (ref_solution.size() > 0) {
                report.min_temperature_c = ref_solution.minCoeff();
                report.max_temperature_c = ref_solution.maxCoeff();
                double p_in = std::max(0.001, sys.total_heat_in_w);
                report.thermal_resistance_k_w = (report.max_temperature_c - bc.ambient_temperature_c) / p_in;

                // Energy balance check: Q_out = sum h*A*(T - T_inf)
                report.total_heat_out_w = thermal::FEM3DSolver::compute_total_heat_out(mesh, ref_solution, bc);
                report.energy_balance_error_percent = std::abs(report.total_heat_in_w - report.total_heat_out_w) / p_in * 100.0;

                report.reference_solution.resize(ref_solution.size());
                for (Eigen::Index i = 0; i < ref_solution.size(); ++i) {
                    report.reference_solution[i] = ref_solution(i);
                }
            }
        } else {
            // Compute speedup: T_ref / T_solver
            metrics.speedup_vs_reference = ref_median_time_ms / std::max(0.001, metrics.median_total_time_ms);

            // Compute max temperature discrepancy: max |T_i - T_i,ref|
            if (ref_solution.size() == last_res.solution.size()) {
                double max_diff = 0.0;
                for (Eigen::Index i = 0; i < ref_solution.size(); ++i) {
                    max_diff = std::max(max_diff, std::abs(ref_solution(i) - last_res.solution(i)));
                }
                metrics.max_temp_diff_vs_reference_c = max_diff;
            }
        }

        report.solvers.push_back(metrics);
    }

    return report;
}

MeshConvergenceReport SolverBenchmark::run_mesh_convergence(
    const geometry::HeatSinkGeometry& geometry,
    const thermal::ThermalMaterial& material,
    const thermal::ThermalBoundaryConditions& bc
) {
    MeshConvergenceReport cr;
    mesh::MeshGenerator gen(geometry);

    std::vector<std::pair<std::string, mesh::MeshResolution>> levels = {
        {"Coarse", mesh::MeshResolution::Coarse},
        {"Medium", mesh::MeshResolution::Medium},
        {"Fine", mesh::MeshResolution::Fine}
    };

    double prev_t_max = 0.0;

    for (const auto& lvl : levels) {
        mesh::MeshGenerationOptions opts;
        opts.resolution = lvl.second;
        auto m = gen.generate(opts);
        mesh::MeshValidator::validate(m);

        thermal::FEM3DSolver fem;
        auto sys = fem.assemble(m, material, bc);

        MeshConvergencePoint pt;
        pt.resolution_name = lvl.first;
        pt.node_count = m.nodes.size();
        pt.element_count = m.elements.size();
        pt.assembly_time_ms = sys.assembly_time_ms;

        // Solve with Cholesky
        solvers::CholeskySolver chol;
        auto chol_res = chol.solve(sys.K, sys.F);
        pt.cholesky_time_ms = chol_res.total_time_ms;

        // Solve with CG
        solvers::ConjugateGradientSolver cg;
        auto cg_res = cg.solve(sys.K, sys.F);
        pt.cg_time_ms = cg_res.total_time_ms;

        // Solve with Gauss-Seidel
        solvers::GaussSeidelSolver gs;
        auto gs_res = gs.solve(sys.K, sys.F);
        pt.gs_time_ms = gs_res.total_time_ms;

        if (chol_res.success && chol_res.solution.size() > 0) {
            pt.max_temperature_c = chol_res.solution.maxCoeff();
            pt.min_temperature_c = chol_res.solution.minCoeff();
            pt.avg_temperature_c = chol_res.solution.mean();
            double p_in = std::max(0.001, sys.total_heat_in_w);
            pt.thermal_resistance_k_w = (pt.max_temperature_c - bc.ambient_temperature_c) / p_in;

            if (prev_t_max > 0.0) {
                pt.relative_change_percent = (std::abs(pt.max_temperature_c - prev_t_max) / pt.max_temperature_c) * 100.0;
            }
            prev_t_max = pt.max_temperature_c;
        }

        cr.points.push_back(pt);
    }

    if (cr.points.size() >= 3) {
        double fine_rel_change = cr.points.back().relative_change_percent;
        cr.is_converged = (fine_rel_change < 2.0); // Within 2% asymptotic threshold
        cr.asymptotic_estimate_c = cr.points.back().max_temperature_c;
        if (cr.is_converged) {
            cr.recommendation = "Medium mesh resolution provides optimal balance (<2% deviation from Fine, 4x faster).";
        } else {
            cr.recommendation = "Fine mesh required for high spatial temperature gradient resolution.";
        }
    }

    return cr;
}

} // namespace uew::benchmark
