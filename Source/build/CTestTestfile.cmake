# CMake generated Testfile for 
# Source directory: /home/lobre/CFD_solver/Source
# Build directory: /home/lobre/CFD_solver/Source/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[Mesh]=] "/home/lobre/CFD_solver/Tests/TestMesh.exe")
set_tests_properties([=[Mesh]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/lobre/CFD_solver/Source/CMakeLists.txt;92;add_test;/home/lobre/CFD_solver/Source/CMakeLists.txt;0;")
add_test([=[MeshConverter]=] "/home/lobre/CFD_solver/Tests/TestMeshConverter.exe")
set_tests_properties([=[MeshConverter]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/lobre/CFD_solver/Source/CMakeLists.txt;108;add_test;/home/lobre/CFD_solver/Source/CMakeLists.txt;0;")
