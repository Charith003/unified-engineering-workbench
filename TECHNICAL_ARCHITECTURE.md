# Technical Architecture — Unified Engineering Workbench (UEW)

## PASS Science Works R&D Internship 2026–2027

---

## 1. System Overview

UEW is a professional desktop engineering workbench that combines CAD geometry visualisation, Finite Element Method thermal analysis, multicore benchmarking, embedded-system integration simulation, and backpropagation computation. All numerical computation runs in a local C++17 HTTP server; React provides only the presentation layer.

```
┌───────────────────────────────────────────────────────────────┐
│                    Browser / Electron Shell                    │
│  React 19 + TypeScript + Vite + Tailwind CSS + Three.js       │
│  Plotly.js charts │ Lucide-React icons                        │
└────────────────────────────┬──────────────────────────────────┘
                             │  HTTP REST (JSON)
                             │  localhost:7800
┌────────────────────────────▼──────────────────────────────────┐
│                    uew_server.exe  (C++17)                     │
│  cpp-httplib HTTPS server │ nlohmann/json │ Eigen 3.4.0        │
├───────────┬───────────┬───────────┬───────────┬───────────────┤
│ Geometry  │   Mesh    │  Thermal  │ Numerical │  Benchmark    │
│ Module    │ Generator │  FEM 3D   │  Solvers  │  Engine       │
├───────────┴───────────┴───────────┴───────────┴───────────────┤
│          Legacy Modules (Phase 1 original)                     │
│  ThermalFEM2D │ PerformanceBench │ HardwareSim │ NeuralNet    │
└───────────────────────────────────────────────────────────────┘
```

---

## 2. Backend Modules

### 2.1 Geometry — `backend/src/geometry/heat_sink_geometry.cpp`

Parametric asymmetric electronic heat sink generator.

| Class / Type       | Responsibility                                               |
|--------------------|--------------------------------------------------------------|
| `FinConfig`        | Per-fin height, thickness, offset, position                 |
| `HeatSinkParams`   | Full geometry parameters (base, fins, heat source)          |
| `HeatSinkGeometry` | Generate facets, validate geometry, export ASCII STL        |
| `Vertex3D`         | 3-component coordinate                                      |
| `TriangleFacet`    | 3 vertices + unit normal (array layout)                     |

Key method: `HeatSinkGeometry::to_ascii_stl()` — produces a valid STL string directly from generated facets.

### 2.2 Mesh — `backend/src/mesh/mesh_generator.cpp`

Hexahedral-to-5-tet decomposition mesh generator.

| Type               | Fields                                                       |
|--------------------|--------------------------------------------------------------|
| `Node3D`           | `x, y, z` (double)                                          |
| `TetElement`       | `nodes[4]` (zero-based indices)                             |
| `SurfaceTriangle`  | `nodes[3]`, `boundary_type`, `area`, `normal[3]`            |
| `TetrahedralMesh`  | `nodes`, `elements`, `boundary_faces`                       |

Three resolution presets (`coarse`, `medium`, `fine`) control the grid density. Each hexahedral cell is decomposed into 5 tetrahedra using a consistent diagonal pattern.

Mesh validation (`mesh_validator.cpp`) checks:
- Non-finite node coordinates
- Degenerate elements (near-zero Jacobian)
- Inverted elements (negative volume)
- Invalid connectivity

### 2.3 Thermal FEM 3D — `backend/src/thermal/fem_3d_solver.cpp`

Steady-state Galerkin FEM on linear tetrahedra (TET4).

**Governing equation:**

$$-\nabla \cdot (k \nabla T) = Q$$

**Element stiffness matrix:**

$$K^e_{ij} = k \cdot V_e \cdot (\nabla N_i \cdot \nabla N_j)$$

where ∇N are the constant shape-function gradients for TET4, computed from the inverse Jacobian of the element.

**Robin (convective) boundary condition** on surface triangles:

$$\int_\Gamma h \, N_i N_j \, dA = \frac{h \cdot A}{12} \begin{bmatrix}2&1&1\\1&2&1\\1&1&2\end{bmatrix}$$

**Neumann (heat flux) load vector:**

$$f^e_i = \frac{q'' \cdot A}{3}$$

**Dirichlet** fixed-temperature BCs are enforced symmetrically (row/column zeroing + diagonal set to 1).

### 2.4 Numerical Solvers

| Solver               | File                              | Algorithm                          |
|----------------------|-----------------------------------|------------------------------------|
| `CholeskySolver`     | `src/solvers/cholesky_solver.cpp` | `Eigen::SimplicialLDLT`            |
| `ConjugateGradientSolver` | `src/solvers/conjugate_gradient_solver.cpp` | CG + Jacobi preconditioner |
| `GaussSeidelSolver`  | `src/solvers/gauss_seidel_solver.cpp` | SSOR (symmetric forward+backward sweep) |

All solvers implement `ISolver` and receive the same `AssembledThermalSystem` (triplet sparse K, dense F).

### 2.5 Benchmark Engine — `backend/src/benchmark/solver_benchmark.cpp`

Protocol:
1. Assemble K and F once for the given mesh.
2. For each solver: 1 warm-up run (discarded) + N measured trials.
3. Report **median** wall-clock time (μs resolution via `std::chrono::high_resolution_clock`).
4. Compute residual ‖KT − F‖₂ / ‖F‖₂.
5. Compute max |ΔT| vs Cholesky reference solution.

Mesh convergence study runs the full protocol at Coarse → Medium → Fine.

### 2.6 HTTP API — `backend/src/api/heat_sink_api.cpp`

| Endpoint                            | Method | Description                      |
|-------------------------------------|--------|----------------------------------|
| `/api/heat-sink/geometry/generate`  | POST   | Generate parametric heat sink    |
| `/api/heat-sink/mesh/generate`      | POST   | Generate tetrahedral mesh        |
| `/api/heat-sink/thermal/solve`      | POST   | Solve thermal FEM                |
| `/api/heat-sink/benchmark/run`      | POST   | Full 3-solver benchmark          |
| `/api/heat-sink/convergence/run`    | POST   | Mesh convergence study           |

---

## 3. Frontend Architecture

```
frontend/src/
├── App.tsx                   ← Router + shell (sidebar, header)
├── pages/
│   ├── OverviewPage.tsx      ← Dashboard
│   ├── GeometryPage.tsx      ← STL/CSV visualisation
│   ├── ThermalPage.tsx       ← 2D thermal analysis
│   ├── HeatSinkPage.tsx      ← 3D FEM heat sink (new)
│   ├── PerformancePage.tsx   ← Multicore benchmark
│   ├── HardwarePage.tsx      ← Embedded simulation
│   ├── NeuralPage.tsx        ← Backpropagation
│   ├── ReportsPage.tsx       ← Export
│   └── DocumentationPage.tsx ← Help
├── services/api.ts           ← Axios REST client
└── types/api.ts              ← TypeScript interfaces
```

**HeatSinkPage.tsx** has 5 tabs:
1. **Geometry** — parameter panel + STL statistics
2. **Mesh** — resolution selector + mesh quality table
3. **Thermal** — solver selector + temperature distribution histogram
4. **Benchmark** — solver comparison table + timing chart (Plotly bar)
5. **Convergence** — mesh convergence curves (Plotly line)

---

## 4. Build System

### Backend (GCC 13.2.0 / MinGW w64devkit)

```powershell
# Set PATH for w64devkit
$env:PATH = "$PWD\tools\w64devkit\bin;$env:PATH"

# Incremental build (all .o files → uew_server.exe)
.\build_server.ps1
```

Object files are written to `build/obj/*.o`. Link command pulls all `.o` files.

**Eigen** is header-only at `backend/third_party/Eigen/`.

### Frontend (pnpm + Vite)

```powershell
cd frontend
pnpm run build        # → frontend/dist/
```

The server serves `frontend/dist/` (or the symlinked `frontend_dist/`) as static files.

### CMake (optional — for IDE integration)

```powershell
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build --target uew_server
cmake --build build --target test_all_phases
```

---

## 5. Packaging

`Launch.exe` — native Windows launcher that:
1. Starts `uew_server.exe` in the background.
2. Opens the browser to `http://127.0.0.1:7800`.

No Node.js or internet connection is required at runtime. All assets are pre-bundled.

---

## 6. Dependency Inventory

| Library         | Version  | Licence   | Usage                         |
|-----------------|----------|-----------|-------------------------------|
| Eigen           | 3.4.0    | MPL-2.0   | Sparse linear algebra          |
| cpp-httplib     | 0.14.x   | MIT       | HTTP server                   |
| nlohmann/json   | 3.11.x   | MIT       | JSON serialisation             |
| GoogleTest      | 1.14.x   | BSD-3     | Unit tests                    |
| React           | 19.3     | MIT       | Frontend UI framework         |
| Three.js        | 0.187    | MIT       | 3D WebGL rendering            |
| Plotly.js       | 4.2      | MIT       | Interactive charts            |
| Lucide-React    | 1.66     | ISC       | Icon set                      |
| Tailwind CSS    | 4.3      | MIT       | Utility-first CSS             |
| Vite            | 8.3      | MIT       | Frontend bundler              |
| TypeScript      | 7.0      | Apache-2  | Type-safe JavaScript          |
| Axios           | 1.21     | MIT       | HTTP client                   |
| react-router-dom| 7.18     | MIT       | Client-side routing           |
