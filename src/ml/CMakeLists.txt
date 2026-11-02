# Supported ONNX Runtime release. Keep in sync with README.md and .github/workflows/ci.yml.
set(CONDUCTOR_ONNXRUNTIME_VERSION 1.31)

find_package(ONNXRuntime ${CONDUCTOR_ONNXRUNTIME_VERSION} EXACT QUIET)
if(NOT ONNXRuntime_FOUND)
  message(FATAL_ERROR
    "ONNX Runtime ${CONDUCTOR_ONNXRUNTIME_VERSION}.x not found "
    "(detected version: '${ONNXRuntime_VERSION}', empty means not installed).\n"
    "Fix one of:\n"
    "  1. Install the SDK from https://github.com/microsoft/onnxruntime/releases/tag/v1.31.1\n"
    "     and configure with -DONNXRUNTIME_ROOT=<extracted dir>\n"
    "  2. Configure with -DCONDUCTOR_WITH_ML=OFF to build the core without ML")
endif()

add_library(conductor_ml onnx_lane_classifier.cpp)
target_link_libraries(conductor_ml PUBLIC conductor_core PRIVATE ONNXRuntime::ONNXRuntime)
