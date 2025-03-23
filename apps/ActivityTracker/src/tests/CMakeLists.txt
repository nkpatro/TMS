find_package(Qt6 REQUIRED COMPONENTS Test)

# Define test files
set(TEST_SOURCES
        ConfigManagerTest.cpp
        # Add more test files as they're created
)

# Create tests executable
add_executable(activity_tracker_tests ${TEST_SOURCES})

# Link with Qt Test framework and core library
target_link_libraries(activity_tracker_tests
        PRIVATE
        activity_tracker_core
        Qt6::Test
        Qt6::Core
        logger
)

target_include_directories(activity_tracker_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Automatically discover and run tests
foreach(test_file ${TEST_SOURCES})
    # Extract test name from file name
    get_filename_component(test_name ${test_file} NAME_WE)

    # Add test to CTest
    add_test(
            NAME ${test_name}
            COMMAND activity_tracker_tests ${test_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()
