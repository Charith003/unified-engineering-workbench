#pragma once

#include <string>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Sparse>

namespace uew::solvers {

/**
 * @brief Configuration parameters for numerical linear solvers.
 */
struct SolverConfig {
    size_t max_iterations{5000};
    double tolerance{1e-6};
    double relaxation_omega{1.0}; // 1.0 = Gauss-Seidel, 1.0 < omega < 2.0 = SOR
    bool verbose{false};
};

/**
 * @brief Benchmark and diagnostic result of solving A * x = b.
 */
struct SolverResult {
    std::string solver_name;
    bool success{false};
    std::string error_message;
    size_t iterations{0};
    double residual_norm{0.0};          // ||A*x - b||_2 / ||b||_2
    double absolute_residual{0.0};       // ||A*x - b||_2
    double factorization_time_ms{0.0};
    double solve_time_ms{0.0};
    double total_time_ms{0.0};
    size_t estimated_memory_bytes{0};    // Memory footprint of solver structures
    Eigen::VectorXd solution;
};

/**
 * @brief Abstract base class for numerical linear system solvers.
 */
class ISolver {
public:
    virtual ~ISolver() = default;
    virtual std::string name() const = 0;
    virtual SolverResult solve(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b,
        const SolverConfig& config = SolverConfig()
    ) = 0;
};

} // namespace uew::solvers
