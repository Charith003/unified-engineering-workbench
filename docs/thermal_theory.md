# 1D Steady-State Thermal Finite Element Analysis — Mathematical Formulation & Verification

**Module:** Thermal FEA Solver  
**Equation:** 1D Poisson Heat Conduction with Uniform Source  
**Author:** Candidate for PASS Science Works R&D Internship 2026–2027

---

## 1. Strong Form Governing Equation

Consider steady-state one-dimensional conduction along a conducting rod of length $L$ with cross-sectional area $A$, constant thermal conductivity $k$, and constant volumetric heat generation rate $q$:

$$-\frac{d}{dx}\left(k A \frac{dT}{dx}\right) = q A, \quad x \in (0, L)$$

Subject to Dirichlet boundary conditions:
$$T(0) = T_L = 100^\circ\text{C}$$
$$T(L) = T_R = 20^\circ\text{C}$$

Physical parameter values:
- Length: $L = 1.0\text{ m}$
- Area: $A = 1.0 \times 10^{-4}\text{ m}^2$
- Conductivity: $k = 10.0\text{ W}/(\text{m}\cdot\text{K})$
- Volumetric source: $q = 10\,000.0\text{ W}/\text{m}^3$

---

## 2. Analytical Exact Solution

Integrating the ODE directly:
$$\frac{dT}{dx} = -\frac{q}{k} x + C_1$$
$$T_{\text{exact}}(x) = -\frac{q}{2k} x^2 + C_1 x + C_2$$

Applying boundary conditions:
- At $x = 0$: $C_2 = T_L = 100^\circ\text{C}$
- At $x = L$: $T(L) = -\frac{q}{2k} L^2 + C_1 L + 100 = 20 \implies C_1 = -\frac{80}{L} + \frac{q L}{2k}$

Substituting $C_1$:
$$T_{\text{exact}}(x) = 100 - \frac{80 x}{L} + \frac{q}{2k} x (L - x)$$

With specified parameters:
$$T_{\text{exact}}(x) = 100 - 80x + 500x(1 - x)$$

---

## 3. Finite Element Weak Form Formulation

Multiplying by a test function $v(x) \in H_0^1(0, L)$ and integrating by parts over element $e = [x_1, x_2]$ of length $h = x_2 - x_1$:
$$\int_{x_1}^{x_2} k A \frac{dv}{dx} \frac{dT}{dx} dx = \int_{x_1}^{x_2} q A v dx + \left[ v k A \frac{dT}{dx} \right]_{x_1}^{x_2}$$

Using linear Lagrange shape functions:
$$N_1(\xi) = 1 - \frac{\xi}{h}, \quad N_2(\xi) = \frac{\xi}{h}, \quad \xi = x - x_1$$

### Element Stiffness Matrix ($K^e$):
$$K_{ij}^e = \int_0^h k A \frac{dN_i}{d\xi} \frac{dN_j}{d\xi} d\xi = \frac{k A}{h} \begin{bmatrix} 1 & -1 \\ -1 & 1 \end{bmatrix}$$

### Element Load Vector ($F^e$):
$$F_i^e = \int_0^h q A N_i(\xi) d\xi = \frac{q A h}{2} \begin{bmatrix} 1 \\ 1 \end{bmatrix}$$

---

## 4. Assembly & Linear System Solution

Global assembly for $N$ uniform elements produces a symmetric tridiagonal matrix of dimension $(N+1) \times (N+1)$.

Dirichlet conditions are enforced by replacing row 0 with $T_0 = 100$ and row $N$ with $T_N = 20$, preserving symmetry and positive-definiteness on internal degrees of freedom.

The resulting tridiagonal system is solved via the Thomas algorithm ($O(N)$ computational complexity and optimal numerical stability).

---

## 5. Midpoint Discretization Error Analysis & Convergence

### Critical Mathematical & Debugging Insight:
Because the one-dimensional Green's function for $-\frac{d^2}{dx^2}$ is piecewise linear, linear finite elements yield **exact nodal values** at the mesh vertices:
$$T_{\text{fem}}(x_i) = T_{\text{exact}}(x_i), \quad \forall i = 0, \dots, N$$

Evaluating error solely at nodes yields misleading $\approx 0$ machine precision numbers ($< 10^{-14}$) regardless of mesh density, concealing the actual discretization error.

**Midpoint Evaluation:**
Within element $e$, the FE solution interpolates linearly:
$$T_{\text{fem}}(x_{\text{mid}}) = \frac{1}{2}\left(T_{\text{fem}}(x_i) + T_{\text{fem}}(x_{i+1})\right)$$

By Taylor series expansion of $T_{\text{exact}}$ around $x_{\text{mid}}$:
$$T_{\text{exact}}(x_{\text{mid}} \pm h/2) = T(x_{\text{mid}}) \pm \frac{h}{2} T'(x_{\text{mid}}) + \frac{h^2}{8} T''(x_{\text{mid}}) + O(h^3)$$
$$\frac{1}{2}\left(T_{\text{exact}}(x_i) + T_{\text{exact}}(x_{i+1})\right) = T_{\text{exact}}(x_{\text{mid}}) + \frac{h^2}{8} T''(x_{\text{mid}})$$

Since $T''(x) = -\frac{q}{k} = -1000$:
$$\text{Error}_{\text{mid}} = |T_{\text{fem}}(x_{\text{mid}}) - T_{\text{exact}}(x_{\text{mid}})| = \frac{h^2}{8} \left|\frac{q}{k}\right| = \frac{125}{N^2}$$

### Theoretical vs Measured Midpoint Errors:
| Mesh Elements ($N$) | Element Size ($h$) | Theoretical Error ($\frac{125}{N^2}$) | Measured Maximum Error | Error Ratio ($\frac{E_N}{E_{2N}}$) |
|---|---|---|---|---|
| $N = 10$ | $0.1000\text{ m}$ | $1.25000000^\circ\text{C}$ | $1.25000000^\circ\text{C}$ | — |
| $N = 20$ | $0.0500\text{ m}$ | $0.31250000^\circ\text{C}$ | $0.31250000^\circ\text{C}$ | $4.0000$ ($O(h^2)$) |
| $N = 40$ | $0.0250\text{ m}$ | $0.07812500^\circ\text{C}$ | $0.07812500^\circ\text{C}$ | $4.0000$ ($O(h^2)$) |
| $N = 80$ | $0.0125\text{ m}$ | $0.01953125^\circ\text{C}$ | $0.01953125^\circ\text{C}$ | $4.0000$ ($O(h^2)$) |

The measured convergence rate is exactly quadratic ($O(h^2)$), confirming theoretical FE convergence and solver correctness.
