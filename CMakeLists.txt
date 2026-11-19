cmake_minimum_required(VERSION 3.20)

# ========================================================================================= #
#                                Command-line shortcuts                                     #
# ========================================================================================= #
# The optional make/clean wrappers call this file in script mode. The normal CMake
# project starts below; source lists and compiler options are defined only there.

if(CMAKE_SCRIPT_MODE_FILE)
    file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}" project_root)

    # ------------------------------------------------------------------------------------- #
    # Configure and compile one build mode
    # ------------------------------------------------------------------------------------- #
    function(cfd_build build_type build_tests build_folder)
        set(build_directory "${project_root}/${build_folder}")

        message(STATUS "Configuring ${build_type} in ${build_directory} (tests: ${build_tests})")

        execute_process(
            COMMAND "${CMAKE_COMMAND}"
                -S "${project_root}"
                -B "${build_directory}"
                -G Ninja
                "-DCMAKE_BUILD_TYPE=${build_type}"
                "-DBUILD_TESTING=${build_tests}"
            RESULT_VARIABLE configure_result
        )

        if(NOT configure_result EQUAL 0)
            message(FATAL_ERROR "Configuration failed (${configure_result})")
        endif()

        message(STATUS "Compiling the project")

        execute_process(
            COMMAND "${CMAKE_COMMAND}" --build "${build_directory}" --parallel
            RESULT_VARIABLE build_result
        )

        if(NOT build_result EQUAL 0)
            message(FATAL_ERROR "Compilation failed (${build_result})")
        endif()

        message(STATUS "Build completed")
    endfunction()

    # ------------------------------------------------------------------------------------- #
    # Check ownership before removing any build directory
    # ------------------------------------------------------------------------------------- #
    function(cfd_is_project_build directory result)
        set(${result} FALSE PARENT_SCOPE)

        if(NOT IS_DIRECTORY "${directory}" OR IS_SYMLINK "${directory}")
            return()
        endif()

        if(NOT EXISTS "${directory}/CMakeCache.txt")
            return()
        endif()

        file(REAL_PATH "${directory}" resolved_directory)
        get_filename_component(parent_directory "${resolved_directory}" DIRECTORY)

        if(NOT parent_directory STREQUAL project_root)
            return()
        endif()

        file(STRINGS "${resolved_directory}/CMakeCache.txt" project_entry
            REGEX "^CMAKE_PROJECT_NAME:STATIC=CFD_solver$"
        )
        file(STRINGS "${resolved_directory}/CMakeCache.txt" source_entry
            REGEX "^CMAKE_HOME_DIRECTORY:INTERNAL="
        )
        string(REGEX REPLACE "^CMAKE_HOME_DIRECTORY:INTERNAL=" ""
            source_directory "${source_entry}"
        )

        # Recognize the same workspace when its cache was created through WSL.
        if(WIN32 AND source_directory MATCHES "^/mnt/([a-zA-Z])/(.*)$")
            set(source_directory "${CMAKE_MATCH_1}:/${CMAKE_MATCH_2}")
        elseif(UNIX AND project_root MATCHES "^/mnt/[a-zA-Z]/"
               AND source_directory MATCHES "^([a-zA-Z]):/(.*)$")
            string(TOLOWER "${CMAKE_MATCH_1}" drive_letter)
            set(source_directory "/mnt/${drive_letter}/${CMAKE_MATCH_2}")
        endif()

        if(WIN32)
            string(TOLOWER "${source_directory}" source_directory)
            string(TOLOWER "${project_root}" expected_source_directory)
        else()
            set(expected_source_directory "${project_root}")
        endif()

        if(project_entry AND source_directory STREQUAL expected_source_directory)
            set(${result} TRUE PARENT_SCOPE)
        endif()
    endfunction()

    # ------------------------------------------------------------------------------------- #
    # Remove generated build files while preserving case data
    # ------------------------------------------------------------------------------------- #
    function(cfd_clean)
        file(GLOB build_candidates LIST_DIRECTORIES TRUE
            "${project_root}/build"
            "${project_root}/build-*"
        )

        foreach(directory IN LISTS build_candidates)
            cfd_is_project_build("${directory}" is_project_build)

            if(NOT is_project_build)
                message(STATUS "Preserving unrelated directory: ${directory}")
                continue()
            endif()

            # Ownership and the resolved parent were checked above.
            file(REAL_PATH "${directory}" resolved_directory)
            message(STATUS "Removing build directory: ${resolved_directory}")
            file(REMOVE_RECURSE "${resolved_directory}")

            if(EXISTS "${resolved_directory}")
                message(FATAL_ERROR "Could not completely remove ${resolved_directory}")
            endif()
        endforeach()

        # Also remove application copies and older platform-specific executable names.
        set(executable_names
            Main Grid mesh_preprocessor
            TestMesh TestVTKIO TestMeshProperties TestPreprocessor
        )

        foreach(target_name IN LISTS executable_names)
            foreach(suffix "" ".exe" ".exe.exe")
                set(executable "${project_root}/${target_name}${suffix}")

                if(NOT EXISTS "${executable}" OR IS_DIRECTORY "${executable}"
                   OR IS_SYMLINK "${executable}")
                    continue()
                endif()

                message(STATUS "Removing executable: ${executable}")
                file(REMOVE "${executable}")

                if(EXISTS "${executable}")
                    message(FATAL_ERROR "Could not remove ${executable}; close any running executable")
                endif()
            endforeach()
        endforeach()

        message(STATUS "Clean completed; source files and case meshes were preserved")
    endfunction()

    # ------------------------------------------------------------------------------------- #
    # Dispatch the requested shortcut
    # ------------------------------------------------------------------------------------- #
    if(CFD_ACTION STREQUAL "build")
        cfd_build(Release OFF build-native)
    elseif(CFD_ACTION STREQUAL "debug")
        cfd_build(Debug ON build-debug)
    elseif(CFD_ACTION STREQUAL "clean")
        cfd_clean()
    else()
        message(FATAL_ERROR "Use -DCFD_ACTION=build, debug or clean in script mode")
    endif()

    return()
endif()

# ========================================================================================= #
#                               Project and compiler settings                               #
# ========================================================================================= #

project(CFD_solver
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Each build owns its binaries. Applications are also copied to the project root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(CFD_ENABLE_GMSH "Enable custom import using the optional Gmsh SDK" ON)
option(BUILD_TESTING "Build the mesh test executables and register CTest tests" OFF)

if(UNIX)
    file(CHMOD "${CMAKE_SOURCE_DIR}/clean"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()

# ========================================================================================= #
#                                      Core library                                         #
# ========================================================================================= #

add_library(cfd_core
    source/src/utils/Math.cpp

    source/src/mesh/ConnectivityBuilder.cpp
    source/src/mesh/GeometryProcessor.cpp
    source/src/mesh/BoundaryPatchBuilder.cpp
    source/src/mesh/MeshValidator.cpp

    source/src/meshio/GridConfigReader.cpp
    source/src/meshio/MeshGenerator.cpp
    source/src/meshio/GmshImporter.cpp
    source/src/meshio/Preprocessor.cpp
    source/src/meshio/VTKWriter.cpp
    source/src/meshio/VTKReader.cpp
    source/src/meshio/NativeMeshIO.cpp
)

target_include_directories(cfd_core
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source/include"
)

# ========================================================================================= #
#                                Optional Gmsh integration                                  #
# ========================================================================================= #

if(CFD_ENABLE_GMSH)
    find_path(GMSH_INCLUDE_DIR gmsh.h
        HINTS ${GMSH_ROOT} ENV GMSH_ROOT
        PATH_SUFFIXES include
    )
    find_library(GMSH_LIBRARY NAMES gmsh
        HINTS ${GMSH_ROOT} ENV GMSH_ROOT
        PATH_SUFFIXES lib bin
    )
endif()

if(CFD_ENABLE_GMSH AND GMSH_INCLUDE_DIR AND GMSH_LIBRARY)
    target_include_directories(cfd_core PRIVATE "${GMSH_INCLUDE_DIR}")
    target_link_libraries(cfd_core PUBLIC "${GMSH_LIBRARY}")
    target_compile_definitions(cfd_core PUBLIC CFD_HAS_GMSH=1)

    message(STATUS "Gmsh API enabled")
else()
    target_compile_definitions(cfd_core PUBLIC CFD_HAS_GMSH=0)

    message(STATUS "Gmsh SDK unavailable or disabled: internal generators remain available")
endif()

# ========================================================================================= #
#                                      Applications                                         #
# ========================================================================================= #

add_executable(mesh_preprocessor source/apps/preprocessor.cpp)
target_link_libraries(mesh_preprocessor PRIVATE cfd_core)

add_executable(Main source/apps/main.cpp)
target_link_libraries(Main PRIVATE cfd_core)

# Always publish the selected mode, even when its binaries need no recompilation.
add_custom_target(publish_apps ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:Main>"
        "${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:Main>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:mesh_preprocessor>"
        "${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:mesh_preprocessor>"
    DEPENDS Main mesh_preprocessor
    COMMENT "Updating application executables in the project root"
    VERBATIM
)

# ========================================================================================= #
#                                    Optional test suite                                    #
# ========================================================================================= #

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
    target_compile_definitions(TestPreprocessor
        PRIVATE CFD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    foreach(group config generated native pipeline gmsh)
        add_test(NAME Preprocessor_${group} COMMAND TestPreprocessor ${group})
    endforeach()

    set_target_properties(TestMesh TestVTKIO TestMeshProperties TestPreprocessor
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
endif()
