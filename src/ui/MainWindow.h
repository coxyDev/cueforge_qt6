// ============================================================================
// MainWindow.h - Main application window (UPDATED)
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QMainWindow>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QToolBar;
class QLabel;
class QAction;
QT_END_NAMESPACE

namespace CueForge {

    class CueManager;
    class ErrorHandler;
    class CueListWidget;
    class InspectorWidget;
    class TransportWidget;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(CueManager* cueManager,
            ErrorHandler* errorHandler,
            QWidget* parent = nullptr);
        ~MainWindow() override;

    protected:
        void closeEvent(QCloseEvent* event) override;

    private slots:
        void onNewWorkspace();
        void onOpenWorkspace();
        void onSaveWorkspace();
        void onSaveWorkspaceAs();

        void onUndo();
        void onRedo();
        void onCut();
        void onCopy();
        void onPaste();
        void onDelete();
        void onSelectAll();

        void onCreateAudioCue();
        void onCreateGroupCue();
        void onCreateWaitCue();
        void onCreateControlCue();
        void onGroupSelection();
        void onUngroupSelection();

        void onGo();
        void onStop();
        void onPause();
        void onPanic();

        void onPreferences();
        void onAbout();

        void onCueCountChanged(int count);
        void onUnsavedChangesChanged(bool hasChanges);
        void onErrorOccurred(const QString& message);

    private:
        void createActions();
        void createMenus();
        void createToolBars();
        void createDockWidgets();
        void createStatusBar();
        void setupConnections();
        void loadSettings();
        void saveSettings();
        void applyStyleSheet();
        bool maybeSave();
        void updateWindowTitle();

        QPointer<CueManager> cueManager_;
        QPointer<ErrorHandler> errorHandler_;

        CueListWidget* cueListWidget_;
        InspectorWidget* inspectorWidget_;
        TransportWidget* transportWidget_;

        QDockWidget* inspectorDock_;
        QDockWidget* transportDock_;

        // Actions - File
        QAction* actionNew_;
        QAction* actionOpen_;
        QAction* actionSave_;
        QAction* actionSaveAs_;
        QAction* actionQuit_;

        // Actions - Edit
        QAction* actionUndo_;
        QAction* actionRedo_;
        QAction* actionCut_;
        QAction* actionCopy_;
        QAction* actionPaste_;
        QAction* actionDelete_;
        QAction* actionSelectAll_;

        // Actions - Cue
        QAction* actionNewAudioCue_;
        QAction* actionNewGroupCue_;
        QAction* actionNewWaitCue_;
        QAction* actionNewControlCue_;
        QAction* actionGroupSelection_;
        QAction* actionUngroupSelection_;

        // Actions - Playback
        QAction* actionGo_;
        QAction* actionStop_;
        QAction* actionPause_;
        QAction* actionPanic_;

        // Actions - View
        QAction* actionShowInspector_;
        QAction* actionShowTransport_;

        // Actions - Help
        QAction* actionPreferences_;
        QAction* actionAbout_;

        QToolBar* fileToolBar_;
        QToolBar* editToolBar_;
        QToolBar* cueToolBar_;
        QToolBar* playbackToolBar_;

        QLabel* statusLabel_;
        QLabel* cueCountLabel_;

        QString currentFilePath_;
    };

} // namespace CueForge