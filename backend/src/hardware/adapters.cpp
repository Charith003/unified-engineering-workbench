#include "core/hardware/adapters.hpp"
#include <chrono>
#include <thread>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace uew::hardware {

// ─────────────────────────────────────────────────────────
// Shared time helper
// ─────────────────────────────────────────────────────────
static double now_s() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// ─────────────────────────────────────────────────────────
// MCU Mock Adapter
// Represents: STM32F4 Discovery (ARM Cortex-M4)
// ─────────────────────────────────────────────────────────
class MCUAdapter : public IHardwareAdapter {
    DeviceState state_{DeviceState::Disconnected};
    double connect_time_{0.0};
    int  telemetry_seq_{0};

public:
    DeviceInfo get_info() const override {
        DeviceInfo d;
        d.id          = "mcu-stm32f4";
        d.name        = "STM32F4 Discovery (Mock)";
        d.type        = DeviceType::MCU;
        d.state       = state_;
        d.description = "ARM Cortex-M4 @ 168 MHz, 1 MB Flash, 192 KB SRAM. "
                        "Simulated device — no physical hardware required.";
        d.capabilities = {"GPIO", "UART", "SPI", "I2C", "ADC-12bit", "DAC"};
        d.board_model  = "STM32F407VGT6";
        d.interface    = "USB-UART (VCP)";
        d.simulated    = true;
        return d;
    }

    HardwareResult connect() override {
        if (state_ == DeviceState::Connected)
            return {true, "MCU already connected.", ""};
        state_ = DeviceState::Connecting;
        // Simulate 120 ms USB enumeration delay
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        state_ = DeviceState::Connected;
        connect_time_ = now_s();
        return {true, "[SIM] MCU STM32F4 connected via USB-UART.", ""};
    }

    HardwareResult disconnect() override {
        state_ = DeviceState::Disconnected;
        return {true, "[SIM] MCU disconnected.", ""};
    }

    TelemetryMessage read_telemetry() override {
        if (state_ != DeviceState::Connected)
            throw std::runtime_error("MCU not connected");
        ++telemetry_seq_;
        double t = now_s() - connect_time_;
        // Simulate temperature sensor on ADC channel 0: 25 + 5*sin(t)
        double val = 25.0 + 5.0 * std::sin(t * 0.5);
        std::ostringstream hex;
        hex << "0x" << std::hex << std::uppercase
            << static_cast<uint16_t>(val * 16.0);
        return {
            "mcu-stm32f4",
            t,
            "ADC_CH0_TEMP",
            val,
            "degC",
            hex.str()
        };
    }
};

// ─────────────────────────────────────────────────────────
// FPGA Mock Adapter
// Represents: Digilent Arty A7-35T (Xilinx Artix-7)
// ─────────────────────────────────────────────────────────
class FPGAAdapter : public IHardwareAdapter {
    DeviceState state_{DeviceState::Disconnected};
    double connect_time_{0.0};
    int  telemetry_seq_{0};

public:
    DeviceInfo get_info() const override {
        DeviceInfo d;
        d.id          = "fpga-arty-a7";
        d.name        = "Digilent Arty A7-35T (Mock)";
        d.type        = DeviceType::FPGA;
        d.state       = state_;
        d.description = "Xilinx Artix-7 FPGA, 33 280 LUTs, 1.8 Mb BRAM, "
                        "DDR3L interface. Simulated device — no bitstream required.";
        d.capabilities = {"AXI4-Lite", "BRAM", "DSP48", "PCI-Express x1", "DDR3"};
        d.board_model  = "XC7A35TICSG324-1L";
        d.interface    = "Ethernet / UART";
        d.simulated    = true;
        return d;
    }

    HardwareResult connect() override {
        if (state_ == DeviceState::Connected)
            return {true, "FPGA already connected.", ""};
        state_ = DeviceState::Connecting;
        // Simulate 200 ms Ethernet ARP + TCP handshake
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        state_ = DeviceState::Connected;
        connect_time_ = now_s();
        return {true, "[SIM] FPGA Arty A7 connected via Ethernet.", ""};
    }

    HardwareResult disconnect() override {
        state_ = DeviceState::Disconnected;
        return {true, "[SIM] FPGA disconnected.", ""};
    }

    TelemetryMessage read_telemetry() override {
        if (state_ != DeviceState::Connected)
            throw std::runtime_error("FPGA not connected");
        ++telemetry_seq_;
        double t = now_s() - connect_time_;
        // Simulate FPGA fabric utilization counter (oscillates)
        double val = 35.0 + 10.0 * std::cos(t * 0.3);
        std::ostringstream hex;
        hex << "0x" << std::hex << std::uppercase << static_cast<uint32_t>(val * 100.0);
        return {
            "fpga-arty-a7",
            t,
            "FABRIC_UTIL_PCT",
            val,
            "%",
            hex.str()
        };
    }
};

// ─────────────────────────────────────────────────────────
// JTAG Mock Adapter
// Represents: Segger J-Link EDU (USB JTAG debug probe)
//
// NOTE: A JTAG adapter is a DEBUG INTERFACE, not a compute board.
// It provides CPU halt, memory read/write, and breakpoint services.
// It cannot execute arbitrary firmware; the target MCU/FPGA handles execution.
// ─────────────────────────────────────────────────────────
class JTAGAdapter : public IHardwareAdapter {
    DeviceState state_{DeviceState::Disconnected};
    double connect_time_{0.0};
    bool timeout_injected_{false};   // Used for recoverable failure scenario
    int  retry_count_{0};

public:
    DeviceInfo get_info() const override {
        DeviceInfo d;
        d.id          = "jtag-jlink-edu";
        d.name        = "Segger J-Link EDU (Mock)";
        d.type        = DeviceType::JTAG;
        d.state       = state_;
        d.description = "USB JTAG/SWD debug probe. Provides CPU halt, "
                        "memory read/write, register access, and breakpoints. "
                        "NOT a compute board — debug interface only. "
                        "Simulated device.";
        d.capabilities = {"JTAG", "SWD", "RTT", "Flash-Download", "CPU-Halt"};
        d.board_model  = "J-Link EDU Mini";
        d.interface    = "USB HID";
        d.simulated    = true;
        return d;
    }

    /// Recoverable failure scenario: first connect attempt times out,
    /// second attempt succeeds.
    HardwareResult connect() override {
        if (state_ == DeviceState::Connected)
            return {true, "JTAG adapter already connected.", ""};

        state_ = DeviceState::Connecting;

        if (!timeout_injected_ && retry_count_ == 0) {
            // Simulate connection timeout on first attempt
            ++retry_count_;
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            state_ = DeviceState::Error;
            timeout_injected_ = true;
            return {false, "",
                    "[SIM] JTAG connection timeout: target not found within 50 ms. "
                    "Check JTAG cable and target power. Retry to attempt again."};
        }

        // Second attempt succeeds
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        state_ = DeviceState::Connected;
        connect_time_ = now_s();
        timeout_injected_ = false;
        return {true,
                "[SIM] JTAG J-Link EDU connected. Target ID: 0x2BA01477 (Cortex-M4).",
                ""};
    }

    HardwareResult disconnect() override {
        state_ = DeviceState::Disconnected;
        retry_count_ = 0;
        return {true, "[SIM] JTAG adapter disconnected. Target resumed.", ""};
    }

    TelemetryMessage read_telemetry() override {
        if (state_ != DeviceState::Connected)
            throw std::runtime_error("JTAG adapter not connected");
        double t = now_s() - connect_time_;
        // Simulate PC (program counter) value — random walk
        uint32_t pc = 0x08001000 + static_cast<uint32_t>(t * 17) % 0x1000;
        std::ostringstream hex;
        hex << "0x" << std::hex << std::uppercase << pc;
        return {
            "jtag-jlink-edu",
            t,
            "TARGET_PC",
            static_cast<double>(pc),
            "address",
            hex.str()
        };
    }
};

// ─────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────
std::vector<std::unique_ptr<IHardwareAdapter>> create_mock_adapters() {
    std::vector<std::unique_ptr<IHardwareAdapter>> adapters;
    adapters.push_back(std::make_unique<MCUAdapter>());
    adapters.push_back(std::make_unique<FPGAAdapter>());
    adapters.push_back(std::make_unique<JTAGAdapter>());
    return adapters;
}

} // namespace uew::hardware
