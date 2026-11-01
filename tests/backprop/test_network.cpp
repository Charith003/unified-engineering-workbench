#include <gtest/gtest.h>
#include "core/backprop/network.hpp"
#include <cmath>

using namespace uew::backprop;

// ─── Forward pass values ──────────────────────────────────
TEST(Backprop, ForwardPassInitial) {
    NetParams p{0.5, 0.0, 1.0, 0.0};
    auto fwd = forward_pass(p, 1.0);
    // h = tanh(0.5*1 + 0) = tanh(0.5) ≈ 0.46212
    double expected_h    = std::tanh(0.5);
    double expected_yhat = 1.0 * expected_h + 0.0;
    double expected_J    = 0.5 * (expected_yhat - 2.0) * (expected_yhat - 2.0);
    EXPECT_NEAR(fwd.h,     expected_h,    1e-12);
    EXPECT_NEAR(fwd.y_hat, expected_yhat, 1e-12);
    EXPECT_NEAR(fwd.J,     expected_J,    1e-12);
}

// ─── Analytical gradients finite-difference agreement ─────
TEST(Backprop, GradientVerification) {
    NetParams p{0.5, 0.0, 1.0, 0.0};
    auto analytical = compute_gradients(p, 1.0, 2.0);
    auto fd         = finite_differences(p, 1.0, 2.0, 1e-6);

    EXPECT_NEAR(analytical.dJ_dw, fd.dJ_dw, 1e-5);
    EXPECT_NEAR(analytical.dJ_db, fd.dJ_db, 1e-5);
    EXPECT_NEAR(analytical.dJ_dv, fd.dJ_dv, 1e-5);
    EXPECT_NEAR(analytical.dJ_dc, fd.dJ_dc, 1e-5);
}

// ─── Gradient direction: moves toward lower loss ──────────
TEST(Backprop, GradientDirectionReducesLoss) {
    NetParams p{0.5, 0.0, 1.0, 0.0};
    auto g    = compute_gradients(p, 1.0, 2.0);
    double lr = 0.1;
    NetParams p2;
    p2.w = p.w - lr * g.dJ_dw;
    p2.b = p.b - lr * g.dJ_db;
    p2.v = p.v - lr * g.dJ_dv;
    p2.c = p.c - lr * g.dJ_dc;
    auto fwd0 = forward_pass(p,  1.0);
    auto fwd1 = forward_pass(p2, 1.0);
    EXPECT_LT(fwd1.J, fwd0.J);
}

// ─── 100-step training reduces loss ──────────────────────
TEST(Backprop, TrainingReducesLoss) {
    auto r = run_backprop(0.1, 100, 1e-6);
    EXPECT_TRUE(r.success) << r.error;
    ASSERT_EQ(static_cast<int>(r.history.size()), 100);
    EXPECT_LT(r.history.back().J, r.history.front().J);
}

// ─── Loss is non-negative ────────────────────────────────
TEST(Backprop, LossNonNegative) {
    auto r = run_backprop(0.1, 100, 1e-6);
    ASSERT_TRUE(r.success);
    for (const auto& h : r.history) {
        EXPECT_GE(h.J, 0.0);
    }
}

// ─── Simultaneous update: all gradients from original params ─
TEST(Backprop, SimultaneousUpdateCorrect) {
    NetParams p{0.5, 0.0, 1.0, 0.0};
    auto g   = compute_gradients(p, 1.0, 2.0);
    double lr = 0.1;
    auto r = run_backprop(lr, 100, 1e-6);
    ASSERT_TRUE(r.success);
    EXPECT_NEAR(r.updated_params.w, p.w - lr * g.dJ_dw, 1e-12);
    EXPECT_NEAR(r.updated_params.b, p.b - lr * g.dJ_db, 1e-12);
    EXPECT_NEAR(r.updated_params.v, p.v - lr * g.dJ_dv, 1e-12);
    EXPECT_NEAR(r.updated_params.c, p.c - lr * g.dJ_dc, 1e-12);
}

// ─── Loss CSV non-empty and has header ───────────────────
TEST(Backprop, LossCSVValid) {
    auto r = run_backprop(0.1, 100, 1e-6);
    ASSERT_TRUE(r.success);
    EXPECT_FALSE(r.loss_csv.empty());
    EXPECT_NE(r.loss_csv.find("step"), std::string::npos);
}

// ─── Verification max error <= 1e-5 ──────────────────────
TEST(Backprop, VerificationPasses) {
    auto r = run_backprop(0.1, 100, 1e-6);
    EXPECT_TRUE(r.success);
    EXPECT_LE(r.max_grad_error, 1e-5);
}
