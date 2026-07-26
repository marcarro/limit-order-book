cmake_minimum_required(VERSION 3.14)
project(limit_orderbook VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create library target
add_library(orderbook_lib STATIC ${SOURCES})

function(apply_project_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(apply_release_flags target_name)
    if(NOT MSVC)
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Release>:-O3 -march=native>
        )
    endif()
endfunction()

# Create executables
add_executable(orderbook main.cpp)
target_link_libraries(orderbook orderbook_lib)

add_executable(orderbook_bench bench/orderbook_bench.cpp)
target_link_libraries(orderbook_bench orderbook_lib)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Build tests
add_subdirectory(tests)


apply_project_warnings(orderbook_lib)
apply_project_warnings(orderbook)
apply_project_warnings(orderbook_bench)
apply_release_flags(orderbook_lib)
apply_release_flags(orderbook)
apply_release_flags(orderbook_bench)

# Install targets
install(TARGETS orderbook DESTINATION bin)
install(TARGETS orderbook_bench DESTINATION bin)
install(TARGETS orderbook_lib DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Package configuration
set(CPACK_PACKAGE_NAME "limit-orderbook")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance limit orderbook implementation")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
