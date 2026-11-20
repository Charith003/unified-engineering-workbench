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
1. Zero row k and column k.
2. Set K[k][k] = 1.
3. Set F[k] = T_k.
4. Subtract column contributions from F for unconstrained rows:
   F[i] -= K_saved[i][k] × T_k.

---

## 3. Numerical Solvers

### 3.1 Sparse Cholesky (SimplicialLDLT)

The assembled K is symmetric positive definite (SPD) for well-posed thermal problems. Eigen's `SimplicialLDLT` performs:

$$K = L D L^T$$

where L is lower triangular and D is diagonal. The solve is:

$$K \mathbf{T} = \mathbf{F} \Rightarrow L D L^T \mathbf{T} = \mathbf{F}$$

via forward/back substitution in $O(nnz \cdot k)$ where k is the fill-in factor.

**Use:** Reference solver. Deterministic, direct, residual < 1×10⁻¹⁰.

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

Iterates until ‖r‖₂ < 0.05 or 4000 iterations. SSOR improves convergence significantly over forward-only GS on FEM matrices.

**Use:** Demonstrates classical iterative solvers; slower convergence than CG for ill-conditioned FEM matrices.

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
