cmake_minimum_required(VERSION 3.20)
project(UEW VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────────────────────────────────────
# Compiler flags
# ─────────────────────────────────────────────────────────
if(MSVC)
  add_compile_options(/W3 /O2 /EHsc /bigobj)
  add_compile_definitions(_WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)
else()
  # GCC / MinGW
  add_compile_options(-Wall -Wextra -O2)
  if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)
  endif()
endif()

# ─────────────────────────────────────────────────────────
# Include paths
# ─────────────────────────────────────────────────────────
set(BACKEND_INC ${CMAKE_SOURCE_DIR}/backend/include)
set(THIRD_PARTY ${CMAKE_SOURCE_DIR}/backend/third_party)

include_directories(
  ${BACKEND_INC}
  ${THIRD_PARTY}
)

# ─────────────────────────────────────────────────────────
# Core library (all engineering computation modules)
# ─────────────────────────────────────────────────────────
add_library(uew_core STATIC
  backend/src/geometry/stl_parser.cpp
  backend/src/geometry/csv_parser.cpp
  backend/src/thermal/fem_solver.cpp
  backend/src/performance/benchmark.cpp
  backend/src/hardware/adapters.cpp
  backend/src/backprop/network.cpp
  backend/src/api/job_manager.cpp
)
target_include_directories(uew_core PUBLIC ${BACKEND_INC} ${THIRD_PARTY})

if(WIN32 AND NOT MSVC)
  target_link_libraries(uew_core PUBLIC ws2_32 mswsock)
endif()
if(MSVC)
  target_link_libraries(uew_core PUBLIC ws2_32)
endif()

# ─────────────────────────────────────────────────────────
# Main server executable
# ─────────────────────────────────────────────────────────
add_executable(uew_server backend/src/main.cpp)
target_link_libraries(uew_server PRIVATE uew_core)

if(WIN32 AND NOT MSVC)
  target_link_libraries(uew_server PRIVATE ws2_32 mswsock)
endif()
if(MSVC)
  target_link_libraries(uew_server PRIVATE ws2_32)
endif()

# ─────────────────────────────────────────────────────────
# GoogleTest — downloaded via FetchContent
# ─────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ─────────────────────────────────────────────────────────
# Test executables
# ─────────────────────────────────────────────────────────
function(add_uew_test name sources)
  add_executable(${name} ${sources})
  target_link_libraries(${name} PRIVATE uew_core GTest::gtest GTest::gtest_main)
  target_include_directories(${name} PRIVATE ${BACKEND_INC} ${THIRD_PARTY})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_uew_test(test_geometry
  "tests/geometry/test_stl_parser.cpp;tests/geometry/test_csv_parser.cpp")

add_uew_test(test_thermal
  "tests/thermal/test_fem_solver.cpp")

add_uew_test(test_performance
  "tests/performance/test_benchmark.cpp")

add_uew_test(test_hardware
  "tests/hardware/test_adapters.cpp")

add_uew_test(test_backprop
  "tests/backprop/test_network.cpp")

# ─────────────────────────────────────────────────────────
# Output binaries to build/bin
# ─────────────────────────────────────────────────────────
set_target_properties(uew_server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
