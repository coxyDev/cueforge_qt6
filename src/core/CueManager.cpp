// ============================================================================
// CueManager.cpp - Central cue management system implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "CueManager.h"
#include "cues/AudioCue.h"
#include "cues/GroupCue.h"
#include "cues/WaitCue.h"
#include "cues/ControlCue.h"
#include <QDebug>
#include <algorithm>

namespace CueForge {

    CueManager::CueManager(QObject* parent)
        : QObject(parent)
        , isPaused_(false)
        , hasUnsavedChanges_(false)
    {
        qDebug() << "CueManager initialized";
    }

    CueManager::~CueManager()
    {
        removeAllCues();
    }

    // ============================================================================
    // Cue Lifecycle
    // ============================================================================

    Cue* CueManager::createCue(CueType type, int index)
    {
        Cue* cue = nullptr;

        switch (type) {
        case CueType::Audio:
            cue = new AudioCue(this);
            break;
        case CueType::Group:
            cue = new GroupCue(this);
            break;
        case CueType::Wait:
            cue = new WaitCue(this);
            break;
        case CueType::Start:
        case CueType::Stop:
        case CueType::Goto:
        case CueType::Pause:
        case CueType::Load:
        case CueType::Reset:
        case CueType::Arm:
        case CueType::Disarm:
        case CueType::Devamp:
            cue = new ControlCue(type, this);
            break;
        default:
            qWarning() << "Cue type not yet implemented:" << cueTypeToString(type);
            return nullptr;
        }

        if (!cue) {
            return nullptr;
        }

        cue->setNumber(generateCueNumber());
        connectCueSignals(cue);

        if (index < 0 || index >= cues_.size()) {
            cues_.append(Cue::CuePtr(cue));
            index = cues_.size() - 1;
        }
        else {
            cues_.insert(index, Cue::CuePtr(cue));
        }

        markUnsaved();
        emit cueAdded(cue, index);
        emit cueCountChanged(cues_.size());

        if (cues_.size() == 1) {
            setStandByCue(cue->id());
        }

        qDebug() << "Created cue:" << cue->number() << cue->name() << "at index" << index;
        return cue;
    }

    bool CueManager::removeCue(const QString& cueId)
    {
        if (!validateCueOperation("remove", cueId)) {
            return false;
        }

        int index = getCueIndex(cueId);
        if (index < 0) {
            return false;
        }

        Cue* cue = cues_[index].get();

        if (activeCues_.contains(cueId)) {
            stopCue(cueId);
        }

        selectedCueIds_.removeAll(cueId);

        if (standByCueId_ == cueId) {
            standByCueId_.clear();
            if (!cues_.isEmpty()) {
                int nextIndex = qMin(index, cues_.size() - 2);
                if (nextIndex >= 0) {
                    setStandByCue(cues_[nextIndex]->id());
                }
            }
        }

        disconnectCueSignals(cue);
        cues_.removeAt(index);

        markUnsaved();
        emit cueRemoved(cueId);
        emit cueCountChanged(cues_.size());

        qDebug() << "Removed cue:" << cueId;
        return true;
    }

    void CueManager::removeAllCues()
    {
        stop();

        for (const auto& cue : cues_) {
            disconnectCueSignals(cue.get());
        }

        cues_.clear();
        selectedCueIds_.clear();
        activeCues_.clear();
        expandedGroups_.clear();
        standByCueId_.clear();

        emit workspaceCleared();
        emit cueCountChanged(0);
        emit selectionCleared();
    }

    Cue* CueManager::duplicateCue(const QString& cueId)
    {
        Cue* original = getCue(cueId);
        if (!original) {
            return nullptr;
        }

        int index = getCueIndex(cueId);
        auto cloned = original->clone();

        if (!cloned) {
            return nullptr;
        }

        Cue* newCue = cloned.release();
        newCue->setParent(this);
        newCue->setNumber(generateCueNumber());

        connectCueSignals(newCue);

        cues_.insert(index + 1, Cue::CuePtr(newCue));

        markUnsaved();
        emit cueAdded(newCue, index + 1);
        emit cueCountChanged(cues_.size());

        qDebug() << "Duplicated cue:" << original->number() << "→" << newCue->number();
        return newCue;
    }

    bool CueManager::renameCue(const QString& cueId, const QString& newNumber)
    {
        Cue* cue = getCue(cueId);
        if (!cue) {
            return false;
        }

        cue->setNumber(newNumber);
        markUnsaved();
        return true;
    }

    void CueManager::renumberAllCues()
    {
        for (int i = 0; i < cues_.size(); ++i) {
            cues_[i]->setNumber(QString::number(i + 1));
        }
        markUnsaved();
    }

    // ============================================================================
    // Cue Access
    // ============================================================================

    Cue* CueManager::getCue(const QString& cueId) const
    {
        auto it = std::find_if(cues_.begin(), cues_.end(),
            [&cueId](const Cue::CuePtr& cue) { return cue->id() == cueId; });

        return (it != cues_.end()) ? it->get() : nullptr;
    }

    Cue* CueManager::getCueByNumber(const QString& number) const
    {
        auto it = std::find_if(cues_.begin(), cues_.end(),
            [&number](const Cue::CuePtr& cue) { return cue->number() == number; });

        return (it != cues_.end()) ? it->get() : nullptr;
    }

    int CueManager::getCueIndex(const QString& cueId) const
    {
        for (int i = 0; i < cues_.size(); ++i) {
            if (cues_[i]->id() == cueId) {
                return i;
            }
        }
        return -1;
    }

    // ============================================================================
    // Selection System
    // ============================================================================

    Cue::CueList CueManager::selectedCues() const
    {
        Cue::CueList selected;
        for (const QString& id : selectedCueIds_) {
            Cue* cue = getCue(id);
            if (cue) {
                selected.append(Cue::CuePtr(cue, [](Cue*) {}));
            }
        }
        return selected;
    }

    bool CueManager::isCueSelected(const QString& cueId) const
    {
        return selectedCueIds_.contains(cueId);
    }

    void CueManager::selectCue(const QString& cueId, bool clearOthers)
    {
        if (!getCue(cueId)) {
            return;
        }

        if (clearOthers) {
            selectedCueIds_.clear();
        }

        if (!selectedCueIds_.contains(cueId)) {
            selectedCueIds_.append(cueId);
        }

        emit selectionChanged();
    }

    void CueManager::addToSelection(const QString& cueId)
    {
        if (!getCue(cueId)) {
            return;
        }

        if (!selectedCueIds_.contains(cueId)) {
            selectedCueIds_.append(cueId);
            emit selectionChanged();
        }
    }

    void CueManager::removeFromSelection(const QString& cueId)
    {
        if (selectedCueIds_.removeAll(cueId) > 0) {
            emit selectionChanged();
        }
    }

    void CueManager::toggleSelection(const QString& cueId)
    {
        if (isCueSelected(cueId)) {
            removeFromSelection(cueId);
        }
        else {
            addToSelection(cueId);
        }
    }

    void CueManager::selectRange(const QString& startId, const QString& endId)
    {
        int startIndex = getCueIndex(startId);
        int endIndex = getCueIndex(endId);

        if (startIndex < 0 || endIndex < 0) {
            return;
        }

        if (startIndex > endIndex) {
            std::swap(startIndex, endIndex);
        }

        selectedCueIds_.clear();

        for (int i = startIndex; i <= endIndex; ++i) {
            selectedCueIds_.append(cues_[i]->id());
        }

        emit selectionChanged();
    }

    void CueManager::selectAll()
    {
        selectedCueIds_.clear();
        for (const auto& cue : cues_) {
            selectedCueIds_.append(cue->id());
        }
        emit selectionChanged();
    }

    void CueManager::clearSelection()
    {
        if (!selectedCueIds_.isEmpty()) {
            selectedCueIds_.clear();
            emit selectionCleared();
            emit selectionChanged();
        }
    }

    // ============================================================================
    // Group Management
    // ============================================================================

    Cue* CueManager::createGroupFromSelection(const QString& groupName)
    {
        if (selectedCueIds_.isEmpty()) {
            emit warning("No cues selected to group");
            return nullptr;
        }

        QList<Cue*> selected = selectedCues();
        if (selected.isEmpty()) {
            return nullptr;
        }

        std::sort(selected.begin(), selected.end(), [this](Cue* a, Cue* b) {
            return getCueIndex(a->id()) < getCueIndex(b->id());
            });

        int firstIndex = getCueIndex(selected.first()->id());

        GroupCue* group = static_cast<GroupCue*>(createCue(CueType::Group, firstIndex));
        if (!group) {
            return nullptr;
        }

        group->setName(groupName);

        for (Cue* cue : selected) {
            int index = getCueIndex(cue->id());
            if (index > firstIndex) {
                cues_.removeAt(index);
                group->addChild(Cue::CuePtr(cue, [](Cue*) {}));
            }
        }

        clearSelection();
        selectCue(group->id());

        markUnsaved();

        qDebug() << "Created group:" << groupName << "with" << group->childCount() << "children";
        return group;
    }

    bool CueManager::ungroupCue(const QString& groupId)
    {
        if (!validateGroupOperation(groupId)) {
            return false;
        }

        GroupCue* group = static_cast<GroupCue*>(getCue(groupId));
        int groupIndex = getCueIndex(groupId);

        Cue::CueList children = group->children();

        for (int i = 0; i < children.size(); ++i) {
            cues_.insert(groupIndex + i + 1, children[i]);
        }

        group->clearChildren();
        removeCue(groupId);

        markUnsaved();

        qDebug() << "Ungrouped:" << group->name() << "released" << children.size() << "children";
        return true;
    }

    bool CueManager::isGroupExpanded(const QString& groupId) const
    {
        return expandedGroups_.contains(groupId);
    }

    void CueManager::setGroupExpanded(const QString& groupId, bool expanded)
    {
        if (expanded) {
            expandedGroups_.insert(groupId);
        }
        else {
            expandedGroups_.remove(groupId);
        }
    }

    // ============================================================================
    // Cue Ordering
    // ============================================================================

    bool CueManager::moveCue(const QString& cueId, int newIndex)
    {
        int oldIndex = getCueIndex(cueId);
        if (oldIndex < 0) {
            return false;
        }

        if (newIndex < 0 || newIndex >= cues_.size()) {
            return false;
        }

        if (oldIndex == newIndex) {
            return true;
        }

        auto cue = cues_[oldIndex];
        cues_.removeAt(oldIndex);
        cues_.insert(newIndex, cue);

        markUnsaved();
        emit cueMoved(cueId, oldIndex, newIndex);

        return true;
    }

    bool CueManager::moveCueUp(const QString& cueId)
    {
        int index = getCueIndex(cueId);
        if (index <= 0) {
            return false;
        }
        return moveCue(cueId, index - 1);
    }

    bool CueManager::moveCueDown(const QString& cueId)
    {
        int index = getCueIndex(cueId);
        if (index < 0 || index >= cues_.size() - 1) {
            return false;
        }
        return moveCue(cueId, index + 1);
    }

    // ============================================================================
    // Playback Control
    // ============================================================================

    bool CueManager::go()
    {
        Cue* standbyCue = standByCue();
        if (!standbyCue) {
            emit warning("No standby cue set");
            return false;
        }

        if (!canExecuteCue(standbyCue)) {
            emit warning("Cannot execute cue: " + standbyCue->name());
            return false;
        }

        executeCue(standbyCue);

        if (standbyCue->continueMode()) {
            advanceStandBy();
        }

        return true;
    }

    void CueManager::stop()
    {
        QStringList activeIds = activeCueIds();
        for (const QString& id : activeIds) {
            stopCue(id);
        }

        isPaused_ = false;
        emit playbackStateChanged();

        qDebug() << "Stopped all cues";
    }

    void CueManager::stopCue(const QString& cueId, double fadeTime)
    {
        Cue* cue = getCue(cueId);
        if (!cue) {
            return;
        }

        cue->stop(fadeTime);
        activeCues_.remove(cueId);
        cue->setStatus(CueStatus::Loaded);

        emit playbackStopped(cueId);
        emit playbackStateChanged();
        emit cueUpdated(cue);

        qDebug() << "Stopped cue:" << cue->number() << cue->name();
    }

    void CueManager::pause()
    {
        if (activeCues_.isEmpty()) {
            return;
        }

        isPaused_ = !isPaused_;

        for (const QString& id : activeCues_) {
            Cue* cue = getCue(id);
            if (cue) {
                if (isPaused_) {
                    cue->pause();
                    cue->setStatus(CueStatus::Paused);
                }
                else {
                    cue->resume();
                    cue->setStatus(CueStatus::Running);
                }
            }
        }

        emit playbackStateChanged();
        qDebug() << (isPaused_ ? "Paused" : "Resumed") << "playback";
    }

    void CueManager::panic()
    {
        for (const QString& id : activeCues_) {
            Cue* cue = getCue(id);
            if (cue) {
                cue->stop(0.0);
                cue->setStatus(CueStatus::Loaded);
            }
        }

        activeCues_.clear();
        isPaused_ = false;

        emit playbackStateChanged();
        emit warning("PANIC - All cues stopped");

        qDebug() << "PANIC STOP executed";
    }

    QStringList CueManager::activeCueIds() const
    {
        return activeCues_.values();
    }

    // ============================================================================
    // Standby System
    // ============================================================================

    void CueManager::setStandByCue(const QString& cueId)
    {
        if (standByCueId_ == cueId) {
            return;
        }

        standByCueId_ = cueId;
        emit standByCueChanged(cueId);

        qDebug() << "Standby cue set to:" << (cueId.isEmpty() ? "none" : getCue(cueId)->number());
    }

    Cue* CueManager::standByCue() const
    {
        return getCue(standByCueId_);
    }

    void CueManager::nextCue()
    {
        if (cues_.isEmpty()) {
            return;
        }

        int currentIndex = getCueIndex(standByCueId_);
        if (currentIndex < cues_.size() - 1) {
            setStandByCue(cues_[currentIndex + 1]->id());
        }
    }

    void CueManager::previousCue()
    {
        if (cues_.isEmpty()) {
            return;
        }

        int currentIndex = getCueIndex(standByCueId_);
        if (currentIndex > 0) {
            setStandByCue(cues_[currentIndex - 1]->id());
        }
        else if (currentIndex < 0 && !cues_.isEmpty()) {
            setStandByCue(cues_[0]->id());
        }
    }

    // ============================================================================
    // Workspace Management
    // ============================================================================

    void CueManager::newWorkspace()
    {
        stop();

        cues_.clear();
        selectedCueIds_.clear();
        activeCues_.clear();
        expandedGroups_.clear();
        standByCueId_.clear();
        clipboard_.clear();

        isPaused_ = false;
        hasUnsavedChanges_ = false;

        emit workspaceCleared();
        emit cueCountChanged(0);
        emit selectionCleared();
        emit standByCueChanged(QString());
        emit unsavedChangesChanged(false);

        qDebug() << "New workspace created";
    }

    bool CueManager::loadWorkspace(const QJsonObject& json)
    {
        stop();

        cues_.clear();
        selectedCueIds_.clear();
        activeCues_.clear();
        expandedGroups_.clear();

        QJsonArray cuesArray = json["cues"].toArray();
        for (const QJsonValue& value : cuesArray) {
            QJsonObject cueJson = value.toObject();

            QString typeStr = cueJson["type"].toString();
            CueType type = stringToCueType(typeStr);

            Cue* cue = createCue(type, -1);
            if (cue) {
                cue->fromJson(cueJson);
            }
        }

        QJsonArray expandedArray = json["expandedGroups"].toArray();
        for (const QJsonValue& value : expandedArray) {
            expandedGroups_.insert(value.toString());
        }

        if (json.contains("standByCueId")) {
            QString standbyId = json["standByCueId"].toString();
            if (getCue(standbyId)) {
                setStandByCue(standbyId);
            }
        }
        else if (!cues_.isEmpty()) {
            setStandByCue(cues_[0]->id());
        }

        hasUnsavedChanges_ = false;

        emit workspaceLoaded();
        emit cueCountChanged(cues_.size());
        emit unsavedChangesChanged(false);

        qDebug() << "Workspace loaded:" << cues_.size() << "cues";
        return true;
    }

    QJsonObject CueManager::saveWorkspace() const
    {
        QJsonObject json;

        QJsonArray cuesArray;
        for (const auto& cue : cues_) {
            cuesArray.append(cue->toJson());
        }
        json["cues"] = cuesArray;

        QJsonArray expandedArray;
        for (const QString& id : expandedGroups_) {
            expandedArray.append(id);
        }
        json["expandedGroups"] = expandedArray;

        if (!standByCueId_.isEmpty()) {
            json["standByCueId"] = standByCueId_;
        }

        json["version"] = "2.0";
        json["createdTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        json["cueCount"] = cues_.size();

        return json;
    }

    void CueManager::markSaved()
    {
        if (hasUnsavedChanges_) {
            hasUnsavedChanges_ = false;
            emit unsavedChangesChanged(false);
        }
    }

    void CueManager::markUnsaved()
    {
        if (!hasUnsavedChanges_) {
            hasUnsavedChanges_ = true;
            emit unsavedChangesChanged(true);
        }
    }

    // ============================================================================
    // Clipboard Operations
    // ============================================================================

    void CueManager::copy()
    {
        clipboard_.clear();

        QList<Cue*> selected = selectedCues();
        if (selected.isEmpty()) {
            return;
        }

        std::sort(selected.begin(), selected.end(), [this](Cue* a, Cue* b) {
            return getCueIndex(a->id()) < getCueIndex(b->id());
            });

        for (Cue* cue : selected) {
            clipboard_.append(cue->toJson());
        }

        qDebug() << "Copied" << clipboard_.size() << "cues to clipboard";
    }

    void CueManager::cut()
    {
        copy();

        QStringList ids = selectedCueIds();
        for (const QString& id : ids) {
            removeCue(id);
        }

        qDebug() << "Cut" << ids.size() << "cues";
    }

    void CueManager::paste(int index)
    {
        if (clipboard_.isEmpty()) {
            return;
        }

        int pasteIndex = index;
        if (pasteIndex < 0) {
            if (!selectedCueIds_.isEmpty()) {
                int maxIndex = -1;
                for (const QString& id : selectedCueIds_) {
                    int idx = getCueIndex(id);
                    if (idx > maxIndex) {
                        maxIndex = idx;
                    }
                }
                pasteIndex = maxIndex + 1;
            }
            else {
                pasteIndex = cues_.size();
            }
        }

        clearSelection();

        QStringList pastedIds;
        for (const QJsonObject& cueJson : clipboard_) {
            QString typeStr = cueJson["type"].toString();
            CueType type = stringToCueType(typeStr);

            Cue* cue = createCue(type, pasteIndex);
            if (cue) {
                cue->fromJson(cueJson);
                pastedIds.append(cue->id());
                pasteIndex++;
            }
        }

        for (const QString& id : pastedIds) {
            addToSelection(id);
        }

        renumberAllCues();

        qDebug() << "Pasted" << pastedIds.size() << "cues";
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    void CueManager::connectCueSignals(Cue* cue)
    {
        if (!cue) return;

        connect(cue, &Cue::statusChanged, this, [this, cue]() {
            onCueStatusChanged(cue);
            });

        connect(cue, &Cue::executionFinished, this, [this, cue]() {
            onCueFinished(cue->id());
            });

        connect(cue, &Cue::numberChanged, this, [this, cue]() {
            emit cueUpdated(cue);
            });

        connect(cue, &Cue::nameChanged, this, [this, cue]() {
            emit cueUpdated(cue);
            });

        connect(cue, &Cue::error, this, &CueManager::error);
        connect(cue, &Cue::warning, this, &CueManager::warning);
    }

    void CueManager::disconnectCueSignals(Cue* cue)
    {
        if (!cue) return;
        disconnect(cue, nullptr, this, nullptr);
    }

    void CueManager::executeCue(Cue* cue)
    {
        if (!cue || !canExecuteCue(cue)) {
            return;
        }

        activeCues_.insert(cue->id());
        cue->setStatus(CueStatus::Running);

        bool started = cue->execute();

        if (started) {
            emit playbackStarted(cue->id());
            emit playbackStateChanged();
            emit cueUpdated(cue);

            qDebug() << "Executing cue:" << cue->number() << cue->name();
        }
        else {
            activeCues_.remove(cue->id());
            cue->setStatus(CueStatus::Loaded);
            emit error("Failed to execute cue: " + cue->name());
        }
    }

    bool CueManager::canExecuteCue(Cue* cue) const
    {
        if (!cue) return false;
        return cue->canExecute();
    }

    void CueManager::advanceStandBy()
    {
        nextCue();
    }

    QString CueManager::generateCueNumber() const
    {
        if (cues_.isEmpty()) {
            return "1";
        }

        int maxNumber = 0;
        for (const auto& cue : cues_) {
            bool ok;
            int num = cue->number().toInt(&ok);
            if (ok && num > maxNumber) {
                maxNumber = num;
            }
        }

        return QString::number(maxNumber + 1);
    }

    bool CueManager::validateCueOperation(const QString& operation, const QString& cueId) const
    {
        if (cueId.isEmpty()) {
            qWarning() << "Cannot" << operation << "- invalid cue ID";
            return false;
        }

        if (!getCue(cueId)) {
            qWarning() << "Cannot" << operation << "- cue not found:" << cueId;
            return false;
        }

        return true;
    }

    bool CueManager::validateGroupOperation(const QString& groupId) const
    {
        if (!validateCueOperation("group operation", groupId)) {
            return false;
        }

        Cue* cue = getCue(groupId);
        if (cue->type() != CueType::Group) {
            qWarning() << "Cue is not a group:" << groupId;
            return false;
        }

        return true;
    }

    void CueManager::onCueFinished(const QString& cueId)
    {
        Cue* cue = getCue(cueId);
        if (!cue) {
            return;
        }

        activeCues_.remove(cueId);
        cue->setStatus(CueStatus::Finished);

        emit playbackStopped(cueId);
        emit playbackStateChanged();
        emit cueUpdated(cue);

        qDebug() << "Cue finished:" << cue->number() << cue->name();

        if (cue->id() == standByCueId_ && cue->continueMode()) {
            advanceStandBy();
            if (standByCue()) {
                QTimer::singleShot(static_cast<int>(cue->postWait() * 1000), this, [this]() {
                    go();
                    });
            }
        }
    }

    void CueManager::onCueStatusChanged(Cue* cue)
    {
        if (!cue) return;
        emit cueUpdated(cue);
    }

} // namespace CueForge