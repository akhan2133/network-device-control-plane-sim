cmake_minimum_required(VERSION 3.14)
project(ControlPlaneSimulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Find pthread
find_package(Threads REQUIRED)

# yaml-cpp setup (must be before targets that use it)
include(FetchContent)
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.7.0
)
FetchContent_MakeAvailable(yaml-cpp)

# Source files
set(SOURCES
    src/main.cpp
    src/port_state_machine.cpp
    src/port_manager.cpp
    src/event_loop.cpp
    src/http_server.cpp
    src/metrics.cpp
    src/config.cpp
    src/logger.cpp
)

# Main executable
add_executable(control_plane_sim ${SOURCES})
target_link_libraries(control_plane_sim PRIVATE 
    Threads::Threads
    yaml-cpp
)

# GoogleTest setup
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test sources
set(TEST_SOURCES
    tests/test_state_machine.cpp
    tests/test_determinism.cpp
    tests/test_thread_safety.cpp
    tests/test_config.cpp
    src/port_state_machine.cpp
    src/port_manager.cpp
    src/event_loop.cpp
    src/metrics.cpp
    src/config.cpp
    src/logger.cpp
)

add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests PRIVATE 
    GTest::gtest_main
    Threads::Threads
    yaml-cpp
)
# Pass source directory to tests for finding config files
target_compile_definitions(unit_tests PRIVATE 
    "PROJECT_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

# Custom targets
add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/control_plane_sim --config ${CMAKE_SOURCE_DIR}/config/config.yaml
    DEPENDS control_plane_sim
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running control plane simulator"
)
