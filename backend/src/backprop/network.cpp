#include "core/backprop/network.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace uew::backprop {

// ─────────────────────────────────────────────────────────
// Forward pass
// h     = tanh(w*x + b)
// y_hat = v*h + c
// J     = 0.5*(y_hat - y)^2
// ─────────────────────────────────────────────────────────
ForwardResult forward_pass(const NetParams& p, double x) {
    double z    = p.w * x + p.b;
    double h    = std::tanh(z);
    double y    = 2.0;   // fixed target
    double yhat = p.v * h + p.c;
    double J    = 0.5 * (yhat - y) * (yhat - y);
    return {h, yhat, J};
}

// ─────────────────────────────────────────────────────────
// Analytical gradients
//
// dJ/dc = (y_hat - y)
// dJ/dv = (y_hat - y) * h
// dJ/db = (y_hat - y) * v * (1 - h^2)
// dJ/dw = (y_hat - y) * v * (1 - h^2) * x
//
// All four use the ORIGINAL (pre-update) parameter values.
// ─────────────────────────────────────────────────────────
Gradients compute_gradients(const NetParams& p, double x, double y) {
    double z    = p.w * x + p.b;
    double h    = std::tanh(z);
    double yhat = p.v * h + p.c;
    double delta = yhat - y;
    double sech2 = 1.0 - h * h;   // derivative of tanh

    Gradients g;
    g.dJ_dc = delta;
    g.dJ_dv = delta * h;
    g.dJ_db = delta * p.v * sech2;
    g.dJ_dw = delta * p.v * sech2 * x;
    return g;
}

// ─────────────────────────────────────────────────────────
// Central finite differences
// dJ/dp ≈ [J(p + eps) - J(p - eps)] / (2*eps)
// ─────────────────────────────────────────────────────────
static double loss(const NetParams& p, double x) {
    auto fwd = forward_pass(p, x);
    return fwd.J;
}

Gradients finite_differences(const NetParams& p, double x, double y, double eps) {
    (void)y; // target is embedded in forward_pass
    Gradients fd;

    // dJ/dw
    {
        NetParams pp = p; pp.w += eps;
        NetParams pm = p; pm.w -= eps;
        fd.dJ_dw = (loss(pp, x) - loss(pm, x)) / (2.0 * eps);
    }
    // dJ/db
    {
        NetParams pp = p; pp.b += eps;
        NetParams pm = p; pm.b -= eps;
        fd.dJ_db = (loss(pp, x) - loss(pm, x)) / (2.0 * eps);
    }
    // dJ/dv
    {
        NetParams pp = p; pp.v += eps;
        NetParams pm = p; pm.v -= eps;
        fd.dJ_dv = (loss(pp, x) - loss(pm, x)) / (2.0 * eps);
    }
    // dJ/dc
    {
        NetParams pp = p; pp.c += eps;
        NetParams pm = p; pm.c -= eps;
        fd.dJ_dc = (loss(pp, x) - loss(pm, x)) / (2.0 * eps);
    }
    return fd;
}

// ─────────────────────────────────────────────────────────
// Full backprop analysis
// ─────────────────────────────────────────────────────────
BackpropResult run_backprop(double lr, int steps, double eps) {
    BackpropResult res;
    res.x_input  = 1.0;
    res.y_target = 2.0;

    const double x = res.x_input;
    const double y = res.y_target;

    NetParams p0{0.5, 0.0, 1.0, 0.0};

    // ── Initial forward pass ─────────────────────────────
    res.initial_forward = forward_pass(p0, x);

    // ── Analytical gradients (from initial params) ────────
    res.analytical_grads = compute_gradients(p0, x, y);

    // ── Finite-difference gradients ────────────────────────
    res.fd_grads = finite_differences(p0, x, y, eps);

    // ── Gradient verification ──────────────────────────────
    double err_w = std::abs(res.analytical_grads.dJ_dw - res.fd_grads.dJ_dw);
    double err_b = std::abs(res.analytical_grads.dJ_db - res.fd_grads.dJ_db);
    double err_v = std::abs(res.analytical_grads.dJ_dv - res.fd_grads.dJ_dv);
    double err_c = std::abs(res.analytical_grads.dJ_dc - res.fd_grads.dJ_dc);
    res.max_grad_error = std::max({err_w, err_b, err_v, err_c});

    if (res.max_grad_error > 1e-5) {
        res.success = false;
        res.error   = "Gradient verification failed: max error " +
                      std::to_string(res.max_grad_error) + " > 1e-5";
        return res;
    }

    // ── One simultaneous gradient-descent update ──────────
    // All gradients computed from ORIGINAL params p0
    res.updated_params.w = p0.w - lr * res.analytical_grads.dJ_dw;
    res.updated_params.b = p0.b - lr * res.analytical_grads.dJ_db;
    res.updated_params.v = p0.v - lr * res.analytical_grads.dJ_dv;
    res.updated_params.c = p0.c - lr * res.analytical_grads.dJ_dc;

    // ── Training loop (100 steps) ──────────────────────────
    NetParams p = p0;
    res.history.reserve(steps);

    for (int step = 0; step < steps; ++step) {
        auto fwd = forward_pass(p, x);
        auto g   = compute_gradients(p, x, y);

        StepRecord rec;
        rec.step = step;
        rec.w = p.w; rec.b = p.b; rec.v = p.v; rec.c = p.c;
        rec.J = fwd.J;
        res.history.push_back(rec);

        // Simultaneous update (use current p for all gradients, then update)
        p.w -= lr * g.dJ_dw;
        p.b -= lr * g.dJ_db;
        p.v -= lr * g.dJ_dv;
        p.c -= lr * g.dJ_dc;
    }

    // ── Loss CSV ──────────────────────────────────────────
    std::ostringstream csv;
    csv << std::fixed << std::setprecision(10);
    csv << "step,w,b,v,c,loss\n";
    for (const auto& r : res.history) {
        csv << r.step << "," << r.w << "," << r.b << ","
            << r.v << "," << r.c << "," << r.J << "\n";
    }
    res.loss_csv = csv.str();

    res.success = true;
    return res;
}

} // namespace uew::backprop
