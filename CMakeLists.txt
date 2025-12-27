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

# Create executable target
add_executable(orderbook main.cpp)
target_link_libraries(orderbook orderbook_lib)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Build tests
add_subdirectory(tests)


# Compiler warnings and optimization
if(MSVC)
    target_compile_options(orderbook_lib PRIVATE /W4)
    target_compile_options(orderbook PRIVATE /W4)
else()
    target_compile_options(orderbook_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(orderbook PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install targets
install(TARGETS orderbook DESTINATION bin)
install(TARGETS orderbook_lib DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Package configuration
set(CPACK_PACKAGE_NAME "limit-orderbook")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance limit orderbook implementation")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
