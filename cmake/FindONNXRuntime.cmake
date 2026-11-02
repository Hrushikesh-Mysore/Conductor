# Finds a system-installed ONNX Runtime SDK (release tarball, distro package, or source install).
# Hint the location with -DONNXRUNTIME_ROOT=<dir> or the ONNXRUNTIME_ROOT environment variable.
# Defines: ONNXRuntime_FOUND, ONNXRuntime_VERSION (major.minor), target ONNXRuntime::ONNXRuntime

find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
  HINTS ${ONNXRUNTIME_ROOT} ENV ONNXRUNTIME_ROOT
  PATH_SUFFIXES include include/onnxruntime include/onnxruntime/core/session)
find_library(ONNXRUNTIME_LIBRARY onnxruntime
  HINTS ${ONNXRUNTIME_ROOT} ENV ONNXRUNTIME_ROOT
  PATH_SUFFIXES lib lib64)

# The headers carry only the API version; ORT_API_VERSION N ships with release 1.N.
if(ONNXRUNTIME_INCLUDE_DIR AND EXISTS "${ONNXRUNTIME_INCLUDE_DIR}/onnxruntime_c_api.h")
  file(STRINGS "${ONNXRUNTIME_INCLUDE_DIR}/onnxruntime_c_api.h" _ort_api_line
    REGEX "^#define ORT_API_VERSION [0-9]+")
  string(REGEX REPLACE "^#define ORT_API_VERSION ([0-9]+).*" "1.\\1" ONNXRuntime_VERSION "${_ort_api_line}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRuntime
  REQUIRED_VARS ONNXRUNTIME_LIBRARY ONNXRUNTIME_INCLUDE_DIR
  VERSION_VAR ONNXRuntime_VERSION)

if(ONNXRuntime_FOUND AND NOT TARGET ONNXRuntime::ONNXRuntime)
  add_library(ONNXRuntime::ONNXRuntime UNKNOWN IMPORTED)
  set_target_properties(ONNXRuntime::ONNXRuntime PROPERTIES
    IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}")
endif()

mark_as_advanced(ONNXRUNTIME_INCLUDE_DIR ONNXRUNTIME_LIBRARY)
