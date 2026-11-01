#pragma once
#include <vector>
#include <string>

namespace uew::thermal {

/// Parameters for 1D steady-state thermal FEM
/// -d/dx(kA dT/dx) = qA
struct ThermalParams {
    double L   = 1.0;      ///< Rod length [m]
    double A   = 1e-4;     ///< Cross-sectional area [m²]
    double k   = 10.0;     ///< Thermal conductivity [W/(m·K)]
    double q   = 10000.0;  ///< Volumetric heat generation [W/m³]
    double T_L = 100.0;    ///< Left boundary temperature [°C]
    double T_R = 20.0;     ///< Right boundary temperature [°C]
};

/// Results for one mesh configuration
struct MeshResult {
    int    N;                   ///< Number of elements
    std::vector<double> x_nodes;     ///< Nodal positions [m]
    std::vector<double> T_fem;       ///< FEM nodal temperatures [°C]
    std::vector<double> T_exact;     ///< Analytical temperatures at nodes [°C]
    std::vector<double> x_midpoints; ///< Element midpoint positions [m]
    std::vector<double> T_fem_mid;   ///< FEM interpolated at midpoints [°C]
    std::vector<double> T_exact_mid; ///< Analytical at midpoints [°C]
    std::vector<double> error_mid;   ///< |T_fem_mid - T_exact_mid| [°C]
    double max_midpoint_error;       ///< Maximum absolute midpoint error [°C]
    std::string csv;                 ///< Exportable CSV string
};

/// Solve 1D FEM for a given number of elements N.
/// Uses two-node linear elements, uniform mesh, Dirichlet BCs.
MeshResult solve_fem_1d(const ThermalParams& p, int N);

/// Analytical solution: T(x) = T_L - (T_L - T_R)*x/L + q/(2k) * x*(L-x)
double analytical_temperature(const ThermalParams& p, double x);

} // namespace uew::thermal
