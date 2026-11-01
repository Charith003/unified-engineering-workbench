#include <gtest/gtest.h>
#include "core/hardware/adapters.hpp"

using namespace uew::hardware;

class HardwareTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapters = create_mock_adapters();
    }
    std::vector<std::unique_ptr<IHardwareAdapter>> adapters;
};

// ─── Device discovery: 3 devices ─────────────────────────
TEST_F(HardwareTest, ThreeDevicesCreated) {
    EXPECT_EQ(adapters.size(), 3u);
}

// ─── Device types ─────────────────────────────────────────
TEST_F(HardwareTest, CorrectDeviceTypes) {
    bool has_mcu = false, has_fpga = false, has_jtag = false;
    for (const auto& a : adapters) {
        auto info = a->get_info();
        if (info.type == DeviceType::MCU)  has_mcu  = true;
        if (info.type == DeviceType::FPGA) has_fpga = true;
        if (info.type == DeviceType::JTAG) has_jtag = true;
    }
    EXPECT_TRUE(has_mcu);
    EXPECT_TRUE(has_fpga);
    EXPECT_TRUE(has_jtag);
}

// ─── Capabilities non-empty ───────────────────────────────
TEST_F(HardwareTest, CapabilitiesNonEmpty) {
    for (const auto& a : adapters) {
        auto info = a->get_info();
        EXPECT_FALSE(info.capabilities.empty()) << info.name;
    }
}

// ─── Simulated flag ───────────────────────────────────────
TEST_F(HardwareTest, AllSimulated) {
    for (const auto& a : adapters) {
        EXPECT_TRUE(a->get_info().simulated);
    }
}

// ─── Initial state is Disconnected ────────────────────────
TEST_F(HardwareTest, InitiallyDisconnected) {
    for (const auto& a : adapters) {
        EXPECT_EQ(a->get_info().state, DeviceState::Disconnected);
    }
}

// ─── MCU connect / disconnect ─────────────────────────────
TEST_F(HardwareTest, MCUConnectDisconnect) {
    for (auto& a : adapters) {
        if (a->get_info().type != DeviceType::MCU) continue;
        auto r = a->connect();
        EXPECT_TRUE(r.success) << r.error;
        EXPECT_EQ(a->get_info().state, DeviceState::Connected);

        auto r2 = a->disconnect();
        EXPECT_TRUE(r2.success);
        EXPECT_EQ(a->get_info().state, DeviceState::Disconnected);
    }
}

// ─── MCU telemetry ───────────────────────────────────────
TEST_F(HardwareTest, MCUTelemetryAfterConnect) {
    for (auto& a : adapters) {
        if (a->get_info().type != DeviceType::MCU) continue;
        a->connect();
        auto tm = a->read_telemetry();
        EXPECT_EQ(tm.device_id, "mcu-stm32f4");
        EXPECT_FALSE(tm.channel.empty());
        a->disconnect();
    }
}

// ─── JTAG recoverable timeout ────────────────────────────
TEST_F(HardwareTest, JTAGFirstConnectFails) {
    for (auto& a : adapters) {
        if (a->get_info().type != DeviceType::JTAG) continue;
        auto r1 = a->connect();
        EXPECT_FALSE(r1.success);   // First attempt: timeout
        EXPECT_EQ(a->get_info().state, DeviceState::Error);

        auto r2 = a->connect();     // Second attempt: success
        EXPECT_TRUE(r2.success) << r2.error;
        EXPECT_EQ(a->get_info().state, DeviceState::Connected);

        a->disconnect();
    }
}

// ─── Telemetry on disconnected device throws ─────────────
TEST_F(HardwareTest, TelemetryOnDisconnectedThrows) {
    for (auto& a : adapters) {
        if (a->get_info().type != DeviceType::MCU) continue;
        EXPECT_THROW(a->read_telemetry(), std::runtime_error);
    }
}
