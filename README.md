# Unified Engineering Workbench (UEW)

> **Engineering Intelligence. Unified.**  
> PASS Science Works — R&D Internship Technical Assignment 2026–2027  
> Target Platform: Windows 11 x64  
> Core Computational Language: C++17/20 | Presentation: React 19 + TypeScript + Three.js

---

## 1. Quick Start: One-Click Execution

UEW is fully packaged for standalone Windows 11 execution with **zero prerequisites** (no Node.js, no npm, no internet, and no administrator rights required):

1. Double-click **`Launch.exe`** in the root directory.
2. The native launcher automatically starts the compiled C++ backend (`uew_server.exe`), selects an available loopback port (`7800–7900`), and opens your default web browser to the workbench interface.
3. To stop the application cleanly, close the console window or press `Ctrl+C`.

### Alternative: VS Code / Antigravity IDE Launch Configuration
1. Open the repository root in VS Code or Antigravity IDE.
2. Press **F5** or navigate to **Run and Debug** and select:
   - `Launch UEW (One-Click Standalone)` — runs `Launch.exe`.
   - `Run Automated Tests (uew_tests.exe)` — executes the 45-test verification suite.

---

## 2. Integrated Engineering Modules

| Module | Core Features | Computational Backend (C++) | Frontend Visualization |
|---|---|---|---|
| **Overview Dashboard** | Module health status, quick launch shortcuts, telemetry preview | Health monitoring, system uptime | Dynamic cards, live backend connection dot |
| **Geometry Studio** | ASCII STL 3D visualization, synthetic temperature node point cloud | ASCII STL parser, CSV parser, bounding box calculator | Three.js WebGL orbit viewer, Plotly 3D scatter point cloud |
| **Thermal Analysis** | 1D steady-state heat equation, analytical solution, $N=10, 20, 40, 80$ | Weak-form FE assembly, Dirichlet enforcement, Thomas tridiagonal solver | Numerical vs analytical curves, $O(h^2)$ midpoint error log-log plot |
| **3D Heat Sink FEM** | Parametric asymmetric heat sink, TET4 mesh, 3-solver benchmark | Conforming 5-tet decomposition, Robin convection BCs, Cholesky/CG/SSOR solvers | Plotly histograms, runtime distributions, mesh convergence curves |
| **Performance Lab** | Multicore CPU scaling, worker pool (1P–4P), 5-run median timings | `std::thread` pool, deterministic batch generator, agreement check | Execution time, speedup factor ($S = T_1/T_p$), parallel efficiency |
| **Hardware Integration** | Microcontroller, FPGA, and JTAG Debug Adapter simulation | State machine mock adapters, simulated timeout fault & recovery | Device controls, live sensor telemetry, schematic documentation |
| **Neural Computation** | Exact scalar $\tanh$ regression, handwritten-gradient backpropagation | Analytical partial derivatives, central finite-difference verification ($\le 10^{-5}$) | 100-step training loss curve, gradient comparison table |
| **Reports & Exports** | Centralized export repository for CSV datasets, logs, summaries | Pure C++ CSV and JSON serialization | Downloadable CSV datasets, schema viewer |
| **Documentation** | Technical theory, mathematical proofs, test summaries | N/A | Interactive markdown documentation viewer |

---

## 3. Automated Test Suite

Run the precompiled GoogleTest verification binary directly:

```powershell
.\uew_tests.exe
```

### Summary of Executed Tests:
- **Total Tests:** 65 across all test suites
- **Legacy Suite (`uew_tests.exe`):** 45 passed / 0 failed (Backprop: 8, Hardware: 9, Performance: 6, Thermal 1D: 9, CSV: 6, STL: 7)
- **3D Thermal & Solvers Suite (`test_all_phases.exe`):** 20 passed / 0 failed (Geometry: 7, Mesh: 5, Thermal 3D: 2, Solvers: 4, Benchmark: 2)
- **Overall Pass Rate:** 100% (65 / 65)

```powershell
# Run the 3D Thermal FEM & Solvers test suite:
.\test_all_phases.exe
```

---

## 4. Source Compilation (CMake & GCC)

If rebuilding from source:

### Prerequisites:
- CMake 3.20+
- C++17 compatible compiler (MinGW-w64 GCC 13.2+ or MSVC 2022)
- Node.js 18+ (only required if rebuilding frontend assets)

### Commands:
```powershell
# 1. Build Frontend Production Bundle
cd frontend
npm install
npm run build
cd ..

# 2. Build C++ Server & Tests
g++ -std=c++17 -O3 -Ibackend/include -Ibackend/third_party `
    backend/src/geometry/stl_parser.cpp `
    backend/src/geometry/csv_parser.cpp `
    backend/src/thermal/fem_solver.cpp `
    backend/src/performance/benchmark.cpp `
    backend/src/hardware/adapters.cpp `
    backend/src/backprop/network.cpp `
    backend/src/api/job_manager.cpp `
    backend/src/main.cpp -lws2_32 -o uew_server.exe

# 3. Build Launcher
g++ -std=c++17 -O2 packaging/launcher.cpp -o Launch.exe
```

---

## 5. Technical Documentation Index

### Core Technical Reports:
- [Technical Architecture Specification](TECHNICAL_ARCHITECTURE.md)
- [3D Thermal FEM Numerical Methods](NUMERICAL_METHODS.md)
- [Solver Benchmark Methodology](BENCHMARK_METHODOLOGY.md)
- [Numerical Validation & Verification Report](VALIDATION_REPORT.md)
- [System Architecture](docs/architecture.md)
- [1D Thermal FEA Mathematical Theory](docs/thermal_theory.md)
- [Embedded Hardware Integration Specification](docs/embedded_design.md)
- [Quality Assurance & Automated Testing Report](docs/testing.md)
- [Technical Report (6-page specification format)](docs/technical_report.md)
- [Candidate Handwritten Derivation Template](docs/handwritten_appendix_template.md)
- [AI Usage Log & Human Validation Audit](docs/AI_usage_log.md)
- [Statement of Ownership & Attribution](docs/ownership.md)
- [Open Source Licenses](LICENSES.md)
