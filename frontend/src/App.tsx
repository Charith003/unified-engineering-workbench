import { useState, useEffect } from 'react';
import { BrowserRouter, Routes, Route, NavLink, useLocation } from 'react-router-dom';
import {
  LayoutDashboard, Box, Thermometer, Zap, Cpu, Brain,
  FileText, BookOpen, Settings, Bell, Activity, Layers
} from 'lucide-react';
import { getHealth } from './services/api';

import OverviewPage        from './pages/OverviewPage';
import GeometryPage        from './pages/GeometryPage';
import ThermalPage         from './pages/ThermalPage';
import PerformancePage     from './pages/PerformancePage';
import HardwarePage        from './pages/HardwarePage';
import NeuralPage          from './pages/NeuralPage';
import ReportsPage         from './pages/ReportsPage';
import DocumentationPage   from './pages/DocumentationPage';
import HeatSinkPage        from './pages/HeatSinkPage';

import './index.css';

const NAV_ITEMS = [
  { to: '/',           label: 'Overview',            icon: LayoutDashboard },
  { to: '/geometry',   label: 'Geometry Studio',     icon: Box             },
  { to: '/thermal',    label: 'Thermal Analysis',    icon: Thermometer     },
  { to: '/heatsink',   label: '3D Heat Sink FEM',    icon: Layers          },
  { to: '/performance',label: 'Performance Lab',     icon: Zap             },
  { to: '/hardware',   label: 'Hardware Integration',icon: Cpu             },
  { to: '/neural',     label: 'Neural Computation',  icon: Brain           },
  { to: '/reports',    label: 'Reports & Exports',   icon: FileText        },
  { to: '/docs',       label: 'Documentation',       icon: BookOpen        },
];

const MODULE_NAMES: Record<string, string> = {
  '/':            'Overview',
  '/geometry':    'Geometry Studio',
  '/thermal':     'Thermal Analysis',
  '/heatsink':    '3D Heat Sink FEM',
  '/performance': 'Performance Lab',
  '/hardware':    'Hardware Integration',
  '/neural':      'Neural Computation',
  '/reports':     'Reports & Exports',
  '/docs':        'Documentation',
};

type ConnStatus = 'connecting' | 'connected' | 'error';

function AppShell() {
  const location = useLocation();
  const [connStatus, setConnStatus] = useState<ConnStatus>('connecting');
  const [backendVersion, setBackendVersion] = useState<string>('');

  useEffect(() => {
    let alive = true;
    const check = async () => {
      try {
        const r = await getHealth();
        if (!alive) return;
        if (r.success) {
          setConnStatus('connected');
          setBackendVersion((r.data as { version?: string })?.version ?? '');
        } else {
          setConnStatus('error');
        }
      } catch {
        if (alive) setConnStatus('error');
      }
    };
    check();
    const id = setInterval(check, 5000);
    return () => { alive = false; clearInterval(id); };
  }, []);

  const currentModule = Object.entries(MODULE_NAMES).reduce((best, [path]) => {
    if (location.pathname.startsWith(path) && path.length > best.length) {
      return path;
    }
    return best;
  }, '/');

  return (
    <div className="app-shell">
      {/* ── Header ── */}
      <header className="app-header">
        <a className="logo-mark" href="/">
          <span className="logo-badge">UEW</span>
          <span className="logo-name">Unified Engineering Workbench</span>
        </a>
        <div className="header-divider" />
        <span className="header-module">{MODULE_NAMES[currentModule]}</span>
        <div className="header-spacer" />

        <div className="status-chip">
          <span className={`status-dot ${connStatus}`} />
          {connStatus === 'connected'
            ? `Backend v${backendVersion}`
            : connStatus === 'connecting'
            ? 'Connecting…'
            : 'Backend offline'}
        </div>

        <button className="btn btn-secondary btn-sm" title="Notifications">
          <Bell size={14} />
        </button>
        <button className="btn btn-secondary btn-sm" title="Settings">
          <Settings size={14} />
        </button>
      </header>

      {/* ── Sidebar ── */}
      <aside className="app-sidebar">
        <div className="nav-section-label">Workbench</div>
        {NAV_ITEMS.map(({ to, label, icon: Icon }) => (
          <NavLink
            key={to}
            to={to}
            end={to === '/'}
            className={({ isActive }) => `nav-item${isActive ? ' active' : ''}`}
          >
            <Icon size={15} className="nav-icon" />
            {label}
          </NavLink>
        ))}

        <div style={{ marginTop: 'auto', paddingTop: 16 }} />
        <div className="nav-section-label">System</div>
        <div className="nav-item" style={{ cursor: 'default' }}>
          <Activity size={15} className="nav-icon" />
          <span style={{ fontSize: 12 }}>
            {connStatus === 'connected'
              ? <span style={{ color: '#4ADE80' }}>● Online</span>
              : <span style={{ color: '#F87171' }}>● Offline</span>}
          </span>
        </div>
      </aside>

      {/* ── Content ── */}
      <main className="app-content">
        <Routes>
          <Route path="/"            element={<OverviewPage      connStatus={connStatus} />} />
          <Route path="/geometry"    element={<GeometryPage />}   />
          <Route path="/thermal"     element={<ThermalPage />}    />
          <Route path="/heatsink"    element={<HeatSinkPage />}   />
          <Route path="/performance" element={<PerformancePage />}/>
          <Route path="/hardware"    element={<HardwarePage />}   />
          <Route path="/neural"      element={<NeuralPage />}     />
          <Route path="/reports"     element={<ReportsPage />}    />
          <Route path="/docs"        element={<DocumentationPage />} />
        </Routes>
      </main>
    </div>
  );
}

export default function App() {
  return (
    <BrowserRouter>
      <AppShell />
    </BrowserRouter>
  );
}
