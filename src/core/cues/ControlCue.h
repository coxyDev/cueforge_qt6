// ============================================================================
// ControlCue.h - Control operation cues (Start/Stop/Goto/etc)
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "../Cue.h"

namespace CueForge {

    class ControlCue : public Cue
    {
        Q_OBJECT
            Q_PROPERTY(double fadeTime READ fadeTime WRITE setFadeTime NOTIFY fadeTimeChanged)

    public:
        explicit ControlCue(CueType type = CueType::Start, QObject* parent = nullptr);
        ~ControlCue() override = default;

        double fadeTime() const { return fadeTime_; }
        void setFadeTime(double seconds);

        bool execute() override;
        void stop(double fadeTime = 0.0) override;
        bool canExecute() const override;
        bool validate() override;
        QString validationError() const override;

        QJsonObject toJson() const override;
        void fromJson(const QJsonObject& json) override;
        std::unique_ptr<Cue> clone() const override;

    signals:
        void fadeTimeChanged(double seconds);

    private:
        void executeStart();
        void executeStop();
        void executeGoto();
        void executePause();
        void executeLoad();
        void executeReset();
        void executeArm();
        void executeDisarm();
        void executeDevamp();

        double fadeTime_;
    };

} // namespace CueForge