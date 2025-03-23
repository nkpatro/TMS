cmake_minimum_required(VERSION 3.26)
project(dna VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_PREFIX_PATH "C:/Qt/6.8.0/llvm-mingw_64")
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.0/msvc2022_64")

# MSVC specific compiler options
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Find Qt packages
find_package(Qt6 COMPONENTS
        Core
        Network
        HttpServer
        WebSockets
        Sql
        REQUIRED
)

# Build the shared libraries first
add_subdirectory(libs/logger)
add_subdirectory(libs/dbservice)
add_subdirectory(libs/httpserver)

# Build the REST server application
add_subdirectory(apps/ActivityTrackerAPI)
add_subdirectory(apps/ActivityTracker)

# Add tests
#add_subdirectory(tests)

