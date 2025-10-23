// ============================================================================
// InspectorWidget.h - Property inspector/editor panel
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QTextEdit;
class QPushButton;
class QLabel;
class QGroupBox;
QT_END_NAMESPACE

namespace CueForge {

    class CueManager;
    class Cue;

    class InspectorWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit InspectorWidget(CueManager* cueManager, QWidget* parent = nullptr);
        ~InspectorWidget() override = default;

    private slots:
        void onSelectionChanged();
        void onCueUpdated(Cue* cue);
        void updateFromCue(Cue* cue);
        void applyChangesToCue();

    private:
        void setupUI();
        void setupConnections();
        void applyStyleSheet();
        void clearInspector();
        void showMultipleSelection(int count);
        void showSingleCue(Cue* cue);
        void setupAudioCueWidgets();

        QPointer<CueManager> cueManager_;
        QPointer<Cue> currentCue_;

        // UI Elements
        QGroupBox* basicGroup_;
        QLineEdit* editNumber_;
        QLineEdit* editName_;
        QDoubleSpinBox* spinDuration_;
        QDoubleSpinBox* spinPreWait_;
        QDoubleSpinBox* spinPostWait_;
        QCheckBox* checkContinue_;
        QPushButton* btnColor_;
        QTextEdit* editNotes_;

        QGroupBox* audioCueGroup_;
        QLineEdit* editFilePath_;
        QPushButton* btnBrowseFile_;
        QDoubleSpinBox* spinVolume_;

        QGroupBox* statusGroup_;
        QLabel* labelType_;
        QLabel* labelStatus_;
        QLabel* labelId_;
        QCheckBox* checkArmed_;

        QFormLayout* mainLayout_;

        bool updatingFromCue_;
    };

} // namespace CueForge