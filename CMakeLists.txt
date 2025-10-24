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
# JUCE Manual Configuration
# ============================================================================
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
set(ASIO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/asiosdk")

if(EXISTS "${JUCE_DIR}/modules")
    message(STATUS "✓ JUCE found at: ${JUCE_DIR}")
    set(HAVE_JUCE TRUE)
    
    # JUCE modules we need
    set(JUCE_MODULES_NEEDED
        juce_core
        juce_events
        juce_audio_basics
        juce_audio_devices
        juce_audio_formats
        juce_audio_utils
    )
    
    # Build include directories
    foreach(module ${JUCE_MODULES_NEEDED})
        list(APPEND JUCE_INCLUDE_DIRS "${JUCE_DIR}/modules")
    endforeach()
    
    if(EXISTS "${ASIO_SDK_DIR}")
        message(STATUS "✓ ASIO SDK found at: ${ASIO_SDK_DIR}")
        set(JUCE_ASIO_SUPPORT ON)
    else()
        message(WARNING "✗ ASIO SDK not found")
        set(JUCE_ASIO_SUPPORT OFF)
    endif()
else()
    message(FATAL_ERROR "✗ JUCE not found at: ${JUCE_DIR}")
endif()

# ============================================================================
# Qt6 Configuration
# ============================================================================
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
    message(WARNING "✗ Qt6 Multimedia not found")
    set(HAVE_MULTIMEDIA FALSE)
endif()

if(Qt6SerialPort_FOUND)
    message(STATUS "✓ Qt6 SerialPort found")
    set(HAVE_SERIALPORT TRUE)
else()
    message(WARNING "✗ Qt6 SerialPort not found")
    set(HAVE_SERIALPORT FALSE)
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
# JUCE Audio Engine (Separate Module Compilation)
# ============================================================================
if(HAVE_JUCE)
    set(AUDIO_ENGINE_SOURCES
        src/audio/JuceAudioEngine.cpp
        src/audio/AudioEngineQt.cpp
    )
    
    set(AUDIO_ENGINE_HEADERS
        src/audio/JuceAudioEngine.h
        src/audio/AudioEngineQt.h
    )
    
    # Compile JUCE modules separately (not as unity build)
    set(JUCE_MODULE_SOURCES
        "${JUCE_DIR}/modules/juce_core/juce_core.cpp"
        "${JUCE_DIR}/modules/juce_events/juce_events.cpp"
        "${JUCE_DIR}/modules/juce_audio_basics/juce_audio_basics.cpp"
        "${JUCE_DIR}/modules/juce_audio_devices/juce_audio_devices.cpp"
        "${JUCE_DIR}/modules/juce_audio_formats/juce_audio_formats.cpp"
        "${JUCE_DIR}/modules/juce_audio_utils/juce_audio_utils.cpp"
    )
    
    add_library(CueForgeAudioEngine STATIC
        ${AUDIO_ENGINE_SOURCES}
        ${AUDIO_ENGINE_HEADERS}
        ${JUCE_MODULE_SOURCES}
        "src/audio/JuceHeader.cpp"
        "src/audio/JuceColourStub.cpp"
    )
    
    target_include_directories(CueForgeAudioEngine PUBLIC
        ${JUCE_DIR}/modules
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_compile_definitions(CueForgeAudioEngine PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CAMERA=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=1
    )
    
    if(WIN32)
        target_compile_definitions(CueForgeAudioEngine PUBLIC
            JUCE_WASAPI=1
            JUCE_DIRECTSOUND=1
            _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING=1
        )
        
        if(JUCE_ASIO_SUPPORT)
            target_compile_definitions(CueForgeAudioEngine PUBLIC
                JUCE_ASIO=1
            )
            target_include_directories(CueForgeAudioEngine PUBLIC
                ${ASIO_SDK_DIR}/common
            )
        endif()
        
        target_link_libraries(CueForgeAudioEngine PUBLIC
            winmm
            ole32
            oleaut32
            uuid
            advapi32
            shell32
            ws2_32
            wininet
            version
            Shlwapi
        )
    endif()
    
    target_link_libraries(CueForgeAudioEngine PRIVATE
        Qt6::Core
    )
    
    if(MSVC)
        target_compile_options(CueForgeAudioEngine PRIVATE 
            /W3 
            /wd4267 
            /wd4244 
            /wd4100
            /wd4458
            /wd4456
            /bigobj
        )
    endif()
endif()

# ============================================================================
# Main Application
# ============================================================================
add_executable(CueForge 
    ${CUEFORGE_SOURCES}
    ${CUEFORGE_HEADERS}
 "src/audio/JuceHeader.cpp")

target_include_directories(CueForge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(CueForge PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

if(HAVE_JUCE)
    target_link_libraries(CueForge PRIVATE
        CueForgeAudioEngine
    )
    target_compile_definitions(CueForge PRIVATE
        HAVE_JUCE_AUDIO=1
    )
endif()

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
    target_compile_options(CueForge PRIVATE /W4 /wd4251 /bigobj)
    target_compile_definitions(CueForge PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "CueForge ${PROJECT_VERSION} Configuration")
message(STATUS "========================================")
message(STATUS "  Qt Version:    ${Qt6_VERSION}")
message(STATUS "  JUCE Audio:    ${HAVE_JUCE}")
if(HAVE_JUCE AND JUCE_ASIO_SUPPORT)
message(STATUS "  ASIO Support:  YES")
else()
message(STATUS "  ASIO Support:  NO")
endif()
message(STATUS "  Multimedia:    ${HAVE_MULTIMEDIA}")
message(STATUS "  SerialPort:    ${HAVE_SERIALPORT}")
message(STATUS "  Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:      ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
message(STATUS "")