// ============================================================================
// AudioCue.cpp - Audio playback cue implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "AudioCue.h"
#include "../../audio/AudioEngineQt.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDebug>

namespace CueForge {

    AudioCue::AudioCue(QObject* parent)
        : Cue(CueType::Audio, parent)
        , audioEngine_(nullptr)
        , playerId_(-1)
        , volume_(0.8)
        , pan_(0.0)
        , rate_(1.0)
        , startTime_(0.0)
        , endTime_(0.0)
    {
        setColor(QColor(100, 255, 150)); // QLab-style green
    }

    AudioCue::~AudioCue()
    {
        // Clean up audio player if still active
        if (audioEngine_ && playerId_ >= 0) {
            audioEngine_->removePlayer(playerId_);
            playerId_ = -1;
        }
    }

    void AudioCue::setAudioEngine(AudioEngineQt* engine)
    {
        audioEngine_ = engine;
    }

    void AudioCue::setFilePath(const QString& filePath)
    {
        if (filePath_ != filePath) {
            filePath_ = filePath;
            loadFileInfo();
            updateModifiedTime();

            // Update cue name if empty
            if (name().isEmpty()) {
                QFileInfo fileInfo(filePath);
                setName(fileInfo.baseName());
            }
        }
    }

    void AudioCue::loadFileInfo()
    {
        fileInfo_ = AudioFileInfo();

        if (filePath_.isEmpty()) {
            return;
        }

        QFileInfo fileInfo(filePath_);
        if (!fileInfo.exists() || !fileInfo.isReadable()) {
            qWarning() << "AudioCue: File not accessible:" << filePath_;
            return;
        }

        // File exists - mark as valid
        // Duration will be loaded when player is created
        fileInfo_.isValid = true;
    }

    void AudioCue::setVolume(double volume)
    {
        volume = qBound(0.0, volume, 1.0);
        if (!qFuzzyCompare(volume_, volume)) {
            volume_ = volume;
            updateModifiedTime();

            // Update live player if active
            if (audioEngine_ && playerId_ >= 0) {
                audioEngine_->setVolume(playerId_, volume_);
            }
        }
    }

    void AudioCue::setPan(double pan)
    {
        pan = qBound(-1.0, pan, 1.0);
        if (!qFuzzyCompare(pan_, pan)) {
            pan_ = pan;
            updateModifiedTime();
        }
    }

    void AudioCue::setRate(double rate)
    {
        rate = qBound(0.5, rate, 2.0);
        if (!qFuzzyCompare(rate_, rate)) {
            rate_ = rate;
            validateTrimPoints();
            updateModifiedTime();
        }
    }

    void AudioCue::setStartTime(double seconds)
    {
        seconds = qMax(0.0, seconds);
        if (!qFuzzyCompare(startTime_, seconds)) {
            startTime_ = seconds;
            validateTrimPoints();
            updateModifiedTime();
        }
    }

    void AudioCue::setEndTime(double seconds)
    {
        seconds = qMax(0.0, seconds);
        if (!qFuzzyCompare(endTime_, seconds)) {
            endTime_ = seconds;
            validateTrimPoints();
            updateModifiedTime();
        }
    }


    double AudioCue::effectiveDuration() const
    {
        if (endTime_ > startTime_) {
            return (endTime_ - startTime_) / rate_;
        }
        return duration() / rate_;
    }

    void AudioCue::validateTrimPoints()
    {
        if (startTime_ >= endTime_ && endTime_ > 0) {
            startTime_ = qMax(0.0, endTime_ - 0.1);
        }

        if (fileInfo_.isValid) {
            startTime_ = qMin(startTime_, fileInfo_.duration);
            if (endTime_ > fileInfo_.duration) {
                endTime_ = fileInfo_.duration;
            }
        }
    }

    void AudioCue::setMatrixRouting(const QVariantMap& routing)
    {
        matrixRouting_ = routing;
        updateModifiedTime();
    }

    void AudioCue::setRoutingLevel(int inputChannel, int outputChannel, double levelDb)
    {
        QString key = makeRoutingKey(inputChannel, outputChannel);

        if (levelDb <= -96.0) {
            matrixRouting_.remove(key);
        }
        else {
            matrixRouting_[key] = levelDb;
        }

        updateModifiedTime();
    }

    double AudioCue::getRoutingLevel(int inputChannel, int outputChannel) const
    {
        QString key = makeRoutingKey(inputChannel, outputChannel);
        return matrixRouting_.value(key, -96.0).toDouble();
    }

    bool AudioCue::isRouted(int inputChannel, int outputChannel) const
    {
        QString key = makeRoutingKey(inputChannel, outputChannel);
        return matrixRouting_.contains(key);
    }

    QString AudioCue::makeRoutingKey(int input, int output) const
    {
        return QString("%1_%2").arg(input).arg(output);
    }

    void AudioCue::setAudioOutputPatch(const QString& patchName)
    {
        if (audioOutputPatch_ != patchName) {
            audioOutputPatch_ = patchName;
            updateModifiedTime();
        }
    }

    // ============================================================================
    // Playback Control - THE CRITICAL CONNECTION
    // ============================================================================

    bool AudioCue::execute()
    {
        // Validation
        if (!canExecute()) {
            qWarning() << "AudioCue::execute() - Cannot execute cue:" << number();
            return false;
        }

        if (filePath_.isEmpty()) {
            qWarning() << "AudioCue::execute() - No file path set";
            return false;
        }

        if (!audioEngine_) {
            qWarning() << "AudioCue::execute() - No audio engine connected!";
            return false;
        }

        if (!audioEngine_->isInitialized()) {
            qWarning() << "AudioCue::execute() - Audio engine not initialized";
            return false;
        }

        // Clean up any existing player
        if (playerId_ >= 0) {
            audioEngine_->removePlayer(playerId_);
            playerId_ = -1;
        }

        // Create new player and load file
        qDebug() << "AudioCue::execute() - Creating player for:" << filePath_;
        playerId_ = audioEngine_->createPlayer(filePath_);

        if (playerId_ < 0) {
            qWarning() << "AudioCue::execute() - Failed to create audio player";
            return false;
        }

        // Get duration from engine
        double loadedDuration = audioEngine_->getDuration(playerId_);
        if (loadedDuration > 0.0) {
            fileInfo_.duration = loadedDuration;
            fileInfo_.isValid = true;
            setDuration(loadedDuration);
        }

        // Apply playback settings
        applyPlaybackSettings();

        // Start playback
        qDebug() << "AudioCue::execute() - Starting playback";
        if (!audioEngine_->play(playerId_)) {
            qWarning() << "AudioCue::execute() - Failed to start playback";
            audioEngine_->removePlayer(playerId_);
            playerId_ = -1;
            return false;
        }

        // Update status
        setStatus(CueStatus::Running);
        qDebug() << "AudioCue::execute() - Successfully started cue" << number();

        return true;
    }

    void AudioCue::stop(double fadeTime)
    {
        Q_UNUSED(fadeTime);
        if (status() == CueStatus::Loaded) {
            return;
        }

        qDebug() << "AudioCue::stop() - Stopping cue" << number();

        // Stop audio player
        if (audioEngine_ && playerId_ >= 0) {
            audioEngine_->stop(playerId_);
            audioEngine_->removePlayer(playerId_);
            playerId_ = -1;
        }

        // Update status
        setStatus(CueStatus::Stopped);
    }

    void AudioCue::pause()
    {
        if (status() != CueStatus::Running) {
            return;
        }

        qDebug() << "AudioCue::pause() - Pausing cue" << number();

        // Pause audio player
        if (audioEngine_ && playerId_ >= 0) {
            audioEngine_->pause(playerId_);
        }

        // Update status
        setStatus(CueStatus::Paused);
    }

    void AudioCue::resume()
    {
        if (status() != CueStatus::Paused) {
            return;
        }

        qDebug() << "AudioCue::resume() - Resuming cue" << number();

        // Resume audio player
        if (audioEngine_ && playerId_ >= 0) {
            audioEngine_->resume(playerId_);
        }

        // Update status
        setStatus(CueStatus::Running);
    }

    void AudioCue::applyPlaybackSettings()
    {
        if (!audioEngine_ || playerId_ < 0) {
            return;
        }

        // Apply volume
        audioEngine_->setVolume(playerId_, volume_);

        // Apply start position if trimmed
        if (startTime_ > 0.0) {
            audioEngine_->setPosition(playerId_, startTime_);
        }

        // Note: Pan and rate will require additional engine support
        // For now, volume and position are applied
    }

    // ============================================================================
    // Serialization
    // ============================================================================

    void AudioCue::toJson(QJsonObject& json) const
    {
        Cue::toJson(json);

        json["filePath"] = filePath_;
        json["volume"] = volume_;
        json["pan"] = pan_;
        json["rate"] = rate_;
        json["startTime"] = startTime_;
        json["endTime"] = endTime_;
        json["audioOutputPatch"] = audioOutputPatch_;

        // Matrix routing
        if (!matrixRouting_.isEmpty()) {
            QJsonObject routingObj;
            for (auto it = matrixRouting_.constBegin(); it != matrixRouting_.constEnd(); ++it) {
                routingObj[it.key()] = it.value().toDouble();
            }
            json["matrixRouting"] = routingObj;
        }
    }

    void AudioCue::fromJson(const QJsonObject& json)
    {
        Cue::fromJson(json);

        setFilePath(json["filePath"].toString());
        setVolume(json["volume"].toDouble(0.8));
        setPan(json["pan"].toDouble(0.0));
        setRate(json["rate"].toDouble(1.0));
        setStartTime(json["startTime"].toDouble(0.0));
        setEndTime(json["endTime"].toDouble(0.0));
        setAudioOutputPatch(json["audioOutputPatch"].toString());

        // Matrix routing
        if (json.contains("matrixRouting")) {
            QJsonObject routingObj = json["matrixRouting"].toObject();
            QVariantMap routing;
            for (auto it = routingObj.constBegin(); it != routingObj.constEnd(); ++it) {
                routing[it.key()] = it.value().toDouble();
            }
            setMatrixRouting(routing);
        }
    }

} // namespace CueForge