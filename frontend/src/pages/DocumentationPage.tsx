import { useState } from 'react';
import {
  Code2, Cpu, Thermometer, Brain, ShieldCheck
} from 'lucide-react';

export default function DocumentationPage() {
  const [section, setSection] = useState<'arch' | 'thermal' | 'bench' | 'hw' | 'backprop' | 'tests'>('arch');

  return (
    <div className="page-container">
      <div className="page-header">
        <div>
          <h1 className="page-title">Technical Documentation &amp; Reference</h1>
          <p className="page-subtitle">
            Formal mathematical formulations, architecture specifications, verification proofs, and QA audit logs
          </p>
        </div>
      </div>

      <div className="flex gap-2 mb-6 border-b border-[var(--border)] pb-2 overflow-x-auto">
        <button
          className={`btn btn-sm ${section === 'arch' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setSection('arch')}
        >
          <Code2 size={14} /> System Architecture
        </button>
        <button
          className={`btn btn-sm ${section === 'thermal' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setSection('thermal')}
        >
          <Thermometer size={14} /> 1D Thermal FEA Theory
        </button>
        <button
          className={`btn btn-sm ${section === 'bench' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setSection('bench')}
        >
          <Cpu size={14} /> Multicore Benchmarking
        </button>
        <button
          className={`btn btn-sm ${section === 'hw' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setSection('hw')}
        >
          <Cpu size={14} /> Embedded Integration
        </button>
        <button
          className={`btn btn-sm ${section === 'backprop' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setSection('backprop')}
        >
          <Brain size={14} /> Backprop &amp; Handwritten Proof
        </button>
        <button
          className={`btn btn-sm ${section === 'tests' ? 'btn-primary' : 'btn-secondary'}`}
          onClick={() => setSection('tests')}
        >
          <ShieldCheck size={14} /> QA Test Suite
        </button>
      </div>

      {section === 'arch' && (
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-3">Modular C++20 Core &amp; Web Presentation Architecture</h2>
            <p className="text-xs text-secondary leading-relaxed mb-4">
              UEW enforces a strict physical separation between engineering calculation and interactive presentation.
              All numerical linear algebra, mesh parsing, parallel scheduling, and backpropagation are written in C++17/20.
              The React frontend handles rendering (Three.js WebGL, Plotly) and user input without performing any engineering calculations in JavaScript.
            </p>
            <div className="bg-[var(--bg-main)] p-4 rounded-xl border border-[var(--border)] font-mono text-xs text-main leading-loose">
              <div>&bull; <strong>Frontend:</strong> React 19, TypeScript, Tailwind CSS, Three.js, Plotly.js, Lucide</div>
              <div>&bull; <strong>Transport:</strong> Localhost HTTP REST via cpp-httplib, nlohmann/json, loopback only</div>
              <div>&bull; <strong>Backend Core:</strong> C++17/20, std::thread worker pool, Thomas tridiagonal algorithm</div>
              <div>&bull; <strong>Automated QA:</strong> GoogleTest (gtest) covering all 6 functional domains</div>
              <div>&bull; <strong>Packaging:</strong> Native standalone Launch.exe with zero runtime Node.js dependency</div>
            </div>
          </div>
        </div>
      )}

      {section === 'thermal' && (
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-3">1D Steady-State Thermal Finite Element Formulation</h2>
            <div className="text-xs text-secondary space-y-3 leading-relaxed">
              <p>
                <strong>Governing Differential Equation:</strong>
                <br />
                <code className="font-mono text-main bg-[var(--bg-main)] px-2 py-1 rounded border border-[var(--border)] inline-block my-1">
                  -d/dx ( k &middot; A &middot; dT/dx ) = q &middot; A
                </code>
              </p>
              <p>
                <strong>Element Stiffness Matrix (2-Node Linear Element):</strong>
                <br />
                For uniform mesh spacing h = L / N:
                <br />
                <code className="font-mono text-main bg-[var(--bg-main)] px-2 py-1 rounded border border-[var(--border)] inline-block my-1">
                  K_e = (k &middot; A / h) &times; [[ 1, -1 ], [ -1, 1 ]]
                </code>
              </p>
              <p>
                <strong>Element Load Vector (Uniform Heat Generation q):</strong>
                <br />
                <code className="font-mono text-main bg-[var(--bg-main)] px-2 py-1 rounded border border-[var(--border)] inline-block my-1">
                  F_e = (q &middot; A &middot; h / 2) &times; [ 1, 1 ]^T
                </code>
              </p>
              <p>
                <strong>Analytical Exact Solution:</strong>
                <br />
                With T(0) = 100 °C and T(L) = 20 °C:
                <br />
                <code className="font-mono text-main bg-[var(--bg-main)] px-2 py-1 rounded border border-[var(--border)] inline-block my-1">
                  T_exact(x) = 100 - (80 &middot; x / L) + (q / 2k) &middot; x &middot; (L - x)
                </code>
              </p>
              <p>
                <strong>Critical Debugging Insight:</strong>
                <br />
                Linear finite elements yield exact nodal temperatures for 1D steady-state heat conduction with constant source terms due to the Green's function property of the differential operator.
                Therefore, convergence and discretization error MUST be evaluated at <em>element midpoints</em> using linear shape function interpolation, where the O(h&sup2;) error is correctly captured.
              </p>
            </div>
          </div>
        </div>
      )}

      {section === 'bench' && (
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-3">Multicore Performance Benchmarking Methodology</h2>
            <div className="text-xs text-secondary space-y-3 leading-relaxed">
              <p>
                <strong>Thread Pool Architecture:</strong>
                <br />
                Bounded C++ worker pool using <code className="font-mono text-main">std::thread</code>, <code className="font-mono text-main">std::mutex</code>, and <code className="font-mono text-main">std::condition_variable</code>. Tasks are distributed across worker queues without thread destruction/creation overhead.
              </p>
              <p>
                <strong>Metrics &amp; Strict Formulations:</strong>
                <br />
                &bull; Speedup: <code className="font-mono text-main">S = T_1 / T_p</code>
                <br />
                &bull; Parallel Efficiency: <code className="font-mono text-main">E = (T_1 / T_p) / p</code>
                <br />
                &bull; Timing: Evaluated across 1 warm-up run + 5 measured repetitions, recording the median runtime.
              </p>
              <p>
                <strong>Deterministic Agreement Criterion:</strong>
                <br />
                Serial and parallel solvers run identical deterministic batches generated from fixed seed 0x5EED.
                Every parallel run is verified to match serial results within <code className="font-mono text-main">&le; 1e-8 °C</code> maximum absolute difference.
              </p>
            </div>
          </div>
        </div>
      )}

      {section === 'hw' && (
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-3">Embedded-System Integration Design</h2>
            <div className="text-xs text-secondary space-y-3 leading-relaxed">
              <p>
                <strong>Simulated Hardware Devices:</strong>
                <br />
                1. <strong>Microcontroller:</strong> STM32H753 (ARM Cortex-M7 @ 480 MHz), 3.3V I/O domain.
                <br />
                2. <strong>FPGA:</strong> Xilinx Artix-7 XC7A35T, 1.0V core, 1.8V/3.3V multi-bank I/O.
                <br />
                3. <strong>JTAG Adapter:</strong> FTDI FT2232H / Segger J-Link debug adapter.
              </p>
              <p>
                <strong>Debug vs Compute Distinction:</strong>
                <br />
                The JTAG adapter is strictly a hardware debug interface operating state machines on TCK/TMS to access internal TAP registers. It cannot execute application firmware or substitute for a compute target.
              </p>
              <p>
                <strong>Recoverable Failure Handling:</strong>
                <br />
                The system models communication timeouts and cable disconnects, providing retry logic and state machine reset without crashing the host workbench.
              </p>
            </div>
          </div>
        </div>
      )}

      {section === 'backprop' && (
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-3">Exact Handwritten Gradient Derivation Reference</h2>
            <div className="text-xs text-secondary space-y-3 leading-relaxed">
              <p>
                <strong>Scalar Architecture:</strong>
                <br />
                h = tanh(w &middot; x + b) &nbsp;|&nbsp; &ycirc; = v &middot; h + c &nbsp;|&nbsp; J = &frac12; (&ycirc; - y)&sup2;
              </p>
              <p>
                <strong>Exact Partial Derivatives:</strong>
                <br />
                &bull; &part;J/&part;&ycirc; = (&ycirc; - y)
                <br />
                &bull; &part;J/&part;c = &part;J/&part;&ycirc; &middot; 1 = (&ycirc; - y)
                <br />
                &bull; &part;J/&part;v = &part;J/&part;&ycirc; &middot; h = (&ycirc; - y) &middot; h
                <br />
                &bull; &part;J/&part;h = &part;J/&part;&ycirc; &middot; v = (&ycirc; - y) &middot; v
                <br />
                &bull; dh/dz = 1 - tanh&sup2;(z) = 1 - h&sup2;, where z = wx + b
                <br />
                &bull; &part;J/&part;b = &part;J/&part;h &middot; (1 - h&sup2;) &middot; 1 = (&ycirc; - y) &middot; v &middot; (1 - h&sup2;)
                <br />
                &bull; &part;J/&part;w = &part;J/&part;h &middot; (1 - h&sup2;) &middot; x = (&ycirc; - y) &middot; v &middot; (1 - h&sup2;) &middot; x
              </p>
              <p>
                <strong>Numerical Verification:</strong>
                <br />
                Every analytical gradient is independently verified against central differences <code className="font-mono text-main">[J(&theta;+&epsilon;) - J(&theta;-&epsilon;)] / 2&epsilon;</code> with &epsilon; = 1e-6.
                Pass criteria: absolute error &le; 1e-5.
              </p>
            </div>
          </div>
        </div>
      )}

      {section === 'tests' && (
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-3">GoogleTest Verification Suite Summary</h2>
            <div className="text-xs text-secondary space-y-3 leading-relaxed">
              <p>
                The complete test suite verifies all mathematical solvers, parsers, and mock adapters:
              </p>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                  <span className="font-semibold text-main block mb-1">Geometry &amp; Parsers</span>
                  <ul className="list-disc list-inside space-y-0.5">
                    <li>Valid ASCII STL loading &amp; triangle extraction</li>
                    <li>Malformed / empty STL syntax rejection</li>
                    <li>CSV node coordinate &amp; temperature parsing</li>
                    <li>Non-finite value (NaN / Inf) rejection</li>
                  </ul>
                </div>
                <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                  <span className="font-semibold text-main block mb-1">Thermal FEA</span>
                  <ul className="list-disc list-inside space-y-0.5">
                    <li>Dirichlet boundary temperature enforcement</li>
                    <li>Analytical vs FE nodal agreement</li>
                    <li>Midpoint error O(h&sup2;) convergence check</li>
                    <li>Thomas algorithm numerical stability</li>
                  </ul>
                </div>
                <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                  <span className="font-semibold text-main block mb-1">Multicore Performance</span>
                  <ul className="list-disc list-inside space-y-0.5">
                    <li>Fixed-seed batch reproducibility</li>
                    <li>Serial vs parallel agreement (&le; 1e-8 °C)</li>
                    <li>Deterministic result ordering</li>
                    <li>Worker pool thread safety &amp; cancellation</li>
                  </ul>
                </div>
                <div className="p-3 bg-[var(--bg-main)] rounded-lg border border-[var(--border)]">
                  <span className="font-semibold text-main block mb-1">Neural Backpropagation</span>
                  <ul className="list-disc list-inside space-y-0.5">
                    <li>Forward activation pass accuracy</li>
                    <li>Analytical vs central FD gradient checks</li>
                    <li>Simultaneous 4-parameter update</li>
                    <li>100-step monotonic loss reduction</li>
                  </ul>
                </div>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
