// ============================================================================
// CueManager.h - Central cue management system
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "Cue.h"
#include <QObject>
#include <QList>
#include <QSet>
#include <QJsonObject>
#include <QJsonArray>

namespace CueForge {

    class CueManager : public QObject
    {
        Q_OBJECT
            Q_PROPERTY(int cueCount READ cueCount NOTIFY cueCountChanged)
            Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
            Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY unsavedChangesChanged)

    public:
        explicit CueManager(QObject* parent = nullptr);
        ~CueManager() override;

        // ========== Cue Lifecycle ==========

        Cue* createCue(CueType type, int index = -1);
        bool removeCue(const QString& cueId);
        void removeAllCues();
        Cue* duplicateCue(const QString& cueId);
        bool renameCue(const QString& cueId, const QString& newNumber);
        void renumberAllCues();

        // ========== Cue Access ==========

        Cue* getCue(const QString& cueId) const;
        Cue* getCueByNumber(const QString& number) const;
        int getCueIndex(const QString& cueId) const;
        Cue::CueList allCues() const { return cues_; }
        int cueCount() const { return cues_.size(); }

        // ========== Selection System ==========

        QStringList selectedCueIds() const { return selectedCueIds_; }
        Cue::CueList selectedCues() const;
        int selectedCount() const { return selectedCueIds_.size(); }
        bool isCueSelected(const QString& cueId) const;

        void selectCue(const QString& cueId, bool clearOthers = true);
        void addToSelection(const QString& cueId);
        void removeFromSelection(const QString& cueId);
        void toggleSelection(const QString& cueId);
        void selectRange(const QString& startId, const QString& endId);
        void selectAll();
        void clearSelection();

        // ========== Group Management ==========

        Cue* createGroupFromSelection(const QString& groupName = "Group");
        bool ungroupCue(const QString& groupId);
        bool isGroupExpanded(const QString& groupId) const;
        void setGroupExpanded(const QString& groupId, bool expanded);

        // ========== Cue Ordering ==========

        bool moveCue(const QString& cueId, int newIndex);
        bool moveCueUp(const QString& cueId);
        bool moveCueDown(const QString& cueId);

        // ========== Playback Control ==========

        bool go();
        void stop();
        void stopCue(const QString& cueId, double fadeTime = 0.0);
        void pause();
        void panic();
        QStringList activeCueIds() const;

        // ========== Standby System (Playhead) ==========

        void setStandByCue(const QString& cueId);
        Cue* standByCue() const;
        void nextCue();
        void previousCue();

        // ========== Workspace Management ==========

        void newWorkspace();
        bool loadWorkspace(const QJsonObject& json);
        QJsonObject saveWorkspace() const;
        bool hasUnsavedChanges() const { return hasUnsavedChanges_; }
        void markSaved();
        void markUnsaved();

        // ========== Clipboard Operations ==========

        void copy();
        void cut();
        void paste(int index = -1);

    signals:
        void cueAdded(Cue* cue, int index);
        void cueRemoved(const QString& cueId);
        void cueMoved(const QString& cueId, int oldIndex, int newIndex);
        void cueUpdated(Cue* cue);
        void cueCountChanged(int count);

        void selectionChanged();
        void selectionCleared();

        void playbackStarted(const QString& cueId);
        void playbackStopped(const QString& cueId);
        void playbackStateChanged();

        void standByCueChanged(const QString& cueId);

        void workspaceLoaded();
        void workspaceCleared();
        void unsavedChangesChanged(bool hasChanges);

        void error(const QString& message);
        void warning(const QString& message);

    private slots:
        void onCueFinished(const QString& cueId);
        void onCueStatusChanged(Cue* cue);

    private:
        void connectCueSignals(Cue* cue);
        void disconnectCueSignals(Cue* cue);
        void executeCue(Cue* cue);
        bool canExecuteCue(Cue* cue) const;
        void advanceStandBy();
        QString generateCueNumber() const;
        bool validateCueOperation(const QString& operation, const QString& cueId) const;
        bool validateGroupOperation(const QString& groupId) const;

        Cue::CueList cues_;
        QStringList selectedCueIds_;
        QSet<QString> activeCues_;
        QSet<QString> expandedGroups_;
        QString standByCueId_;
        QList<QJsonObject> clipboard_;
        bool isPaused_;
        bool hasUnsavedChanges_;
    };

} // namespace CueForge