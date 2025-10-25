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

# ============================================================================
# JUCE 8 Configuration
# ============================================================================
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
set(ASIO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/asiosdk")

# Check JUCE exists
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "✗ JUCE not found at: ${JUCE_DIR}")
endif()

message(STATUS "✓ JUCE 8 found at: ${JUCE_DIR}")

# JUCE build options - set BEFORE add_subdirectory
option(JUCE_BUILD_EXTRAS "Build JUCE Extras" OFF)
option(JUCE_BUILD_EXAMPLES "Build JUCE Examples" OFF)

# Set ASIO SDK path for JUCE (before add_subdirectory)
if(EXISTS "${ASIO_SDK_DIR}")
    message(STATUS "✓ ASIO SDK found at: ${ASIO_SDK_DIR}")
    set(JUCE_ASIOSDK_PATH "${ASIO_SDK_DIR}" CACHE PATH "Path to ASIO SDK" FORCE)
endif()

# Add JUCE to the build - this makes juce:: targets available
add_subdirectory(${JUCE_DIR})

set(HAVE_JUCE TRUE)

# ============================================================================
# Qt6 Configuration
# ============================================================================
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt6 COMPONENTS Multimedia MultimediaWidgets QUIET)

if(Qt6Multimedia_FOUND)
    message(STATUS "✓ Qt6 Multimedia found")
    set(HAVE_MULTIMEDIA TRUE)
endif()

find_package(Qt6SerialPort QUIET)
if(Qt6SerialPort_FOUND)
    message(STATUS "✓ Qt6 SerialPort found")
    set(HAVE_SERIALPORT TRUE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ============================================================================
# Source Files
# ============================================================================
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

# ============================================================================
# JUCE Audio Engine - Using JUCE 8 Module Targets
# ============================================================================
add_library(CueForgeAudioEngine STATIC
    src/audio/JuceAudioEngine.cpp
    src/audio/JuceAudioEngine.h
    src/audio/AudioEngineQt.cpp
    src/audio/AudioEngineQt.h
)

# Link to JUCE 8 modules - JUCE handles ALL dependencies
target_link_libraries(CueForgeAudioEngine
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
        Qt6::Core
)

# Audio format configuration
target_compile_definitions(CueForgeAudioEngine PUBLIC
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_WINDOWS_MEDIA_FORMAT=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CAMERA=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# ASIO support - NOW the target exists!
if(EXISTS "${ASIO_SDK_DIR}")
    target_compile_definitions(CueForgeAudioEngine PUBLIC JUCE_ASIO=1)
    target_include_directories(CueForgeAudioEngine PRIVATE 
        ${ASIO_SDK_DIR}/common
    )
    message(STATUS "✓ ASIO headers added: ${ASIO_SDK_DIR}/common")
endif()

target_include_directories(CueForgeAudioEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Main Application
# ============================================================================
add_executable(CueForge ${CUEFORGE_SOURCES} ${CUEFORGE_HEADERS})

target_include_directories(CueForge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(CueForge PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    CueForgeAudioEngine
)

target_compile_definitions(CueForge PRIVATE HAVE_JUCE_AUDIO=1)

if(HAVE_MULTIMEDIA)
    target_link_libraries(CueForge PRIVATE Qt6::Multimedia Qt6::MultimediaWidgets)
endif()

if(HAVE_SERIALPORT)
    target_link_libraries(CueForge PRIVATE Qt6::SerialPort)
endif()

if(MSVC)
    target_compile_options(CueForge PRIVATE /W4 /wd4251)
    target_compile_definitions(CueForge PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "CueForge ${PROJECT_VERSION} Configuration")
message(STATUS "========================================")
message(STATUS "  JUCE Version:  8.x")
message(STATUS "  Qt Version:    ${Qt6_VERSION}")
if(EXISTS "${ASIO_SDK_DIR}")
message(STATUS "  ASIO Support:  YES")
else()
message(STATUS "  ASIO Support:  NO")
endif()
message(STATUS "  Multimedia:    ${HAVE_MULTIMEDIA}")
message(STATUS "  SerialPort:    ${HAVE_SERIALPORT}")
message(STATUS "  Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
message(STATUS "")