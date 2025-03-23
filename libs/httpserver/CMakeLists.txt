project(httpserver VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Network HttpServer WebSockets REQUIRED)

set(SOURCES
        src/controller.cpp
        src/server.cpp
        src/response.cpp
)

set(HEADERS
        include/httpserver/controller.h
        include/httpserver/server.h
        include/httpserver/response.h
)

add_library(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
)

add_library(Http::Server ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/libs/logger/include
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Qt::Core
        Qt::Network
        Qt::HttpServer
        Qt::WebSockets
)

# Installation rules
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        DIRECTORY include/httpserver
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE Http::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

#include(CMakePackageConfigHelpers)
#
#write_basic_package_version_file(
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#        VERSION ${PROJECT_VERSION}
#        COMPATIBILITY SameMajorVersion
#)
#
#configure_package_config_file(
#        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#)
#
#install(
#        FILES
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#)
#
## Testing
#if(BUILD_TESTING)
#    enable_testing()
#    add_subdirectory(tests)
#endif()
