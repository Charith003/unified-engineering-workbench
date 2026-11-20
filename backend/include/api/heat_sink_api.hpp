#pragma once

#include "httplib.h"

namespace uew::api {

/**
 * @brief Registers all REST API endpoints for 3D Heat Sink simulation, meshing, and solver benchmarking.
 */
void register_heat_sink_routes(httplib::Server& svr);

} // namespace uew::api
