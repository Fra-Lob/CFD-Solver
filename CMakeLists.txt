
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
    src/meshio/MeshGenerator.cpp
    src/mesh/ConnectivityBuilder.cpp
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
add_executable(test_Mesh tests/test_Mesh.cpp)
target_link_libraries(test_Mesh PRIVATE cfd_core)
add_test(
    NAME Mesh
    COMMAND test_Mesh)

# Mesh generator
add_executable(test_MeshGenerator tests/test_MeshGenerator.cpp)
target_link_libraries(test_MeshGenerator PRIVATE cfd_core)
add_test(
    NAME MeshGenerator
    COMMAND test_MeshGenerator)

# Mesh connectivity
add_executable(test_MeshConnectivity tests/test_MeshConnectivity.cpp)
target_link_libraries(test_MeshConnectivity PRIVATE cfd_core)
add_test(
    NAME MeshConnectivity
    COMMAND test_MeshConnectivity)
