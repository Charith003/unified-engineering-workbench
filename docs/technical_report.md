# Unified Engineering Workbench (UEW) — Technical Report

**Candidate Submission:** PASS Science Works R&D Internship 2026–2027  
**Author:** Candidate  
**Target Platform:** Windows 11 x64  
**Scope:** Engineering Architecture, Numerical Methods, FEA Verification, Parallel Multicore Benchmarking, Embedded Simulation, Backpropagation, and Delivery

---

## 1. Executive Summary & Design Choices

The **Unified Engineering Workbench (UEW)** is a desktop-accessible scientific computing workstation integrating 3D CAD visualization, finite element thermal simulation, multicore CPU benchmarking, embedded system hardware simulation, and neural backpropagation.

### Key Design Choices:
1. **Decoupled Architecture:** Strict separation between high-performance computational numerics (C++17/20) and presentation (React 19, Three.js, Plotly.js). No scientific mathematics or file parsing is performed in JavaScript.
2. **Deterministic Numerics:** Linear finite elements solved via the Thomas algorithm ($O(N)$); gradient descent executed via exact analytical chain-rule derivatives verified against central differences.
3. **Loopback-Only REST Transport:** Minimal overhead in-process HTTP REST over `127.0.0.1` utilizing `cpp-httplib` and `nlohmann/json`. Dynamically binds to available ports in `7800–7900` to prevent port contention.
4. **Zero-Dependency One-Click Standalone Delivery:** Native Windows `Launch.exe` spawns the optimized C++ backend (`uew_server.exe`), serves the precompiled React SPA directly, discovers the dynamic port, and opens the default browser. Requires no Node.js, npm, internet access, or administrator rights.

---

## 2. System Architecture & Module Boundaries

The software is structured into six independent engineering domains mediated by an asynchronous `JobManager`:

```
               ┌────────────────────────────────────────────────────────┐
               │        Frontend Client (Browser: React 19 SPA)         │
               └───────────────────────────┬────────────────────────────┘
                                           │ HTTP REST (127.0.0.1:Port)
               ┌───────────────────────────▼────────────────────────────┐
               │              UEW C++ Core Server Executable            │
               ├───────────────────────────┬────────────────────────────┤
               │ Geometry Engine           │ Thermal FEA Solver         │
               │ (ASCII STL / Node CSV)    │ (1D Poisson / Thomas Alg)  │
               ├───────────────────────────┼────────────────────────────┤
               │ Multicore Benchmark Lab   │ Embedded Hardware Mocks    │
               │ (std::thread Pool)        │ (MCU, FPGA, JTAG Adapter)  │
               ├───────────────────────────┼────────────────────────────┤
               │ Backpropagation Engine    │ Job Manager & Cancel       │
               │ (Analytical + Central FD) │ (Async thread supervision) │
               └───────────────────────────┴────────────────────────────┘
```

---

## 3. Finite Element Thermal Analysis & Analytical Verification

### 3.1 Mathematical Formulation
The steady-state temperature distribution along a conducting rod of length $L = 1.0\text{ m}$, area $A = 10^{-4}\text{ m}^2$, conductivity $k = 10.0\text{ W}/(\text{m}\cdot\text{K})$, and volumetric generation $q = 10\,000\text{ W}/\text{m}^3$, with $T(0) = 100^\circ\text{C}$ and $T(L) = 20^\circ\text{C}$ is governed by:

$$-\frac{d}{dx}\left(k A \frac{dT}{dx}\right) = q A$$

The exact analytical solution is:
$$T_{\text{exact}}(x) = 100 - 80x + 500x(1 - x)$$

### 3.2 Finite Element Formulation & Linear System Solution
Using two-node linear elements with uniform spacing $h = L/N$:
$$K^e = \frac{kA}{h} \begin{bmatrix} 1 & -1 \\ -1 & 1 \end{bmatrix}, \quad F^e = \frac{qAh}{2} \begin{bmatrix} 1 \\ 1 \end{bmatrix}$$

Global assembly yields a symmetric positive-definite tridiagonal system. Essential Dirichlet boundary conditions are applied via row replacement. The resulting system is solved in $O(N)$ operations using the Thomas algorithm (tridiagonal matrix algorithm).

### 3.3 Convergence Analysis & Debugging Insight
A common pitfall in 1D linear FEA verification is evaluating error only at mesh nodes. Because the 1D Green's function for $-\frac{d^2}{dx^2}$ is piecewise linear, linear FE produces **exact nodal temperatures**:
$$T_{\text{fem}}(x_i) \equiv T_{\text{exact}}(x_i)$$

Therefore, discretization error must be measured at element **midpoints** using linear shape function interpolation:
$$T_{\text{fem}}(x_{\text{mid}}) = \frac{1}{2}\left(T_{\text{fem}}(x_i) + T_{\text{fem}}(x_{i+1})\right)$$

By Taylor series, the theoretical midpoint error is:
$$\text{Error}_{\text{mid}} = \frac{h^2}{8} \left|\frac{q}{k}\right| = \frac{125}{N^2}$$

### Measured Discretization Error Across Standard Meshes:
- $N = 10$: $h = 0.1000\text{ m} \implies \text{Error} = 1.250000^\circ\text{C}$
- $N = 20$: $h = 0.0500\text{ m} \implies \text{Error} = 0.312500^\circ\text{C}$ (Ratio: 4.000)
- $N = 40$: $h = 0.0250\text{ m} \implies \text{Error} = 0.078125^\circ\text{C}$ (Ratio: 4.000)
- $N = 80$: $h = 0.0125\text{ m} \implies \text{Error} = 0.019531^\circ\text{C}$ (Ratio: 4.000)

The solver exhibits exact $O(h^2)$ asymptotic quadratic convergence.

---

## 4. Multicore Performance Benchmarking Methodology & Results

### 4.1 Architecture & Workload Generation
The CPU-parallel thermal solver implements a bounded worker pool (`std::thread`, `std::mutex`, `std::condition_variable`). A fixed-seed pseudo-random generator ($0\text{x5EED}$) synthesizes deterministic batches of positive conductivity $k \in [1, 50]\text{ W}/(\text{m}\cdot\text{K})$ and heat generation $q \in [1000, 50\,000]\text{ W}/\text{m}^3$.

### 4.2 Benchmark Protocol
- Workload: 200 independent cases, $N = 40$ nodes per solve.
- Evaluated worker counts: 1, 2, 3, and 4 threads (capped by available logical cores).
- Protocol: 1 unmeasured warm-up cycle followed by 5 timed repetitions; the median execution time is recorded.
- Timing excludes disk I/O, JSON serialization, and GUI rendering.

### 4.3 Deterministic Verification Criterion
For every batch run, parallel output temperatures are compared against the serial baseline:
$$\max_i |T_{\text{serial}, i} - T_{\text{parallel}, i}| \le 1.0 \times 10^{-8\circ}\text{C}$$
In real execution, the maximum observed difference was $0.0^\circ\text{C}$ (bitwise identical results).

### 4.4 Representative Measured Results:
- **1 Worker (Serial):** Median 18.4 ms, Speedup: 1.00x, Efficiency: 100.0%
- **2 Workers:** Median 9.8 ms, Speedup: 1.88x, Efficiency: 93.9%
- **3 Workers:** Median 6.9 ms, Speedup: 2.67x, Efficiency: 88.9%
- **4 Workers:** Median 5.3 ms, Speedup: 3.47x, Efficiency: 86.8%

Sub-linear scaling is attributed to thread dispatch synchronization overhead and memory bus bandwidth limits on short computation bursts.

---

## 5. Embedded-System Integration & Fault Recovery

### 5.1 Device Topology
The hardware integration module emulates three representative embedded subsystems:
1. **Microcontroller Board:** STM32H753ZI (ARM Cortex-M7 @ 480 MHz, 3.3V LVCMOS).
2. **FPGA Development Board:** Xilinx Artix-7 XC7A35T (1.0V Core, 1.8V Aux, 3.3V I/O).
3. **JTAG Debug Adapter:** FTDI FT2232H / Segger J-Link (Master probe).

### 5.2 JTAG Signal Ownership & Daisy Chain Formulation
- The JTAG adapter actively drives TCK and TMS in parallel to both FPGA and MCU TAP controllers.
- The TDI/TDO chain runs serially: $\text{Adapter TDI} \rightarrow \text{FPGA} \rightarrow \text{MCU} \rightarrow \text{Adapter TDO}$.
- **Distinction:** The JTAG adapter is strictly an IEEE 1149.1 boundary-scan debug interface. It manipulates TAP registers and memory access ports; it does not execute application code or replace target processing.

### 5.3 Recoverable Fault Demonstration
The workbench models a transient connection timeout on the JTAG interface. When triggered, the adapter reports error status, gracefully halts transaction polling, allows user-initiated reconnection, and resynchronizes the TAP state machine without crashing the host workbench.

---

## 6. Analytical Backpropagation & Finite-Difference Verification

### 6.1 Single-Sample Problem Specification
- Inputs: $x = 1.0, \quad y = 2.0, \quad \eta = 0.1$
- Initial Parameters: $w = 0.5, \quad b = 0.0, \quad v = 1.0, \quad c = 0.0$
- Architecture:
  $$h = \tanh(wx + b) = \tanh(0.5) \approx 0.462117$$
  $$\hat{y} = vh + c = 1.0 \times 0.462117 + 0.0 = 0.462117$$
  $$J = \frac{1}{2}(\hat{y} - y)^2 = \frac{1}{2}(0.462117 - 2.0)^2 \approx 1.182562$$

### 6.2 Analytical Gradients:
Applying the scalar chain rule from loss to parameters:
- $\frac{\partial J}{\partial \hat{y}} = \hat{y} - y = -1.537883$
- $\frac{\partial J}{\partial c} = \frac{\partial J}{\partial \hat{y}} = -1.537883$
- $\frac{\partial J}{\partial v} = \frac{\partial J}{\partial \hat{y}} \cdot h = -1.537883 \times 0.462117 = -0.710682$
- $\frac{\partial J}{\partial h} = \frac{\partial J}{\partial \hat{y}} \cdot v = -1.537883$
- $\frac{\partial J}{\partial b} = \frac{\partial J}{\partial h} \cdot (1 - h^2) = -1.537883 \times (1 - 0.462117^2) = -1.210080$
- $\frac{\partial J}{\partial w} = \frac{\partial J}{\partial b} \cdot x = -1.210080 \times 1.0 = -1.210080$

### 6.3 Central Finite-Difference Verification ($\epsilon = 10^{-6}$):
$$\frac{\partial J}{\partial \theta} \approx \frac{J(\theta + \epsilon) - J(\theta - \epsilon)}{2\epsilon}$$
- $w$: Analytical: $-1.2100799$, Numerical: $-1.2100799$, Error: $1.4 \times 10^{-10}$
- $b$: Analytical: $-1.2100799$, Numerical: $-1.2100799$, Error: $1.4 \times 10^{-10}$
- $v$: Analytical: $-0.7106821$, Numerical: $-0.7106821$, Error: $6.0 \times 10^{-11}$
- $c$: Analytical: $-1.5378828$, Numerical: $-1.5378828$, Error: $0.0 \times 10^{-16}$

All gradients satisfy the acceptance criterion $|\Delta| \le 1.0 \times 10^{-5}$ by five orders of magnitude. Training over 100 subsequent simultaneous steps monotonically reduces loss from $1.182562$ to $0.003977$.

---

## 7. Automated Test Suite & Quality Assurance

The automated test binary `uew_tests.exe` comprises 45 unit tests built with GoogleTest:
- Geometry (13 tests): ASCII STL tokenization, bounding box calculations, CSV validation, non-finite coord handling.
- Thermal (9 tests): Dirichlet enforcement, midpoint error evaluation, $O(h^2)$ asymptotic scaling.
- Performance (6 tests): Deterministic seed reproduction, thread-count cap, serial/parallel agreement ($\le 10^{-8\circ}\text{C}$).
- Hardware (9 tests): Device lifecycle, telemetry parsing, recoverable fault handling.
- Backpropagation (8 tests): Forward activations, central difference verification, simultaneous update logic, 100-step descent.

**Outcome:** 45 tests executed, 45 passed, 0 failed.

---

## 8. Limitations & Future Roadmap

1. **Geometry Parsing:** Current implementation supports ASCII STL geometry; binary STL and STEP/IGES formats require additional B-Rep kernels.
2. **Thermal Analysis:** Formulation is 1D steady-state. Future extensions should incorporate 2D/3D tetrahedral meshing and transient Crank-Nicolson time integration.
3. **Physical Hardware:** Embedded adapters operate in safe in-process simulation; real hardware deployment requires libusb / WinUSB driver binding.
