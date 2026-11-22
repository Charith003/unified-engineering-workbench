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
#include <iomanip>
#include <sstream>
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
    bool cholesky_valid = false;
    double ref_median_time_ms = 0.0;

    auto compute_median = [](std::vector<double>& v) -> double {
        if (v.empty()) return 0.0;
        std::sort(v.begin(), v.end());
        size_t n = v.size();
        if (n % 2 == 1) return v[n / 2];
        return (v[n / 2 - 1] + v[n / 2]) * 0.5;
    };

    for (size_t s_idx = 0; s_idx < solver_instances.size(); ++s_idx) {
        auto& solver = solver_instances[s_idx];
        SolverBenchmarkMetrics metrics;
        metrics.solver_name = solver->name();

        // Warm-up run (excluded from timing to prime CPU cache and stabilize frequency)
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

        metrics.trial_times_ms = total_times;

        // Compute min, max, and median across trials
        auto sorted_totals = total_times;
        std::sort(sorted_totals.begin(), sorted_totals.end());
        metrics.min_total_time_ms = sorted_totals.empty() ? 0.0 : sorted_totals.front();
        metrics.max_total_time_ms = sorted_totals.empty() ? 0.0 : sorted_totals.back();
        metrics.median_total_time_ms = compute_median(total_times);
        bool is_direct_cholesky = (solver->name().find("Cholesky") != std::string::npos);
        if (is_direct_cholesky) {
            metrics.median_factorization_time_ms = compute_median(fact_times);
            metrics.median_setup_time_ms = -1.0;
        } else {
            metrics.median_factorization_time_ms = -1.0;
            metrics.median_setup_time_ms = compute_median(fact_times);
        }
        metrics.median_solve_time_ms = compute_median(solve_times);

        metrics.success = last_res.success;
        metrics.status_message = last_res.success ? "Converged" : ("Failed: " + last_res.error_message);
        metrics.iterations = last_res.iterations;
        metrics.memory_kb = last_res.estimated_memory_bytes / 1024;

        // Calculate residual directly from the original assembled system K * x - F
        if (last_res.solution.size() == static_cast<Eigen::Index>(sys.dof_count)) {
            Eigen::VectorXd r_eval = sys.K * last_res.solution - sys.F;
            double b_norm = sys.F.norm();
            if (b_norm < 1e-15) b_norm = 1.0;
            metrics.residual_norm = r_eval.norm() / b_norm;
        } else {
            metrics.residual_norm = last_res.residual_norm;
        }

        if (s_idx == 0) {
            // First solver is Cholesky (Reference)
            if (last_res.success && last_res.solution.size() == static_cast<Eigen::Index>(sys.dof_count)) {
                cholesky_valid = true;
                ref_solution = last_res.solution;
                ref_median_time_ms = std::max(0.001, metrics.median_total_time_ms);
                metrics.speedup_vs_reference = 1.0;
                metrics.max_temp_diff_vs_reference_c = 0.0;

                // Compute global thermal properties from reference solution
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
            } else {
                cholesky_valid = false;
                metrics.success = false;
                metrics.status_message = "Cholesky reference solver failed: " + last_res.error_message;
                metrics.speedup_vs_reference = 0.0;
                metrics.max_temp_diff_vs_reference_c = -1.0;
            }
        } else {
            // Compute speedup: T_ref / T_solver (only if both reference and solver succeeded)
            if (cholesky_valid && last_res.success && metrics.median_total_time_ms > 0.0) {
                metrics.speedup_vs_reference = ref_median_time_ms / metrics.median_total_time_ms;
            } else {
                metrics.speedup_vs_reference = 0.0;
            }

            // Compute max temperature discrepancy only if both reference and current solver succeeded
            if (cholesky_valid && last_res.success && ref_solution.size() == last_res.solution.size()) {
                double max_diff = 0.0;
                for (Eigen::Index i = 0; i < ref_solution.size(); ++i) {
                    max_diff = std::max(max_diff, std::abs(ref_solution(i) - last_res.solution(i)));
                }
                metrics.max_temp_diff_vs_reference_c = max_diff;
            } else {
                metrics.max_temp_diff_vs_reference_c = -1.0; // N/A due to failure
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
        auto val_report = mesh::MeshValidator::validate(m);
        if (!val_report.is_valid) {
            throw std::runtime_error("Convergence study failed: mesh validation failed for level " + lvl.first + ": " + val_report.message);
        }

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
        pt.cholesky_converged = chol_res.success && (chol_res.solution.size() == static_cast<Eigen::Index>(m.nodes.size())) && chol_res.solution.allFinite();

        // Solve with CG
        solvers::ConjugateGradientSolver cg;
        auto cg_res = cg.solve(sys.K, sys.F);
        pt.cg_time_ms = cg_res.total_time_ms;
        pt.cg_converged = cg_res.success && (cg_res.solution.size() == static_cast<Eigen::Index>(m.nodes.size())) && cg_res.solution.allFinite();

        // Solve with Gauss-Seidel
        solvers::GaussSeidelSolver gs;
        auto gs_res = gs.solve(sys.K, sys.F);
        pt.gs_time_ms = gs_res.total_time_ms;
        pt.gs_converged = gs_res.success && (gs_res.solution.size() == static_cast<Eigen::Index>(m.nodes.size())) && gs_res.solution.allFinite();

        if (pt.cholesky_converged) {
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
        const auto& coarse = cr.points[0];
        const auto& medium = cr.points[1];
        const auto& fine = cr.points[2];

        bool all_cholesky_converged = coarse.cholesky_converged && medium.cholesky_converged && fine.cholesky_converged;
        double fine_rel_change = fine.relative_change_percent;
        cr.is_converged = all_cholesky_converged && (fine_rel_change < 2.0);
        cr.asymptotic_estimate_c = fine.max_temperature_c;

        // Calculate actual measured timing ratios from measured data
        double fine_chol_time = std::max(0.001, fine.cholesky_time_ms);
        double med_chol_time = std::max(0.001, medium.cholesky_time_ms);
        double actual_med_chol_speedup = fine_chol_time / med_chol_time;

        double fine_cg_time = std::max(0.001, fine.cg_time_ms);
        double med_cg_time = std::max(0.001, medium.cg_time_ms);
        double actual_med_cg_speedup = fine_cg_time / med_cg_time;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);

        if (!all_cholesky_converged) {
            oss << "Cholesky reference solver failed to converge at one or more mesh levels; mesh convergence is inconclusive.";
        } else if (cr.is_converged) {
            oss << "Medium mesh resolution achieves asymptotic convergence ("
                << fine_rel_change << "% relative change vs Fine, below 2.0% threshold). "
                << "Measured Medium solve speedup: " << actual_med_chol_speedup << "x (Cholesky), "
                << actual_med_cg_speedup << "x (CG). "
                << "Solvers status at Fine (" << fine.node_count << " DOFs): CG "
                << (fine.cg_converged ? "converged" : "failed") << ", SSOR "
                << (fine.gs_converged ? "converged" : "reached iteration limit") << ".";
        } else {
            oss << "Mesh has not fully converged at Fine level ("
                << fine_rel_change << "% change exceeds 2.0% asymptotic threshold). "
                << "Fine mesh resolution (" << fine.node_count << " DOFs) is recommended for high gradient accuracy. "
                << "Measured solve time increased by " << actual_med_chol_speedup << "x from Medium to Fine. "
                << "Solvers status at Fine: CG " << (fine.cg_converged ? "converged" : "failed")
                << ", SSOR " << (fine.gs_converged ? "converged" : "reached iteration limit") << ".";
        }
        cr.recommendation = oss.str();
    }

    return cr;
}

} // namespace uew::benchmark
