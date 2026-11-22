
cmake_minimum_required(VERSION 3.20)

project(CFD_solver
    VERSION 0.1.0
    LANGUAGES CXX
)

# ============================================================
# Project directories
# ============================================================

# CMakeLists.txt is inside Source/
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# CFD_solver/ is the parent directory of Source/
get_filename_component(ROOT_DIR "${SOURCE_DIR}" DIRECTORY)

set(BIN_DIR "${ROOT_DIR}/Bin")
set(TESTS_DIR "${ROOT_DIR}/Tests")

# Main executables go to Bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

# ============================================================
# C++ configuration
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Core library
# ============================================================

add_library(cfd_core
    src/utils/Math.cpp
    src/meshio/MeshGenerator.cpp
    src/meshio/MeshConverter.cpp
    src/meshio/VTKConverter.cpp
    src/mesh/ConnectivityBuilder.cpp
    src/mesh/GeometryBuilder.cpp
    src/mesh/BoundaryPatchBuilder.cpp
    src/process/Diffusion.cpp
)

target_include_directories(cfd_core
    PUBLIC
        "${SOURCE_DIR}/include"
)

# ============================================================
# Main application
# ============================================================

add_executable(Main.exe
    apps/main.cpp
)

target_link_libraries(Main.exe PRIVATE cfd_core)

# ============================================================
# Grid generation
# ============================================================

add_executable(Grid.exe
    apps/grid.cpp
)

target_link_libraries(Grid.exe PRIVATE cfd_core)

# ============================================================
# Testing
# ============================================================

enable_testing()

set(TEST_INCLUDE_DIR "${SOURCE_DIR}/tests")

function(add_test_runner target_name output_name runner_header test_name)
    set_source_files_properties("${runner_header}" PROPERTIES LANGUAGE CXX)
    add_executable("${target_name}" "${runner_header}")
    target_include_directories("${target_name}" PRIVATE "${TEST_INCLUDE_DIR}")
    target_link_libraries("${target_name}" PRIVATE cfd_core)
    set_target_properties("${target_name}" PROPERTIES
        OUTPUT_NAME "${output_name}"
        RUNTIME_OUTPUT_DIRECTORY "${TESTS_DIR}"
    )
    add_test(NAME "${test_name}" COMMAND "${target_name}")
endfunction()

add_test_runner(
    MeshTests
    MeshTests.exe
    "${SOURCE_DIR}/tests/mesh/MeshTests.hpp"
    Mesh
)

add_test_runner(
    MeshIOTests
    MeshIOTests.exe
    "${SOURCE_DIR}/tests/meshIO/MeshIOTests.hpp"
    MeshIO
)

add_test_runner(
    NumericsTests
    NumericsTests.exe
    "${SOURCE_DIR}/tests/numerics/NumericsTests.hpp"
    Numerics
)

add_test_runner(
    FieldsTests
    FieldsTests.exe
    "${SOURCE_DIR}/tests/fields/FieldsTests.hpp"
    Fields
)

add_test_runner(
    ProcessTests
    ProcessTests.exe
    "${SOURCE_DIR}/tests/process/ProcessTests.hpp"
    Process
)

