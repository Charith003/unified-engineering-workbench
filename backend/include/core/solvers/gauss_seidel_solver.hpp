#pragma once

#include "core/solvers/isolver.hpp"

namespace uew::solvers {

/**
 * @brief Sparse Gauss-Seidel / Successive Over-Relaxation (SOR) Iterative Solver.
 * Implements direct Compressed Sparse Row (CSR) traversal for high memory locality.
 * Features zero-diagonal guards, divergence detection, and relaxation tuning (omega).
 */
class GaussSeidelSolver : public ISolver {
public:
    std::string name() const override { return "Gauss-Seidel (CSR Iterative)"; }

    SolverResult solve(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b,
        const SolverConfig& config = SolverConfig()
    ) override;
};

} // namespace uew::solvers
