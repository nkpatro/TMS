project(dbservice VERSION 1.0.0 LANGUAGES CXX)

# Create library
add_library(${PROJECT_NAME}
        src/dbconfig.cpp
        src/dbmanager.cpp
)

# Create alias for use in other parts of the project
add_library(Qt::DbService ALIAS ${PROJECT_NAME})

# Define headers for this library
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Set properties for MSVC
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Find the Logger library
#find_package(logger REQUIRED)

# Link Qt libraries and Logger
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Qt::Core
        Qt::Sql
        logger
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

install(DIRECTORY include/dbservice
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE Qt::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)