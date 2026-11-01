# Embedded Hardware Integration Specification & Safe Integration Plan

**Module:** Hardware Integration  
**Target Systems:** Microcontroller (MCU), FPGA Development Board, JTAG Debug Adapter  
**Author:** Candidate for PASS Science Works R&D Internship 2026–2027

---

## 1. System Block Diagram & Interconnect Topology

```
                  ┌──────────────────────────────────────────────┐
                  │          Host Workstation (UEW)             │
                  └──────────────┬───────────────────────────────┘
                                 │ USB 2.0 / 3.0 Host Ports
         ┌───────────────────────┼────────────────────────┐
         │                       │                        │
         ▼ USB Virtual COM       ▼ USB / PCIe             ▼ USB
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────────┐
│ MCU Subsystem   │     │ FPGA Subsystem   │     │ JTAG Debug Adapter  │
│ STM32H753ZI     │     │ Artix-7 XC7A35T  │     │ Segger J-Link/FTDI  │
│ (ARM Cortex-M7) │     │ (Fabric + BRAM)  │     │ (Master Controller) │
└────────┬────────┘     └────────┬─────────┘     └──────────┬──────────┘
         │                       │                          │
         │   3.3V UART / SPI     │                          │
         │◄─────────────────────►│                          │
         │                       │                          │
         │     JTAG Daisy Chain: TCK / TMS (Parallel)       │
         ├───────────────────────┼──────────────────────────┤
         │ TDI                   │ TDO ──► TDI              │ TCK, TMS
         │◄──────────────────────┴──────────────────────────┴───────────
         │ TDO ─────────────────────────────────────────────► Host TDO
         │
    [Ground Plane: Low-Impedance Star Ground Network]
```

---

## 2. Voltage Domains & Level Shifting Matrix

| Subsystem | Power Domain | Nominal Voltage | Tolerances | Level Translation Needed? |
|---|---|---|---|---|
| **MCU Core** | VDD_CORE (Internal LDO) | 1.20 V | $\pm 5\%$ | Internal only |
| **MCU I/O** | VDD_MCU | 3.30 V | $3.0\text{V} - 3.6\text{V}$ | None for 3.3V peripherals; 5V tolerant on specific GPIOs |
| **FPGA Core** | VCCINT | 1.00 V | $0.95\text{V} - 1.05\text{V}$ | Direct point-of-load DC/DC buck converter |
| **FPGA Aux** | VCCAUX | 1.80 V | $1.71\text{V} - 1.89\text{V}$ | Dedicated LDO rail |
| **FPGA I/O Banks** | VCCO_0..3 | 1.80 V / 3.30 V | Configurable | Bank 14/15 set to 3.3V for direct MCU interface; Bank 16 set to 1.8V |
| **JTAG Bus** | VREF_SENSE | Dynamic ($1.2\text{V} - 5.0\text{V}$) | Matches Target | Auto-sensing bi-directional level translators on probe |

---

## 3. Detailed Signal Ownership & Daisy Chain Formulation

### JTAG TAP Controller Topology (IEEE 1149.1 / 1149.7):
- **TCK (Test Clock):** Driven solely by the JTAG Debug Adapter (Master). Distributed to both FPGA and MCU in parallel with $33\,\Omega$ series termination resistors to suppress transmission-line reflections.
- **TMS (Test Mode Select):** Driven solely by the JTAG Master. Connected in parallel to all target TAP controllers to synchronously step TAP finite state machines.
- **TDI / TDO Daisy Chaining:**
  - JTAG Adapter `TDO_HOST` $\rightarrow$ FPGA `TDI`
  - FPGA `TDO` $\rightarrow$ MCU `TDI`
  - MCU `TDO` $\rightarrow$ JTAG Adapter `TDI_HOST`
- **TRST / nSRST:** Optional active-low open-drain resets with $10\,\text{k}\Omega$ pull-up resistors to $3.3\,\text{V}$.

### Why a JTAG Adapter is a Debug Interface (NOT a Compute Target):
The JTAG adapter is a hardware protocol transceiver that translates high-level host commands (e.g., SWD, IEEE 1149.1) into bit-level boundary-scan serial bitstreams. It operates target test access ports (TAPs), manipulates instruction/data registers (IR/DR), triggers hardware breakpoints, and accesses memory-mapped peripherals.
**It contains no user application firmware runtime** and cannot execute numeric FEA, data processing, or algorithmic workloads in place of an MCU or FPGA.

---

## 4. Safe Integration & Commissioning Plan

1. **Common Ground Integrity:** Before connecting data cables, establish a low-impedance ground bonding wire between host chassis ground, MCU ground plane, and FPGA ground plane to prevent dangerous ground-loop potential differences.
2. **Power Sequencing Order:**
   - Power up Host and bench supplies.
   - Power up target boards: FPGA $1.0\,\text{V} \rightarrow 1.8\,\text{V} \rightarrow 3.3\,\text{V}$; MCU $3.3\,\text{V}$.
   - Connect JTAG probe VREF sense line to target $3.3\,\text{V}$ rail.
   - Enable JTAG clock line.
3. **Hot-Plugging Safeguards:**
   - TVS diode arrays on all exposed connector pins (IEC 61000-4-2 ESD protection).
   - $100\,\Omega$ series current-limiting resistors on test signals.
4. **Fault Recovery Architecture:**
   - The workbench simulates communication timeouts (e.g., target asleep, disconnected probe).
   - Upon timeout detection, the C++ driver attempts hardware TAP reset (5 consecutive TMS high cycles) and re-identification without crashing the application.
