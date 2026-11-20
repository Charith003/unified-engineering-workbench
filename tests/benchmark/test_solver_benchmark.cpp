#include <gtest/gtest.h>
#include "core/benchmark/solver_benchmark.hpp"
#include "core/geometry/heat_sink_geometry.hpp"
#include "core/mesh/mesh_generator.hpp"

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

    for (const auto& s : report.solvers) {
        EXPECT_TRUE(s.success);
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
}
