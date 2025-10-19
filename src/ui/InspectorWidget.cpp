// ============================================================================
// InspectorWidget.cpp - Property inspector/editor implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "InspectorWidget.h"
#include "../core/CueManager.h"
#include "../core/Cue.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>

namespace CueForge {

    InspectorWidget::InspectorWidget(CueManager* cueManager, QWidget* parent)
        : QWidget(parent)
        , cueManager_(cueManager)
        , currentCue_(nullptr)
        , updatingFromCue_(false)
    {
        setupUI();
        setupConnections();
        applyStyleSheet();
        clearInspector();
    }

    void InspectorWidget::setupUI()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);

        // Basic Properties Group
        basicGroup_ = new QGroupBox(tr("Basic Properties"), this);
        mainLayout_ = new QFormLayout(basicGroup_);

        editNumber_ = new QLineEdit(this);
        editNumber_->setPlaceholderText(tr("1"));
        mainLayout_->addRow(tr("Number:"), editNumber_);

        editName_ = new QLineEdit(this);
        editName_->setPlaceholderText(tr("Cue Name"));
        mainLayout_->addRow(tr("Name:"), editName_);

        spinDuration_ = new QDoubleSpinBox(this);
        spinDuration_->setRange(0.0, 86400.0);
        spinDuration_->setDecimals(3);
        spinDuration_->setSuffix(" s");
        mainLayout_->addRow(tr("Duration:"), spinDuration_);

        spinPreWait_ = new QDoubleSpinBox(this);
        spinPreWait_->setRange(0.0, 3600.0);
        spinPreWait_->setDecimals(3);
        spinPreWait_->setSuffix(" s");
        mainLayout_->addRow(tr("Pre-Wait:"), spinPreWait_);

        spinPostWait_ = new QDoubleSpinBox(this);
        spinPostWait_->setRange(0.0, 3600.0);
        spinPostWait_->setDecimals(3);
        spinPostWait_->setSuffix(" s");
        mainLayout_->addRow(tr("Post-Wait:"), spinPostWait_);

        checkContinue_ = new QCheckBox(tr("Auto-continue to next cue"), this);
        mainLayout_->addRow(tr("Continue:"), checkContinue_);

        btnColor_ = new QPushButton(tr("Choose Color..."), this);
        connect(btnColor_, &QPushButton::clicked, this, [this]() {
            if (!currentCue_) return;
            QColor color = QColorDialog::getColor(currentCue_->color(), this);
            if (color.isValid()) {
                currentCue_->setColor(color);
                updateFromCue(currentCue_);
            }
            });
        mainLayout_->addRow(tr("Color:"), btnColor_);

        editNotes_ = new QTextEdit(this);
        editNotes_->setMaximumHeight(100);
        editNotes_->setPlaceholderText(tr("Notes..."));
        mainLayout_->addRow(tr("Notes:"), editNotes_);

        layout->addWidget(basicGroup_);

        // Status Group
        statusGroup_ = new QGroupBox(tr("Status"), this);
        QFormLayout* statusLayout = new QFormLayout(statusGroup_);

        labelType_ = new QLabel(tr("None"), this);
        statusLayout->addRow(tr("Type:"), labelType_);

        labelStatus_ = new QLabel(tr("None"), this);
        statusLayout->addRow(tr("Status:"), labelStatus_);

        labelId_ = new QLabel(tr("None"), this);
        labelId_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        statusLayout->addRow(tr("ID:"), labelId_);

        checkArmed_ = new QCheckBox(tr("Cue is armed"), this);
        statusLayout->addRow(tr("Armed:"), checkArmed_);

        layout->addWidget(statusGroup_);

        layout->addStretch();
    }

    void InspectorWidget::setupConnections()
    {
        if (!cueManager_) return;

        connect(cueManager_, &CueManager::selectionChanged,
            this, &InspectorWidget::onSelectionChanged);
        connect(cueManager_, &CueManager::cueUpdated,
            this, &InspectorWidget::onCueUpdated);

        connect(editNumber_, &QLineEdit::editingFinished,
            this, &InspectorWidget::applyChangesToCue);
        connect(editName_, &QLineEdit::editingFinished,
            this, &InspectorWidget::applyChangesToCue);
        connect(spinDuration_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &InspectorWidget::applyChangesToCue);
        connect(spinPreWait_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &InspectorWidget::applyChangesToCue);
        connect(spinPostWait_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &InspectorWidget::applyChangesToCue);
        connect(checkContinue_, &QCheckBox::toggled,
            this, &InspectorWidget::applyChangesToCue);
        connect(editNotes_, &QTextEdit::textChanged,
            this, &InspectorWidget::applyChangesToCue);
        connect(checkArmed_, &QCheckBox::toggled,
            this, &InspectorWidget::applyChangesToCue);
    }

    void InspectorWidget::applyStyleSheet()
    {
        QString styleSheet = R"(
        QGroupBox {
            background-color: #2b2b2b;
            border: 2px solid #555;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: bold;
            color: #4a90e2;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        
        QLineEdit, QDoubleSpinBox, QSpinBox {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 6px;
            selection-background-color: #4a90e2;
        }
        
        QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
            border: 2px solid #4a90e2;
        }
        
        QTextEdit {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 6px;
            selection-background-color: #4a90e2;
        }
        
        QTextEdit:focus {
            border: 2px solid #4a90e2;
        }
        
        QCheckBox {
            color: #e0e0e0;
            spacing: 8px;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #555;
            border-radius: 3px;
            background-color: #3c3c3c;
        }
        
        QCheckBox::indicator:checked {
            background-color: #4a90e2;
            border-color: #4a90e2;
        }
        
        QCheckBox::indicator:hover {
            border-color: #4a90e2;
        }
        
        QPushButton {
            background-color: #4c4c4c;
            color: #e0e0e0;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 6px 12px;
        }
        
        QPushButton:hover {
            background-color: #5a5a5a;
            border-color: #4a90e2;
        }
        
        QPushButton:pressed {
            background-color: #3a3a3a;
        }
        
        QLabel {
            color: #e0e0e0;
        }
    )";

        setStyleSheet(styleSheet);
    }

    void InspectorWidget::onSelectionChanged()
    {
        if (!cueManager_) return;

        QStringList selected = cueManager_->selectedCueIds();

        if (selected.isEmpty()) {
            clearInspector();
        }
        else if (selected.size() == 1) {
            Cue* cue = cueManager_->getCue(selected.first());
            if (cue) {
                showSingleCue(cue);
            }
            else {
                clearInspector();
            }
        }
        else {
            showMultipleSelection(selected.size());
        }
    }

    void InspectorWidget::onCueUpdated(Cue* cue)
    {
        // Safety check: make sure the cue pointer is still valid
        if (!cue) {
            clearInspector();
            return;
        }

        // Only update if this is the cue we're currently showing
        if (cue == currentCue_.data()) {
            updateFromCue(cue);
        }
    }

    void InspectorWidget::updateFromCue(Cue* cue)
    {
        if (!cue || updatingFromCue_) return;

        updatingFromCue_ = true;

        editNumber_->setText(cue->number());
        editName_->setText(cue->name());
        spinDuration_->setValue(cue->duration());
        spinPreWait_->setValue(cue->preWait());
        spinPostWait_->setValue(cue->postWait());
        checkContinue_->setChecked(cue->continueMode());
        editNotes_->setPlainText(cue->notes());

        QColor color = cue->color();
        btnColor_->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; }")
            .arg(color.name())
            .arg(color.lightness() > 128 ? "#000" : "#fff"));

        labelType_->setText(cueTypeToString(cue->type()));
        labelStatus_->setText(cueStatusToString(cue->status()));
        labelId_->setText(cue->id());
        checkArmed_->setChecked(cue->isArmed());

        updatingFromCue_ = false;
    }

    void InspectorWidget::applyChangesToCue()
    {
        // Safety check: make sure currentCue_ is still valid
        if (!currentCue_ || updatingFromCue_) return;

        // Double-check the pointer is still valid
        if (currentCue_.isNull()) {
            clearInspector();
            return;
        }

        currentCue_->setNumber(editNumber_->text());
        currentCue_->setName(editName_->text());
        currentCue_->setDuration(spinDuration_->value());
        currentCue_->setPreWait(spinPreWait_->value());
        currentCue_->setPostWait(spinPostWait_->value());
        currentCue_->setContinueMode(checkContinue_->isChecked());
        currentCue_->setNotes(editNotes_->toPlainText());
        currentCue_->setArmed(checkArmed_->isChecked());
    }

    void InspectorWidget::clearInspector()
    {
        currentCue_ = nullptr;

        basicGroup_->setTitle(tr("Basic Properties"));
        basicGroup_->setEnabled(false);
        statusGroup_->setEnabled(false);

        editNumber_->clear();
        editName_->clear();
        spinDuration_->setValue(0.0);
        spinPreWait_->setValue(0.0);
        spinPostWait_->setValue(0.0);
        checkContinue_->setChecked(false);
        editNotes_->clear();

        labelType_->setText(tr("None"));
        labelStatus_->setText(tr("None"));
        labelId_->setText(tr("None"));
        checkArmed_->setChecked(false);

        btnColor_->setStyleSheet("");
    }

    void InspectorWidget::showMultipleSelection(int count)
    {
        currentCue_ = nullptr;

        basicGroup_->setTitle(tr("Multiple Selection (%1 cues)").arg(count));
        basicGroup_->setEnabled(false);
        statusGroup_->setEnabled(false);

        editNumber_->setText(tr("(Multiple)"));
        editName_->setText(tr("(Multiple)"));
    }

    void InspectorWidget::showSingleCue(Cue* cue)
    {
        if (!cue) {
            clearInspector();
            return;
        }

        currentCue_ = cue;

        basicGroup_->setTitle(tr("Basic Properties - %1").arg(cue->number()));
        basicGroup_->setEnabled(true);
        statusGroup_->setEnabled(true);

        updateFromCue(cue);
    }

} // namespace CueForge