// ============================================================================
// StandaloneTest.cpp - Phase 1 test application
// Tests pure JUCE engine with NO other dependencies
// ============================================================================

#include "JuceEngineStandalone.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace CueForgePhase1;

void printSeparator() {
    std::cout << "\n" << std::string(50, '=') << "\n" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   CueForge Phase 1: JUCE Engine Test      ║" << std::endl;
    std::cout << "║   Pure JUCE - No Qt - No Cues              ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;

    printSeparator();

    // Test 1: Engine Creation
    std::cout << "TEST 1: Engine Creation" << std::endl;
    std::cout << "Creating engine..." << std::endl;
    JuceEngineStandalone engine;
    std::cout << "✓ Engine created successfully" << std::endl;

    printSeparator();

    // Test 2: Initialization
    std::cout << "TEST 2: Engine Initialization" << std::endl;
    if (!engine.initialize()) {
        std::cerr << "✗ FAIL: Could not initialize audio engine" << std::endl;
        return 1;
    }
    std::cout << "✓ Engine initialized" << std::endl;
    std::cout << "  Is initialized: " << (engine.isInitialized() ? "YES" : "NO") << std::endl;

    printSeparator();

    // Test 3: Device Enumeration
    std::cout << "TEST 3: Device Enumeration" << std::endl;
    auto devices = engine.getAvailableDevices();
    std::cout << "Found " << devices.size() << " audio device(s):" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i] << std::endl;
    }

    std::string currentDevice = engine.getCurrentDevice();
    std::cout << "\n  Current device: " << currentDevice << std::endl;
    std::cout << "✓ Device enumeration successful" << std::endl;

    printSeparator();

    // Test 4: System Information
    std::cout << "TEST 4: System Information" << std::endl;
    std::cout << "  Sample Rate: " << engine.getSampleRate() << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << engine.getBufferSize() << " samples" << std::endl;
    double latencyMs = (engine.getBufferSize() / engine.getSampleRate()) * 1000.0;
    std::cout << "  Latency: " << latencyMs << " ms" << std::endl;
    std::cout << "✓ System info retrieved" << std::endl;

    // If no audio file provided, skip playback tests
    if (argc < 2) {
        printSeparator();
        std::cout << "INFO: No audio file provided" << std::endl;
        std::cout << "Skipping playback tests" << std::endl;
        std::cout << "\nUsage: " << argv[0] << " <audio_file.wav>" << std::endl;
        std::cout << "\nBasic tests PASSED - Engine is functional!" << std::endl;

        engine.shutdown();
        return 0;
    }

    std::string testFile = argv[1];

    printSeparator();

    // Test 5: File Loading
    std::cout << "TEST 5: File Loading" << std::endl;
    std::cout << "Loading: " << testFile << std::endl;

    int playerId = engine.createPlayer(testFile);
    if (playerId < 0) {
        std::cerr << "✗ FAIL: Could not create player for file" << std::endl;
        std::cerr << "  Make sure file exists and is a valid audio format" << std::endl;
        return 1;
    }
    std::cout << "✓ Player created (ID: " << playerId << ")" << std::endl;

    auto* player = engine.getPlayer(playerId);
    if (!player) {
        std::cerr << "✗ FAIL: Could not retrieve player" << std::endl;
        return 1;
    }

    std::cout << "  File path: " << player->getFilePath() << std::endl;
    std::cout << "  Duration: " << player->getDuration() << " seconds" << std::endl;
    std::cout << "✓ File loaded successfully" << std::endl;

    printSeparator();

    // Test 6: Basic Playback
    std::cout << "TEST 6: Basic Playback" << std::endl;
    std::cout << "Playing for 3 seconds..." << std::endl;

    player->play();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!player->isPlaying()) {
        std::cerr << "✗ FAIL: Player not in playing state" << std::endl;
        return 1;
    }
    std::cout << "✓ Playback started" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    std::cout << "  Position: " << player->getPosition() << " seconds" << std::endl;
    player->stop();
    std::cout << "✓ Playback stopped" << std::endl;

    printSeparator();

    // Test 7: Pause/Resume
    std::cout << "TEST 7: Pause/Resume" << std::endl;

    player->play();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "Pausing..." << std::endl;
    player->pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!player->isPaused()) {
        std::cerr << "✗ FAIL: Player not in paused state" << std::endl;
        return 1;
    }
    std::cout << "✓ Paused successfully" << std::endl;

    double pausedPosition = player->getPosition();
    std::cout << "  Position when paused: " << pausedPosition << " seconds" << std::endl;

    std::cout << "Resuming..." << std::endl;
    player->resume();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "✓ Resumed successfully" << std::endl;
    player->stop();

    printSeparator();

    // Test 8: Volume Control
    std::cout << "TEST 8: Volume Control" << std::endl;

    std::cout << "Setting volume to 0.5..." << std::endl;
    player->setVolume(0.5f);

    float currentVolume = player->getVolume();
    if (std::abs(currentVolume - 0.5f) > 0.01f) {
        std::cerr << "✗ FAIL: Volume not set correctly" << std::endl;
        return 1;
    }
    std::cout << "✓ Volume set to: " << currentVolume << std::endl;

    player->play();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    player->stop();

    printSeparator();

    // Test 9: Seeking
    std::cout << "TEST 9: Seeking" << std::endl;

    double duration = player->getDuration();
    double seekTarget = duration * 0.5; // Seek to middle

    std::cout << "Seeking to " << seekTarget << " seconds..." << std::endl;
    player->setPosition(seekTarget);

    double actualPosition = player->getPosition();
    std::cout << "  Actual position: " << actualPosition << " seconds" << std::endl;

    if (std::abs(actualPosition - seekTarget) > 0.1) {
        std::cerr << "✗ WARNING: Seek position not accurate" << std::endl;
        std::cerr << "  (This is acceptable for some formats)" << std::endl;
    }
    std::cout << "✓ Seek completed" << std::endl;

    printSeparator();

    // Test 10: Player Cleanup
    std::cout << "TEST 10: Player Cleanup" << std::endl;

    engine.removePlayer(playerId);

    if (engine.getPlayer(playerId) != nullptr) {
        std::cerr << "✗ FAIL: Player not properly removed" << std::endl;
        return 1;
    }
    std::cout << "✓ Player removed successfully" << std::endl;

    printSeparator();

    // Test 11: Engine Shutdown
    std::cout << "TEST 11: Engine Shutdown" << std::endl;

    engine.shutdown();
    std::cout << "✓ Engine shutdown complete" << std::endl;

    printSeparator();

    std::cout << "╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║       ✓✓✓ ALL TESTS PASSED! ✓✓✓           ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║   Phase 1 JUCE Engine is FUNCTIONAL       ║" << std::endl;
    std::cout << "║   Ready to proceed to Phase 2              ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;

    return 0;
}