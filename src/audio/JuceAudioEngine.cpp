// ============================================================================
// JuceAudioEngine.cpp - Pure JUCE audio engine implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "JuceAudioEngine.h"
#include <iostream>

namespace CueForge {

    // ============================================================================
    // JuceAudioEngine Implementation
    // ============================================================================

    JuceAudioEngine::JuceAudioEngine()
        : nextPlayerId_(1)
        , initialized_(false)
    {
        // Register audio formats
        formatManager_.registerBasicFormats(); // WAV, AIFF

#if JUCE_USE_MP3AUDIOFORMAT
        formatManager_.registerFormat(new juce::MP3AudioFormat(), true);
#endif

#if JUCE_USE_WINDOWS_MEDIA_FORMAT
        formatManager_.registerFormat(new juce::WindowsMediaAudioFormat(), true);
#endif
    }

    JuceAudioEngine::~JuceAudioEngine()
    {
        shutdown();
    }

    bool JuceAudioEngine::initialize()
    {
        if (initialized_) {
            return true;
        }

        // Initialize with default device
        juce::String error = deviceManager_.initialise(
            0,      // numInputChannelsNeeded
            2,      // numOutputChannelsNeeded (stereo)
            nullptr, // preferredDefaultDevice
            true    // selectDefaultDeviceOnFailure
        );

        if (error.isNotEmpty()) {
            std::cerr << "Audio device init failed: " << error.toStdString() << std::endl;
            return false;
        }

        // Add this engine as the audio callback
        deviceManager_.addAudioCallback(this);

        initialized_ = true;

        std::cout << "Audio engine initialized" << std::endl;
        std::cout << "  Device: " << getCurrentDevice() << std::endl;
        std::cout << "  Sample Rate: " << getSampleRate() << " Hz" << std::endl;
        std::cout << "  Buffer Size: " << getBufferSize() << " samples" << std::endl;

        return true;
    }

    void JuceAudioEngine::shutdown()
    {
        if (!initialized_) {
            return;
        }

        deviceManager_.removeAudioCallback(this);
        deviceManager_.closeAudioDevice();

        players_.clear();

        initialized_ = false;

        std::cout << "Audio engine shut down" << std::endl;
    }

    std::vector<std::string> JuceAudioEngine::getAvailableDevices() const
    {
        std::vector<std::string> devices;

        auto* deviceType = deviceManager_.getCurrentDeviceTypeObject();
        if (!deviceType) {
            return devices;
        }

        auto deviceNames = deviceType->getDeviceNames(false); // false = output devices

        for (const auto& name : deviceNames) {
            devices.push_back(name.toStdString());
        }

        return devices;
    }

    std::string JuceAudioEngine::getCurrentDevice() const
    {
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (!device) {
            return "No device";
        }

        return device->getName().toStdString();
    }

    bool JuceAudioEngine::setDevice(const std::string& deviceName)
    {
        auto* deviceType = deviceManager_.getCurrentDeviceTypeObject();
        if (!deviceType) {
            return false;
        }

        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager_.getAudioDeviceSetup(setup);
        setup.outputDeviceName = deviceName;

        juce::String error = deviceManager_.setAudioDeviceSetup(setup, true);

        return error.isEmpty();
    }

    void JuceAudioEngine::audioDeviceIOCallbackWithContext(
        const float* const* /*inputChannelData*/,
        int /*numInputChannels*/,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& /*context*/)
    {
        // Create audio buffer for output
        juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);

        // Clear buffer
        buffer.clear();

        // Get audio from mixer
        juce::AudioSourceChannelInfo channelInfo;
        channelInfo.buffer = &buffer;
        channelInfo.startSample = 0;
        channelInfo.numSamples = numSamples;

        mixer_.getNextAudioBlock(channelInfo);
    }

    void JuceAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
    {
        mixer_.prepareToPlay(device->getCurrentBufferSizeSamples(),
            device->getCurrentSampleRate());
    }

    void JuceAudioEngine::audioDeviceStopped()
    {
        mixer_.releaseResources();
    }

    int JuceAudioEngine::createPlayer(const std::string& filePath)
    {
        juce::ScopedLock lock(playerLock_);

        int playerId = nextPlayerId_++;

        auto player = std::make_unique<AudioPlayer>(this, playerId);

        if (!player->loadFile(filePath)) {
            return -1;
        }

        // Add player's audio source to mixer
        mixer_.addInputSource(player->getTransportSource(), false);

        players_[playerId] = std::move(player);

        return playerId;
    }

    void JuceAudioEngine::removePlayer(int playerId)
    {
        juce::ScopedLock lock(playerLock_);

        auto it = players_.find(playerId);
        if (it != players_.end()) {
            // Remove from mixer
            mixer_.removeInputSource(it->second->getTransportSource());

            // Delete player
            players_.erase(it);
        }
    }

    AudioPlayer* JuceAudioEngine::getPlayer(int playerId)
    {
        juce::ScopedLock lock(playerLock_);

        auto it = players_.find(playerId);
        if (it != players_.end()) {
            return it->second.get();
        }

        return nullptr;
    }

    double JuceAudioEngine::getSampleRate() const
    {
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (device) {
            return device->getCurrentSampleRate();
        }
        return 44100.0;
    }

    int JuceAudioEngine::getBufferSize() const
    {
        auto* device = deviceManager_.getCurrentAudioDevice();
        if (device) {
            return device->getCurrentBufferSizeSamples();
        }
        return 512;
    }

    // ============================================================================
    // AudioPlayer Implementation
    // ============================================================================

    AudioPlayer::AudioPlayer(JuceAudioEngine* engine, int id)
        : engine_(engine)
        , id_(id)
        , volume_(1.0f)
        , loaded_(false)
    {
    }

    AudioPlayer::~AudioPlayer()
    {
        unload();
    }

    bool AudioPlayer::loadFile(const std::string& filePath)
    {
        unload();

        juce::File file(filePath);
        if (!file.existsAsFile()) {
            std::cerr << "File not found: " << filePath << std::endl;
            return false;
        }

        auto* reader = engine_->formatManager_.createReaderFor(file);
        if (!reader) {
            std::cerr << "Could not create reader for: " << filePath << std::endl;
            return false;
        }

        readerSource_ = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

        transportSource_.setSource(readerSource_.get(), 0, nullptr,
            reader->sampleRate);

        filePath_ = filePath;
        loaded_ = true;

        std::cout << "Loaded audio file: " << filePath << std::endl;
        std::cout << "  Duration: " << getDuration() << " seconds" << std::endl;
        std::cout << "  Sample Rate: " << reader->sampleRate << " Hz" << std::endl;

        return true;
    }

    void AudioPlayer::unload()
    {
        if (!loaded_) {
            return;
        }

        stop();
        transportSource_.setSource(nullptr);
        readerSource_.reset();

        loaded_ = false;
        filePath_.clear();
    }

    void AudioPlayer::play()
    {
        if (!loaded_) {
            return;
        }

        transportSource_.start();
        std::cout << "Playing: " << filePath_ << std::endl;
    }

    void AudioPlayer::stop()
    {
        transportSource_.stop();
        transportSource_.setPosition(0.0);
    }

    void AudioPlayer::pause()
    {
        transportSource_.stop();
    }

    void AudioPlayer::resume()
    {
        if (!loaded_) {
            return;
        }

        transportSource_.start();
    }

    bool AudioPlayer::isPlaying() const
    {
        return transportSource_.isPlaying();
    }

    bool AudioPlayer::isPaused() const
    {
        return loaded_ && !transportSource_.isPlaying() && getPosition() > 0.0;
    }

    void AudioPlayer::setVolume(float volume)
    {
        volume_ = juce::jlimit(0.0f, 1.0f, volume);
        transportSource_.setGain(volume_);
    }

    float AudioPlayer::getVolume() const
    {
        return volume_;
    }

    void AudioPlayer::setPosition(double seconds)
    {
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

} // namespace CueForge