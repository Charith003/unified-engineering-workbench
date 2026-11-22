#include <gtest/gtest.h>
#include "core/mesh/mesh_generator.hpp"
#include "core/mesh/mesh_validator.hpp"
#include "core/geometry/heat_sink_geometry.hpp"

using namespace uew::mesh;
using namespace uew::geometry;

TEST(MeshGenerator, CoarseMeshGeneration) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    EXPECT_GT(mesh.nodes.size(), 100);
    EXPECT_GT(mesh.elements.size(), 200);
    EXPECT_GT(mesh.boundary_faces.size(), 100);

    auto result = MeshValidator::validate(mesh);
    EXPECT_TRUE(result.is_valid);
    EXPECT_EQ(result.inverted_elements, 0);
    EXPECT_EQ(result.degenerate_elements, 0);
    EXPECT_GT(result.min_volume, 0.0);
    EXPECT_GT(result.mean_quality, 0.30);
    EXPECT_GT(result.min_quality, 0.15);
}

TEST(MeshGenerator, MediumMeshGeneration) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Medium;
    auto mesh = gen.generate(opts);

    auto result = MeshValidator::validate(mesh);
    EXPECT_TRUE(result.is_valid);
    EXPECT_EQ(result.inverted_elements, 0);
    EXPECT_EQ(result.degenerate_elements, 0);
    EXPECT_GT(result.min_volume, 0.0);
    EXPECT_GT(result.mean_quality, 0.40);
}

TEST(MeshGenerator, MeshMonotonicity) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts_coarse;
    opts_coarse.resolution = MeshResolution::Coarse;
    auto coarse = gen.generate(opts_coarse);

    MeshGenerationOptions opts_medium;
    opts_medium.resolution = MeshResolution::Medium;
    auto medium = gen.generate(opts_medium);

    EXPECT_LT(coarse.nodes.size(), medium.nodes.size());
    EXPECT_LT(coarse.elements.size(), medium.elements.size());
}

TEST(MeshGenerator, BoundaryConditionTagging) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    int conv_count = 0;
    int heat_src_count = 0;
    double tb = geom.params().base_thickness;

    for (const auto& tri : mesh.boundary_faces) {
        if (tri.boundary_type == BoundaryType::Convection) conv_count++;
        if (tri.boundary_type == BoundaryType::HeatSource) {
            heat_src_count++;
            // Priority 2 Regression Check: Heat source faces MUST be on the top surface of the base plate (z = tb)
            const auto& p0 = mesh.nodes[tri.nodes[0]];
            const auto& p1 = mesh.nodes[tri.nodes[1]];
            const auto& p2 = mesh.nodes[tri.nodes[2]];
            double avg_z = (p0.z + p1.z + p2.z) / 3.0;
            EXPECT_NEAR(avg_z, tb, 1e-4);
            EXPECT_GT(tri.normal[2], 0.5); // Outward normal pointing +Z
        }
    }

    EXPECT_GT(conv_count, 0);
    EXPECT_GT(heat_src_count, 0);
}

TEST(MeshGenerator, HeatSourceTopSurfaceLocationAndArea) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Medium;
    auto mesh = gen.generate(opts);

    double tb = geom.params().base_thickness;
    double heat_src_area = 0.0;
    int heat_src_faces = 0;

    for (const auto& tri : mesh.boundary_faces) {
        if (tri.boundary_type == BoundaryType::HeatSource) {
            heat_src_faces++;
            heat_src_area += tri.area;

            // Confirm every heat source face is at base top plate z = tb
            for (int k = 0; k < 3; ++k) {
                EXPECT_NEAR(mesh.nodes[tri.nodes[k]].z, tb, 1e-4);
            }
        }
    }

    EXPECT_GT(heat_src_faces, 0);
    EXPECT_GT(heat_src_area, 0.0);
    // Area must be bounded by the nominal 30 x 25 mm footprint (0.00075 m^2)
    EXPECT_LE(heat_src_area, geom.params().heat_source_length * geom.params().heat_source_width + 1e-6);
}

TEST(MeshGenerator, BottomFaceHasConvection) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    int bottom_conv_count = 0;
    for (const auto& tri : mesh.boundary_faces) {
        const auto& p0 = mesh.nodes[tri.nodes[0]];
        const auto& p1 = mesh.nodes[tri.nodes[1]];
        const auto& p2 = mesh.nodes[tri.nodes[2]];
        double avg_z = (p0.z + p1.z + p2.z) / 3.0;

        // Bottom face at z = 0 with normal -Z
        if (avg_z <= 1e-5 && tri.normal[2] < -0.5) {
            // Must be convection (or fixed temp if mounting hole clamped)
            EXPECT_TRUE(tri.boundary_type == BoundaryType::Convection ||
                        tri.boundary_type == BoundaryType::FixedTemp);
            if (tri.boundary_type == BoundaryType::Convection) {
                bottom_conv_count++;
            }
        }
    }
    EXPECT_GT(bottom_conv_count, 0);
}

TEST(MeshValidator, DetectsCorruptedMesh) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    // Corrupt an element by creating negative volume (swapping two vertices)
    if (!mesh.elements.empty()) {
        std::swap(mesh.elements[0].nodes[0], mesh.elements[0].nodes[1]);
    }

    auto result = MeshValidator::validate(mesh);
    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.inverted_elements, 0);
}

TEST(MeshValidator, DetectsOutOfRangeNodeIndex) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    // Corrupt an element by assigning out-of-range node index
    if (!mesh.elements.empty()) {
        mesh.elements[0].nodes[0] = 999999;
    }

    auto result = MeshValidator::validate(mesh);
    EXPECT_FALSE(result.is_valid);
}

TEST(MeshValidator, DetectsNonFiniteCoordinates) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    // Corrupt a node by injecting NaN
    if (!mesh.nodes.empty()) {
        mesh.nodes[0].x = std::numeric_limits<double>::quiet_NaN();
    }

    auto result = MeshValidator::validate(mesh);
    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.non_finite_nodes, 0);
}

TEST(MeshValidator, DetectsDegenerateDuplicateNodesInTet) {
    HeatSinkGeometry geom;
    MeshGenerator gen(geom);
    MeshGenerationOptions opts;
    opts.resolution = MeshResolution::Coarse;
    auto mesh = gen.generate(opts);

    // Corrupt an element with duplicate nodes (zero volume)
    if (!mesh.elements.empty()) {
        mesh.elements[0].nodes[1] = mesh.elements[0].nodes[0];
    }

    auto result = MeshValidator::validate(mesh);
    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.degenerate_elements, 0);
}
