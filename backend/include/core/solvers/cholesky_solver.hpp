#pragma once

#include "core/solvers/isolver.hpp"
#include <Eigen/SparseCholesky>

namespace uew::solvers {

/**
 * @brief Direct Sparse LDLT Solver (Eigen::SimplicialLDLT).
 * Computes exact root-free Cholesky decomposition A = P^T * L * D * L^T * P.
 * Avoids square roots of diagonal pivots and guarantees numerical stability
 * on symmetric positive definite and semi-definite FEM thermal systems.
 * Serves as the gold-standard direct reference solver.
 */
class CholeskySolver : public ISolver {
public:
    std::string name() const override { return "Sparse LDLT"; }

    SolverResult solve(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b,
        const SolverConfig& config = SolverConfig()
    ) override;
};

// Type alias for explicit terminology
using SparseLDLTSolver = CholeskySolver;

} // namespace uew::solvers
