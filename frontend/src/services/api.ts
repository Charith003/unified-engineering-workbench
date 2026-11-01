import axios, { type AxiosInstance } from 'axios';
import type {
  ApiResponse, STLResult, CSVResult, ThermalResult, MeshConvergenceResult,
  JobStatus, DeviceInfo, TelemetryReading, BackpropResult
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
