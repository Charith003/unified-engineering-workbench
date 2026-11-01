// ═══════════════════════════════════════════════════════════════════════════
// Unified Engineering Workbench — C++ HTTP Backend
// PASS Science Works R&D Internship 2026–2027
//
// Technology: cpp-httplib v0.18.0, nlohmann/json v3.11.3
// Standard:   C++17
// ═══════════════════════════════════════════════════════════════════════════

#define WIN32_LEAN_AND_MEAN
#undef CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"
#include "json.hpp"

#include "core/geometry/stl_parser.hpp"
#include "core/geometry/csv_parser.hpp"
#include "core/thermal/fem_solver.hpp"
#include "core/performance/benchmark.hpp"
#include "core/hardware/adapters.hpp"
#include "core/backprop/network.hpp"
#include "api/job_manager.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// Global state
// ─────────────────────────────────────────────────────────
static std::mutex g_hw_mutex;
static std::vector<std::unique_ptr<uew::hardware::IHardwareAdapter>> g_adapters;
static std::atomic<bool> g_running{true};

// ─────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────
static json success_response(const std::string& msg, const json& data = json::object()) {
    return {{"success", true}, {"message", msg}, {"data", data}, {"error", nullptr}};
}
static json error_response(const std::string& err, int /*code*/ = 400) {
    return {{"success", false}, {"message", ""}, {"data", json::object()}, {"error", err}};
}

static void set_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static void json_response(const httplib::Request&, httplib::Response& res,
                           const json& j, int status = 200) {
    set_cors(res);
    res.status = status;
    res.set_content(j.dump(2), "application/json");
}

// ─────────────────────────────────────────────────────────
// Job state → string
// ─────────────────────────────────────────────────────────
static std::string job_state_str(uew::api::JobState s) {
    using JS = uew::api::JobState;
    switch (s) {
        case JS::Pending:   return "pending";
        case JS::Running:   return "running";
        case JS::Done:      return "done";
        case JS::Failed:    return "failed";
        case JS::Cancelled: return "cancelled";
    }
    return "unknown";
}

// ─────────────────────────────────────────────────────────
// Frontend serving helper
// ─────────────────────────────────────────────────────────
static std::string g_frontend_dist;

static bool serve_file(const std::string& rel_path, httplib::Response& res) {
    if (g_frontend_dist.empty()) return false;
    std::string full = g_frontend_dist + "/" + rel_path;
    // Path traversal protection
    fs::path canon = fs::weakly_canonical(fs::path(full));
    fs::path base  = fs::weakly_canonical(fs::path(g_frontend_dist));
    auto [a,b] = std::mismatch(base.begin(), base.end(), canon.begin());
    if (a != base.end()) return false;

    std::ifstream f(canon, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss; ss << f.rdbuf();
    std::string ext = canon.extension().string();
    std::string mime = "text/plain";
    if (ext == ".html") mime = "text/html";
    else if (ext == ".js")   mime = "application/javascript";
    else if (ext == ".css")  mime = "text/css";
    else if (ext == ".json") mime = "application/json";
    else if (ext == ".svg")  mime = "image/svg+xml";
    else if (ext == ".png")  mime = "image/png";
    else if (ext == ".ico")  mime = "image/x-icon";
    set_cors(res);
    res.set_content(ss.str(), mime);
    return true;
}

// ─────────────────────────────────────────────────────────
// Port file
// ─────────────────────────────────────────────────────────
static void write_port_file(int port) {
    // Write next to the executable so launcher can read it
    std::ofstream pf("uew_port.txt");
    pf << port;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    // Parse CLI args
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--frontend=", 0) == 0) {
            g_frontend_dist = a.substr(11);
        }
    }

    // Initialise hardware adapters
    {
        std::lock_guard<std::mutex> lk(g_hw_mutex);
        g_adapters = uew::hardware::create_mock_adapters();
    }

    httplib::Server svr;

    // ── CORS preflight ─────────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        res.status = 204;
    });

    // ── Health ─────────────────────────────────────────
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        json data = {
            {"version",   "1.0.0"},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                              std::chrono::system_clock::now().time_since_epoch()).count()},
            {"modules",   {"geometry","thermal","performance","hardware","backprop"}}
        };
        json_response({}, res, success_response("UEW backend running.", data));
    });

    // ── Geometry: STL ──────────────────────────────────
    svr.Post("/api/geometry/stl", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string content = body.value("content", "");
            auto result = uew::geometry::parse_ascii_stl(content);
            if (!result.success) {
                json_response(req, res, error_response(result.error), 400);
                return;
            }
            json tris = json::array();
            for (const auto& t : result.triangles) {
                tris.push_back({
                    {"v0", {t.v0[0], t.v0[1], t.v0[2]}},
                    {"v1", {t.v1[0], t.v1[1], t.v1[2]}},
                    {"v2", {t.v2[0], t.v2[1], t.v2[2]}},
                    {"n",  {t.normal[0], t.normal[1], t.normal[2]}}
                });
            }
            json data = {
                {"solid_name",      result.solid_name},
                {"triangle_count",  result.triangles.size()},
                {"triangles",       tris},
                {"bbox", {
                    {"xmin", result.bbox.xmin}, {"xmax", result.bbox.xmax},
                    {"ymin", result.bbox.ymin}, {"ymax", result.bbox.ymax},
                    {"zmin", result.bbox.zmin}, {"zmax", result.bbox.zmax},
                    {"dx",   result.bbox.dx()},
                    {"dy",   result.bbox.dy()},
                    {"dz",   result.bbox.dz()}
                }},
                {"file_size_note",  result.file_size_note}
            };
            json_response(req, res, success_response("STL parsed successfully.", data));
        } catch (const std::exception& e) {
            json_response(req, res, error_response(std::string("STL parse error: ") + e.what()), 400);
        }
    });

    // ── Geometry: CSV ──────────────────────────────────
    svr.Post("/api/geometry/csv", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string content = body.value("content", "");
            auto result = uew::geometry::parse_temperature_csv(content);
            if (!result.success) {
                json_response(req, res, error_response(result.error), 400);
                return;
            }
            json nodes = json::array();
            for (const auto& n : result.nodes) {
                nodes.push_back({
                    {"id",   n.node_id},
                    {"x",    n.x},
                    {"y",    n.y},
                    {"z",    n.z},
                    {"temp", n.temperature_C}
                });
            }
            json data = {
                {"node_count", result.nodes.size()},
                {"t_min",      result.t_min},
                {"t_max",      result.t_max},
                {"nodes",      nodes}
            };
            json_response(req, res, success_response("Temperature CSV parsed successfully.", data));
        } catch (const std::exception& e) {
            json_response(req, res, error_response(std::string("CSV parse error: ") + e.what()), 400);
        }
    });

    // ── Thermal: solve ─────────────────────────────────
    svr.Post("/api/thermal/solve", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            int N = body.value("N", 10);
            if (N < 1 || N > 10000) {
                json_response(req, res, error_response("N must be between 1 and 10000"), 400);
                return;
            }

            uew::thermal::ThermalParams p;
            p.L   = body.value("L",   1.0);
            p.A   = body.value("A",   1e-4);
            p.k   = body.value("k",   10.0);
            p.q   = body.value("q",   10000.0);
            p.T_L = body.value("T_L", 100.0);
            p.T_R = body.value("T_R", 20.0);

            auto mr = uew::thermal::solve_fem_1d(p, N);

            json data = {
                {"N",                  mr.N},
                {"x_nodes",            mr.x_nodes},
                {"T_fem",              mr.T_fem},
                {"T_exact",            mr.T_exact},
                {"x_midpoints",        mr.x_midpoints},
                {"T_fem_mid",          mr.T_fem_mid},
                {"T_exact_mid",        mr.T_exact_mid},
                {"error_mid",          mr.error_mid},
                {"max_midpoint_error", mr.max_midpoint_error},
                {"csv",                mr.csv},
                {"params", {
                    {"L",p.L},{"A",p.A},{"k",p.k},{"q",p.q},
                    {"T_L",p.T_L},{"T_R",p.T_R}
                }}
            };
            json_response(req, res, success_response("Thermal FEM solved.", data));
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 400);
        }
    });

    // ── Thermal: convergence (all 4 meshes) ────────────
    svr.Post("/api/thermal/convergence", [](const httplib::Request& req, httplib::Response& res) {
        try {
            uew::thermal::ThermalParams p; // default params
            json meshes = json::array();
            for (int N : {10, 20, 40, 80}) {
                auto mr = uew::thermal::solve_fem_1d(p, N);
                meshes.push_back({
                    {"N",                  mr.N},
                    {"max_midpoint_error", mr.max_midpoint_error},
                    {"x_nodes",            mr.x_nodes},
                    {"T_fem",              mr.T_fem},
                    {"T_exact",            mr.T_exact},
                    {"x_midpoints",        mr.x_midpoints},
                    {"T_fem_mid",          mr.T_fem_mid},
                    {"T_exact_mid",        mr.T_exact_mid}
                });
            }
            json_response(req, res, success_response("Convergence study complete.", {{"meshes", meshes}}));
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 400);
        }
    });

    // ── Performance: run (async job) ──────────────────
    svr.Post("/api/performance/run", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            int batch_size = body.value("batch_size", 50);
            int mesh_size  = body.value("mesh_size",  20);
            int workers    = body.value("workers",     2);

            if (batch_size < 1 || batch_size > 5000)
                { json_response(req, res, error_response("batch_size out of range [1,5000]"), 400); return; }
            if (mesh_size < 5 || mesh_size > 500)
                { json_response(req, res, error_response("mesh_size out of range [5,500]"), 400); return; }
            if (workers < 1 || workers > 32)
                { json_response(req, res, error_response("workers out of range [1,32]"), 400); return; }

            auto& jm = uew::api::JobManager::instance();
            std::string job_id = jm.create("performance");

            jm.run(job_id, [batch_size, mesh_size, workers, job_id](uew::api::Job& job) {
                job.message  = "Generating batch...";
                job.progress = 5;

                auto cases = uew::performance::generate_batch(batch_size);
                job.message  = "Running serial benchmark...";
                job.progress = 15;

                auto serial = uew::performance::run_serial(cases, mesh_size);
                job.message  = "Running parallel benchmark...";
                job.progress = 40;

                auto parallel = uew::performance::run_parallel(cases, mesh_size, workers, 5,
                    [&job, batch_size](int done, int total) {
                        job.progress = 40 + (done * 50) / total;
                    });

                // Set speedup and efficiency from serial median
                double T1 = serial.median_time_ms;
                double Tp = parallel.median_time_ms;
                parallel.speedup    = (Tp > 0.0) ? T1 / Tp : 1.0;
                parallel.efficiency = parallel.speedup / parallel.num_workers;

                // Verify serial/parallel agreement <= 1e-8
                double max_diff = 0.0;
                for (size_t i = 0; i < cases.size(); ++i) {
                    double d = std::abs(serial.case_results[i].T_max -
                                        parallel.case_results[i].T_max);
                    max_diff = std::max(max_diff, d);
                }

                json result = {
                    {"serial",           {
                        {"median_ms",    serial.median_time_ms},
                        {"times_ms",     {serial.all_times_ms[0], serial.all_times_ms[1],
                                          serial.all_times_ms[2], serial.all_times_ms[3],
                                          serial.all_times_ms[4]}},
                        {"workers",      1},
                        {"csv",          serial.csv}
                    }},
                    {"parallel",         {
                        {"median_ms",    parallel.median_time_ms},
                        {"times_ms",     {parallel.all_times_ms[0], parallel.all_times_ms[1],
                                          parallel.all_times_ms[2], parallel.all_times_ms[3],
                                          parallel.all_times_ms[4]}},
                        {"workers",      parallel.num_workers},
                        {"speedup",      parallel.speedup},
                        {"efficiency",   parallel.efficiency},
                        {"csv",          parallel.csv}
                    }},
                    {"max_serial_parallel_diff_C", max_diff},
                    {"agreement_ok",     max_diff <= 1e-8},
                    {"system", {
                        {"cpu",          serial.cpu_model},
                        {"logical_cores",serial.logical_cores},
                        {"physical_cores",serial.physical_cores},
                        {"ram",          serial.ram_info},
                        {"compiler",     serial.compiler_info},
                        {"opt_flags",    serial.opt_flags}
                    }},
                    {"config", {
                        {"batch_size",  batch_size},
                        {"mesh_size",   mesh_size},
                        {"workers",     workers}
                    }}
                };

                job.result   = result;
                job.progress = 100;
                job.message  = "Benchmark complete.";
            });

            json_response(req, res, success_response("Benchmark job started.", {{"job_id", job_id}}), 202);
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 400);
        }
    });

    // ── Job status ──────────────────────────────────────
    svr.Get("/api/jobs/:id", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string id = req.path_params.at("id");
            auto& jm = uew::api::JobManager::instance();
            auto snap = jm.get(id);
            json j = {
                {"id",       snap.id},
                {"type",     snap.type},
                {"state",    job_state_str(snap.state)},
                {"progress", snap.progress},
                {"message",  snap.message},
                {"error",    snap.error},
                {"has_result", snap.has_result}
            };
            if (snap.has_result && snap.state == uew::api::JobState::Done) {
                try {
                    j["result"] = jm.get_result<json>(id);
                } catch (...) {}
            }
            json_response(req, res, success_response("", j));
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 404);
        }
    });

    // ── Job cancel ──────────────────────────────────────
    svr.Post("/api/jobs/:id/cancel", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string id = req.path_params.at("id");
            bool ok = uew::api::JobManager::instance().cancel(id);
            if (ok) json_response(req, res, success_response("Job cancelled."));
            else    json_response(req, res, error_response("Cannot cancel job."), 400);
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 404);
        }
    });

    // ── Hardware: list devices ──────────────────────────
    svr.Get("/api/hardware/devices", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_hw_mutex);
        json devices = json::array();
        for (const auto& a : g_adapters) {
            auto info = a->get_info();
            std::string type_str = (info.type == uew::hardware::DeviceType::MCU)  ? "MCU"  :
                                   (info.type == uew::hardware::DeviceType::FPGA) ? "FPGA" : "JTAG";
            std::string state_str;
            switch (info.state) {
                case uew::hardware::DeviceState::Disconnected: state_str = "disconnected"; break;
                case uew::hardware::DeviceState::Connecting:   state_str = "connecting";   break;
                case uew::hardware::DeviceState::Connected:    state_str = "connected";    break;
                case uew::hardware::DeviceState::Error:        state_str = "error";        break;
            }
            devices.push_back({
                {"id",           info.id},
                {"name",         info.name},
                {"type",         type_str},
                {"state",        state_str},
                {"description",  info.description},
                {"capabilities", info.capabilities},
                {"board_model",  info.board_model},
                {"interface",    info.interface},
                {"simulated",    info.simulated}
            });
        }
        json_response(req, res, success_response("Device list retrieved.", {{"devices", devices}}));
    });

    // ── Hardware: connect ──────────────────────────────
    svr.Post("/api/hardware/connect", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string dev_id = body.value("device_id", "");
            std::lock_guard<std::mutex> lk(g_hw_mutex);
            for (auto& a : g_adapters) {
                if (a->get_info().id == dev_id) {
                    auto r = a->connect();
                    if (r.success) json_response(req, res, success_response(r.message));
                    else           json_response(req, res, error_response(r.error), 503);
                    return;
                }
            }
            json_response(req, res, error_response("Device not found: " + dev_id), 404);
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 400);
        }
    });

    // ── Hardware: disconnect ────────────────────────────
    svr.Post("/api/hardware/disconnect", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string dev_id = body.value("device_id", "");
            std::lock_guard<std::mutex> lk(g_hw_mutex);
            for (auto& a : g_adapters) {
                if (a->get_info().id == dev_id) {
                    auto r = a->disconnect();
                    json_response(req, res, success_response(r.message));
                    return;
                }
            }
            json_response(req, res, error_response("Device not found: " + dev_id), 404);
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 400);
        }
    });

    // ── Hardware: telemetry ─────────────────────────────
    svr.Post("/api/hardware/telemetry", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string dev_id = body.value("device_id", "");
            std::lock_guard<std::mutex> lk(g_hw_mutex);
            for (auto& a : g_adapters) {
                if (a->get_info().id == dev_id) {
                    auto tm = a->read_telemetry();
                    json data = {
                        {"device_id",   tm.device_id},
                        {"timestamp_s", tm.timestamp_s},
                        {"channel",     tm.channel},
                        {"value",       tm.value},
                        {"unit",        tm.unit},
                        {"raw_hex",     tm.raw_hex}
                    };
                    json_response(req, res, success_response("Telemetry read.", data));
                    return;
                }
            }
            json_response(req, res, error_response("Device not found: " + dev_id), 404);
        } catch (const std::exception& e) {
            json_response(req, res, error_response(std::string("Telemetry error: ") + e.what()), 400);
        }
    });

    // ── Backprop ────────────────────────────────────────
    svr.Post("/api/backprop/run", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            double lr      = body.value("lr",      0.1);
            int    steps   = body.value("steps",   100);
            double epsilon = body.value("epsilon", 1e-6);

            if (lr <= 0.0 || lr > 10.0)
                { json_response(req, res, error_response("lr must be in (0, 10]"), 400); return; }
            if (steps < 1 || steps > 10000)
                { json_response(req, res, error_response("steps must be in [1, 10000]"), 400); return; }

            auto r = uew::backprop::run_backprop(lr, steps, epsilon);
            if (!r.success) {
                json_response(req, res, error_response(r.error), 400);
                return;
            }

            json history = json::array();
            for (const auto& h : r.history) {
                history.push_back({{"step",h.step},{"w",h.w},{"b",h.b},
                                   {"v",h.v},{"c",h.c},{"loss",h.J}});
            }

            json data = {
                {"x_input",  r.x_input},
                {"y_target", r.y_target},
                {"initial_forward", {
                    {"h",     r.initial_forward.h},
                    {"y_hat", r.initial_forward.y_hat},
                    {"J",     r.initial_forward.J}
                }},
                {"analytical_gradients", {
                    {"dJ_dw", r.analytical_grads.dJ_dw},
                    {"dJ_db", r.analytical_grads.dJ_db},
                    {"dJ_dv", r.analytical_grads.dJ_dv},
                    {"dJ_dc", r.analytical_grads.dJ_dc}
                }},
                {"fd_gradients", {
                    {"dJ_dw", r.fd_grads.dJ_dw},
                    {"dJ_db", r.fd_grads.dJ_db},
                    {"dJ_dv", r.fd_grads.dJ_dv},
                    {"dJ_dc", r.fd_grads.dJ_dc}
                }},
                {"max_grad_error",  r.max_grad_error},
                {"verification_ok", r.max_grad_error <= 1e-5},
                {"updated_params", {
                    {"w", r.updated_params.w},
                    {"b", r.updated_params.b},
                    {"v", r.updated_params.v},
                    {"c", r.updated_params.c}
                }},
                {"history",  history},
                {"loss_csv", r.loss_csv}
            };
            json_response(req, res, success_response("Backpropagation complete.", data));
        } catch (const std::exception& e) {
            json_response(req, res, error_response(e.what()), 400);
        }
    });

    // ── Frontend static files ───────────────────────────
    if (!g_frontend_dist.empty()) {
        svr.Get("/.*", [](const httplib::Request& req, httplib::Response& res) {
            std::string path = req.path;
            if (path == "/") path = "/index.html";
            if (path.front() == '/') path = path.substr(1);

            if (!serve_file(path, res)) {
                serve_file("index.html", res);
            }
        });
    }

    // ── Find available port ─────────────────────────────
    int port = 0;
    for (int try_port = 7800; try_port <= 7900; ++try_port) {
        if (svr.bind_to_port("127.0.0.1", try_port)) {
            port = try_port;
            break;
        }
    }
    if (port == 0) {
        std::cerr << "UEW: Failed to bind to any port in range 7800-7900\n";
        return 1;
    }

    write_port_file(port);
    std::cout << "UEW_PORT=" << port << std::endl;
    std::cout << "UEW backend running on http://127.0.0.1:" << port << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    svr.listen_after_bind();
    return 0;
}
