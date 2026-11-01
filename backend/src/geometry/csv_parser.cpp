#include "core/geometry/csv_parser.hpp"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <limits>

namespace uew::geometry {

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

CSVParseResult parse_temperature_csv(const std::string& content) {
    CSVParseResult res;

    if (content.empty()) {
        res.error = "Empty CSV file.";
        return res;
    }

    std::istringstream stream(content);
    std::string line;

    // Parse header
    if (!std::getline(stream, line)) {
        res.error = "Could not read header line.";
        return res;
    }
    std::string header = trim(line);
    // Normalize: lowercase, remove spaces
    std::string norm_header = header;
    std::transform(norm_header.begin(), norm_header.end(), norm_header.begin(), ::tolower);
    // Remove all spaces from the normalized header for comparison
    norm_header.erase(std::remove(norm_header.begin(), norm_header.end(), ' '), norm_header.end());

    const std::string expected = "node_id,x_m,y_m,z_m,temperature_c";
    if (norm_header != expected) {
        res.error = "Invalid CSV header. Expected: node_id,x_m,y_m,z_m,temperature_C. Got: " + header;
        return res;
    }

    double tmin =  std::numeric_limits<double>::max();
    double tmax = -std::numeric_limits<double>::max();
    int row = 1;

    while (std::getline(stream, line)) {
        ++row;
        std::string t = trim(line);
        if (t.empty()) continue;

        std::istringstream row_ss(t);
        std::string tok;
        std::vector<std::string> fields;
        while (std::getline(row_ss, tok, ',')) {
            fields.push_back(trim(tok));
        }

        if (fields.size() != 5) {
            res.error = "Row " + std::to_string(row) + ": expected 5 columns, got " + std::to_string(fields.size());
            return res;
        }

        TempNode node;
        try {
            node.node_id      = std::stoi(fields[0]);
            node.x            = std::stod(fields[1]);
            node.y            = std::stod(fields[2]);
            node.z            = std::stod(fields[3]);
            node.temperature_C = std::stod(fields[4]);
        } catch (const std::exception& e) {
            res.error = "Row " + std::to_string(row) + ": parse error: " + e.what();
            return res;
        }

        if (!std::isfinite(node.x) || !std::isfinite(node.y) || !std::isfinite(node.z)) {
            res.error = "Row " + std::to_string(row) + ": non-finite spatial coordinate.";
            return res;
        }
        if (!std::isfinite(node.temperature_C)) {
            res.error = "Row " + std::to_string(row) + ": non-finite temperature value.";
            return res;
        }

        tmin = std::min(tmin, node.temperature_C);
        tmax = std::max(tmax, node.temperature_C);
        res.nodes.push_back(node);
    }

    if (res.nodes.empty()) {
        res.error = "CSV contains header but no data rows.";
        return res;
    }

    res.t_min   = tmin;
    res.t_max   = tmax;
    res.success = true;
    return res;
}

} // namespace uew::geometry
