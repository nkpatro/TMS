cmake_minimum_required(VERSION 3.26)
project(logger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find required Qt packages
find_package(Qt6 COMPONENTS Core REQUIRED)

# Source files
set(SOURCES
        src/logger.cpp
)

# Header files
set(HEADERS
        include/logger/logger.h
)

# Create the library
add_library(${PROJECT_NAME} SHARED
        ${SOURCES}
        ${HEADERS}
)

# Add the LOGGER_LIBRARY define to mark this as the implementation
target_compile_definitions(${PROJECT_NAME} PRIVATE LOGGER_LIBRARY)

# Set include directories
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        src
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Qt6::Core
)

# Set the version property
set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# MSVC specific compiler options
if(MSVC)
    # Disable warning about dll-interface for Qt meta-objects
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4251 /wd4273)
endif()

# Add installation targets
install(TARGETS ${PROJECT_NAME}
        EXPORT loggerTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(
        DIRECTORY include/logger
        DESTINATION include
)

# Export the targets
install(EXPORT loggerTargets
        FILE loggerTargets.cmake
        NAMESPACE logger::
        DESTINATION lib/cmake/logger
)
