// ============================================================================
// GroupCue.h - Container cue for organizing other cues
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include "../Cue.h"
#include <QList>

namespace CueForge {

    enum class GroupMode {
        Sequential,    // Execute children one after another
        Simultaneous   // Execute all children at once
    };

    class GroupCue : public Cue
    {
        Q_OBJECT

    public:
        explicit GroupCue(QObject* parent = nullptr);
        ~GroupCue() override = default;

        // Cue interface - match base class signatures
        bool execute() override;           // CHANGED: returns bool
        void stop() override;
        void pause() override;
        void resume() override;
        QJsonObject toJson() const override;
        void fromJson(const QJsonObject& json) override;

        // Group-specific methods
        void addChild(CuePtr child);
        CuePtr removeChild(int index);
        Cue* getChildAt(int index) const;
        int childCount() const { return children_.size(); }
        void clearChildren();

        GroupMode mode() const { return mode_; }
        void setMode(GroupMode mode) { mode_ = mode; }

    private:
        QString groupModeToString(GroupMode mode) const;

        QList<CuePtr> children_;
        GroupMode mode_;
    };

} // namespace CueForge