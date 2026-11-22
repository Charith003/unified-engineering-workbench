import { useState, useRef, useEffect } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { Layers, Play, Download, AlertCircle, CheckCircle, Info, Eye, EyeOff, RotateCcw, Box, Scissors } from 'lucide-react';
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
  HeatSinkConvergenceResult, HeatSinkMeshNode, HeatSinkMeshFace, HeatSinkMeshElement
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
      {activeTab === 'thermal' && <ThermalTab result={thermalResult} meshResult={meshResult} />}
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

// ─── 3D FEA Heat Sink & Temperature Viewer ──────────────────────────────────
interface ViewerProps {
  nodes?: HeatSinkMeshNode[];
  boundaryFaces?: HeatSinkMeshFace[];
  elements?: HeatSinkMeshElement[];
  nodalTemperatures?: number[];
  minTemp?: number;
  maxTemp?: number;
  mode: 'mesh' | 'thermal';
}

function turboColor(t: number): THREE.Color {
  const v = Math.max(0, Math.min(1, t));
  let r = 0, g = 0, b = 0;
  if (v <= 0.25) {
    const f = v / 0.25;
    r = 0; g = 0.8 * f; b = 0.7 + 0.3 * f;
  } else if (v <= 0.5) {
    const f = (v - 0.25) / 0.25;
    r = 0; g = 0.8 + 0.1 * f; b = 1.0 - 0.8 * f;
  } else if (v <= 0.75) {
    const f = (v - 0.5) / 0.25;
    r = f; g = 0.9 - 0.05 * f; b = 0.2 * (1.0 - f);
  } else {
    const f = (v - 0.75) / 0.25;
    r = 1.0 - 0.1 * f; g = 0.85 * (1.0 - f); b = 0;
  }
  return new THREE.Color(r, g, b);
}

function boundaryColor(bt: string): THREE.Color {
  switch (bt) {
    case 'HeatSource': return new THREE.Color(0xF97316); // Bright Orange
    case 'Convection': return new THREE.Color(0x36B8C5); // Teal / Cyan
    case 'FixedTemp':  return new THREE.Color(0xB7F34A); // Lime Accent
    case 'Adiabatic':  return new THREE.Color(0x94A3B8); // Slate
    default:           return new THREE.Color(0x64748B);
  }
}

function HeatSink3DViewer({
  nodes,
  boundaryFaces,
  elements,
  nodalTemperatures,
  minTemp = 25,
  maxTemp = 75,
  mode
}: ViewerProps) {
  const mountRef = useRef<HTMLDivElement>(null);
  const [wireframeVisible, setWireframeVisible] = useState(mode === 'mesh');
  const [surfaceVisible, setSurfaceVisible] = useState(true);
  const [internalTetsVisible, setInternalTetsVisible] = useState(false);
  const [sectionCutVisible, setSectionCutVisible] = useState(false);
  const [hoverInfo, setHoverInfo] = useState<{
    id: number;
    x: number;
    y: number;
    z: number;
    temp?: number;
    boundaryType?: string;
  } | null>(null);

  const controlsRef = useRef<OrbitControls | null>(null);
  const cameraRef = useRef<THREE.PerspectiveCamera | null>(null);
  const surfaceMeshRef = useRef<THREE.Mesh | null>(null);
  const wireMeshRef = useRef<THREE.LineSegments | null>(null);
  const internalMeshRef = useRef<THREE.LineSegments | null>(null);
  const materialsRef = useRef<THREE.Material[]>([]);
  const clipPlaneRef = useRef<THREE.Plane>(new THREE.Plane(new THREE.Vector3(0, 0, 1), 0));
  const resetCameraPos = useRef<THREE.Vector3>(new THREE.Vector3());

  // Update visibility on state changes
  useEffect(() => {
    if (wireMeshRef.current) wireMeshRef.current.visible = wireframeVisible;
  }, [wireframeVisible]);

  useEffect(() => {
    if (surfaceMeshRef.current) surfaceMeshRef.current.visible = surfaceVisible;
  }, [surfaceVisible]);

  useEffect(() => {
    if (internalMeshRef.current) internalMeshRef.current.visible = internalTetsVisible;
  }, [internalTetsVisible]);

  useEffect(() => {
    const planes = sectionCutVisible ? [clipPlaneRef.current] : [];
    for (const m of materialsRef.current) {
      m.clippingPlanes = planes;
      m.needsUpdate = true;
    }
  }, [sectionCutVisible]);

  useEffect(() => {
    if (!mountRef.current || !nodes || nodes.length === 0 || !boundaryFaces || boundaryFaces.length === 0) {
      return;
    }
    const container = mountRef.current;
    const width = container.clientWidth || 600;
    const height = container.clientHeight || 380;

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setClearColor(0x0e1410, 1.0);
    renderer.localClippingEnabled = true;
    container.appendChild(renderer.domElement);

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 1000);
    cameraRef.current = camera;

    // Lighting
    scene.add(new THREE.AmbientLight(0xffffff, 0.75));
    const dirLight1 = new THREE.DirectionalLight(0xffffff, 0.85);
    dirLight1.position.set(20, 40, 30);
    scene.add(dirLight1);
    const dirLight2 = new THREE.DirectionalLight(0xffffff, 0.4);
    dirLight2.position.set(-20, -30, -20);
    scene.add(dirLight2);

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.08;
    controlsRef.current = controls;

    // Build geometry from actual backend nodes and boundary faces
    const nodeMap = new Map<number, HeatSinkMeshNode>();
    let minX = Infinity, maxX = -Infinity;
    let minY = Infinity, maxY = -Infinity;
    let minZ = Infinity, maxZ = -Infinity;

    for (const nd of nodes) {
      nodeMap.set(nd.id, nd);
      if (nd.x < minX) minX = nd.x; if (nd.x > maxX) maxX = nd.x;
      if (nd.y < minY) minY = nd.y; if (nd.y > maxY) maxY = nd.y;
      if (nd.z < minZ) minZ = nd.z; if (nd.z > maxZ) maxZ = nd.z;
    }

    const cx = (minX + maxX) / 2;
    const cy = (minY + maxY) / 2;
    const cz = (minZ + maxZ) / 2;
    const maxDim = Math.max(maxX - minX, maxY - minY, maxZ - minZ) || 0.1;
    const scale = 5.0 / maxDim;

    const positions: number[] = [];
    const colors: number[] = [];
    const tSpan = Math.max(1e-4, maxTemp - minTemp);

    for (const face of boundaryFaces) {
      const n0 = nodeMap.get(face.nodes[0]);
      const n1 = nodeMap.get(face.nodes[1]);
      const n2 = nodeMap.get(face.nodes[2]);
      if (!n0 || !n1 || !n2) continue;

      // Coordinate mapping: Z is height (up in Three.js = Y), X is right (X), Y is depth (-Z)
      positions.push((n0.x - cx) * scale, (n0.z - cz) * scale, -(n0.y - cy) * scale);
      positions.push((n1.x - cx) * scale, (n1.z - cz) * scale, -(n1.y - cy) * scale);
      positions.push((n2.x - cx) * scale, (n2.z - cz) * scale, -(n2.y - cy) * scale);

      if (mode === 'thermal' && nodalTemperatures && nodalTemperatures.length > 0) {
        const t0 = nodalTemperatures[n0.id] ?? minTemp;
        const t1 = nodalTemperatures[n1.id] ?? minTemp;
        const t2 = nodalTemperatures[n2.id] ?? minTemp;
        const c0 = turboColor((t0 - minTemp) / tSpan);
        const c1 = turboColor((t1 - minTemp) / tSpan);
        const c2 = turboColor((t2 - minTemp) / tSpan);
        colors.push(c0.r, c0.g, c0.b, c1.r, c1.g, c1.b, c2.r, c2.g, c2.b);
      } else {
        const c = boundaryColor(face.boundary_type);
        colors.push(c.r, c.g, c.b, c.r, c.g, c.b, c.r, c.g, c.b);
      }
    }

    const geom = new THREE.BufferGeometry();
    geom.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    geom.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
    geom.computeVertexNormals();

    const activeClipPlanes = sectionCutVisible ? [clipPlaneRef.current] : [];

    const mat = new THREE.MeshStandardMaterial({
      vertexColors: true,
      side: THREE.DoubleSide,
      roughness: 0.35,
      metalness: 0.15,
      clippingPlanes: activeClipPlanes,
    });
    const surfaceMesh = new THREE.Mesh(geom, mat);
    surfaceMesh.visible = surfaceVisible;
    scene.add(surfaceMesh);
    surfaceMeshRef.current = surfaceMesh;

    // Crisp wireframe overlay
    const wireGeo = new THREE.WireframeGeometry(geom);
    const wireMat = new THREE.LineBasicMaterial({
      color: 0xB7F34A,
      opacity: 0.25,
      transparent: true,
      clippingPlanes: activeClipPlanes,
    });
    const wireMesh = new THREE.LineSegments(wireGeo, wireMat);
    wireMesh.visible = wireframeVisible;
    scene.add(wireMesh);
    wireMeshRef.current = wireMesh;

    // Internal Tetrahedral Mesh (actual backend element connectivity)
    let internalGeom: THREE.BufferGeometry | null = null;
    let internalMat: THREE.LineBasicMaterial | null = null;
    if (elements && elements.length > 0) {
      const edgeSet = new Set<string>();
      const internalPositions: number[] = [];

      for (const el of elements) {
        const [e0, e1, e2, e3] = el.nodes;
        const tetEdges: [number, number][] = [
          [e0, e1], [e0, e2], [e0, e3],
          [e1, e2], [e1, e3], [e2, e3]
        ];
        for (const [a, b] of tetEdges) {
          const minN = Math.min(a, b);
          const maxN = Math.max(a, b);
          const key = `${minN}_${maxN}`;
          if (!edgeSet.has(key)) {
            edgeSet.add(key);
            const na = nodeMap.get(a);
            const nb = nodeMap.get(b);
            if (na && nb) {
              internalPositions.push((na.x - cx) * scale, (na.z - cz) * scale, -(na.y - cy) * scale);
              internalPositions.push((nb.x - cx) * scale, (nb.z - cz) * scale, -(nb.y - cy) * scale);
            }
          }
        }
      }

      if (internalPositions.length > 0) {
        internalGeom = new THREE.BufferGeometry();
        internalGeom.setAttribute('position', new THREE.Float32BufferAttribute(internalPositions, 3));
        internalMat = new THREE.LineBasicMaterial({
          color: 0xF59E0B, // Amber
          opacity: 0.35,
          transparent: true,
          clippingPlanes: activeClipPlanes,
        });
        const internalLineMesh = new THREE.LineSegments(internalGeom, internalMat);
        internalLineMesh.visible = internalTetsVisible;
        scene.add(internalLineMesh);
        internalMeshRef.current = internalLineMesh;
      }
    }

    materialsRef.current = [mat, wireMat, ...(internalMat ? [internalMat] : [])];

    // Ground orientation grid
    const grid = new THREE.GridHelper(8, 16, 0x223522, 0x142014);
    grid.position.y = (minZ - cz) * scale;
    scene.add(grid);

    // Initial camera position (isometric viewpoint)
    camera.position.set(6.5, 5.0, 7.5);
    camera.lookAt(0, 0, 0);
    resetCameraPos.current.copy(camera.position);
    controls.update();

    // Raycasting for interactive hover inspection
    const raycaster = new THREE.Raycaster();
    const mouse = new THREE.Vector2();

    const onPointerMove = (evt: PointerEvent) => {
      const rect = renderer.domElement.getBoundingClientRect();
      mouse.x = ((evt.clientX - rect.left) / rect.width) * 2 - 1;
      mouse.y = -((evt.clientY - rect.top) / rect.height) * 2 + 1;

      raycaster.setFromCamera(mouse, camera);
      if (surfaceMeshRef.current && surfaceMeshRef.current.visible) {
        const hits = raycaster.intersectObject(surfaceMeshRef.current);
        if (hits.length > 0) {
          const hit = hits[0];
          const faceIdx = hit.faceIndex;
          if (faceIdx !== undefined && faceIdx !== null && faceIdx < boundaryFaces.length) {
            const face = boundaryFaces[faceIdx as number];
            let closestId = face.nodes[0];
            let minDist = Infinity;
            for (const nid of face.nodes) {
              const nd = nodeMap.get(nid);
              if (nd) {
                const px = (nd.x - cx) * scale;
                const py = (nd.z - cz) * scale;
                const pz = -(nd.y - cy) * scale;
                const dx = px - hit.point.x;
                const dy = py - hit.point.y;
                const dz = pz - hit.point.z;
                const dist = dx * dx + dy * dy + dz * dz;
                if (dist < minDist) {
                  minDist = dist;
                  closestId = nid;
                }
              }
            }
            const closest = nodeMap.get(closestId);
            if (closest) {
              setHoverInfo({
                id: closestId,
                x: closest.x * 1000,
                y: closest.y * 1000,
                z: closest.z * 1000,
                temp: nodalTemperatures && nodalTemperatures.length > closestId ? nodalTemperatures[closestId] : undefined,
                boundaryType: face.boundary_type
              });
              return;
            }
          }
        }
      }
      setHoverInfo(null);
    };

    const onPointerLeave = () => setHoverInfo(null);

    renderer.domElement.addEventListener('pointermove', onPointerMove);
    renderer.domElement.addEventListener('pointerleave', onPointerLeave);

    let animId: number;
    const animate = () => {
      animId = requestAnimationFrame(animate);
      controls.update();
      renderer.render(scene, camera);
    };
    animate();

    const handleResize = () => {
      const w = container.clientWidth;
      const h = container.clientHeight;
      if (w > 0 && h > 0) {
        camera.aspect = w / h;
        camera.updateProjectionMatrix();
        renderer.setSize(w, h);
      }
    };
    window.addEventListener('resize', handleResize);

    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener('resize', handleResize);
      renderer.domElement.removeEventListener('pointermove', onPointerMove);
      renderer.domElement.removeEventListener('pointerleave', onPointerLeave);
      if (container.contains(renderer.domElement)) {
        container.removeChild(renderer.domElement);
      }
      geom.dispose();
      mat.dispose();
      wireGeo.dispose();
      wireMat.dispose();
      if (internalGeom) internalGeom.dispose();
      if (internalMat) internalMat.dispose();
      renderer.dispose();
    };
  }, [nodes, boundaryFaces, elements, nodalTemperatures, minTemp, maxTemp, mode]);

  const handleReset = () => {
    if (cameraRef.current && controlsRef.current) {
      cameraRef.current.position.copy(resetCameraPos.current);
      controlsRef.current.target.set(0, 0, 0);
      controlsRef.current.update();
    }
  };

  return (
    <div style={{ position: 'relative', width: '100%', borderRadius: 8, overflow: 'hidden', border: '1px solid #233523', background: '#0e1410' }}>
      {/* 3D WebGL Canvas */}
      <div ref={mountRef} style={{ width: '100%', height: 380, cursor: 'grab' }} />

      {/* Top Controls Overlay */}
      <div style={{
        position: 'absolute', top: 10, left: 12, right: 12,
        display: 'flex', justifyContent: 'space-between', alignItems: 'center',
        pointerEvents: 'none'
      }}>
        <div style={{
          background: 'rgba(16, 24, 18, 0.85)', backdropFilter: 'blur(4px)',
          border: '1px solid #2a3d2a', borderRadius: 6, padding: '4px 10px',
          color: '#B7F34A', fontSize: 12, fontWeight: 600, display: 'flex', alignItems: 'center', gap: 6
        }}>
          <Box size={14} />
          {mode === 'thermal' ? '3D FEA Nodal Temperature Field' : '3D Computational Boundary Mesh'}
        </div>

        <div style={{ display: 'flex', gap: 6, pointerEvents: 'auto' }}>
          {elements && elements.length > 0 && (
            <button
              onClick={() => setInternalTetsVisible(v => !v)}
              style={{
                background: internalTetsVisible ? '#F59E0B' : 'rgba(20, 30, 22, 0.85)',
                color: internalTetsVisible ? '#101810' : '#E5EAE2',
                border: '1px solid #2a3d2a', borderRadius: 5, padding: '4px 8px',
                fontSize: 11, fontWeight: 500, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4
              }}>
              <Layers size={12} />
              Internal Tets
            </button>
          )}
          <button
            onClick={() => setSectionCutVisible(v => !v)}
            style={{
              background: sectionCutVisible ? '#EC4899' : 'rgba(20, 30, 22, 0.85)',
              color: sectionCutVisible ? '#FFFFFF' : '#E5EAE2',
              border: '1px solid #2a3d2a', borderRadius: 5, padding: '4px 8px',
              fontSize: 11, fontWeight: 500, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4
            }}>
            <Scissors size={12} />
            Section Cut
          </button>
          <button
            onClick={() => setWireframeVisible(v => !v)}
            style={{
              background: wireframeVisible ? '#B7F34A' : 'rgba(20, 30, 22, 0.85)',
              color: wireframeVisible ? '#101810' : '#E5EAE2',
              border: '1px solid #2a3d2a', borderRadius: 5, padding: '4px 8px',
              fontSize: 11, fontWeight: 500, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4
            }}>
            {wireframeVisible ? <Eye size={12} /> : <EyeOff size={12} />}
            Wireframe
          </button>
          <button
            onClick={() => setSurfaceVisible(v => !v)}
            style={{
              background: surfaceVisible ? '#36B8C5' : 'rgba(20, 30, 22, 0.85)',
              color: surfaceVisible ? '#101810' : '#E5EAE2',
              border: '1px solid #2a3d2a', borderRadius: 5, padding: '4px 8px',
              fontSize: 11, fontWeight: 500, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4
            }}>
            {surfaceVisible ? <Eye size={12} /> : <EyeOff size={12} />}
            Surface
          </button>
          <button
            onClick={handleReset}
            style={{
              background: 'rgba(20, 30, 22, 0.85)', color: '#E5EAE2',
              border: '1px solid #2a3d2a', borderRadius: 5, padding: '4px 8px',
              fontSize: 11, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 4
            }}>
            <RotateCcw size={12} /> Reset View
          </button>
        </div>
      </div>

      {/* Hover Inspection HUD Overlay */}
      <div style={{
        position: 'absolute', bottom: mode === 'thermal' ? 62 : 44, left: 12,
        background: 'rgba(14, 20, 16, 0.90)', backdropFilter: 'blur(4px)',
        border: '1px solid #2a3d2a', borderRadius: 6, padding: '5px 10px',
        fontSize: 11, color: '#C8D4C8', pointerEvents: 'none'
      }}>
        {hoverInfo ? (
          <div style={{ display: 'flex', gap: 12, alignItems: 'center' }}>
            <span>Node <strong style={{ color: '#B7F34A' }}>#{hoverInfo.id}</strong></span>
            <span>Coord: <strong>({hoverInfo.x.toFixed(1)}, {hoverInfo.y.toFixed(1)}, {hoverInfo.z.toFixed(1)}) mm</strong></span>
            {hoverInfo.temp !== undefined && (
              <span>Temp: <strong style={{ color: '#F97316' }}>{hoverInfo.temp.toFixed(2)} °C</strong></span>
            )}
            {hoverInfo.boundaryType && (
              <span>Boundary: <strong>{hoverInfo.boundaryType}</strong></span>
            )}
          </div>
        ) : (
          <span style={{ color: '#758078' }}>Hover cursor over surface to inspect nodal coordinates & temperature</span>
        )}
      </div>

      {/* Legend Bar Overlay */}
      <div style={{
        position: 'absolute', bottom: 10, left: 12, right: 12,
        background: 'rgba(14, 20, 16, 0.92)', backdropFilter: 'blur(4px)',
        border: '1px solid #2a3d2a', borderRadius: 6, padding: '6px 12px',
        display: 'flex', flexDirection: 'column', gap: 4
      }}>
        {mode === 'thermal' ? (
          <div>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 10, color: '#A0B0A0', marginBottom: 2 }}>
              <span style={{ color: '#60A5FA', fontWeight: 600 }}>T_min: {minTemp.toFixed(1)} °C</span>
              <span>{(minTemp + (maxTemp - minTemp) * 0.25).toFixed(1)} °C</span>
              <span>{(minTemp + (maxTemp - minTemp) * 0.50).toFixed(1)} °C</span>
              <span>{(minTemp + (maxTemp - minTemp) * 0.75).toFixed(1)} °C</span>
              <span style={{ color: '#F87171', fontWeight: 600 }}>T_max: {maxTemp.toFixed(1)} °C</span>
            </div>
            <div style={{
              height: 10, borderRadius: 3,
              background: 'linear-gradient(to right, #0000ff 0%, #00ffff 25%, #00ff00 50%, #ffff00 75%, #ff0000 100%)',
              border: '1px solid #3a4d3a'
            }} />
          </div>
        ) : (
          <div style={{ display: 'flex', gap: 16, flexWrap: 'wrap', alignItems: 'center', fontSize: 11 }}>
            <span style={{ color: '#758078', fontWeight: 600 }}>Boundary Conditions:</span>
            <div style={{ display: 'flex', alignItems: 'center', gap: 5 }}>
              <span style={{ width: 10, height: 10, borderRadius: 2, background: '#F97316' }} />
              <span style={{ color: '#E5EAE2' }}>Heat Source (Base Top)</span>
            </div>
            <div style={{ display: 'flex', alignItems: 'center', gap: 5 }}>
              <span style={{ width: 10, height: 10, borderRadius: 2, background: '#36B8C5' }} />
              <span style={{ color: '#E5EAE2' }}>Convection (Fins & Base)</span>
            </div>
            <div style={{ display: 'flex', alignItems: 'center', gap: 5 }}>
              <span style={{ width: 10, height: 10, borderRadius: 2, background: '#B7F34A' }} />
              <span style={{ color: '#E5EAE2' }}>Fixed Temp</span>
            </div>
            <div style={{ display: 'flex', alignItems: 'center', gap: 5 }}>
              <span style={{ width: 10, height: 10, borderRadius: 2, background: '#94A3B8' }} />
              <span style={{ color: '#E5EAE2' }}>Adiabatic</span>
            </div>
          </div>
        )}
      </div>
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
      {/* 3D Tetrahedral Mesh Viewer */}
      <div className="card" style={{ marginBottom: 16 }}>
        <div className="card-title" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
          <span><span className="accent-dot" />Interactive 3D Finite Element Mesh</span>
          <span style={{ fontSize: 11, color: '#758078', fontWeight: 400 }}>
            {result.node_count.toLocaleString()} nodes · {result.element_count.toLocaleString()} tets · {result.boundary_face_count.toLocaleString()} faces
          </span>
        </div>
        <HeatSink3DViewer
          nodes={result.nodes}
          boundaryFaces={result.boundary_faces}
          elements={result.elements}
          mode="mesh"
        />
      </div>

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
function ThermalTab({
  result,
  meshResult
}: {
  result: HeatSinkThermalResult | null;
  meshResult: HeatSinkMeshResult | null;
}) {
  if (!result) return <EmptyState label="Click 'Run Thermal Solve' to execute the 3D FEM simulation" />;

  // 1. THERMAL API FAILURE HANDLING:
  // Do not calculate min/max/average temperature, heat output, energy balance or visualization data from an invalid solution.
  if (!result.success) {
    return (
      <div>
        <div className="alert" style={{
          background: '#FEE2E2', border: '1px solid #FCA5A5', color: '#991B1B',
          padding: 16, borderRadius: 8, marginBottom: 16
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 8, fontWeight: 700, fontSize: 15, marginBottom: 4 }}>
            <AlertCircle size={18} />
            Solver Execution Incomplete — {result.solver_name} Did Not Converge
          </div>
          <div style={{ fontSize: 13, color: '#7F1D1D' }}>
            {result.status_message}. Temperature metrics, energy conservation calculations, and 3D visual contours are withheld for invalid numerical solutions.
          </div>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Solver Diagnostic Details</div>
          <table className="data-table">
            <tbody>
              <tr><td>Solver</td><td style={{ fontWeight: 600 }}>{result.solver_name}</td></tr>
              <tr><td>Status</td><td style={{ color: '#F87171', fontWeight: 600 }}>✗ {result.status_message}</td></tr>
              <tr><td>Iterations Completed</td><td>{result.iterations}</td></tr>
              <tr><td>Final Residual Norm ‖r‖/‖b‖</td><td>{result.residual_norm.toExponential(3)}</td></tr>
              <tr><td>Assembly Time</td><td>{result.assembly_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Cholesky Factorization Time</td><td>{result.factorization_time_ms >= 0 ? `${result.factorization_time_ms.toFixed(2)} ms` : 'N/A'}</td></tr>
              <tr><td>Preconditioner / Setup Time</td><td>{result.setup_time_ms !== undefined && result.setup_time_ms >= 0 ? `${result.setup_time_ms.toFixed(2)} ms` : 'N/A'}</td></tr>
              <tr><td>Iteration / Solve Time</td><td>{result.solve_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Total Time</td><td>{result.total_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Memory (est.)</td><td>{result.memory_kb} KB</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    );
  }

  const nTemp = result.nodal_temperatures;
  const histBins = 20;
  const tMin = result.min_temperature_c ?? 25;
  const tMax = result.max_temperature_c ?? 25;
  const step = Math.max(0.1, (tMax - tMin) / histBins);
  const bins = Array.from({ length: histBins }, (_, i) => tMin + i * step);
  const counts = bins.map((b, i) =>
    nTemp.filter(t => t >= b && (i === histBins - 1 ? t <= b + step : t < b + step)).length
  );

  const displayNodes = result.nodes ?? meshResult?.nodes ?? [];
  const displayFaces = result.boundary_faces ?? meshResult?.boundary_faces ?? [];
  const displayElements = result.elements ?? meshResult?.elements;

  return (
    <div>
      {/* 3D Thermal Field Viewer */}
      {displayNodes.length > 0 && displayFaces.length > 0 && (
        <div className="card" style={{ marginBottom: 16 }}>
          <div className="card-title" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span><span className="accent-dot" />3D Thermal Distribution (FEA Temperature Field)</span>
            <span style={{ fontSize: 11, color: '#758078', fontWeight: 400 }}>
              Solver: <strong>{result.solver_name.split(' (')[0]}</strong> · T: [{tMin.toFixed(1)}°C – {tMax.toFixed(1)}°C]
            </span>
          </div>
          <HeatSink3DViewer
            nodes={displayNodes}
            boundaryFaces={displayFaces}
            elements={displayElements}
            nodalTemperatures={nTemp}
            minTemp={tMin}
            maxTemp={tMax}
            mode="thermal"
          />
        </div>
      )}

      <div className="grid-4" style={{ gap: 12, marginBottom: 16 }}>
        {[
          { label: 'Min Temperature', value: `${(result.min_temperature_c ?? 0).toFixed(2)} °C`, accent: '#36B8C5' },
          { label: 'Max Temperature', value: `${(result.max_temperature_c ?? 0).toFixed(2)} °C`, accent: '#F4A261' },
          { label: 'Avg Temperature', value: `${(result.avg_temperature_c ?? 0).toFixed(2)} °C`, accent: '#B7F34A' },
          { label: 'Thermal Resistance', value: `${(result.thermal_resistance_k_w ?? 0).toFixed(3)} K/W`, accent: '#202820' },
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
              <tr><td>Cholesky Factorization Time</td><td>{result.factorization_time_ms >= 0 ? `${result.factorization_time_ms.toFixed(2)} ms` : 'N/A'}</td></tr>
              <tr><td>Preconditioner / Setup Time</td><td>{result.setup_time_ms !== undefined && result.setup_time_ms >= 0 ? `${result.setup_time_ms.toFixed(2)} ms` : 'N/A'}</td></tr>
              <tr><td>Iteration / Solve Time</td><td>{result.solve_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Total Time</td><td>{result.total_time_ms.toFixed(1)} ms</td></tr>
              <tr><td>Memory (est.)</td><td>{result.memory_kb} KB</td></tr>
            </tbody>
          </table>
        </div>

        <div className="card">
          <div className="card-title"><span className="accent-dot" />Energy Balance</div>
          <table className="data-table" style={{ marginBottom: 12 }}>
            <tbody>
              <tr><td>Heat In (source)</td><td>{(result.total_heat_in_w ?? 0).toFixed(3)} W</td></tr>
              <tr><td>Heat Out (convection)</td><td>{(result.total_heat_out_w ?? 0).toFixed(3)} W</td></tr>
              <tr><td>Balance Error</td><td>
                <span style={{ color: (result.energy_balance_error_percent ?? 0) < 5 ? '#4ADE80' : '#F87171' }}>
                  {(result.energy_balance_error_percent ?? 0).toFixed(2)}%
                </span>
              </td></tr>
            </tbody>
          </table>
          {(result.energy_balance_error_percent ?? 0) < 5 && (
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
              <th>Median Total (ms)</th>
              <th>Cholesky Factorization (ms)</th>
              <th>Setup / Precond (ms)</th>
              <th>Solve / Iteration (ms)</th>
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
                <td>{s.median_factorization_time_ms >= 0 ? s.median_factorization_time_ms.toFixed(2) : 'N/A'}</td>
                <td>{s.median_setup_time_ms !== undefined && s.median_setup_time_ms >= 0 ? s.median_setup_time_ms.toFixed(2) : 'N/A'}</td>
                <td>{s.median_solve_time_ms.toFixed(2)}</td>
                <td>
                  <span style={{
                    padding: '2px 8px', borderRadius: 12,
                    background: s.speedup_vs_reference > 1 ? '#B7F34A33' : '#F4A26133',
                    color: s.speedup_vs_reference > 1 ? '#4a7a00' : '#c05000',
                    fontWeight: 600
                  }}>
                    {s.speedup_vs_reference > 0 ? `${s.speedup_vs_reference.toFixed(2)}×` : 'N/A'}
                  </span>
                </td>
                <td>{s.max_temp_diff_vs_reference_c >= 0 ? s.max_temp_diff_vs_reference_c.toFixed(4) : 'N/A'}</td>
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
              <tr>
                <th>Mesh</th>
                <th>Nodes</th>
                <th>Tets</th>
                <th>T_max (°C)</th>
                <th>Δ (%)</th>
                <th>Cholesky</th>
                <th>CG</th>
                <th>SSOR</th>
              </tr>
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
                  <td><span style={{ color: pt.cholesky_converged !== false ? '#4ADE80' : '#F87171' }}>
                    {pt.cholesky_converged !== false ? '✓' : '✗'}
                  </span></td>
                  <td><span style={{ color: pt.cg_converged !== false ? '#4ADE80' : '#F87171' }}>
                    {pt.cg_converged !== false ? '✓' : '✗'}
                  </span></td>
                  <td><span style={{ color: pt.gs_converged ? '#4ADE80' : '#F87171' }}>
                    {pt.gs_converged ? '✓' : '✗'}
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
