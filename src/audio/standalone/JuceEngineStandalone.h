// ============================================================================
// JuceEngineStandalone.h - Pure JUCE audio engine (Phase 1)
// NO Qt dependencies, NO Cue dependencies
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

namespace CueForgePhase1 {  // Different namespace to avoid conflicts

    class AudioPlayer;

    class JuceEngineStandalone : public juce::AudioIODeviceCallback
    {
    public:
        JuceEngineStandalone();
        ~JuceEngineStandalone() override;

        // Device management
        bool initialize();
        void shutdown();
        bool isInitialized() const { return initialized_; }

        std::vector<std::string> getAvailableDevices();
        std::string getCurrentDevice() const;
        bool setDevice(const std::string& deviceName);

        // Audio callback (from JUCE)
        void audioDeviceIOCallbackWithContext(
            const float* const* inputChannelData,
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

        // System info
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

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuceEngineStandalone)
    };

    class AudioPlayer
    {
    public:
        AudioPlayer(JuceEngineStandalone* engine, int id);
        ~AudioPlayer();

        bool loadFile(const std::string& filePath);
        void unload();

        void play();
        void stop();
        void pause();
        void resume();

        bool isPlaying() const;
        bool isPaused() const;

        void setVolume(float volume);
        float getVolume() const;

        void setPosition(double seconds);
        double getPosition() const;
        double getDuration() const;

        std::string getFilePath() const { return filePath_; }
        int getId() const { return id_; }

        juce::AudioTransportSource* getTransportSource() { return &transportSource_; }

    private:
        JuceEngineStandalone* engine_;
        int id_;
        std::string filePath_;

        std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
        juce::AudioTransportSource transportSource_;

        float volume_;
        bool loaded_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
    };

} // namespace CueForgePhase1