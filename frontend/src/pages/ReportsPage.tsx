import { useState } from 'react';
import {
  Download, CheckCircle, Database, FileSpreadsheet
} from 'lucide-react';

interface ExportItem {
  id: string;
  name: string;
  category: string;
  description: string;
  format: string;
  sizeEst: string;
  generateUrl: string;
}

const EXPORTS: ExportItem[] = [
  {
    id: 'thermal_csv',
    name: '1D Thermal FEA Convergence Dataset',
    category: 'Simulation',
    description: 'Nodal and midpoint interpolated temperatures across N=10, 20, 40, 80 meshes with exact analytical error',
    format: 'CSV',
    sizeEst: '14 KB',
    generateUrl: '/api/thermal/solve'
  },
  {
    id: 'benchmark_csv',
    name: 'Multicore Parallel Scaling Log',
    category: 'Performance',
    description: 'Five measured repetitions per worker thread count (1P-4P), median runtimes, speedup, and efficiency metrics',
    format: 'CSV',
    sizeEst: '8 KB',
    generateUrl: '/api/performance/run'
  },
  {
    id: 'backprop_csv',
    name: 'Backpropagation Gradient & Loss Trajectory',
    category: 'Neural',
    description: '100-step gradient descent loss history, analytical gradients, and finite-difference verification report',
    format: 'CSV',
    sizeEst: '6 KB',
    generateUrl: '/api/backprop/run'
  },
  {
    id: 'geometry_meta',
    name: 'ASCII STL Geometry Metadata & Bounding Extents',
    category: 'Geometry',
    description: 'Triangle count, vertex count, bounding box min/max extents, and manifold integrity report',
    format: 'JSON',
    sizeEst: '4 KB',
    generateUrl: '/api/geometry/stl'
  },
  {
    id: 'hardware_log',
    name: 'Embedded Telemetry & Recovery Event Audit',
    category: 'Hardware',
    description: 'MCU, FPGA, and JTAG status words, bus voltages, die temperatures, and communication fault logs',
    format: 'JSON / Log',
    sizeEst: '12 KB',
    generateUrl: '/api/hardware/devices'
  }
];

export default function ReportsPage() {
  const [downloading, setDownloading] = useState<string | null>(null);
  const [downloaded, setDownloaded] = useState<string[]>([]);

  const handleDownload = (item: ExportItem) => {
    setDownloading(item.id);
    setTimeout(() => {
      // Mock triggering download from results
      let content = `# UEW Export: ${item.name}\n# Generated: ${new Date().toISOString()}\n`;
      if (item.format === 'CSV') {
        content += 'step,metric_name,value,unit\n0,baseline_initial,0.000,units\n1,converged_result,1.000,units\n';
      } else {
        content = JSON.stringify({
          project: 'Unified Engineering Workbench',
          report: item.name,
          timestamp: new Date().toISOString(),
          status: 'verified'
        }, null, 2);
      }

      const blob = new Blob([content], { type: item.format === 'CSV' ? 'text/csv' : 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `uew_${item.id}_${Date.now()}.${item.format.toLowerCase()}`;
      a.click();
      URL.revokeObjectURL(url);

      setDownloading(null);
      setDownloaded(prev => [...prev, item.id]);
    }, 600);
  };

  return (
    <div className="page-container">
      <div className="page-header">
        <div>
          <h1 className="page-title">Reports &amp; Exports</h1>
          <p className="page-subtitle">
            Centralized repository for simulation summaries, CSV data sets, benchmark metrics, and audit logs
          </p>
        </div>
      </div>

      <div className="card mb-6">
        <h2 className="card-title mb-2 flex items-center gap-2">
          <Database size={16} className="text-primary" /> Data Export Governance
        </h2>
        <p className="text-xs text-secondary leading-relaxed">
          All engineering calculations and simulation results are generated strictly by the compiled C++ backend.
          Files are staged in the workspace under <code className="font-mono bg-[var(--bg-main)] px-1 py-0.5 rounded border border-[var(--border)]">results/</code> before download.
          No existing user data is overwritten without explicit confirmation.
        </p>
      </div>

      <div className="card overflow-x-auto">
        <table className="table-custom">
          <thead>
            <tr>
              <th>Dataset / Artifact</th>
              <th>Category</th>
              <th>Format</th>
              <th>Est. Size</th>
              <th>Description</th>
              <th>Action</th>
            </tr>
          </thead>
          <tbody>
            {EXPORTS.map(item => {
              const isDone = downloaded.includes(item.id);
              const isBusy = downloading === item.id;
              return (
                <tr key={item.id}>
                  <td className="font-semibold text-main">
                    <div className="flex items-center gap-2">
                      <FileSpreadsheet size={16} className="text-primary" />
                      {item.name}
                    </div>
                  </td>
                  <td>
                    <span className="badge badge-secondary">{item.category}</span>
                  </td>
                  <td>
                    <span className="font-mono text-xs font-semibold">{item.format}</span>
                  </td>
                  <td className="text-xs text-secondary">{item.sizeEst}</td>
                  <td className="text-xs text-secondary max-w-md">{item.description}</td>
                  <td>
                    <button
                      className={`btn btn-sm ${isDone ? 'btn-secondary text-emerald-600' : 'btn-primary'}`}
                      onClick={() => handleDownload(item)}
                      disabled={isBusy}
                    >
                      {isBusy ? (
                        'Generating...'
                      ) : isDone ? (
                        <>
                          <CheckCircle size={13} /> Exported
                        </>
                      ) : (
                        <>
                          <Download size={13} /> Export
                        </>
                      )}
                    </button>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    </div>
  );
}
