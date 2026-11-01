import { useState } from 'react';
import Plot from 'react-plotly.js';
import { Play, Info, Download } from 'lucide-react';
import { solveThermal, getThermalConvergence } from '../services/api';
import type { ThermalResult, MeshConvergenceResult } from '../types/api';

const DEFAULT_PARAMS = { L: 1.0, A: 1e-4, k: 10.0, q: 10000.0, T_L: 100.0, T_R: 20.0 };

function downloadCSV(csv: string, filename: string) {
  const blob = new Blob([csv], { type: 'text/csv' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url; a.download = filename; a.click();
  URL.revokeObjectURL(url);
}

export default function ThermalPage() {
  const [N, setN] = useState(10);
  const [params, setParams] = useState(DEFAULT_PARAMS);
  const [result, setResult] = useState<ThermalResult | null>(null);
  const [convergence, setConvergence] = useState<MeshConvergenceResult[] | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  async function runSolve() {
    setLoading(true); setError(''); setResult(null);
    try {
      const r = await solveThermal({ N, ...params });
      if (r.success) setResult(r.data);
      else setError(r.error ?? 'Solve failed');
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Network error');
    }
    setLoading(false);
  }

  async function runConvergence() {
    setLoading(true); setError(''); setConvergence(null);
    try {
      const r = await getThermalConvergence();
      if (r.success) setConvergence(r.data.meshes);
      else setError(r.error ?? 'Convergence study failed');
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Network error');
    }
    setLoading(false);
  }

  return (
    <div className="page-container">
      <div className="page-header">
        <h1 className="page-title">Thermal Analysis</h1>
        <p className="page-subtitle">
          1D steady-state FEM heat conduction: −d/dx(kA dT/dx) = qA.
          Two-node linear elements, TDMA solver, Dirichlet BCs.
        </p>
      </div>

      <div className="grid-2" style={{ marginBottom: 20 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Physical Parameters</div>
          <div className="alert alert-info" style={{ marginBottom: 14, fontSize: 12 }}>
            <Info size={13} />
            Default parameters from the assignment specification (L=1m, k=10 W/(m·K), q=10000 W/m³)
          </div>
          {[
            { key: 'L',   label: 'Length L', unit: 'm' },
            { key: 'k',   label: 'Conductivity k', unit: 'W/(m·K)' },
            { key: 'q',   label: 'Heat generation q', unit: 'W/m³' },
            { key: 'T_L', label: 'Left BC T(0)', unit: '°C' },
            { key: 'T_R', label: 'Right BC T(L)', unit: '°C' },
          ].map(({ key, label, unit }) => (
            <div className="form-group" key={key}>
              <label className="form-label">{label} [{unit}]</label>
              <input className="form-input" type="number" step="any"
                value={params[key as keyof typeof params]}
                onChange={e => setParams(p => ({ ...p, [key]: parseFloat(e.target.value) }))} />
            </div>
          ))}
          <div className="form-group">
            <label className="form-label">Number of elements N</label>
            <select className="form-select" value={N} onChange={e => setN(parseInt(e.target.value))}>
              {[10, 20, 40, 80].map(v => <option key={v} value={v}>N = {v}</option>)}
            </select>
          </div>
          <div style={{ display: 'flex', gap: 8, marginTop: 4 }}>
            <button className="btn btn-primary" onClick={runSolve} disabled={loading}>
              {loading ? <div className="spinner" /> : <Play size={14} />} Solve FEM
            </button>
            <button className="btn btn-accent" onClick={runConvergence} disabled={loading}>
              {loading ? <div className="spinner" /> : <Play size={14} />} Convergence Study
            </button>
          </div>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Governing Equation</div>
          <div className="code-block">
{`Steady-state 1D heat conduction:
  -d/dx(kA dT/dx) = qA

Element stiffness (h = L/N):
  Ke = (kA/h) * [[1,-1],[-1,1]]

Element load:
  Fe = (qAh/2) * [1,1]

Analytical solution:
  T(x) = T_L + (T_R - T_L)·x/L
        + q/(2k) · x·(L - x)

Boundary conditions:
  T(0) = ${params.T_L} °C  (Dirichlet)
  T(L) = ${params.T_R} °C  (Dirichlet)

Error evaluated at element MIDPOINTS
using FE interpolation (T = 0.5·T_i + 0.5·T_{i+1})
→ Reveals O(h²) convergence correctly.`}
          </div>
        </div>
      </div>

      {error && <div className="alert alert-error" style={{ marginBottom: 16 }}><Info size={14} />{error}</div>}

      {/* Single mesh result */}
      {result && (
        <>
          <div className="stat-grid" style={{ marginBottom: 20 }}>
            <div className="stat-card">
              <div className="stat-label">Elements N</div>
              <div className="stat-value">{result.N}</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">Max midpoint error</div>
              <div className="stat-value" style={{ fontSize: 18 }}>{result.max_midpoint_error.toExponential(4)}</div>
              <div className="stat-unit">°C</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">T(0) FEM</div>
              <div className="stat-value" style={{ fontSize: 18 }}>{result.T_fem[0].toFixed(3)}</div>
              <div className="stat-unit">°C (exact: {params.T_L})</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">T(L) FEM</div>
              <div className="stat-value" style={{ fontSize: 18 }}>{result.T_fem[result.T_fem.length-1].toFixed(3)}</div>
              <div className="stat-unit">°C (exact: {params.T_R})</div>
            </div>
          </div>

          <div className="chart-container" style={{ marginBottom: 20 }}>
            <Plot
              data={[
                {
                  x: result.x_nodes, y: result.T_fem,
                  name: `FEM (N=${result.N})`, type: 'scatter', mode: 'lines+markers',
                  line: { color: '#B7F34A', width: 2 },
                  marker: { size: 6, symbol: 'circle' },
                },
                {
                  x: result.x_nodes, y: result.T_exact,
                  name: 'Analytical', type: 'scatter', mode: 'lines',
                  line: { color: '#36B8C5', width: 2, dash: 'dash' },
                },
              ]}
              layout={{
                title: { text: `Temperature Distribution — N=${result.N} elements`, font: { color: 'var(--text-main)', size: 13 } },
                xaxis: { title: 'Position x (m)', gridcolor: '#E5EAE2' },
                yaxis: { title: 'Temperature (°C)', gridcolor: '#E5EAE2' },
                paper_bgcolor: 'transparent', plot_bgcolor: 'transparent',
                legend: { orientation: 'h', y: -0.2 },
                font: { family: 'Inter', color: 'var(--text-main)' },
                margin: { l: 60, r: 20, t: 40, b: 60 },
              }}
              config={{ displayModeBar: false, responsive: true }}
              style={{ width: '100%', height: 320 }}
            />
          </div>

          <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: 20 }}>
            <button className="btn btn-secondary btn-sm" onClick={() => downloadCSV(result.csv, `thermal_N${result.N}.csv`)}>
              <Download size={13} /> Export CSV
            </button>
          </div>
        </>
      )}

      {/* Convergence study */}
      {convergence && (
        <>
          <div className="card" style={{ marginBottom: 20 }}>
            <div className="card-title"><span className="accent-dot" />Mesh Convergence Study</div>
            <table className="data-table">
              <thead>
                <tr>
                  <th>N elements</th>
                  <th>h (m)</th>
                  <th>Max midpoint error (°C)</th>
                  <th>Error ratio (prev/curr)</th>
                  <th>Order</th>
                </tr>
              </thead>
              <tbody>
                {convergence.map((m, idx) => {
                  const h = 1.0 / m.N;
                  const prev = idx > 0 ? convergence[idx-1].max_midpoint_error : null;
                  const ratio = prev ? prev / m.max_midpoint_error : null;
                  const hPrev = idx > 0 ? 1.0 / convergence[idx-1].N : null;
                  const order = (ratio && hPrev) ? Math.log(ratio) / Math.log(hPrev / h) : null;
                  return (
                    <tr key={m.N}>
                      <td><strong>{m.N}</strong></td>
                      <td className="mono">{h.toFixed(4)}</td>
                      <td className="mono">{m.max_midpoint_error.toExponential(4)}</td>
                      <td className="mono">{ratio ? ratio.toFixed(2) : '—'}</td>
                      <td className="mono">{order ? order.toFixed(2) : '—'}</td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>

          <div className="grid-2" style={{ marginBottom: 20 }}>
            <div className="chart-container">
              <Plot
                data={convergence.map(m => ({
                  x: m.x_nodes, y: m.T_fem,
                  name: `N=${m.N}`, type: 'scatter', mode: 'lines',
                }))}
                layout={{
                  title: { text: 'FEM Solutions — All Meshes', font: { size: 12 } },
                  xaxis: { title: 'x (m)', gridcolor: '#E5EAE2' },
                  yaxis: { title: 'T (°C)', gridcolor: '#E5EAE2' },
                  paper_bgcolor: 'transparent', plot_bgcolor: 'transparent',
                  legend: { orientation: 'h', y: -0.25, font: { size: 11 } },
                  font: { family: 'Inter', size: 11 },
                  margin: { l: 55, r: 15, t: 35, b: 60 },
                }}
                config={{ displayModeBar: false, responsive: true }}
                style={{ width: '100%', height: 280 }}
              />
            </div>

            <div className="chart-container">
              <Plot
                data={[{
                  x: convergence.map(m => 1.0 / m.N),
                  y: convergence.map(m => m.max_midpoint_error),
                  type: 'scatter', mode: 'lines+markers',
                  name: 'Max midpoint error',
                  line: { color: '#F59E0B', width: 2 },
                  marker: { size: 8, color: '#F59E0B' },
                }]}
                layout={{
                  title: { text: 'Convergence Plot (log-log)', font: { size: 12 } },
                  xaxis: { title: 'h = L/N (m)', type: 'log', gridcolor: '#E5EAE2' },
                  yaxis: { title: 'Max midpoint error (°C)', type: 'log', gridcolor: '#E5EAE2' },
                  paper_bgcolor: 'transparent', plot_bgcolor: 'transparent',
                  font: { family: 'Inter', size: 11 },
                  margin: { l: 65, r: 15, t: 35, b: 55 },
                }}
                config={{ displayModeBar: false, responsive: true }}
                style={{ width: '100%', height: 280 }}
              />
            </div>
          </div>

          <div className="card">
            <div className="card-title"><span className="accent-dot" />Debugging Insight</div>
            <p style={{ fontSize: 13, color: 'var(--text-secondary)', lineHeight: 1.7, margin: 0 }}>
              <strong>Issue identified:</strong> An early version of the solver evaluated the FEM error
              at mesh <em>nodes</em> — the very interpolation points where the FEM solution is constrained
              to match boundary conditions. This gave misleadingly small errors and disguised the true
              spatial approximation accuracy.
              <br/><br/>
              <strong>Correction:</strong> Error is now evaluated at element <em>midpoints</em> using
              FE interpolation (T_mid = &frac12;&middot;T_i + &frac12;&middot;T_{'{i+1}'}). This correctly captures the inter-element
              approximation error and demonstrates the expected O(h&sup2;) convergence rate visible in the
              log-log plot above.
            </p>
          </div>
        </>
      )}
    </div>
  );
}
