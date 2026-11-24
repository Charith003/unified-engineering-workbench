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

### 2.6 Benchmark Tests (`test_solver_benchmark.cpp`) — 3 tests

| Test | Result |
|------|--------|
| FullBenchmarkRun | ✓ PASS |
| MeshConvergenceStudy (multi-metric & L2 spatial probe field) | ✓ PASS |
| AllBenchmarkTrialsValidationAndResidualAudit | ✓ PASS |

**Total: 39/39 tests PASSED across 7 test suites (Phase 9)**
**Legacy total: 45/45 tests PASSED (uew_tests.exe)**
**Grand total: 84/84 tests PASSED (100% success rate)**

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
| Coarse     | 590          | 15.000                       | 14.821                         | 1.19%              | ✓     |
| Medium     | 1,944        | 15.000                       | 14.896                         | 0.69%              | ✓     |
| Fine       | 5,880        | 15.000                       | 14.938                         | 0.41%              | ✓     |

---

## 5. Solver Agreement Study

Numerical solvers evaluated on the identical assembled heat-sink FEM system:

### 5.1 Simple Synthetic System vs Heat-Sink FEM
- **Synthetic 4×4 Diagonally Dominant System:** SSOR converges in 18 iterations with residual $< 1.0 \times 10^{-6}$ and agreement with exact analytical solution $< 1.0 \times 10^{-5}$.
- **Heat-Sink 3D FEM System (Coarse Mesh: 590 DOFs):**
  - **Sparse LDLT:** Direct reference solve in 1 step ($249.0\ \mu\text{s}$), residual $\|KT-F\|/\|F\| = 1.76 \times 10^{-15}$.
  - **Conjugate Gradient (Jacobi PCG):** Converged in 38 iterations ($186.0\ \mu\text{s}$), residual $= 8.21 \times 10^{-7}$, $\max |\Delta T| = 0.00018^\circ\text{C}$ vs Sparse LDLT.
  - **SSOR (Iterative):** Converged in 211 iterations ($1,946.5\ \mu\text{s}$) at tolerance $10^{-4}$ ($\max |\Delta T| = 0.027^\circ\text{C}$). However, on Medium/Fine meshes with strict tolerance ($10^{-6}$), SSOR spectral radius $\rho \approx 0.999$ requires $> 5,000$ iterations and honestly reports non-convergence (HTTP 422).

---

## 6. Mesh Convergence Behaviour

Convergence analysis across discrete mesh refinement levels:

| Mesh Level | Nodes / DOFs | Tetrahedra | $T_{\max}$ [°C] | $T_{\text{avg}}$ [°C] | $R_{\text{th}}$ [K/W] | $\Delta T_{\max}$ [%] | Base IDW Discrepancy [%] |
|------------|--------------|------------|-----------------|-----------------------|-----------------------|-----------------------|--------------------------|
| Coarse     | 590          | 1,480      | 38.42           | 31.85                 | 0.895                 | Baseline              | Baseline                 |
| Medium     | 1,944        | 5,920      | 39.81           | 32.41                 | 0.987                 | +3.62%                | 1.88%                    |
| Fine       | 5,880        | 20,720     | 40.54           | 32.84                 | 1.036                 | +1.83%                | 1.12%                    |

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
