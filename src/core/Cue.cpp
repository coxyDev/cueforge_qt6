// ============================================================================
// Cue.cpp - Base cue class implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "Cue.h"
#include "CueManager.h"
#include <QUuid>
#include <QDebug>

namespace CueForge {

    // ============================================================================
    // Utility Functions
    // ============================================================================

    QString cueTypeToString(CueType type)
    {
        switch (type) {
        case CueType::Audio:    return "Audio";
        case CueType::Video:    return "Video";
        case CueType::Midi:     return "MIDI";
        case CueType::Group:    return "Group";
        case CueType::Fade:     return "Fade";
        case CueType::Wait:     return "Wait";
        case CueType::Start:    return "Start";
        case CueType::Stop:     return "Stop";
        case CueType::Goto:     return "Goto";
        case CueType::Pause:    return "Pause";
        case CueType::Load:     return "Load";
        case CueType::Reset:    return "Reset";
        case CueType::Arm:      return "Arm";
        case CueType::Disarm:   return "Disarm";
        case CueType::Devamp:   return "Devamp";
        case CueType::Memo:     return "Memo";
        case CueType::Text:     return "Text";
        case CueType::Network:  return "Network";
        case CueType::Light:    return "Light";
        default:                return "Unknown";
        }
    }

    CueType stringToCueType(const QString& str)
    {
        QString lower = str.toLower();
        if (lower == "audio") return CueType::Audio;
        if (lower == "video") return CueType::Video;
        if (lower == "midi") return CueType::Midi;
        if (lower == "group") return CueType::Group;
        if (lower == "fade") return CueType::Fade;
        if (lower == "wait") return CueType::Wait;
        if (lower == "start") return CueType::Start;
        if (lower == "stop") return CueType::Stop;
        if (lower == "goto") return CueType::Goto;
        if (lower == "pause") return CueType::Pause;
        if (lower == "load") return CueType::Load;
        if (lower == "reset") return CueType::Reset;
        if (lower == "arm") return CueType::Arm;
        if (lower == "disarm") return CueType::Disarm;
        if (lower == "devamp") return CueType::Devamp;
        if (lower == "memo") return CueType::Memo;
        if (lower == "text") return CueType::Text;
        if (lower == "network") return CueType::Network;
        if (lower == "light") return CueType::Light;
        return CueType::Audio;
    }

    QString cueStatusToString(CueStatus status)
    {
        switch (status) {
        case CueStatus::Loaded:   return "Loaded";
        case CueStatus::Running:  return "Running";
        case CueStatus::Paused:   return "Paused";
        case CueStatus::Finished: return "Finished";
        case CueStatus::Broken:   return "Broken";
        default:                  return "Unknown";
        }
    }

    // ============================================================================
    // Constructor
    // ============================================================================

    Cue::Cue(CueType type, QObject* parent)
        : QObject(parent)
        , id_(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , type_(type)
        , number_("1")
        , name_("New Cue")
        , duration_(0.0)
        , preWait_(0.0)
        , postWait_(0.0)
        , continueMode_(false)
        , color_(Qt::white)
        , status_(CueStatus::Loaded)
        , isArmed_(true)
        , isBroken_(false)
        , createdTime_(QDateTime::currentDateTime())
        , modifiedTime_(QDateTime::currentDateTime())
    {
    }

    // ============================================================================
    // Basic Properties
    // ============================================================================

    void Cue::setNumber(const QString& number)
    {
        if (number_ != number) {
            number_ = number;
            updateModifiedTime();
            emit numberChanged(number_);
        }
    }

    void Cue::setName(const QString& name)
    {
        if (name_ != name) {
            name_ = name;
            updateModifiedTime();
            emit nameChanged(name_);
        }
    }

    void Cue::setDuration(double seconds)
    {
        seconds = qMax(0.0, seconds);
        if (qAbs(duration_ - seconds) > 0.001) {
            duration_ = seconds;
            updateModifiedTime();
            emit durationChanged(duration_);
        }
    }

    void Cue::setPreWait(double seconds)
    {
        seconds = qMax(0.0, seconds);
        if (qAbs(preWait_ - seconds) > 0.001) {
            preWait_ = seconds;
            updateModifiedTime();
            emit preWaitChanged(preWait_);
        }
    }

    void Cue::setPostWait(double seconds)
    {
        seconds = qMax(0.0, seconds);
        if (qAbs(postWait_ - seconds) > 0.001) {
            postWait_ = seconds;
            updateModifiedTime();
            emit postWaitChanged(postWait_);
        }
    }

    void Cue::setContinueMode(bool enabled)
    {
        if (continueMode_ != enabled) {
            continueMode_ = enabled;
            updateModifiedTime();
            emit continueModeChanged(continueMode_);
        }
    }

    void Cue::setColor(const QColor& color)
    {
        if (color_ != color) {
            color_ = color;
            updateModifiedTime();
            emit colorChanged(color_);
        }
    }

    void Cue::setNotes(const QString& notes)
    {
        if (notes_ != notes) {
            notes_ = notes;
            updateModifiedTime();
            emit notesChanged(notes_);
        }
    }

    void Cue::setStatus(CueStatus status)
    {
        if (status_ != status) {
            status_ = status;
            emit statusChanged(status_);
        }
    }

    void Cue::setArmed(bool armed)
    {
        if (isArmed_ != armed) {
            isArmed_ = armed;
            updateModifiedTime();
        }
    }

    void Cue::setIsBroken(bool broken)
    {
        if (isBroken_ != broken) {
            isBroken_ = broken;
            if (broken) {
                setStatus(CueStatus::Broken);
            }
        }
    }

    // ============================================================================
    // Target System
    // ============================================================================

    void Cue::setTargetCueId(const QString& id)
    {
        if (targetCueId_ != id) {
            targetCueId_ = id;
            updateModifiedTime();
        }
    }

    Cue* Cue::targetCue() const
    {
        CueManager* manager = cueManager();
        if (manager && !targetCueId_.isEmpty()) {
            return manager->getCue(targetCueId_);
        }
        return nullptr;
    }

    bool Cue::hasValidTarget() const
    {
        return targetCue() != nullptr;
    }

    CueManager* Cue::cueManager() const
    {
        QObject* p = parent();
        while (p) {
            if (CueManager* manager = qobject_cast<CueManager*>(p)) {
                return manager;
            }
            p = p->parent();
        }
        return nullptr;
    }

    // ============================================================================
    // Execution Interface
    // ============================================================================

    void Cue::stop(double fadeTime)
    {
        Q_UNUSED(fadeTime);
        setStatus(CueStatus::Loaded);
    }

    void Cue::pause()
    {
        if (status_ == CueStatus::Running) {
            setStatus(CueStatus::Paused);
        }
    }

    void Cue::resume()
    {
        if (status_ == CueStatus::Paused) {
            setStatus(CueStatus::Running);
        }
    }

    bool Cue::canExecute() const
    {
        return isArmed_ && !isBroken_ && status_ != CueStatus::Running;
    }

    bool Cue::validate()
    {
        return !isBroken_;
    }

    QString Cue::validationError() const
    {
        if (isBroken_) {
            return "Cue is broken";
        }
        return QString();
    }

    // ============================================================================
    // Serialization
    // ============================================================================

    QJsonObject Cue::toJson() const
    {
        QJsonObject json;

        json["id"] = id_;
        json["type"] = cueTypeToString(type_);
        json["number"] = number_;
        json["name"] = name_;
        json["duration"] = duration_;
        json["preWait"] = preWait_;
        json["postWait"] = postWait_;
        json["continueMode"] = continueMode_;
        json["color"] = color_.name();
        json["notes"] = notes_;
        json["isArmed"] = isArmed_;
        json["targetCueId"] = targetCueId_;
        json["createdTime"] = createdTime_.toString(Qt::ISODate);
        json["modifiedTime"] = modifiedTime_.toString(Qt::ISODate);

        return json;
    }

    void Cue::fromJson(const QJsonObject& json)
    {
        if (json.contains("id")) {
            id_ = json["id"].toString();
        }

        setNumber(json.value("number").toString("1"));
        setName(json.value("name").toString("New Cue"));
        setDuration(json.value("duration").toDouble(0.0));
        setPreWait(json.value("preWait").toDouble(0.0));
        setPostWait(json.value("postWait").toDouble(0.0));
        setContinueMode(json.value("continueMode").toBool(false));

        if (json.contains("color")) {
            setColor(QColor(json["color"].toString()));
        }

        setNotes(json.value("notes").toString());
        setArmed(json.value("isArmed").toBool(true));
        setTargetCueId(json.value("targetCueId").toString());

        if (json.contains("createdTime")) {
            createdTime_ = QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
        }
        if (json.contains("modifiedTime")) {
            modifiedTime_ = QDateTime::fromString(json["modifiedTime"].toString(), Qt::ISODate);
        }
    }

    // ============================================================================
    // Timestamps
    // ============================================================================

    void Cue::updateModifiedTime()
    {
        modifiedTime_ = QDateTime::currentDateTime();
    }

} // namespace CueForge