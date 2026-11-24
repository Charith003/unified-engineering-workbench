#pragma once

#include "httplib.h"
#include "json.hpp"

namespace uew::api {

/**
 * @brief Processes a thermal solve request body directly, returning the response JSON and HTTP status code.
 * Used by the REST route handler and for direct in-process regression testing.
 */
nlohmann::json handle_thermal_solve_request(const nlohmann::json& body, int& http_status);

/**
 * @brief Registers all REST API endpoints for 3D Heat Sink simulation, meshing, and solver benchmarking.
 */
void register_heat_sink_routes(httplib::Server& svr);

} // namespace uew::api
