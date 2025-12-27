# Collect test source files
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit/*.cpp")
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")

# Unit tests executable
add_executable(unit_tests ${UNIT_TEST_SOURCES})
target_link_libraries(unit_tests 
  GTest::gtest_main
  GTest::gmock_main
  orderbook_lib
)

# Integration tests executable
add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(integration_tests 
  GTest::gtest_main
  GTest::gmock_main
  orderbook_lib
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)