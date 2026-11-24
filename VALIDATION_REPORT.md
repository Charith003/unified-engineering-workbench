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

### 2.5 Solver & API Tests (`test_numerical_solvers.cpp`) — 13 tests

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

### 2.6 Benchmark Tests (`test_solver_benchmark.cpp`) — 3 tests

| Test | Result |
|------|--------|
| FullBenchmarkRun | ✓ PASS |
| MeshConvergenceStudy (multi-metric & L2 spatial probe field) | ✓ PASS |
| AllBenchmarkTrialsValidationAndResidualAudit | ✓ PASS |

**Total: 37/37 tests PASSED across 6 test suites (Phase 9)**
**Legacy total: 45/45 tests PASSED (uew_tests.exe)**
**Grand total: 82/82 tests PASSED (100% success rate)**

---

## 3. Symmetric Positive Definiteness Verification

The assembled global stiffness matrix K must be SPD for the problem to be well-posed and for Cholesky factorisation to succeed.

**Verification method:**
- Attempt `Eigen::SimplicialLDLT` factorisation on a test mesh.
- Check `.info() == Eigen::Success`.
- Optionally verify diagonal of D factor is all-positive.

**Result:** K is SPD for all three mesh resolutions when at least one Dirichlet BC is applied. ✓

---

## 4. Energy Balance Verification

**Setup:**
- Heat sink with 15 W heat generation on the central source region.
- Ambient temperature T∞ = 25°C.
- Convection coefficient h = 15 W/m²K on all fin surfaces and base underside.

**Check:**

$$\varepsilon = \frac{|Q_{\text{in}} - Q_{\text{conv}}|}{Q_{\text{in}}} < 5\%$$

| Mesh Level | Q_in [W] | Q_conv [W] | Error [%] | Pass? |
|------------|----------|------------|-----------|-------|
| Coarse     | 15.0     | ~14.3–14.9 | < 5%      | ✓     |
| Medium     | 15.0     | ~14.6–15.0 | < 5%      | ✓     |
| Fine       | 15.0     | ~14.8–15.0 | < 5%      | ✓     |

(Exact values depend on mesh resolution and boundary condition assignment.)

---

## 5. Solver Agreement Study

Comparison of CG and GS solutions vs Cholesky gold standard on the medium mesh:

| DOF | Cholesky T_max [°C] | CG T_max [°C] | CG max|ΔT| [K] | GS T_max [°C] | GS max|ΔT| [K] |
|-----|---------------------|---------------|----------------|---------------|----------------|
| ~5k | Reference           | ≈ Reference    | < 0.001        | ≈ Reference   | < 1.0          |

---

## 6. Mesh Convergence Behaviour

As mesh is refined from coarse to fine, maximum temperature converges monotonically. This is expected for a well-posed linear elliptic problem with consistent FEM discretisation.

The convergence rate for TET4 (linear elements) in the energy norm is:

$$\|e\|_{H^1} = O(h^1)$$

For smooth solutions, the pointwise error converges as:

$$\|e\|_{L^\infty} \approx O(h^2)$$

---

## 7. Known Limitations

| Limitation | Impact | Status |
|------------|--------|--------|
| SSOR GS on 3D FEM (ρ ≈ 0.999) | Requires O(10 000) iterations to reach tol=1e-6; honestly reports non-convergence | Expected — documented |
| No AMG preconditioner | CG may slow on very fine meshes; Jacobi preconditioner adequate for medium mesh | Acceptable for educational benchmarks |
| No parallel assembly | Assembly time grows with mesh size; adequate for benchmarking up to Fine mesh | Known limitation |
| TET4 locking not applicable | Thermal problem, not mechanical | No issue |
| Mounting holes = parametric zones | Volume/mesh statistics consistent; STL export documents this approximation | Documented |
