# Statement of Ownership & Attribution of Work

**Project:** Unified Engineering Workbench (UEW)  
**Assignment:** PASS Science Works — R&D Internship 2026–2027  
**Author / Candidate:** Candidate Submission 2026

---

## 1. Original vs. Reused Work Breakdown

### Original Work Implemented by Author:
1. **Mathematical FEA Engine:**
   - 1D linear element formulation, element stiffness/load matrix derivation, assembly algorithm, Dirichlet boundary conditions, and Thomas tridiagonal linear solver in `backend/src/thermal/fem_solver.cpp`.
   - Analytical derivation of exact solution and midpoint discretization error proof ($\frac{125}{N^2}$).
2. **Multicore Parallel Scheduling Engine:**
   - Bounded C++ worker pool (`std::thread`, `std::mutex`, `std::condition_variable`) with deterministic fixed-seed pseudo-random generator, serial/parallel agreement verification ($\le 10^{-8\circ}\text{C}$), and 5-repetition median timing in `backend/src/performance/benchmark.cpp`.
3. **Embedded Hardware Mock Subsystem:**
   - Modular C++ interface `IHardwareAdapter` and state machine implementations for STM32H753 MCU, Artix-7 FPGA, and JTAG Debug Adapter, including simulated transient connection timeout fault and telemetry in `backend/src/hardware/adapters.cpp`.
4. **Analytical Backpropagation Engine:**
   - Manual scalar chain rule derivations for $\tanh$ regression, simultaneous 4-parameter update, central finite-difference verification engine ($\epsilon = 10^{-6}$), and 100-step training loop in `backend/src/backprop/network.cpp`.
5. **Geometry & Result Parsers:**
   - Resilient ASCII STL grammar parser (bounding box calculation, non-finite coord detection) in `backend/src/geometry/stl_parser.cpp`.
   - CSV temperature parser in `backend/src/geometry/csv_parser.cpp`.
6. **Native Packaging & Launcher:**
   - Native Windows `Launch.exe` in `packaging/launcher.cpp` providing automated server launch, loopback port discovery, default browser spawning, and clean process termination.
7. **Frontend Presentation Workbench:**
   - Interactive scientific workstation UI in React 19, TypeScript, Tailwind CSS, Three.js, and Plotly.js across all 8 modules.
8. **Automated Test Suite:**
   - 45 automated unit tests in GoogleTest (`uew_tests.exe`) covering all 6 functional domains.

### Third-Party Libraries & Reused Components:
All third-party libraries used in this project are permissible open-source components with permissive licenses:
1. **`cpp-httplib` (v0.18.0):** MIT License. Single-header C++ HTTP client/server library by Yuji Hirose. Used solely for loopback HTTP transport.
2. **`nlohmann/json` (v3.11.3):** MIT License. Single-header JSON library for modern C++ by Niels Lohmann.
3. **`GoogleTest` (v1.14.0):** BSD-3-Clause License. Automated testing framework by Google Inc.
4. **`React` (v19):** MIT License. Facebook / Meta open source.
5. **`Three.js` (r174):** MIT License. WebGL 3D rendering library.
6. **`Plotly.js` / `react-plotly.js`:** MIT License. Scientific plotting library.
7. **`Lucide React`:** ISC License. Open-source icons.

---

## 2. Integrity Declaration

The candidate certifies that all engineering algorithms, mathematical derivations, C++ solvers, and React interfaces presented in this repository reflect genuine implementation, testing, and documentation without fabricated benchmarks, falsified FEA errors, or simulated test passes.
