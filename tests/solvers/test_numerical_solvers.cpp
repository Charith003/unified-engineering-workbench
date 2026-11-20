#include <gtest/gtest.h>
#include "core/solvers/cholesky_solver.hpp"
#include "core/solvers/conjugate_gradient_solver.hpp"
#include "core/solvers/gauss_seidel_solver.hpp"
#include "core/thermal/fem_3d_solver.hpp"
#include "core/mesh/mesh_generator.hpp"
#include "core/geometry/heat_sink_geometry.hpp"

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

TEST_F(NumericalSolversTest, GaussSeidelConvergence) {
    GaussSeidelSolver solver;
    SolverConfig cfg;
    cfg.tolerance = 1e-4;
    cfg.max_iterations = 4000;
    cfg.relaxation_omega = 1.0;

    auto res = solver.solve(sys_.K, sys_.F, cfg);

    EXPECT_TRUE(res.success);
    EXPECT_LT(res.residual_norm, 0.05);
    EXPECT_GT(res.iterations, 0);
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

    GaussSeidelSolver gs;
    SolverConfig gs_cfg;
    gs_cfg.tolerance = 1e-4;
    gs_cfg.max_iterations = 4000;
    auto gs_res = gs.solve(sys_.K, sys_.F, gs_cfg);
    ASSERT_TRUE(gs_res.success);

    // Verify Cholesky vs Gauss-Seidel max discrepancy < 1.0 deg C (reflecting asymptotic convergence)
    double max_diff_gs = (chol_res.solution - gs_res.solution).cwiseAbs().maxCoeff();
    EXPECT_LT(max_diff_gs, 1.0);
}
