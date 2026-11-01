# Unified Engineering Workbench (UEW) — System Architecture

**Product Name:** UEW (Unified Engineering Workbench)  
**Assignment:** PASS Science Works — R&D Internship 2026–2027  
**Architecture:** Decoupled C++20 Computation Core + React WebGL/Canvas Presentation Layer

---

## 1. Architectural Philosophy

UEW enforces a strict physical and computational separation between engineering calculation and interactive presentation:
- **Zero Scientific Computation in JavaScript:** All linear algebra, Thomas tridiagonal algorithms, mesh parsers, thread synchronization, and backpropagation gradients execute exclusively in compiled C++17/20.
- **Presentation Layer Exclusivity:** The React 19 application handles user input, Three.js 3D WebGL rendering, Plotly.js charts, and responsive layouts.
- **Air-Gapped Standalone Delivery:** The application requires no Node.js runtime, no npm packages, no internet connection, and no administrative privileges at execution time.

```
┌──────────────────────────────────────────────────────────┐
│              Browser / User Interface                    │
│   (React 19 + TypeScript + Tailwind CSS + Three.js)     │
└────────────────────────────┬─────────────────────────────┘
                             │ Local HTTP REST (JSON)
                             │ Loopback 127.0.0.1:7800-7900
┌────────────────────────────▼─────────────────────────────┐
│                 UEW C++ Core Server                      │
│        (cpp-httplib + nlohmann/json + MSVC/GCC)          │
├──────────────────────────────────────────────────────────┤
│ • STL & Temperature CSV Parsers                          │
│ • 1D Steady-State Thermal Finite Element Solver          │
│ • Bounded Multicore Worker Pool (std::thread)            │
│ • Embedded Device Adapters (MCU, FPGA, JTAG)             │
│ • Analytical Backpropagation & Central Differences       │
│ • Asynchronous Job Manager & Cancellation Controller     │
└──────────────────────────────────────────────────────────┘
```

---

## 2. Directory Layout & Module Boundaries

```
Unified-Engineering-Workbench/
├── backend/
│   ├── include/
│   │   ├── api/job_manager.hpp
│   │   └── core/
│   │       ├── geometry/ (stl_parser.hpp, csv_parser.hpp)
│   │       ├── thermal/ (fem_solver.hpp)
│   │       ├── performance/ (benchmark.hpp)
│   │       ├── hardware/ (adapters.hpp)
│   │       └── backprop/ (network.hpp)
│   ├── src/
│   │   ├── api/job_manager.cpp
│   │   ├── geometry/ (stl_parser.cpp, csv_parser.cpp)
│   │   ├── thermal/ (fem_solver.cpp)
│   │   ├── performance/ (benchmark.cpp)
│   │   ├── hardware/ (adapters.cpp)
│   │   ├── backprop/ (network.cpp)
│   │   └── main.cpp
│   └── third_party/ (httplib.h, json.hpp)
├── frontend/
│   ├── src/
│   │   ├── pages/ (Overview, Geometry, Thermal, Performance, Hardware, Neural, Reports, Documentation)
│   │   ├── services/ (api.ts)
│   │   ├── types/ (api.ts)
│   │   ├── App.tsx, main.tsx, index.css
│   └── dist/ (Precompiled standalone production assets)
├── tests/
│   ├── geometry/
│   ├── thermal/
│   ├── performance/
│   ├── hardware/
│   └── backprop/
├── data/
│   ├── geometry/
│   ├── ansys/
│   └── benchmarks/
├── packaging/
│   └── launcher.cpp (Native Windows Launch.exe)
├── Launch.exe
└── uew_server.exe
```

---

## 3. Communication Protocol & REST API

All API routes communicate over local loopback (`127.0.0.1`) on an automatically selected available port in range `7800–7900`.

Standard JSON response contract:
```json
{
  "success": true,
  "message": "Human readable status string",
  "data": { ... },
  "error": null
}
```

### Registered Endpoints:
1. `GET /api/health` — System status, uptime, module readiness
2. `POST /api/geometry/stl` — Parse and validate ASCII STL geometry
3. `POST /api/geometry/csv` — Parse and validate temperature node CSV
4. `POST /api/thermal/solve` — Solve 1D FEA thermal equation for arbitrary parameters
5. `POST /api/thermal/convergence` — Execute 4 standard meshes ($N=10, 20, 40, 80$)
6. `POST /api/performance/run` — Launch asynchronous multicore scaling benchmark
7. `GET /api/jobs/:id` — Poll background job status and progress
8. `POST /api/jobs/:id/cancel` — Terminate background compute job
9. `GET /api/hardware/devices` — Enumerate simulated hardware adapters
10. `POST /api/hardware/connect` — Connect device (with simulated fault recovery)
11. `POST /api/hardware/disconnect` — Disconnect target adapter
12. `POST /api/hardware/telemetry` — Sample sensor bus readings
13. `POST /api/backprop/run` — Compute exact gradients, finite-difference verification, and 100-step training trajectory
