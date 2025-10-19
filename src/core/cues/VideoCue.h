// ============================================================================
// VideoCue.h - Video playback cue
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "../Cue.h"
#include <QString>
#include <QVariantMap>

namespace CueForge {

    class VideoCue : public Cue
    {
        Q_OBJECT
            Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
            Q_PROPERTY(double opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
            Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)

    public:
        explicit VideoCue(QObject* parent = nullptr);
        ~VideoCue() override = default;

        struct VideoFileInfo {
            int width = 0;
            int height = 0;
            double frameRate = 0.0;
            double duration = 0.0;
            QString codec;
            bool hasAudio = false;
            bool isValid = false;
        };

        QString filePath() const { return filePath_; }
        void setFilePath(const QString& path);

        VideoFileInfo fileInfo() const { return fileInfo_; }
        bool hasValidFile() const { return fileInfo_.isValid; }

        double opacity() const { return opacity_; }
        void setOpacity(double opacity);

        double volume() const { return volume_; }
        void setVolume(double volume);

        double startTime() const { return startTime_; }
        void setStartTime(double seconds);

        double endTime() const { return endTime_; }
        void setEndTime(double seconds);

        bool loopEnabled() const { return loopEnabled_; }
        void setLoopEnabled(bool enabled);

        QString videoStage() const { return videoStage_; }
        void setVideoStage(const QString& stage);

        QVariantMap geometry() const { return geometry_; }
        void setGeometry(const QVariantMap& geometry);

        bool execute() override;
        void stop(double fadeTime = 0.0) override;
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
        void opacityChanged(double opacity);
        void volumeChanged(double volume);

    private:
        void loadFileInfo();

        QString filePath_;
        VideoFileInfo fileInfo_;
        double opacity_;
        double volume_;
        double startTime_;
        double endTime_;
        bool loopEnabled_;
        QString videoStage_;
        QVariantMap geometry_;
        bool isPlaying_;
    };

} // namespace CueForge