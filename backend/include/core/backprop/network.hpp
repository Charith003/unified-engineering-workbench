#pragma once
#include <vector>
#include <string>

namespace uew::backprop {

/// Network parameters (single hidden unit, regression)
/// h = tanh(w*x + b)
/// y_hat = v*h + c
/// J = 0.5*(y_hat - y)^2
struct NetParams {
    double w{0.5}, b{0.0}, v{1.0}, c{0.0};
};

struct ForwardResult {
    double h;       ///< tanh activation
    double y_hat;   ///< network output
    double J;       ///< loss
};

struct Gradients {
    double dJ_dw, dJ_db, dJ_dv, dJ_dc;
};

struct StepRecord {
    int    step;
    double w, b, v, c;
    double J;
};

struct BackpropResult {
    bool success{false};
    std::string error;

    // Single sample
    double x_input{1.0}, y_target{2.0};

    // Initial forward pass
    ForwardResult initial_forward;

    // Initial analytical gradients (computed from initial params)
    Gradients analytical_grads;

    // Finite-difference gradients
    Gradients fd_grads;
    double max_grad_error; ///< max |analytical - fd| across all 4 parameters

    // Updated parameters after one step
    NetParams updated_params;

    // Training history (100 steps)
    std::vector<StepRecord> history;

    // CSV export of loss history
    std::string loss_csv;
};

/// Run the full backprop analysis:
/// 1. Forward pass with initial params
/// 2. Compute analytical gradients
/// 3. Compute finite-difference gradients (epsilon=1e-6)
/// 4. Verify agreement (must be <= 1e-5)
/// 5. Apply one gradient-descent update (lr=0.1)
/// 6. Train for 100 steps and record loss
BackpropResult run_backprop(double lr = 0.1, int steps = 100, double epsilon = 1e-6);

/// Forward pass only
ForwardResult forward_pass(const NetParams& p, double x);

/// Analytical gradients at given params
Gradients compute_gradients(const NetParams& p, double x, double y);

/// Central finite differences (epsilon)
Gradients finite_differences(const NetParams& p, double x, double y, double eps);

} // namespace uew::backprop
