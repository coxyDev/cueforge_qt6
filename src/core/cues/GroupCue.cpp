// ============================================================================
// GroupCue.cpp - Hierarchical cue grouping implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "GroupCue.h"
#include <QDebug>
#include <algorithm>

namespace CueForge {

    GroupCue::GroupCue(QObject* parent)
        : Cue(CueType::Group, parent)
        , mode_(GroupMode::Sequential)
        , currentChildIndex_(-1)
    {
        setName("Group");
        setColor(QColor(150, 150, 150));
    }

    void GroupCue::setChildren(const CueList& children)
    {
        children_ = children;
        setDuration(totalDuration());
        emit childCountChanged(children_.size());
    }

    void GroupCue::addChild(CuePtr cue, int index)
    {
        if (!cue) {
            return;
        }

        cue->setParent(this);

        if (index < 0 || index >= children_.size()) {
            children_.append(cue);
        }
        else {
            children_.insert(index, cue);
        }

        setDuration(totalDuration());
        updateModifiedTime();
        emit childCountChanged(children_.size());
    }

    bool GroupCue::removeChild(const QString& cueId)
    {
        auto it = std::find_if(children_.begin(), children_.end(),
            [&cueId](const CuePtr& cue) { return cue->id() == cueId; });

        if (it == children_.end()) {
            return false;
        }

        children_.erase(it);
        setDuration(totalDuration());
        updateModifiedTime();
        emit childCountChanged(children_.size());

        return true;
    }

    Cue* GroupCue::getChild(const QString& cueId) const
    {
        auto it = std::find_if(children_.begin(), children_.end(),
            [&cueId](const CuePtr& cue) { return cue->id() == cueId; });

        return (it != children_.end()) ? it->get() : nullptr;
    }

    Cue* GroupCue::getChildAt(int index) const
    {
        if (index >= 0 && index < children_.size()) {
            return children_[index].get();
        }
        return nullptr;
    }

    void GroupCue::clearChildren()
    {
        children_.clear();
        setDuration(0.0);
        emit childCountChanged(0);
    }

    void GroupCue::setMode(GroupMode mode)
    {
        if (mode_ != mode) {
            mode_ = mode;
            setDuration(totalDuration());
            updateModifiedTime();
            emit modeChanged(mode);
        }
    }

    double GroupCue::totalDuration() const
    {
        if (children_.isEmpty()) {
            return 0.0;
        }

        if (mode_ == GroupMode::Sequential) {
            double total = 0.0;
            for (const auto& child : children_) {
                total += child->preWait() + child->duration() + child->postWait();
            }
            return total;
        }
        else {
            double maxDuration = 0.0;
            for (const auto& child : children_) {
                double childTotal = child->preWait() + child->duration() + child->postWait();
                maxDuration = qMax(maxDuration, childTotal);
            }
            return maxDuration;
        }
    }

    bool GroupCue::execute()
    {
        if (!canExecute()) {
            return false;
        }

        setStatus(CueStatus::Running);
        currentChildIndex_ = -1;
        activeChildren_.clear();

        if (mode_ == GroupMode::Sequential) {
            executeNextChild();
        }
        else {
            executeAllChildren();
        }

        qDebug() << "GroupCue execute:" << name() << "mode:" << (int)mode_
            << "children:" << children_.size();

        return true;
    }

    void GroupCue::stop(double fadeTime)
    {
        for (const QString& childId : activeChildren_) {
            Cue* child = getChild(childId);
            if (child) {
                child->stop(fadeTime);
            }
        }

        activeChildren_.clear();
        currentChildIndex_ = -1;
        setStatus(CueStatus::Loaded);

        qDebug() << "GroupCue stopped:" << name();
    }

    void GroupCue::pause()
    {
        for (const QString& childId : activeChildren_) {
            Cue* child = getChild(childId);
            if (child) {
                child->pause();
            }
        }

        setStatus(CueStatus::Paused);
    }

    void GroupCue::resume()
    {
        for (const QString& childId : activeChildren_) {
            Cue* child = getChild(childId);
            if (child) {
                child->resume();
            }
        }

        setStatus(CueStatus::Running);
    }

    bool GroupCue::canExecute() const
    {
        if (!Cue::canExecute()) {
            return false;
        }

        for (const auto& child : children_) {
            if (child->canExecute()) {
                return true;
            }
        }

        return false;
    }

    bool GroupCue::validate()
    {
        if (children_.isEmpty()) {
            return false;
        }

        bool allValid = true;
        for (const auto& child : children_) {
            if (!child->validate()) {
                allValid = false;
            }
        }

        return allValid;
    }

    QString GroupCue::validationError() const
    {
        if (children_.isEmpty()) {
            return "Group has no children";
        }

        int brokenCount = 0;
        for (const auto& child : children_) {
            if (child->isBroken()) {
                brokenCount++;
            }
        }

        if (brokenCount > 0) {
            return QString("Group has %1 broken child cue(s)").arg(brokenCount);
        }

        return QString();
    }

    QJsonObject GroupCue::toJson() const
    {
        QJsonObject json = Cue::toJson();

        json["mode"] = (mode_ == GroupMode::Sequential) ? "sequential" : "parallel";

        QJsonArray childrenArray;
        for (const auto& child : children_) {
            childrenArray.append(child->toJson());
        }
        json["children"] = childrenArray;

        return json;
    }

    void GroupCue::fromJson(const QJsonObject& json)
    {
        Cue::fromJson(json);

        QString modeStr = json.value("mode").toString("sequential");
        setMode(modeStr == "parallel" ? GroupMode::Parallel : GroupMode::Sequential);

        if (json.contains("children")) {
            qDebug() << "GroupCue: Child deserialization needs CueFactory implementation";
        }
    }

    std::unique_ptr<Cue> GroupCue::clone() const
    {
        auto cloned = std::make_unique<GroupCue>();

        cloned->setNumber(number());
        cloned->setName(name() + " (copy)");
        cloned->setDuration(duration());
        cloned->setPreWait(preWait());
        cloned->setPostWait(postWait());
        cloned->setContinueMode(continueMode());
        cloned->setColor(color());
        cloned->setNotes(notes());
        cloned->setMode(mode_);

        for (const auto& child : children_) {
            auto childClone = child->clone();
            if (childClone) {
                cloned->addChild(std::shared_ptr<Cue>(childClone.release()));
            }
        }

        return cloned;
    }

    void GroupCue::executeNextChild()
    {
        currentChildIndex_++;

        if (currentChildIndex_ >= children_.size()) {
            setStatus(CueStatus::Finished);
            emit executionFinished();
            return;
        }

        Cue* child = children_[currentChildIndex_].get();
        if (!child->canExecute()) {
            executeNextChild();
            return;
        }

        connect(child, &Cue::executionFinished, this, [this, child]() {
            onChildFinished(child->id());
            }, Qt::UniqueConnection);

        activeChildren_.insert(child->id());
        emit childExecutionStarted(child->id());

        child->execute();
    }

    void GroupCue::executeAllChildren()
    {
        for (const auto& child : children_) {
            if (!child->canExecute()) {
                continue;
            }

            connect(child.get(), &Cue::executionFinished, this, [this, child]() {
                onChildFinished(child->id());
                }, Qt::UniqueConnection);

            activeChildren_.insert(child->id());
            emit childExecutionStarted(child->id());

            child->execute();
        }

        if (activeChildren_.isEmpty()) {
            setStatus(CueStatus::Finished);
            emit executionFinished();
        }
    }

    void GroupCue::onChildFinished(const QString& childId)
    {
        activeChildren_.remove(childId);
        emit childExecutionFinished(childId);

        if (mode_ == GroupMode::Sequential) {
            executeNextChild();
        }
        else {
            if (activeChildren_.isEmpty()) {
                setStatus(CueStatus::Finished);
                emit executionFinished();
            }
        }
    }

} // namespace CueForge