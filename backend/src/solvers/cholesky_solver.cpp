#include "core/solvers/cholesky_solver.hpp"
#include <chrono>
#include <iostream>

namespace uew::solvers {

SolverResult CholeskySolver::solve(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b,
    const SolverConfig& config
) {
    SolverResult res;
    res.solver_name = name();
    res.iterations = 1; // Direct solver resolves in 1 step

    if (A.rows() != b.size() || A.rows() != A.cols()) {
        res.success = false;
        res.error_message = "Dimension mismatch or non-square matrix";
        return res;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // Direct sparse LDLT factorization (root-free Cholesky variant)
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> ldlt;
    ldlt.compute(A);

    auto t_fact = std::chrono::high_resolution_clock::now();
    res.factorization_time_ms = std::chrono::duration<double, std::milli>(t_fact - t0).count();

    if (ldlt.info() != Eigen::Success) {
        res.success = false;
        res.error_message = "Sparse LDLT factorization failed (matrix may be singular or indefinite)";
        return res;
    }

    // Solve L*y = b, D*L^T*x = y
    res.solution = ldlt.solve(b);

    auto t_solve = std::chrono::high_resolution_clock::now();
    res.solve_time_ms = std::chrono::duration<double, std::milli>(t_solve - t_fact).count();
    res.total_time_ms = std::chrono::duration<double, std::milli>(t_solve - t0).count();

    if (ldlt.info() != Eigen::Success) {
        res.success = false;
        res.error_message = "Sparse LDLT back-substitution solve failed";
        return res;
    }

    // Residual computation: ||A*x - b||_2
    Eigen::VectorXd r = A * res.solution - b;
    double b_norm = b.norm();
    res.absolute_residual = r.norm();
    res.residual_norm = (b_norm > 1e-15) ? (res.absolute_residual / b_norm) : res.absolute_residual;

    // Memory estimation: sparsity factor representation
    res.estimated_memory_bytes = A.nonZeros() * (sizeof(double) + sizeof(int)) * 2;
    res.success = true;

    return res;
}

} // namespace uew::solvers
