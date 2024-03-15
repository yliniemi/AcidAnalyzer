cmake_minimum_required(VERSION 3.10)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PROJECT_NAME ${PROJECT_NAME})
project(${PROJECT_NAME} C)

# OpenGL Flags
set(OpenGL_GL_PREFERENCE GLVND)

# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(glad REQUIRED)
# find_package(ALSA REQUIRED)

pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ../../glfw/include ../../glfw/deps ../../OpenSimplex2/_old/c)
link_libraries(PkgConfig::PIPEWIRE m fftw3 fftw3_threads ncursesw pthread glfw OpenGL::GL)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ALSA_LIBRARIES_DIR})
# link_libraries(PkgConfig::PIPEWIRE m fftw3 fftw3_threads ncursesw pthread ${ALSA_LIBRARIES})

add_executable(${PROJECT_NAME} main.c kaiser.c ringBuffer.c drawNcurses.c properFFTalgorithm.c defaults.h nanoTime.c drawGLFW.c ../../OpenSimplex2/_old/c/OpenSimplex2F.c approxFunctions.c readPipewire.c dummyAudio.c)
add_executable(test test.c kaiser.c ringBuffer.c drawNcurses.c properFFTalgorithm.c defaults.h nanoTime.c drawGLFW.c ../../OpenSimplex2/_old/c/OpenSimplex2F.c approxFunctions.c readPipewire.c dummyAudio.c)

# Enable multithreading in FFTW
# add_definitions(-DFFTW_ENABLE_THREADS)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
