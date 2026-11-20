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
    for (const auto& tri : mesh.boundary_faces) {
        if (tri.boundary_type == BoundaryType::Convection) conv_count++;
        if (tri.boundary_type == BoundaryType::HeatSource) heat_src_count++;
    }

    EXPECT_GT(conv_count, 0);
    EXPECT_GT(heat_src_count, 0);
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
