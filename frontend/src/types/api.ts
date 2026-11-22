// UEW API types — mirroring C++ backend JSON responses

export interface ApiResponse<T = unknown> {
  success: boolean;
  message: string;
  data: T;
  error: string | null;
}

// Geometry
export interface BBox {
  xmin: number; xmax: number;
  ymin: number; ymax: number;
  zmin: number; zmax: number;
  dx: number; dy: number; dz: number;
}

export interface Triangle {
  v0: [number, number, number];
  v1: [number, number, number];
  v2: [number, number, number];
  n:  [number, number, number];
}

export interface STLResult {
  solid_name: string;
  triangle_count: number;
  triangles: Triangle[];
  bbox: BBox;
  file_size_note: string;
}

export interface TempNode {
  id: number;
  x: number; y: number; z: number;
  temp: number;
}

export interface CSVResult {
  node_count: number;
  t_min: number;
  t_max: number;
  nodes: TempNode[];
}

// Thermal
export interface ThermalResult {
  N: number;
  x_nodes: number[];
  T_fem: number[];
  T_exact: number[];
  x_midpoints: number[];
  T_fem_mid: number[];
  T_exact_mid: number[];
  error_mid: number[];
  max_midpoint_error: number;
  csv: string;
  params: {
    L: number; A: number; k: number; q: number; T_L: number; T_R: number;
  };
}

export interface MeshConvergenceResult {
  N: number;
  max_midpoint_error: number;
  x_nodes: number[];
  T_fem: number[];
  T_exact: number[];
  x_midpoints: number[];
  T_fem_mid: number[];
  T_exact_mid: number[];
}

// Performance
export type JobState = 'pending' | 'running' | 'done' | 'failed' | 'cancelled';

export interface JobStatus {
  id: string;
  type: string;
  state: JobState;
  progress: number;
  message: string;
  error: string;
  has_result: boolean;
  result?: BenchmarkJobResult;
}

export interface BenchTiming {
  median_ms: number;
  times_ms: number[];
  workers: number;
  csv: string;
  speedup?: number;
  efficiency?: number;
}

export interface SystemInfo {
  cpu: string;
  logical_cores: number;
  physical_cores: number;
  ram: string;
  compiler: string;
  opt_flags: string;
}

export interface BenchmarkJobResult {
  serial: BenchTiming;
  parallel: BenchTiming;
  max_serial_parallel_diff_C: number;
  agreement_ok: boolean;
  system: SystemInfo;
  config: { batch_size: number; mesh_size: number; workers: number };
}

// Hardware
export type DeviceType  = 'MCU' | 'FPGA' | 'JTAG';
export type DeviceState = 'disconnected' | 'connecting' | 'connected' | 'error';

export interface DeviceInfo {
  id: string;
  name: string;
  type: DeviceType;
  state: DeviceState;
  description: string;
  capabilities: string[];
  board_model: string;
  interface: string;
  simulated: boolean;
}

export interface TelemetryReading {
  device_id: string;
  timestamp_s: number;
  channel: string;
  value: number;
  unit: string;
  raw_hex: string;
}

// Backpropagation
export interface BackpropResult {
  x_input: number;
  y_target: number;
  initial_forward: { h: number; y_hat: number; J: number };
  analytical_gradients: { dJ_dw: number; dJ_db: number; dJ_dv: number; dJ_dc: number };
  fd_gradients: { dJ_dw: number; dJ_db: number; dJ_dv: number; dJ_dc: number };
  max_grad_error: number;
  verification_ok: boolean;
  updated_params: { w: number; b: number; v: number; c: number };
  history: Array<{ step: number; w: number; b: number; v: number; c: number; loss: number }>;
  loss_csv: string;
}

// ─────────────────────────────────────────────────────────
// 3D Heat Sink FEM & Numerical Solver Benchmark Types
// ─────────────────────────────────────────────────────────
export interface HeatSinkFin {
  y_start: number;
  thickness: number;
  height: number;
}

export interface HeatSinkParams {
  base_length: number;
  base_width: number;
  base_thickness: number;
  fin_count: number;
  fins: HeatSinkFin[];
  has_mounting_holes: boolean;
  hole_diameter: number;
  heat_source_length: number;
  heat_source_width: number;
  heat_source_x_off: number;
  heat_source_y_off: number;
}

export interface SurfaceFacet {
  v0: { x: number; y: number; z: number };
  v1: { x: number; y: number; z: number };
  v2: { x: number; y: number; z: number };
  normal: { x: number; y: number; z: number };
  tag: string;
}

export interface HeatSinkGeometryResult {
  is_valid: boolean;
  message: string;
  total_volume_m3: number;
  total_surface_area_m2: number;
  fin_surface_area_m2: number;
  base_surface_area_m2: number;
  heat_source_area_m2: number;
  facet_count: number;
  facets: SurfaceFacet[];
  stl_data: string;
}

export interface HeatSinkMeshNode {
  id: number;
  x: number;
  y: number;
  z: number;
}

export interface HeatSinkMeshFace {
  id: number;
  nodes: [number, number, number];
  boundary_type: string;
  area: number;
  normal: [number, number, number];
}

export interface HeatSinkMeshResult {
  is_valid: boolean;
  status_message: string;
  resolution: string;
  node_count: number;
  element_count: number;
  boundary_face_count: number;
  degrees_of_freedom: number;
  generation_time_ms: number;
  approx_memory_kb: number;
  min_quality: number;
  max_quality: number;
  mean_quality: number;
  nodes: HeatSinkMeshNode[];
  boundary_faces: HeatSinkMeshFace[];
}

export interface SolverMetric {
  solver_name: string;
  success: boolean;
  status_message: string;
  iterations: number;
  residual_norm: number;
  max_temp_diff_vs_reference_c: number;
  median_total_time_ms: number;
  min_total_time_ms: number;
  max_total_time_ms: number;
  median_factorization_time_ms: number;
  median_solve_time_ms: number;
  speedup_vs_reference: number;
  memory_kb: number;
  trial_times_ms: number[];
}

export interface HeatSinkBenchmarkResult {
  resolution: string;
  dof_count: number;
  element_count: number;
  non_zeros: number;
  assembly_time_ms: number;
  total_heat_in_w: number;
  total_heat_out_w: number;
  energy_balance_error_percent: number;
  min_temperature_c: number;
  max_temperature_c: number;
  thermal_resistance_k_w: number;
  solvers: SolverMetric[];
  reference_solution: number[];
}

export interface ConvergencePoint {
  resolution_name: string;
  node_count: number;
  element_count: number;
  max_temperature_c: number;
  min_temperature_c: number;
  avg_temperature_c: number;
  thermal_resistance_k_w: number;
  relative_change_percent: number;
  assembly_time_ms: number;
  cholesky_time_ms: number;
  cg_time_ms: number;
  gs_time_ms: number;
}

export interface HeatSinkConvergenceResult {
  is_converged: boolean;
  asymptotic_estimate_c: number;
  recommendation: string;
  points: ConvergencePoint[];
}

export interface HeatSinkThermalResult {
  success: boolean;
  solver_name: string;
  status_message: string;
  min_temperature_c: number;
  max_temperature_c: number;
  avg_temperature_c: number;
  thermal_resistance_k_w: number;
  total_heat_in_w: number;
  total_heat_out_w: number;
  energy_balance_error_percent: number;
  assembly_time_ms: number;
  factorization_time_ms: number;
  solve_time_ms: number;
  total_time_ms: number;
  iterations: number;
  residual_norm: number;
  memory_kb: number;
  nodal_temperatures: number[];
  nodes?: HeatSinkMeshNode[];
  boundary_faces?: HeatSinkMeshFace[];
}
