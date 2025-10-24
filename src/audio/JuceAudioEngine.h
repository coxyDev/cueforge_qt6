// ============================================================================
// JuceAudioEngine.h - Pure JUCE audio engine (no Qt dependencies)
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>
#include <string>
#include <map>

namespace CueForge {

    // Forward declarations
    class AudioPlayer;

    /**
     * Pure JUCE audio engine - manages audio devices and playback
     * No Qt dependencies - pure C++ and JUCE
     */
    class JuceAudioEngine : public juce::AudioIODeviceCallback
    {
    public:
        JuceAudioEngine();
        ~JuceAudioEngine() override;

        // Device management
        bool initialize();
        void shutdown();
        bool isInitialized() const { return initialized_; }

        std::vector<std::string> getAvailableDevices() const;
        std::string getCurrentDevice() const;
        bool setDevice(const std::string& deviceName);

        // Audio callback (from JUCE)
        void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
            int numInputChannels,
            float* const* outputChannelData,
            int numOutputChannels,
            int numSamples,
            const juce::AudioIODeviceCallbackContext& context) override;

        void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
        void audioDeviceStopped() override;

        // Player management
        int createPlayer(const std::string& filePath);
        void removePlayer(int playerId);
        AudioPlayer* getPlayer(int playerId);

        // Mixer access
        double getSampleRate() const;
        int getBufferSize() const;

    private:
        friend class AudioPlayer;

        juce::AudioDeviceManager deviceManager_;
        juce::AudioFormatManager formatManager_;
        juce::MixerAudioSource mixer_;

        std::map<int, std::unique_ptr<AudioPlayer>> players_;
        int nextPlayerId_;

        bool initialized_;

        juce::CriticalSection playerLock_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuceAudioEngine)
    };

    /**
     * Individual audio player for one cue
     */
    class AudioPlayer
    {
    public:
        AudioPlayer(JuceAudioEngine* engine, int id);
        ~AudioPlayer();

        bool loadFile(const std::string& filePath);
        void unload();

        void play();
        void stop();
        void pause();
        void resume();

        bool isPlaying() const;
        bool isPaused() const;

        void setVolume(float volume); // 0.0 to 1.0
        float getVolume() const;

        void setPosition(double seconds);
        double getPosition() const;
        double getDuration() const;

        std::string getFilePath() const { return filePath_; }
        int getId() const { return id_; }

        // Internal - get audio source for mixing
        juce::AudioTransportSource* getTransportSource() { return &transportSource_; }

    private:
        JuceAudioEngine* engine_;
        int id_;
        std::string filePath_;

        std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
        juce::AudioTransportSource transportSource_;

        float volume_;
        bool loaded_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
    };

} // namespace CueForge