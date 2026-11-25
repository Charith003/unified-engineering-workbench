#include <gtest/gtest.h>
#include "core/benchmark/solver_benchmark.hpp"
#include "core/geometry/heat_sink_geometry.hpp"
#include "core/mesh/mesh_generator.hpp"
#include "core/solvers/gauss_seidel_solver.hpp"

using namespace uew::benchmark;
using namespace uew::geometry;
using namespace uew::mesh;
using namespace uew::thermal;

TEST(SolverBenchmark, FullBenchmarkRun) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    SolverBenchmark benchmark;
    ThermalMaterial mat = ThermalMaterial::aluminum_6061();
    ThermalBoundaryConditions bc;
    bc.heat_transfer_coefficient = 25.0;
    bc.ambient_temperature_c = 25.0;
    bc.heat_source_power_w = 15.0;

    auto report = benchmark.run_benchmark(mesh, mat, bc, 3);

    EXPECT_EQ(report.solvers.size(), 3);
    EXPECT_GT(report.dof_count, 100);
    EXPECT_GT(report.max_temperature_c, 25.0);
    EXPECT_GT(report.thermal_resistance_k_w, 0.0);
    EXPECT_LT(report.energy_balance_error_percent, 5.0);

    EXPECT_TRUE(report.solvers[0].success); // Cholesky reference succeeds
    EXPECT_TRUE(report.solvers[1].success); // Conjugate Gradient succeeds
    // Gauss-Seidel accurately reports its convergence/iteration status without fabricated success
    EXPECT_FALSE(report.solvers[2].status_message.empty());

    for (const auto& s : report.solvers) {
        EXPECT_GT(s.median_total_time_ms, 0.0);
        EXPECT_EQ(s.trial_times_ms.size(), 3);
    }
}

TEST(SolverBenchmark, MeshConvergenceStudy) {
    HeatSinkGeometry geom;
    SolverBenchmark benchmark;
    ThermalMaterial mat = ThermalMaterial::aluminum_6061();
    ThermalBoundaryConditions bc;
    bc.heat_transfer_coefficient = 25.0;
    bc.ambient_temperature_c = 25.0;
    bc.heat_source_power_w = 15.0;

    auto cr = benchmark.run_mesh_convergence(geom, mat, bc);

    EXPECT_EQ(cr.points.size(), 3);
    // Monotonic node scaling
    EXPECT_LT(cr.points[0].node_count, cr.points[1].node_count);
    EXPECT_LT(cr.points[1].node_count, cr.points[2].node_count);

    // Monotonic element scaling
    EXPECT_LT(cr.points[0].element_count, cr.points[1].element_count);
    EXPECT_LT(cr.points[1].element_count, cr.points[2].element_count);

    // Temperature bounds sanity
    for (const auto& pt : cr.points) {
        EXPECT_GT(pt.max_temperature_c, 25.0);
        EXPECT_LT(pt.max_temperature_c, 150.0);
        EXPECT_GT(pt.thermal_resistance_k_w, 0.0);
    }

    // QA Requirement 4: Multi-metric convergence verification
    // Point 0 (Coarse) has no prior mesh, so relative changes are 0
    EXPECT_EQ(cr.points[0].rel_max_temp_change_percent, 0.0);

    // Points 1 and 2 (Medium, Fine) have measured relative changes
    EXPECT_GT(cr.points[1].rel_max_temp_change_percent, 0.0);
    EXPECT_GT(cr.points[1].rel_field_l2_difference_percent, 0.0);
    EXPECT_GT(cr.points[2].rel_max_temp_change_percent, 0.0);
    EXPECT_GT(cr.points[2].rel_field_l2_difference_percent, 0.0);

    // Verification of convergence distinction
    EXPECT_TRUE(cr.solver_converged); // Sparse LDLT and CG converged
    EXPECT_TRUE(cr.max_temp_converged); // Fine rel max temp change < 2.0%
    EXPECT_TRUE(cr.field_converged); // Fine rel field L2 diff < 5.0%

    // Fine-grid maximum temperature verified
    EXPECT_GT(cr.fine_grid_max_temperature_c, 25.0);
    EXPECT_DOUBLE_EQ(cr.fine_grid_max_temperature_c, cr.points[2].max_temperature_c);
    EXPECT_DOUBLE_EQ(cr.asymptotic_estimate_c, cr.points[2].max_temperature_c); // Backwards compatibility check

    // Audit Requirement 1 & 2: SSOR convergence status and independent residual verification at all mesh points
    for (size_t i = 0; i < cr.points.size(); ++i) {
        const auto& pt = cr.points[i];
        EXPECT_TRUE(pt.cholesky_converged) << "Sparse LDLT must converge at level " << pt.resolution_name;
        EXPECT_TRUE(pt.cg_converged) << "CG must converge at level " << pt.resolution_name;
        // SSOR spectral radius rho ~ 0.999 on 3D FEM prevents reaching tol=1e-6 within 5000 iterations:
        // verify that SSOR is honestly reported as NOT converged across all levels
        EXPECT_FALSE(pt.gs_converged) << "SSOR must not be reported as converged at level " << pt.resolution_name;
    }

    // Recommendation contains data-driven timing comparison, base-domain sampled discrepancy, and extrapolation limitation
    EXPECT_FALSE(cr.recommendation.empty());
    EXPECT_NE(cr.recommendation.find("Median Total Solve Time"), std::string::npos);
    EXPECT_NE(cr.recommendation.find("Base-Domain Sampled Field Discrepancy"), std::string::npos);
    EXPECT_NE(cr.recommendation.find("Extrapolation Limitation"), std::string::npos);
}

TEST(SolverBenchmark, SSORResidualStrictValidationAudit) {
    // Audit Requirement 1 & 2: Test that a residual of 8.94e-5 (or any > 1e-6) fails when tolerance is 1e-6
    double measured_residual = 8.94e-5;
    double strict_tolerance = 1e-6;

    // Direct solver requires <= 1e-9; CG requires <= 1.5e-6; SSOR requires <= tolerance
    bool passes_strict_tolerance = (measured_residual <= strict_tolerance);
    EXPECT_FALSE(passes_strict_tolerance) << "A residual of 8.94e-5 must never be accepted under tolerance 1e-6";

    // Rejection simulation through SolverConfig
    uew::solvers::SolverConfig cfg;
    cfg.tolerance = strict_tolerance;
    cfg.max_iterations = 200; // Small iteration budget

    Eigen::SparseMatrix<double> A(2, 2);
    std::vector<Eigen::Triplet<double>> trips = {{0, 0, 2.0}, {0, 1, 0.5}, {1, 0, 0.5}, {1, 1, 2.0}};
    A.setFromTriplets(trips.begin(), trips.end());
    Eigen::VectorXd b = Eigen::VectorXd::Constant(2, 1.0);

    uew::solvers::GaussSeidelSolver solver;
    // Deliberately tight tolerance that requires more iterations than allowed
    cfg.tolerance = 1e-12;
    cfg.max_iterations = 2;
    auto res = solver.solve(A, b, cfg);
    EXPECT_FALSE(res.success);
    EXPECT_GT(res.residual_norm, cfg.tolerance);
}

TEST(SolverBenchmark, AllBenchmarkTrialsValidationAndResidualAudit) {
    // QA Requirement 3: Every trial validated independently, no hiding earlier failures
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    SolverBenchmark benchmark;
    ThermalMaterial mat = ThermalMaterial::aluminum_6061();
    ThermalBoundaryConditions bc;
    bc.heat_transfer_coefficient = 25.0;
    bc.ambient_temperature_c = 25.0;
    bc.heat_source_power_w = 15.0;

    const int trials = 4;
    auto report = benchmark.run_benchmark(mesh, mat, bc, trials);

    // Direct solver is named Sparse LDLT
    EXPECT_EQ(report.solvers[0].solver_name, "Sparse LDLT");
    EXPECT_EQ(report.solvers[0].successful_trials, static_cast<size_t>(trials));
    EXPECT_EQ(report.solvers[0].total_trials, static_cast<size_t>(trials));
    EXPECT_EQ(report.solvers[0].trial_residuals.size(), static_cast<size_t>(trials));
    for (double res : report.solvers[0].trial_residuals) {
        EXPECT_LT(res, 1e-10); // True independent residual
    }

    // Conjugate Gradient trial verification
    EXPECT_EQ(report.solvers[1].successful_trials, static_cast<size_t>(trials));
    EXPECT_EQ(report.solvers[1].total_trials, static_cast<size_t>(trials));
    EXPECT_EQ(report.solvers[1].trial_residuals.size(), static_cast<size_t>(trials));
    for (double res : report.solvers[1].trial_residuals) {
        EXPECT_LT(res, 1e-5);
    }

    // SSOR Gauss-Seidel trial verification: accurately reports non-convergence
    // when iteration limit is reached across trials
    EXPECT_FALSE(report.solvers[2].success);
    EXPECT_EQ(report.solvers[2].total_trials, static_cast<size_t>(trials));
    EXPECT_NE(report.solvers[2].status_message.find("Median timing represents all 4 attempted trials"), std::string::npos);
}
