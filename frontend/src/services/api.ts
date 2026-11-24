import axios, { type AxiosInstance } from 'axios';
import type {
  ApiResponse, STLResult, CSVResult, ThermalResult, MeshConvergenceResult,
  JobStatus, DeviceInfo, TelemetryReading, BackpropResult,
  HeatSinkParams, HeatSinkGeometryResult, HeatSinkMeshResult,
  HeatSinkThermalResult, HeatSinkBenchmarkResult, HeatSinkConvergenceResult
} from '../types/api';

// In dev, Vite proxies /api → the C++ server.
// In production, the C++ server serves both frontend and API on the same port.
const BASE_URL = '';

const http: AxiosInstance = axios.create({
  baseURL: BASE_URL,
  timeout: 60000,
  headers: { 'Content-Type': 'application/json' },
});

// ─── Health ──────────────────────────────────────────────
export async function getHealth() {
  const r = await http.get<ApiResponse>('/api/health');
  return r.data;
}

// ─── Geometry ────────────────────────────────────────────
export async function parseSTL(content: string): Promise<ApiResponse<STLResult>> {
  const r = await http.post<ApiResponse<STLResult>>('/api/geometry/stl', { content });
  return r.data;
}

export async function parseCSV(content: string): Promise<ApiResponse<CSVResult>> {
  const r = await http.post<ApiResponse<CSVResult>>('/api/geometry/csv', { content });
  return r.data;
}

// ─── Thermal ─────────────────────────────────────────────
export interface ThermalParams {
  N: number; L?: number; A?: number; k?: number; q?: number; T_L?: number; T_R?: number;
}

export async function solveThermal(params: ThermalParams): Promise<ApiResponse<ThermalResult>> {
  const r = await http.post<ApiResponse<ThermalResult>>('/api/thermal/solve', params);
  return r.data;
}

export async function getThermalConvergence(): Promise<ApiResponse<{ meshes: MeshConvergenceResult[] }>> {
  const r = await http.post<ApiResponse<{ meshes: MeshConvergenceResult[] }>>('/api/thermal/convergence', {});
  return r.data;
}

// ─── Performance ──────────────────────────────────────────
export async function runBenchmark(config: { batch_size: number; mesh_size: number; workers: number }) {
  const r = await http.post<ApiResponse<{ job_id: string }>>('/api/performance/run', config);
  return r.data;
}

export async function getJobStatus(id: string): Promise<ApiResponse<JobStatus>> {
  const r = await http.get<ApiResponse<JobStatus>>(`/api/jobs/${id}`);
  return r.data;
}

export async function cancelJob(id: string) {
  const r = await http.post<ApiResponse>(`/api/jobs/${id}/cancel`);
  return r.data;
}

// ─── Hardware ─────────────────────────────────────────────
export async function getDevices(): Promise<ApiResponse<{ devices: DeviceInfo[] }>> {
  const r = await http.get<ApiResponse<{ devices: DeviceInfo[] }>>('/api/hardware/devices');
  return r.data;
}

export async function connectDevice(device_id: string) {
  const r = await http.post<ApiResponse>('/api/hardware/connect', { device_id });
  return r.data;
}

export async function disconnectDevice(device_id: string) {
  const r = await http.post<ApiResponse>('/api/hardware/disconnect', { device_id });
  return r.data;
}

export async function readTelemetry(device_id: string): Promise<ApiResponse<TelemetryReading>> {
  const r = await http.post<ApiResponse<TelemetryReading>>('/api/hardware/telemetry', { device_id });
  return r.data;
}

// ─── Backpropagation ─────────────────────────────────────
export interface BackpropConfig { lr?: number; steps?: number; epsilon?: number; }

export async function runBackprop(config: BackpropConfig): Promise<ApiResponse<BackpropResult>> {
  const r = await http.post<ApiResponse<BackpropResult>>('/api/backprop/run', config);
  return r.data;
}

// ─── 3D Heat Sink FEM & Solver Benchmark ─────────────────
export async function generateHeatSinkGeometry(params?: Partial<HeatSinkParams>): Promise<ApiResponse<HeatSinkGeometryResult>> {
  const r = await http.post<ApiResponse<HeatSinkGeometryResult>>('/api/heat-sink/geometry/generate', params ?? {});
  return r.data;
}

export async function generateHeatSinkMesh(payload: {
  params?: Partial<HeatSinkParams>;
  resolution: 'coarse' | 'medium' | 'fine';
}): Promise<ApiResponse<HeatSinkMeshResult>> {
  const r = await http.post<ApiResponse<HeatSinkMeshResult>>('/api/heat-sink/mesh/generate', {
    ...(payload.params ?? {}),
    resolution: payload.resolution
  });
  return r.data;
}

export async function solveHeatSinkThermal(payload: {
  params?: Partial<HeatSinkParams>;
  resolution: 'coarse' | 'medium' | 'fine';
  solver: 'cholesky' | 'cg' | 'gauss_seidel';
  material?: { name: string; thermal_conductivity: number; density: number; specific_heat: number };
  boundary_conditions?: {
    ambient_temperature_c: number;
    heat_transfer_coefficient: number;
    heat_source_power_w: number;
    fix_mounting_holes: boolean;
    fixed_mounting_temperature_c: number;
  };
  tolerance?: number;
  max_iterations?: number;
  relaxation_omega?: number;
}): Promise<ApiResponse<HeatSinkThermalResult>> {
  try {
    const r = await http.post<ApiResponse<HeatSinkThermalResult>>('/api/heat-sink/thermal/solve', {
      ...(payload.params ?? {}),
      resolution: payload.resolution,
      solver: payload.solver,
      material: payload.material,
      boundary_conditions: payload.boundary_conditions,
      tolerance: payload.tolerance,
      max_iterations: payload.max_iterations,
      relaxation_omega: payload.relaxation_omega
    }, {
      validateStatus: (status) => status < 500
    });
    return r.data;
  } catch (err: any) {
    if (err.response?.data) {
      return err.response.data;
    }
    throw err;
  }
}

export async function runHeatSinkBenchmark(payload: {
  params?: Partial<HeatSinkParams>;
  resolution: 'coarse' | 'medium' | 'fine';
  trial_count?: number;
  material?: { name: string; thermal_conductivity: number };
  boundary_conditions?: {
    ambient_temperature_c: number;
    heat_transfer_coefficient: number;
    heat_source_power_w: number;
  };
}): Promise<ApiResponse<HeatSinkBenchmarkResult>> {
  const r = await http.post<ApiResponse<HeatSinkBenchmarkResult>>('/api/heat-sink/benchmark/run', {
    ...(payload.params ?? {}),
    resolution: payload.resolution,
    trial_count: payload.trial_count ?? 5,
    material: payload.material,
    boundary_conditions: payload.boundary_conditions
  });
  return r.data;
}

export async function runHeatSinkConvergence(payload?: {
  params?: Partial<HeatSinkParams>;
  material?: { name: string; thermal_conductivity: number };
  boundary_conditions?: {
    ambient_temperature_c: number;
    heat_transfer_coefficient: number;
    heat_source_power_w: number;
  };
}): Promise<ApiResponse<HeatSinkConvergenceResult>> {
  const r = await http.post<ApiResponse<HeatSinkConvergenceResult>>('/api/heat-sink/convergence/run', {
    ...(payload?.params ?? {}),
    material: payload?.material,
    boundary_conditions: payload?.boundary_conditions
  });
  return r.data;
}
