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
| Relative residual   | ‖r‖₂/‖F‖₂      | Accuracy of linear system solution       |
| Max temperature deviation | max|ΔT| [K] | vs Cholesky reference                |
| Speedup             | S = t_CG / t_x  | Relative to Conjugate Gradient           |
| Memory usage        | Mem [KB]        | Approximate solver working storage       |
| Iterations          | iter            | (Iterative solvers only)                 |

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

### 3.3 Statistic Reported: Median

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
- SSOR GS: r_rel < 0.05 (slower convergence characteristic of stationary methods)

### 4.2 Solution Agreement

The Cholesky solution is used as the gold-standard reference. For each iterative solver:

$$\max_i |T_i^{\text{solver}} - T_i^{\text{Cholesky}}|$$

**Acceptance thresholds:**
- CG vs Cholesky: max|ΔT| < 1×10⁻³ K
- SSOR GS vs Cholesky: max|ΔT| < 1.0 K

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
- Each solver's median time, residual, and max ΔT vs reference

Convergence is assessed by plotting max temperature vs mesh DOF count on a log-log scale.

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
