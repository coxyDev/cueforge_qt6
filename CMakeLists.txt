cmake_minimum_required(VERSION 3.20)

project(CueForge 
    VERSION 2.0.0
    DESCRIPTION "Professional show control software"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)

find_package(Qt6 COMPONENTS 
    Multimedia 
    MultimediaWidgets 
    SerialPort
)

if(Qt6Multimedia_FOUND)
    message(STATUS "✓ Qt6 Multimedia found")
    set(HAVE_MULTIMEDIA TRUE)
else()
    message(WARNING "✗ Qt6 Multimedia not found - Audio/Video disabled")
    set(HAVE_MULTIMEDIA FALSE)
endif()

if(Qt6SerialPort_FOUND)
    message(STATUS "✓ Qt6 SerialPort found")
    set(HAVE_SERIALPORT TRUE)
else()
    message(WARNING "✗ Qt6 SerialPort not found - MIDI disabled")
    set(HAVE_SERIALPORT FALSE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CUEFORGE_SOURCES
    src/main.cpp
    src/core/Cue.cpp
    src/core/CueManager.cpp
    src/core/ErrorHandler.cpp
    src/core/cues/AudioCue.cpp
    src/core/cues/GroupCue.cpp
    src/core/cues/WaitCue.cpp
    src/core/cues/ControlCue.cpp
    src/ui/MainWindow.cpp
    src/ui/CueTreeModel.cpp
    src/ui/CueListWidget.cpp
    src/ui/TransportWidget.cpp
    src/ui/InspectorWidget.cpp
)

set(CUEFORGE_HEADERS
    src/core/Cue.h
    src/core/CueManager.h
    src/core/ErrorHandler.h
    src/core/cues/AudioCue.h
    src/core/cues/GroupCue.h
    src/core/cues/WaitCue.h
    src/core/cues/ControlCue.h
    src/ui/MainWindow.h
    src/ui/CueTreeModel.h
    src/ui/CueListWidget.h
    src/ui/TransportWidget.h
    src/ui/InspectorWidget.h
)

add_executable(CueForge 
    ${CUEFORGE_SOURCES}
    ${CUEFORGE_HEADERS}
)

target_include_directories(CueForge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(CueForge PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

if(HAVE_MULTIMEDIA)
    target_link_libraries(CueForge PRIVATE 
        Qt6::Multimedia
        Qt6::MultimediaWidgets
    )
endif()

if(HAVE_SERIALPORT)
    target_link_libraries(CueForge PRIVATE Qt6::SerialPort)
endif()

if(MSVC)
    target_compile_options(CueForge PRIVATE /W4 /wd4251)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(CueForge PRIVATE -Wall -Wextra)
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "CueForge ${PROJECT_VERSION} Configuration")
message(STATUS "========================================")
message(STATUS "  Qt Version:    ${Qt6_VERSION}")
message(STATUS "  Multimedia:    ${HAVE_MULTIMEDIA}")
message(STATUS "  SerialPort:    ${HAVE_SERIALPORT}")
message(STATUS "  Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:      ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
message(STATUS "")