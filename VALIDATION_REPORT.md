# Validation Report — UEW 3D Thermal FEM Engine

## PASS Science Works R&D Internship 2026–2027

---

## 1. Validation Strategy

Validation confirms that the implemented FEM engine produces physically correct results for known problems. Three levels of validation are applied:

1. **Unit tests** — component-level correctness.
2. **System tests** — assembled pipeline produces SPD matrices and physically plausible temperatures.
3. **Energy balance** — global conservation check.

---

## 2. Unit Test Results

All tests are compiled into `test_all_phases.exe`.

### 2.1 Geometry Tests (`test_heat_sink_geometry.cpp`) — 8 tests

| Test | Result |
|------|--------|
| DefaultParamsValid | ✓ PASS |
| FacetCountNonZero | ✓ PASS |
| STLExportNonEmpty | ✓ PASS |
| ValidationAcceptsDefault | ✓ PASS |
| ValidationRejectsZeroBase | ✓ PASS |
| ValidationRejectsNegativeK | ✓ PASS |
| FinConfigConsistency | ✓ PASS |
| VolumeConsistencyWithPhysicalDimensions | ✓ PASS |

### 2.2 Mesh Generator Tests (`test_mesh_generator.cpp`) — 6 tests

| Test | Result |
|------|--------|
| CoarseMeshGeneration (inverted_elements == 0) | ✓ PASS |
| MediumMeshGeneration | ✓ PASS |
| FineMeshGeneration | ✓ PASS |
| HeatSourceTopSurfaceLocationAndArea | ✓ PASS |
| BottomFaceHasConvection | ✓ PASS |
| MeshValidation_ValidMesh | ✓ PASS |

### 2.3 Mesh Validator Tests (`test_mesh_validator.cpp`) — 4 tests

| Test | Result |
|------|--------|
| DetectsOutOfRangeNodeIndex | ✓ PASS |
| DetectsNonFiniteCoordinates | ✓ PASS |
| DetectsDegenerateDuplicateNodesInTet | ✓ PASS |
| MeshValidation_DetectsCorruptedMesh | ✓ PASS |

### 2.4 Thermal FEM Tests (`test_fem_3d.cpp`) — 3 tests

| Test | Criterion | Result |
|------|-----------|--------|
| SPD_check | All eigenvalues > 0 | ✓ PASS |
| Energy_conservation | |Q_in − Q_conv| / Q_in < 5% | ✓ PASS |
| ManufacturedLinearSolutionPatchTest | max_err < 1×10⁻¹⁰ | ✓ PASS |

**Manufactured Patch Test:** Applied T(x,y,z) = 2x + 3y + z as a known linear solution. Dirichlet BCs at boundary nodes enforced. After FEM solve, maximum nodal error was < 1×10⁻¹⁰ °C, confirming that the symmetric Dirichlet enforcement and B-matrix gradient computation are correct.

### 2.5 Solver & API Tests (`test_numerical_solvers.cpp`) — 15 tests

| Test | Criterion | Result |
|------|-----------|--------|
| Cholesky_residual | ‖r‖/‖F‖ < 1e-10 | ✓ PASS |
| CG_convergence | Converges in < 10,000 iters | ✓ PASS |
| GaussSeidelConvergenceOnDiagonallyDominantSystem | success = true on simple diagonal-dominant system | ✓ PASS |
| GaussSeidelNonConvergedDetection | success = false when max_iterations reached | ✓ PASS |
| GaussSeidelDivergentSystemDetection | success = false and divergence flagged | ✓ PASS |
| SolversAgreementOnIdenticalSystem | CG max|ΔT| < 1e-3 K vs Sparse LDLT | ✓ PASS |
| ThermalAPIFailureHandlingOnNonConvergentSolver | API returns success=false and withholds thermal metrics on failure | ✓ PASS |
| DirectSolverNamingAndImplementationConsistency | Name strictly "Sparse LDLT", exact residual < 1e-10 | ✓ PASS |
| NonFiniteSolutionRejection | Correctly catches and rejects NaN / Inf entries | ✓ PASS |
| IncorrectSolutionDimensionRejection | Catches and rejects solution size != dof_count | ✓ PASS |
| SolverResidualVerificationAgainstOriginalSystem | Independent verification against original K and F | ✓ PASS |
| ThermalAPIFailureSemanticsIntegration | Returns HTTP 422, outer & inner success: false, no fabricated stats | ✓ PASS |
| ThermalAPISuccessSemanticsIntegration | Returns HTTP 200, outer & inner success: true, genuine physics | ✓ PASS |
| RealHttpThermalSolveSuccessAndFailure | End-to-end TCP loopback HTTP route test via httplib::Server & Client | ✓ PASS |
| ThermalAPIValidationRejectsResidualExceedingTolerance | Rejects solution exceeding residual tolerance threshold | ✓ PASS |

### 2.6 Benchmark Tests (`test_solver_benchmark.cpp`) — 4 tests

| Test | Result |
|------|--------|
| FullBenchmarkRun | ✓ PASS |
| MeshConvergenceStudy (multi-metric & L2 spatial probe field) | ✓ PASS |
| AllBenchmarkTrialsValidationAndResidualAudit | ✓ PASS |
| SSORResidualStrictValidationAudit | ✓ PASS |

**Total: 40/40 tests PASSED across 7 test suites (Phase 9)**
**Legacy total: 45/45 tests PASSED (uew_tests.exe)**
**Grand total: 85/85 tests PASSED (100% success rate)**

---

## 3. Symmetric Positive Definiteness Verification

The assembled global stiffness matrix K must be SPD for the problem to be well-posed and for direct factorisation to succeed.

**Verification method:**
- Attempt `Eigen::SimplicialLDLT` factorisation ($A = P^T L D L^T P$) on a test mesh.
- Check `.info() == Eigen::Success`.
- Verify diagonal of D factor is strictly positive.

**Result:** K is SPD for all three mesh resolutions when at least one Dirichlet BC is applied. ✓

---

## 4. Energy Balance Verification

**Setup:**
- Heat sink with 15.0 W heat generation applied to the top surface of the base plate ($z = \text{base\_thickness}$).
- Ambient temperature $T_\infty = 25.0^\circ\text{C}$.
- Convection coefficient $h = 15.0\text{ W/m}^2\text{K}$ on all fin surfaces and remaining base exterior faces.

**Measured Verification:**

$$\varepsilon = \frac{|Q_{\text{in}} - Q_{\text{conv}}|}{Q_{\text{in}}} < 5\%$$

| Mesh Level | DOFs / Nodes | Measured $Q_{\text{in}}$ [W] | Measured $Q_{\text{conv}}$ [W] | Measured Error [%] | Pass? |
|------------|--------------|------------------------------|--------------------------------|--------------------|-------|
| Coarse     | 836          | 15.000                       | 14.821                         | 1.19%              | ✓     |
| Medium     | 2,280        | 15.000                       | 14.896                         | 0.69%              | ✓     |
| Fine       | 5,238        | 15.000                       | 14.938                         | 0.41%              | ✓     |

---

## 5. Solver Agreement Study

Numerical solvers evaluated on the identical assembled heat-sink FEM system:

### 5.1 Simple Synthetic System vs Heat-Sink FEM
- **Synthetic 4×4 Diagonally Dominant System:** SSOR converges in 18 iterations with residual $< 1.0 \times 10^{-6}$ and agreement with exact analytical solution $< 1.0 \times 10^{-5}$.
- **Heat-Sink 3D FEM System (Strict Baseline tol = 1e-6, max_iter = 5,000):**
  - **Sparse LDLT:** Direct reference solve in 1 step ($1.188\ \text{ms}$ on Coarse), residual $\|KT-F\|/\|F\| = 3.72 \times 10^{-13}$.
  - **Conjugate Gradient (Jacobi PCG):** Converged in 167 iterations ($1.244\ \text{ms}$), residual $= 9.01 \times 10^{-7}$, $\max |\Delta T| = 0.00018^\circ\text{C}$ vs Sparse LDLT.
  - **SSOR (Iterative):** Did not converge to $1.0 \times 10^{-6}$ within 5,000 iterations (achieved residual $r_{\text{rel}} = 5.36 \times 10^{-3}$, status: non-convergent, HTTP 422). Across all 3 mesh levels under the configured $10^{-6}$ tolerance, SSOR is accurately reported as **failed / non-convergent** due to the spectral radius $\rho \approx 0.999$ of stiff 3D FEM conduction matrices.
  - *Exploratory Historical Note:* Under a deliberately loosened tolerance of $1.0 \times 10^{-4}$ on small meshes, SSOR can reach residual $\sim 8.94 \times 10^{-5}$; however, in the strict $1.0 \times 10^{-6}$ production configuration, SSOR does not meet tolerance and is never reported as successful.

---

## 6. Mesh Convergence Behaviour

Convergence analysis across discrete mesh refinement levels:

| Mesh Level | Nodes / DOFs | Tetrahedra | $T_{\max}$ [°C] | $T_{\text{avg}}$ [°C] | $R_{\text{th}}$ [K/W] | $\Delta T_{\max}$ [%] | Base IDW Discrepancy [%] | Sparse LDLT Converged? | CG Converged? | SSOR Converged (tol 1e-6)? |
|------------|--------------|------------|-----------------|-----------------------|-----------------------|-----------------------|--------------------------|------------------------|---------------|----------------------------|
| Coarse     | 836          | 2,200      | 38.42           | 31.85                 | 0.895                 | Baseline              | Baseline                 | ✓ Yes (res 3.72e-13)   | ✓ Yes (167 iters) | ✗ No (res 5.36e-3)         |
| Medium     | 2,280        | 6,570      | 39.81           | 32.41                 | 0.987                 | +3.62%                | 1.88%                    | ✓ Yes (res 5.18e-13)   | ✓ Yes (183 iters) | ✗ No (res 1.24e-2)         |
| Fine       | 5,238        | 16,300     | 40.54           | 32.84                 | 1.036                 | +1.83%                | 1.12%                    | ✓ Yes (res 1.15e-12)   | ✓ Yes (223 iters) | ✗ No (res 3.64e-2)         |

**Methodological Notes & Limitations:**
- **Sampled Field Discrepancy:** The field metric evaluates the relative discrete $L_2$ difference across 600 fixed probe points inside the base plate using four-nearest-node Inverse Distance Weighting (IDW). It is an approximate base-domain sampled comparison, not a full-domain 3D FEM element interpolation.
- **Extrapolation Limitation:** Three discrete mesh levels are insufficient for rigorous Grid Convergence Index (GCI) or Richardson extrapolation. The fine-grid peak temperature ($40.54^\circ\text{C}$) is reported strictly as the highest-resolution measured value, not a mathematically proven asymptotic limit.

---

## 7. Known Limitations

| Limitation | Impact | Status |
|------------|--------|--------|
| SSOR GS on 3D FEM (ρ ≈ 0.999) | Requires O(10 000) iterations to reach tol=1e-6; honestly reports non-convergence | Expected — documented |
| No AMG preconditioner | CG may slow on very fine meshes; Jacobi preconditioner adequate for medium mesh | Acceptable for educational benchmarks |
| No parallel assembly | Assembly time grows with mesh size; adequate for benchmarking up to Fine mesh | Known limitation |
| TET4 locking not applicable | Thermal problem, not mechanical | No issue |
| Mounting holes = parametric zones | Volume/mesh statistics consistent; STL export documents this approximation | Documented |
