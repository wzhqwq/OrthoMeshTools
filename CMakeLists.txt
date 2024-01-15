cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
project ("gumTrimLine" C CXX)

add_compile_definitions(NOMINMAX)

find_package(Eigen3 REQUIRED)
find_package(CGAL REQUIRED)
find_package(OpenMP REQUIRED)
find_package(assimp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(pybind11 CONFIG)
find_package(Ceres)
find_package(argparse)
include(CGAL_Eigen3_support)
include(CGAL_Ceres_support)

add_library(Ortho "Polyhedron.cpp" "Polyhedron.h" "Ortho.h")
target_link_libraries(Ortho PUBLIC CGAL::CGAL CGAL::Eigen3_support nlohmann_json::nlohmann_json OpenMP::OpenMP_CXX assimp::assimp)

if(pybind11_FOUND)
    pybind11_add_module(gumTrimLine "PyBind.cpp" "print.cpp" "MeshFix/MeshFix.cpp" "Polyhedron.cpp" "Polyhedron.h"
     "GumTrimLine/GumTrimLine.cpp")
    target_link_libraries(gumTrimLine PUBLIC Ortho)
    target_compile_definitions(gumTrimLine PUBLIC FOUND_PYBIND11)
endif()