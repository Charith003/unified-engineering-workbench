#include "core/solvers/gauss_seidel_solver.hpp"
#include <chrono>
#include <cmath>
#include <vector>
#include <sstream>
#include <iomanip>

namespace uew::solvers {

SolverResult GaussSeidelSolver::solve(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b,
    const SolverConfig& config
) {
    SolverResult res;
    res.solver_name = name();

    Eigen::Index n = b.size();
    if (A.rows() != n || A.cols() != n) {
        res.success = false;
        res.error_message = "Dimension mismatch or non-square matrix";
        return res;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // Setup: Convert to RowMajor for sequential row traversal (CSR)
    Eigen::SparseMatrix<double, Eigen::RowMajor> A_row = A;

    // Pre-extract diagonal elements and verify no near-zero diagonals
    std::vector<double> diag(n, 0.0);
    for (Eigen::Index i = 0; i < n; ++i) {
        diag[i] = A_row.coeff(i, i);
        if (!std::isfinite(diag[i]) || std::abs(diag[i]) < 1e-15) {
            res.success = false;
            res.error_message = "Gauss-Seidel failure: near-zero or non-finite diagonal element at index " + std::to_string(i);
            return res;
        }
    }

    auto t_fact = std::chrono::high_resolution_clock::now();
    res.factorization_time_ms = std::chrono::duration<double, std::milli>(t_fact - t0).count();

    // Initial guess x_0 = b / diag(A) or zero
    Eigen::VectorXd x = Eigen::VectorXd::Zero(n);
    for (Eigen::Index i = 0; i < n; ++i) {
        x(i) = b(i) / diag[i];
        if (!std::isfinite(x(i))) {
            x(i) = 0.0;
        }
    }

    double b_norm = b.norm();
    if (b_norm < 1e-15) b_norm = 1.0;

    // Compute initial residual
    Eigen::VectorXd r0 = A * x - b;
    double r0_norm = r0.norm();
    double initial_rel_res = r0_norm / b_norm;

    if (initial_rel_res <= config.tolerance) {
        auto t_end = std::chrono::high_resolution_clock::now();
        res.solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_fact).count();
        res.total_time_ms = std::chrono::duration<double, std::milli>(t_end - t0).count();
        res.solution = x;
        res.iterations = 0;
        res.absolute_residual = r0_norm;
        res.residual_norm = initial_rel_res;
        res.estimated_memory_bytes = A.nonZeros() * (sizeof(double) + sizeof(int)) + n * sizeof(double);
        res.success = true;
        return res;
    }

    double omega = config.relaxation_omega;
    bool converged = false;
    bool diverged = false;
    size_t iter = 0;

    for (iter = 1; iter <= config.max_iterations; ++iter) {
        // Forward sweep (0 to n-1)
        for (Eigen::Index i = 0; i < n; ++i) {
            double sum = 0.0;
            for (Eigen::SparseMatrix<double, Eigen::RowMajor>::InnerIterator it(A_row, i); it; ++it) {
                Eigen::Index j = it.col();
                if (j != i) {
                    sum += it.value() * x(j);
                }
            }
            double x_star = (b(i) - sum) / diag[i];
            x(i) = (1.0 - omega) * x(i) + omega * x_star;
        }

        // Backward sweep (n-1 down to 0) - Symmetric Gauss-Seidel (SSOR)
        for (Eigen::Index i = n - 1; i >= 0; --i) {
            double sum = 0.0;
            for (Eigen::SparseMatrix<double, Eigen::RowMajor>::InnerIterator it(A_row, i); it; ++it) {
                Eigen::Index j = it.col();
                if (j != i) {
                    sum += it.value() * x(j);
                }
            }
            double x_star = (b(i) - sum) / diag[i];
            x(i) = (1.0 - omega) * x(i) + omega * x_star;
        }

        // Check for non-finite values during iteration
        if (!x.allFinite()) {
            diverged = true;
            res.error_message = "Gauss-Seidel diverged: non-finite values (NaN/Inf) detected in solution vector at iteration " + std::to_string(iter);
            break;
        }

        // Check residual every 5 iterations or on the final iteration
        if (iter % 5 == 0 || iter == config.max_iterations) {
            Eigen::VectorXd r = A * x - b;
            double r_norm = r.norm();
            double rel_res = r_norm / b_norm;

            if (!std::isfinite(rel_res) || rel_res > 1e6 * std::max(1.0, initial_rel_res)) {
                diverged = true;
                res.error_message = "Gauss-Seidel diverged: relative residual exploded to " + std::to_string(rel_res) + " at iteration " + std::to_string(iter);
                res.solution = x;
                res.iterations = iter;
                res.residual_norm = rel_res;
                res.absolute_residual = r_norm;
                res.success = false;
                break;
            }

            if (rel_res <= config.tolerance) {
                converged = true;
                break;
            }
        }
    }

    auto t_solve = std::chrono::high_resolution_clock::now();
    res.solve_time_ms = std::chrono::duration<double, std::milli>(t_solve - t_fact).count();
    res.total_time_ms = std::chrono::duration<double, std::milli>(t_solve - t0).count();

    res.solution = x;
    res.iterations = std::min(iter, config.max_iterations);

    Eigen::VectorXd r_final = A * x - b;
    res.absolute_residual = r_final.norm();
    res.residual_norm = res.absolute_residual / b_norm;
    res.estimated_memory_bytes = A.nonZeros() * (sizeof(double) + sizeof(int)) + n * sizeof(double);

    if (diverged) {
        res.success = false;
        // error_message already populated
    } else if (converged) {
        res.success = true;
        res.error_message.clear();
    } else {
        // Did not converge within max iterations
        res.success = false;
        std::ostringstream ss;
        ss << std::scientific << std::setprecision(3);
        ss << "Maximum iterations reached (" << config.max_iterations
           << ") without convergence (relative residual " << res.residual_norm
           << " > tolerance " << config.tolerance << ")";
        res.error_message = ss.str();
    }

    return res;
}

} // namespace uew::solvers
