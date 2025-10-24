// ============================================================================
// CueListWidget.cpp - Main cue list view (UPDATED)
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "CueListWidget.h"
#include "CueTreeModel.h"
#include "../core/CueManager.h"
#include "../core/Cue.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QHeaderView>
#include <QDebug>

namespace CueForge {

    CueListWidget::CueListWidget(CueManager* cueManager, QWidget* parent)
        : QWidget(parent)
        , cueManager_(cueManager)
        , model_(nullptr)
        , treeView_(nullptr)
        , layout_(nullptr)
    {
        setupUI();
        setupConnections();
        applyStyleSheet();
    }

    void CueListWidget::setupUI()
    {
        layout_ = new QVBoxLayout(this);
        layout_->setContentsMargins(0, 0, 0, 0);

        // Create model
        model_ = new CueTreeModel(cueManager_, this);

        // Create tree view
        treeView_ = new QTreeView(this);
        treeView_->setModel(model_);
        treeView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        treeView_->setDragEnabled(true);
        treeView_->setAcceptDrops(true);
        treeView_->setDropIndicatorShown(true);
        treeView_->setDragDropMode(QAbstractItemView::DragDrop);
        treeView_->setEditTriggers(QAbstractItemView::DoubleClicked |
            QAbstractItemView::EditKeyPressed);
        treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
        treeView_->setAlternatingRowColors(true);
        treeView_->setAnimated(true);
        treeView_->setIndentation(20);
        treeView_->setUniformRowHeights(true);
        treeView_->setItemsExpandable(true);
        treeView_->setExpandsOnDoubleClick(false);
        treeView_->setRootIsDecorated(true);

        // Configure header - ALL columns user-resizable
        QHeaderView* header = treeView_->header();
        header->setStretchLastSection(true);  // CHANGED: Last column stretches to fill
        header->setSectionsMovable(false);     // Don't allow reordering columns
        header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Make ALL columns fully interactive and resizable
        header->setSectionResizeMode(CueTreeModel::ColumnNumber, QHeaderView::Interactive);
        header->setSectionResizeMode(CueTreeModel::ColumnName, QHeaderView::Interactive);
        header->setSectionResizeMode(CueTreeModel::ColumnDuration, QHeaderView::Interactive);
        header->setSectionResizeMode(CueTreeModel::ColumnType, QHeaderView::Interactive);
        header->setSectionResizeMode(CueTreeModel::ColumnStatus, QHeaderView::Interactive);

        // Set default column widths (users can adjust ANY of these)
        treeView_->setColumnWidth(CueTreeModel::ColumnNumber, 80);
        treeView_->setColumnWidth(CueTreeModel::ColumnName, 250);   // Name gets more space by default
        treeView_->setColumnWidth(CueTreeModel::ColumnDuration, 100);
        treeView_->setColumnWidth(CueTreeModel::ColumnType, 100);
        // Status column stretches to fill remaining space (due to setStretchLastSection)

        // Set minimum widths to prevent columns from becoming too small
        header->setMinimumSectionSize(50);

        // Allow double-click on header divider to auto-resize column to contents
        header->setSectionsClickable(true);

        layout_->addWidget(treeView_);
    }

    void CueListWidget::setupConnections()
    {
        connect(treeView_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CueListWidget::onSelectionChanged);

        connect(treeView_, &QTreeView::doubleClicked,
            this, &CueListWidget::onDoubleClicked);

        connect(treeView_, &QTreeView::customContextMenuRequested,
            this, &CueListWidget::onContextMenuRequested);

        connect(treeView_->header(), &QHeaderView::customContextMenuRequested,
            this, &CueListWidget::onHeaderContextMenuRequested);
    }

    void CueListWidget::applyStyleSheet()
    {
        QString styleSheet = R"(
        QTreeView {
            background-color: #2b2b2b;
            color: #e0e0e0;
            border: none;
            outline: none;
            font-size: 10pt;
            gridline-color: #3c3c3c;
            show-decoration-selected: 1;
        }
        
        QTreeView::item {
            padding: 4px;
            border: none;
            height: 28px;
        }
        
        QTreeView::item:selected {
            background-color: #4a90e2;
            color: white;
        }
        
        QTreeView::item:hover {
            background-color: #3a3a3a;
        }
        
        QTreeView::item:alternate {
            background-color: #2e2e2e;
        }
        
        QTreeView::branch {
            background-color: #2b2b2b;
        }
        
        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            border-image: none;
            image: none;
        }
        
        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings {
            border-image: none;
            image: none;
        }
        
        QTreeView::branch:has-children:closed::before {
            content: "▶";
            color: #888;
            font-size: 10pt;
            padding-right: 4px;
        }
        
        QTreeView::branch:has-children:open::before {
            content: "▼";
            color: #888;
            font-size: 10pt;
            padding-right: 4px;
        }
        
        QHeaderView::section {
            background-color: #3c3c3c;
            color: #e0e0e0;
            padding: 8px 4px;
            border: none;
            border-right: 1px solid #2b2b2b;
            border-bottom: 2px solid #4a90e2;
            font-weight: bold;
            font-size: 10pt;
        }
        
        QHeaderView::section:hover {
            background-color: #4a4a4a;
        }
        
        QHeaderView::section:first {
            border-left: none;
        }
    )";

        setStyleSheet(styleSheet);
    }

    void CueListWidget::keyPressEvent(QKeyEvent* event)
    {
        if (!cueManager_) {
            QWidget::keyPressEvent(event);
            return;
        }

        switch (event->key()) {
        case Qt::Key_Space:
            cueManager_->go();
            event->accept();
            break;

        case Qt::Key_S:
            if (event->modifiers() == Qt::NoModifier) {
                cueManager_->stop();
                event->accept();
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter: {
            // Execute selected cue
            QModelIndexList selected = treeView_->selectionModel()->selectedIndexes();
            if (!selected.isEmpty()) {
                onDoubleClicked(selected.first());
            }
            event->accept();
            break;
        }

        case Qt::Key_Up:
            if (event->modifiers() == Qt::ControlModifier) {
                QModelIndexList selected = treeView_->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) {
                    QString cueId = model_->data(selected.first(),
                        CueTreeModel::CueIdRole).toString();
                    cueManager_->moveCueUp(cueId);
                }
                event->accept();
            }
            else {
                QWidget::keyPressEvent(event);
            }
            break;

        case Qt::Key_Down:
            if (event->modifiers() == Qt::ControlModifier) {
                QModelIndexList selected = treeView_->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) {
                    QString cueId = model_->data(selected.first(),
                        CueTreeModel::CueIdRole).toString();
                    cueManager_->moveCueDown(cueId);
                }
                event->accept();
            }
            else {
                QWidget::keyPressEvent(event);
            }
            break;

        case Qt::Key_Right:
            // Expand group
            if (event->modifiers() == Qt::NoModifier) {
                QModelIndexList selected = treeView_->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) {
                    treeView_->expand(selected.first());
                }
                event->accept();
            }
            break;

        case Qt::Key_Left:
            // Collapse group
            if (event->modifiers() == Qt::NoModifier) {
                QModelIndexList selected = treeView_->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) {
                    treeView_->collapse(selected.first());
                }
                event->accept();
            }
            break;

        default:
            QWidget::keyPressEvent(event);
            break;
        }
    }

    void CueListWidget::onSelectionChanged()
    {
        if (!cueManager_) return;

        QModelIndexList selected = treeView_->selectionModel()->selectedIndexes();

        cueManager_->clearSelection();
        for (const QModelIndex& index : selected) {
            QString cueId = model_->data(index, CueTreeModel::CueIdRole).toString();
            cueManager_->addToSelection(cueId);
        }
    }

    void CueListWidget::onDoubleClicked(const QModelIndex& index)
    {
        if (!index.isValid() || !cueManager_) return;

        QString cueId = model_->data(index, CueTreeModel::CueIdRole).toString();
        bool isGroup = model_->data(index, CueTreeModel::IsGroupRole).toBool();

        if (isGroup) {
            // Toggle expand/collapse for groups
            if (treeView_->isExpanded(index)) {
                treeView_->collapse(index);
            }
            else {
                treeView_->expand(index);
            }
        }
        else {
            // Execute non-group cues
            cueManager_->setStandByCue(cueId);
            cueManager_->go();
        }
    }

    void CueListWidget::onContextMenuRequested(const QPoint& pos)
    {
        QModelIndex index = treeView_->indexAt(pos);

        QMenu menu(this);
        menu.setStyleSheet(R"(
        QMenu {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border: 1px solid #555;
        }
        QMenu::item:selected {
            background-color: #4a90e2;
        }
    )");

        if (index.isValid()) {
            bool isGroup = model_->data(index, CueTreeModel::IsGroupRole).toBool();

            if (!isGroup) {
                menu.addAction("▶️ Execute", [this, index]() {
                    onDoubleClicked(index);
                    });

                menu.addAction("➡️ Set as Standby", [this, index]() {
                    QString cueId = model_->data(index, CueTreeModel::CueIdRole).toString();
                    cueManager_->setStandByCue(cueId);
                    });

                menu.addSeparator();
            }

            menu.addAction("📋 Duplicate", [this, index]() {
                QString cueId = model_->data(index, CueTreeModel::CueIdRole).toString();
                cueManager_->duplicateCue(cueId);
                });

            if (isGroup) {
                menu.addAction("📂 Ungroup", [this, index]() {
                    QString cueId = model_->data(index, CueTreeModel::CueIdRole).toString();
                    cueManager_->ungroupCue(cueId);
                    });
            }

            menu.addSeparator();
            menu.addAction("🗑️ Delete", [this, index]() {
                QString cueId = model_->data(index, CueTreeModel::CueIdRole).toString();
                cueManager_->removeCue(cueId);
                });

        }
        else {
            menu.addAction("🎵 New Audio Cue", [this]() {
                cueManager_->createCue(CueType::Audio);
                });

            menu.addAction("📁 New Group Cue", [this]() {
                cueManager_->createCue(CueType::Group);
                });

            menu.addAction("⏱️ New Wait Cue", [this]() {
                cueManager_->createCue(CueType::Wait);
                });

            menu.addAction("⚙️ New Control Cue", [this]() {
                cueManager_->createCue(CueType::Start);
                });
        }

        menu.exec(treeView_->mapToGlobal(pos));
    }

    void CueListWidget::onHeaderContextMenuRequested(const QPoint& pos)
    {
        QMenu menu(this);
        menu.setStyleSheet(R"(
        QMenu {
            background-color: #3c3c3c;
            color: #e0e0e0;
            border: 1px solid #555;
        }
        QMenu::item:selected {
            background-color: #4a90e2;
        }
    )");

        QAction* autoResize = menu.addAction("Auto-Resize All Columns");
        QAction* resetWidths = menu.addAction("Reset Column Widths");

        QAction* selected = menu.exec(treeView_->header()->mapToGlobal(pos));

        if (selected == autoResize) {
            // Auto-resize all columns to contents
            for (int i = 0; i < CueTreeModel::ColumnCount; ++i) {
                treeView_->resizeColumnToContents(i);
            }
        }
        else if (selected == resetWidths) {
            // Reset to default widths
            treeView_->setColumnWidth(CueTreeModel::ColumnNumber, 80);
            treeView_->setColumnWidth(CueTreeModel::ColumnName, 200);
            treeView_->setColumnWidth(CueTreeModel::ColumnDuration, 100);
            treeView_->setColumnWidth(CueTreeModel::ColumnType, 100);
            treeView_->setColumnWidth(CueTreeModel::ColumnStatus, 100);
        }
    }

} // namespace CueForge