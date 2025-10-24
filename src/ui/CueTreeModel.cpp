// ============================================================================
// CueTreeModel.cpp - Hierarchical tree model implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "CueTreeModel.h"
#include "../core/CueManager.h"
#include "../core/Cue.h"
#include "../core/cues/GroupCue.h"
#include <QMimeData>
#include <QFont>
#include <QBrush>
#include <QDebug>

namespace CueForge {

    CueTreeModel::CueTreeModel(CueManager* cueManager, QObject* parent)
        : QAbstractItemModel(parent)
        , cueManager_(cueManager)
    {
        if (cueManager_) {
            connect(cueManager_, &CueManager::cueAdded, this, &CueTreeModel::onCueAdded);
            connect(cueManager_, &CueManager::cueRemoved, this, &CueTreeModel::onCueRemoved);
            connect(cueManager_, &CueManager::cueMoved, this, &CueTreeModel::onCueMoved);
            connect(cueManager_, &CueManager::cueUpdated, this, &CueTreeModel::onCueUpdated);
            connect(cueManager_, &CueManager::workspaceCleared, this, &CueTreeModel::onWorkspaceCleared);
        }
    }

    QModelIndex CueTreeModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent) || !cueManager_) {
            return QModelIndex();
        }

        if (!parent.isValid()) {
            // Top level cue
            if (row >= 0 && row < cueManager_->cueCount()) {
                Cue* cue = cueManager_->allCues().at(row).get();
                return createIndex(row, column, cue);
            }
        }
        else {
            // Child of a group
            Cue* parentCue = static_cast<Cue*>(parent.internalPointer());
            if (parentCue && parentCue->type() == CueType::Group) {
                GroupCue* group = static_cast<GroupCue*>(parentCue);
                if (row >= 0 && row < group->childCount()) {
                    Cue* childCue = group->getChildAt(row);
                    return createIndex(row, column, childCue);
                }
            }
        }

        return QModelIndex();
    }

    QModelIndex CueTreeModel::parent(const QModelIndex& child) const
    {
        if (!child.isValid() || !cueManager_) {
            return QModelIndex();
        }

        Cue* childCue = static_cast<Cue*>(child.internalPointer());
        if (!childCue) {
            return QModelIndex();
        }

        // Check if this cue is a child of a group
        QObject* parentObj = childCue->parent();
        if (GroupCue* parentGroup = qobject_cast<GroupCue*>(parentObj)) {
            // Find the row of the parent group in the top-level list
            int row = cueManager_->getCueIndex(parentGroup->id());
            if (row >= 0) {
                return createIndex(row, 0, parentGroup);
            }
        }

        return QModelIndex();
    }

    int CueTreeModel::rowCount(const QModelIndex& parent) const
    {
        if (!cueManager_) {
            return 0;
        }

        if (!parent.isValid()) {
            // Top level
            return cueManager_->cueCount();
        }

        // Child count of a group
        Cue* parentCue = static_cast<Cue*>(parent.internalPointer());
        if (parentCue && parentCue->type() == CueType::Group) {
            GroupCue* group = static_cast<GroupCue*>(parentCue);
            return group->childCount();
        }

        return 0;
    }

    int CueTreeModel::columnCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return ColumnCount;
    }

    QVariant CueTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || !cueManager_) {
            return QVariant();
        }

        Cue* cue = static_cast<Cue*>(index.internalPointer());
        if (!cue) {
            return QVariant();
        }

        int column = index.column();

        switch (role) {
        case Qt::DisplayRole:
            switch (column) {
            case ColumnNumber:
                return cue->number();
            case ColumnName:
                return cue->name();
            case ColumnDuration:
                return QString("%1s").arg(cue->duration(), 0, 'f', 1);
            case ColumnType:
                return cueTypeToString(cue->type());
            case ColumnStatus:
                return cueStatusToString(cue->status());
            default:
                return QVariant();
            }

        case NumberRole:
            return cue->number();

        case NameRole:
            return cue->name();

        case DurationRole:
            return cue->duration();

        case StatusRole:
            return static_cast<int>(cue->status());

        case ColorRole:
            return cue->color();

        case TypeRole:
            return cueTypeToString(cue->type());

        case CueIdRole:
            return cue->id();

        case CuePointerRole:
            return QVariant::fromValue(cue);

        case IsGroupRole:
            return cue->type() == CueType::Group;

        case IsStandbyRole:
            return cueManager_->standByCue() == cue;

        case IsBrokenRole:
            return cue->isBroken();

        case Qt::BackgroundRole: {
            // Only apply to first column for tree structure visibility
            if (column != ColumnNumber) {
                return QVariant();
            }

            // Standby cue gets special highlight
            if (cueManager_->standByCue() == cue) {
                return QBrush(QColor(70, 130, 180, 80)); // Steel blue, semi-transparent
            }

            // Running cues get green
            if (cue->status() == CueStatus::Running) {
                return QBrush(QColor(60, 179, 113, 80)); // Medium sea green
            }

            // Paused cues get yellow
            if (cue->status() == CueStatus::Paused) {
                return QBrush(QColor(255, 215, 0, 80)); // Gold
            }

            return QVariant();
        }

        case Qt::ForegroundRole: {
            if (cue->isBroken()) {
                return QBrush(QColor(255, 100, 100)); // Light red for broken
            }
            if (!cue->isArmed()) {
                return QBrush(QColor(150, 150, 150)); // Gray for disarmed
            }
            return QBrush(QColor(240, 240, 240)); // Light gray text
        }

        case Qt::FontRole: {
            QFont font;
            if (cueManager_->standByCue() == cue) {
                font.setBold(true);
            }
            if (cue->type() == CueType::Group) {
                font.setBold(true);
            }
            return font;
        }

        case Qt::DecorationRole: {
            // Only show icons in first column
            if (column != ColumnNumber) {
                return QVariant();
            }

            // Return emoji/icon based on cue type
            switch (cue->type()) {
            case CueType::Audio: return QString("🎵");
            case CueType::Video: return QString("🎬");
            case CueType::Group: return QString("📁");
            case CueType::Wait: return QString("⏱️");
            case CueType::Start: return QString("▶️");
            case CueType::Stop: return QString("⏹️");
            case CueType::Goto: return QString("➡️");
            case CueType::Pause: return QString("⏸️");
            default: return QString("⚙️");
            }
        }

        case Qt::ToolTipRole: {
            QString tip = QString("<b>%1: %2</b><br>").arg(cue->number(), cue->name());
            tip += QString("Type: %1<br>").arg(cueTypeToString(cue->type()));
            tip += QString("Duration: %1s<br>").arg(cue->duration());
            tip += QString("Status: %1").arg(cueStatusToString(cue->status()));
            if (!cue->notes().isEmpty()) {
                tip += QString("<br><i>%1</i>").arg(cue->notes());
            }
            return tip;
        }

        default:
            return QVariant();
        }
    }

    QVariant CueTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal) {
            return QVariant();
        }

        if (role == Qt::DisplayRole) {
            switch (section) {
            case ColumnNumber:   return tr("Cue");
            case ColumnName:     return tr("Name");
            case ColumnDuration: return tr("Duration");
            case ColumnType:     return tr("Type");
            case ColumnStatus:   return tr("Status");
            default:             return QVariant();
            }
        }

        if (role == Qt::TextAlignmentRole) {
            switch (section) {
            case ColumnNumber:
            case ColumnDuration:
                return static_cast<int>(Qt::AlignCenter);
            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
            }
        }

        return QVariant();
    }

    Qt::ItemFlags CueTreeModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

        if (index.isValid()) {
            return defaultFlags | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }

        return defaultFlags | Qt::ItemIsDropEnabled;
    }

    bool CueTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (!index.isValid() || !cueManager_) {
            return false;
        }

        Cue* cue = static_cast<Cue*>(index.internalPointer());
        if (!cue) {
            return false;
        }

        switch (role) {
        case NameRole:
        case Qt::EditRole:
            cue->setName(value.toString());
            emit dataChanged(index, index, { role });
            return true;

        case NumberRole:
            cue->setNumber(value.toString());
            emit dataChanged(index, index, { role });
            return true;

        default:
            return false;
        }
    }

    // ============================================================================
    // Drag and Drop
    // ============================================================================

    Qt::DropActions CueTreeModel::supportedDropActions() const
    {
        return Qt::MoveAction | Qt::CopyAction;
    }

    QStringList CueTreeModel::mimeTypes() const
    {
        return QStringList() << "application/x-cueforge-cue-id";
    }

    bool CueTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
        int row, int column, const QModelIndex& parent) const
    {
        Q_UNUSED(row);
        Q_UNUSED(column);
        Q_UNUSED(action);

        if (!data->hasFormat("application/x-cueforge-cue-id")) {
            return false;
        }

        // Can drop into groups or at top level
        if (parent.isValid()) {
            Cue* parentCue = static_cast<Cue*>(parent.internalPointer());
            return parentCue && parentCue->type() == CueType::Group;
        }

        return true;
    }

    QMimeData* CueTreeModel::mimeData(const QModelIndexList& indexes) const
    {
        if (indexes.isEmpty()) {
            return nullptr;
        }

        QMimeData* mimeData = new QMimeData();
        QStringList cueIds;

        for (const QModelIndex& index : indexes) {
            if (index.isValid()) {
                QString cueId = data(index, CueIdRole).toString();
                cueIds.append(cueId);
            }
        }

        mimeData->setData("application/x-cueforge-cue-id", cueIds.join(",").toUtf8());
        return mimeData;
    }

    bool CueTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
        int row, int column, const QModelIndex& parent)
    {
        Q_UNUSED(column);

        if (action == Qt::IgnoreAction) {
            return true;
        }

        if (!data->hasFormat("application/x-cueforge-cue-id")) {
            return false;
        }

        QString cueIdsData = QString::fromUtf8(data->data("application/x-cueforge-cue-id"));
        QStringList cueIds = cueIdsData.split(",");

        if (cueIds.isEmpty()) {
            return false;
        }

        if (parent.isValid()) {
            // Dropping into a group
            Cue* parentCue = static_cast<Cue*>(parent.internalPointer());
            if (!parentCue || parentCue->type() != CueType::Group) {
                return false;
            }

            GroupCue* group = static_cast<GroupCue*>(parentCue);

            // Add each cue to the group
            for (const QString& cueId : cueIds) {
                Cue* cue = cueManager_->getCue(cueId);
                if (!cue || cue == parentCue) {
                    continue; // Skip if invalid or trying to add group to itself
                }

                // Get the index before removing
                int oldIndex = cueManager_->getCueIndex(cueId);
                if (oldIndex < 0) {
                    continue;
                }

                // Get the shared_ptr before removing
                Cue::CuePtr cuePtr = cueManager_->allCues()[oldIndex];

                // Remove from main list
                beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
                cueManager_->removeCueWithoutSignals(cueId); // We need to add this method
                endRemoveRows();

                // Add to group
                int childRow = group->childCount();
                QModelIndex groupIndex = indexForCue(group->id());
                beginInsertRows(groupIndex, childRow, childRow);
                group->addChild(cuePtr);
                endInsertRows();
            }

            cueManager_->markUnsaved();
            return true;

        }
        else {
            // Dropping at top level - reorder
            int dropRow = row < 0 ? rowCount() : row;

            for (const QString& cueId : cueIds) {
                cueManager_->moveCue(cueId, dropRow);
                dropRow++;
            }
            return true;
        }
    }

    // ============================================================================
    // Helper Methods
    // ============================================================================

    Cue* CueTreeModel::getCue(const QModelIndex& index) const
    {
        if (!index.isValid()) {
            return nullptr;
        }
        return static_cast<Cue*>(index.internalPointer());
    }

    QModelIndex CueTreeModel::indexForCue(const QString& cueId) const
    {
        if (!cueManager_) {
            return QModelIndex();
        }

        // Search top level
        for (int i = 0; i < cueManager_->cueCount(); ++i) {
            Cue* cue = cueManager_->allCues().at(i).get();
            if (cue->id() == cueId) {
                return createIndex(i, 0, cue);
            }

            // Search children if it's a group
            if (cue->type() == CueType::Group) {
                GroupCue* group = static_cast<GroupCue*>(cue);
                for (int j = 0; j < group->childCount(); ++j) {
                    Cue* child = group->getChildAt(j);
                    if (child && child->id() == cueId) {
                        return createIndex(j, 0, child);
                    }
                }
            }
        }

        return QModelIndex();
    }

    // ============================================================================
    // Slots
    // ============================================================================

    void CueTreeModel::onCueAdded(Cue* cue, int index)
    {
        Q_UNUSED(cue);
        beginInsertRows(QModelIndex(), index, index);
        endInsertRows();
    }

    void CueTreeModel::onCueRemoved(const QString& cueId)
    {
        int idx = cueManager_->getCueIndex(cueId);
        if (idx >= 0) {
            beginRemoveRows(QModelIndex(), idx, idx);
            endRemoveRows();
        }
    }

    void CueTreeModel::onCueMoved(const QString& cueId, int oldIndex, int newIndex)
    {
        Q_UNUSED(cueId);

        if (!beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(),
            newIndex > oldIndex ? newIndex + 1 : newIndex)) {
            qWarning() << "Failed to move rows in model";
            return;
        }

        endMoveRows();
    }

    void CueTreeModel::onCueUpdated(Cue* cue)
    {
        if (!cue) return;

        QModelIndex idx = indexForCue(cue->id());
        if (idx.isValid()) {
            emit dataChanged(idx, idx);
        }
    }

    void CueTreeModel::onWorkspaceCleared()
    {
        beginResetModel();
        endResetModel();
    }

} // namespace CueForge