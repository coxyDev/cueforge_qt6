// ============================================================================
// AudioEngineQt.h - Qt bridge to JUCE audio engine
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace CueForge {

    class JuceAudioEngine;

    /**
     * Qt-friendly wrapper around JUCE audio engine
     * Provides signals/slots interface for Qt application
     */
    class AudioEngineQt : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioEngineQt(QObject* parent = nullptr);
        ~AudioEngineQt() override;

        // Initialization
        bool initialize();
        void shutdown();
        bool isInitialized() const;

        // Device management
        QStringList getAvailableDevices() const;
        QString getCurrentDevice() const;
        bool setDevice(const QString& deviceName);

        // Player management
        int createPlayer(const QString& filePath);
        void removePlayer(int playerId);

        // Playback control
        bool play(int playerId);
        void stop(int playerId);
        void pause(int playerId);
        void resume(int playerId);

        bool isPlaying(int playerId) const;
        bool isPaused(int playerId) const;

        // Audio properties
        void setVolume(int playerId, double volume);
        double getVolume(int playerId) const;

        void setPosition(int playerId, double seconds);
        double getPosition(int playerId) const;
        double getDuration(int playerId) const;

    signals:
        void deviceChanged(const QString& deviceName);
        void playerCreated(int playerId);
        void playerRemoved(int playerId);
        void playbackStarted(int playerId);
        void playbackStopped(int playerId);
        void playbackPaused(int playerId);
        void playbackResumed(int playerId);
        void positionChanged(int playerId, double seconds);
        void error(const QString& message);

    private:
        std::unique_ptr<JuceAudioEngine> juceEngine_;
    };

} // namespace CueForge