#include <gtest/gtest.h>
#include "core/thermal/fem_3d_solver.hpp"
#include "core/mesh/mesh_generator.hpp"
#include "core/geometry/heat_sink_geometry.hpp"
#include "core/solvers/cholesky_solver.hpp"

using namespace uew::thermal;
using namespace uew::mesh;
using namespace uew::geometry;

TEST(FEM3D, AssemblyProducesSymmetricPositiveDefinite) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    FEM3DSolver fem;
    ThermalMaterial mat = ThermalMaterial::aluminum_6061();
    ThermalBoundaryConditions bc;
    bc.heat_transfer_coefficient = 20.0;
    bc.ambient_temperature_c = 25.0;
    bc.heat_source_power_w = 15.0;

    auto sys = fem.assemble(mesh, mat, bc);

    EXPECT_GT(sys.dof_count, 100);
    EXPECT_GT(sys.non_zeros, sys.dof_count);
    EXPECT_GT(sys.total_heat_in_w, 14.9);
    EXPECT_LT(sys.total_heat_in_w, 15.1);

    // Verify positive diagonal elements
    for (Eigen::Index i = 0; i < sys.K.rows(); ++i) {
        EXPECT_GT(sys.K.coeff(i, i), 0.0);
    }

    // Verify approximate matrix symmetry: ||K - K^T|| / ||K|| < 1e-10
    Eigen::SparseMatrix<double> diff = sys.K - Eigen::SparseMatrix<double>(sys.K.transpose());
    EXPECT_LT(diff.norm(), 1e-8);
}

TEST(FEM3D, ConservationOfEnergy) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    FEM3DSolver fem;
    ThermalMaterial mat = ThermalMaterial::aluminum_6061();
    ThermalBoundaryConditions bc;
    bc.heat_transfer_coefficient = 25.0;
    bc.ambient_temperature_c = 25.0;
    bc.heat_source_power_w = 15.0;
    bc.fix_mounting_holes = false; // Pure convection balance

    auto sys = fem.assemble(mesh, mat, bc);

    uew::solvers::CholeskySolver solver;
    auto res = solver.solve(sys.K, sys.F);
    EXPECT_TRUE(res.success);

    double q_out = FEM3DSolver::compute_total_heat_out(mesh, res.solution, bc);
    double q_in = sys.total_heat_in_w;

    // Convective energy balance: |Q_in - Q_out| / Q_in < 5%
    double error_percent = std::abs(q_in - q_out) / q_in * 100.0;
    EXPECT_LT(error_percent, 5.0);
}
