// ============================================================================
// AudioCue.cpp - Audio playback cue implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "AudioCue.h"
#include <QFileInfo>
#include <QDebug>
#include <QtMath>
#include <QTimer>

namespace CueForge {

    AudioCue::AudioCue(QObject* parent)
        : Cue(CueType::Audio, parent)
        , volume_(1.0)
        , pan_(0.0)
        , rate_(1.0)
        , startTime_(0.0)
        , endTime_(0.0)
        , loopEnabled_(false)
        , currentPosition_(0.0)
        , isPlaying_(false)
    {
        setName("Audio Cue");
        setColor(QColor(100, 150, 255));
    }

    void AudioCue::setFilePath(const QString& path)
    {
        if (filePath_ == path) {
            return;
        }

        filePath_ = path;
        loadFileInfo();

        if (fileInfo_.isValid) {
            setDuration(fileInfo_.duration);
            endTime_ = fileInfo_.duration;
            setIsBroken(false);
        }
        else {
            setIsBroken(true);
        }

        updateModifiedTime();
        emit filePathChanged(path);
    }

    void AudioCue::loadFileInfo()
    {
        fileInfo_ = AudioFileInfo();

        if (filePath_.isEmpty()) {
            return;
        }

        QFileInfo fileInfo(filePath_);
        if (!fileInfo.exists()) {
            qWarning() << "Audio file not found:" << filePath_;
            return;
        }

        fileInfo_.fileSize = fileInfo.size();
        fileInfo_.format = fileInfo.suffix().toUpper();

        QStringList audioExts = { "wav", "mp3", "aiff", "aif", "flac", "ogg", "m4a", "aac" };
        if (audioExts.contains(fileInfo.suffix().toLower())) {
            fileInfo_.isValid = true;
            fileInfo_.channels = 2;
            fileInfo_.sampleRate = 48000;
            fileInfo_.duration = 10.0;
            fileInfo_.bitDepth = 24;
        }

        qDebug() << "Loaded audio file info:" << filePath_
            << "channels:" << fileInfo_.channels
            << "duration:" << fileInfo_.duration;
    }

    void AudioCue::setVolume(double volume)
    {
        volume = qBound(0.0, volume, 2.0);

        if (qAbs(volume_ - volume) > 0.001) {
            volume_ = volume;
            updateModifiedTime();
            emit volumeChanged(volume_);
        }
    }

    double AudioCue::volumeDb() const
    {
        if (volume_ <= 0.0) {
            return -96.0;
        }
        return 20.0 * qLn(volume_) / qLn(10.0);
    }

    void AudioCue::setVolumeDb(double db)
    {
        db = qBound(-96.0, db, 12.0);
        if (db <= -96.0) {
            setVolume(0.0);
        }
        else {
            setVolume(qPow(10.0, db / 20.0));
        }
    }

    void AudioCue::setPan(double pan)
    {
        pan = qBound(-1.0, pan, 1.0);

        if (qAbs(pan_ - pan) > 0.001) {
            pan_ = pan;
            updateModifiedTime();
            emit panChanged(pan_);
        }
    }

    void AudioCue::setRate(double rate)
    {
        rate = qBound(0.1, rate, 4.0);

        if (qAbs(rate_ - rate) > 0.001) {
            rate_ = rate;
            updateModifiedTime();
            emit rateChanged(rate_);
        }
    }

    void AudioCue::setStartTime(double seconds)
    {
        seconds = qMax(0.0, seconds);

        if (qAbs(startTime_ - seconds) > 0.001) {
            startTime_ = seconds;
            validateTrimPoints();
            updateModifiedTime();
        }
    }

    void AudioCue::setEndTime(double seconds)
    {
        if (qAbs(endTime_ - seconds) > 0.001) {
            endTime_ = seconds;
            validateTrimPoints();
            updateModifiedTime();
        }
    }

    void AudioCue::setLoopEnabled(bool enabled)
    {
        if (loopEnabled_ != enabled) {
            loopEnabled_ = enabled;
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

    bool AudioCue::execute()
    {
        if (!canExecute()) {
            return false;
        }

        isPlaying_ = true;
        currentPosition_ = startTime_;
        setStatus(CueStatus::Running);

        qDebug() << "AudioCue execute:" << name() << filePath_;

        QTimer::singleShot(static_cast<int>(effectiveDuration() * 1000), this, [this]() {
            if (isPlaying_) {
                isPlaying_ = false;
                setStatus(CueStatus::Finished);
                emit executionFinished();
            }
            });

        return true;
    }

    void AudioCue::stop(double fadeTime)
    {
        Q_UNUSED(fadeTime);

        if (isPlaying_) {
            isPlaying_ = false;
            currentPosition_ = 0.0;
            setStatus(CueStatus::Loaded);
            qDebug() << "AudioCue stopped:" << name();
        }
    }

    void AudioCue::pause()
    {
        if (isPlaying_) {
            isPlaying_ = false;
            setStatus(CueStatus::Paused);
            qDebug() << "AudioCue paused:" << name();
        }
    }

    void AudioCue::resume()
    {
        if (status() == CueStatus::Paused) {
            isPlaying_ = true;
            setStatus(CueStatus::Running);
            qDebug() << "AudioCue resumed:" << name();
        }
    }

    bool AudioCue::canExecute() const
    {
        return Cue::canExecute() && hasValidFile();
    }

    bool AudioCue::validate()
    {
        if (!hasValidFile()) {
            return false;
        }

        validateTrimPoints();
        return true;
    }

    QString AudioCue::validationError() const
    {
        if (!hasValidFile()) {
            if (filePath_.isEmpty()) {
                return "No audio file assigned";
            }
            return "Audio file not found: " + filePath_;
        }
        return QString();
    }

    QJsonObject AudioCue::toJson() const
    {
        QJsonObject json = Cue::toJson();

        json["filePath"] = filePath_;
        json["volume"] = volume_;
        json["volumeDb"] = volumeDb();
        json["pan"] = pan_;
        json["rate"] = rate_;
        json["startTime"] = startTime_;
        json["endTime"] = endTime_;
        json["loopEnabled"] = loopEnabled_;
        json["matrixRouting"] = QJsonObject::fromVariantMap(matrixRouting_);
        json["audioOutputPatch"] = audioOutputPatch_;

        return json;
    }

    void AudioCue::fromJson(const QJsonObject& json)
    {
        Cue::fromJson(json);

        setFilePath(json.value("filePath").toString());
        setVolume(json.value("volume").toDouble(1.0));
        setPan(json.value("pan").toDouble(0.0));
        setRate(json.value("rate").toDouble(1.0));
        setStartTime(json.value("startTime").toDouble(0.0));
        setEndTime(json.value("endTime").toDouble(0.0));
        setLoopEnabled(json.value("loopEnabled").toBool(false));

        if (json.contains("matrixRouting")) {
            setMatrixRouting(json["matrixRouting"].toObject().toVariantMap());
        }
        setAudioOutputPatch(json.value("audioOutputPatch").toString("main"));
    }

    std::unique_ptr<Cue> AudioCue::clone() const
    {
        auto cloned = std::make_unique<AudioCue>();

        cloned->setNumber(number());
        cloned->setName(name() + " (copy)");
        cloned->setDuration(duration());
        cloned->setPreWait(preWait());
        cloned->setPostWait(postWait());
        cloned->setContinueMode(continueMode());
        cloned->setColor(color());
        cloned->setNotes(notes());
        cloned->setFilePath(filePath_);
        cloned->setVolume(volume_);
        cloned->setPan(pan_);
        cloned->setRate(rate_);
        cloned->setStartTime(startTime_);
        cloned->setEndTime(endTime_);
        cloned->setLoopEnabled(loopEnabled_);
        cloned->setMatrixRouting(matrixRouting_);
        cloned->setAudioOutputPatch(audioOutputPatch_);

        return cloned;
    }

} // namespace CueForge