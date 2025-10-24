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

        // Cue interface - MUST match base class signatures exactly
        bool execute() override;
        void stop(double fadeTime = 0.0) override;
        void pause() override;
        void resume() override;
        bool canExecute() const override;          // ADD THIS
        bool validate() override;                  // ADD THIS
        QString validationError() const override;  // ADD THIS
        QJsonObject toJson() const override;
        void fromJson(const QJsonObject& json) override;
        std::unique_ptr<Cue> clone() const override;  // ADD THIS

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