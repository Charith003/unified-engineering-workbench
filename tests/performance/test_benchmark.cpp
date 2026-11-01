#include <gtest/gtest.h>
#include "core/performance/benchmark.hpp"
#include <cmath>
#include <algorithm>
#include <thread>

using namespace uew::performance;

// ─── Reproducibility ─────────────────────────────────────
TEST(Performance, FixedSeedReproducible) {
    auto a = generate_batch(20, 42);
    auto b = generate_batch(20, 42);
    ASSERT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        EXPECT_DOUBLE_EQ(a[i].k, b[i].k);
        EXPECT_DOUBLE_EQ(a[i].q, b[i].q);
    }
}

// ─── Different seeds give different batches ───────────────
TEST(Performance, DifferentSeedsDiffer) {
    auto a = generate_batch(10, 42);
    auto b = generate_batch(10, 99);
    bool same = true;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].k != b[i].k) { same = false; break; }
    }
    EXPECT_FALSE(same);
}

// ─── Serial deterministic ordering ────────────────────────
TEST(Performance, SerialOrderingDeterministic) {
    auto cases = generate_batch(5, 42);
    auto r = run_serial(cases, 10);
    ASSERT_EQ(r.case_results.size(), cases.size());
    for (size_t i = 0; i < cases.size(); ++i) {
        EXPECT_EQ(r.case_results[i].case_id, cases[i].case_id);
    }
}

// ─── Parallel/serial temperature agreement ───────────────
TEST(Performance, SerialParallelAgreement) {
    auto cases = generate_batch(10, 42);
    auto ser = run_serial(cases, 10);
    auto par = run_parallel(cases, 10, 2);

    double max_diff = 0.0;
    for (size_t i = 0; i < cases.size(); ++i) {
        double d = std::abs(ser.case_results[i].T_max - par.case_results[i].T_max);
        max_diff = std::max(max_diff, d);
    }
    EXPECT_LE(max_diff, 1e-8)
        << "Max serial/parallel temperature difference: " << max_diff << " °C";
}

// ─── Positive conductivity and heat generation ────────────
TEST(Performance, BatchValuesPositive) {
    auto cases = generate_batch(50, 7);
    for (const auto& c : cases) {
        EXPECT_GT(c.k, 0.0);
        EXPECT_GT(c.q, 0.0);
    }
}

// ─── Workers capped at hardware concurrency ──────────────
TEST(Performance, WorkerCapRespected) {
    auto cases = generate_batch(4, 1);
    int max_threads = static_cast<int>(std::thread::hardware_concurrency());
    auto r = run_parallel(cases, 5, max_threads + 100);
    EXPECT_LE(r.num_workers, max_threads);
}
