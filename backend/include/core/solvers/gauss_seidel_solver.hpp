#pragma once

#include "core/solvers/isolver.hpp"

namespace uew::solvers {

/**
 * @brief Symmetric Gauss-Seidel (SSOR) / Successive Over-Relaxation Iterative Solver.
 * 
 * Implements Compressed Sparse Row (CSR) traversal for high cache locality.
 * Features:
 *  - Forward sweep followed by backward sweep (SSOR) to preserve symmetry
 *    for symmetric positive definite FEM systems.
 *  - Strict stopping criterion based on relative residual norm: ||A*x - b||_2 / ||b||_2 <= tolerance.
 *  - Robust divergence detection (non-finite value check and residual explosion monitoring).
 *  - Zero-diagonal detection and reporting.
 *  - Relaxation parameter omega (omega = 1.0 is standard Symmetric Gauss-Seidel).
 */
class GaussSeidelSolver : public ISolver {
public:
    std::string name() const override { return "Symmetric Gauss-Seidel (SSOR Iterative)"; }

    SolverResult solve(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b,
        const SolverConfig& config = SolverConfig()
    ) override;
};

} // namespace uew::solvers
