#pragma once
#include <string>
#include <vector>
#include <memory>

namespace uew::hardware {

enum class DeviceType { MCU, FPGA, JTAG };
enum class DeviceState { Disconnected, Connecting, Connected, Error };

struct DeviceInfo {
    std::string id;
    std::string name;
    DeviceType  type;
    DeviceState state{DeviceState::Disconnected};
    std::string description;
    std::vector<std::string> capabilities;
    std::string board_model;
    std::string interface;
    bool simulated{true};
};

struct TelemetryMessage {
    std::string device_id;
    double      timestamp_s;
    std::string channel;
    double      value;
    std::string unit;
    std::string raw_hex;
};

struct HardwareResult {
    bool success{false};
    std::string message;
    std::string error;
};

/// Abstract hardware adapter interface (RAII, no raw pointers in API)
class IHardwareAdapter {
public:
    virtual ~IHardwareAdapter() = default;
    virtual DeviceInfo   get_info() const = 0;
    virtual HardwareResult connect() = 0;
    virtual HardwareResult disconnect() = 0;
    virtual TelemetryMessage read_telemetry() = 0;
};

/// Factory: returns all three mock adapters
std::vector<std::unique_ptr<IHardwareAdapter>> create_mock_adapters();

} // namespace uew::hardware
