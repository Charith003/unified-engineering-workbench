#include "core/mesh/mesh_generator.hpp"
#include "core/mesh/mesh_validator.hpp"
#include <chrono>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>

namespace uew::mesh {

std::string MeshGenerator::resolution_to_string(MeshResolution res) {
    switch (res) {
        case MeshResolution::Coarse: return "Coarse";
        case MeshResolution::Medium: return "Medium";
        case MeshResolution::Fine:   return "Fine";
    }
    return "Medium";
}

MeshGenerator::MeshGenerator(const geometry::HeatSinkGeometry& geometry)
    : geometry_(geometry) {}

TetrahedralMesh MeshGenerator::generate(const MeshGenerationOptions& options) {
    auto start_time = std::chrono::high_resolution_clock::now();
    TetrahedralMesh mesh;

    const auto& gp = geometry_.params();
    const double L  = gp.base_length;
    const double W  = gp.base_width;
    const double tb = gp.base_thickness;

    // ── 1. Determine Discretization Grid Density ─────────────
    int nx = 12;
    int nz_base = 2;
    int nz_fin = 3;
    if (options.resolution == MeshResolution::Coarse) {
        nx = 10;
        nz_base = 2;
        nz_fin = 2;
    } else if (options.resolution == MeshResolution::Medium) {
        nx = 18;
        nz_base = 3;
        nz_fin = 4;
    } else if (options.resolution == MeshResolution::Fine) {
        nx = 26;
        nz_base = 4;
        nz_fin = 6;
    }

    // Build unique monotonic coordinate partitions along X, Y, Z
    std::vector<double> x_coords;
    for (int i = 0; i <= nx; ++i) {
        x_coords.push_back((L * i) / nx);
    }

    // Y coordinates: must align with all fin boundaries
    std::set<double> y_set;
    y_set.insert(0.0);
    y_set.insert(W);
    for (const auto& f : gp.fins) {
        y_set.insert(f.y_start);
        y_set.insert(f.y_start + f.thickness);
    }
    // Subdivide gaps if needed for finer resolutions
    std::vector<double> y_coords_raw(y_set.begin(), y_set.end());
    std::vector<double> y_coords;
    for (size_t i = 0; i + 1 < y_coords_raw.size(); ++i) {
        double y0 = y_coords_raw[i];
        double y1 = y_coords_raw[i+1];
        double dy = y1 - y0;
        int sub = 1;
        if (options.resolution == MeshResolution::Fine && dy > 0.006) {
            sub = 2;
        }
        for (int s = 0; s < sub; ++s) {
            y_coords.push_back(y0 + (dy * s) / sub);
        }
    }
    y_coords.push_back(W);

    // Z coordinates for base
    std::vector<double> z_base;
    for (int i = 0; i <= nz_base; ++i) {
        z_base.push_back((tb * i) / nz_base);
    }

    // ── 2. Grid-to-Node Mapping (Spatial Key Hash) ───────────
    auto coord_key = [](double x, double y, double z) {
        long long ix = static_cast<long long>(std::round(x * 1000000.0));
        long long iy = static_cast<long long>(std::round(y * 1000000.0));
        long long iz = static_cast<long long>(std::round(z * 1000000.0));
        return std::make_tuple(ix, iy, iz);
    };

    std::map<std::tuple<long long, long long, long long>, int> node_lookup;
    auto get_or_add_node = [&](double x, double y, double z) -> int {
        auto key = coord_key(x, y, z);
        auto it = node_lookup.find(key);
        if (it != node_lookup.end()) {
            return it->second;
        }
        int new_id = static_cast<int>(mesh.nodes.size());
        Node3D n;
        n.id = new_id;
        n.x  = x;
        n.y  = y;
        n.z  = z;
        mesh.nodes.push_back(n);
        node_lookup[key] = new_id;
        return new_id;
    };

    // ── 3. Decompose Hexahedral Brick into 5 Conforming Tets ─
    auto push_tet = [&](int a, int b, int c, int d) {
        const auto& na = mesh.nodes[a];
        const auto& nb = mesh.nodes[b];
        const auto& nc = mesh.nodes[c];
        const auto& nd = mesh.nodes[d];
        if (MeshValidator::compute_tet_volume(na, nb, nc, nd) < 0.0) {
            std::swap(b, c); // Ensure positive orientation
        }
        mesh.elements.push_back({ static_cast<int>(mesh.elements.size()), {a, b, c, d}, 0.0, 0.0 });
    };

    auto add_brick_tets = [&](int n0, int n1, int n2, int n3,
                              int n4, int n5, int n6, int n7,
                              bool alternate) {
        if (!alternate) {
            // Even decomposition: (0,1,3,4), (1,2,3,6), (1,4,5,6), (3,4,6,7), (1,3,4,6)
            push_tet(n0, n1, n3, n4);
            push_tet(n1, n2, n3, n6);
            push_tet(n1, n4, n5, n6);
            push_tet(n3, n4, n6, n7);
            push_tet(n1, n3, n4, n6);
        } else {
            // Odd decomposition: (0,1,2,5), (0,2,3,7), (0,4,5,7), (2,5,6,7), (0,5,2,7)
            push_tet(n0, n1, n2, n5);
            push_tet(n0, n2, n3, n7);
            push_tet(n0, n4, n5, n7);
            push_tet(n2, n5, n6, n7);
            push_tet(n0, n5, n2, n7);
        }
    };

    // ── 4. Mesh the Base Plate ──────────────────────────────
    int ny_cells = static_cast<int>(y_coords.size()) - 1;
    for (int ix = 0; ix < nx; ++ix) {
        double x0 = x_coords[ix];
        double x1 = x_coords[ix+1];
        double xc = (x0 + x1) * 0.5;

        for (int iy = 0; iy < ny_cells; ++iy) {
            double y0 = y_coords[iy];
            double y1 = y_coords[iy+1];
            double yc = (y0 + y1) * 0.5;

            // Optional: Skip cells inside mounting holes
            if (options.exclude_mounting_holes && geometry_.is_in_mounting_hole(xc, yc)) {
                continue;
            }

            for (int iz = 0; iz < nz_base; ++iz) {
                double z0 = z_base[iz];
                double z1 = z_base[iz+1];

                int n0 = get_or_add_node(x0, y0, z0);
                int n1 = get_or_add_node(x1, y0, z0);
                int n2 = get_or_add_node(x1, y1, z0);
                int n3 = get_or_add_node(x0, y1, z0);
                int n4 = get_or_add_node(x0, y0, z1);
                int n5 = get_or_add_node(x1, y0, z1);
                int n6 = get_or_add_node(x1, y1, z1);
                int n7 = get_or_add_node(x0, y1, z1);

                bool parity = ((ix + iy + iz) % 2) != 0;
                add_brick_tets(n0, n1, n2, n3, n4, n5, n6, n7, parity);
            }
        }
    }

    // ── 5. Mesh the Asymmetric Vertical Cooling Fins ─────────
    for (size_t f_idx = 0; f_idx < gp.fins.size(); ++f_idx) {
        const auto& fin = gp.fins[f_idx];
        double y0 = fin.y_start;
        double y1 = fin.y_start + fin.thickness;

        // Discretize fin height into layers
        std::vector<double> z_fin;
        for (int iz = 0; iz <= nz_fin; ++iz) {
            z_fin.push_back(tb + (fin.height * iz) / nz_fin);
        }

        for (int ix = 0; ix < nx; ++ix) {
            double x0 = x_coords[ix];
            double x1 = x_coords[ix+1];

            for (int iz = 0; iz < nz_fin; ++iz) {
                double z0 = z_fin[iz];
                double z1 = z_fin[iz+1];

                int n0 = get_or_add_node(x0, y0, z0);
                int n1 = get_or_add_node(x1, y0, z0);
                int n2 = get_or_add_node(x1, y1, z0);
                int n3 = get_or_add_node(x0, y1, z0);
                int n4 = get_or_add_node(x0, y0, z1);
                int n5 = get_or_add_node(x1, y0, z1);
                int n6 = get_or_add_node(x1, y1, z1);
                int n7 = get_or_add_node(x0, y1, z1);

                bool parity = ((ix + static_cast<int>(f_idx) + iz) % 2) != 0;
                add_brick_tets(n0, n1, n2, n3, n4, n5, n6, n7, parity);
            }
        }
    }

    // ── 6. Extract Outer Boundary Surface Triangles ───────────
    struct FaceEntry {
        int count = 0;
        int elem_id = -1;
        std::array<int, 3> original_nodes;
        int apex_node = -1; // 4th node of parent tet for outward normal orientation
    };

    // Standard 4 faces of tetrahedron (n0, n1, n2, n3)
    const std::array<std::array<int, 4>, 4> tet_faces = {{
        {0, 1, 2, 3}, // Face 0: (0,1,2), apex 3
        {0, 2, 3, 1}, // Face 1: (0,2,3), apex 1
        {0, 3, 1, 2}, // Face 2: (0,3,1), apex 2
        {1, 3, 2, 0}  // Face 3: (1,3,2), apex 0
    }};

    std::map<std::array<int, 3>, FaceEntry> face_map;
    for (int e = 0; e < static_cast<int>(mesh.elements.size()); ++e) {
        const auto& el = mesh.elements[e];
        for (const auto& tf : tet_faces) {
            int na = el.nodes[tf[0]];
            int nb = el.nodes[tf[1]];
            int nc = el.nodes[tf[2]];
            int apex = el.nodes[tf[3]];

            std::array<int, 3> sorted = { na, nb, nc };
            std::sort(sorted.begin(), sorted.end());

            auto& entry = face_map[sorted];
            entry.count++;
            entry.elem_id = e;
            entry.original_nodes = { na, nb, nc };
            entry.apex_node = apex;
        }
    }

    int b_id = 0;
    for (const auto& [sorted_key, entry] : face_map) {
        if (entry.count == 1) { // Appears exactly once -> Boundary face
            SurfaceTriangle st;
            st.id = b_id++;
            st.owning_element = entry.elem_id;

            const auto& p0 = mesh.nodes[entry.original_nodes[0]];
            const auto& p1 = mesh.nodes[entry.original_nodes[1]];
            const auto& p2 = mesh.nodes[entry.original_nodes[2]];
            const auto& pa = mesh.nodes[entry.apex_node];

            // Compute outward normal: (p1-p0) x (p2-p0)
            double ux = p1.x - p0.x, uy = p1.y - p0.y, uz = p1.z - p0.z;
            double vx = p2.x - p0.x, vy = p2.y - p0.y, vz = p2.z - p0.z;
            double nx_val = uy * vz - uz * vy;
            double ny_val = uz * vx - ux * vz;
            double nz_val = ux * vy - uy * vx;
            double norm = std::sqrt(nx_val * nx_val + ny_val * ny_val + nz_val * nz_val);

            // Vector from apex towards face center
            double fc_x = (p0.x + p1.x + p2.x) / 3.0;
            double fc_y = (p0.y + p1.y + p2.y) / 3.0;
            double fc_z = (p0.z + p1.z + p2.z) / 3.0;
            double dir_x = fc_x - pa.x;
            double dir_y = fc_y - pa.y;
            double dir_z = fc_z - pa.z;

            // Ensure normal points outward (away from apex)
            if (nx_val * dir_x + ny_val * dir_y + nz_val * dir_z < 0.0) {
                st.nodes = { entry.original_nodes[0], entry.original_nodes[2], entry.original_nodes[1] };
                nx_val = -nx_val; ny_val = -ny_val; nz_val = -nz_val;
            } else {
                st.nodes = entry.original_nodes;
            }

            if (norm > 1e-16) {
                st.normal = { nx_val / norm, ny_val / norm, nz_val / norm };
                st.area   = norm * 0.5;
            }

            // Boundary Condition Classification:
            // Priority 2 Correction: The heat source is located on the top surface of the base plate
            // at z = base_thickness (tb) with outward normal pointing upwards (+Z).
            bool is_base_top = (std::abs(fc_z - tb) <= 1e-5) && (st.normal[2] > 0.5);
            bool is_base_bottom = (fc_z <= 1e-5) && (st.normal[2] < -0.5);

            if (is_base_top && geometry_.is_in_heat_source(fc_x, fc_y)) {
                st.boundary_type = BoundaryType::HeatSource;
            } else if (is_base_bottom && geometry_.is_in_mounting_hole(fc_x, fc_y)) {
                st.boundary_type = BoundaryType::FixedTemp;
            } else {
                st.boundary_type = BoundaryType::Convection;
            }

            mesh.boundary_faces.push_back(st);
        }
    }

    // ── 7. Run Comprehensive Geometric Validation ────────────
    auto report = MeshValidator::validate(mesh);

    auto end_time = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    mesh.stats.node_count          = static_cast<int>(mesh.nodes.size());
    mesh.stats.element_count       = static_cast<int>(mesh.elements.size());
    mesh.stats.boundary_face_count = static_cast<int>(mesh.boundary_faces.size());
    mesh.stats.degrees_of_freedom  = mesh.stats.node_count;
    mesh.stats.generation_time_ms  = ms;

    // Approximate memory footprint (nodes + tets + boundary faces)
    double mem_bytes = mesh.nodes.size() * sizeof(Node3D) +
                       mesh.elements.size() * sizeof(TetElement) +
                       mesh.boundary_faces.size() * sizeof(SurfaceTriangle);
    mesh.stats.approx_memory_kb = mem_bytes / 1024.0;

    return mesh;
}

} // namespace uew::mesh
