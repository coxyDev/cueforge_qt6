// ============================================================================
// TransportWidget.cpp - Playback transport controls implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "TransportWidget.h"
#include "../core/CueManager.h"
#include "../core/Cue.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QFont>

namespace CueForge {

    TransportWidget::TransportWidget(CueManager* cueManager, QWidget* parent)
        : QWidget(parent)
        , cueManager_(cueManager)
    {
        setupUI();
        setupConnections();
        updateStandbyDisplay();
    }

    void TransportWidget::setupUI()
    {
        layout_ = new QHBoxLayout(this);
        layout_->setSpacing(8);
        layout_->setContentsMargins(12, 8, 12, 8);

        // Style for all buttons
        QString buttonStyle = R"(
        QPushButton {
            background-color: #4c4c4c;
            color: #e0e0e0;
            border: 2px solid #555;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 11pt;
        }
        QPushButton:hover {
            background-color: #5a5a5a;
            border-color: #4a90e2;
        }
        QPushButton:pressed {
            background-color: #3a3a3a;
        }
    )";

        // Previous button
        btnPrevious_ = new QPushButton("◀◀", this);
        btnPrevious_->setToolTip(tr("Previous Cue (Up Arrow)"));
        btnPrevious_->setMinimumSize(70, 50);
        btnPrevious_->setStyleSheet(buttonStyle);
        layout_->addWidget(btnPrevious_);

        // Go button (large, prominent green)
        btnGo_ = new QPushButton("▶ GO", this);
        btnGo_->setToolTip(tr("Execute Standby Cue (Space)"));
        btnGo_->setMinimumSize(140, 50);
        QFont goFont = btnGo_->font();
        goFont.setPointSize(16);
        goFont.setBold(true);
        btnGo_->setFont(goFont);
        btnGo_->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: 3px solid #45a049;
            border-radius: 8px;
            padding: 12px 24px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45a049;
            border-color: #3d8b40;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
    )");
        layout_->addWidget(btnGo_);

        // Stop button (red)
        btnStop_ = new QPushButton("■", this);
        btnStop_->setToolTip(tr("Stop All (S)"));
        btnStop_->setMinimumSize(70, 50);
        btnStop_->setStyleSheet(R"(
        QPushButton {
            background-color: #f44336;
            color: white;
            border: 2px solid #da190b;
            border-radius: 6px;
            font-size: 18pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #da190b;
        }
        QPushButton:pressed {
            background-color: #c41104;
        }
    )");
        layout_->addWidget(btnStop_);

        // Pause button (yellow)
        btnPause_ = new QPushButton("⏸", this);
        btnPause_->setToolTip(tr("Pause/Resume (P)"));
        btnPause_->setMinimumSize(70, 50);
        btnPause_->setStyleSheet(R"(
        QPushButton {
            background-color: #FFC107;
            color: #2b2b2b;
            border: 2px solid #FFA000;
            border-radius: 6px;
            font-size: 18pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #FFA000;
        }
        QPushButton:pressed {
            background-color: #FF8F00;
        }
    )");
        layout_->addWidget(btnPause_);

        // Next button
        btnNext_ = new QPushButton("▶▶", this);
        btnNext_->setToolTip(tr("Next Cue (Down Arrow)"));
        btnNext_->setMinimumSize(70, 50);
        btnNext_->setStyleSheet(buttonStyle);
        layout_->addWidget(btnNext_);

        layout_->addSpacing(30);

        // Standby display (large, prominent)
        labelStandby_ = new QLabel(tr("Standby: None"), this);
        QFont standbyFont;
        standbyFont.setPointSize(13);
        standbyFont.setBold(true);
        labelStandby_->setFont(standbyFont);
        labelStandby_->setStyleSheet(R"(
        QLabel {
            color: #4a90e2;
            padding: 8px 16px;
            background-color: #2b2b2b;
            border: 2px solid #4a90e2;
            border-radius: 6px;
        }
    )");
        labelStandby_->setMinimumWidth(300);
        layout_->addWidget(labelStandby_);

        layout_->addStretch();

        // Status display
        labelStatus_ = new QLabel(tr("Ready"), this);
        QFont statusFont;
        statusFont.setPointSize(11);
        statusFont.setBold(true);
        labelStatus_->setFont(statusFont);
        labelStatus_->setStyleSheet(R"(
        QLabel {
            color: #e0e0e0;
            padding: 8px 16px;
            background-color: #3c3c3c;
            border: 1px solid #555;
            border-radius: 4px;
        }
    )");
        layout_->addWidget(labelStatus_);

        layout_->addSpacing(30);

        // Panic button (dark red, far right)
        btnPanic_ = new QPushButton("🛑 PANIC", this);
        btnPanic_->setToolTip(tr("Emergency Stop All (Ctrl+Esc)"));
        btnPanic_->setMinimumSize(100, 50);
        QFont panicFont;
        panicFont.setPointSize(12);
        panicFont.setBold(true);
        btnPanic_->setFont(panicFont);
        btnPanic_->setStyleSheet(R"(
        QPushButton {
            background-color: #8B0000;
            color: white;
            border: 3px solid #660000;
            border-radius: 6px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #660000;
            border-color: #4d0000;
        }
        QPushButton:pressed {
            background-color: #4d0000;
        }
    )");
        layout_->addWidget(btnPanic_);

        // Set background for entire widget
        setStyleSheet(R"(
        CueForge--TransportWidget {
            background-color: #2b2b2b;
            border-top: 2px solid #555;
        }
    )");
    }

    void TransportWidget::setupConnections()
    {
        if (!cueManager_) return;

        connect(btnPrevious_, &QPushButton::clicked, cueManager_, &CueManager::previousCue);
        connect(btnGo_, &QPushButton::clicked, cueManager_, &CueManager::go);
        connect(btnStop_, &QPushButton::clicked, cueManager_, &CueManager::stop);
        connect(btnPause_, &QPushButton::clicked, cueManager_, &CueManager::pause);
        connect(btnNext_, &QPushButton::clicked, cueManager_, &CueManager::nextCue);
        connect(btnPanic_, &QPushButton::clicked, cueManager_, &CueManager::panic);

        connect(cueManager_, &CueManager::standByCueChanged,
            this, &TransportWidget::onStandByCueChanged);
        connect(cueManager_, &CueManager::playbackStateChanged,
            this, &TransportWidget::onPlaybackStateChanged);
    }

    void TransportWidget::onStandByCueChanged(const QString& cueId)
    {
        Q_UNUSED(cueId);
        updateStandbyDisplay();
    }

    void TransportWidget::onPlaybackStateChanged()
    {
        int activeCount = cueManager_->activeCueIds().size();

        if (activeCount > 0) {
            labelStatus_->setText(tr("Playing (%1)").arg(activeCount));
            labelStatus_->setStyleSheet(R"(
            QLabel {
                color: white;
                padding: 8px 16px;
                background-color: #4CAF50;
                border: 1px solid #45a049;
                border-radius: 4px;
                font-weight: bold;
            }
        )");
        }
        else {
            labelStatus_->setText(tr("Ready"));
            labelStatus_->setStyleSheet(R"(
            QLabel {
                color: #e0e0e0;
                padding: 8px 16px;
                background-color: #3c3c3c;
                border: 1px solid #555;
                border-radius: 4px;
            }
        )");
        }
    }

    void TransportWidget::updateStandbyDisplay()
    {
        if (!cueManager_) return;

        Cue* standbyCue = cueManager_->standByCue();

        if (standbyCue) {
            labelStandby_->setText(tr("Standby: %1 - %2")
                .arg(standbyCue->number(), standbyCue->name()));
            labelStandby_->setStyleSheet(R"(
            QLabel {
                color: #4a90e2;
                padding: 8px 16px;
                background-color: #1a1a2e;
                border: 2px solid #4a90e2;
                border-radius: 6px;
                font-weight: bold;
            }
        )");
        }
        else {
            labelStandby_->setText(tr("Standby: None"));
            labelStandby_->setStyleSheet(R"(
            QLabel {
                color: #888;
                padding: 8px 16px;
                background-color: #2b2b2b;
                border: 2px solid #555;
                border-radius: 6px;
            }
        )");
        }
    }

} // namespace CueForge