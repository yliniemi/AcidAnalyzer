cmake_minimum_required(VERSION 3.10)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PROJECT_NAME ${PROJECT_NAME})
project(${PROJECT_NAME} C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

add_executable(${PROJECT_NAME} main.c)
add_executable(test test.c)

# Enable multithreading in FFTW
# add_definitions(-DFFTW_ENABLE_THREADS)

target_link_libraries(${PROJECT_NAME} PkgConfig::PIPEWIRE m fftw3 fftw3_threads ncursesw pthread blas)
target_link_libraries(test PkgConfig::PIPEWIRE m fftw3 fftw3_threads ncursesw pthread blas)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

