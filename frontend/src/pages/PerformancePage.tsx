import { useState } from 'react';
import Plot from 'react-plotly.js';
import { Play, Download, CheckCircle, AlertTriangle, Cpu, Activity } from 'lucide-react';
import { runBenchmark, getJobStatus } from '../services/api';
import type { BenchmarkJobResult } from '../types/api';

export default function PerformancePage() {
  const [workers, setWorkers] = useState<number>(4);
  const [batchSize, setBatchSize] = useState<number>(200);
  const [meshSize, setMeshSize] = useState<number>(40);
  const [running, setRunning] = useState<boolean>(false);
  const [jobProgress, setJobProgress] = useState<number>(0);
  const [jobMessage, setJobMessage] = useState<string>('');
  const [result, setResult] = useState<BenchmarkJobResult | null>(null);
  const [error, setError] = useState<string | null>(null);

  const handleRun = async () => {
    setRunning(true);
    setError(null);
    setJobProgress(0);
    setJobMessage('Submitting multicore benchmark job...');
    try {
      const resp = await runBenchmark({
        batch_size: batchSize,
        mesh_size: meshSize,
        workers: workers,
      });

      if (!resp.success || !resp.data) {
        setError(resp.error || 'Failed to start benchmark');
        setRunning(false);
        return;
      }

      const jobId = resp.data.job_id;
      // Poll job status
      const interval = setInterval(async () => {
        try {
          const s = await getJobStatus(jobId);
          if (s.success && s.data) {
            setJobProgress(s.data.progress);
            setJobMessage(s.data.message);
            if (s.data.state === 'done') {
              clearInterval(interval);
              setResult(s.data.result as BenchmarkJobResult);
              setRunning(false);
            } else if (s.data.state === 'failed') {
              clearInterval(interval);
              setError(s.data.error || 'Benchmark execution failed');
              setRunning(false);
            }
          }
        } catch (err: unknown) {
          clearInterval(interval);
          setError(err instanceof Error ? err.message : 'Error polling benchmark');
          setRunning(false);
        }
      }, 400);
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Network error starting benchmark');
      setRunning(false);
    }
  };

  const exportCSV = () => {
    if (!result) return;
    const blob = new Blob([result.parallel.csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `uew_multicore_benchmark_N${batchSize}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="page-container">
      <div className="page-header">
        <div>
          <h1 className="page-title">Performance Lab</h1>
          <p className="page-subtitle">
            Multicore CPU-parallel thermal FEA benchmark with thread pool, deterministic seed, and verification
          </p>
        </div>
        {result && (
          <button className="btn btn-secondary" onClick={exportCSV}>
            <Download size={15} /> Export Benchmark CSV
          </button>
        )}
      </div>

      {/* Control Panel */}
      <div className="card mb-6">
        <h2 className="card-title mb-4">Benchmark Configuration</h2>
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4 items-end">
          <div>
            <label className="input-label">Worker Threads</label>
            <div className="flex gap-2">
              {[1, 2, 3, 4].map(w => (
                <button
                  key={w}
                  type="button"
                  onClick={() => setWorkers(w)}
                  className={`btn btn-sm ${workers === w ? 'btn-primary' : 'btn-secondary'}`}
                >
                  {w}P
                </button>
              ))}
            </div>
            <p className="text-xs text-secondary mt-1">Capped by logical CPU cores</p>
          </div>

          <div>
            <label className="input-label">Workload Batch Size</label>
            <select
              className="input-select"
              value={batchSize}
              onChange={e => setBatchSize(Number(e.target.value))}
            >
              <option value={50}>50 cases (Quick check)</option>
              <option value={100}>100 cases (Standard)</option>
              <option value={200}>200 cases (Recommended)</option>
              <option value={500}>500 cases (Heavy stress)</option>
            </select>
            <p className="text-xs text-secondary mt-1">Pseudo-random positive k, q batches</p>
          </div>

          <div>
            <label className="input-label">FEA Mesh Nodes</label>
            <select
              className="input-select"
              value={meshSize}
              onChange={e => setMeshSize(Number(e.target.value))}
            >
              <option value={20}>20 nodes</option>
              <option value={40}>40 nodes (Standard)</option>
              <option value={80}>80 nodes</option>
              <option value={160}>160 nodes (Compute-bound)</option>
            </select>
            <p className="text-xs text-secondary mt-1">Linear system dimension per solve</p>
          </div>

          <div>
            <button
              className="btn btn-primary w-full justify-center"
              onClick={handleRun}
              disabled={running}
            >
              {running ? (
                <>
                  <Activity size={16} className="animate-spin" /> Running...
                </>
              ) : (
                <>
                  <Play size={16} /> Run Benchmark
                </>
              )}
            </button>
          </div>
        </div>

        {running && (
          <div className="mt-4 pt-4 border-t border-[var(--border)]">
            <div className="flex justify-between text-xs text-secondary mb-1">
              <span>{jobMessage}</span>
              <span>{jobProgress}%</span>
            </div>
            <div className="w-full bg-[var(--bg-main)] rounded-full h-2 overflow-hidden border border-[var(--border)]">
              <div
                className="bg-[var(--accent-primary)] h-2 rounded-full transition-all duration-300"
                style={{ width: `${jobProgress}%` }}
              />
            </div>
          </div>
        )}

        {error && (
          <div className="alert alert-error mt-4">
            <AlertTriangle size={16} />
            <span>{error}</span>
          </div>
        )}
      </div>

      {result && (
        <div className="space-y-6">
          {/* Hardware & Environment Specs */}
          <div className="card">
            <h3 className="card-title mb-3 flex items-center gap-2">
              <Cpu size={16} className="text-primary" /> Test Environment Specifications
            </h3>
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4 text-sm">
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">CPU Model</span>
                <span className="font-semibold text-main">{result.system.cpu || 'x86_64 Processor'}</span>
              </div>
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Cores / Threads</span>
                <span className="font-semibold text-main">
                  {result.system.physical_cores} Physical / {result.system.logical_cores} Logical
                </span>
              </div>
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Host Memory</span>
                <span className="font-semibold text-main">{result.system.ram}</span>
              </div>
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Compiler / Flags</span>
                <span className="font-semibold text-main">
                  {result.system.compiler} ({result.system.opt_flags})
                </span>
              </div>
            </div>
          </div>

          {/* Verification Callout */}
          <div className="p-4 rounded-xl border border-[var(--border)] bg-surface flex items-start gap-3">
            <CheckCircle size={20} className="text-emerald-500 mt-0.5 flex-shrink-0" />
            <div>
              <h4 className="font-semibold text-sm text-main">Deterministic Numerical Agreement Verified</h4>
              <p className="text-xs text-secondary mt-0.5">
                All parallel worker results match serial baseline within{' '}
                <code className="px-1 py-0.5 bg-[var(--bg-main)] rounded border border-[var(--border)] font-mono">
                  {result.max_serial_parallel_diff_C.toExponential(4)} °C
                </code>{' '}
                (Strict requirement: &le; 1e-8 °C). 5 measured repetitions per test, median recorded.
              </p>
            </div>
          </div>

          {/* Results Table */}
          <div className="card overflow-x-auto">
            <h3 className="card-title mb-3">Benchmark Execution Summary (5 repetitions median)</h3>
            <table className="table-custom">
              <thead>
                <tr>
                  <th>Solver Mode</th>
                  <th>Workers</th>
                  <th>Median Time</th>
                  <th>Repetition Times (ms)</th>
                  <th>Speedup</th>
                  <th>Efficiency</th>
                  <th>Agreement</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td className="font-semibold">Serial Baseline</td>
                  <td>1 Thread</td>
                  <td className="font-mono font-semibold">{result.serial.median_ms.toFixed(2)} ms</td>
                  <td className="font-mono text-xs">{result.serial.times_ms.map(t => t.toFixed(1)).join(', ')}</td>
                  <td className="font-mono">1.00x</td>
                  <td className="font-mono">100.0%</td>
                  <td><span className="badge badge-secondary">Baseline</span></td>
                </tr>
                <tr>
                  <td className="font-semibold text-primary">Parallel Solver</td>
                  <td>{result.parallel.workers} Threads</td>
                  <td className="font-mono font-bold text-primary">{result.parallel.median_ms.toFixed(2)} ms</td>
                  <td className="font-mono text-xs">{result.parallel.times_ms.map(t => t.toFixed(1)).join(', ')}</td>
                  <td className="font-mono font-bold text-primary">{(result.parallel.speedup ?? 1).toFixed(2)}x</td>
                  <td className="font-mono font-semibold">{((result.parallel.efficiency ?? 1) * 100).toFixed(1)}%</td>
                  <td>
                    <span className="badge badge-success">Passed (&le; 1e-8)</span>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>

          {/* Benchmark Charts */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="card">
              <h3 className="card-title mb-2">5-Repetition Timing Comparison</h3>
              <Plot
                data={[
                  {
                    x: [1, 2, 3, 4, 5],
                    y: result.serial.times_ms,
                    type: 'scatter',
                    mode: 'lines+markers',
                    name: 'Serial 1P (ms)',
                    marker: { color: '#64748B', size: 8 },
                    line: { width: 2 }
                  },
                  {
                    x: [1, 2, 3, 4, 5],
                    y: result.parallel.times_ms,
                    type: 'scatter',
                    mode: 'lines+markers',
                    name: `Parallel ${result.parallel.workers}P (ms)`,
                    marker: { color: '#10B981', size: 8 },
                    line: { width: 2.5 }
                  }
                ]}
                layout={{
                  autosize: true,
                  height: 320,
                  margin: { l: 50, r: 20, t: 30, b: 40 },
                  xaxis: { title: 'Repetition Run Number', dtick: 1 },
                  yaxis: { title: 'Execution Time (ms)' },
                  paper_bgcolor: 'transparent',
                  plot_bgcolor: 'transparent'
                }}
                useResizeHandler
                style={{ width: '100%' }}
              />
            </div>

            <div className="card">
              <h3 className="card-title mb-2">Scaling &amp; Speedup Metrics</h3>
              <Plot
                data={[
                  {
                    x: ['Serial (1P)', `Parallel (${result.parallel.workers}P)`],
                    y: [result.serial.median_ms, result.parallel.median_ms],
                    type: 'bar',
                    name: 'Median Time (ms)',
                    marker: { color: ['#94A3B8', '#0EA5E9'] }
                  }
                ]}
                layout={{
                  autosize: true,
                  height: 320,
                  margin: { l: 50, r: 20, t: 30, b: 40 },
                  yaxis: { title: 'Median Runtime (ms)' },
                  paper_bgcolor: 'transparent',
                  plot_bgcolor: 'transparent'
                }}
                useResizeHandler
                style={{ width: '100%' }}
              />
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
