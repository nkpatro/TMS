cmake_minimum_required(VERSION 3.16)
project(ActivityTracker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find required Qt packages
find_package(Qt6 COMPONENTS Core Network StateMachine REQUIRED)

# Determine the platform and include appropriate monitors
if(WIN32)
    add_subdirectory(src/monitors/win)
    set(PLATFORM_MONITORS windows_monitors)
    add_compile_definitions(Q_OS_WIN)
elseif(APPLE)
    add_subdirectory(src/monitors/mac)
    set(PLATFORM_MONITORS mac_monitors)
    add_compile_definitions(Q_OS_MACOS)
    # For macOS, we need to use Objective-C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")
    # Link to required macOS frameworks
    find_library(APPKIT AppKit)
    find_library(FOUNDATION Foundation)
    find_library(COREGRAPHICS CoreGraphics)
    set(MAC_FRAMEWORKS ${APPKIT} ${FOUNDATION} ${COREGRAPHICS})
elseif(UNIX)
    add_subdirectory(src/monitors/linux)
    set(PLATFORM_MONITORS linux_monitors)
    add_compile_definitions(Q_OS_LINUX)
    # Find X11 libraries for Linux
    find_package(X11 REQUIRED)
endif()

# Source files for core functionality
set(CORE_SOURCES
        src/core/ActivityTrackerClient.cpp
        src/core/ActivityMonitorBatcher.cpp
        src/core/APIManager.cpp
        src/core/SessionManager.cpp
        src/core/SessionStateMachine.cpp
        src/core/SyncManager.cpp
)

# Header files for core functionality
set(CORE_HEADERS
        src/core/ActivityTrackerClient.h
        src/core/ActivityMonitorBatcher.h
        src/core/APIManager.h
        src/core/SessionManager.h
        src/core/SessionStateMachine.h
        src/core/SyncManager.h
)

# Source files for manager components
set(MANAGER_SOURCES
        src/managers/ConfigManager.cpp
        src/managers/MonitorManager.cpp
)

# Header files for manager components
set(MANAGER_HEADERS
        src/managers/ConfigManager.h
        src/managers/MonitorManager.h
)

# Source files for abstract monitor base classes
set(MONITOR_BASE_SOURCES
        src/monitors/KeyboardMouseMonitor.cpp
        src/monitors/AppMonitor.cpp
        src/monitors/SessionMonitor.cpp
        src/monitors/SystemMonitor.cpp
)

# Header files for abstract monitor base classes
set(MONITOR_BASE_HEADERS
        src/monitors/KeyboardMouseMonitor.h
        src/monitors/AppMonitor.h
        src/monitors/SessionMonitor.h
        src/monitors/SystemMonitor.h
)

# Service application sources
set(SERVICE_SOURCES
        src/service/main.cpp
        src/service/ActivityTrackerService.cpp
        src/service/MultiUserManager.cpp
        src/service/ServiceManager.cpp
)

# Service application headers
set(SERVICE_HEADERS
        src/service/ActivityTrackerService.h
        src/service/MultiUserManager.h
        src/service/ServiceManager.h
)

# Create core library
add_library(activity_tracker_core
        ${CORE_SOURCES}
        ${CORE_HEADERS}
        ${MANAGER_SOURCES}
        ${MANAGER_HEADERS}
        ${MONITOR_BASE_SOURCES}
        ${MONITOR_BASE_HEADERS}
)

# Link core library dependencies
target_link_libraries(activity_tracker_core
        PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::StateMachine
        logger
)

target_include_directories(activity_tracker_core
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libs/logger/include
)

# Create service executable
add_executable(${PROJECT_NAME}
        ${SERVICE_SOURCES}
        ${SERVICE_HEADERS}
)

# Link service executable dependencies
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        activity_tracker_core
        ${PLATFORM_MONITORS}
        Qt6::Core
        Qt6::Network
        logger
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libs/logger/include
)

# Add platform-specific dependencies
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MAC_FRAMEWORKS})
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
endif()

# Install the service
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
)

# Install service configuration
install(FILES res/config/activity_tracker.conf
        DESTINATION etc/activity_tracker
)

# Install platform-specific service files
if(WIN32)
    install(FILES install/windows/install-service.bat
            DESTINATION bin
    )
    install(FILES install/windows/uninstall-service.bat
            DESTINATION bin
    )
    install(FILES install/windows/service-wrapper.xml
            DESTINATION bin
    )
elseif(APPLE)
    install(FILES install/macos/activity-tracker.plist
            DESTINATION Library/LaunchDaemons
    )
    install(FILES install/macos/install-service.sh
            DESTINATION bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    install(FILES install/macos/uninstall-service.sh
            DESTINATION bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
elseif(UNIX)
    install(FILES install/linux/activity-tracker.service
            DESTINATION lib/systemd/system
    )
    install(FILES install/linux/install-service.sh
            DESTINATION bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    install(FILES install/linux/uninstall-service.sh
            DESTINATION bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()

# Add client/reporting application if requested
option(BUILD_CLIENT "Build the reporting client application" OFF)

if(BUILD_CLIENT)
    add_subdirectory(src/client)
endif()

# Add testing if requested
option(BUILD_TESTS "Build tests for the application" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (WIN32)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Network HttpServer Sql WebSockets StateMachine)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
    # add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E copy
    #         "${CMAKE_BINARY_DIR}/restapi/librestapi.dll"
    #         "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    #    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #            COMMAND ${CMAKE_COMMAND} -E copy
    #            "$<TARGET_FILE:restapi>"
    #            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    # Create sqldrivers directory if it doesn't exist
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers")

    # Copy the QPSQL plugin
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_PREFIX_PATH}/plugins/sqldrivers/qsqlpsqld.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")

    # Copy PostgreSQL DLLs
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Program Files/PostgreSQL/17/bin/libpq.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Program Files/PostgreSQL/17/bin/libcrypto-3-x64.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Program Files/PostgreSQL/17/bin/libssl-3-x64.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Users/navinkumar.patro/AppData/Local/Programs/CLion/bin/gdb/win/x64/bin/libgcc_s_seh-1.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Users/navinkumar.patro/AppData/Local/Programs/CLion/bin/gdb/win/x64/bin/libstdc++-6.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/windows/system32/kernel32.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/windows/system32/SHELL32.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/windows/system32/msvcrt.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/windows/system32/ucrtbase${DEBUG_SUFFIX}.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/windows/system32/vcruntime140${DEBUG_SUFFIX}.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/windows/system32/MSVCP140${DEBUG_SUFFIX}.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Program Files/PostgreSQL/17/bin/libiconv-2.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "C:/Program Files/PostgreSQL/17/bin/libintl-9.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    # Copy logger DLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:logger>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif ()