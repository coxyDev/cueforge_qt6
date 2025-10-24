// ============================================================================
// MainWindow.cpp - Main application window (UPDATED)
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "MainWindow.h"
#include "CueListWidget.h"
#include "InspectorWidget.h"
#include "TransportWidget.h"
#include "../core/CueManager.h"
#include "../core/ErrorHandler.h"
#include "../audio/AudioEngineQt.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QJsonDocument>
#include <QFile>
#include <QInputDialog>

namespace CueForge {

    MainWindow::MainWindow(CueManager* cueManager,
        ErrorHandler* errorHandler,
        QWidget* parent)
        : QMainWindow(parent)
        , cueManager_(cueManager)
        , errorHandler_(errorHandler)
        , cueListWidget_(nullptr)
        , inspectorWidget_(nullptr)
        , transportWidget_(nullptr)
    {
        setWindowTitle("CueForge");
        resize(1400, 900);
        setMinimumSize(1000, 600);

        #ifdef HAVE_JUCE_AUDIO
                audioEngine_ = new AudioEngineQt(this);
                if (audioEngine_->initialize()) {
                    qDebug() << "✓ Audio engine initialized";
                    statusLabel_->setText("Audio engine ready");
                }
                else {
                    qWarning() << "✗ Audio engine failed to initialize";
                    statusLabel_->setText("Audio engine unavailable");
                }
        #else
                qWarning() << "Built without JUCE audio support";
        #endif

        createActions();
        createMenus();
        createToolBars();
        createDockWidgets();
        createStatusBar();
        setupConnections();
        applyStyleSheet();
        loadSettings();
        updateWindowTitle();

        cueListWidget_ = new CueListWidget(cueManager_, this);
        setCentralWidget(cueListWidget_);
    }

    MainWindow::~MainWindow()
    {
        #ifdef HAVE_JUCE_AUDIO
                if (audioEngine_) {
                    audioEngine_->shutdown();
                }
        #endif

        saveSettings();
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (maybeSave()) {
            saveSettings();
            event->accept();
        }
        else {
            event->ignore();
        }
    }

    void MainWindow::createActions()
    {
        // File Actions
        actionNew_ = new QAction(tr("&New Workspace"), this);
        actionNew_->setShortcut(QKeySequence::New);
        actionNew_->setStatusTip(tr("Create a new workspace"));
        connect(actionNew_, &QAction::triggered, this, &MainWindow::onNewWorkspace);

        actionOpen_ = new QAction(tr("&Open Workspace..."), this);
        actionOpen_->setShortcut(QKeySequence::Open);
        actionOpen_->setStatusTip(tr("Open an existing workspace"));
        connect(actionOpen_, &QAction::triggered, this, &MainWindow::onOpenWorkspace);

        actionSave_ = new QAction(tr("&Save"), this);
        actionSave_->setShortcut(QKeySequence::Save);
        actionSave_->setStatusTip(tr("Save the workspace"));
        connect(actionSave_, &QAction::triggered, this, &MainWindow::onSaveWorkspace);

        actionSaveAs_ = new QAction(tr("Save &As..."), this);
        actionSaveAs_->setShortcut(QKeySequence::SaveAs);
        connect(actionSaveAs_, &QAction::triggered, this, &MainWindow::onSaveWorkspaceAs);

        actionQuit_ = new QAction(tr("&Quit"), this);
        actionQuit_->setShortcut(QKeySequence::Quit);
        connect(actionQuit_, &QAction::triggered, this, &QWidget::close);

        // Edit Actions
        actionUndo_ = new QAction(tr("&Undo"), this);
        actionUndo_->setShortcut(QKeySequence::Undo);
        actionUndo_->setEnabled(false);

        actionRedo_ = new QAction(tr("&Redo"), this);
        actionRedo_->setShortcut(QKeySequence::Redo);
        actionRedo_->setEnabled(false);

        actionCut_ = new QAction(tr("Cu&t"), this);
        actionCut_->setShortcut(QKeySequence::Cut);
        connect(actionCut_, &QAction::triggered, this, &MainWindow::onCut);

        actionCopy_ = new QAction(tr("&Copy"), this);
        actionCopy_->setShortcut(QKeySequence::Copy);
        connect(actionCopy_, &QAction::triggered, this, &MainWindow::onCopy);

        actionPaste_ = new QAction(tr("&Paste"), this);
        actionPaste_->setShortcut(QKeySequence::Paste);
        connect(actionPaste_, &QAction::triggered, this, &MainWindow::onPaste);

        actionDelete_ = new QAction(tr("&Delete"), this);
        actionDelete_->setShortcut(QKeySequence::Delete);
        connect(actionDelete_, &QAction::triggered, this, &MainWindow::onDelete);

        actionSelectAll_ = new QAction(tr("Select &All"), this);
        actionSelectAll_->setShortcut(QKeySequence::SelectAll);
        connect(actionSelectAll_, &QAction::triggered, this, &MainWindow::onSelectAll);

        // Cue Actions - UPDATED SHORTCUTS
        actionNewAudioCue_ = new QAction(tr("🎵 &Audio Cue"), this);
        actionNewAudioCue_->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);  // Changed from Ctrl+A
        actionNewAudioCue_->setStatusTip(tr("Create new audio cue (Ctrl+Shift+A)"));
        connect(actionNewAudioCue_, &QAction::triggered, this, &MainWindow::onCreateAudioCue);

        actionNewGroupCue_ = new QAction(tr("📁 &Group Cue"), this);
        actionNewGroupCue_->setShortcut(Qt::CTRL | Qt::Key_G);
        actionNewGroupCue_->setStatusTip(tr("Create new group cue (Ctrl+G)"));
        connect(actionNewGroupCue_, &QAction::triggered, this, &MainWindow::onCreateGroupCue);

        actionNewWaitCue_ = new QAction(tr("⏱️ &Wait Cue"), this);
        actionNewWaitCue_->setShortcut(Qt::CTRL | Qt::Key_W);
        connect(actionNewWaitCue_, &QAction::triggered, this, &MainWindow::onCreateWaitCue);

        actionNewControlCue_ = new QAction(tr("⚙️ &Control Cue"), this);
        actionNewControlCue_->setShortcut(Qt::CTRL | Qt::Key_K);
        connect(actionNewControlCue_, &QAction::triggered, this, &MainWindow::onCreateControlCue);

        // NEW: Group/Ungroup actions
        actionGroupSelection_ = new QAction(tr("Group Selected Cues"), this);
        actionGroupSelection_->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_G);
        actionGroupSelection_->setStatusTip(tr("Create a group from selected cues (Ctrl+Shift+G)"));
        connect(actionGroupSelection_, &QAction::triggered, this, &MainWindow::onGroupSelection);

        actionUngroupSelection_ = new QAction(tr("Ungroup"), this);
        actionUngroupSelection_->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_U);
        actionUngroupSelection_->setStatusTip(tr("Ungroup selected group cue (Ctrl+Shift+U)"));
        connect(actionUngroupSelection_, &QAction::triggered, this, &MainWindow::onUngroupSelection);

        // Playback Actions
        actionGo_ = new QAction(tr("▶️ &GO"), this);
        actionGo_->setShortcut(Qt::Key_Space);
        actionGo_->setStatusTip(tr("Execute standby cue (Space)"));
        connect(actionGo_, &QAction::triggered, this, &MainWindow::onGo);

        actionStop_ = new QAction(tr("⏹️ &Stop"), this);
        actionStop_->setShortcut(Qt::Key_S);
        connect(actionStop_, &QAction::triggered, this, &MainWindow::onStop);

        actionPause_ = new QAction(tr("⏸️ &Pause"), this);
        actionPause_->setShortcut(Qt::Key_P);
        connect(actionPause_, &QAction::triggered, this, &MainWindow::onPause);

        actionPanic_ = new QAction(tr("🛑 PANIC"), this);
        actionPanic_->setShortcut(Qt::CTRL | Qt::Key_Escape);
        connect(actionPanic_, &QAction::triggered, this, &MainWindow::onPanic);

        // View Actions
        actionShowInspector_ = new QAction(tr("Show &Inspector"), this);
        actionShowInspector_->setCheckable(true);
        actionShowInspector_->setChecked(true);

        actionShowTransport_ = new QAction(tr("Show &Transport"), this);
        actionShowTransport_->setCheckable(true);
        actionShowTransport_->setChecked(true);

        // Help Actions
        actionPreferences_ = new QAction(tr("&Preferences..."), this);
        actionPreferences_->setShortcut(QKeySequence::Preferences);
        connect(actionPreferences_, &QAction::triggered, this, &MainWindow::onPreferences);

        actionAbout_ = new QAction(tr("&About CueForge"), this);
        connect(actionAbout_, &QAction::triggered, this, &MainWindow::onAbout);
    }

    void MainWindow::createMenus()
    {
        QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
        fileMenu->addAction(actionNew_);
        fileMenu->addAction(actionOpen_);
        fileMenu->addSeparator();
        fileMenu->addAction(actionSave_);
        fileMenu->addAction(actionSaveAs_);
        fileMenu->addSeparator();
        fileMenu->addAction(actionQuit_);

        QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
        editMenu->addAction(actionUndo_);
        editMenu->addAction(actionRedo_);
        editMenu->addSeparator();
        editMenu->addAction(actionCut_);
        editMenu->addAction(actionCopy_);
        editMenu->addAction(actionPaste_);
        editMenu->addAction(actionDelete_);
        editMenu->addSeparator();
        editMenu->addAction(actionSelectAll_);
        editMenu->addSeparator();
        editMenu->addAction(actionGroupSelection_);
        editMenu->addAction(actionUngroupSelection_);

        QMenu* cueMenu = menuBar()->addMenu(tr("&Cue"));
        cueMenu->addAction(actionNewAudioCue_);
        cueMenu->addAction(actionNewGroupCue_);
        cueMenu->addAction(actionNewWaitCue_);
        cueMenu->addAction(actionNewControlCue_);

        QMenu* playbackMenu = menuBar()->addMenu(tr("&Playback"));
        playbackMenu->addAction(actionGo_);
        playbackMenu->addAction(actionStop_);
        playbackMenu->addAction(actionPause_);
        playbackMenu->addSeparator();
        playbackMenu->addAction(actionPanic_);

        QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
        viewMenu->addAction(actionShowInspector_);
        viewMenu->addAction(actionShowTransport_);

        QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
        helpMenu->addAction(actionPreferences_);
        helpMenu->addSeparator();
        helpMenu->addAction(actionAbout_);
    }

    void MainWindow::createToolBars()
    {
        fileToolBar_ = addToolBar(tr("File"));
        fileToolBar_->setObjectName("FileToolBar");
        fileToolBar_->setMovable(false);
        fileToolBar_->addAction(actionNew_);
        fileToolBar_->addAction(actionOpen_);
        fileToolBar_->addAction(actionSave_);

        editToolBar_ = addToolBar(tr("Edit"));
        editToolBar_->setObjectName("EditToolBar");
        editToolBar_->setMovable(false);
        editToolBar_->addAction(actionCut_);
        editToolBar_->addAction(actionCopy_);
        editToolBar_->addAction(actionPaste_);
        editToolBar_->addAction(actionDelete_);
        editToolBar_->addSeparator();
        editToolBar_->addAction(actionGroupSelection_);
        editToolBar_->addAction(actionUngroupSelection_);

        cueToolBar_ = addToolBar(tr("Cues"));
        cueToolBar_->setObjectName("CueToolBar");
        cueToolBar_->setMovable(false);
        cueToolBar_->addAction(actionNewAudioCue_);
        cueToolBar_->addAction(actionNewGroupCue_);
        cueToolBar_->addAction(actionNewWaitCue_);
        cueToolBar_->addAction(actionNewControlCue_);

        playbackToolBar_ = addToolBar(tr("Playback"));
        playbackToolBar_->setObjectName("PlaybackToolBar");
        playbackToolBar_->setMovable(false);
        playbackToolBar_->setIconSize(QSize(32, 32));
        playbackToolBar_->addAction(actionGo_);
        playbackToolBar_->addAction(actionStop_);
        playbackToolBar_->addAction(actionPause_);
    }

    void MainWindow::createDockWidgets()
    {
        inspectorDock_ = new QDockWidget(tr("Inspector"), this);
        inspectorDock_->setObjectName("InspectorDock");
        inspectorDock_->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        inspectorWidget_ = new InspectorWidget(cueManager_, this);
        inspectorDock_->setWidget(inspectorWidget_);
        addDockWidget(Qt::RightDockWidgetArea, inspectorDock_);

        connect(actionShowInspector_, &QAction::toggled,
            inspectorDock_, &QDockWidget::setVisible);
        connect(inspectorDock_, &QDockWidget::visibilityChanged,
            actionShowInspector_, &QAction::setChecked);

        transportDock_ = new QDockWidget(tr("Transport"), this);
        transportDock_->setObjectName("TransportDock");
        transportDock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        transportWidget_ = new TransportWidget(cueManager_, this);
        transportDock_->setWidget(transportWidget_);
        addDockWidget(Qt::BottomDockWidgetArea, transportDock_);

        connect(actionShowTransport_, &QAction::toggled,
            transportDock_, &QDockWidget::setVisible);
        connect(transportDock_, &QDockWidget::visibilityChanged,
            actionShowTransport_, &QAction::setChecked);
    }

    void MainWindow::createStatusBar()
    {
        statusLabel_ = new QLabel(tr("Ready"));
        statusBar()->addWidget(statusLabel_, 1);

        cueCountLabel_ = new QLabel(tr("Cues: 0"));
        statusBar()->addPermanentWidget(cueCountLabel_);
    }

    void MainWindow::setupConnections()
    {
        if (cueManager_) {
            connect(cueManager_, &CueManager::cueCountChanged,
                this, &MainWindow::onCueCountChanged);
            connect(cueManager_, &CueManager::unsavedChangesChanged,
                this, &MainWindow::onUnsavedChangesChanged);
            connect(cueManager_, &CueManager::error,
                this, &MainWindow::onErrorOccurred);
        }
    }

    void MainWindow::applyStyleSheet()
    {
        QString styleSheet = R"(
        QMainWindow {
            background-color: #2b2b2b;
        }
        
        QMenuBar {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border-bottom: 1px solid #555;
        }
        
        QMenuBar::item:selected {
            background-color: #4a90e2;
        }
        
        QMenu {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border: 1px solid #555;
        }
        
        QMenu::item:selected {
            background-color: #4a90e2;
        }
        
        QToolBar {
            background-color: #3c3c3c;
            border: none;
            spacing: 3px;
            padding: 4px;
        }
        
        QToolButton {
            background-color: #4c4c4c;
            color: #e0e0e0;
            border: 1px solid #555;
            border-radius: 3px;
            padding: 5px 10px;
            margin: 2px;
        }
        
        QToolButton:hover {
            background-color: #5a5a5a;
            border-color: #4a90e2;
        }
        
        QToolButton:pressed {
            background-color: #4a90e2;
        }
        
        QStatusBar {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border-top: 1px solid #555;
        }
        
        QDockWidget {
            color: #e0e0e0;
            titlebar-close-icon: url(close.png);
            titlebar-normal-icon: url(float.png);
        }
        
        QDockWidget::title {
            background-color: #3c3c3c;
            border: 1px solid #555;
            padding: 4px;
        }
    )";

        setStyleSheet(styleSheet);
    }

    // Slot implementations...

    void MainWindow::onNewWorkspace()
    {
        if (!maybeSave()) return;

        cueManager_->newWorkspace();
        currentFilePath_.clear();
        updateWindowTitle();
        statusLabel_->setText(tr("New workspace created"));
    }

    void MainWindow::onOpenWorkspace()
    {
        if (!maybeSave()) return;

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open Workspace"),
            QString(),
            tr("CueForge Workspaces (*.cueforge);;All Files (*)")
        );

        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Error"),
                tr("Could not open file: %1").arg(file.errorString()));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (cueManager_->loadWorkspace(doc.object())) {
            currentFilePath_ = fileName;
            updateWindowTitle();
            statusLabel_->setText(tr("Workspace loaded"));
        }
        else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load workspace"));
        }
    }

    void MainWindow::onSaveWorkspace()
    {
        if (currentFilePath_.isEmpty()) {
            onSaveWorkspaceAs();
            return;
        }

        QJsonObject workspace = cueManager_->saveWorkspace();
        QJsonDocument doc(workspace);

        QFile file(currentFilePath_);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Error"),
                tr("Could not save file: %1").arg(file.errorString()));
            return;
        }

        file.write(doc.toJson());
        file.close();

        cueManager_->markSaved();
        updateWindowTitle();
        statusLabel_->setText(tr("Workspace saved"));
    }

    void MainWindow::onSaveWorkspaceAs()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("Save Workspace"),
            QString(),
            tr("CueForge Workspaces (*.cueforge);;All Files (*)")
        );

        if (fileName.isEmpty()) return;

        if (!fileName.endsWith(".cueforge", Qt::CaseInsensitive)) {
            fileName += ".cueforge";
        }

        currentFilePath_ = fileName;
        onSaveWorkspace();
    }

    void MainWindow::onUndo() { /* TODO */ }
    void MainWindow::onRedo() { /* TODO */ }

    void MainWindow::onCut()
    {
        cueManager_->cut();
        statusLabel_->setText(tr("Cut"));
    }

    void MainWindow::onCopy()
    {
        cueManager_->copy();
        statusLabel_->setText(tr("Copied"));
    }

    void MainWindow::onPaste()
    {
        cueManager_->paste();
        statusLabel_->setText(tr("Pasted"));
    }

    void MainWindow::onDelete()
    {
        if (!cueManager_) return;

        QStringList selected = cueManager_->selectedCueIds();
        if (selected.isEmpty()) return;

        // Ask for confirmation if deleting multiple
        if (selected.size() > 1) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                tr("Delete Cues"),
                tr("Delete %1 cues?").arg(selected.size()),
                QMessageBox::Yes | QMessageBox::No
            );

            if (reply != QMessageBox::Yes) {
                return;
            }
        }

        // Delete in reverse order to avoid index issues
        // (though our new code should handle this)
        int count = 0;
        for (const QString& id : selected) {
            if (cueManager_->removeCue(id)) {
                count++;
            }
        }

        statusLabel_->setText(tr("Deleted %1 cue(s)").arg(count));
    }

    void MainWindow::onSelectAll()
    {
        cueManager_->selectAll();
    }

    void MainWindow::onCreateAudioCue()
    {
        cueManager_->createCue(CueType::Audio);
        statusLabel_->setText(tr("Created audio cue"));
    }

    void MainWindow::onCreateGroupCue()
    {
        cueManager_->createCue(CueType::Group);
        statusLabel_->setText(tr("Created group cue"));
    }

    void MainWindow::onCreateWaitCue()
    {
        cueManager_->createCue(CueType::Wait);
        statusLabel_->setText(tr("Created wait cue"));
    }

    void MainWindow::onCreateControlCue()
    {
        cueManager_->createCue(CueType::Start);
        statusLabel_->setText(tr("Created control cue"));
    }

    void MainWindow::onGroupSelection()
    {
        if (!cueManager_) return;

        QStringList selected = cueManager_->selectedCueIds();

        if (selected.isEmpty()) {
            statusLabel_->setText(tr("No cues selected to group"));
            return;
        }

        if (selected.size() < 2) {
            statusLabel_->setText(tr("Select at least 2 cues to create a group"));
            return;
        }

        bool ok;
        QString groupName = QInputDialog::getText(
            this,
            tr("Create Group"),
            tr("Group name:"),
            QLineEdit::Normal,
            tr("Group"),
            &ok
        );

        if (ok && !groupName.isEmpty()) {
            Cue* group = cueManager_->createGroupFromSelection(groupName);
            if (group) {
                statusLabel_->setText(tr("Created group: %1").arg(groupName));
            }
            else {
                statusLabel_->setText(tr("Failed to create group"));
            }
        }
    }

    void MainWindow::onUngroupSelection()
    {
        QStringList selected = cueManager_->selectedCueIds();
        if (selected.size() != 1) {
            QMessageBox::information(this, tr("Ungroup"),
                tr("Please select exactly one group cue to ungroup."));
            return;
        }

        if (cueManager_->ungroupCue(selected.first())) {
            statusLabel_->setText(tr("Ungrouped"));
        }
        else {
            QMessageBox::warning(this, tr("Error"),
                tr("Selected cue is not a group."));
        }
    }

    void MainWindow::onGo()
    {
        if (cueManager_->go()) {
            statusLabel_->setText(tr("GO"));
        }
    }

    void MainWindow::onStop()
    {
        cueManager_->stop();
        statusLabel_->setText(tr("Stopped"));
    }

    void MainWindow::onPause()
    {
        cueManager_->pause();
        statusLabel_->setText(tr("Paused"));
    }

    void MainWindow::onPanic()
    {
        cueManager_->panic();
        statusLabel_->setText(tr("PANIC STOP"));
    }

    void MainWindow::onPreferences()
    {
        QMessageBox::information(this, tr("Preferences"),
            tr("Preferences dialog not yet implemented"));
    }

    void MainWindow::onAbout()
    {
        QMessageBox::about(this, tr("About CueForge"),
            tr("<h2>CueForge Qt6</h2>"
                "<p><b>Version 2.0.0</b></p>"
                "<p>Professional show control software</p>"
                "<p>Built with Qt %1</p>"
                "<p>© 2025 CueForge Project</p>").arg(QT_VERSION_STR));
    }

    void MainWindow::onCueCountChanged(int count)
    {
        cueCountLabel_->setText(tr("Cues: %1").arg(count));
    }

    void MainWindow::onUnsavedChangesChanged(bool hasChanges)
    {
        Q_UNUSED(hasChanges);
        updateWindowTitle();
    }

    void MainWindow::onErrorOccurred(const QString& message)
    {
        statusLabel_->setText(message);
    }

    void MainWindow::loadSettings()
    {
        QSettings settings;
        restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
        restoreState(settings.value("mainWindow/state").toByteArray());
        currentFilePath_ = settings.value("mainWindow/lastFile").toString();
    }

    void MainWindow::saveSettings()
    {
        QSettings settings;
        settings.setValue("mainWindow/geometry", saveGeometry());
        settings.setValue("mainWindow/state", saveState());
        settings.setValue("mainWindow/lastFile", currentFilePath_);
    }

    bool MainWindow::maybeSave()
    {
        if (!cueManager_->hasUnsavedChanges()) {
            return true;
        }

        QMessageBox::StandardButton ret = QMessageBox::warning(
            this,
            tr("Unsaved Changes"),
            tr("The workspace has been modified.\nDo you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (ret == QMessageBox::Save) {
            onSaveWorkspace();
            return true;
        }
        else if (ret == QMessageBox::Cancel) {
            return false;
        }

        return true;
    }

    void MainWindow::updateWindowTitle()
    {
        QString title = "CueForge";

        if (!currentFilePath_.isEmpty()) {
            QFileInfo fileInfo(currentFilePath_);
            title = fileInfo.fileName() + " - CueForge";
        }

        if (cueManager_->hasUnsavedChanges()) {
            title.prepend("● ");
        }

        setWindowTitle(title);
    }

} // namespace CueForge