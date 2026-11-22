# Benchmark Methodology — UEW Solver Performance Study

## PASS Science Works R&D Internship 2026–2027

---

## 1. Research Question

> How do different numerical solvers perform when solving the same three-dimensional thermal FEM problem across increasing geometric and mesh complexity?

---

## 2. Experimental Design

### 2.1 Control Variables

All three solvers receive **identical** inputs:
- Same assembled sparse stiffness matrix K (from the same mesh)
- Same right-hand-side vector F (from the same boundary conditions)
- Same initial guess (zero vector)
- Same hardware (single process, same thread count)

### 2.2 Independent Variable

Solver type: Cholesky (direct), Conjugate Gradient (iterative Krylov), Symmetric Gauss-Seidel (iterative stationary).

### 2.3 Dependent Variables

| Metric              | Symbol          | Description                              |
|---------------------|-----------------|------------------------------------------|
| Solve time          | t_median [μs]   | Median wall-clock time over N trials     |
| Min / Max time      | t_min, t_max    | Trial distribution bounds                |
| Relative residual   | ‖r‖₂/‖F‖₂      | Computed from original K and F after solve |
| Max temperature deviation | max|ΔT| [K] | vs Cholesky reference (only if both converged) |
| Speedup             | S = t_Cholesky / t_solver | Relative to Cholesky direct reference |
| Memory usage        | Mem [KB]        | Approximate solver working storage       |
| Iterations          | iter            | (Iterative solvers only)                 |
| Convergence status  | success/failed  | Strictly `true` only when tolerance met before max_iterations |

---

## 3. Timing Protocol

### 3.1 Warm-Up Run

Before measurement begins, one complete solve is performed and discarded. This ensures:
- CPU caches are primed with the matrix data.
- Dynamic CPU frequency scaling (turbo boost) has stabilised.
- JIT-style OS paging effects are avoided.

### 3.2 Measured Trials

N consecutive timed solves are performed. Each trial:
1. Reset solution vector T to zero.
2. Record start time (`std::chrono::high_resolution_clock::now()`).
3. Execute solver.
4. Record end time.
5. Store elapsed duration in microseconds.

### 3.3 Timing Breakdown & Metrics Reported

To avoid misrepresenting iterative solver setup as direct matrix factorization, the benchmark rigorously separates:
- **Cholesky Factorization Time (`median_factorization_time_ms`):** Time spent performing symbolic and numeric $L D L^T$ decomposition. For iterative solvers, this is recorded as `N/A`.
- **Iterative Setup / Preconditioner Time (`median_setup_time_ms`):** Time spent computing the Jacobi diagonal inverse preconditioner or SSOR diagonal extraction. For direct Cholesky, this is recorded as `N/A`.
- **Iteration / Solve Time (`median_solve_time_ms`):** Pure execution time of Krylov inner iterations, relaxation sweeps, or triangular substitutions.
- **Median Total Time (`median_time_ms`):** The 50th percentile of total execution time ($t_{\text{setup}} + t_{\text{solve}}$) across N trials.

**Why median instead of mean?**

The mean is sensitive to outlier spikes caused by OS scheduling interruptions, garbage collection, or cache misses on the first trial. The **median** provides a robust central estimate of the true solver performance.

$$t_{\text{median}} = \text{percentile}_{50}(t_1, t_2, \ldots, t_N)$$

Default: N = 5 trials (configurable via frontend).

### 3.4 Timing Implementation

```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... solver execution ...
auto end = std::chrono::high_resolution_clock::now();
double elapsed_us = std::chrono::duration<double, std::micro>(end - start).count();
```

---

## 4. Accuracy Metrics

### 4.1 Relative Residual

$$r_{\text{rel}} = \frac{\|K\mathbf{T} - \mathbf{F}\|_2}{\|\mathbf{F}\|_2}$$

Computed after each solve from the returned solution vector.

**Acceptance thresholds:**
- Cholesky: r_rel < 1×10⁻¹⁰
- CG: r_rel < 1×10⁻⁸
- SSOR GS: Must meet configured tolerance (default 1×10⁻⁶). If not met before `max_iterations`, the solver reports `success = false`. On 3D FEM matrices with spectral radius ρ ≈ 0.999, SSOR typically reaches r_rel ≈ 0.03 at 5 000 iterations and must honestly report non-convergence.

### 4.2 Solution Agreement

The Cholesky solution is used as the gold-standard reference **only when Cholesky successfully converges**. If Cholesky fails, no reference is set and max|ΔT| is not computed for any solver.

For each converging iterative solver:

$$\max_i |T_i^{\text{solver}} - T_i^{\text{Cholesky}}|$$

**Acceptance thresholds:**
- CG vs Cholesky: max|ΔT| < 1×10⁻³ K
- SSOR GS vs Cholesky: reported as N/A when SSOR does not converge (expected on typical FEM matrices)

---

## 5. Mesh Convergence Study

The same benchmark protocol is run at three mesh resolutions:

| Level  | Description      | Purpose                        |
|--------|------------------|--------------------------------|
| Coarse | Low DOF count    | Fast sweep, coarse temperature |
| Medium | Moderate DOF     | Balance of speed and accuracy  |
| Fine   | High DOF count   | Reference solution             |

For each level, the following are recorded:
- Node count, element count, DOF count
- Max temperature (°C)
- Solver convergence status (`cholesky_converged`, `cg_converged`, `gs_converged`)
- Each solver's median total time, factorization time, setup time, iteration time, residual, and max ΔT vs reference

### 5.1 Data-Driven Mesh & Solver Recommendation

Rather than using hardcoded speedup assertions, UEW computes actual runtime ratios directly from measured benchmark data:
- Temperature shift between medium and fine grids: $\Delta T = |T_{\text{max, fine}} - T_{\text{max, med}}| / T_{\text{max, fine}} \times 100\%$
- Measured Cholesky speedup: $S_{\text{chol}} = t_{\text{chol, fine}} / t_{\text{chol, med}}$
- Measured CG speedup: $S_{\text{CG}} = t_{\text{CG, fine}} / t_{\text{CG, med}}$
- Verification of convergence status for each solver across all three mesh resolutions.

The recommendation dynamically evaluates whether the medium mesh satisfies engineering fidelity within 5% of the fine mesh while reporting the exact measured speedup factor.

---

## 6. Solver Comparison Summary

| Criterion         | Cholesky (Direct)   | CG (Krylov)         | SSOR GS (Stationary)  |
|-------------------|---------------------|---------------------|-----------------------|
| Algorithm class   | Direct factorisation| Krylov subspace     | Stationary iterative  |
| Memory            | O(n·fill)           | O(n)                | O(n)                  |
| Time complexity   | O(n^{1.5}) (sparse) | O(n·iter·nnz/n)     | O(n·iter·nnz/n)       |
| Convergence       | Exact (single step) | Superlinear          | Linear                |
| Parallelism       | Limited             | Partial (SpMV)      | Sequential (SSOR)     |
| Best for          | Small-medium systems | Large sparse systems| Demonstration / preconditioner |

---

## 7. Reproducibility

All benchmark results are computed live from C++ using the actual assembled K and F matrices. No results are hard-coded. To reproduce:

1. Start `uew_server.exe`
2. Navigate to 3D Heat Sink → Benchmark tab
3. Select mesh resolution and trial count
4. Click "Run Benchmark"

The JSON response contains all raw trial times, residuals, and solution vectors.
