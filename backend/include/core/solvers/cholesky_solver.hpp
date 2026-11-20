#pragma once

#include "core/solvers/isolver.hpp"
#include <Eigen/SparseCholesky>

namespace uew::solvers {

/**
 * @brief Direct Sparse Cholesky Solver (Eigen::SimplicialLDLT).
 * Computes exact decomposition A = P^T * L * D * L^T * P.
 * Optimal baseline for direct solution with zero iterative residual drift.
 */
class CholeskySolver : public ISolver {
public:
    std::string name() const override { return "Sparse Cholesky (Direct LDLT)"; }

    SolverResult solve(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b,
        const SolverConfig& config = SolverConfig()
    ) override;
};

} // namespace uew::solvers
