cmake_minimum_required(VERSION 3.24...3.31)
project(conductor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(CONDUCTOR_WITH_ML "Build the ONNX Runtime ML component (src/ml)" ON)
option(CONDUCTOR_BUILD_TESTS "Build tests" ON)

find_package(SQLite3 REQUIRED)
# Newer CMake renamed the imported target; older releases only know SQLite::SQLite3.
if(TARGET SQLite3::SQLite3)
  set(CONDUCTOR_SQLITE_TARGET SQLite3::SQLite3)
else()
  set(CONDUCTOR_SQLITE_TARGET SQLite::SQLite3)
endif()

# Core: task model, state machine, router, worker interface. No third-party deps.
add_library(conductor_core
  src/core/task.cpp
  src/core/router.cpp
  src/core/brief.cpp
  src/core/dispatch.cpp)
target_include_directories(conductor_core PUBLIC src)

# Store: SQLite persistence.
add_library(conductor_store src/store/store.cpp)
target_link_libraries(conductor_store PUBLIC conductor_core ${CONDUCTOR_SQLITE_TARGET})

# Adapters: workers that drive vendor CLIs headless.
add_library(conductor_adapters
  src/adapters/cli_worker.cpp
  src/adapters/http_worker.cpp
  src/adapters/manual_worker.cpp)
target_link_libraries(conductor_adapters PUBLIC conductor_core)

# Planner API: the surface any planner AI uses; MCP exposes it to MCP-capable planners.
add_library(conductor_planner src/planner/planner_api.cpp)
target_link_libraries(conductor_planner PUBLIC conductor_store)
add_library(conductor_mcp src/mcp/mcp_server.cpp)
target_link_libraries(conductor_mcp PUBLIC conductor_planner)

if(CONDUCTOR_WITH_ML)
  add_subdirectory(src/ml)
endif()

add_executable(conductor src/app/main.cpp)
target_link_libraries(conductor PRIVATE conductor_store conductor_adapters)
if(CONDUCTOR_WITH_ML)
  target_link_libraries(conductor PRIVATE conductor_ml)
  target_compile_definitions(conductor PRIVATE CONDUCTOR_WITH_ML)
endif()

if(CONDUCTOR_BUILD_TESTS)
  enable_testing()
  add_executable(conductor_tests tests/test_main.cpp)
  target_link_libraries(conductor_tests PRIVATE conductor_store conductor_adapters)
  add_test(NAME conductor_tests COMMAND conductor_tests)
endif()
