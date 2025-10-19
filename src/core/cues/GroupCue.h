// ============================================================================
// GroupCue.h - Hierarchical cue grouping
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "../Cue.h"
#include <QList>

namespace CueForge {

    class GroupCue : public Cue
    {
        Q_OBJECT
            Q_PROPERTY(int childCount READ childCount NOTIFY childCountChanged)
            Q_PROPERTY(GroupMode mode READ mode WRITE setMode NOTIFY modeChanged)

    public:
        enum class GroupMode {
            Sequential,
            Parallel
        };
        Q_ENUM(GroupMode)

            explicit GroupCue(QObject* parent = nullptr);
        ~GroupCue() override = default;

        CueList children() const { return children_; }
        void setChildren(const CueList& children);
        void addChild(CuePtr cue, int index = -1);
        bool removeChild(const QString& cueId);
        Cue* getChild(const QString& cueId) const;
        Cue* getChildAt(int index) const;
        int childCount() const { return children_.size(); }
        void clearChildren();
        Cue::CuePtr removeChild(int index);

        GroupMode mode() const { return mode_; }
        void setMode(GroupMode mode);

        double totalDuration() const;

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
        void childCountChanged(int count);
        void modeChanged(GroupMode mode);
        void childExecutionStarted(const QString& childId);
        void childExecutionFinished(const QString& childId);

    private slots:
        void onChildFinished(const QString& childId);

    private:
        void executeNextChild();
        void executeAllChildren();

        CueList children_;
        GroupMode mode_;
        int currentChildIndex_;
        QSet<QString> activeChildren_;
    };

} // namespace CueForge

Q_DECLARE_METATYPE(CueForge::GroupCue::GroupMode)