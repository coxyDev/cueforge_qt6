// ============================================================================
// CueListWidget.h - Main cue list view (UPDATED for tree view)
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QTreeView;
class QVBoxLayout;
QT_END_NAMESPACE

namespace CueForge {

    class CueManager;
    class CueTreeModel;

    class CueListWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit CueListWidget(CueManager* cueManager, QWidget* parent = nullptr);
        ~CueListWidget() override = default;

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private slots:
        void onSelectionChanged();
        void onDoubleClicked(const QModelIndex& index);
        void onContextMenuRequested(const QPoint& pos);
        void onHeaderContextMenuRequested(const QPoint& pos);

    private:
        void setupUI();
        void setupConnections();
        void applyStyleSheet();

        QPointer<CueManager> cueManager_;
        CueTreeModel* model_;
        QTreeView* treeView_;
        QVBoxLayout* layout_;
    };

} // namespace CueForge