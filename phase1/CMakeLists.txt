cmake_minimum_required(VERSION 3.20)
project(CueForgePhase1 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "")
message(STATUS "╔════════════════════════════════════════════╗")
message(STATUS "║   CueForge Phase 1: Standalone JUCE        ║")
message(STATUS "║   NO Qt • NO Cues • Pure Audio Engine      ║")
message(STATUS "╚════════════════════════════════════════════╝")
message(STATUS "")

# JUCE Configuration (relative to THIS file location)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/JUCE")

if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at: ${JUCE_DIR}")
endif()

message(STATUS "✓ JUCE found: ${JUCE_DIR}")

option(JUCE_BUILD_EXTRAS "Build JUCE Extras" OFF)
option(JUCE_BUILD_EXAMPLES "Build JUCE Examples" OFF)

add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/juce)

# Phase 1 Audio Engine Library
add_library(JuceEngineStandalone STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/standalone/JuceEngineStandalone.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/standalone/JuceEngineStandalone.h
)

target_link_libraries(JuceEngineStandalone
    PUBLIC
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(JuceEngineStandalone PUBLIC
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_WINDOWS_MEDIA_FORMAT=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CAMERA=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

target_include_directories(JuceEngineStandalone PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Phase 1 Test Executable
add_executable(StandaloneTest
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/standalone/StandaloneTest.cpp
)

target_link_libraries(StandaloneTest
    PRIVATE
        JuceEngineStandalone
)

set_target_properties(StandaloneTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "")
message(STATUS "Build Targets:")
message(STATUS "  • JuceEngineStandalone (library)")
message(STATUS "  • StandaloneTest (executable)")
message(STATUS "")
message(STATUS "Usage after build:")
message(STATUS "  ./bin/StandaloneTest <audio_file.wav>")
message(STATUS "")