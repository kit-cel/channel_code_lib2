cmake_minimum_required(VERSION 3.27)
project(channel_coding)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-g -O2 -fPIC -w")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(testing "Enable testing" OFF)
option(python "Enable python bindings" ON)



find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# --- nlohmann_json ---
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
file(COPY ${nlohmann_json_SOURCE_DIR}/json.hpp
     DESTINATION ${nlohmann_json_SOURCE_DIR}/nlohmann)

if(python)
  find_package(pybind11 CONFIG REQUIRED)
endif()
# Create c++ library (channel_coding_lib)

add_subdirectory(src)
add_library(channel_coding_lib SHARED
  $<TARGET_OBJECTS:decoder>
  $<TARGET_OBJECTS:encoder>
  $<TARGET_OBJECTS:simenv>
)
target_include_directories(channel_coding_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(channel_coding_lib PRIVATE OpenMP::OpenMP_CXX)
add_library(channel_coding_lib_static STATIC
  $<TARGET_OBJECTS:decoder>
  $<TARGET_OBJECTS:encoder>
  $<TARGET_OBJECTS:simenv>
)
target_include_directories(channel_coding_lib_static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${nlohmann_json_SOURCE_DIR}
)
target_link_libraries(channel_coding_lib_static PRIVATE OpenMP::OpenMP_CXX)



# -----------------------------
# Add unit tests
if(testing)
  enable_testing()
  add_subdirectory(tests)
endif()



# Create python module
if(python)
  pybind11_add_module(channel_coding src/bindings.cpp)

  target_link_libraries(channel_coding PRIVATE
    channel_coding_lib_static
    OpenMP::OpenMP_CXX
    pybind11::module
  )


  ### INSTALL MODULE TO PYTHON ENVIRONMENT ###

  # Ensure Python executable is defined
  if(NOT DEFINED PYTHON_EXECUTABLE)
    find_package(Python REQUIRED COMPONENTS Interpreter Development)
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
  endif()

  # Get the site-packages directory of the python environment
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )

  # Convert the path to CMake format
  file(TO_CMAKE_PATH "${PYTHON_SITE_PACKAGES}" PYTHON_SITE_PACKAGES)
  message(STATUS "Python site-packages directory: ${PYTHON_SITE_PACKAGES}")

  # Install the Python module into the environment's site-packages directory
  install(TARGETS channel_coding
    DESTINATION channel_code_lib2
    COMPONENT pythonapi)
endif()
