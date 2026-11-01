#pragma once
#include <string>
#include <vector>

namespace uew::geometry {

/// A single temperature node from a CSV file.
struct TempNode {
    int    node_id;
    double x, y, z;       ///< metres
    double temperature_C; ///< degrees Celsius
};

struct CSVParseResult {
    bool success{false};
    std::string error;
    std::vector<TempNode> nodes;
    double t_min{0.0}, t_max{0.0};
};

/// Parse a CSV with header: node_id,x_m,y_m,z_m,temperature_C
/// Validates headers, numeric values, and finite coordinates.
CSVParseResult parse_temperature_csv(const std::string& content);

} // namespace uew::geometry
