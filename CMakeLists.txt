
cmake_minimum_required(VERSION 3.20)

# Lightweight command wrappers reuse this file in CMake script mode.
if(CMAKE_SCRIPT_MODE_FILE)
    file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}" project_root)
    if(CFD_ACTION STREQUAL "build" OR CFD_ACTION STREQUAL "debug")
        if(CFD_ACTION STREQUAL "debug")
            set(build_directory "${project_root}/build-debug")
            set(build_type Debug)
            set(build_tests ON)
        else()
            set(build_directory "${project_root}/build-native")
            set(build_type Release)
            set(build_tests OFF)
        endif()
        message(STATUS "Configuring ${build_type} in ${build_directory} (tests: ${build_tests})")
        execute_process(COMMAND "${CMAKE_COMMAND}" -S "${project_root}" -B "${build_directory}"
            -G Ninja "-DCMAKE_BUILD_TYPE=${build_type}" "-DBUILD_TESTING=${build_tests}" RESULT_VARIABLE result)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Configuration failed (${result})")
        endif()
        message(STATUS "Compiling the project")
        execute_process(COMMAND "${CMAKE_COMMAND}" --build "${build_directory}"
            --parallel RESULT_VARIABLE result)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Compilation failed (${result})")
        endif()
        message(STATUS "Build completed")
    elseif(CFD_ACTION STREQUAL "clean")
        # Only direct build/build-* children with this project's CMake cache
        # are removable. Resolve paths before any recursive removal.
        file(GLOB candidates LIST_DIRECTORIES true "${project_root}/build" "${project_root}/build-*")
        foreach(directory IN LISTS candidates)
            if(NOT IS_DIRECTORY "${directory}" OR IS_SYMLINK "${directory}" OR NOT EXISTS "${directory}/CMakeCache.txt")
                continue()
            endif()
            file(REAL_PATH "${directory}" resolved)
            get_filename_component(parent "${resolved}" DIRECTORY)
            if(NOT parent STREQUAL project_root)
                message(WARNING "Skipping build directory outside the project: ${directory}")
                continue()
            endif()
            file(STRINGS "${resolved}/CMakeCache.txt" project_line REGEX "^CMAKE_PROJECT_NAME:STATIC=CFD_solver$")
            file(STRINGS "${resolved}/CMakeCache.txt" source_line REGEX "^CMAKE_HOME_DIRECTORY:INTERNAL=")
            string(REGEX REPLACE "^CMAKE_HOME_DIRECTORY:INTERNAL=" "" source_root "${source_line}")
            # Recognize caches produced by WSL for the same Windows workspace.
            if(WIN32 AND source_root MATCHES "^/mnt/([a-zA-Z])/(.*)$")
                set(source_root "${CMAKE_MATCH_1}:/${CMAKE_MATCH_2}")
            elseif(UNIX AND project_root MATCHES "^/mnt/[a-zA-Z]/" AND source_root MATCHES "^([a-zA-Z]):/(.*)$")
                string(TOLOWER "${CMAKE_MATCH_1}" drive)
                set(source_root "/mnt/${drive}/${CMAKE_MATCH_2}")
            endif()
            if(WIN32)
                string(TOLOWER "${source_root}" source_root)
                string(TOLOWER "${project_root}" expected_source_root)
            else()
                set(expected_source_root "${project_root}")
            endif()
            if(NOT project_line OR NOT source_root STREQUAL expected_source_root)
                message(STATUS "Skipping unrelated build directory: ${resolved}")
                continue()
            endif()
            message(STATUS "Removing build directory: ${resolved}")
            file(REMOVE_RECURSE "${resolved}")
            if(EXISTS "${resolved}")
                message(FATAL_ERROR "Could not completely remove ${resolved}")
            endif()
        endforeach()
        # Includes legacy Windows double suffixes and WSL/Linux executables.
        foreach(target Main Grid mesh_preprocessor TestMesh TestVTKIO TestMeshProperties TestPreprocessor)
            foreach(suffix "" ".exe" ".exe.exe")
                set(binary "${project_root}/${target}${suffix}")
                if(EXISTS "${binary}" AND NOT IS_DIRECTORY "${binary}" AND NOT IS_SYMLINK "${binary}")
                    message(STATUS "Removing executable: ${binary}")
                    file(REMOVE "${binary}")
                    if(EXISTS "${binary}")
                        message(FATAL_ERROR "Could not remove ${binary}; close any running executable")
                    endif()
                endif()
            endforeach()
        endforeach()
        message(STATUS "Clean completed; source files and case meshes were preserved")
    else()
        message(FATAL_ERROR "Use -DCFD_ACTION=build, debug or clean in script mode")
    endif()
    return()
endif()

project(CFD_solver
    VERSION 0.1.0
    LANGUAGES CXX
)

if(UNIX)
    file(CHMOD "${CMAKE_SOURCE_DIR}/clean" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# Keep each build's binaries separate so switching modes cannot reuse another
# build's executable. The applications are also published at the source root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core library
add_library(cfd_core
    src/utils/Math.cpp
    src/meshio/MeshGenerator.cpp
    src/meshio/GridConfigReader.cpp
    src/meshio/NativeMeshIO.cpp
    src/meshio/GmshImporter.cpp
    src/meshio/Preprocessor.cpp
    src/meshio/VTKWriter.cpp
    src/meshio/VTKReader.cpp
    src/mesh/ConnectivityBuilder.cpp
    src/mesh/GeometryProcessor.cpp
    src/mesh/MeshValidator.cpp
    src/mesh/BoundaryPatchBuilder.cpp
)

# Headers
target_include_directories(cfd_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(CFD_ENABLE_GMSH "Enable custom import using the optional Gmsh SDK" ON)
if(CFD_ENABLE_GMSH)
    find_path(GMSH_INCLUDE_DIR gmsh.h HINTS ${GMSH_ROOT} ENV GMSH_ROOT PATH_SUFFIXES include)
    find_library(GMSH_LIBRARY NAMES gmsh HINTS ${GMSH_ROOT} ENV GMSH_ROOT PATH_SUFFIXES lib bin)
endif()
if(CFD_ENABLE_GMSH AND GMSH_INCLUDE_DIR AND GMSH_LIBRARY)
    target_include_directories(cfd_core PRIVATE ${GMSH_INCLUDE_DIR})
    target_link_libraries(cfd_core PUBLIC ${GMSH_LIBRARY})
    target_compile_definitions(cfd_core PUBLIC CFD_HAS_GMSH=1)
    message(STATUS "Gmsh API enabled")
else()
    target_compile_definitions(cfd_core PUBLIC CFD_HAS_GMSH=0)
    message(STATUS "Gmsh SDK unavailable or disabled: internal generators remain available")
endif()

add_executable(mesh_preprocessor apps/preprocessor.cpp)
target_link_libraries(mesh_preprocessor PRIVATE cfd_core)

# Main application
add_executable(Main
    apps/main.cpp
)

# Link the core library to the main application
target_link_libraries(Main PRIVATE cfd_core)

add_custom_target(publish_apps ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:Main>" "${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:Main>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:mesh_preprocessor>" "${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:mesh_preprocessor>"
    DEPENDS Main mesh_preprocessor
    COMMENT "Updating application executables in the project root"
    VERBATIM)

# ============================================================================================== #
# Tests are opt-in; the debug wrapper enables them explicitly.
option(BUILD_TESTING "Build the mesh test executables and register CTest tests" OFF)
if(BUILD_TESTING)
    enable_testing()

    add_executable(TestMesh
        tests/mesh/testMesh.cpp
        tests/mesh/testGenerator.cpp
        tests/mesh/testConnectivity.cpp
        tests/mesh/testGeometry.cpp
    )
    target_link_libraries(TestMesh PRIVATE cfd_core)
    add_test(NAME Mesh COMMAND TestMesh)

    add_executable(TestVTKIO tests/mesh/testVTKIO.cpp)
    target_link_libraries(TestVTKIO PRIVATE cfd_core)
    add_test(NAME VTKIO COMMAND TestVTKIO)

    add_executable(TestMeshProperties tests/mesh/testMeshProperties.cpp)
    target_link_libraries(TestMeshProperties PRIVATE cfd_core)
    add_test(NAME MeshProperties COMMAND TestMeshProperties)

    add_executable(TestPreprocessor tests/mesh/testPreprocessor.cpp)
    target_link_libraries(TestPreprocessor PRIVATE cfd_core)
    target_compile_definitions(TestPreprocessor PRIVATE CFD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    foreach(group config generated native pipeline gmsh)
        add_test(NAME Preprocessor_${group} COMMAND TestPreprocessor ${group})
    endforeach()

    set_target_properties(TestMesh TestVTKIO TestMeshProperties TestPreprocessor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endif()

