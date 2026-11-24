# Numerical Methods Reference — UEW 3D Thermal FEM Engine

## PASS Science Works R&D Internship 2026–2027

---

## 1. Governing Equation

Steady-state thermal conduction in a solid domain Ω:

$$-\nabla \cdot (k \nabla T) = Q \quad \text{in } \Omega$$

| Symbol | Description          | Units  |
|--------|----------------------|--------|
| T      | Temperature field    | K or °C|
| k      | Thermal conductivity | W/m·K  |
| Q      | Volumetric heat gen. | W/m³   |

**Boundary conditions:**

| Type       | Equation                              | Applied to          |
|------------|---------------------------------------|---------------------|
| Dirichlet  | T = T_ambient on Γ_D                  | Fixed-temp faces    |
| Neumann    | −k ∂T/∂n = q'' on Γ_N               | Heat-flux faces     |
| Robin      | −k ∂T/∂n = h(T − T_∞) on Γ_R       | Convective faces    |

---

## 2. Finite Element Discretisation

### 2.1 Linear Tetrahedral Element (TET4)

A tetrahedron has 4 nodes. The shape functions are:

$$N_1 = 1 - \xi - \eta - \zeta, \quad N_2 = \xi, \quad N_3 = \eta, \quad N_4 = \zeta$$

where (ξ, η, ζ) are volume coordinates. These are **linear** functions → constant gradient within each element.

### 2.2 Element Stiffness Matrix

The weak-form Galerkin residual for one element e with volume V_e:

$$K^e_{ij} = \int_{\Omega_e} k \, \nabla N_i \cdot \nabla N_j \, d\Omega = k \cdot V_e \cdot \nabla N_i \cdot \nabla N_j$$

The gradient of shape function i is computed from the inverse Jacobian:

$$\nabla N = J^{-T} \begin{pmatrix} \partial N / \partial \xi \end{pmatrix}$$

where **J** is the 3×3 Jacobian of the isoparametric mapping from physical to reference coordinates.

For TET4, the natural-coordinate gradients are constant:

$$\frac{\partial N_1}{\partial \xi} = -1, \quad \frac{\partial N_2}{\partial \xi} = 1, \quad \ldots$$

so the full **B**-matrix (3×4) is:

$$\mathbf{B} = J^{-T} \begin{bmatrix} -1 & 1 & 0 & 0 \\ -1 & 0 & 1 & 0 \\ -1 & 0 & 0 & 1 \end{bmatrix}$$

and the element stiffness is:

$$K^e = k \cdot V_e \cdot \mathbf{B}^T \mathbf{B} \in \mathbb{R}^{4 \times 4}$$

### 2.3 Volumetric Heat Load Vector

$$f^e_i = Q \cdot V_e / 4 \quad \forall i \in \{1,2,3,4\}$$

(uniform load → equal distribution to all 4 nodes.)

### 2.4 Robin (Convection) Surface Integral

For a triangular face with area A and convection coefficient h:

$$K^{\text{conv}}_{ij} = \frac{h \cdot A}{12} \begin{bmatrix}2&1&1\\1&2&1\\1&1&2\end{bmatrix}$$

$$f^{\text{conv}}_i = \frac{h \cdot T_\infty \cdot A}{3}$$

### 2.5 Assembly

Global K and F are assembled by scatter-adding element contributions:

```
for each element e:
    compute K^e, f^e
    for i, j in {0..3}:
        K[global_i][global_j] += K^e[i][j]
        F[global_i]           += f^e[i]
```

Stored as `Eigen::SparseMatrix<double>` via triplet insertion.

### 2.6 Dirichlet Enforcement

Symmetric enforcement (preserves SPD property):

For a constrained DOF k with prescribed value T_k:
1. Zero row k and column k (both row and column entries, in a ColMajor sparse matrix this requires a full outer-loop pass).
2. Set K[k][k] = 1.
3. Set F[k] = T_k.
4. Subtract column contributions from F for unconstrained rows:
   F[i] -= K_saved[i][k] × T_k.

**Correctness note:** In Eigen `ColMajor` sparse storage, zeroing only columns leaves row off-diagonals intact. The implementation symmetrically zeros both the row entries (scanning all column outer vectors) and column entries in a single pass. This was verified by a manufactured linear-solution patch test with error < 1×10⁻¹⁰.

---

## 2.7 Heat Source Boundary Location

The heat source (Neumann flux) patch is applied to the **top surface of the aluminum base plate**, at z = base_thickness (z = 8 mm), with outward normal +Z. The heat flux applied is:

$$q'' = P / A_{\text{src}} \quad [\text{W/m}^2]$$

where A_src = heat_source_length × heat_source_width = 0.030 × 0.025 m².

**Boundary face classification rule:**
- Heat source: `|fc_z − base_thickness| ≤ 1×10⁻⁵` AND `normal[z] > 0.5` AND face centroid within heat source x–y patch.
- Convection: all remaining exposed surfaces (fin surfaces, base bottom, fin tips).
- Not the bottom face (z = 0).

This corrects the previous erroneous assignment to the bottom face (z ≤ 1×10⁻⁶).

---

## 3. Numerical Solvers

### 3.1 Sparse LDLT Direct Solver (SimplicialLDLT)

The assembled stiffness matrix $K$ is symmetric positive definite (SPD) for well-posed thermal problems. Rather than standard Cholesky $L L^T$ (which requires taking square roots of diagonal pivots), the reference direct solver employs Eigen's `SimplicialLDLT`, performing the exact root-free factorization:

$$K = P^T L D L^T P$$

where $P$ is a fill-reducing fill-in permutation matrix, $L$ is a unit lower triangular matrix, and $D$ is a diagonal matrix. The solve sequence is:

$$L \mathbf{y} = P \mathbf{F}, \quad D \mathbf{z} = \mathbf{y}, \quad L^T (P \mathbf{T}) = \mathbf{z}$$

via forward and backward triangular substitution in $O(\text{nnz} \cdot k)$ operations (where $k$ is the average fill-in factor).

**Why Sparse LDLT over LLT:**
- Root-free: Eliminates square-root evaluations on diagonal entries, avoiding breakdown if numerical rounding produces tiny pivots.
- Exactness: Evaluates machine-precision direct solution with relative residual $\|r\|_2 / \|F\|_2 < 1 \times 10^{-10}$.
- Gold-standard reference: Used as the benchmark baseline against which all iterative solvers (CG, SSOR) and mesh levels are evaluated.

### 3.2 Conjugate Gradient (Jacobi Preconditioned)

Iterative Krylov method. With preconditioner M = diag(K):

$$M^{-1} K \mathbf{T} = M^{-1} \mathbf{F}$$

Algorithm (each iteration):
1. Compute residual r = F − Kx
2. Apply preconditioner z = M⁻¹r
3. Update search direction p
4. Line search: α = rᵀz / pᵀKp
5. Update: x += αp, r -= αKp

**Convergence:** ‖r‖₂ / ‖F‖₂ < 1×10⁻⁸ (or max 10 000 iterations).

**Use:** Best for large sparse systems; scales well with mesh refinement.

### 3.3 Symmetric Gauss-Seidel (SSOR)

One SSOR sweep = one forward sweep + one backward sweep:

**Forward sweep** (i = 0 → n−1):
$$x_i^{(k+1)} = \frac{1}{K_{ii}} \left( F_i - \sum_{j < i} K_{ij} x_j^{(k+1)} - \sum_{j > i} K_{ij} x_j^{(k)} \right)$$

**Backward sweep** (i = n−1 → 0):
$$x_i^{(k+1)} = \frac{1}{K_{ii}} \left( F_i - \sum_{j < i} K_{ij} x_j^{(k)} - \sum_{j > i} K_{ij} x_j^{(k+1)} \right)$$

**Convergence criterion (strict):** ‖r‖₂ / ‖F‖₂ < configured tolerance (default 1×10⁻⁶). The solver **only** reports `success = true` when this criterion is strictly met before reaching max iterations.

**Non-convergence policy:**
- Reaching `max_iterations` returns `success = false` with message "Max iterations reached".
- Divergence detection: if `‖r‖ > 1×10⁶ × ‖r₀‖` or any `x_i` becomes non-finite, the solver halts immediately and reports `success = false`.
- The old permissive condition `success = (converged || residual_norm < 0.05)` has been removed.

**Note on 3D FEM matrices:** For large 3D elliptic systems (n ≈ 1 000–20 000 DOFs with h = 25 W/(m²K)), the spectral radius ρ ≈ 0.999, requiring O(10 000) iterations to reach ‖r‖/‖b‖ < 10⁻⁶. SSOR typically reaches ‖r‖/‖b‖ ≈ 0.03 at 5 000 iterations and must honestly report non-convergence.

**Use:** Demonstrates classical stationary iterative solvers and their limitations on stiff 3D FEM matrices. Compare with CG, which converges in far fewer iterations.

### 3.4 API Solution Validation Contract & Failure Semantics

To guarantee that invalid or non-convergent numerical solutions are never presented as valid thermal physics:
1. **Convergence Status Check:** The solver must report `sres.success == true`.
2. **DOF Dimension Check:** The resulting solution vector size must strictly equal `sys.dof_count`.
3. **Finite Value Check:** All vector elements must be finite (`sres.solution.allFinite()`).
4. **Structured Failure Response (HTTP 422):** If any check fails, the API responds with:
   - HTTP status: `422 Unprocessable Entity` (indicating a well-formed request whose numerical solve failed convergence)
   - Outer status: `success: false`
   - Inner payload: `data.success: false`
   - Diagnostic status message explaining the cause (e.g. "Maximum iterations reached (5000/5000)")
   - **Diagnostic telemetry preserved:** `assembly_time_ms`, `solve_time_ms`, `iterations`, `residual_norm`, `memory_kb`, `solver_name`
   - **Withheld downstream metrics:** Minimum, maximum, average temperatures, total heat output, and global energy balance error are strictly omitted from the payload
   - **Empty visualization arrays:** `nodal_temperatures`, `nodes`, `boundary_faces`, and `elements` are returned as empty arrays `[]` to prevent rendering false physics.

---

## 4. Heat Sink Geometry

### 4.1 Aluminum Material Properties

| Property          | Value       | Units  |
|-------------------|-------------|--------|
| Thermal conductivity (k) | 205 | W/m·K |
| Density           | 2700        | kg/m³  |
| Specific heat     | 900         | J/kg·K |

### 4.2 Default Geometry

| Parameter       | Value    |
|-----------------|----------|
| Base length     | 120 mm   |
| Base width      | 80 mm    |
| Base thickness  | 8 mm     |
| Fin count       | 7        |
| Fin heights     | 20–45 mm (non-uniform) |
| Fin thickness   | 3 mm     |
| Fin spacing     | Variable |
| Heat source     | 30×25 mm |
| Heat generation | 15 W     |
| Heat density Q  | 15 / (0.030 × 0.025 × 0.008) W/m³ |

### 4.3 Mounting Hole Representation in Structured 5-Tet Mesh

The four corner mounting holes (3.2 mm diameter for standard M3 hardware) are approximated in the structured hexahedral-to-tetrahedral decomposition as boundary condition zones (Dirichlet fixed mounting temperature or non-convecting adiabatic zones) at their physical $(x, y)$ coordinates:
- Hole 1: $(8\text{ mm}, 8\text{ mm})$
- Hole 2: $(112\text{ mm}, 8\text{ mm})$
- Hole 3: $(8\text{ mm}, 72\text{ mm})$
- Hole 4: $(112\text{ mm}, 72\text{ mm})$

**Explicit CAD Approximation Notice:**
They are **not** modeled as exact conforming circular CAD cutouts in the structured 5-tet mesh or ASCII STL export. Conforming cylindrical cutouts would require non-uniform unstructured Delaunay/Voronoi tetrahedralization (e.g. TetGen/Netgen), which introduces irregular sliver elements and compromises structured mesh grading. The parametric boundary zone approximation ensures consistent topology, volume metrics, and zero inverted elements across all mesh levels.

---

## 5. Mesh Generation

### 5.1 Hexahedral-to-Tetrahedral Decomposition

The heat sink domain is discretised on a structured grid. Each hexahedral cell (8 nodes) is split into 5 tetrahedra using a consistent diagonal orientation to ensure mesh conformity (no hanging nodes).

The 5-tet decomposition pattern:
```
Tet 0: [0,1,3,4]
Tet 1: [1,2,3,6]
Tet 2: [3,4,6,7]
Tet 3: [1,4,5,6]
Tet 4: [1,3,4,6]  ← shared interior diagonal
```

### 5.2 Resolution Presets

| Preset | Grid divisions (approx.) | Nodes  | Elements  |
|--------|--------------------------|--------|-----------|
| Coarse | 8×6×3                    | ~1,000 | ~4,000    |
| Medium | 12×9×5                   | ~5,000 | ~20,000   |
| Fine   | 18×13×8                  | ~20,000| ~80,000   |

### 5.3 Internal Tetrahedral Representation & Sectioning

To inspect volumetric mesh topology without artificial interior decors:
- Each linear tetrahedron comprises 6 unique internal edges: $(n_0, n_1), (n_0, n_2), (n_0, n_3), (n_1, n_2), (n_1, n_3), (n_2, n_3)$.
- Edge pairs are uniquely deduplicated across adjacent tetrahedral elements to generate an authentic internal wireframe.
- A dynamic local clipping plane normal to the Z-axis ($z \le z_{\text{cut}}$) permits cross-sectional examination of core temperature gradients and interior volumetric tetrahedra.

### 5.4 Spatial Domain Probe Grid for Mesh Field Convergence

Comparing raw nodal solution vectors between meshes of different resolutions (e.g. 590 DOFs vs 1,944 DOFs) is mathematically invalid due to incompatible discrete function spaces.

To evaluate field-level convergence rigorously without vector truncation or padding, UEW deploys an independent 600-point 3D Cartesian spatial probe grid distributed through the solid domain volume:

$$\mathbf{p}_k = (x_i, y_j, z_l), \quad x_i \in (0, L), \, y_j \in (0, W), \, z_l \in (0, t_{\text{base}})$$

At each probe point, the temperature field $T^{(m)}(\mathbf{p}_k)$ is evaluated using local 4-node Inverse Distance Weighting (IDW) from the surrounding tetrahedral nodes. The discrete relative $L_2$ field difference between successive mesh levels is:

$$\varepsilon_{\text{field}} = \frac{\sqrt{\sum_{k=1}^{600} \left( T^{(i)}(\mathbf{p}_k) - T^{(i-1)}(\mathbf{p}_k) \right)^2}}{\sqrt{\sum_{k=1}^{600} \left( T^{(i)}(\mathbf{p}_k) \right)^2}} \times 100\%$$

This ensures that field-level convergence is physically and mathematically sound.

---

## 6. Energy Balance Verification

After solving, the energy balance is checked:

$$Q_{\text{in}} = Q_{\text{conv}}$$

where:
- $Q_{\text{in}}$ = prescribed heat generation (W)
- $Q_{\text{conv}} = \sum_{\text{conv faces}} h \cdot A_f \cdot (T_f - T_\infty)$

Acceptance criterion: |Q_in − Q_conv| / Q_in < 5%.

---

## 7. References

1. O. C. Zienkiewicz, R. L. Taylor, J. Z. Zhu, *The Finite Element Method: Its Basis and Fundamentals*, 7th ed., Butterworth-Heinemann, 2013.
2. J. N. Reddy, *An Introduction to the Finite Element Method*, 4th ed., McGraw-Hill, 2019.
3. Y. Saad, *Iterative Methods for Sparse Linear Systems*, 2nd ed., SIAM, 2003.
4. Eigen library documentation: https://eigen.tuxfamily.org/dox/
