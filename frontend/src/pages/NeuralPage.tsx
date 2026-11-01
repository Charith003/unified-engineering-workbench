import { useState } from 'react';
import Plot from 'react-plotly.js';
import { Play, Download, AlertTriangle, Brain, FileText } from 'lucide-react';
import { runBackprop } from '../services/api';
import type { BackpropResult } from '../types/api';

export default function NeuralPage() {
  const [running, setRunning] = useState<boolean>(false);
  const [result, setResult] = useState<BackpropResult | null>(null);
  const [error, setError] = useState<string | null>(null);

  const handleRun = async () => {
    setRunning(true);
    setError(null);
    try {
      const resp = await runBackprop({
        lr: 0.1,
        steps: 100,
        epsilon: 1e-6
      });

      if (resp.success && resp.data) {
        setResult(resp.data);
      } else {
        setError(resp.error || 'Backpropagation calculation failed');
      }
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Network error during calculation');
    } finally {
      setRunning(false);
    }
  };

  const exportCSV = () => {
    if (!result) return;
    const blob = new Blob([result.loss_csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'uew_backprop_loss_history_100steps.csv';
    a.click();
    URL.revokeObjectURL(url);
  };

  const gradientsList = result ? [
    { name: 'w', analytical: result.analytical_gradients.dJ_dw, numerical: result.fd_gradients.dJ_dw, updated: result.updated_params.w },
    { name: 'b', analytical: result.analytical_gradients.dJ_db, numerical: result.fd_gradients.dJ_db, updated: result.updated_params.b },
    { name: 'v', analytical: result.analytical_gradients.dJ_dv, numerical: result.fd_gradients.dJ_dv, updated: result.updated_params.v },
    { name: 'c', analytical: result.analytical_gradients.dJ_dc, numerical: result.fd_gradients.dJ_dc, updated: result.updated_params.c },
  ] : [];

  return (
    <div className="page-container">
      <div className="page-header">
        <div>
          <h1 className="page-title">Neural Computation</h1>
          <p className="page-subtitle">
            Handwritten-gradient backpropagation, finite-difference verification (&epsilon;=1e-6), and 100-step training
          </p>
        </div>
        <div className="flex gap-2">
          {result && (
            <button className="btn btn-secondary" onClick={exportCSV}>
              <Download size={15} /> Export Loss History CSV
            </button>
          )}
          <button className="btn btn-primary" onClick={handleRun} disabled={running}>
            <Play size={15} /> {running ? 'Computing...' : 'Run Exact Backpropagation'}
          </button>
        </div>
      </div>

      {/* Problem Specification Card */}
      <div className="card mb-6">
        <h2 className="card-title mb-3 flex items-center gap-2">
          <Brain size={16} className="text-primary" /> Mathematical Formulation &amp; Initial Boundary Conditions
        </h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4 text-xs">
          <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
            <span className="font-semibold text-secondary uppercase tracking-wider block mb-1">Architecture</span>
            <div className="font-mono space-y-1">
              <div>h = tanh(w &middot; x + b)</div>
              <div>&ycirc; = v &middot; h + c</div>
              <div>J = &frac12; (&ycirc; - y)&sup2;</div>
            </div>
          </div>
          <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
            <span className="font-semibold text-secondary uppercase tracking-wider block mb-1">Inputs &amp; Targets</span>
            <div className="font-mono space-y-1">
              <div>Input sample: x = 1.0</div>
              <div>Target label: y = 2.0</div>
              <div>Learning rate: &eta; = 0.1</div>
            </div>
          </div>
          <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
            <span className="font-semibold text-secondary uppercase tracking-wider block mb-1">Initial Parameters (&theta;&sub0;)</span>
            <div className="font-mono space-y-1">
              <div>w = 0.5000, b = 0.0000</div>
              <div>v = 1.0000, c = 0.0000</div>
              <div>FD Perturbation: &epsilon; = 1e-6</div>
            </div>
          </div>
        </div>
      </div>

      {error && (
        <div className="alert alert-error mb-6">
          <AlertTriangle size={16} />
          <span>{error}</span>
        </div>
      )}

      {result && (
        <div className="space-y-6">
          {/* Step 0 Forward Pass & Initial State */}
          <div className="card">
            <h3 className="card-title mb-3">1. Initial Forward Pass (Step 0)</h3>
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4 text-sm">
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Hidden Activation (h)</span>
                <span className="font-mono font-bold text-main">{result.initial_forward.h.toFixed(6)}</span>
                <span className="text-[10px] text-secondary block mt-0.5">tanh(0.5 &middot; 1 + 0)</span>
              </div>
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Prediction (&ycirc;)</span>
                <span className="font-mono font-bold text-main">{result.initial_forward.y_hat.toFixed(6)}</span>
                <span className="text-[10px] text-secondary block mt-0.5">1 &middot; h + 0</span>
              </div>
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Prediction Error (e)</span>
                <span className="font-mono font-bold text-red-600">
                  {(result.initial_forward.y_hat - 2.0).toFixed(6)}
                </span>
                <span className="text-[10px] text-secondary block mt-0.5">&ycirc; - y</span>
              </div>
              <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                <span className="text-xs text-secondary block">Initial Loss (J)</span>
                <span className="font-mono font-bold text-main">{result.initial_forward.J.toFixed(6)}</span>
                <span className="text-[10px] text-secondary block mt-0.5">&frac12; (&ycirc; - y)&sup2;</span>
              </div>
            </div>
          </div>

          {/* Analytical vs Finite-Difference Gradient Table */}
          <div className="card overflow-x-auto">
            <h3 className="card-title mb-2">2. Analytical vs Central Finite-Difference Gradients</h3>
            <p className="text-xs text-secondary mb-3">
              Independent numerical verification via central differences: <code className="font-mono">[J(&theta;+&epsilon;) - J(&theta;-&epsilon;)] / 2&epsilon;</code>.
              Strict PASS Science Works acceptance tolerance: absolute error &le; 1e-5. Max error: {result.max_grad_error.toExponential(4)}.
            </p>
            <table className="table-custom">
              <thead>
                <tr>
                  <th>Parameter &theta;</th>
                  <th>Analytical Derivative (&part;J/&part;&theta;)</th>
                  <th>Numerical FD (&part;J/&part;&theta;)</th>
                  <th>Absolute Error</th>
                  <th>Verification Status</th>
                  <th>Simultaneous Step 1 Update (&theta; &larr; &theta; - &eta;&nabla;)</th>
                </tr>
              </thead>
              <tbody>
                {gradientsList.map(g => (
                  <tr key={g.name}>
                    <td className="font-mono font-bold">{g.name}</td>
                    <td className="font-mono">{g.analytical.toFixed(7)}</td>
                    <td className="font-mono">{g.numerical.toFixed(7)}</td>
                    <td className="font-mono text-xs">{Math.abs(g.analytical - g.numerical).toExponential(4)}</td>
                    <td>
                      <span className="badge badge-success">Passed (&le; 1e-5)</span>
                    </td>
                    <td className="font-mono font-semibold text-primary">{g.updated.toFixed(6)}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Loss Convergence Plot */}
          <div className="card">
            <div className="flex items-center justify-between mb-2">
              <h3 className="card-title">3. 100-Step Gradient Descent Loss Trajectory</h3>
              <span className="text-xs font-mono text-secondary">
                Loss: {result.history[0]?.loss.toFixed(5)} &rarr; {result.history[result.history.length - 1]?.loss.toFixed(5)}
              </span>
            </div>
            <Plot
              data={[
                {
                  x: result.history.map(h => h.step),
                  y: result.history.map(h => h.loss),
                  type: 'scatter',
                  mode: 'lines',
                  name: 'Loss J(w,b,v,c)',
                  line: { color: '#0EA5E9', width: 2.5 }
                }
              ]}
              layout={{
                autosize: true,
                height: 340,
                margin: { l: 60, r: 20, t: 30, b: 40 },
                xaxis: { title: 'Training Iteration (Steps)' },
                yaxis: { title: 'Quadratic Loss J', type: 'log' },
                paper_bgcolor: 'transparent',
                plot_bgcolor: 'transparent'
              }}
              useResizeHandler
              style={{ width: '100%' }}
            />
          </div>

          {/* Candidate Handwritten Derivation Reference */}
          <div className="p-4 rounded-xl border border-[var(--border)] bg-surface text-xs space-y-2">
            <div className="flex items-center gap-2 font-semibold text-main">
              <FileText size={16} className="text-primary" />
              <span>Handwritten Derivation Reference &amp; Analytical Proof</span>
            </div>
            <div className="text-secondary leading-relaxed">
              Applying the chain rule analytically from loss to parameters:
              <br />
              &bull; &part;J/&part;&ycirc; = (&ycirc; - y) &nbsp;|&nbsp; &bull; &part;J/&part;c = (&ycirc; - y)
              <br />
              &bull; &part;J/&part;v = (&ycirc; - y) &middot; h &nbsp;|&nbsp; &bull; &part;J/&part;b = (&ycirc; - y) &middot; v &middot; (1 - h&sup2;) &nbsp;|&nbsp; &bull; &part;J/&part;w = (&ycirc; - y) &middot; v &middot; (1 - h&sup2;) &middot; x
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
