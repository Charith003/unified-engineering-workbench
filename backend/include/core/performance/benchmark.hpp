#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstdint>

namespace uew::performance {

/// Single thermal benchmark case (varied k and q, fixed seed)
struct BenchCase {
    int    case_id;
    double k;   ///< conductivity [W/(m·K)]
    double q;   ///< heat generation [W/m³]
};

/// Result for one case
struct CaseResult {
    int    case_id;
    double T_max;   ///< Maximum nodal temperature [°C]
    double T_min;   ///< Minimum nodal temperature [°C]
};

/// Timing / statistical result for one benchmark configuration
struct BenchmarkResult {
    int    num_workers;
    int    batch_size;
    int    mesh_size;
    double median_time_ms;     ///< Median over 5 repetitions
    double all_times_ms[5];    ///< All 5 repetitions
    double speedup;            ///< T_serial / T_parallel
    double efficiency;         ///< speedup / num_workers
    std::string cpu_model;
    int    physical_cores;
    int    logical_cores;
    std::string ram_info;
    std::string compiler_info;
    std::string opt_flags;
    std::vector<CaseResult> case_results;
    std::string csv;
};

/// Generate a deterministic batch of cases with fixed seed.
std::vector<BenchCase> generate_batch(int batch_size, uint64_t seed = 42);

/// Run serial benchmark: solve all cases in sequence.
BenchmarkResult run_serial(const std::vector<BenchCase>& cases, int mesh_size, int repetitions = 5);

/// Run parallel benchmark with a bounded worker pool.
BenchmarkResult run_parallel(const std::vector<BenchCase>& cases, int mesh_size,
                              int num_workers, int repetitions = 5,
                              std::function<void(int,int)> progress_cb = nullptr);

/// Get system info string
std::string get_cpu_model();
int get_logical_cores();
std::string get_ram_info();

} // namespace uew::performance
