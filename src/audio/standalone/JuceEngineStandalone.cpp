// ============================================================================
// JuceEngineStandalone.cpp - Pure JUCE audio engine implementation (Phase 1)
// CueForge Qt6 - Professional show control software
// NO Qt dependencies, NO Cue dependencies - Pure C++ and JUCE
// ============================================================================

#include "JuceEngineStandalone.h"
#include <iostream>

namespace CueForgePhase1 {

    // ============================================================================
    // JuceEngineStandalone Implementation
    // ============================================================================

    JuceEngineStandalone::JuceEngineStandalone()
        : nextPlayerId_(1)
        , initialized_(false)
    {
        // Register audio formats - basic formats included by default
        formatManager_.registerBasicFormats(); // WAV, AIFF

        // Register additional formats if available
#if JUCE_USE_MP3AUDIOFORMAT
        formatManager_.registerFormat(new juce::MP3AudioFormat(), true);
#endif

#if JUCE_USE_WINDOWS_MEDIA_FORMAT
        formatManager_.registerFormat(new juce::WindowsMediaAudioFormat(), true);
#endif

#if JUCE_USE_FLAC
        formatManager_.registerFormat(new juce::FlacAudioFormat(), true);
#endif

#if JUCE_USE_OGGVORBIS
        formatManager_.registerFormat(new juce::OggVorbisAudioFormat(), true);
#endif

        std::cout << "JuceEngineStandalone: Constructor completed" << std::endl;
    }

    JuceEngineStandalone::~JuceEngineStandalone()
    {
        shutdown();
        std::cout << "JuceEngineStandalone: Destructor completed" << std::endl;
    }

    bool JuceEngineStandalone::initialize()
    {
        if (initialized_) {
            std::cout << "JuceEngineStandalone: Already initialized" << std::endl;
            return true;
        }

        std::cout << "JuceEngineStandalone: Initializing..." << std::endl;

        // Initialize with default device
        // 0 input channels, 2 output channels (stereo)
        juce::String error = deviceManager_.initialise(
            0,          // numInputChannelsNeeded
            2,          // numOutputChannelsNeeded (stereo)
            nullptr,    // preferredDefaultDeviceTypeName
            true        // selectDefaultDeviceOnFailure
        );

        if (error.isNotEmpty()) {
            std::cerr << "Audio device init failed: " << error.toStdString() << std::endl;
            return false;
        }

        // Add this engine as the audio callback
        deviceManager_.addAudioCallback(this);

        initialized_ = true;

        // Print initialization info
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (device) {
            std::cout << "Audio device initialized:" << std::endl;
            std::cout << "  Device: " << device->getName().toStdString() << std::endl;
            std::cout << "  Sample Rate: " << device->getCurrentSampleRate() << " Hz" << std::endl;
            std::cout << "  Buffer Size: " << device->getCurrentBufferSizeSamples() << " samples" << std::endl;

            double latencyMs = (device->getCurrentBufferSizeSamples() / device->getCurrentSampleRate()) * 1000.0;
            std::cout << "  Latency: " << latencyMs << " ms" << std::endl;
        }

        return true;
    }

    void JuceEngineStandalone::shutdown()
    {
        if (!initialized_) {
            return;
        }

        std::cout << "JuceEngineStandalone: Shutting down..." << std::endl;

        // Remove audio callback first
        deviceManager_.removeAudioCallback(this);

        // Clean up all players
        {
            juce::ScopedLock lock(playerLock_);

            // Stop and remove all players
            for (auto& pair : players_) {
                std::cout << "  Removing player " << pair.first << std::endl;
                mixer_.removeInputSource(pair.second->getTransportSource());
            }

            players_.clear();
        }

        // Close audio device
        deviceManager_.closeAudioDevice();

        initialized_ = false;
        std::cout << "JuceEngineStandalone: Shutdown complete" << std::endl;
    }

    std::vector<std::string> JuceEngineStandalone::getAvailableDevices()
    {
        std::vector<std::string> devices;

        auto& deviceTypes = deviceManager_.getAvailableDeviceTypes();

        for (auto* type : deviceTypes) {
            type->scanForDevices(); // Refresh device list

            auto deviceNames = type->getDeviceNames(false); // false = output devices

            for (const auto& name : deviceNames) {
                devices.push_back(name.toStdString());
            }
        }

        return devices;
    }

    std::string JuceEngineStandalone::getCurrentDevice() const
    {
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (device) {
            return device->getName().toStdString();
        }
        return "";
    }

    bool JuceEngineStandalone::setDevice(const std::string& deviceName)
    {
        if (!initialized_) {
            std::cerr << "Cannot set device - engine not initialized" << std::endl;
            return false;
        }

        // Find the device type that contains this device
        auto& deviceTypes = deviceManager_.getAvailableDeviceTypes();

        for (auto* type : deviceTypes) {
            auto deviceNames = type->getDeviceNames(false); // output devices

            if (deviceNames.contains(juce::String(deviceName))) {
                // Setup device
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                deviceManager_.getAudioDeviceSetup(setup);

                setup.outputDeviceName = deviceName;

                juce::String error = deviceManager_.setAudioDeviceSetup(setup, true);

                if (error.isEmpty()) {
                    std::cout << "Device changed to: " << deviceName << std::endl;
                    return true;
                }
                else {
                    std::cerr << "Failed to set device: " << error.toStdString() << std::endl;
                    return false;
                }
            }
        }

        std::cerr << "Device not found: " << deviceName << std::endl;
        return false;
    }

    void JuceEngineStandalone::audioDeviceIOCallbackWithContext(
        const float* const* /*inputChannelData*/,
        int /*numInputChannels*/,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& /*context*/)
    {
        // CRITICAL: This runs on the audio thread - must be real-time safe!
        // NO allocations, NO locks (except very brief ones), NO I/O

        // Create audio buffer for output
        juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);

        // Clear buffer first
        buffer.clear();

        // Get audio from mixer (mixes all playing sources)
        juce::AudioSourceChannelInfo channelInfo;
        channelInfo.buffer = &buffer;
        channelInfo.startSample = 0;
        channelInfo.numSamples = numSamples;

        mixer_.getNextAudioBlock(channelInfo);
    }

    void JuceEngineStandalone::audioDeviceAboutToStart(juce::AudioIODevice* device)
    {
        if (!device) {
            return;
        }

        std::cout << "Audio device starting:" << std::endl;
        std::cout << "  Sample Rate: " << device->getCurrentSampleRate() << " Hz" << std::endl;
        std::cout << "  Buffer Size: " << device->getCurrentBufferSizeSamples() << " samples" << std::endl;

        // Prepare mixer for playback
        mixer_.prepareToPlay(
            device->getCurrentBufferSizeSamples(),
            device->getCurrentSampleRate()
        );
    }

    void JuceEngineStandalone::audioDeviceStopped()
    {
        std::cout << "Audio device stopped" << std::endl;
        mixer_.releaseResources();
    }

    int JuceEngineStandalone::createPlayer(const std::string& filePath)
    {
        if (!initialized_) {
            std::cerr << "Cannot create player - engine not initialized" << std::endl;
            return -1;
        }

        juce::ScopedLock lock(playerLock_);

        int playerId = nextPlayerId_++;

        std::cout << "Creating player " << playerId << " for: " << filePath << std::endl;

        auto player = std::make_unique<AudioPlayer>(this, playerId);

        if (!player->loadFile(filePath)) {
            std::cerr << "Failed to load file: " << filePath << std::endl;
            return -1;
        }

        // Add player's audio source to mixer
        // false = don't delete source when removed
        mixer_.addInputSource(player->getTransportSource(), false);

        players_[playerId] = std::move(player);

        std::cout << "Player " << playerId << " created successfully" << std::endl;

        return playerId;
    }

    void JuceEngineStandalone::removePlayer(int playerId)
    {
        juce::ScopedLock lock(playerLock_);

        auto it = players_.find(playerId);
        if (it != players_.end()) {
            std::cout << "Removing player " << playerId << std::endl;

            // Remove from mixer
            mixer_.removeInputSource(it->second->getTransportSource());

            // Delete player (unique_ptr handles cleanup)
            players_.erase(it);
        }
        else {
            std::cerr << "Player " << playerId << " not found" << std::endl;
        }
    }

    AudioPlayer* JuceEngineStandalone::getPlayer(int playerId)
    {
        juce::ScopedLock lock(playerLock_);

        auto it = players_.find(playerId);
        if (it != players_.end()) {
            return it->second.get();
        }

        return nullptr;
    }

    double JuceEngineStandalone::getSampleRate() const
    {
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (device) {
            return device->getCurrentSampleRate();
        }
        return 44100.0; // Default fallback
    }

    int JuceEngineStandalone::getBufferSize() const
    {
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (device) {
            return device->getCurrentBufferSizeSamples();
        }
        return 512; // Default fallback
    }

    // ============================================================================
    // AudioPlayer Implementation
    // ============================================================================

    AudioPlayer::AudioPlayer(JuceEngineStandalone* engine, int id)
        : engine_(engine)
        , id_(id)
        , volume_(1.0f)
        , loaded_(false)
    {
        std::cout << "AudioPlayer " << id_ << ": Constructor" << std::endl;
    }

    AudioPlayer::~AudioPlayer()
    {
        std::cout << "AudioPlayer " << id_ << ": Destructor" << std::endl;
        unload();
    }

    bool AudioPlayer::loadFile(const std::string& filePath)
    {
        // Clean up any existing file
        unload();

        std::cout << "AudioPlayer " << id_ << ": Loading file: " << filePath << std::endl;

        juce::File file(filePath);

        // Check file exists
        if (!file.existsAsFile()) {
            std::cerr << "File not found: " << filePath << std::endl;
            return false;
        }

        // Create reader for the file
        auto* reader = engine_->formatManager_.createReaderFor(file);
        if (!reader) {
            std::cerr << "Could not create reader for: " << filePath << std::endl;
            std::cerr << "Supported formats: WAV, AIFF";
#if JUCE_USE_MP3AUDIOFORMAT
            std::cerr << ", MP3";
#endif
#if JUCE_USE_WINDOWS_MEDIA_FORMAT
            std::cerr << ", WMA";
#endif
            std::cerr << std::endl;
            return false;
        }

        // Create reader source
        // true = reader source takes ownership of reader and will delete it
        readerSource_ = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

        // Set up transport source
        // prepareToPlay will be called by the mixer when audio starts
        transportSource_.setSource(
            readerSource_.get(),
            0,                          // readAheadBufferSize (0 = use default)
            nullptr,                    // backgroundThread (nullptr = read on audio thread)
            reader->sampleRate,         // sourceSampleRate
            reader->numChannels         // maxNumChannels
        );

        // Store file path
        filePath_ = filePath;
        loaded_ = true;

        // Print file info
        std::cout << "AudioPlayer " << id_ << ": File loaded successfully" << std::endl;
        std::cout << "  Duration: " << getDuration() << " seconds" << std::endl;
        std::cout << "  Channels: " << reader->numChannels << std::endl;
        std::cout << "  Sample Rate: " << reader->sampleRate << " Hz" << std::endl;
        std::cout << "  Bit Depth: " << reader->bitsPerSample << " bits" << std::endl;

        return true;
    }

    void AudioPlayer::unload()
    {
        if (!loaded_) {
            return;
        }

        std::cout << "AudioPlayer " << id_ << ": Unloading file" << std::endl;

        // Stop playback
        stop();

        // Clear transport source
        transportSource_.setSource(nullptr);

        // Delete reader source (which deletes the reader)
        readerSource_.reset();

        loaded_ = false;
        filePath_.clear();
    }

    void AudioPlayer::play()
    {
        if (!loaded_) {
            std::cerr << "AudioPlayer " << id_ << ": Cannot play - no file loaded" << std::endl;
            return;
        }

        std::cout << "AudioPlayer " << id_ << ": Playing" << std::endl;
        transportSource_.start();
    }

    void AudioPlayer::stop()
    {
        std::cout << "AudioPlayer " << id_ << ": Stopping" << std::endl;
        transportSource_.stop();
        transportSource_.setPosition(0.0);
    }

    void AudioPlayer::pause()
    {
        std::cout << "AudioPlayer " << id_ << ": Pausing" << std::endl;
        transportSource_.stop();
    }

    void AudioPlayer::resume()
    {
        if (!loaded_) {
            std::cerr << "AudioPlayer " << id_ << ": Cannot resume - no file loaded" << std::endl;
            return;
        }

        std::cout << "AudioPlayer " << id_ << ": Resuming" << std::endl;
        transportSource_.start();
    }

    bool AudioPlayer::isPlaying() const
    {
        return transportSource_.isPlaying();
    }

    bool AudioPlayer::isPaused() const
    {
        // Paused = file loaded, not playing, position > 0
        return loaded_ &&
            !transportSource_.isPlaying() &&
            getPosition() > 0.0;
    }

    void AudioPlayer::setVolume(float volume)
    {
        // Clamp volume to valid range
        volume_ = juce::jlimit(0.0f, 1.0f, volume);

        std::cout << "AudioPlayer " << id_ << ": Volume set to " << volume_ << std::endl;

        transportSource_.setGain(volume_);
    }

    float AudioPlayer::getVolume() const
    {
        return volume_;
    }

    void AudioPlayer::setPosition(double seconds)
    {
        if (!loaded_) {
            std::cerr << "AudioPlayer " << id_ << ": Cannot seek - no file loaded" << std::endl;
            return;
        }

        // Clamp position to valid range
        double duration = getDuration();
        seconds = juce::jlimit(0.0, duration, seconds);

        std::cout << "AudioPlayer " << id_ << ": Seeking to " << seconds << " seconds" << std::endl;

        transportSource_.setPosition(seconds);
    }

    double AudioPlayer::getPosition() const
    {
        return transportSource_.getCurrentPosition();
    }

    double AudioPlayer::getDuration() const
    {
        return transportSource_.getLengthInSeconds();
    }

} // namespace CueForgePhase1