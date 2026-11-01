#include "core/thermal/fem_solver.hpp"
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace uew::thermal {

// ─────────────────────────────────────────────────────────
// Analytical solution
// T(x) = T_L + (T_R - T_L)*x/L + q/(2k) * x*(L - x)
// ─────────────────────────────────────────────────────────
double analytical_temperature(const ThermalParams& p, double x) {
    return p.T_L + (p.T_R - p.T_L) * x / p.L
           + p.q / (2.0 * p.k) * x * (p.L - x);
}

// ─────────────────────────────────────────────────────────
// Thomas algorithm (tridiagonal solver — TDMA)
// Solves A*T = f where A is tridiagonal.
// a: sub-diagonal (size n-1), b: main diagonal (size n), c: super-diagonal (size n-1)
// ─────────────────────────────────────────────────────────
static std::vector<double> tdma(std::vector<double> a, std::vector<double> b,
                                 std::vector<double> c, std::vector<double> f) {
    int n = static_cast<int>(b.size());
    // Forward sweep
    for (int i = 1; i < n; ++i) {
        double m = a[i-1] / b[i-1];
        b[i] -= m * c[i-1];
        f[i] -= m * f[i-1];
    }
    // Back substitution
    std::vector<double> x(n);
    x[n-1] = f[n-1] / b[n-1];
    for (int i = n-2; i >= 0; --i) {
        x[i] = (f[i] - c[i] * x[i+1]) / b[i];
    }
    return x;
}

// ─────────────────────────────────────────────────────────
// 1D FEM solver — steady-state heat conduction
//
// Element stiffness: Ke = (kA/h) * [[1,-1],[-1,1]]
// Element load:      Fe = (qAh/2) * [1,1]
//
// After assembly the global system is tridiagonal.
// Dirichlet BCs applied by row-zeroing and setting diagonal = 1.
//
// Debugging insight:
//   Early versions evaluated error at mesh NODES where the FEM solution is
//   exact (interpolation nodes). Moving error evaluation to ELEMENT MIDPOINTS
//   (FE interpolated, not re-solved) correctly reveals the O(h²) convergence.
// ─────────────────────────────────────────────────────────
MeshResult solve_fem_1d(const ThermalParams& p, int N) {
    if (N < 1) throw std::invalid_argument("N must be >= 1");

    const int    n_nodes = N + 1;            // total degrees of freedom
    const double h       = p.L / N;          // element length [m]
    const double kAh     = p.k * p.A / h;    // stiffness coefficient
    const double qAh2    = p.q * p.A * h / 2.0; // load coefficient per node

    // ── Assemble global tridiagonal system ──────────────────
    std::vector<double> diag  (n_nodes, 0.0);
    std::vector<double> upper (n_nodes-1, 0.0);
    std::vector<double> lower (n_nodes-1, 0.0);
    std::vector<double> rhs   (n_nodes, 0.0);

    for (int e = 0; e < N; ++e) {
        int i = e;   // left node
        int j = e+1; // right node

        // Ke contribution
        diag[i]  += kAh;
        diag[j]  += kAh;
        upper[i] -= kAh; // K[i][j]
        lower[i] -= kAh; // K[j][i]

        // Fe contribution
        rhs[i] += qAh2;
        rhs[j] += qAh2;
    }

    // ── Apply Dirichlet BCs by row replacement ───────────────
    // Node 0: T = T_L
    diag[0]  = 1.0;
    upper[0] = 0.0;
    rhs[0]   = p.T_L;
    // Fix rhs[1] for the lower band
    if (n_nodes > 1) {
        rhs[1] -= lower[0] * p.T_L;
        lower[0] = 0.0;
    }

    // Node N: T = T_R
    diag[N]   = 1.0;
    lower[N-1] = 0.0;
    rhs[N]    = p.T_R;
    // Fix rhs[N-1] for the upper band
    if (N > 0) {
        rhs[N-1] -= upper[N-1] * p.T_R;
        upper[N-1] = 0.0;
    }

    // ── Solve ────────────────────────────────────────────────
    std::vector<double> T_fem = tdma(lower, diag, upper, rhs);

    // ── Build result ─────────────────────────────────────────
    MeshResult res;
    res.N = N;

    // Nodal positions and temperatures
    res.x_nodes.resize(n_nodes);
    res.T_fem  .resize(n_nodes);
    res.T_exact.resize(n_nodes);
    for (int i = 0; i < n_nodes; ++i) {
        res.x_nodes[i] = i * h;
        res.T_fem[i]   = T_fem[i];
        res.T_exact[i] = analytical_temperature(p, res.x_nodes[i]);
    }

    // ── Midpoint error (correct convergence test) ─────────────
    res.x_midpoints .resize(N);
    res.T_fem_mid   .resize(N);
    res.T_exact_mid .resize(N);
    res.error_mid   .resize(N);
    res.max_midpoint_error = 0.0;

    for (int e = 0; e < N; ++e) {
        double x_mid = (e + 0.5) * h;
        // FE interpolation at midpoint: shape functions N1=N2=0.5
        double T_fe_mid = 0.5 * T_fem[e] + 0.5 * T_fem[e+1];
        double T_ex_mid = analytical_temperature(p, x_mid);
        double err      = std::abs(T_fe_mid - T_ex_mid);

        res.x_midpoints[e]  = x_mid;
        res.T_fem_mid[e]    = T_fe_mid;
        res.T_exact_mid[e]  = T_ex_mid;
        res.error_mid[e]    = err;
        res.max_midpoint_error = std::max(res.max_midpoint_error, err);
    }

    // ── Generate CSV ─────────────────────────────────────────
    std::ostringstream csv;
    csv << std::fixed << std::setprecision(8);
    csv << "x_m,T_fem_C,T_exact_C,T_fem_mid_C,T_exact_mid_C,error_mid_C\n";
    // Nodes
    for (int i = 0; i < n_nodes; ++i) {
        csv << res.x_nodes[i] << "," << res.T_fem[i] << "," << res.T_exact[i];
        if (i < N) {
            csv << "," << res.T_fem_mid[i] << "," << res.T_exact_mid[i] << "," << res.error_mid[i];
        }
        csv << "\n";
    }
    res.csv = csv.str();

    return res;
}

} // namespace uew::thermal
