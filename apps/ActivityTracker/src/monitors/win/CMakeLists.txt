set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Source files for Windows monitors
set(WIN_MONITOR_SOURCES
        KeyboardMouseMonitorWin.cpp
        AppMonitorWin.cpp
        SessionMonitorWin.cpp
        SystemMonitorWin.cpp
)

# Header files for Windows monitors
set(WIN_MONITOR_HEADERS
        KeyboardMouseMonitorWin.h
        AppMonitorWin.h
        SessionMonitorWin.h
        SystemMonitorWin.h
)

# Create a library for Windows monitoring components
add_library(windows_monitors STATIC
        ${WIN_MONITOR_SOURCES}
        ${WIN_MONITOR_HEADERS}
)

# Link with Windows-specific libraries
target_link_libraries(windows_monitors
        PUBLIC
        Qt6::Core
        logger
        # Windows libraries required
        wtsapi32
        psapi
        pdh
)

# Include directories
target_include_directories(windows_monitors
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)