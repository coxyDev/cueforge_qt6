// ============================================================================
// ControlCue.cpp - Control operation cues implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "ControlCue.h"
#include "../CueManager.h"
#include <QDebug>

namespace CueForge {

    ControlCue::ControlCue(CueType type, QObject* parent)
        : Cue(type, parent)
        , fadeTime_(0.0)
    {
        switch (type) {
        case CueType::Start:
            setName("Start");
            setColor(QColor(100, 255, 100));
            break;
        case CueType::Stop:
            setName("Stop");
            setColor(QColor(255, 100, 100));
            break;
        case CueType::Goto:
            setName("Goto");
            setColor(QColor(100, 150, 255));
            break;
        case CueType::Pause:
            setName("Pause");
            setColor(QColor(255, 255, 100));
            break;
        case CueType::Load:
            setName("Load");
            setColor(QColor(200, 200, 100));
            break;
        case CueType::Reset:
            setName("Reset");
            setColor(QColor(255, 150, 100));
            break;
        case CueType::Arm:
            setName("Arm");
            setColor(QColor(100, 255, 200));
            break;
        case CueType::Disarm:
            setName("Disarm");
            setColor(QColor(200, 100, 255));
            break;
        case CueType::Devamp:
            setName("Devamp");
            setColor(QColor(200, 50, 50));
            break;
        default:
            setName("Control");
            setColor(QColor(150, 150, 150));
            break;
        }

        setDuration(0.0);
    }

    void ControlCue::setFadeTime(double seconds)
    {
        seconds = qMax(0.0, seconds);

        if (qAbs(fadeTime_ - seconds) > 0.001) {
            fadeTime_ = seconds;
            updateModifiedTime();
            emit fadeTimeChanged(seconds);
        }
    }

    bool ControlCue::execute()
    {
        if (!canExecute()) {
            return false;
        }

        setStatus(CueStatus::Running);

        switch (type()) {
        case CueType::Start:
            executeStart();
            break;
        case CueType::Stop:
            executeStop();
            break;
        case CueType::Goto:
            executeGoto();
            break;
        case CueType::Pause:
            executePause();
            break;
        case CueType::Load:
            executeLoad();
            break;
        case CueType::Reset:
            executeReset();
            break;
        case CueType::Arm:
            executeArm();
            break;
        case CueType::Disarm:
            executeDisarm();
            break;
        case CueType::Devamp:
            executeDevamp();
            break;
        default:
            emit warning("Unknown control cue type");
            setStatus(CueStatus::Loaded);
            return false;
        }

        setStatus(CueStatus::Finished);
        emit executionFinished();

        return true;
    }

    void ControlCue::stop(double fadeTime)
    {
        Q_UNUSED(fadeTime);
        setStatus(CueStatus::Loaded);
    }

    bool ControlCue::canExecute() const
    {
        if (!Cue::canExecute()) {
            return false;
        }

        if (type() != CueType::Pause && !hasValidTarget()) {
            return false;
        }

        return true;
    }

    bool ControlCue::validate()
    {
        if (type() != CueType::Pause && !hasValidTarget()) {
            setIsBroken(true);
            return false;
        }

        setIsBroken(false);
        return true;
    }

    QString ControlCue::validationError() const
    {
        if (!hasValidTarget() && type() != CueType::Pause) {
            if (targetCueId().isEmpty()) {
                return "No target cue assigned";
            }
            return "Target cue not found: " + targetCueId();
        }
        return QString();
    }

    QJsonObject ControlCue::toJson() const
    {
        QJsonObject json = Cue::toJson();
        json["fadeTime"] = fadeTime_;
        return json;
    }

    void ControlCue::fromJson(const QJsonObject& json)
    {
        Cue::fromJson(json);
        setFadeTime(json.value("fadeTime").toDouble(0.0));
    }

    std::unique_ptr<Cue> ControlCue::clone() const
    {
        auto cloned = std::make_unique<ControlCue>(type());

        cloned->setNumber(number());
        cloned->setName(name() + " (copy)");
        cloned->setDuration(duration());
        cloned->setPreWait(preWait());
        cloned->setPostWait(postWait());
        cloned->setContinueMode(continueMode());
        cloned->setColor(color());
        cloned->setNotes(notes());
        cloned->setTargetCueId(targetCueId());
        cloned->setFadeTime(fadeTime_);

        return cloned;
    }

    void ControlCue::executeStart()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot start - target cue not found");
            return;
        }

        if (target->canExecute()) {
            target->execute();
            qDebug() << "ControlCue START:" << name() << "→" << target->name();
        }
        else {
            emit warning("Cannot start target cue: " + target->name());
        }
    }

    void ControlCue::executeStop()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot stop - target cue not found");
            return;
        }

        target->stop(fadeTime_);
        qDebug() << "ControlCue STOP:" << name() << "→" << target->name()
            << "fade:" << fadeTime_;
    }

    void ControlCue::executeGoto()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot goto - target cue not found");
            return;
        }

        CueManager* manager = cueManager();
        if (manager) {
            manager->setStandByCue(target->id());
            qDebug() << "ControlCue GOTO:" << name() << "→" << target->name();
        }
        else {
            emit warning("Cannot access cue manager");
        }
    }

    void ControlCue::executePause()
    {
        if (hasValidTarget()) {
            Cue* target = targetCue();
            if (target->status() == CueStatus::Running) {
                target->pause();
            }
            else if (target->status() == CueStatus::Paused) {
                target->resume();
            }
            qDebug() << "ControlCue PAUSE:" << name() << "→" << target->name();
        }
        else {
            CueManager* manager = cueManager();
            if (manager) {
                manager->pause();
                qDebug() << "ControlCue PAUSE (global):" << name();
            }
        }
    }

    void ControlCue::executeLoad()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot load - target cue not found");
            return;
        }

        CueManager* manager = cueManager();
        if (manager) {
            manager->setStandByCue(target->id());
            qDebug() << "ControlCue LOAD:" << name() << "→" << target->name();
        }
    }

    void ControlCue::executeReset()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot reset - target cue not found");
            return;
        }

        target->stop(0.0);
        target->setStatus(CueStatus::Loaded);
        qDebug() << "ControlCue RESET:" << name() << "→" << target->name();
    }

    void ControlCue::executeArm()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot arm - target cue not found");
            return;
        }

        target->setArmed(true);
        qDebug() << "ControlCue ARM:" << name() << "→" << target->name();
    }

    void ControlCue::executeDisarm()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot disarm - target cue not found");
            return;
        }

        target->setArmed(false);
        qDebug() << "ControlCue DISARM:" << name() << "→" << target->name();
    }

    void ControlCue::executeDevamp()
    {
        Cue* target = targetCue();
        if (!target) {
            emit error("Cannot devamp - target cue not found");
            return;
        }

        double devampTime = (fadeTime_ > 0.0) ? fadeTime_ : 0.5;
        target->stop(devampTime);
        qDebug() << "ControlCue DEVAMP:" << name() << "→" << target->name()
            << "fade:" << devampTime;
    }

} // namespace CueForge