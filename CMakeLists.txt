cmake_minimum_required(VERSION 3.13)

project(btwm VERSION 0.1 LANGUAGES CXX)

find_package(Freetype REQUIRED)
find_package(Fontconfig REQUIRED) #Dependencies of Xft
find_package(X11 REQUIRED)

include(cmake/compiler_warnings.cmake)

message(STATUS "X11_Xft_FOUND >${X11_Xft_FOUND}<")

add_executable(btwm btwm.cpp)
target_compile_features(btwm PUBLIC cxx_std_17)
target_link_libraries(btwm PUBLIC
	btwm::compiler_warnings
	X11::X11)

target_include_directories(btwm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
