// ============================================================================
// WaitCue.h - Timing wait/delay cue
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "../Cue.h"
#include <QTimer>
#include <QElapsedTimer>

namespace CueForge {

    class WaitCue : public Cue
    {
        Q_OBJECT

    public:
        explicit WaitCue(QObject* parent = nullptr);
        ~WaitCue() override = default;

        void setWaitDuration(double seconds);
        double remainingTime() const { return remainingTime_; }
        double elapsedTime() const;

        bool execute() override;
        void stop(double fadeTime = 0.0) override;
        void pause() override;
        void resume() override;
        bool canExecute() const override { return true; }

        QJsonObject toJson() const override;
        void fromJson(const QJsonObject& json) override;
        std::unique_ptr<Cue> clone() const override;

    private slots:
        void onTimerTick();
        void onWaitFinished();

    private:
        QTimer* timer_;
        QElapsedTimer elapsed_;
        double remainingTime_;
        qint64 pauseTime_;
    };

} // namespace CueForge