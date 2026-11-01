#include "core/performance/benchmark.hpp"
#include "core/thermal/fem_solver.hpp"

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <cstring>
#include <random>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <sysinfoapi.h>
#  if defined(_MSC_VER)
#    include <intrin.h>
#  elif defined(__GNUC__)
#    include <cpuid.h>
#  endif
#endif

namespace uew::performance {

// ─────────────────────────────────────────────────────────
// System info
// ─────────────────────────────────────────────────────────
std::string get_cpu_model() {
#ifdef _WIN32
    char brand[49] = {};
    int cpuInfo[4] = {};
#if defined(_MSC_VER)
    __cpuid(cpuInfo, 0x80000002);
    memcpy(brand,      cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
#elif defined(__GNUC__)
    __get_cpuid(0x80000002, (unsigned int*)&cpuInfo[0], (unsigned int*)&cpuInfo[1], (unsigned int*)&cpuInfo[2], (unsigned int*)&cpuInfo[3]);
    memcpy(brand,      cpuInfo, sizeof(cpuInfo));
    __get_cpuid(0x80000003, (unsigned int*)&cpuInfo[0], (unsigned int*)&cpuInfo[1], (unsigned int*)&cpuInfo[2], (unsigned int*)&cpuInfo[3]);
    memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
    __get_cpuid(0x80000004, (unsigned int*)&cpuInfo[0], (unsigned int*)&cpuInfo[1], (unsigned int*)&cpuInfo[2], (unsigned int*)&cpuInfo[3]);
    memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
#endif
    // Trim leading spaces
    char* p = brand;
    while (*p == ' ') ++p;
    return std::string(p);
#else
    return "Unknown CPU";
#endif
}

int get_logical_cores() {
    return static_cast<int>(std::thread::hardware_concurrency());
}

std::string get_ram_info() {
#ifdef _WIN32
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    unsigned long long gb = ms.ullTotalPhys / (1024ULL * 1024 * 1024);
    return std::to_string(gb) + " GB";
#else
    return "Unknown";
#endif
}

// ─────────────────────────────────────────────────────────
// Deterministic batch generator (fixed seed)
// k in [5, 50], q in [1000, 50000] — varied positive values
// ─────────────────────────────────────────────────────────
std::vector<BenchCase> generate_batch(int batch_size, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> k_dist(5.0, 50.0);
    std::uniform_real_distribution<double> q_dist(1000.0, 50000.0);

    std::vector<BenchCase> cases;
    cases.reserve(batch_size);
    for (int i = 0; i < batch_size; ++i) {
        BenchCase bc;
        bc.case_id = i;
        bc.k       = k_dist(rng);
        bc.q       = q_dist(rng);
        cases.push_back(bc);
    }
    return cases;
}

// ─────────────────────────────────────────────────────────
// Solve one case using thermal FEM
// ─────────────────────────────────────────────────────────
static CaseResult solve_case(const BenchCase& bc, int mesh_size) {
    thermal::ThermalParams p;
    p.k = bc.k;
    p.q = bc.q;
    auto mr = thermal::solve_fem_1d(p, mesh_size);
    CaseResult cr;
    cr.case_id = bc.case_id;
    cr.T_max   = *std::max_element(mr.T_fem.begin(), mr.T_fem.end());
    cr.T_min   = *std::min_element(mr.T_fem.begin(), mr.T_fem.end());
    return cr;
}

// ─────────────────────────────────────────────────────────
// Timing helper
// ─────────────────────────────────────────────────────────
static double time_ms(std::function<void()> fn) {
    auto t0 = std::chrono::high_resolution_clock::now();
    fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double median5(double arr[5]) {
    double tmp[5];
    for (int i = 0; i < 5; ++i) tmp[i] = arr[i];
    std::sort(tmp, tmp + 5);
    return tmp[2];
}

// ─────────────────────────────────────────────────────────
// Serial solver
// ─────────────────────────────────────────────────────────
BenchmarkResult run_serial(const std::vector<BenchCase>& cases, int mesh_size, int repetitions) {
    // Warm-up
    for (const auto& c : cases) solve_case(c, mesh_size);

    BenchmarkResult res;
    res.num_workers = 1;
    res.batch_size  = static_cast<int>(cases.size());
    res.mesh_size   = mesh_size;
    res.compiler_info = "GCC " + std::string(__VERSION__);
    res.opt_flags   = "-O2";
    res.cpu_model   = get_cpu_model();
    res.logical_cores = get_logical_cores();
    res.physical_cores = res.logical_cores / 2;
    res.ram_info    = get_ram_info();

    for (int r = 0; r < repetitions; ++r) {
        std::vector<CaseResult> results(cases.size());
        res.all_times_ms[r] = time_ms([&]() {
            for (size_t i = 0; i < cases.size(); ++i) {
                results[i] = solve_case(cases[i], mesh_size);
            }
        });
        if (r == repetitions - 1) {
            res.case_results = std::move(results);
        }
    }
    res.median_time_ms = median5(res.all_times_ms);
    res.speedup        = 1.0;
    res.efficiency     = 1.0;

    // Generate CSV
    std::ostringstream csv;
    csv << "repetition,time_ms\n";
    for (int r = 0; r < repetitions; ++r)
        csv << (r+1) << "," << res.all_times_ms[r] << "\n";
    csv << "median," << res.median_time_ms << "\n";
    res.csv = csv.str();

    return res;
}

// ─────────────────────────────────────────────────────────
// Bounded worker pool parallel solver
// ─────────────────────────────────────────────────────────
BenchmarkResult run_parallel(const std::vector<BenchCase>& cases, int mesh_size,
                              int num_workers, int repetitions,
                              std::function<void(int,int)> progress_cb) {
    int max_workers = get_logical_cores();
    int workers = std::min(num_workers, max_workers);
    if (workers < 1) workers = 1;

    // Warm-up
    {
        std::vector<CaseResult> warmup(cases.size());
        std::mutex q_mtx;
        std::atomic<size_t> idx{0};
        std::vector<std::thread> warm_threads;
        for (int w = 0; w < workers; ++w) {
            warm_threads.emplace_back([&]() {
                while (true) {
                    size_t i = idx.fetch_add(1);
                    if (i >= cases.size()) break;
                    warmup[i] = solve_case(cases[i], mesh_size);
                }
            });
        }
        for (auto& t : warm_threads) t.join();
    }

    BenchmarkResult res;
    res.num_workers    = workers;
    res.batch_size     = static_cast<int>(cases.size());
    res.mesh_size      = mesh_size;
    res.compiler_info  = "GCC " + std::string(__VERSION__);
    res.opt_flags      = "-O2";
    res.cpu_model      = get_cpu_model();
    res.logical_cores  = get_logical_cores();
    res.physical_cores = res.logical_cores / 2;
    res.ram_info       = get_ram_info();

    for (int r = 0; r < repetitions; ++r) {
        std::vector<CaseResult> results(cases.size());
        std::atomic<size_t> idx{0};
        std::atomic<int> done{0};

        res.all_times_ms[r] = time_ms([&]() {
            std::vector<std::thread> threads;
            for (int w = 0; w < workers; ++w) {
                threads.emplace_back([&]() {
                    while (true) {
                        size_t i = idx.fetch_add(1);
                        if (i >= cases.size()) break;
                        results[i] = solve_case(cases[i], mesh_size);
                        if (progress_cb) {
                            int d = done.fetch_add(1) + 1;
                            progress_cb(d, static_cast<int>(cases.size()));
                        }
                    }
                });
            }
            for (auto& t : threads) t.join();
        });

        if (r == repetitions - 1) {
            res.case_results = std::move(results);
        }
    }

    res.median_time_ms = median5(res.all_times_ms);
    // speedup and efficiency will be set by caller after comparing to serial
    res.speedup    = 0.0;
    res.efficiency = 0.0;

    std::ostringstream csv;
    csv << "repetition,time_ms\n";
    for (int r = 0; r < repetitions; ++r)
        csv << (r+1) << "," << res.all_times_ms[r] << "\n";
    csv << "median," << res.median_time_ms << "\n";
    res.csv = csv.str();

    return res;
}

} // namespace uew::performance
