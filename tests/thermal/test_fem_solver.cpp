#include <gtest/gtest.h>
#include "core/thermal/fem_solver.hpp"
#include <cmath>

using namespace uew::thermal;

static const ThermalParams DEFAULT_P;

// Helpers
static double tol_rel(double fem, double exact, double tol) {
    return std::abs(fem - exact) <= tol;
}

// ─── Boundary conditions ─────────────────────────────────
TEST(ThermalFEM, LeftBoundaryCorrect) {
    auto r = solve_fem_1d(DEFAULT_P, 10);
    EXPECT_NEAR(r.T_fem.front(), DEFAULT_P.T_L, 1e-10);
}

TEST(ThermalFEM, RightBoundaryCorrect) {
    auto r = solve_fem_1d(DEFAULT_P, 10);
    EXPECT_NEAR(r.T_fem.back(), DEFAULT_P.T_R, 1e-10);
}

// ─── Analytical solution at x=0.5 ────────────────────────
TEST(ThermalFEM, AnalyticalSolutionAtMidpoint) {
    // T(0.5) = 100 - 80*0.5 + 10000/(2*10) * 0.5 * 0.5 = 100 - 40 + 125 = 185 °C
    double T_exact = analytical_temperature(DEFAULT_P, 0.5);
    EXPECT_NEAR(T_exact, 185.0, 1e-10);
}

// ─── Mesh convergence: midpoint error should decrease with N ─
TEST(ThermalFEM, MeshConvergence) {
    double prev_error = 1e10;
    for (int N : {10, 20, 40, 80}) {
        auto r = solve_fem_1d(DEFAULT_P, N);
        EXPECT_LT(r.max_midpoint_error, prev_error)
            << "Error did not decrease at N=" << N;
        prev_error = r.max_midpoint_error;
    }
}

// ─── Discretization midpoint error matches theoretical 125/N^2 ──
TEST(ThermalFEM, N10TheoreticalErrorExact) {
    auto r = solve_fem_1d(DEFAULT_P, 10);
    // Theoretical midpoint error = (h^2 / 8) * (q / k) = (0.01 / 8) * 1000 = 1.25 °C
    EXPECT_NEAR(r.max_midpoint_error, 1.25, 1e-9)
        << "N=10 midpoint error: " << r.max_midpoint_error;
}

TEST(ThermalFEM, N80TheoreticalErrorExact) {
    auto r = solve_fem_1d(DEFAULT_P, 80);
    // Theoretical midpoint error = 125.0 / (80 * 80) = 0.01953125 °C
    EXPECT_NEAR(r.max_midpoint_error, 125.0 / (80.0 * 80.0), 1e-9)
        << "N=80 midpoint error: " << r.max_midpoint_error;
}

// ─── Nodal count matches N+1 ──────────────────────────────
TEST(ThermalFEM, NodeCountCorrect) {
    for (int N : {10, 20, 40}) {
        auto r = solve_fem_1d(DEFAULT_P, N);
        EXPECT_EQ(static_cast<int>(r.x_nodes.size()), N+1);
        EXPECT_EQ(static_cast<int>(r.T_fem.size()),   N+1);
        EXPECT_EQ(static_cast<int>(r.x_midpoints.size()), N);
    }
}

// ─── Midpoint positions correct ───────────────────────────
TEST(ThermalFEM, MidpointPositionsCorrect) {
    int N = 10;
    double h = DEFAULT_P.L / N;
    auto r = solve_fem_1d(DEFAULT_P, N);
    for (int e = 0; e < N; ++e) {
        EXPECT_NEAR(r.x_midpoints[e], (e + 0.5) * h, 1e-12);
    }
}

// ─── CSV non-empty ────────────────────────────────────────
TEST(ThermalFEM, CSVNonEmpty) {
    auto r = solve_fem_1d(DEFAULT_P, 10);
    EXPECT_FALSE(r.csv.empty());
    EXPECT_NE(r.csv.find("x_m"), std::string::npos);
}
