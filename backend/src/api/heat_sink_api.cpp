#include "api/heat_sink_api.hpp"
#include "core/geometry/heat_sink_geometry.hpp"
#include "core/mesh/mesh_generator.hpp"
#include "core/mesh/mesh_validator.hpp"
#include "core/thermal/fem_3d_solver.hpp"
#include "core/thermal/fem_3d_types.hpp"
#include "core/benchmark/solver_benchmark.hpp"
#include "core/solvers/cholesky_solver.hpp"
#include "core/solvers/conjugate_gradient_solver.hpp"
#include "core/solvers/gauss_seidel_solver.hpp"
#include "json.hpp"
#include <iostream>
#include <memory>

using json = nlohmann::json;

namespace uew::api {

static void set_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static void json_response(const httplib::Request&, httplib::Response& res,
                          const json& payload, int status = 200) {
    set_cors(res);
    res.status = status;
    res.set_content(payload.dump(2), "application/json");
}

static geometry::HeatSinkParams parse_geometry_params(const json& j) {
    auto p = geometry::HeatSinkParams::default_asymmetric();
    if (j.contains("base_length")) p.base_length = j["base_length"].get<double>();
    if (j.contains("base_width")) p.base_width = j["base_width"].get<double>();
    if (j.contains("base_thickness")) p.base_thickness = j["base_thickness"].get<double>();
    if (j.contains("has_mounting_holes")) p.has_mounting_holes = j["has_mounting_holes"].get<bool>();
    if (j.contains("hole_diameter")) p.hole_diameter = j["hole_diameter"].get<double>();
    if (j.contains("heat_source_length")) p.heat_source_length = j["heat_source_length"].get<double>();
    if (j.contains("heat_source_width")) p.heat_source_width = j["heat_source_width"].get<double>();
    if (j.contains("heat_source_x_off")) p.heat_source_x_off = j["heat_source_x_off"].get<double>();
    if (j.contains("heat_source_y_off")) p.heat_source_y_off = j["heat_source_y_off"].get<double>();

    // Fin parameters
    if (j.contains("fins") && j["fins"].is_array()) {
        p.fins.clear();
        for (const auto& fj : j["fins"]) {
            geometry::FinConfig f;
            f.y_start = fj["y_start"].get<double>();
            f.thickness = fj["thickness"].get<double>();
            f.height = fj["height"].get<double>();
            p.fins.push_back(f);
        }
        p.fin_count = static_cast<int>(p.fins.size());
    }
    return p;
}

static thermal::ThermalMaterial parse_material(const json& j) {
    thermal::ThermalMaterial mat = thermal::ThermalMaterial::aluminum_6061();
    if (j.contains("material")) {
        const auto& mj = j["material"];
        if (mj.contains("name")) mat.name = mj["name"].get<std::string>();
        if (mj.contains("thermal_conductivity")) mat.thermal_conductivity = mj["thermal_conductivity"].get<double>();
        if (mj.contains("density")) mat.density = mj["density"].get<double>();
        if (mj.contains("specific_heat")) mat.specific_heat = mj["specific_heat"].get<double>();
    }
    return mat;
}

static thermal::ThermalBoundaryConditions parse_bcs(const json& j) {
    thermal::ThermalBoundaryConditions bc;
    if (j.contains("boundary_conditions")) {
        const auto& bj = j["boundary_conditions"];
        if (bj.contains("ambient_temperature_c")) bc.ambient_temperature_c = bj["ambient_temperature_c"].get<double>();
        if (bj.contains("heat_transfer_coefficient")) bc.heat_transfer_coefficient = bj["heat_transfer_coefficient"].get<double>();
        if (bj.contains("heat_source_power_w")) bc.heat_source_power_w = bj["heat_source_power_w"].get<double>();
        if (bj.contains("fix_mounting_holes")) bc.fix_mounting_holes = bj["fix_mounting_holes"].get<bool>();
        if (bj.contains("fixed_mounting_temperature_c")) bc.fixed_mounting_temperature_c = bj["fixed_mounting_temperature_c"].get<double>();
    }
    return bc;
}

json handle_thermal_solve_request(const json& body, int& http_status) {
    auto params = parse_geometry_params(body);
    auto mat = parse_material(body);
    auto bc = parse_bcs(body);

    mesh::MeshResolution res_preset = mesh::MeshResolution::Medium;
    if (body.contains("resolution")) {
        std::string r_str = body["resolution"].get<std::string>();
        if (r_str == "coarse" || r_str == "Coarse") res_preset = mesh::MeshResolution::Coarse;
        else if (r_str == "fine" || r_str == "Fine") res_preset = mesh::MeshResolution::Fine;
    }

    std::string solver_type = "cholesky";
    if (body.contains("solver")) {
        solver_type = body["solver"].get<std::string>();
    }

    geometry::HeatSinkGeometry geom(params);
    mesh::MeshGenerator gen(geom);
    mesh::MeshGenerationOptions opts;
    opts.resolution = res_preset;
    auto m = gen.generate(opts);
    auto val_report = mesh::MeshValidator::validate(m);
    if (!val_report.is_valid) {
        http_status = 400;
        return {{"success", false}, {"error", "Cannot perform thermal solve on invalid mesh: " + val_report.message}};
    }

    thermal::FEM3DSolver fem;
    auto sys = fem.assemble(m, mat, bc);

    std::unique_ptr<solvers::ISolver> solver;
    if (solver_type == "cg" || solver_type == "conjugate_gradient") {
        solver = std::make_unique<solvers::ConjugateGradientSolver>();
    } else if (solver_type == "gauss_seidel" || solver_type == "gs") {
        solver = std::make_unique<solvers::GaussSeidelSolver>();
    } else {
        solver = std::make_unique<solvers::CholeskySolver>(); // Sparse LDLT direct reference
    }

    solvers::SolverConfig scfg;
    if (body.contains("tolerance")) scfg.tolerance = body["tolerance"].get<double>();
    if (body.contains("max_iterations")) scfg.max_iterations = body["max_iterations"].get<size_t>();
    if (body.contains("relaxation_omega")) scfg.relaxation_omega = body["relaxation_omega"].get<double>();

    auto sres = solver->solve(sys.K, sys.F, scfg);

    bool is_direct_ldlt = (sres.solver_name.find("LDLT") != std::string::npos || sres.solver_name.find("Cholesky") != std::string::npos);
    bool solution_valid = sres.success &&
                          (sres.solution.size() == static_cast<Eigen::Index>(sys.dof_count)) &&
                          sres.solution.allFinite();

    // 1. THERMAL API FAILURE HANDLING:
    // If solver fails or solution is invalid, return HTTP 422 with outer success: false and inner data.success: false.
    // Preserve useful diagnostics (iterations, residual, assembly time, solve time, etc.).
    // Withhold min/max/avg temperature, heat output, energy balance, and visual mesh contours.
    if (!solution_valid) {
        http_status = 422;
        std::string diag_msg = sres.error_message.empty() 
            ? "Numerical solver failed to reach specified convergence tolerance" 
            : sres.error_message;
        json data = {
            {"success", false},
            {"solver_name", sres.solver_name},
            {"status_message", diag_msg},
            {"assembly_time_ms", sys.assembly_time_ms},
            {"factorization_time_ms", is_direct_ldlt ? sres.factorization_time_ms : -1.0},
            {"setup_time_ms", !is_direct_ldlt ? sres.factorization_time_ms : -1.0},
            {"solve_time_ms", sres.solve_time_ms},
            {"total_time_ms", sres.total_time_ms},
            {"iterations", sres.iterations},
            {"residual_norm", sres.residual_norm},
            {"memory_kb", sres.estimated_memory_bytes / 1024},
            {"nodal_temperatures", json::array()},
            {"nodes", json::array()},
            {"boundary_faces", json::array()},
            {"elements", json::array()}
        };
        return {
            {"success", false},
            {"error", diag_msg},
            {"data", data}
        };
    }

    // Only compute downstream metrics on genuine valid solutions
    double q_out = thermal::FEM3DSolver::compute_total_heat_out(m, sres.solution, bc);
    double t_min = sres.solution.minCoeff();
    double t_max = sres.solution.maxCoeff();
    double t_avg = sres.solution.mean();
    double r_th = (t_max - bc.ambient_temperature_c) / std::max(0.001, sys.total_heat_in_w);
    double energy_err = std::abs(sys.total_heat_in_w - q_out) / std::max(0.001, sys.total_heat_in_w) * 100.0;

    std::vector<double> nodal_temps(sres.solution.size());
    for (Eigen::Index i = 0; i < sres.solution.size(); ++i) {
        nodal_temps[i] = sres.solution(i);
    }

    // Serialize nodes and boundary faces for 3D temperature visualization
    json nodes_json = json::array();
    for (const auto& n : m.nodes) {
        nodes_json.push_back({{"id", n.id}, {"x", n.x}, {"y", n.y}, {"z", n.z}});
    }

    json faces_json = json::array();
    for (const auto& f : m.boundary_faces) {
        faces_json.push_back({
            {"id", f.id},
            {"nodes", {f.nodes[0], f.nodes[1], f.nodes[2]}},
            {"boundary_type", mesh::to_string(f.boundary_type)},
            {"area", f.area},
            {"normal", {f.normal[0], f.normal[1], f.normal[2]}}
        });
    }

    json elements_json = json::array();
    for (const auto& el : m.elements) {
        elements_json.push_back({
            {"id", el.id},
            {"nodes", {el.nodes[0], el.nodes[1], el.nodes[2], el.nodes[3]}},
            {"quality", el.quality}
        });
    }

    json data = {
        {"success", true},
        {"solver_name", sres.solver_name},
        {"status_message", sres.error_message.empty() ? "Optimal Convergence" : sres.error_message},
        {"min_temperature_c", t_min},
        {"max_temperature_c", t_max},
        {"avg_temperature_c", t_avg},
        {"thermal_resistance_k_w", r_th},
        {"total_heat_in_w", sys.total_heat_in_w},
        {"total_heat_out_w", q_out},
        {"energy_balance_error_percent", energy_err},
        {"assembly_time_ms", sys.assembly_time_ms},
        {"factorization_time_ms", is_direct_ldlt ? sres.factorization_time_ms : -1.0},
        {"setup_time_ms", !is_direct_ldlt ? sres.factorization_time_ms : -1.0},
        {"solve_time_ms", sres.solve_time_ms},
        {"total_time_ms", sres.total_time_ms},
        {"iterations", sres.iterations},
        {"residual_norm", sres.residual_norm},
        {"memory_kb", sres.estimated_memory_bytes / 1024},
        {"nodal_temperatures", nodal_temps},
        {"nodes", nodes_json},
        {"boundary_faces", faces_json},
        {"elements", elements_json}
    };

    http_status = 200;
    return {
        {"success", true},
        {"data", data}
    };
}

void register_heat_sink_routes(httplib::Server& svr) {
    // 1. POST /api/heat-sink/geometry/generate
    svr.Post("/api/heat-sink/geometry/generate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = req.body.empty() ? json::object() : json::parse(req.body);
            auto params = parse_geometry_params(body);
            geometry::HeatSinkGeometry geom(params);
            auto val = geom.validate();

            json facets_json = json::array();
            auto facets = geom.generate_surface_facets();
            for (const auto& f : facets) {
                facets_json.push_back({
                    {"v0", {{"x", f.vertices[0].x}, {"y", f.vertices[0].y}, {"z", f.vertices[0].z}}},
                    {"v1", {{"x", f.vertices[1].x}, {"y", f.vertices[1].y}, {"z", f.vertices[1].z}}},
                    {"v2", {{"x", f.vertices[2].x}, {"y", f.vertices[2].y}, {"z", f.vertices[2].z}}},
                    {"normal", {{"x", f.normal[0]}, {"y", f.normal[1]}, {"z", f.normal[2]}}}
                });
            }

            json data = {
                {"is_valid", val.is_valid},
                {"message", val.error_message.empty() ? "Geometry valid" : val.error_message},
                {"total_volume_m3", val.total_volume_m3},
                {"total_surface_area_m2", val.total_surface_area_m2},
                {"heat_source_area_m2", val.heat_source_area_m2},
                {"facet_count", facets.size()},
                {"facets", facets_json},
                {"stl_data", geom.to_ascii_stl()}
            };

            json_response(req, res, {{"success", true}, {"data", data}});
        } catch (const std::exception& e) {
            json_response(req, res, {{"success", false}, {"error", e.what()}}, 400);
        }
    });

    // 2. POST /api/heat-sink/mesh/generate
    svr.Post("/api/heat-sink/mesh/generate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = req.body.empty() ? json::object() : json::parse(req.body);
            auto params = parse_geometry_params(body);
            geometry::HeatSinkGeometry geom(params);

            mesh::MeshResolution res_preset = mesh::MeshResolution::Medium;
            if (body.contains("resolution")) {
                std::string r_str = body["resolution"].get<std::string>();
                if (r_str == "coarse" || r_str == "Coarse") res_preset = mesh::MeshResolution::Coarse;
                else if (r_str == "fine" || r_str == "Fine") res_preset = mesh::MeshResolution::Fine;
            }

            mesh::MeshGenerator gen(geom);
            mesh::MeshGenerationOptions opts;
            opts.resolution = res_preset;
            auto m = gen.generate(opts);
            auto report = mesh::MeshValidator::validate(m);

            // Serialize boundary faces and nodes for WebGL rendering
            json nodes_json = json::array();
            for (const auto& n : m.nodes) {
                nodes_json.push_back({{"id", n.id}, {"x", n.x}, {"y", n.y}, {"z", n.z}});
            }

            json faces_json = json::array();
            for (const auto& f : m.boundary_faces) {
                faces_json.push_back({
                    {"id", f.id},
                    {"nodes", {f.nodes[0], f.nodes[1], f.nodes[2]}},
                    {"boundary_type", mesh::to_string(f.boundary_type)},
                    {"area", f.area},
                    {"normal", {f.normal[0], f.normal[1], f.normal[2]}}
                });
            }

            json elements_json = json::array();
            for (const auto& el : m.elements) {
                elements_json.push_back({
                    {"id", el.id},
                    {"nodes", {el.nodes[0], el.nodes[1], el.nodes[2], el.nodes[3]}},
                    {"quality", el.quality}
                });
            }

            json data = {
                {"is_valid", report.is_valid},
                {"status_message", report.message},
                {"resolution", mesh::MeshGenerator::resolution_to_string(res_preset)},
                {"node_count", m.nodes.size()},
                {"element_count", m.elements.size()},
                {"boundary_face_count", m.boundary_faces.size()},
                {"degrees_of_freedom", m.nodes.size()},
                {"generation_time_ms", m.stats.generation_time_ms},
                {"approx_memory_kb", m.stats.approx_memory_kb},
                {"min_quality", m.stats.min_quality},
                {"max_quality", m.stats.max_quality},
                {"mean_quality", m.stats.mean_quality},
                {"nodes", nodes_json},
                {"boundary_faces", faces_json},
                {"elements", elements_json}
            };

            json_response(req, res, {{"success", true}, {"data", data}});
        } catch (const std::exception& e) {
            json_response(req, res, {{"success", false}, {"error", e.what()}}, 400);
        }
    });

    // 3. POST /api/heat-sink/thermal/solve
    svr.Post("/api/heat-sink/thermal/solve", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = req.body.empty() ? json::object() : json::parse(req.body);
            int status = 200;
            json resp = handle_thermal_solve_request(body, status);
            json_response(req, res, resp, status);
        } catch (const std::exception& e) {
            json_response(req, res, {{"success", false}, {"error", e.what()}}, 400);
        }
    });

    // 4. POST /api/heat-sink/benchmark/run
    svr.Post("/api/heat-sink/benchmark/run", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = req.body.empty() ? json::object() : json::parse(req.body);
            auto params = parse_geometry_params(body);
            auto mat = parse_material(body);
            auto bc = parse_bcs(body);

            mesh::MeshResolution res_preset = mesh::MeshResolution::Coarse;
            if (body.contains("resolution")) {
                std::string r_str = body["resolution"].get<std::string>();
                if (r_str == "medium" || r_str == "Medium") res_preset = mesh::MeshResolution::Medium;
                else if (r_str == "fine" || r_str == "Fine") res_preset = mesh::MeshResolution::Fine;
            }

            int trials = 5;
            if (body.contains("trial_count")) trials = body["trial_count"].get<int>();

            geometry::HeatSinkGeometry geom(params);
            mesh::MeshGenerator gen(geom);
            mesh::MeshGenerationOptions opts;
            opts.resolution = res_preset;
            auto m = gen.generate(opts);
            auto val_report = mesh::MeshValidator::validate(m);
            if (!val_report.is_valid) {
                json_response(req, res, {{"success", false}, {"error", "Cannot perform solver benchmark on invalid mesh: " + val_report.message}}, 400);
                return;
            }

            benchmark::SolverBenchmark bench;
            auto report = bench.run_benchmark(m, mat, bc, trials);

            json solvers_json = json::array();
            for (const auto& s : report.solvers) {
                solvers_json.push_back({
                    {"solver_name", s.solver_name},
                    {"success", s.success},
                    {"status_message", s.status_message},
                    {"iterations", s.iterations},
                    {"residual_norm", s.residual_norm},
                    {"max_temp_diff_vs_reference_c", s.max_temp_diff_vs_reference_c},
                    {"median_total_time_ms", s.median_total_time_ms},
                    {"min_total_time_ms", s.min_total_time_ms},
                    {"max_total_time_ms", s.max_total_time_ms},
                    {"median_factorization_time_ms", s.median_factorization_time_ms},
                    {"median_setup_time_ms", s.median_setup_time_ms},
                    {"median_solve_time_ms", s.median_solve_time_ms},
                    {"speedup_vs_reference", s.speedup_vs_reference},
                    {"memory_kb", s.memory_kb},
                    {"trial_times_ms", s.trial_times_ms}
                });
            }

            json data = {
                {"resolution", mesh::MeshGenerator::resolution_to_string(res_preset)},
                {"dof_count", report.dof_count},
                {"element_count", report.element_count},
                {"non_zeros", report.non_zeros},
                {"assembly_time_ms", report.assembly_time_ms},
                {"total_heat_in_w", report.total_heat_in_w},
                {"total_heat_out_w", report.total_heat_out_w},
                {"energy_balance_error_percent", report.energy_balance_error_percent},
                {"min_temperature_c", report.min_temperature_c},
                {"max_temperature_c", report.max_temperature_c},
                {"thermal_resistance_k_w", report.thermal_resistance_k_w},
                {"solvers", solvers_json},
                {"reference_solution", report.reference_solution}
            };

            json_response(req, res, {{"success", true}, {"data", data}});
        } catch (const std::exception& e) {
            json_response(req, res, {{"success", false}, {"error", e.what()}}, 400);
        }
    });

    // 5. POST /api/heat-sink/convergence/run
    svr.Post("/api/heat-sink/convergence/run", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = req.body.empty() ? json::object() : json::parse(req.body);
            auto params = parse_geometry_params(body);
            auto mat = parse_material(body);
            auto bc = parse_bcs(body);

            geometry::HeatSinkGeometry geom(params);
            benchmark::SolverBenchmark bench;
            auto cr = bench.run_mesh_convergence(geom, mat, bc);

            json points_json = json::array();
            for (const auto& pt : cr.points) {
                points_json.push_back({
                    {"resolution_name", pt.resolution_name},
                    {"node_count", pt.node_count},
                    {"element_count", pt.element_count},
                    {"max_temperature_c", pt.max_temperature_c},
                    {"min_temperature_c", pt.min_temperature_c},
                    {"avg_temperature_c", pt.avg_temperature_c},
                    {"thermal_resistance_k_w", pt.thermal_resistance_k_w},
                    {"relative_change_percent", pt.relative_change_percent},
                    {"rel_max_temp_change_percent", pt.rel_max_temp_change_percent},
                    {"rel_avg_temp_change_percent", pt.rel_avg_temp_change_percent},
                    {"rel_thermal_resistance_change_percent", pt.rel_thermal_resistance_change_percent},
                    {"rel_field_l2_difference_percent", pt.rel_field_l2_difference_percent},
                    {"assembly_time_ms", pt.assembly_time_ms},
                    {"cholesky_time_ms", pt.cholesky_time_ms},
                    {"cg_time_ms", pt.cg_time_ms},
                    {"gs_time_ms", pt.gs_time_ms},
                    {"cholesky_converged", pt.cholesky_converged},
                    {"cg_converged", pt.cg_converged},
                    {"gs_converged", pt.gs_converged}
                });
            }

            json data = {
                {"is_converged", cr.is_converged},
                {"max_temp_converged", cr.max_temp_converged},
                {"field_converged", cr.field_converged},
                {"solver_converged", cr.solver_converged},
                {"asymptotic_estimate_c", cr.asymptotic_estimate_c},
                {"recommendation", cr.recommendation},
                {"points", points_json}
            };

            // Exactly one JSON response call per request
            json_response(req, res, {{"success", true}, {"data", data}});
        } catch (const std::exception& e) {
            json_response(req, res, {{"success", false}, {"error", e.what()}}, 400);
        }
    });
}

} // namespace uew::api
