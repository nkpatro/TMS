cmake_minimum_required(VERSION 3.16)

project(ActivityTrackerAPI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer WebSockets)

# Set up automoc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Models
        ${CMAKE_CURRENT_SOURCE_DIR}/Repositories
        ${CMAKE_CURRENT_SOURCE_DIR}/Controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/Server
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/Core
)

# Define source files by directory
set(MODELS_SOURCES
        Models/UserModel.cpp
        Models/MachineModel.cpp
        Models/RoleModel.cpp
        Models/ActivityEventModel.cpp
        Models/AfkPeriodModel.cpp
        Models/ApplicationModel.cpp
        Models/AppUsageModel.cpp
        Models/DisciplineModel.cpp
#        Models/EventTypes.cpp
        Models/SessionModel.cpp
        Models/SessionEventModel.cpp
        Models/SystemMetricsModel.cpp
        Models/UserRoleDisciplineModel.cpp
)

# Define header files by directory
set(MODELS_HEADERS
        Models/UserModel.h
        Models/MachineModel.h
        Models/RoleModel.h
        Models/ActivityEventModel.h
        Models/AfkPeriodModel.h
        Models/AppUsageModel.h
        Models/ApplicationModel.h
        Models/DisciplineModel.h
        Models/EventTypes.h
        Models/SessionModel.h
        Models/SessionEventModel.h
        Models/SystemMetricsModel.h
        Models/UserRoleDisciplineModel.h
)

set(REPOSITORIES_SOURCES
        Repositories/UserRepository.cpp
        Repositories/RoleRepository.cpp
        Repositories/DisciplineRepository.cpp
        Repositories/MachineRepository.cpp
        Repositories/ActivityEventRepository.cpp
        Repositories/ApplicationRepository.cpp
        Repositories/SessionRepository.cpp
        Repositories/AfkPeriodRepository.cpp
        Repositories/AppUsageRepository.cpp
        Repositories/SystemMetricsRepository.cpp
        Repositories/SessionEventRepository.cpp
        Repositories/UserRoleDisciplineRepository.cpp
)

set(REPOSITORIES_HEADERS
        Repositories/BaseRepository.h
        Repositories/UserRepository.h
        Repositories/RoleRepository.h
        Repositories/DisciplineRepository.h
        Repositories/MachineRepository.h
        Repositories/ApplicationRepository.h
        Repositories/SessionRepository.h
        Repositories/AfkPeriodRepository.h
        Repositories/AppUsageRepository.h
        Repositories/SystemMetricsRepository.h
        Repositories/SessionEventRepository.h
        Repositories/UserRoleDisciplineRepository.h
)

set(CONTROLLERS_SOURCES
        Controllers/ApiControllerBase.cpp
        Controllers/AuthController.cpp
        Controllers/MachineController.cpp
        Controllers/SessionController.cpp
        Controllers/ApplicationController.cpp
        Controllers/SystemMetricsController.cpp
        Controllers/AppUsageController.cpp
        Controllers/ActivityEventController.cpp
        Controllers/SessionEventController.cpp
        Controllers/UserRoleDisciplineController.cpp
)

set(CONTROLLERS_HEADERS
        Controllers/ApiControllerBase.h
        Controllers/AuthController.h
        Controllers/MachineController.h
        Controllers/SessionController.h
        Controllers/ApplicationController.h
        Controllers/SystemMetricsController.h
        Controllers/AppUsageController.h
        Controllers/ActivityEventController.h
        Controllers/SessionEventController.h
        Controllers/UserRoleDisciplineController.h
)

set(SERVER_SOURCES
        Server/ApiServer.cpp
        Services/ADVerificationService.cpp
)

set(SERVER_HEADERS
        Server/ApiServer.h
        Services/ADVerificationService.h
)

set(UTILS_SOURCES
        Utils/SystemInfo.cpp
)

set(UTILS_HEADERS
        Utils/SystemInfo.h
)

set(CORE_SOURCES
        Core/AuthFramework.cpp
        Core/ModelFactory.cpp
)

set(CORE_HEADERS
        Core/AuthFramework.h
        Core/ModelFactory.h
)

# Combine all sources and headers
set(SOURCES
        main.cpp
        ActivityTrackerDbTemplates.cpp
        ${MODELS_SOURCES}
        ${REPOSITORIES_SOURCES}
        ${CONTROLLERS_SOURCES}
        ${SERVER_SOURCES}
        ${UTILS_SOURCES}
        ${CORE_SOURCES}
)

set(HEADERS
        ${MODELS_HEADERS}
        ${REPOSITORIES_HEADERS}
        ${CONTROLLERS_HEADERS}
        ${SERVER_HEADERS}
        ${UTILS_HEADERS}
        ${CORE_HEADERS}
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        Qt6::HttpServer
        Qt::WebSockets
        Http::Server
        Qt::DbService
        logger
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libs/logger/include
)

# Install the executable
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
)

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
    foreach (QT_LIB Core Gui Widgets Network HttpServer Sql WebSockets)
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