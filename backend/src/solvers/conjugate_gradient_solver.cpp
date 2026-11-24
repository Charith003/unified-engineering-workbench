#include "core/solvers/conjugate_gradient_solver.hpp"
#include <chrono>

namespace uew::solvers {

SolverResult ConjugateGradientSolver::solve(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b,
    const SolverConfig& config
) {
    SolverResult res;
    res.solver_name = name();

    if (A.rows() != b.size() || A.rows() != A.cols()) {
        res.success = false;
        res.error_message = "Dimension mismatch or non-square matrix";
        return res;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper, Eigen::DiagonalPreconditioner<double>> cg;
    cg.setMaxIterations(static_cast<Eigen::Index>(config.max_iterations));
    cg.setTolerance(config.tolerance);

    cg.compute(A);
    auto t_fact = std::chrono::high_resolution_clock::now();
    res.factorization_time_ms = std::chrono::duration<double, std::milli>(t_fact - t0).count();

    if (cg.info() != Eigen::Success) {
        res.success = false;
        res.error_message = "Conjugate gradient preconditioner computation failed";
        return res;
    }

    res.solution = cg.solve(b);
    auto t_solve = std::chrono::high_resolution_clock::now();
    res.solve_time_ms = std::chrono::duration<double, std::milli>(t_solve - t_fact).count();
    res.total_time_ms = std::chrono::duration<double, std::milli>(t_solve - t0).count();

    res.iterations = static_cast<size_t>(cg.iterations());

    // Solution dimension and finite value validation
    if (res.solution.size() != b.size() || !res.solution.allFinite()) {
        res.success = false;
        res.error_message = "Conjugate Gradient produced non-finite or incorrect dimension solution";
        return res;
    }
    
    // Independent residual verification: ||A*x - b||_2 / ||b||_2 with safe near-zero handling
    Eigen::VectorXd r = A * res.solution - b;
    double b_norm = b.norm();
    res.absolute_residual = r.norm();
    res.residual_norm = (b_norm > 1e-15) ? (res.absolute_residual / b_norm) : res.absolute_residual;

    // Check finite residual and convergence tolerance (with documented 1.5x allowance for floating point / Krylov residual drift)
    double allowed_tol = std::max(1e-12, config.tolerance * 1.5);
    bool residual_ok = std::isfinite(res.residual_norm) && (res.residual_norm <= allowed_tol);

    res.success = (cg.info() == Eigen::Success) && residual_ok;
    if (!residual_ok && res.error_message.empty()) {
        res.error_message = "Conjugate Gradient residual (" + std::to_string(res.residual_norm) + ") exceeded tolerance (" + std::to_string(config.tolerance) + ")";
    }

    // Memory: A matrix + ~4 vectors of size N (x, r, p, z)
    res.estimated_memory_bytes = A.nonZeros() * (sizeof(double) + sizeof(int)) + 4 * b.size() * sizeof(double);

    return res;
}

} // namespace uew::solvers
