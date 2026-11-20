cmake_minimum_required(VERSION 3.16)
project(cinema_booking VERSION 1.0.0 LANGUAGES CXX)

# D1: C++17, standard only (no compiler extensions) for portability.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows: link the C++ runtime statically by DEFAULT so cinema_cli.exe runs on any PC without
# installing the "Visual C++ Redistributable" (important for non-technical users).
# It is only a default: a consumer whose own code uses the dynamic runtime (/MD, the MSVC and
# Conan default) must build this library with the same one, otherwise linking fails with
# LNK2038 "mismatch detected for 'RuntimeLibrary'". Override it with
#   -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL   (the generator expression below also works)
# Conan sets this itself, so `conan create` follows the profile's compiler.runtime setting.
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(CINEMA_BUILD_TESTS "Build the tests" ON)
option(CINEMA_BUILD_CLI   "Build the interactive CLI" ON)

# The whole backend (+ the CLI loop, which takes streams so it can be tested).
add_library(cinema STATIC src/seat.cpp src/booking_service.cpp src/cli.cpp)
add_library(cinema::cinema ALIAS cinema)
target_include_directories(cinema PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(cinema PUBLIC cxx_std_17)
find_package(Threads REQUIRED)  # std::mutex / std::thread need pthread on Linux
target_link_libraries(cinema PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(cinema PRIVATE /W4 /permissive-)
else()
    target_compile_options(cinema PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CINEMA_BUILD_CLI)
    add_executable(cinema_cli app/main.cpp)
    target_link_libraries(cinema_cli PRIVATE cinema)
endif()

# Tests need nothing but the C++ standard library (see tests/tests.cpp).
if(CINEMA_BUILD_TESTS)
    enable_testing()
    add_executable(cinema_tests tests/tests.cpp)
    target_link_libraries(cinema_tests PRIVATE cinema)
    add_test(NAME cinema_tests COMMAND cinema_tests)
endif()

# Bonus (Doxygen): `cmake --build build --target docs`, only if Doxygen is installed.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()

# Install rules (used by the Conan package, bonus).
include(GNUInstallDirs)
install(TARGETS cinema ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
