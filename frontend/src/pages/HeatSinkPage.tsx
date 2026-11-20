import { useState } from 'react';
import { Layers, Play, Download, AlertCircle, CheckCircle, Info } from 'lucide-react';
import {
  generateHeatSinkGeometry,
  generateHeatSinkMesh,
  solveHeatSinkThermal,
  runHeatSinkBenchmark,
  runHeatSinkConvergence
} from '../services/api';
import type {
  HeatSinkGeometryResult, HeatSinkMeshResult,
  HeatSinkThermalResult, HeatSinkBenchmarkResult,
  HeatSinkConvergenceResult
} from '../types/api';
import Plot from 'react-plotly.js';

type TabId = 'geometry' | 'mesh' | 'thermal' | 'benchmark' | 'convergence';
type Resolution = 'coarse' | 'medium' | 'fine';
type SolverType = 'cholesky' | 'cg' | 'gauss_seidel';

interface SimConfig {
  resolution: Resolution;
  solver: SolverType;
  ambientTemp: number;
  htCoeff: number;
  heatPower: number;
  trialCount: number;
}

const SOLVER_COLORS = ['#B7F34A', '#36B8C5', '#F4A261'];

export default function HeatSinkPage() {
  const [activeTab, setActiveTab] = useState<TabId>('geometry');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [config, setConfig] = useState<SimConfig>({
    resolution: 'coarse',
    solver: 'cholesky',
    ambientTemp: 25,
    htCoeff: 15,
    heatPower: 15,
    trialCount: 5,
  });

  const [geomResult, setGeomResult] = useState<HeatSinkGeometryResult | null>(null);
  const [meshResult, setMeshResult] = useState<HeatSinkMeshResult | null>(null);
  const [thermalResult, setThermalResult] = useState<HeatSinkThermalResult | null>(null);
  const [benchResult, setBenchResult] = useState<HeatSinkBenchmarkResult | null>(null);
  const [convResult, setConvResult] = useState<HeatSinkConvergenceResult | null>(null);

  const bcPayload = () => ({
    boundary_conditions: {
      ambient_temperature_c: config.ambientTemp,
      heat_transfer_coefficient: config.htCoeff,
      heat_source_power_w: config.heatPower,
      fix_mounting_holes: false,
      fixed_mounting_temperature_c: 30,
    }
  });

  async function handleGenerate() {
    setLoading(true); setError('');
    try {
      const r = await generateHeatSinkGeometry();
      if (r.success) { setGeomResult(r.data as HeatSinkGeometryResult); setActiveTab('geometry'); }
      else setError((r as any).error ?? 'Geometry generation failed');
    } catch (e: any) { setError(e.message ?? 'Network error'); }
    setLoading(false);
  }

  async function handleMesh() {
    setLoading(true); setError('');
    try {
      const r = await generateHeatSinkMesh({ resolution: config.resolution });
      if (r.success) { setMeshResult(r.data as HeatSinkMeshResult); setActiveTab('mesh'); }
      else setError((r as any).error ?? 'Mesh generation failed');
    } catch (e: any) { setError(e.message ?? 'Network error'); }
    setLoading(false);
  }

  async function handleSolve() {
    setLoading(true); setError('');
    try {
      const r = await solveHeatSinkThermal({
        resolution: config.resolution,
        solver: config.solver,
        ...bcPayload()
      });
      if (r.success) { setThermalResult(r.data as HeatSinkThermalResult); setActiveTab('thermal'); }
      else setError((r as any).error ?? 'Thermal solve failed');
    } catch (e: any) { setError(e.message ?? 'Network error'); }
    setLoading(false);
  }

  async function handleBenchmark() {
    setLoading(true); setError('');
    try {
      const r = await runHeatSinkBenchmark({
        resolution: config.resolution,
        trial_count: config.trialCount,
        ...bcPayload()
      });
      if (r.success) { setBenchResult(r.data as HeatSinkBenchmarkResult); setActiveTab('benchmark'); }
      else setError((r as any).error ?? 'Benchmark failed');
    } catch (e: any) { setError(e.message ?? 'Network error'); }
    setLoading(false);
  }

  async function handleConvergence() {
    setLoading(true); setError('');
    try {
      const r = await runHeatSinkConvergence(bcPayload());
      if (r.success) { setConvResult(r.data as HeatSinkConvergenceResult); setActiveTab('convergence'); }
      else setError((r as any).error ?? 'Convergence study failed');
    } catch (e: any) { setError(e.message ?? 'Network error'); }
    setLoading(false);
  }

  const tabs: { id: TabId; label: string; badge?: string }[] = [
    { id: 'geometry', label: 'Geometry', badge: geomResult ? '✓' : undefined },
    { id: 'mesh', label: 'Mesh', badge: meshResult ? '✓' : undefined },
    { id: 'thermal', label: 'Thermal', badge: thermalResult ? '✓' : undefined },
    { id: 'benchmark', label: 'Solver Benchmark', badge: benchResult ? '✓' : undefined },
    { id: 'convergence', label: 'Convergence', badge: convResult ? '✓' : undefined },
  ];

  return (
    <div className="page-container">
      <div className="page-header">
        <h1 className="page-title">3D Thermal FEM — Heat Sink Simulation</h1>
        <p className="page-subtitle">
          Asymmetric electronic heat sink: parametric geometry → tetrahedral mesh → 3D FEM assembly →
          Cholesky / CG / Gauss-Seidel solver comparison
        </p>
      </div>

      {/* Config Controls */}
      <div className="card" style={{ marginBottom: 20 }}>
        <div className="card-title"><span className="accent-dot" />Simulation Configuration</div>
        <div className="grid-3" style={{ gap: 16 }}>
          <div className="form-group">
            <label className="form-label">Mesh Resolution</label>
            <select className="form-input" value={config.resolution}
              onChange={e => setConfig(c => ({ ...c, resolution: e.target.value as Resolution }))}>
              <option value="coarse">Coarse (~1,500 tets)</option>
              <option value="medium">Medium (~6,000 tets)</option>
              <option value="fine">Fine (~20,000 tets)</option>
            </select>
          </div>
          <div className="form-group">
            <label className="form-label">Solver (Single Run)</label>
            <select className="form-input" value={config.solver}
              onChange={e => setConfig(c => ({ ...c, solver: e.target.value as SolverType }))}>
              <option value="cholesky">Sparse Cholesky (Direct)</option>
              <option value="cg">Conjugate Gradient (CG)</option>
              <option value="gauss_seidel">Gauss-Seidel (Iterative)</option>
            </select>
          </div>
          <div className="form-group">
            <label className="form-label">Heat Source Power [W]</label>
            <input className="form-input" type="number" step="1" min="1" max="200"
              value={config.heatPower}
              onChange={e => setConfig(c => ({ ...c, heatPower: parseFloat(e.target.value) }))} />
          </div>
          <div className="form-group">
            <label className="form-label">Ambient Temperature [°C]</label>
            <input className="form-input" type="number" step="1" min="-40" max="60"
              value={config.ambientTemp}
              onChange={e => setConfig(c => ({ ...c, ambientTemp: parseFloat(e.target.value) }))} />
          </div>
          <div className="form-group">
            <label className="form-label">Heat Transfer Coeff h [W/(m²·K)]</label>
            <input className="form-input" type="number" step="1" min="1" max="1000"
              value={config.htCoeff}
              onChange={e => setConfig(c => ({ ...c, htCoeff: parseFloat(e.target.value) }))} />
          </div>
          <div className="form-group">
            <label className="form-label">Benchmark Trials</label>
            <input className="form-input" type="number" step="1" min="1" max="20"
              value={config.trialCount}
              onChange={e => setConfig(c => ({ ...c, trialCount: parseInt(e.target.value) }))} />
          </div>
        </div>

        <div style={{ display: 'flex', gap: 10, flexWrap: 'wrap', marginTop: 16 }}>
          <button className="btn btn-primary" onClick={handleGenerate} disabled={loading}>
            {loading ? '…' : <><Layers size={14} style={{ marginRight: 6 }} />Generate Geometry</>}
          </button>
          <button className="btn btn-secondary" onClick={handleMesh} disabled={loading}>
            {loading ? '…' : 'Generate Mesh'}
          </button>
          <button className="btn btn-secondary" onClick={handleSolve} disabled={loading}>
            {loading ? '…' : <><Play size={13} style={{ marginRight: 5 }} />Run Thermal Solve</>}
          </button>
          <button className="btn btn-secondary" onClick={handleBenchmark} disabled={loading}>
            {loading ? '…' : 'Run Solver Benchmark'}
          </button>
          <button className="btn btn-secondary" onClick={handleConvergence} disabled={loading}>
            {loading ? '…' : 'Mesh Convergence Study'}
          </button>
        </div>

        {loading && (
          <div className="alert alert-info" style={{ marginTop: 12 }}>
            <Info size={13} /> Computing — please wait…
          </div>
        )}
        {error && (
          <div className="alert alert-error" style={{ marginTop: 12 }}>
            <AlertCircle size={13} /> {error}
          </div>
        )}
      </div>

      {/* Tab Navigation */}
      <div style={{ display: 'flex', gap: 4, marginBottom: 16, borderBottom: '2px solid #E5EAE2' }}>
        {tabs.map(t => (
          <button key={t.id}
            onClick={() => setActiveTab(t.id)}
            style={{
              padding: '8px 16px',
              borderBottom: activeTab === t.id ? '2px solid #B7F34A' : '2px solid transparent',
              marginBottom: -2,
              background: 'none',
              border: 'none',
              color: activeTab === t.id ? '#202820' : '#758078',
              fontWeight: activeTab === t.id ? 600 : 400,
              fontSize: 14,
              cursor: 'pointer',
              display: 'flex', alignItems: 'center', gap: 6,
            }}>
            {t.label}
            {t.badge && <span style={{ background: '#B7F34A', color: '#202820', fontSize: 10, padding: '1px 5px', borderRadius: 10 }}>{t.badge}</span>}
          </button>
        ))}
      </div>

      {/* Tab Content */}
      {activeTab === 'geometry' && <GeometryTab result={geomResult} />}
      {activeTab === 'mesh' && <MeshTab result={meshResult} />}
      {activeTab === 'thermal' && <ThermalTab result={thermalResult} />}
      {activeTab === 'benchmark' && <BenchmarkTab result={benchResult} />}
      {activeTab === 'convergence' && <ConvergenceTab result={convResult} />}
    </div>
  );
}

// ─── Geometry Tab ───────────────────────────────────────────────────────────
function GeometryTab({ result }: { result: HeatSinkGeometryResult | null }) {
  if (!result) return <EmptyState label="Click 'Generate Geometry' to build the asymmetric heat sink" />;

  const toMM3 = (v: number) => (v * 1e9).toFixed(2);
  const toMM2 = (a: number) => (a * 1e6).toFixed(2);

  return (
    <div>
      <div className="grid-2" style={{ gap: 16, marginBottom: 16 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Geometry Properties</div>
          <table className="data-table">
            <tbody>
              <tr><td>Total Volume</td><td>{toMM3(result.total_volume_m3)} mm³</td></tr>
              <tr><td>Total Surface Area</td><td>{toMM2(result.total_surface_area_m2)} mm²</td></tr>
              <tr><td>Heat Source Area</td><td>{toMM2(result.heat_source_area_m2)} mm²</td></tr>
              <tr><td>Surface Facets</td><td>{result.facet_count.toLocaleString()}</td></tr>
              <tr><td>Status</td><td>
                <span style={{ color: result.is_valid ? '#4ADE80' : '#F87171' }}>
                  {result.is_valid ? '✓ Valid' : `✗ ${result.message}`}
                </span>
              </td></tr>
            </tbody>
          </table>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Heat Sink Specification</div>
          <table className="data-table">
            <tbody>
              <tr><td>Base Dimensions</td><td>120 × 80 × 8 mm</td></tr>
              <tr><td>Fin Count</td><td>7 asymmetric fins</td></tr>
              <tr><td>Fin Height Range</td><td>22 – 45 mm</td></tr>
              <tr><td>Fin Spacing</td><td>Non-uniform</td></tr>
              <tr><td>Heat Source</td><td>30 × 25 mm patch</td></tr>
              <tr><td>Material</td><td>Aluminum 6061-T6</td></tr>
              <tr><td>k (conductivity)</td><td>205 W/(m·K)</td></tr>
            </tbody>
          </table>
        </div>
      </div>

      {result.stl_data && (
        <div className="card">
          <div className="card-title"><span className="accent-dot" />ASCII STL Export</div>
          <div className="alert alert-info" style={{ marginBottom: 10 }}>
            <Info size={13} /> {result.facet_count} triangular facets — ready for CAD import
          </div>
          <pre style={{
            background: '#F7F8F5', border: '1px solid #E5EAE2', borderRadius: 6,
            padding: 12, fontSize: 11, maxHeight: 160, overflow: 'auto',
            fontFamily: 'monospace', color: '#758078'
          }}>
            {result.stl_data.substring(0, 1500)}…
          </pre>
          <button className="btn btn-secondary btn-sm" style={{ marginTop: 10 }}
            onClick={() => {
              const blob = new Blob([result.stl_data], { type: 'text/plain' });
              const a = document.createElement('a');
              a.href = URL.createObjectURL(blob);
              a.download = 'heat_sink.stl'; a.click();
            }}>
            <Download size={13} style={{ marginRight: 6 }} />Download STL
          </button>
        </div>
      )}
    </div>
  );
}

// ─── Mesh Tab ────────────────────────────────────────────────────────────────
function MeshTab({ result }: { result: HeatSinkMeshResult | null }) {
  if (!result) return <EmptyState label="Click 'Generate Mesh' to generate the tetrahedral FEM mesh" />;

  const qualityData = [{
    type: 'bar' as const,
    x: ['Min Quality', 'Mean Quality', 'Max Quality'],
    y: [result.min_quality, result.mean_quality, result.max_quality],
    marker: { color: ['#F4A261', '#B7F34A', '#36B8C5'] },
  }];

  return (
    <div>
      <div className="grid-2" style={{ gap: 16, marginBottom: 16 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Mesh Statistics</div>
          <table className="data-table">
            <tbody>
              <tr><td>Resolution</td><td style={{ fontWeight: 600 }}>{result.resolution}</td></tr>
              <tr><td>Nodes (DOFs)</td><td>{result.node_count.toLocaleString()}</td></tr>
              <tr><td>Tetrahedral Elements</td><td>{result.element_count.toLocaleString()}</td></tr>
              <tr><td>Boundary Faces</td><td>{result.boundary_face_count.toLocaleString()}</td></tr>
              <tr><td>Approx Memory</td><td>{result.approx_memory_kb.toFixed(1)} KB</td></tr>
              <tr><td>Generation Time</td><td>{result.generation_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Status</td><td>
                <span style={{ color: result.is_valid ? '#4ADE80' : '#F87171' }}>
                  {result.is_valid ? '✓ Valid' : '✗ Invalid'}
                </span>
              </td></tr>
            </tbody>
          </table>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Element Quality Metrics</div>
          <Plot
            data={qualityData}
            layout={{
              height: 220,
              margin: { l: 40, r: 10, t: 10, b: 40 },
              paper_bgcolor: '#FFFFFF',
              plot_bgcolor: '#F7F8F5',
              yaxis: { range: [0, 1], title: 'Quality [0–1]', tickfont: { size: 10 } },
              xaxis: { tickfont: { size: 11 } },
              showlegend: false,
            }}
            config={{ displayModeBar: false }}
            style={{ width: '100%' }}
          />
        </div>
      </div>

      <div className="card">
        <div className="card-title"><span className="accent-dot" />Boundary Condition Summary</div>
        <div style={{ display: 'flex', gap: 16, flexWrap: 'wrap' }}>
          {(['Convection', 'HeatSource', 'FixedTemp', 'Adiabatic'] as const).map(bt => {
            const count = result.boundary_faces.filter(f => f.boundary_type === bt).length;
            if (!count) return null;
            const colors: Record<string, string> = {
              Convection: '#36B8C5', HeatSource: '#F4A261',
              FixedTemp: '#B7F34A', Adiabatic: '#E5EAE2'
            };
            return (
              <div key={bt} style={{
                padding: '8px 16px', borderRadius: 8,
                background: colors[bt] + '22',
                border: `1px solid ${colors[bt]}55`,
              }}>
                <div style={{ fontSize: 10, color: '#758078', textTransform: 'uppercase', letterSpacing: 1 }}>{bt}</div>
                <div style={{ fontSize: 20, fontWeight: 700, color: '#202820' }}>{count}</div>
                <div style={{ fontSize: 11, color: '#758078' }}>faces</div>
              </div>
            );
          })}
        </div>
        <div className="alert alert-info" style={{ marginTop: 12 }}>
          <Info size={13} /> All {result.node_count.toLocaleString()} nodes are boundary condition–tagged
          for use by the 3D FEM assembly engine.
        </div>
      </div>
    </div>
  );
}

// ─── Thermal Tab ─────────────────────────────────────────────────────────────
function ThermalTab({ result }: { result: HeatSinkThermalResult | null }) {
  if (!result) return <EmptyState label="Click 'Run Thermal Solve' to execute the 3D FEM simulation" />;

  const nTemp = result.nodal_temperatures;
  const histBins = 20;
  const tMin = result.min_temperature_c;
  const tMax = result.max_temperature_c;
  const step = (tMax - tMin) / histBins;
  const bins = Array.from({ length: histBins }, (_, i) => tMin + i * step);
  const counts = bins.map((b, i) =>
    nTemp.filter(t => t >= b && (i === histBins - 1 ? t <= b + step : t < b + step)).length
  );

  return (
    <div>
      <div className="grid-4" style={{ gap: 12, marginBottom: 16 }}>
        {[
          { label: 'Min Temperature', value: `${result.min_temperature_c.toFixed(2)} °C`, accent: '#36B8C5' },
          { label: 'Max Temperature', value: `${result.max_temperature_c.toFixed(2)} °C`, accent: '#F4A261' },
          { label: 'Avg Temperature', value: `${result.avg_temperature_c.toFixed(2)} °C`, accent: '#B7F34A' },
          { label: 'Thermal Resistance', value: `${result.thermal_resistance_k_w.toFixed(3)} K/W`, accent: '#202820' },
        ].map(({ label, value, accent }) => (
          <div key={label} className="card" style={{ padding: '14px 16px' }}>
            <div style={{ fontSize: 11, color: '#758078', marginBottom: 4 }}>{label}</div>
            <div style={{ fontSize: 22, fontWeight: 700, color: accent }}>{value}</div>
          </div>
        ))}
      </div>

      <div className="grid-2" style={{ gap: 16, marginBottom: 16 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Solver Performance</div>
          <table className="data-table">
            <tbody>
              <tr><td>Solver</td><td style={{ fontWeight: 600 }}>{result.solver_name}</td></tr>
              <tr><td>Status</td><td style={{ color: result.success ? '#4ADE80' : '#F87171' }}>
                {result.success ? '✓ Converged' : result.status_message}
              </td></tr>
              <tr><td>Iterations</td><td>{result.iterations}</td></tr>
              <tr><td>Residual Norm ‖r‖/‖b‖</td><td>{result.residual_norm.toExponential(3)}</td></tr>
              <tr><td>Assembly Time</td><td>{result.assembly_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Factorization Time</td><td>{result.factorization_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Solve Time</td><td>{result.solve_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Total Time</td><td>{result.total_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Memory (est.)</td><td>{result.memory_kb} KB</td></tr>
            </tbody>
          </table>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Energy Balance</div>
          <table className="data-table" style={{ marginBottom: 12 }}>
            <tbody>
              <tr><td>Heat In (source)</td><td>{result.total_heat_in_w.toFixed(3)} W</td></tr>
              <tr><td>Heat Out (convection)</td><td>{result.total_heat_out_w.toFixed(3)} W</td></tr>
              <tr><td>Balance Error</td><td>
                <span style={{ color: result.energy_balance_error_percent < 5 ? '#4ADE80' : '#F87171' }}>
                  {result.energy_balance_error_percent.toFixed(2)}%
                </span>
              </td></tr>
            </tbody>
          </table>
          {result.energy_balance_error_percent < 5 && (
            <div className="alert alert-success">
              <CheckCircle size={13} /> Energy conservation satisfied (&lt;5% error)
            </div>
          )}
        </div>
      </div>

      {nTemp.length > 0 && (
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Nodal Temperature Distribution (Histogram)</div>
          <Plot
            data={[{
              type: 'bar',
              x: bins.map(b => b.toFixed(1)),
              y: counts,
              marker: { color: '#36B8C5' },
              name: 'Node Count'
            }]}
            layout={{
              height: 240,
              margin: { l: 50, r: 20, t: 10, b: 50 },
              paper_bgcolor: '#FFFFFF',
              plot_bgcolor: '#F7F8F5',
              xaxis: { title: 'Temperature (°C)', tickfont: { size: 10 } },
              yaxis: { title: 'Node Count', tickfont: { size: 10 } },
              showlegend: false,
            }}
            config={{ displayModeBar: false }}
            style={{ width: '100%' }}
          />
        </div>
      )}
    </div>
  );
}

// ─── Benchmark Tab ───────────────────────────────────────────────────────────
function BenchmarkTab({ result }: { result: HeatSinkBenchmarkResult | null }) {
  if (!result) return <EmptyState label="Click 'Run Solver Benchmark' to compare all three solvers" />;

  const solvers = result.solvers;

  const timingPlot = [{
    type: 'bar' as const,
    name: 'Median Total Time (ms)',
    x: solvers.map(s => s.solver_name.split(' ')[0]),
    y: solvers.map(s => +s.median_total_time_ms.toFixed(3)),
    marker: { color: SOLVER_COLORS },
  }];

  const residualPlot = [{
    type: 'bar' as const,
    name: 'Residual Norm',
    x: solvers.map(s => s.solver_name.split(' ')[0]),
    y: solvers.map(s => s.residual_norm),
    marker: { color: SOLVER_COLORS },
  }];

  return (
    <div>
      <div className="grid-4" style={{ gap: 12, marginBottom: 16 }}>
        {[
          { label: 'DOFs (Nodes)', value: result.dof_count.toLocaleString() },
          { label: 'Non-zeros', value: result.non_zeros.toLocaleString() },
          { label: 'Max Temperature', value: `${result.max_temperature_c.toFixed(2)} °C` },
          { label: 'Energy Balance Error', value: `${result.energy_balance_error_percent.toFixed(2)}%` },
        ].map(({ label, value }) => (
          <div key={label} className="card" style={{ padding: '14px 16px' }}>
            <div style={{ fontSize: 11, color: '#758078', marginBottom: 4 }}>{label}</div>
            <div style={{ fontSize: 20, fontWeight: 700, color: '#202820' }}>{value}</div>
          </div>
        ))}
      </div>

      <div className="card" style={{ marginBottom: 16 }}>
        <div className="card-title"><span className="accent-dot" />Solver Comparison Table</div>
        <table className="data-table">
          <thead>
            <tr>
              <th>Solver</th>
              <th>Status</th>
              <th>Iterations</th>
              <th>Residual ‖r‖/‖b‖</th>
              <th>Median Time (ms)</th>
              <th>Factorization (ms)</th>
              <th>Speedup vs Cholesky</th>
              <th>Max ΔT vs Ref (°C)</th>
              <th>Memory (KB)</th>
            </tr>
          </thead>
          <tbody>
            {solvers.map((s, i) => (
              <tr key={i}>
                <td style={{ fontWeight: 600, color: SOLVER_COLORS[i] }}>{s.solver_name.split(' (')[0]}</td>
                <td><span style={{ color: s.success ? '#4ADE80' : '#F87171' }}>
                  {s.success ? '✓' : '✗'} {s.status_message}
                </span></td>
                <td>{s.iterations}</td>
                <td>{s.residual_norm.toExponential(3)}</td>
                <td><strong>{s.median_total_time_ms.toFixed(2)}</strong></td>
                <td>{s.median_factorization_time_ms.toFixed(2)}</td>
                <td>
                  <span style={{
                    padding: '2px 8px', borderRadius: 12,
                    background: s.speedup_vs_reference > 1 ? '#B7F34A33' : '#F4A26133',
                    color: s.speedup_vs_reference > 1 ? '#4a7a00' : '#c05000',
                    fontWeight: 600
                  }}>
                    {s.speedup_vs_reference.toFixed(2)}×
                  </span>
                </td>
                <td>{s.max_temp_diff_vs_reference_c.toFixed(4)}</td>
                <td>{s.memory_kb}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="grid-2" style={{ gap: 16 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Solve Time Comparison</div>
          <Plot data={timingPlot}
            layout={{
              height: 220, margin: { l: 50, r: 10, t: 10, b: 50 },
              paper_bgcolor: '#FFFFFF', plot_bgcolor: '#F7F8F5',
              yaxis: { title: 'Time (ms)', tickfont: { size: 10 } },
              xaxis: { tickfont: { size: 11 } },
              showlegend: false,
            }}
            config={{ displayModeBar: false }} style={{ width: '100%' }} />
        </div>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Residual Norm Comparison</div>
          <Plot data={residualPlot}
            layout={{
              height: 220, margin: { l: 60, r: 10, t: 10, b: 50 },
              paper_bgcolor: '#FFFFFF', plot_bgcolor: '#F7F8F5',
              yaxis: { title: '‖r‖/‖b‖', type: 'log', tickfont: { size: 10 } },
              xaxis: { tickfont: { size: 11 } },
              showlegend: false,
            }}
            config={{ displayModeBar: false }} style={{ width: '100%' }} />
        </div>
      </div>

      {/* Trial Times Box Plots */}
      {solvers[0]?.trial_times_ms?.length > 0 && (
        <div className="card" style={{ marginTop: 16 }}>
          <div className="card-title"><span className="accent-dot" />Runtime Distribution per Solver ({result.solvers[0]?.trial_times_ms?.length ?? 0} trials)</div>
          <Plot
            data={solvers.map((s, i) => ({
              type: 'box' as const,
              name: s.solver_name.split(' ')[0],
              y: s.trial_times_ms,
              marker: { color: SOLVER_COLORS[i] },
            }))}
            layout={{
              height: 250, margin: { l: 50, r: 20, t: 10, b: 50 },
              paper_bgcolor: '#FFFFFF', plot_bgcolor: '#F7F8F5',
              yaxis: { title: 'Time (ms)', tickfont: { size: 10 } },
              showlegend: false,
            }}
            config={{ displayModeBar: false }} style={{ width: '100%' }} />
        </div>
      )}
    </div>
  );
}

// ─── Convergence Tab ──────────────────────────────────────────────────────────
function ConvergenceTab({ result }: { result: HeatSinkConvergenceResult | null }) {
  if (!result) return <EmptyState label="Click 'Mesh Convergence Study' to run Coarse → Medium → Fine analysis" />;

  const pts = result.points;

  return (
    <div>
      <div className="grid-2" style={{ gap: 16, marginBottom: 16 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Convergence Assessment</div>
          <div className="alert" style={{
            background: result.is_converged ? '#B7F34A22' : '#F4A26122',
            border: `1px solid ${result.is_converged ? '#B7F34A' : '#F4A261'}66`,
            padding: 12, borderRadius: 8, marginBottom: 12
          }}>
            <strong>{result.is_converged ? '✓ Converged' : '⚠ Not fully converged'}</strong>
            {result.asymptotic_estimate_c > 0 && (
              <div style={{ marginTop: 4, fontSize: 12 }}>
                Asymptotic T_max estimate: <strong>{result.asymptotic_estimate_c.toFixed(2)} °C</strong>
              </div>
            )}
          </div>
          <p style={{ fontSize: 13, color: '#758078' }}>{result.recommendation}</p>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Mesh Level Comparison</div>
          <table className="data-table">
            <thead>
              <tr><th>Mesh</th><th>Nodes</th><th>Tets</th><th>T_max (°C)</th><th>Δ (%)</th></tr>
            </thead>
            <tbody>
              {pts.map(pt => (
                <tr key={pt.resolution_name}>
                  <td style={{ fontWeight: 600 }}>{pt.resolution_name}</td>
                  <td>{pt.node_count.toLocaleString()}</td>
                  <td>{pt.element_count.toLocaleString()}</td>
                  <td>{pt.max_temperature_c.toFixed(2)}</td>
                  <td><span style={{ color: pt.relative_change_percent < 2 ? '#4ADE80' : '#F87171' }}>
                    {pt.relative_change_percent.toFixed(2)}%
                  </span></td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      <div className="grid-2" style={{ gap: 16 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Max Temperature vs Mesh Level</div>
          <Plot
            data={[{
              type: 'scatter' as const,
              mode: 'lines+markers',
              x: pts.map(p => p.resolution_name),
              y: pts.map(p => p.max_temperature_c),
              marker: { color: '#F4A261', size: 10 },
              line: { color: '#F4A261', width: 2 },
              name: 'T_max (°C)'
            }]}
            layout={{
              height: 220, margin: { l: 55, r: 10, t: 10, b: 40 },
              paper_bgcolor: '#FFFFFF', plot_bgcolor: '#F7F8F5',
              yaxis: { title: 'T_max (°C)', tickfont: { size: 10 } },
              xaxis: { tickfont: { size: 12 } },
              showlegend: false,
            }}
            config={{ displayModeBar: false }} style={{ width: '100%' }} />
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Solver Time vs Mesh Level</div>
          <Plot
            data={[
              {
                type: 'bar' as const, name: 'Cholesky',
                x: pts.map(p => p.resolution_name), y: pts.map(p => p.cholesky_time_ms),
                marker: { color: '#B7F34A' }
              },
              {
                type: 'bar' as const, name: 'CG',
                x: pts.map(p => p.resolution_name), y: pts.map(p => p.cg_time_ms),
                marker: { color: '#36B8C5' }
              },
              {
                type: 'bar' as const, name: 'Gauss-Seidel',
                x: pts.map(p => p.resolution_name), y: pts.map(p => p.gs_time_ms),
                marker: { color: '#F4A261' }
              },
            ]}
            layout={{
              height: 220, barmode: 'group',
              margin: { l: 50, r: 10, t: 10, b: 40 },
              paper_bgcolor: '#FFFFFF', plot_bgcolor: '#F7F8F5',
              yaxis: { title: 'Time (ms)', tickfont: { size: 10 } },
              xaxis: { tickfont: { size: 12 } },
              legend: { orientation: 'h', y: -0.3, font: { size: 10 } },
            }}
            config={{ displayModeBar: false }} style={{ width: '100%' }} />
        </div>
      </div>
    </div>
  );
}

// ─── Empty State ─────────────────────────────────────────────────────────────
function EmptyState({ label }: { label: string }) {
  return (
    <div style={{
      textAlign: 'center', padding: '60px 20px',
      background: '#FFFFFF', borderRadius: 12, border: '1px solid #E5EAE2'
    }}>
      <div style={{ fontSize: 48, marginBottom: 12 }}>🔬</div>
      <p style={{ color: '#758078', fontSize: 14 }}>{label}</p>
    </div>
  );
}
