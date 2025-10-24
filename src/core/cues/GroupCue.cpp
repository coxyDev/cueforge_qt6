// ============================================================================
// GroupCue.cpp - Container cue implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "GroupCue.h"
#include <QJsonArray>
#include <QDebug>
#include <QCoreApplication>

namespace CueForge {

    GroupCue::GroupCue(QObject* parent)
        : Cue(CueType::Group, parent)
        , mode_(GroupMode::Sequential)
    {
        setName("Group");
        setColor(QColor(100, 149, 237)); // Cornflower blue
    }

    bool GroupCue::execute()
    {
        if (status() == CueStatus::Running) {
            return false;
        }

        setStatus(CueStatus::Running);

        if (mode_ == GroupMode::Sequential) {
            // Execute first child, rest will follow via continue mode
            if (!children_.isEmpty() && children_.first()) {
                children_.first()->execute();
            }
        }
        else {
            // Execute all children simultaneously
            for (auto& child : children_) {
                if (child) {
                    child->execute();
                }
            }
        }

        return true;
    }

    void GroupCue::stop(double fadeTime)  // FIXED: Added fadeTime parameter
    {
        // Stop all children with the same fade time
        for (auto& child : children_) {
            if (child && (child->status() == CueStatus::Running ||
                child->status() == CueStatus::Paused)) {
                child->stop(fadeTime);  // Pass fade time to children
            }
        }

        setStatus(CueStatus::Idle);
    }

    void GroupCue::pause()
    {
        if (status() != CueStatus::Running) {
            return;
        }

        // Pause all running children
        for (auto& child : children_) {
            if (child && child->status() == CueStatus::Running) {
                child->pause();
            }
        }

        setStatus(CueStatus::Paused);
    }

    void GroupCue::resume()
    {
        if (status() != CueStatus::Paused) {
            return;
        }

        // Resume all paused children
        for (auto& child : children_) {
            if (child && child->status() == CueStatus::Paused) {
                child->resume();
            }
        }

        setStatus(CueStatus::Running);
    }

    QJsonObject GroupCue::toJson() const
    {
        QJsonObject json = Cue::toJson();

        // Save children as an array
        QJsonArray childrenArray;
        for (const auto& child : children_) {
            if (child) {
                childrenArray.append(child->toJson());
            }
        }

        json["children"] = childrenArray;
        json["mode"] = groupModeToString(mode_);

        return json;
    }

    void GroupCue::fromJson(const QJsonObject& json)
    {
        Cue::fromJson(json);

        // Clear existing children
        children_.clear();

        // Load mode
        QString modeStr = json["mode"].toString();
        if (modeStr == "Sequential") {
            mode_ = GroupMode::Sequential;
        }
        else if (modeStr == "Simultaneous") {
            mode_ = GroupMode::Simultaneous;
        }

        // Note: Children will be loaded by CueManager in a second pass
    }

    void GroupCue::addChild(CuePtr child)
    {
        if (!child) {
            return;
        }

        children_.append(child);
        child->setParent(this);

        qDebug() << "Added child to group:" << child->number();
    }

    Cue::CuePtr GroupCue::removeChild(int index)
    {
        if (index < 0 || index >= children_.size()) {
            return CuePtr();
        }

        CuePtr child = children_.takeAt(index);

        if (child) {
            child->setParent(nullptr);
        }

        return child;
    }

    Cue* GroupCue::getChildAt(int index) const
    {
        if (index < 0 || index >= children_.size()) {
            return nullptr;
        }

        return children_[index].get();
    }

    void GroupCue::clearChildren()
    {
        for (auto& child : children_) {
            if (child) {
                child->setParent(nullptr);
            }
        }
        children_.clear();
    }

    QString GroupCue::groupModeToString(GroupMode mode) const
    {
        switch (mode) {
        case GroupMode::Sequential:
            return "Sequential";
        case GroupMode::Simultaneous:
            return "Simultaneous";
        default:
            return "Sequential";
        }
    }

    bool GroupCue::canExecute() const
    {
        // Group can execute if it has children
        return !children_.isEmpty();
    }

    bool GroupCue::validate()
    {
        // Group is valid if it has at least one child
        if (children_.isEmpty()) {
            setIsBroken(true);
            return false;
        }

        // Also validate all children
        bool allValid = true;
        for (const auto& child : children_) {
            if (child && !child->validate()) {
                allValid = false;
            }
        }

        setIsBroken(!allValid);
        return allValid;
    }

    QString GroupCue::validationError() const
    {
        if (children_.isEmpty()) {
            return tr("Group has no children");
        }

        // Check if any children are broken
        for (const auto& child : children_) {
            if (child && child->isBroken()) {
                return tr("One or more children have errors");
            }
        }

        return QString();
    }

    std::unique_ptr<Cue> GroupCue::clone() const
    {
        auto cloned = std::make_unique<GroupCue>();

        // Copy base properties
        cloned->setNumber(number());
        cloned->setName(name() + " Copy");
        cloned->setDuration(duration());
        cloned->setPreWait(preWait());
        cloned->setPostWait(postWait());
        cloned->setContinueMode(continueMode());
        cloned->setColor(color());
        cloned->setNotes(notes());
        cloned->setArmed(isArmed());

        // Copy group-specific properties
        cloned->setMode(mode_);

        // Clone all children
        for (const auto& child : children_) {
            if (child) {
                auto clonedChild = child->clone();
                if (clonedChild) {
                    cloned->addChild(CuePtr(clonedChild.release()));
                }
            }
        }

        return cloned;
    }

} // namespace CueForge