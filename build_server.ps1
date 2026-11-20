$ErrorActionPreference = "Stop"
$env:PATH = "$PWD\tools\w64devkit\bin;$env:PATH"

if (!(Test-Path "build/bin")) { New-Item -ItemType Directory -Path "build/bin" -Force }
if (!(Test-Path "build/obj")) { New-Item -ItemType Directory -Path "build/obj" -Force }

$sources = @(
  "backend/src/geometry/stl_parser.cpp",
  "backend/src/geometry/csv_parser.cpp",
  "backend/src/geometry/heat_sink_geometry.cpp",
  "backend/src/mesh/mesh_generator.cpp",
  "backend/src/mesh/mesh_validator.cpp",
  "backend/src/thermal/fem_solver.cpp",
  "backend/src/thermal/fem_3d_solver.cpp",
  "backend/src/solvers/cholesky_solver.cpp",
  "backend/src/solvers/conjugate_gradient_solver.cpp",
  "backend/src/solvers/gauss_seidel_solver.cpp",
  "backend/src/benchmark/solver_benchmark.cpp",
  "backend/src/performance/benchmark.cpp",
  "backend/src/hardware/adapters.cpp",
  "backend/src/backprop/network.cpp",
  "backend/src/api/job_manager.cpp",
  "backend/src/api/heat_sink_api.cpp",
  "backend/src/main.cpp"
)

$objs = @()

foreach ($src in $sources) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $obj = "build/obj/$name.o"
    $objs += $obj
    if (!(Test-Path $obj) -or ((Get-Item $src).LastWriteTime -gt (Get-Item $obj).LastWriteTime)) {
        Write-Host "Compiling $src..."
        & g++ -std=c++17 -O2 -Ibackend/include -Ibackend/third_party -c $src -o $obj
    }
}

Write-Host "Linking uew_server.exe..."
& g++ -std=c++17 -O2 $objs -lws2_32 -lmswsock -o build/bin/uew_server.exe

if (Test-Path "build/bin/uew_server.exe") {
    Copy-Item "build/bin/uew_server.exe" "uew_server.exe" -Force
    Write-Host "SUCCESS: uew_server.exe built successfully! Size: $((Get-Item uew_server.exe).Length) bytes"
} else {
    Write-Host "ERROR: Linking failed."
    exit 1
}
