#include <gtest/gtest.h>
#include "core/geometry/heat_sink_geometry.hpp"

using namespace uew::geometry;

TEST(HeatSinkGeometry, DefaultParamsValid) {
    HeatSinkGeometry hs;
    auto val = hs.validate();
    EXPECT_TRUE(val.is_valid);
    EXPECT_GT(val.total_volume_m3, 0.0);
    EXPECT_GT(val.total_surface_area_m2, 0.0);
    EXPECT_GT(val.facet_count, 0);
}

TEST(HeatSinkGeometry, RejectsNegativeBaseDimensions) {
    auto p = HeatSinkParams::default_asymmetric();
    p.base_length = -0.05;
    HeatSinkGeometry hs(p);
    auto val = hs.validate();
    EXPECT_FALSE(val.is_valid);
}

TEST(HeatSinkGeometry, RejectsOverlappingFins) {
    auto p = HeatSinkParams::default_asymmetric();
    // Force fin 1 to start before fin 0 ends
    p.fins[1].y_start = p.fins[0].y_start + 0.0005; // overlap
    HeatSinkGeometry hs(p);
    auto val = hs.validate();
    EXPECT_FALSE(val.is_valid);
}

TEST(HeatSinkGeometry, RejectsHeatSourceOutOfBounds) {
    auto p = HeatSinkParams::default_asymmetric();
    p.heat_source_x_off = 0.100; // Shift far outside base plate
    HeatSinkGeometry hs(p);
    auto val = hs.validate();
    EXPECT_FALSE(val.is_valid);
}

TEST(HeatSinkGeometry, MountingHolesDetection) {
    HeatSinkGeometry hs;
    // (8, 8) mm is a mounting hole center
    EXPECT_TRUE(hs.is_in_mounting_hole(0.008, 0.008));
    // (60, 40) mm is base center, not a hole
    EXPECT_FALSE(hs.is_in_mounting_hole(0.060, 0.040));
}

TEST(HeatSinkGeometry, HeatSourceFootprintDetection) {
    HeatSinkGeometry hs;
    const auto& p = hs.params();
    double cx = p.base_length * 0.5 + p.heat_source_x_off;
    double cy = p.base_width * 0.5 + p.heat_source_y_off;

    EXPECT_TRUE(hs.is_in_heat_source(cx, cy));
    EXPECT_FALSE(hs.is_in_heat_source(0.002, 0.002));
}

TEST(HeatSinkGeometry, GeneratesValidAsciiSTL) {
    HeatSinkGeometry hs;
    std::string stl = hs.to_ascii_stl("test_heat_sink");
    EXPECT_FALSE(stl.empty());
    EXPECT_NE(stl.find("solid test_heat_sink"), std::string::npos);
    EXPECT_NE(stl.find("facet normal"), std::string::npos);
    EXPECT_NE(stl.find("endsolid test_heat_sink"), std::string::npos);
}
