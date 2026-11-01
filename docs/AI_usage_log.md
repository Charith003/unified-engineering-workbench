# AI Usage Log & Independent Engineering Verification Audit

**Project:** Unified Engineering Workbench (UEW)  
**Assignment:** PASS Science Works — R&D Internship 2026–2027  
**Candidate ID:** Candidate Submission 2026

---

## 1. AI Tooling & Models Utilized

- **Primary AI Agent:** Antigravity (Google DeepMind Advanced Agentic Coding)
- **Language Models Evaluated / Used:**
  - Claude Sonnet 4.6 (Anthropic) via Antigravity IDE
  - Gemini 3.8 Flash (Google DeepMind) via Antigravity IDE
- **Scope of Assistance:**
  - Boilerplate scaffolding for Three.js OrbitControls canvas
  - Initial TypeScript interface mirroring C++ JSON structures
  - Initial CMakeLists.txt structure

---

## 2. Representative Prompts & Generated Outputs

### Interaction 1: 1D Thermal FEA Element Formulation
- **Prompt:**  
  *"Implement the local element stiffness matrix $K^e$ and element load vector $F^e$ for steady-state 1D heat conduction with uniform internal heat generation $q$ over two-node linear elements of length $h$."*
- **Model Output:**  
  $$K^e = \frac{kA}{h}\begin{bmatrix}1 & -1 \\ -1 & 1\end{bmatrix}, \quad F^e = \frac{qAh}{2}\begin{bmatrix}1 \\ 1\end{bmatrix}$$
- **Human Modification & Integration:**  
  Verified weak-form variational derivation. Enforced essential Dirichlet boundary conditions at $x=0$ ($100^\circ\text{C}$) and $x=L$ ($20^\circ\text{C}$) via algebraic row-replacement on the assembled tridiagonal global system.
- **Independent Validation:**  
  Derived analytical exact temperature $T_{\text{exact}}(x) = 100 - 80x + 500x(1-x)$. Verified that linear FE yields exact nodal temperatures and derived the midpoint interpolation error formula $\text{Error}_{\text{mid}} = \frac{125}{N^2}$.

### Interaction 2: Analytical Backpropagation Derivation
- **Prompt:**  
  *"For $h = \tanh(wx + b)$, $\hat{y} = vh + c$, and $J = \frac{1}{2}(\hat{y} - y)^2$, derive partial derivatives $\frac{\partial J}{\partial w}, \frac{\partial J}{\partial b}, \frac{\partial J}{\partial v}, \frac{\partial J}{\partial c}$ without automatic differentiation."*
- **Model Output:**  
  Correct scalar chain rule applications with $\frac{dh}{dz} = 1 - h^2$.
- **Human Modification & Integration:**  
  Implemented simultaneous parameter descent $\theta \leftarrow \theta - \eta \nabla_\theta J$ where all four updates evaluate from the unupdated initial state $\theta_0$. Added central finite-difference verification:
  $$\frac{\partial J}{\partial \theta} \approx \frac{J(\theta + \epsilon) - J(\theta - \epsilon)}{2\epsilon}, \quad \epsilon = 10^{-6}$$
- **Independent Validation:**  
  Automated tests in `tests/backprop/test_network.cpp` confirmed that analytical gradients match finite-difference evaluations within $|\Delta| \le 1.6 \times 10^{-10}$, strictly surpassing the required $\le 10^{-5}$ threshold.

---

## 3. Unsuitable Suggestions Identified & Corrected (Deliberate Challenge / Real Interaction)

### Unsuitable Suggestion 1: Node-Based Error Evaluation in 1D Linear FEA
- **Model Suggestion:**  
  The initial model draft proposed calculating the numerical FEA error as:
  $$\text{Error} = \max_i |T_{\text{fem}}(x_i) - T_{\text{exact}}(x_i)|$$
  evaluating the error at the mesh *nodes*.
- **Why It Was Unsuitable & Mathematically Defective:**  
  In 1D steady-state heat conduction with constant coefficients and constant source terms, the Green's function is piecewise linear. As a result, the linear finite element method yields the **exact solution at all mesh nodes** ($T_{\text{fem}}(x_i) \equiv T_{\text{exact}}(x_i)$ within floating-point roundoff).
  Evaluating error at nodes gives $\approx 10^{-15}$, falsely creating the impression of infinite accuracy on even a 2-element mesh, and completely hides the $O(h^2)$ inter-element discretization error.
- **Correction Applied:**  
  The evaluation was rewritten to interpolate the FE solution at element midpoints:
  $$T_{\text{fem}}(x_{\text{mid}}) = \frac{1}{2}\left(T_i + T_{i+1}\right)$$
  Comparing this to $T_{\text{exact}}(x_{\text{mid}})$ correctly exposed the true spatial discretization error $\frac{125}{N^2}$, demonstrating genuine $O(h^2)$ convergence ($1.25^\circ\text{C}$ for $N=10$, $0.3125^\circ\text{C}$ for $N=20$, $0.078125^\circ\text{C}$ for $N=40$, and $0.01953125^\circ\text{C}$ for $N=80$).

### Unsuitable Suggestion 2: Administrator-Requiring Visual Studio Build Tools via winget
- **Model Suggestion:**  
  The model attempted to invoke `winget install Microsoft.VisualStudio.2022.BuildTools` in the background to acquire an MSVC compiler.
- **Why It Was Unsuitable:**  
  `vs_BuildTools.exe` immediately hung waiting for interactive UAC (Administrator) elevation, violating the primary project constraint: *"Must NOT require Administrator permissions, internet, or first-run compilation on target machines"*.
- **Correction Applied:**  
  Replaced heavy administrative installers with a self-contained, portable, zero-admin MinGW-w64 toolchain (`w64devkit` with GCC 13.2.0, POSIX threads, static runtime). This enabled clean compilation of `uew_server.exe` and `Launch.exe` with zero installation friction and zero administrator privileges.

---

## 4. Engineering Ownership & Integrity Statement

All generated code was thoroughly reviewed, refactored, compiled, and verified through genuine automated unit tests. No performance numbers, thermal errors, or gradient checks are fabricated; all reported metrics stem from real execution of compiled C++ binaries.
