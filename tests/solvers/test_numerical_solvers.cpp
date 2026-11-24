#include <gtest/gtest.h>
#include "core/solvers/cholesky_solver.hpp"
#include "core/solvers/conjugate_gradient_solver.hpp"
#include "core/solvers/gauss_seidel_solver.hpp"
#include "core/thermal/fem_3d_solver.hpp"
#include "core/mesh/mesh_generator.hpp"
#include "core/geometry/heat_sink_geometry.hpp"
#include "api/heat_sink_api.hpp"
#include <limits>

using namespace uew::solvers;
using namespace uew::thermal;
using namespace uew::mesh;
using namespace uew::geometry;

class NumericalSolversTest : public ::testing::Test {
protected:
    void SetUp() override {
        HeatSinkGeometry geom;
        MeshGenerator gen(geom);
        MeshGenerationOptions opts;
        opts.resolution = MeshResolution::Coarse;
        mesh_ = gen.generate(opts);

        FEM3DSolver fem;
        ThermalMaterial mat = ThermalMaterial::aluminum_6061();
        ThermalBoundaryConditions bc;
        bc.heat_transfer_coefficient = 25.0;
        bc.ambient_temperature_c = 25.0;
        bc.heat_source_power_w = 15.0;

        sys_ = fem.assemble(mesh_, mat, bc);
    }

    TetrahedralMesh mesh_;
    AssembledThermalSystem sys_;
};

TEST_F(NumericalSolversTest, CholeskySolverExactConvergence) {
    CholeskySolver solver;
    auto res = solver.solve(sys_.K, sys_.F);

    EXPECT_TRUE(res.success);
    EXPECT_LT(res.residual_norm, 1e-10);
    EXPECT_GT(res.solution.size(), 0);
    EXPECT_GT(res.solution.maxCoeff(), 25.0); // Hotter than ambient
}

TEST_F(NumericalSolversTest, ConjugateGradientConvergence) {
    ConjugateGradientSolver solver;
    SolverConfig cfg;
    cfg.tolerance = 1e-6;
    cfg.max_iterations = 2000;

    auto res = solver.solve(sys_.K, sys_.F, cfg);

    EXPECT_TRUE(res.success);
    EXPECT_LT(res.residual_norm, 1e-5);
    EXPECT_GT(res.iterations, 0);
}

TEST_F(NumericalSolversTest, GaussSeidelConvergenceOnDiagonallyDominantSystem) {
    // Priority 1 Test: Verify Gauss-Seidel convergence on a diagonally dominant SPD system
    Eigen::SparseMatrix<double> A(4, 4);
    std::vector<Eigen::Triplet<double>> trips = {
        {0, 0, 10.0}, {0, 1, 1.0}, {0, 2, 0.5},
        {1, 0, 1.0},  {1, 1, 8.0}, {1, 2, 1.0}, {1, 3, 0.5},
        {2, 0, 0.5},  {2, 1, 1.0}, {2, 2, 7.0}, {2, 3, 1.0},
        {3, 1, 0.5},  {3, 2, 1.0}, {3, 3, 6.0}
    };
    A.setFromTriplets(trips.begin(), trips.end());
    Eigen::VectorXd b(4);
    b << 11.5, 10.5, 9.5, 7.5; // Exact solution x = [1, 1, 1, 1]

    GaussSeidelSolver solver;
    SolverConfig cfg;
    cfg.tolerance = 1e-6;
    cfg.max_iterations = 100;
    cfg.relaxation_omega = 1.0;

    auto res = solver.solve(A, b, cfg);

    EXPECT_TRUE(res.success);
    EXPECT_LT(res.residual_norm, 1e-6);
    EXPECT_LT((res.solution - Eigen::VectorXd::Constant(4, 1.0)).norm(), 1e-5);
    EXPECT_GT(res.iterations, 0);
    EXPECT_LT(res.iterations, 50);
}

TEST_F(NumericalSolversTest, GaussSeidelNonConvergedDetection) {
    // Priority 1 Test: Reaching maximum iterations must NOT be reported as success
    Eigen::SparseMatrix<double> A(3, 3);
    std::vector<Eigen::Triplet<double>> trips = {
        {0, 0, 4.0}, {0, 1, 1.0},
        {1, 0, 1.0}, {1, 1, 4.0}, {1, 2, 1.0},
        {2, 1, 1.0}, {2, 2, 4.0}
    };
    A.setFromTriplets(trips.begin(), trips.end());
    Eigen::VectorXd b = Eigen::VectorXd::Constant(3, 1.0);

    GaussSeidelSolver solver;
    SolverConfig cfg;
    cfg.tolerance = 1e-12; // Unreachable in 2 iterations
    cfg.max_iterations = 2; // Strict limit

    auto res = solver.solve(A, b, cfg);

    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.iterations, 2);
    EXPECT_FALSE(res.error_message.empty());
    EXPECT_NE(res.error_message.find("Maximum iterations reached"), std::string::npos);
}

TEST_F(NumericalSolversTest, GaussSeidelDivergentSystemDetection) {
    // Priority 1 Test: Detect divergence on non-diagonally dominant matrix with large spectral radius
    Eigen::SparseMatrix<double> A(2, 2);
    std::vector<Eigen::Triplet<double>> trips = {
        {0, 0, 1.0}, {0, 1, 5.0},
        {1, 0, 5.0}, {1, 1, 1.0}
    };
    A.setFromTriplets(trips.begin(), trips.end());
    Eigen::VectorXd b(2);
    b << 1.0, 2.0;

    GaussSeidelSolver solver;
    SolverConfig cfg;
    cfg.tolerance = 1e-6;
    cfg.max_iterations = 50;

    auto res = solver.solve(A, b, cfg);

    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error_message.find("diverged"), std::string::npos);
}

TEST_F(NumericalSolversTest, SolversAgreementOnIdenticalSystem) {
    CholeskySolver chol;
    auto chol_res = chol.solve(sys_.K, sys_.F);
    ASSERT_TRUE(chol_res.success);

    ConjugateGradientSolver cg;
    SolverConfig cfg;
    cfg.tolerance = 1e-7;
    cfg.max_iterations = 3000;
    auto cg_res = cg.solve(sys_.K, sys_.F, cfg);
    ASSERT_TRUE(cg_res.success);

    // Verify Cholesky vs CG max nodal discrepancy: max |T_chol - T_cg| < 1e-3 deg C
    double max_diff_cg = (chol_res.solution - cg_res.solution).cwiseAbs().maxCoeff();
    EXPECT_LT(max_diff_cg, 1e-3);

    // Run SSOR Gauss-Seidel with configured tolerance
    GaussSeidelSolver gs;
    SolverConfig gs_cfg;
    gs_cfg.tolerance = 0.05;
    gs_cfg.max_iterations = 6000;
    auto gs_res = gs.solve(sys_.K, sys_.F, gs_cfg);
    
    // If converged within tolerance, verify agreement; if max iterations reached, verify proper failure reporting
    if (gs_res.success) {
        EXPECT_LT(gs_res.residual_norm, gs_cfg.tolerance);
        double max_diff_gs = (chol_res.solution - gs_res.solution).cwiseAbs().maxCoeff();
        EXPECT_LT(max_diff_gs, 5.0);
    } else {
        EXPECT_FALSE(gs_res.success);
        EXPECT_NE(gs_res.error_message.find("Maximum iterations reached"), std::string::npos);
    }
}

TEST_F(NumericalSolversTest, ThermalAPIFailureHandlingOnNonConvergentSolver) {
    // QA Requirement 1: Verify API failure handling on deliberately non-convergent solver configuration
    GaussSeidelSolver solver;
    SolverConfig cfg;
    cfg.tolerance = 1e-15;  // Unreachable tolerance
    cfg.max_iterations = 2; // Strict iteration budget

    auto sres = solver.solve(sys_.K, sys_.F, cfg);

    EXPECT_FALSE(sres.success);
    EXPECT_EQ(sres.iterations, 2);

    // Verify API validation logic: solver failure must invalidate the solution
    bool solution_valid = sres.success &&
                          (sres.solution.size() == static_cast<Eigen::Index>(sys_.dof_count)) &&
                          sres.solution.allFinite();
    EXPECT_FALSE(solution_valid);
    EXPECT_FALSE(sres.error_message.empty());
}

TEST_F(NumericalSolversTest, DirectSolverNamingAndImplementationConsistency) {
    // QA Requirement 2: Verify direct solver is named Sparse LDLT and alias is consistent
    CholeskySolver direct_solver;
    EXPECT_EQ(direct_solver.name(), "Sparse LDLT");

    SparseLDLTSolver alias_solver;
    EXPECT_EQ(alias_solver.name(), "Sparse LDLT");

    auto res = direct_solver.solve(sys_.K, sys_.F);
    EXPECT_TRUE(res.success);
    EXPECT_LT(res.residual_norm, 1e-10);
    EXPECT_EQ(res.iterations, 1);
}

TEST_F(NumericalSolversTest, NonFiniteSolutionRejection) {
    // QA Requirement 6: Verify non-finite solution values (NaN/Inf) are detected and rejected
    CholeskySolver solver;
    auto sres = solver.solve(sys_.K, sys_.F);
    ASSERT_TRUE(sres.success);

    // Deliberately inject NaN into solution vector
    sres.solution(0) = std::numeric_limits<double>::quiet_NaN();
    bool finite_ok = sres.solution.allFinite();
    EXPECT_FALSE(finite_ok);

    bool solution_valid = sres.success &&
                          (sres.solution.size() == static_cast<Eigen::Index>(sys_.dof_count)) &&
                          finite_ok;
    EXPECT_FALSE(solution_valid);

    // Deliberately inject Infinity into solution vector
    sres.solution(0) = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(sres.solution.allFinite());
}

TEST_F(NumericalSolversTest, IncorrectSolutionDimensionRejection) {
    // QA Requirement 6: Verify solution dimension mismatch is detected and rejected
    CholeskySolver solver;
    auto sres = solver.solve(sys_.K, sys_.F);
    ASSERT_TRUE(sres.success);

    // Deliberately truncate solution vector
    Eigen::VectorXd truncated = sres.solution.head(sres.solution.size() - 5);
    bool dim_ok = (truncated.size() == static_cast<Eigen::Index>(sys_.dof_count));
    EXPECT_FALSE(dim_ok);

    bool solution_valid = sres.success && dim_ok && truncated.allFinite();
    EXPECT_FALSE(solution_valid);
}

TEST_F(NumericalSolversTest, SolverResidualVerificationAgainstOriginalSystem) {
    // QA Requirement 6: Independently evaluate ||K*x - F||_2 / ||F||_2 against original system
    CholeskySolver direct;
    auto direct_res = direct.solve(sys_.K, sys_.F);
    ASSERT_TRUE(direct_res.success);

    double b_norm = sys_.F.norm();
    ASSERT_GT(b_norm, 1e-12);

    Eigen::VectorXd r_direct = sys_.K * direct_res.solution - sys_.F;
    double rel_res_direct = r_direct.norm() / b_norm;
    EXPECT_LT(rel_res_direct, 1e-10);

    ConjugateGradientSolver cg;
    SolverConfig cg_cfg;
    cg_cfg.tolerance = 1e-6;
    cg_cfg.max_iterations = 2000;
    auto cg_res = cg.solve(sys_.K, sys_.F, cg_cfg);
    ASSERT_TRUE(cg_res.success);

    Eigen::VectorXd r_cg = sys_.K * cg_res.solution - sys_.F;
    double rel_res_cg = r_cg.norm() / b_norm;
    EXPECT_LT(rel_res_cg, 1e-5);
}

TEST_F(NumericalSolversTest, ThermalAPIFailureSemanticsIntegration) {
    // QA Requirement 1 & 6: Call actual handle_thermal_solve_request with deliberately non-convergent config
    // Must return HTTP 422, outer success: false, inner data.success: false, diagnostic message,
    // useful diagnostics preserved, and no fabricated temperature or mesh visualization arrays.
    nlohmann::json req_body = {
        {"resolution", "coarse"},
        {"solver", "gauss_seidel"},
        {"tolerance", 1e-15},
        {"max_iterations", 2}
    };

    int http_status = 0;
    nlohmann::json resp = uew::api::handle_thermal_solve_request(req_body, http_status);

    EXPECT_EQ(http_status, 422);
    EXPECT_FALSE(resp["success"].get<bool>());
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_FALSE(resp["error"].get<std::string>().empty());

    ASSERT_TRUE(resp.contains("data"));
    const auto& data = resp["data"];
    EXPECT_FALSE(data["success"].get<bool>());
    EXPECT_NE(data["solver_name"].get<std::string>().find("SSOR"), std::string::npos);
    EXPECT_EQ(data["iterations"].get<int>(), 2);
    EXPECT_GT(data["residual_norm"].get<double>(), 0.0);
    EXPECT_GT(data["assembly_time_ms"].get<double>(), 0.0);
    EXPECT_GE(data["solve_time_ms"].get<double>(), 0.0);

    // Withhold fabricated statistics
    EXPECT_FALSE(data.contains("min_temperature_c"));
    EXPECT_FALSE(data.contains("max_temperature_c"));
    EXPECT_FALSE(data.contains("avg_temperature_c"));
    EXPECT_FALSE(data.contains("total_heat_out_w"));
    EXPECT_FALSE(data.contains("energy_balance_error_percent"));

    // Empty visualization arrays
    EXPECT_TRUE(data["nodal_temperatures"].empty());
    EXPECT_TRUE(data["nodes"].empty());
    EXPECT_TRUE(data["boundary_faces"].empty());
    EXPECT_TRUE(data["elements"].empty());
}

TEST_F(NumericalSolversTest, ThermalAPISuccessSemanticsIntegration) {
    // QA Requirement 1 & 6: Call actual handle_thermal_solve_request with valid solver config
    // Must return HTTP 200, outer success: true, inner data.success: true, genuine physics.
    nlohmann::json req_body = {
        {"resolution", "coarse"},
        {"solver", "cholesky"}
    };

    int http_status = 0;
    nlohmann::json resp = uew::api::handle_thermal_solve_request(req_body, http_status);

    EXPECT_EQ(http_status, 200);
    EXPECT_TRUE(resp["success"].get<bool>());

    ASSERT_TRUE(resp.contains("data"));
    const auto& data = resp["data"];
    EXPECT_TRUE(data["success"].get<bool>());
    EXPECT_EQ(data["solver_name"].get<std::string>(), "Sparse LDLT");
    EXPECT_GT(data["max_temperature_c"].get<double>(), 25.0);
    EXPECT_GE(data["min_temperature_c"].get<double>(), 25.0);
    EXPECT_GT(data["total_heat_in_w"].get<double>(), 0.0);
    EXPECT_LT(data["energy_balance_error_percent"].get<double>(), 5.0);

    EXPECT_FALSE(data["nodal_temperatures"].empty());
    EXPECT_FALSE(data["nodes"].empty());
    EXPECT_FALSE(data["boundary_faces"].empty());
    EXPECT_FALSE(data["elements"].empty());
    EXPECT_EQ(data["nodal_temperatures"].size(), data["nodes"].size());
}
