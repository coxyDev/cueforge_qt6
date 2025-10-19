// ============================================================================
// TransportWidget.h - Playback transport controls
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLabel;
class QHBoxLayout;
QT_END_NAMESPACE

namespace CueForge {

    class CueManager;

    class TransportWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit TransportWidget(CueManager* cueManager, QWidget* parent = nullptr);
        ~TransportWidget() override = default;

    private slots:
        void onStandByCueChanged(const QString& cueId);
        void onPlaybackStateChanged();

    private:
        void setupUI();
        void setupConnections();
        void updateStandbyDisplay();

        QPointer<CueManager> cueManager_;

        QPushButton* btnPrevious_;
        QPushButton* btnGo_;
        QPushButton* btnStop_;
        QPushButton* btnPause_;
        QPushButton* btnNext_;
        QPushButton* btnPanic_;

        QLabel* labelStandby_;
        QLabel* labelStatus_;

        QHBoxLayout* layout_;
    };

} // namespace CueForge