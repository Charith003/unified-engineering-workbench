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

Solver type:
- **Sparse LDLT (Direct Reference):** Root-free Cholesky factorization ($A = P^T L D L^T P$) via `Eigen::SimplicialLDLT`.
- **Conjugate Gradient (Iterative Krylov):** Jacobi-preconditioned Krylov subspace solver.
- **Symmetric Gauss-Seidel / SSOR (Iterative Stationary):** Symmetric forward/backward relaxation sweeps.

### 2.3 Dependent Variables

| Metric              | Symbol          | Description                              |
|---------------------|-----------------|------------------------------------------|
| Total solve time    | t_median [ms]   | Median total wall-clock time over N trials |
| Min / Max time      | t_min, t_max    | Measured trial distribution bounds       |
| Factorization time  | t_fact [ms]     | Direct solver numerical factorization only (Sparse LDLT) |
| Setup time          | t_setup [ms]    | Preconditioner/diagonal inversion setup (CG / SSOR) |
| Iteration time      | t_solve [ms]    | Triangular solve or Krylov iteration loop |
| Relative residual   | ‖r‖₂/‖F‖₂      | Independently evaluated from original assembled K and F |
| Max temperature deviation | max|ΔT| [K] | vs Sparse LDLT reference (only if both converged) |
| Speedup             | S = t_LDLT / t_solver | Relative to Sparse LDLT direct reference |
| Memory usage        | Mem [KB]        | Measured working storage                 |
| Iterations          | iter            | (Iterative solvers only)                 |
| Convergence status  | success/failed  | Strictly `true` only when all trials satisfy criteria |

---

## 3. Timing Protocol

### 3.1 Warm-Up Run

Before measurement begins, one complete solve is performed and discarded. This ensures:
- CPU caches are primed with the matrix data.
- Dynamic CPU frequency scaling (turbo boost) has stabilised.
- JIT-style OS paging effects are avoided.

### 3.2 Measured Trials & Multi-Trial Validation Protocol

N consecutive timed solves are performed. Each trial undergoes rigorous validation:
1. Reset solution vector $\mathbf{T}$ to zero.
2. Record high-resolution start timestamp.
3. Execute solver on identical assembled matrix $K$ and load vector $F$.
4. Record high-resolution end timestamp.
5. **Solution validity checks on every trial:**
   - Dimension check: $\text{size}(\mathbf{T}) == \text{dof\_count}$.
   - Finite value check: $\mathbf{T}$ must be 100% free of NaN and Inf values.
   - **Independent residual evaluation:** Rather than relying on solver self-reporting, the benchmark engine independently computes:
     $$r_{\text{rel}} = \frac{\|K \mathbf{T} - F\|_2}{\|F\|_2}$$
     directly from the original assembled system.
   - For iterative solvers, require $r_{\text{rel}} \le \text{tolerance} \times 1.5$.
6. **Strict All-Trial Success Requirement:**
   - A solver is only reported as `success = true` if **all $N$ attempted trials** satisfy convergence criteria.
   - A successful final trial **cannot** mask an earlier trial that failed.
   - If any trial fails, `success` is set to `false`, reporting the exact failure count (e.g. `1/5 trials failed`) and clarifying that the median timing represents all attempted trials.
   - Individual trial timings (`trial_times_ms`) and trial residuals (`trial_residuals`) are fully preserved in the API report.

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

The same benchmark protocol is run across three geometric discretisation levels:

| Level  | Description      | Node Count | Element Count | Purpose                        |
|--------|------------------|------------|---------------|--------------------------------|
| Coarse | Fast screening   | ~590 DOFs  | ~1,745 tets   | Rapid sweep & baseline checks  |
| Medium | Engineering bal. | ~1,944 DOFs| ~7,128 tets   | Optimal accuracy / speed trade |
| Fine   | Reference mesh   | ~5,880 DOFs| ~23,808 tets  | Detailed gradient capture      |

### 5.1 Multi-Metric Convergence Evaluation

To avoid claiming mesh independence solely based on a single scalar value, UEW calculates four distinct relative metrics between successive mesh levels:

1. **Relative Maximum Temperature Change:**
   $$\Delta T_{\max} = \frac{|T_{\max}^{(i)} - T_{\max}^{(i-1)}|}{T_{\max}^{(i)}} \times 100\%$$
2. **Relative Average Temperature Change:**
   $$\Delta T_{\text{avg}} = \frac{|T_{\text{avg}}^{(i)} - T_{\text{avg}}^{(i-1)}|}{T_{\text{avg}}^{(i)}} \times 100\%$$
3. **Relative Thermal Resistance Change:**
   $$\Delta R_{\text{th}} = \frac{|R_{\text{th}}^{(i)} - R_{\text{th}}^{(i-1)}|}{R_{\text{th}}^{(i)}} \times 100\%$$
4. **Base-Domain Sampled Relative $L_2$ Discrepancy:**
   Evaluated over an independent spatial probe grid of 600 sample points distributed across the solid base domain. Temperature is sampled via approximate 4-node local Inverse Distance Weighting (IDW) interpolation:
   $$\varepsilon_{\text{field}} = \frac{\sqrt{\sum_{k=1}^{600} (T^{(i)}(\mathbf{p}_k) - T^{(i-1)}(\mathbf{p}_k))^2}}{\sqrt{\sum_{k=1}^{600} (T^{(i)}(\mathbf{p}_k))^2}} \times 100\%$$
   *Note: This is an approximate base-domain sampled field comparison, not a full-domain 3D element-level interpolation.*

### 5.2 Distinct Convergence Classifications

The engine strictly distinguishes:
- **Solver Convergence:** Did the numerical solvers (Sparse LDLT and CG) converge to the specified tolerance on the discretized matrix at this mesh level based on independent residual verification? (Yes, verified at all 3 levels).
- **Peak Temperature Convergence:** Has the maximum hot-spot temperature stabilized within engineering tolerance ($\Delta T_{\max} < 2.0\%$)?
- **Field-Level Convergence:** Has the sampled base-domain distribution stabilized ($\varepsilon_{\text{field}} < 5.0\%$)?

### 5.3 Fine-Grid Limit & GCI Limitation Notice

Three mesh levels are insufficient to rigorously establish asymptotic Grid Convergence Index (GCI) extrapolation or Richardson error bands without higher-order refinement. The fine-grid peak temperature ($40.54^\circ\text{C}$) is reported as the highest-resolution discrete result, not a proven mathematical asymptotic limit. The engine transparently reports this limitation rather than fabricating mathematical claims.

### 5.4 Data-Driven Timing Comparison

Timing recommendations explicitly identify the metric being evaluated: **Median Total Solve Time**. The engine reports actual measured ratios (e.g. Medium mesh Sparse LDLT solve is 3.8x faster than Fine mesh) rather than arbitrary assertions.

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
