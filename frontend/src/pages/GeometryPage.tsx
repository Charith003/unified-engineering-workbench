import { useRef, useEffect, useState, useCallback } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import Plot from 'react-plotly.js';
import { Upload, Box, Info } from 'lucide-react';
import { parseSTL, parseCSV } from '../services/api';
import type { STLResult, CSVResult } from '../types/api';

// ─── STL Three.js Viewer ─────────────────────────────────
function STLViewer({ data }: { data: STLResult | null }) {
  const mountRef = useRef<HTMLDivElement>(null);
  const sceneRef = useRef<{ renderer: THREE.WebGLRenderer; controls: OrbitControls; camera: THREE.PerspectiveCamera; } | null>(null);

  useEffect(() => {
    if (!mountRef.current) return;
    const el = mountRef.current;
    const W = el.clientWidth, H = el.clientHeight;

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(W, H);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setClearColor(0x0f1210);
    el.appendChild(renderer.domElement);

    const scene  = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(50, W / H, 0.001, 10000);
    camera.position.set(2, 2, 4);

    scene.add(new THREE.AmbientLight(0xffffff, 0.5));
    const dir = new THREE.DirectionalLight(0xffffff, 1.0);
    dir.position.set(5, 10, 7);
    scene.add(dir);

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.1;
    sceneRef.current = { renderer, controls, camera };

    // Grid
    const grid = new THREE.GridHelper(10, 20, 0x2a3a2a, 0x1e2a1e);
    scene.add(grid);

    let animId: number;
    const animate = () => {
      animId = requestAnimationFrame(animate);
      controls.update();
      renderer.render(scene, camera);
    };
    animate();

    // Build mesh if data
    if (data) {
      const geom = new THREE.BufferGeometry();
      const verts: number[] = [];
      for (const tri of data.triangles) {
        verts.push(...tri.v0, ...tri.v1, ...tri.v2);
      }
      geom.setAttribute('position', new THREE.Float32BufferAttribute(verts, 3));
      geom.computeVertexNormals();

      const mat  = new THREE.MeshPhongMaterial({ color: 0x36B8C5, shininess: 60, side: THREE.DoubleSide });
      const mesh = new THREE.Mesh(geom, mat);

      // Centre mesh
      geom.computeBoundingBox();
      const box = geom.boundingBox!;
      const cx = (box.max.x + box.min.x) / 2;
      const cy = (box.max.y + box.min.y) / 2;
      const cz = (box.max.z + box.min.z) / 2;
      mesh.position.set(-cx, -cy, -cz);
      scene.add(mesh);

      // Wireframe overlay
      const wireGeo = new THREE.WireframeGeometry(geom);
      const wireMat = new THREE.LineBasicMaterial({ color: 0xB7F34A, opacity: 0.15, transparent: true });
      const wire    = new THREE.LineSegments(wireGeo, wireMat);
      wire.position.copy(mesh.position);
      scene.add(wire);

      // Fit camera
      const size = Math.max(data.bbox.dx, data.bbox.dy, data.bbox.dz);
      camera.position.set(size * 1.5, size * 1.2, size * 2);
      camera.lookAt(0, 0, 0);
      controls.update();
    }

    const handleResize = () => {
      const w = el.clientWidth, h = el.clientHeight;
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
      renderer.setSize(w, h);
    };
    window.addEventListener('resize', handleResize);

    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener('resize', handleResize);
      el.removeChild(renderer.domElement);
      renderer.dispose();
    };
  }, [data]);

  if (!data) {
    return (
      <div className="viewport-3d" style={{ height: 360, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <div className="empty-state">
          <Box size={40} className="empty-state-icon" style={{ color: '#B7F34A' }} />
          <div className="empty-state-title" style={{ color: '#aaa' }}>No geometry loaded</div>
          <div className="empty-state-desc" style={{ color: '#666' }}>Upload an ASCII STL file to view the 3D model</div>
        </div>
      </div>
    );
  }

  return <div ref={mountRef} className="viewport-3d" style={{ height: 360, width: '100%' }} />;
}

// ─── Temperature Point Cloud Viewer ──────────────────────
function TempViewer({ data }: { data: CSVResult | null }) {
  if (!data) {
    return (
      <div className="viewport-3d" style={{ height: 360, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <div className="empty-state">
          <Box size={40} className="empty-state-icon" style={{ color: '#36B8C5' }} />
          <div className="empty-state-title" style={{ color: '#aaa' }}>No temperature data</div>
          <div className="empty-state-desc" style={{ color: '#666' }}>Upload a temperature CSV to view node cloud</div>
        </div>
      </div>
    );
  }

  const xs = data.nodes.map(n => n.x);
  const ys = data.nodes.map(n => n.y);
  const zs = data.nodes.map(n => n.z);
  const ts = data.nodes.map(n => n.temp);
  const ids = data.nodes.map(n => `Node ${n.id}: ${n.temp.toFixed(2)} °C`);

  return (
    <Plot
      data={[{
        type: 'scatter3d',
        x: xs, y: ys, z: zs,
        mode: 'markers',
        marker: {
          size: 5,
          color: ts,
          colorscale: 'Jet',
          colorbar: { title: '°C', thickness: 14, len: 0.8 },
          showscale: true,
          cmin: data.t_min,
          cmax: data.t_max,
        },
        text: ids,
        hovertemplate: '%{text}<extra></extra>',
      } as any]}
      layout={{
        margin: { l: 0, r: 0, t: 0, b: 0 },
        paper_bgcolor: '#0f1210',
        plot_bgcolor:  '#0f1210',
        scene: {
          bgcolor: '#0f1210',
          xaxis: { title: 'X (m)', color: '#aaa', gridcolor: '#2a3a2a' },
          yaxis: { title: 'Y (m)', color: '#aaa', gridcolor: '#2a3a2a' },
          zaxis: { title: 'Z (m)', color: '#aaa', gridcolor: '#2a3a2a' },
        },
        font: { color: '#aaa', family: 'Inter' },
      }}
      config={{ displayModeBar: true, responsive: true }}
      style={{ height: 360, width: '100%' }}
    />
  );
}

// ─── File Upload Zone ─────────────────────────────────────
function FileUpload({ label, accept, onContent, loading }: {
  label: string; accept: string; onContent: (s: string) => void; loading: boolean;
}) {
  const [dragging, setDragging] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);

  const handle = useCallback((file: File) => {
    const reader = new FileReader();
    reader.onload = e => onContent(e.target!.result as string);
    reader.readAsText(file);
  }, [onContent]);

  return (
    <div
      className={`upload-zone ${dragging ? 'drag-over' : ''}`}
      onClick={() => inputRef.current?.click()}
      onDragOver={e => { e.preventDefault(); setDragging(true); }}
      onDragLeave={() => setDragging(false)}
      onDrop={e => { e.preventDefault(); setDragging(false); const f = e.dataTransfer.files[0]; if (f) handle(f); }}
    >
      <input ref={inputRef} type="file" accept={accept}
             onChange={e => { const f = e.target.files?.[0]; if (f) handle(f); }} />
      {loading ? <div className="spinner" /> : <Upload size={24} style={{ opacity: 0.4, marginBottom: 8 }} />}
      <div style={{ fontSize: 13, fontWeight: 600, marginBottom: 4 }}>{label}</div>
      <div style={{ fontSize: 12, color: 'var(--text-secondary)' }}>Click to browse or drag & drop</div>
    </div>
  );
}

// ─── Page ─────────────────────────────────────────────────
export default function GeometryPage() {
  const [stlData,   setStlData]   = useState<STLResult | null>(null);
  const [csvData,   setCsvData]   = useState<CSVResult | null>(null);
  const [stlError,  setStlError]  = useState('');
  const [csvError,  setCsvError]  = useState('');
  const [stlLoading, setStlLoading] = useState(false);
  const [csvLoading, setCsvLoading] = useState(false);

  // Sample STL
  const SAMPLE_STL = `solid sample_cube
  facet normal 0.0 0.0 1.0
    outer loop
      vertex 0.0 0.0 1.0
      vertex 1.0 0.0 1.0
      vertex 1.0 1.0 1.0
    endloop
  endfacet
  facet normal 0.0 0.0 1.0
    outer loop
      vertex 0.0 0.0 1.0
      vertex 1.0 1.0 1.0
      vertex 0.0 1.0 1.0
    endloop
  endfacet
  facet normal 0.0 0.0 -1.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 1.0 1.0 0.0
      vertex 1.0 0.0 0.0
    endloop
  endfacet
  facet normal 0.0 0.0 -1.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 0.0 1.0 0.0
      vertex 1.0 1.0 0.0
    endloop
  endfacet
  facet normal 0.0 1.0 0.0
    outer loop
      vertex 0.0 1.0 0.0
      vertex 1.0 1.0 1.0
      vertex 1.0 1.0 0.0
    endloop
  endfacet
  facet normal 0.0 1.0 0.0
    outer loop
      vertex 0.0 1.0 0.0
      vertex 0.0 1.0 1.0
      vertex 1.0 1.0 1.0
    endloop
  endfacet
  facet normal 0.0 -1.0 0.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 1.0 0.0 0.0
      vertex 1.0 0.0 1.0
    endloop
  endfacet
  facet normal 0.0 -1.0 0.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 1.0 0.0 1.0
      vertex 0.0 0.0 1.0
    endloop
  endfacet
  facet normal 1.0 0.0 0.0
    outer loop
      vertex 1.0 0.0 0.0
      vertex 1.0 1.0 0.0
      vertex 1.0 1.0 1.0
    endloop
  endfacet
  facet normal 1.0 0.0 0.0
    outer loop
      vertex 1.0 0.0 0.0
      vertex 1.0 1.0 1.0
      vertex 1.0 0.0 1.0
    endloop
  endfacet
  facet normal -1.0 0.0 0.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 0.0 1.0 1.0
      vertex 0.0 1.0 0.0
    endloop
  endfacet
  facet normal -1.0 0.0 0.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 0.0 0.0 1.0
      vertex 0.0 1.0 1.0
    endloop
  endfacet
endsolid sample_cube`;

  const SAMPLE_CSV = `node_id,x_m,y_m,z_m,temperature_C
1,0.0,0.0,0.0,100.0
2,0.25,0.0,0.0,145.3
3,0.5,0.0,0.0,175.2
4,0.75,0.0,0.0,160.8
5,1.0,0.0,0.0,20.0
6,0.0,0.5,0.0,98.5
7,0.5,0.5,0.0,170.1
8,1.0,0.5,0.0,22.3
9,0.0,1.0,0.0,95.2
10,0.5,1.0,0.0,165.7
11,1.0,1.0,0.0,21.1
12,0.5,0.5,0.5,180.5`;

  async function handleSTL(content: string) {
    setStlLoading(true); setStlError('');
    try {
      const r = await parseSTL(content);
      if (r.success) setStlData(r.data);
      else setStlError(r.error ?? 'Parse failed');
    } catch (e: unknown) {
      setStlError(e instanceof Error ? e.message : 'Network error');
    }
    setStlLoading(false);
  }

  async function handleCSV(content: string) {
    setCsvLoading(true); setCsvError('');
    try {
      const r = await parseCSV(content);
      if (r.success) setCsvData(r.data);
      else setCsvError(r.error ?? 'Parse failed');
    } catch (e: unknown) {
      setCsvError(e instanceof Error ? e.message : 'Network error');
    }
    setCsvLoading(false);
  }

  return (
    <div className="page-container">
      <div className="page-header">
        <h1 className="page-title">Geometry Studio</h1>
        <p className="page-subtitle">
          Upload ASCII STL meshes and temperature CSV exports for 3D visualization.
          All parsing is performed by the C++ backend.
        </p>
      </div>

      <div className="grid-2" style={{ marginBottom: 24 }}>
        {/* STL Panel */}
        <div className="card">
          <div className="card-title"><span className="accent-dot" />STL Geometry (ASCII)</div>
          <FileUpload label="Upload ASCII STL" accept=".stl" onContent={handleSTL} loading={stlLoading} />
          <button className="btn btn-secondary btn-sm" style={{ marginTop: 10 }} onClick={() => handleSTL(SAMPLE_STL)}>
            Load Sample Cube STL
          </button>
          {stlError && <div className="alert alert-error" style={{ marginTop: 10 }}><Info size={13} />{stlError}</div>}
          {stlData && (
            <div style={{ marginTop: 12, display: 'flex', gap: 8, flexWrap: 'wrap' }}>
              <span className="badge badge-success">✓ Parsed</span>
              <span className="badge badge-neutral">{stlData.triangle_count} triangles</span>
              <span className="badge badge-neutral">{stlData.solid_name || 'unnamed'}</span>
            </div>
          )}
        </div>

        {/* CSV Panel */}
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Temperature CSV</div>
          <div className="alert alert-info" style={{ marginBottom: 12, fontSize: 12 }}>
            <Info size={13} />
            Expected columns: node_id, x_m, y_m, z_m, temperature_C
            <br/><strong>⚠ Synthetic data — not an ANSYS export.</strong>
          </div>
          <FileUpload label="Upload Temperature CSV" accept=".csv" onContent={handleCSV} loading={csvLoading} />
          <button className="btn btn-secondary btn-sm" style={{ marginTop: 10 }} onClick={() => handleCSV(SAMPLE_CSV)}>
            Load Synthetic CSV
          </button>
          {csvError && <div className="alert alert-error" style={{ marginTop: 10 }}><Info size={13} />{csvError}</div>}
          {csvData && (
            <div style={{ marginTop: 12, display: 'flex', gap: 8, flexWrap: 'wrap' }}>
              <span className="badge badge-success">✓ Parsed</span>
              <span className="badge badge-neutral">{csvData.node_count} nodes</span>
              <span className="badge badge-neutral">{csvData.t_min.toFixed(1)}–{csvData.t_max.toFixed(1)} °C</span>
            </div>
          )}
        </div>
      </div>

      {/* 3D Viewers */}
      <div className="grid-2" style={{ marginBottom: 24 }}>
        <div>
          <div style={{ fontSize: 12, fontWeight: 600, color: 'var(--text-secondary)', marginBottom: 8 }}>
            STL MESH VIEWER — Three.js · Orbit / Pan / Zoom
          </div>
          <STLViewer data={stlData} />
          {stlData && (
            <div style={{ marginTop: 8, display: 'flex', gap: 16, fontSize: 12, color: 'var(--text-secondary)' }}>
              <span>ΔX: {stlData.bbox.dx.toFixed(4)} m</span>
              <span>ΔY: {stlData.bbox.dy.toFixed(4)} m</span>
              <span>ΔZ: {stlData.bbox.dz.toFixed(4)} m</span>
            </div>
          )}
        </div>
        <div>
          <div style={{ fontSize: 12, fontWeight: 600, color: 'var(--text-secondary)', marginBottom: 8 }}>
            TEMPERATURE POINT CLOUD — Plotly 3D scatter · Jet colormap
          </div>
          <TempViewer data={csvData} />
          {csvData && (
            <div style={{ marginTop: 8, display: 'flex', gap: 16, fontSize: 12, color: 'var(--text-secondary)' }}>
              <span>Min: {csvData.t_min.toFixed(2)} °C</span>
              <span>Max: {csvData.t_max.toFixed(2)} °C</span>
              <span>Nodes: {csvData.node_count}</span>
            </div>
          )}
        </div>
      </div>

      {/* STL metadata table */}
      {stlData && (
        <div className="card">
          <div className="card-title"><span className="accent-dot" />Geometry Metadata</div>
          <table className="data-table">
            <thead><tr><th>Property</th><th>Value</th></tr></thead>
            <tbody>
              <tr><td>Solid name</td><td>{stlData.solid_name || '(unnamed)'}</td></tr>
              <tr><td>Triangle count</td><td>{stlData.triangle_count.toLocaleString()}</td></tr>
              <tr><td>Bounding X</td><td>[{stlData.bbox.xmin.toFixed(4)}, {stlData.bbox.xmax.toFixed(4)}] m → Δ{stlData.bbox.dx.toFixed(4)} m</td></tr>
              <tr><td>Bounding Y</td><td>[{stlData.bbox.ymin.toFixed(4)}, {stlData.bbox.ymax.toFixed(4)}] m → Δ{stlData.bbox.dy.toFixed(4)} m</td></tr>
              <tr><td>Bounding Z</td><td>[{stlData.bbox.zmin.toFixed(4)}, {stlData.bbox.zmax.toFixed(4)}] m → Δ{stlData.bbox.dz.toFixed(4)} m</td></tr>
              <tr><td>File size limit (tested)</td><td>{stlData.file_size_note}</td></tr>
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
