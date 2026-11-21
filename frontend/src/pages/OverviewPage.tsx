import { useNavigate } from 'react-router-dom';
import { Box, Thermometer, Zap, Cpu, Brain, ArrowRight, CheckCircle, Clock, AlertCircle, Layers } from 'lucide-react';

interface Props { connStatus: 'connecting' | 'connected' | 'error'; }

const MODULES = [
  {
    id: 'geometry', label: 'Geometry Studio', icon: Box,
    desc: 'ASCII STL mesh viewer & temperature point cloud visualization',
    path: '/geometry', color: '#36B8C5',
  },
  {
    id: 'thermal', label: 'Thermal Analysis', icon: Thermometer,
    desc: '1D FEM steady-state heat conduction with convergence study',
    path: '/thermal', color: '#F59E0B',
  },
  {
    id: 'heatsink', label: '3D Heat Sink FEM', icon: Layers,
    desc: 'Parametric asymmetric heat sink → TET4 mesh → Cholesky / CG / SSOR solver benchmark',
    path: '/heatsink', color: '#B7F34A',
    badge: 'NEW',
  },
  {
    id: 'performance', label: 'Performance Lab', icon: Zap,
    desc: 'Multi-core benchmark: serial vs parallel FEM batch solver',
    path: '/performance', color: '#B7F34A',
  },
  {
    id: 'hardware', label: 'Hardware Integration', icon: Cpu,
    desc: 'MCU, FPGA and JTAG mock adapter simulation & telemetry',
    path: '/hardware', color: '#A78BFA',
  },
  {
    id: 'neural', label: 'Neural Computation', icon: Brain,
    desc: 'Single-unit backprop with analytical & finite-difference gradients',
    path: '/neural', color: '#F87171',
  },
];

export default function OverviewPage({ connStatus }: Props) {
  const navigate = useNavigate();

  return (
    <div className="page-container">
      {/* Hero */}
      <div style={{ marginBottom: 32 }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 8 }}>
          <h1 className="page-title" style={{ margin: 0 }}>
            Unified Engineering Workbench
          </h1>
          <span className="badge badge-info">v1.0.0</span>
        </div>
        <p className="page-subtitle" style={{ maxWidth: 620 }}>
          Engineering Intelligence. Unified. — A fully integrated C++ + React workstation
          for geometry visualization, thermal FEA, parallel benchmarking,
          embedded-system integration, and neural network computation.
          Built for PASS Science Works R&D Internship 2026–2027.
        </p>

        {/* Backend status banner */}
        {connStatus === 'connected' && (
          <div className="alert alert-success" style={{ marginTop: 16, maxWidth: 480 }}>
            <CheckCircle size={15} />
            C++ backend connected — all modules ready.
          </div>
        )}
        {connStatus === 'connecting' && (
          <div className="alert alert-info" style={{ marginTop: 16, maxWidth: 480 }}>
            <div className="spinner" />
            Connecting to C++ backend server…
          </div>
        )}
        {connStatus === 'error' && (
          <div className="alert alert-error" style={{ marginTop: 16, maxWidth: 480 }}>
            <AlertCircle size={15} />
            Backend offline — start <code>uew_server.exe</code> to enable calculations.
          </div>
        )}
      </div>

      {/* Module cards */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(320px, 1fr))', gap: 16, marginBottom: 32 }}>
        {MODULES.map(({ id, label, icon: Icon, desc, path, color, badge }) => (
          <div key={id} className="card" style={{ cursor: 'pointer', transition: 'box-shadow 0.2s, border-color 0.2s' }}
               onClick={() => navigate(path)}
               onMouseEnter={e => { (e.currentTarget as HTMLElement).style.borderColor = color; (e.currentTarget as HTMLElement).style.boxShadow = `0 4px 20px rgba(0,0,0,0.08)`; }}
               onMouseLeave={e => { (e.currentTarget as HTMLElement).style.borderColor = ''; (e.currentTarget as HTMLElement).style.boxShadow = ''; }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
              <div style={{ background: color + '20', borderRadius: 8, padding: 10, marginBottom: 12 }}>
                <Icon size={20} style={{ color }} />
              </div>
              <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
                {badge && (
                  <span style={{
                    background: '#B7F34A',
                    color: '#202820',
                    fontSize: 10,
                    fontWeight: 700,
                    padding: '2px 8px',
                    borderRadius: 10,
                    letterSpacing: '0.05em'
                  }}>
                    {badge}
                  </span>
                )}
                <ArrowRight size={14} style={{ color: 'var(--text-secondary)', marginTop: 4 }} />
              </div>
            </div>
            <div style={{ fontWeight: 700, fontSize: 14, marginBottom: 6 }}>{label}</div>
            <div style={{ fontSize: 12, color: 'var(--text-secondary)', lineHeight: 1.5 }}>{desc}</div>
          </div>
        ))}
      </div>

      {/* Info grid */}
      <div className="grid-2" style={{ marginBottom: 24 }}>
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Technology Stack</div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
            {[
              ['Backend',     'C++17/20 · CMake · cpp-httplib · nlohmann/json · Eigen 3.4.0'],
              ['Frontend',    'React 19 · TypeScript · Vite · Tailwind CSS'],
              ['Visualization','Three.js · Plotly.js · Lucide React'],
              ['Solvers',     'SimplicialLDLT · Conjugate Gradient · SSOR Gauss-Seidel'],
              ['Testing',     'GoogleTest (C++ 65 tests)'],
              ['Platform',    'Windows 11 x64'],
            ].map(([k, v]) => (
              <div key={k} style={{ display: 'flex', gap: 8, fontSize: 12 }}>
                <span style={{ color: 'var(--text-secondary)', minWidth: 100, fontWeight: 600 }}>{k}</span>
                <span style={{ color: 'var(--text-main)' }}>{v}</span>
              </div>
            ))}
          </div>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Assignment Compliance</div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
            {[
              ['A — Geometry Studio',       true],
              ['B — Thermal FEA',           true],
              ['C — Performance Lab',       true],
              ['D — Hardware Integration',  true],
              ['E — Neural Computation',    true],
              ['Automated Tests',           true],
              ['Windows Launcher',          true],
            ].map(([label, done]) => (
              <div key={label as string} style={{ display: 'flex', alignItems: 'center', gap: 8, fontSize: 12 }}>
                {done
                  ? <CheckCircle size={13} style={{ color: '#4ADE80', flexShrink: 0 }} />
                  : <Clock size={13} style={{ color: '#F59E0B', flexShrink: 0 }} />}
                <span>{label}</span>
              </div>
            ))}
          </div>
        </div>
      </div>

      <div className="card">
        <div className="card-title"><span className="accent-dot" />Quick Start</div>
        <p style={{ fontSize: 12, color: 'var(--text-secondary)', margin: '0 0 12px', lineHeight: 1.6 }}>
          Each module connects to the local C++ HTTP server. Navigate using the sidebar.
          All computations run in C++17; the React frontend only handles rendering.
        </p>
        <div style={{ display: 'flex', gap: 8, flexWrap: 'wrap' }}>
          {MODULES.map(m => (
            <button key={m.id} className="btn btn-secondary btn-sm" onClick={() => navigate(m.path)}>
              <m.icon size={13} /> {m.label}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
