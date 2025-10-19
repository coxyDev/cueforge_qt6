// ============================================================================
// CueTreeModel.h - Hierarchical tree model for cues with group support
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QAbstractItemModel>
#include <QPointer>

namespace CueForge {

    class CueManager;
    class Cue;

    class CueTreeModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        enum CueRoles {
            NumberRole = Qt::UserRole + 1,
            NameRole,
            DurationRole,
            StatusRole,
            ColorRole,
            TypeRole,
            CueIdRole,
            CuePointerRole,
            IsGroupRole,
            IsStandbyRole,
            IsBrokenRole
        };

        explicit CueTreeModel(CueManager* cueManager, QObject* parent = nullptr);
        ~CueTreeModel() override = default;

        // QAbstractItemModel interface
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        // Drag and drop
        Qt::DropActions supportedDropActions() const override;
        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
            int row, int column, const QModelIndex& parent) const override;
        bool dropMimeData(const QMimeData* data, Qt::DropAction action,
            int row, int column, const QModelIndex& parent) override;

        // Custom methods
        Cue* getCue(const QModelIndex& index) const;
        QModelIndex indexForCue(const QString& cueId) const;

    private slots:
        void onCueAdded(Cue* cue, int index);
        void onCueRemoved(const QString& cueId);
        void onCueMoved(const QString& cueId, int oldIndex, int newIndex);
        void onCueUpdated(Cue* cue);
        void onWorkspaceCleared();

    private:
        Cue* getCueForIndex(const QModelIndex& index) const;
        QModelIndex createIndexForCue(Cue* cue, int row) const;

        QPointer<CueManager> cueManager_;
    };

} // namespace CueForge