
cmake_minimum_required(VERSION 3.20)

project(CFD_solver
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core library
add_library(cfd_core
    src/utils/Math.cpp
    src/meshio/MeshGenerator.cpp
    src/mesh/ConnectivityBuilder.cpp
    src/mesh/GeometryProcessor.cpp
)

# Headers
target_include_directories(cfd_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main application
add_executable(CFD_solver
    apps/main.cpp
)

# Link the core library to the main application
target_link_libraries(CFD_solver
    PRIVATE
        cfd_core
)

# ============================================================================================== #
# Enable testing
enable_testing()

# Mesh tests
add_executable(testMesh
    tests/mesh/testMesh.cpp
    tests/mesh/testGenerator.cpp
    tests/mesh/testConnectivity.cpp
    tests/mesh/testGeometry.cpp
)

target_link_libraries(testMesh PRIVATE cfd_core)

add_test(NAME Mesh COMMAND testMesh)

