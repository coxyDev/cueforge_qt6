// ============================================================================
// CueManager.h - Central cue management and workspace control
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPointer>
#include <QJsonObject>
#include <memory>
#include "Cue.h"

namespace CueForge {

    class ErrorHandler;

    class CueManager : public QObject
    {
        Q_OBJECT

    public:
        using CueList = QList<Cue::CuePtr>;

        explicit CueManager(QObject* parent = nullptr);
        ~CueManager() override = default;

        // Cue creation and management
        Cue* createCue(CueType type, int index = -1);
        Cue::CuePtr removeChild(int index);
        bool removeCue(const QString& cueId);
        void removeCueWithoutSignals(const QString& cueId);
        Cue* getCue(const QString& cueId) const;
        int getCueIndex(const QString& cueId) const;
        bool moveCue(const QString& cueId, int newIndex);
        bool moveCueUp(const QString& cueId);
        bool moveCueDown(const QString& cueId);
        Cue* duplicateCue(const QString& cueId);

        // Standby and playback
        Cue* standByCue() const;
        void setStandByCue(const QString& cueId);
        void setStandByCue(Cue* cue);
        QStringList activeCueIds() const;

        bool go();
        void stop();
        void pause();
        void panic();
        void previousCue();
        void nextCue();

        // Group operations
        Cue* createGroupFromSelection(const QString& groupName = "Group");
        bool ungroupCue(const QString& groupCueId);

        // Selection
        void selectCue(const QString& cueId);
        void addToSelection(const QString& cueId);
        void removeFromSelection(const QString& cueId);
        void clearSelection();
        void selectAll();
        QStringList selectedCueIds() const { return selectedCueIds_; }
        int selectedCount() const { return selectedCueIds_.size(); }

        // Clipboard
        void copy();
        void cut();
        void paste(int index = -1);

        // Workspace management
        void newWorkspace();
        bool loadWorkspace(const QJsonObject& workspace);
        QJsonObject saveWorkspace() const;
        void markSaved();
        void markUnsaved();
        bool hasUnsavedChanges() const { return hasUnsavedChanges_; }

        // Queries
        const CueList& allCues() const { return cues_; }
        int cueCount() const { return cues_.size(); }
        void renumberAllCues();

    signals:
        void cueAdded(Cue* cue, int index);
        void cueRemoved(const QString& cueId);
        void cueMoved(const QString& cueId, int oldIndex, int newIndex);
        void cueUpdated(Cue* cue);
        void cueCountChanged(int count);

        void selectionChanged();
        void standByCueChanged(const QString& cueId);
        void playbackStateChanged();

        void workspaceCleared();
        void unsavedChangesChanged(bool hasChanges);

        void error(const QString& message);
        void warning(const QString& message);
        void info(const QString& message);

    private:
        CueList cues_;
        QStringList selectedCueIds_;
        QStringList activeCueIds_;
        QPointer<Cue> standByCue_;
        QList<QJsonObject> clipboard_;
        QString currentWorkspacePath_;
        bool hasUnsavedChanges_;
    };

} // namespace CueForge