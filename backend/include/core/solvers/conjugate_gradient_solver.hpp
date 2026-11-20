#pragma once

#include "core/solvers/isolver.hpp"
#include <Eigen/IterativeLinearSolvers>

namespace uew::solvers {

/**
 * @brief Conjugate Gradient Solver with Diagonal (Jacobi) Preconditioning.
 * Optimal Krylov subspace iterative solver for Symmetric Positive-Definite (SPD) systems.
 */
class ConjugateGradientSolver : public ISolver {
public:
    std::string name() const override { return "Conjugate Gradient (Jacobi Preconditioned)"; }

    SolverResult solve(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b,
        const SolverConfig& config = SolverConfig()
    ) override;
};

} // namespace uew::solvers
