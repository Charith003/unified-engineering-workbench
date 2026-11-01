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
