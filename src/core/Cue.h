// ============================================================================
// Cue.h - Base class for all cue types
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QDateTime>
#include <QJsonObject>
#include <QVariantMap>
#include <memory>

namespace CueForge {

    class CueManager;

    enum class CueType {
        Audio,
        Video,
        Midi,
        Group,
        Fade,
        Wait,
        Start,
        Stop,
        Goto,
        Pause,
        Load,
        Reset,
        Arm,
        Disarm,
        Devamp,
        Memo,
        Text,
        Network,
        Light
    };

    enum class CueStatus {
        Idle,
        Loaded,
        Running,
        Paused,
        Stopped,
        Finished,
        Broken
    };

    class Cue : public QObject
    {
        Q_OBJECT
            Q_PROPERTY(QString id READ id CONSTANT)
            Q_PROPERTY(QString number READ number WRITE setNumber NOTIFY numberChanged)
            Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
            Q_PROPERTY(double duration READ duration WRITE setDuration NOTIFY durationChanged)
            Q_PROPERTY(double preWait READ preWait WRITE setPreWait NOTIFY preWaitChanged)
            Q_PROPERTY(double postWait READ postWait WRITE setPostWait NOTIFY postWaitChanged)
            Q_PROPERTY(bool continueMode READ continueMode WRITE setContinueMode NOTIFY continueModeChanged)
            Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
            Q_PROPERTY(QString notes READ notes WRITE setNotes NOTIFY notesChanged)
            Q_PROPERTY(CueStatus status READ status WRITE setStatus NOTIFY statusChanged)

    public:
        using CuePtr = std::shared_ptr<Cue>;
        using CueList = QList<CuePtr>;

        explicit Cue(CueType type, QObject* parent = nullptr);
        virtual ~Cue() = default;

        // ========== Basic Properties ==========

        QString id() const { return id_; }
        CueType type() const { return type_; }

        QString number() const { return number_; }
        void setNumber(const QString& number);

        QString name() const { return name_; }
        void setName(const QString& name);

        double duration() const { return duration_; }
        void setDuration(double seconds);

        double preWait() const { return preWait_; }
        void setPreWait(double seconds);

        double postWait() const { return postWait_; }
        void setPostWait(double seconds);

        bool continueMode() const { return continueMode_; }
        void setContinueMode(bool enabled);

        QColor color() const { return color_; }
        void setColor(const QColor& color);

        QString notes() const { return notes_; }
        void setNotes(const QString& notes);

        CueStatus status() const { return status_; }
        void setStatus(CueStatus status);

        bool isArmed() const { return isArmed_; }
        void setArmed(bool armed);

        bool isBroken() const { return isBroken_; }
        void setIsBroken(bool broken);

        // ========== Target System ==========

        QString targetCueId() const { return targetCueId_; }
        void setTargetCueId(const QString& id);
        Cue* targetCue() const;
        bool hasValidTarget() const;

        CueManager* cueManager() const;

        // ========== Execution Interface ==========

        virtual bool execute();
        virtual void stop(double fadeTime = 0.0);
        virtual void pause();
        virtual void resume();
        virtual bool canExecute() const;
        virtual bool validate();
        virtual QString validationError() const;

        // ========== Serialization ==========

        virtual QJsonObject toJson() const;
        virtual void fromJson(const QJsonObject& json);
        virtual std::unique_ptr<Cue> clone() const = 0;

        // ========== Timestamps ==========

        QDateTime createdTime() const { return createdTime_; }
        QDateTime modifiedTime() const { return modifiedTime_; }
        void updateModifiedTime();

    signals:
        void numberChanged(const QString& number);
        void nameChanged(const QString& name);
        void durationChanged(double duration);
        void preWaitChanged(double seconds);
        void postWaitChanged(double seconds);
        void continueModeChanged(bool enabled);
        void colorChanged(const QColor& color);
        void notesChanged(const QString& notes);
        void statusChanged(CueStatus status);
        void executionStarted();
        void executionFinished();
        void executionProgress(double progress);
        void error(const QString& message);
        void warning(const QString& message);

    protected:
        QString id_;
        CueType type_;
        QString number_;
        QString name_;
        double duration_;
        double preWait_;
        double postWait_;
        bool continueMode_;
        QColor color_;
        QString notes_;
        CueStatus status_;
        bool isArmed_;
        bool isBroken_;
        QString targetCueId_;
        QDateTime createdTime_;
        QDateTime modifiedTime_;
    };

    QString cueTypeToString(CueType type);
    CueType stringToCueType(const QString& str);
    QString cueStatusToString(CueStatus status);

} // namespace CueForge

Q_DECLARE_METATYPE(CueForge::CueType)
Q_DECLARE_METATYPE(CueForge::CueStatus)