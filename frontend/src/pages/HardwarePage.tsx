import { useState, useEffect } from 'react';
import {
  Cpu, Activity, RefreshCw, Power, AlertTriangle,
  HelpCircle, Zap, Radio
} from 'lucide-react';
import {
  getDevices, connectDevice, disconnectDevice, readTelemetry
} from '../services/api';
import type { DeviceInfo, TelemetryReading } from '../types/api';

export default function HardwarePage() {
  const [devices, setDevices] = useState<DeviceInfo[]>([]);
  const [telemetry, setTelemetry] = useState<Record<string, TelemetryReading>>({});
  const [actionLoading, setActionLoading] = useState<string | null>(null);
  const [errorNotice, setErrorNotice] = useState<string | null>(null);
  const [activeTab, setActiveTab] = useState<'status' | 'docs'>('status');

  const fetchDevices = async () => {
    try {
      const resp = await getDevices();
      if (resp.success && resp.data) {
        setDevices(resp.data.devices);
      }
    } catch (err: unknown) {
      console.error(err);
    }
  };

  useEffect(() => {
    fetchDevices();
    const interval = setInterval(async () => {
      setDevices(prev => {
        prev.forEach(async dev => {
          if (dev.state === 'connected') {
            try {
              const tel = await readTelemetry(dev.id);
              if (tel.success && tel.data) {
                setTelemetry(t => ({ ...t, [dev.id]: tel.data as TelemetryReading }));
              }
            } catch {
              // ignore transient telemetry fetch errors
            }
          }
        });
        return prev;
      });
    }, 1500);

    return () => clearInterval(interval);
  }, []);

  const handleConnectToggle = async (dev: DeviceInfo) => {
    setActionLoading(dev.id);
    setErrorNotice(null);
    try {
      if (dev.state === 'connected') {
        const res = await disconnectDevice(dev.id);
        if (res.success) {
          setDevices(prev => prev.map(d => d.id === dev.id ? { ...d, state: 'disconnected' } : d));
        } else {
          setErrorNotice(res.error || 'Failed to disconnect');
        }
      } else {
        const res = await connectDevice(dev.id);
        if (res.success) {
          setDevices(prev => prev.map(d => d.id === dev.id ? { ...d, state: 'connected' } : d));
        } else {
          // Recoverable failure scenario: simulated link timeout
          setErrorNotice(res.error || `Connection to ${dev.name} failed: Simulated link timeout. Retry available.`);
          setDevices(prev => prev.map(d => d.id === dev.id ? { ...d, state: 'error' } : d));
        }
      }
    } catch (err: unknown) {
      setErrorNotice(err instanceof Error ? err.message : 'Device communication error');
    } finally {
      setActionLoading(null);
    }
  };

  return (
    <div className="page-container">
      <div className="page-header">
        <div>
          <h1 className="page-title">Hardware Integration</h1>
          <p className="page-subtitle">
            Embedded-system integration simulation: MCU, FPGA, and JTAG Debug Adapter with telemetry and failure recovery
          </p>
        </div>
        <div className="flex gap-2">
          <button
            className={`btn btn-sm ${activeTab === 'status' ? 'btn-primary' : 'btn-secondary'}`}
            onClick={() => setActiveTab('status')}
          >
            <Cpu size={14} /> Device Control
          </button>
          <button
            className={`btn btn-sm ${activeTab === 'docs' ? 'btn-primary' : 'btn-secondary'}`}
            onClick={() => setActiveTab('docs')}
          >
            <HelpCircle size={14} /> Architecture &amp; Schematics
          </button>
          <button className="btn btn-secondary btn-sm" onClick={fetchDevices}>
            <RefreshCw size={14} />
          </button>
        </div>
      </div>

      {/* Simulation Banner Notice */}
      <div className="alert alert-info mb-6">
        <Radio size={18} />
        <div>
          <span className="font-semibold text-xs uppercase tracking-wider block">Explicit Simulation Notice</span>
          <span className="text-xs">
            All devices below operate via C++ mock adapters running safe in-process hardware emulation. No physical USB or JTAG hardware is connected.
          </span>
        </div>
      </div>

      {errorNotice && (
        <div className="alert alert-error mb-6 flex items-start justify-between">
          <div className="flex items-start gap-2">
            <AlertTriangle size={18} className="mt-0.5" />
            <div>
              <span className="font-semibold block text-sm">Recoverable Hardware Communication Event</span>
              <span className="text-xs">{errorNotice}</span>
            </div>
          </div>
          <button className="btn btn-secondary btn-sm text-xs" onClick={() => setErrorNotice(null)}>
            Dismiss
          </button>
        </div>
      )}

      {activeTab === 'status' ? (
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          {devices.map(dev => {
            const tel = telemetry[dev.id];
            const isConnected = dev.state === 'connected';
            const isJtag = dev.type === 'JTAG';
            return (
              <div
                key={dev.id}
                className="card flex flex-col justify-between border-t-4"
                style={{
                  borderTopColor: isConnected ? '#10B981' : dev.state === 'error' ? '#EF4444' : '#E2E8F0'
                }}
              >
                <div>
                  <div className="flex items-center justify-between mb-2">
                    <span className="badge badge-secondary uppercase tracking-wider text-[10px]">
                      {dev.type}
                    </span>
                    <span className={`inline-flex items-center gap-1.5 text-xs font-semibold ${isConnected ? 'text-emerald-600' : dev.state === 'error' ? 'text-red-600' : 'text-slate-400'}`}>
                      <span className={`w-2 h-2 rounded-full ${isConnected ? 'bg-emerald-500 animate-pulse' : dev.state === 'error' ? 'bg-red-500' : 'bg-slate-300'}`} />
                      {dev.state.toUpperCase()}
                    </span>
                  </div>

                  <h3 className="font-bold text-main text-base">{dev.name}</h3>
                  <p className="text-xs text-secondary mb-3">{dev.description}</p>

                  <div className="space-y-1.5 text-xs border-t border-b border-[var(--border)] py-3 my-3">
                    <div className="flex justify-between">
                      <span className="text-secondary">Board Model</span>
                      <span className="font-mono text-main">{dev.board_model}</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-secondary">Host Interface</span>
                      <span className="font-mono text-main">{dev.interface}</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-secondary">Capabilities</span>
                      <span className="font-mono text-[11px] text-main">{dev.capabilities.join(', ')}</span>
                    </div>
                  </div>

                  {/* Telemetry Display */}
                  {isConnected && tel && (
                    <div className="bg-[var(--bg-main)] p-3 rounded-lg border border-[var(--border)] mb-4">
                      <span className="text-[11px] font-semibold text-secondary uppercase tracking-wider block mb-2 flex items-center gap-1">
                        <Activity size={12} className="text-primary" /> Live Telemetry
                      </span>
                      <div className="grid grid-cols-2 gap-2 text-xs">
                        <div>
                          <span className="text-[10px] text-secondary block">Channel</span>
                          <span className="font-mono font-semibold">{tel.channel}</span>
                        </div>
                        <div>
                          <span className="text-[10px] text-secondary block">Reading</span>
                          <span className="font-mono font-semibold">{tel.value.toFixed(2)} {tel.unit}</span>
                        </div>
                        <div className="col-span-2">
                          <span className="text-[10px] text-secondary block">Raw Hex</span>
                          <span className="font-mono text-[10px] font-semibold">{tel.raw_hex}</span>
                        </div>
                      </div>
                    </div>
                  )}

                  {isJtag && (
                    <div className="p-2.5 bg-amber-50 rounded border border-amber-200 text-amber-900 text-[11px] mb-3">
                      <strong>Note:</strong> Debug interface adapter. Does not execute user code or replace target processing.
                    </div>
                  )}
                </div>

                <div className="pt-2">
                  <button
                    className={`btn w-full justify-center ${isConnected ? 'btn-secondary text-red-600 hover:bg-red-50' : 'btn-primary'}`}
                    onClick={() => handleConnectToggle(dev)}
                    disabled={actionLoading === dev.id}
                  >
                    {actionLoading === dev.id ? (
                      <Activity size={14} className="animate-spin" />
                    ) : isConnected ? (
                      <>
                        <Power size={14} /> Disconnect
                      </>
                    ) : (
                      <>
                        <Zap size={14} /> Connect &amp; Initialize
                      </>
                    )}
                  </button>
                </div>
              </div>
            );
          })}
        </div>
      ) : (
        /* Architecture & Schematics Documentation Tab */
        <div className="space-y-6">
          <div className="card">
            <h2 className="card-title mb-4">Embedded Hardware Integration Architecture</h2>
            
            {/* Voltage Domain Table */}
            <h3 className="font-semibold text-sm text-main mb-2">1. Voltage Domains &amp; Level Shifting</h3>
            <table className="table-custom mb-6">
              <thead>
                <tr>
                  <th>Device</th>
                  <th>Core Logic</th>
                  <th>I/O Domain</th>
                  <th>Host Connection</th>
                  <th>Level Shifting Required?</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td className="font-medium">MCU (STM32H753)</td>
                  <td>1.2V LDO</td>
                  <td>3.3V LVCMOS</td>
                  <td>USB 2.0 FS (5V bus / 3.3V phy)</td>
                  <td>None for 3.3V SPI/UART; 5V tolerant pins on GPIO</td>
                </tr>
                <tr>
                  <td className="font-medium">FPGA (Artix-7)</td>
                  <td>1.0V VCCINT</td>
                  <td>1.8V / 2.5V / 3.3V (Bank-dependent)</td>
                  <td>PCIe / High-Speed USB FX3</td>
                  <td>Active bi-directional level shifters (TXS0108E) required for 3.3V MCU bus</td>
                </tr>
                <tr>
                  <td className="font-medium">JTAG Adapter (J-Link / FTDI)</td>
                  <td>3.3V Internal</td>
                  <td>VREF Sense (1.2V – 5.0V dynamic)</td>
                  <td>USB 2.0 High-Speed</td>
                  <td>Internal level translation buffers referenced to target VDD_SENSE</td>
                </tr>
              </tbody>
            </table>

            {/* JTAG Ownership */}
            <h3 className="font-semibold text-sm text-main mb-2">2. JTAG Signal Ownership &amp; Daisy-Chaining</h3>
            <div className="p-4 bg-[var(--bg-main)] rounded-lg border border-[var(--border)] text-xs space-y-2 mb-6">
              <p>
                <strong>TCK / TMS:</strong> Driven exclusively by the JTAG master adapter (active push-pull). Connected in parallel to both FPGA and MCU TAP controllers.
              </p>
              <p>
                <strong>TDI / TDO Daisy Chain:</strong> Host TDI &rarr; FPGA TDI &rarr; FPGA TDO &rarr; MCU TDI &rarr; MCU TDO &rarr; Host TDO.
              </p>
              <p>
                <strong>Why JTAG is NOT a compute board:</strong> The JTAG adapter is a serial state-machine test controller conforming to IEEE 1149.1 / 1149.7. It provides boundary scan, memory write/read, and processor halt/resume capabilities. It contains no user application firmware runtime.
              </p>
            </div>

            {/* Safe Integration Plan */}
            <h3 className="font-semibold text-sm text-main mb-2">3. Safe Integration &amp; Power Sequencing Plan</h3>
            <ol className="list-decimal list-inside text-xs space-y-1.5 text-secondary">
              <li><strong>Common Ground Reference:</strong> Connect low-impedance ground planes of Host, MCU, and FPGA before applying power to eliminate ground loop offsets.</li>
              <li><strong>Power-Up Sequence:</strong> Power 3.3V rails first, followed by FPGA 1.8V aux, then 1.0V core rail to prevent latch-up.</li>
              <li><strong>JTAG Hot-Plugging Protection:</strong> Clamp diodes and series 100&Omega; current-limiting resistors on TCK, TMS, TDI lines.</li>
            </ol>
          </div>
        </div>
      )}
    </div>
  );
}
