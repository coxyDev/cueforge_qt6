// ============================================================================
// AudioEngineQt.cpp - Qt bridge implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "AudioEngineQt.h"
#include "JuceAudioEngine.h"
#include <QDebug>

namespace CueForge {

    AudioEngineQt::AudioEngineQt(QObject* parent)
        : QObject(parent)
        , juceEngine_(std::make_unique<JuceAudioEngine>())
    {
    }

    AudioEngineQt::~AudioEngineQt()
    {
        shutdown();
    }

    bool AudioEngineQt::initialize()
    {
        if (!juceEngine_) {
            emit error("Audio engine not created");
            return false;
        }

        if (juceEngine_->initialize()) {
            qDebug() << "AudioEngineQt: Initialized successfully";
            return true;
        }

        emit error("Failed to initialize audio engine");
        return false;
    }

    void AudioEngineQt::shutdown()
    {
        if (juceEngine_) {
            juceEngine_->shutdown();
        }
    }

    bool AudioEngineQt::isInitialized() const
    {
        return juceEngine_ && juceEngine_->isInitialized();
    }

    QStringList AudioEngineQt::getAvailableDevices() const
    {
        if (!juceEngine_) {
            return QStringList();
        }

        QStringList devices;
        auto deviceList = juceEngine_->getAvailableDevices();

        for (const auto& device : deviceList) {
            devices.append(QString::fromStdString(device));
        }

        return devices;
    }

    QString AudioEngineQt::getCurrentDevice() const
    {
        if (!juceEngine_) {
            return QString();
        }

        return QString::fromStdString(juceEngine_->getCurrentDevice());
    }

    bool AudioEngineQt::setDevice(const QString& deviceName)
    {
        if (!juceEngine_) {
            return false;
        }

        bool success = juceEngine_->setDevice(deviceName.toStdString());

        if (success) {
            emit deviceChanged(deviceName);
        }

        return success;
    }

    int AudioEngineQt::createPlayer(const QString& filePath)
    {
        if (!juceEngine_) {
            emit error("Audio engine not initialized");
            return -1;
        }

        int playerId = juceEngine_->createPlayer(filePath.toStdString());

        if (playerId > 0) {
            emit playerCreated(playerId);
            qDebug() << "Created player" << playerId << "for" << filePath;
        }
        else {
            emit error(QString("Failed to create player for: %1").arg(filePath));
        }

        return playerId;
    }

    void AudioEngineQt::removePlayer(int playerId)
    {
        if (!juceEngine_) {
            return;
        }

        juceEngine_->removePlayer(playerId);
        emit playerRemoved(playerId);
    }

    bool AudioEngineQt::play(int playerId)
    {
        if (!juceEngine_) {
            return false;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        if (!player) {
            emit error(QString("Player %1 not found").arg(playerId));
            return false;
        }

        player->play();
        emit playbackStarted(playerId);
        return true;
    }

    void AudioEngineQt::stop(int playerId)
    {
        if (!juceEngine_) {
            return;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        if (player) {
            player->stop();
            emit playbackStopped(playerId);
        }
    }

    void AudioEngineQt::pause(int playerId)
    {
        if (!juceEngine_) {
            return;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        if (player) {
            player->pause();
            emit playbackPaused(playerId);
        }
    }

    void AudioEngineQt::resume(int playerId)
    {
        if (!juceEngine_) {
            return;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        if (player) {
            player->resume();
            emit playbackResumed(playerId);
        }
    }

    bool AudioEngineQt::isPlaying(int playerId) const
    {
        if (!juceEngine_) {
            return false;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        return player && player->isPlaying();
    }

    bool AudioEngineQt::isPaused(int playerId) const
    {
        if (!juceEngine_) {
            return false;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        return player && player->isPaused();
    }

    void AudioEngineQt::setVolume(int playerId, double volume)
    {
        if (!juceEngine_) {
            return;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        if (player) {
            player->setVolume(static_cast<float>(volume));
        }
    }

    double AudioEngineQt::getVolume(int playerId) const
    {
        if (!juceEngine_) {
            return 0.0;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        return player ? player->getVolume() : 0.0;
    }

    void AudioEngineQt::setPosition(int playerId, double seconds)
    {
        if (!juceEngine_) {
            return;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        if (player) {
            player->setPosition(seconds);
            emit positionChanged(playerId, seconds);
        }
    }

    double AudioEngineQt::getPosition(int playerId) const
    {
        if (!juceEngine_) {
            return 0.0;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        return player ? player->getPosition() : 0.0;
    }

    double AudioEngineQt::getDuration(int playerId) const
    {
        if (!juceEngine_) {
            return 0.0;
        }

        auto* player = juceEngine_->getPlayer(playerId);
        return player ? player->getDuration() : 0.0;
    }

} // namespace CueForge