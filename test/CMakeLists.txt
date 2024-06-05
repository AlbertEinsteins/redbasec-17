cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

file(GLOB_RECURSE REDBASE_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test.cpp")

# #####################################################################################################################
# MAKE TARGETS
# #####################################################################################################################

# #########################################
# "make XYZ_test"
# #########################################

foreach (redbase_test_source ${REDBASE_TEST_SOURCES})
    # Create a human readable name.
    get_filename_component(redbase_test_filename ${redbase_test_source} NAME)
    string(REPLACE ".cpp" "" redbase_test_name ${redbase_test_filename})

    message(${redbase_test_name})

    add_executable(${redbase_test_name} ${redbase_test_source})
#    add_dependencies(build-tests ${redbase_test_name})

    gtest_discover_tests(${redbase_test_name}
        EXTRA_ARGS
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test/${redbase_test_name}.xml
        --gtest_catch_exceptions=0
        DISCOVERY_TIMEOUT 120
        PROPERTIES
        TIMEOUT 120)

    # Add the test target separately and as part of "make check-tests".
    target_link_libraries(${redbase_test_name} redbase gtest gtest_main)

endforeach ()


