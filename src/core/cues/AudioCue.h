// ============================================================================
// AudioCue.h - Audio playback cue (CORRECTED VERSION)
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "../Cue.h"
#include <QString>
#include <QUrl>
#include <QVariantMap>

namespace CueForge {

    // Forward declaration for audio engine
    class AudioEngineQt;

    class AudioCue : public Cue
    {
        Q_OBJECT
            Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
            Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
            Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
            Q_PROPERTY(double rate READ rate WRITE setRate NOTIFY rateChanged)

    public:
        explicit AudioCue(QObject* parent = nullptr);
        ~AudioCue() override;

        struct AudioFileInfo {
            int channels = 0;
            int sampleRate = 0;
            double duration = 0.0;
            QString format;
            int bitDepth = 0;
            qint64 fileSize = 0;
            bool isValid = false;
        };

        // *** AUDIO ENGINE CONNECTION - NEW ***
        void setAudioEngine(AudioEngineQt* engine);
        AudioEngineQt* audioEngine() const { return audioEngine_; }
        int playerId() const { return playerId_; }

        // File management
        QString filePath() const { return filePath_; }
        void setFilePath(const QString& path);
        AudioFileInfo fileInfo() const { return fileInfo_; }
        bool hasValidFile() const { return fileInfo_.isValid; }

        // Playback properties
        double volume() const { return volume_; }
        void setVolume(double volume);
        double volumeDb() const;
        void setVolumeDb(double db);

        double pan() const { return pan_; }
        void setPan(double pan);

        double rate() const { return rate_; }
        void setRate(double rate);

        // Trim controls
        double startTime() const { return startTime_; }
        void setStartTime(double seconds);
        double endTime() const { return endTime_; }
        void setEndTime(double seconds);

        bool loopEnabled() const { return loopEnabled_; }
        void setLoopEnabled(bool enabled);

        double effectiveDuration() const;

        // Matrix routing
        QVariantMap matrixRouting() const { return matrixRouting_; }
        void setMatrixRouting(const QVariantMap& routing);
        void setRoutingLevel(int inputChannel, int outputChannel, double levelDb);
        double getRoutingLevel(int inputChannel, int outputChannel) const;
        bool isRouted(int inputChannel, int outputChannel) const;

        QString audioOutputPatch() const { return audioOutputPatch_; }
        void setAudioOutputPatch(const QString& patchName);

        // Cue interface overrides - MATCH BASE CLASS SIGNATURES
        bool execute() override;
        void stop(double fadeTime = 0.0) override;  // Note: fadeTime parameter
        void pause() override;
        void resume() override;
        bool canExecute() const override;
        bool validate() override;
        QString validationError() const override;

        QJsonObject toJson() const override;
        void fromJson(const QJsonObject& json) override;
        std::unique_ptr<Cue> clone() const override;

    signals:
        void filePathChanged(const QString& path);
        void volumeChanged(double volume);
        void panChanged(double pan);
        void rateChanged(double rate);
        void playbackPositionChanged(double seconds);

    private:
        void loadFileInfo();
        void validateTrimPoints();
        QString makeRoutingKey(int input, int output) const;
        void applyPlaybackSettings();

        // *** AUDIO ENGINE CONNECTION - NEW ***
        AudioEngineQt* audioEngine_;  // Not owned - just a reference
        int playerId_;                // ID of player in audio engine (-1 if none)

        // Existing members
        QString filePath_;
        AudioFileInfo fileInfo_;
        double volume_;
        double pan_;
        double rate_;
        double startTime_;
        double endTime_;
        bool loopEnabled_;
        QVariantMap matrixRouting_;
        QString audioOutputPatch_;
        double currentPosition_;
        bool isPlaying_;
    };

} // namespace CueForge