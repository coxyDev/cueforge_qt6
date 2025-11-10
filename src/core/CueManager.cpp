// ============================================================================
// CueManager.cpp - Central cue management implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "CueManager.h"
#include "cues/AudioCue.h"
#include "../audio/AudioEngineQt.h"
#include "cues/GroupCue.h"
#include "cues/WaitCue.h"
#include "cues/ControlCue.h"
#include <QDebug>
#include <QJsonArray>
#include <algorithm>
#include <QPair>

namespace CueForge {

CueManager::CueManager(QObject* parent)
    : QObject(parent)
    , standByCue_(nullptr)
    , hasUnsavedChanges_(false)
	, audioEngine_(nullptr)
{
    qDebug() << "CueManager initialized";
}

void CueManager::setAudioEngine(AudioEngineQt* engine)
{
    audioEngine_ = engine;

    // Update any existing audio cues
    for (int i = 0; i < cues_.size(); ++i) {
        if (AudioCue* audioCue = qobject_cast<AudioCue*>(cues_[i].get())) {
            audioCue->setAudioEngine(engine);
        }
    }

    qDebug() << "CueManager: Audio engine connected";
}

// ============================================================================
// Cue Creation and Management
// ============================================================================

Cue* CueManager::createCue(CueType type, int index)
{
    Cue* cue = nullptr;

    switch (type) {
    case CueType::Audio: {
        AudioCue* audioCue = new AudioCue(this);
        if (audioEngine_) {
            audioCue->setAudioEngine(audioEngine_);
        }
        cue = audioCue;
        break;
    }
    case CueType::Group:
        cue = new GroupCue(this);
        break;
    case CueType::Wait:
        cue = new WaitCue(this);
        break;
    case CueType::Start:
        cue = new ControlCue(ControlCue::ControlType::Start, this);
        break;
    case CueType::Stop:
        cue = new ControlCue(ControlCue::ControlType::Stop, this);
        break;
    case CueType::Goto:
        cue = new ControlCue(ControlCue::ControlType::Goto, this);
        break;
    default:
        qWarning() << "CueManager::createCue - Unsupported cue type";
        return nullptr;
    }

    if (!cue) {
        return nullptr;
    }

    // Assign cue number
    if (cue->number().isEmpty() || cue->number() == "1") {
        cue->setNumber(generateCueNumber(index));
    }

    // Wrap in shared pointer
    CuePtr cuePtr(cue);

    // Insert at specified index
    if (index >= 0 && index < cues_.size()) {
        cues_.insert(index, cuePtr);
    }
    else {
        cues_.append(cuePtr);
    }

    // Connect signals
    connect(cue, &Cue::numberChanged, this, &CueManager::onCueUpdated);
    connect(cue, &Cue::nameChanged, this, &CueManager::onCueUpdated);
    connect(cue, &Cue::durationChanged, this, &CueManager::onCueUpdated);
    connect(cue, &Cue::executionFinished, this, &CueManager::onCueFinished);

    // Mark as changed
    hasUnsavedChanges_ = true;
    emit cueAdded(cue->id());

    qDebug() << "CueManager: Created cue" << cue->number() << "-" << cue->name();

    return cue;
}

bool CueManager::removeCue(const QString& cueId)
{
    int index = getCueIndex(cueId);
    if (index < 0) {
        return false;
    }
    
    Cue* cue = cues_[index].get();
    if (!cue) {
        return false;
    }
    
    // CRITICAL: Emit signal BEFORE deleting so listeners can clean up
    emit cueRemoved(cueId);
    
    // Remove from selection
    selectedCueIds_.removeAll(cueId);
    
    // Clear standby if this was it
    if (standByCue_ && standByCue_->id() == cueId) {
        standByCue_ = nullptr;
        emit standByCueChanged(QString());
    }
    
    // Stop if running
    if (cue->status() == CueStatus::Running || cue->status() == CueStatus::Paused) {
        cue->stop();
    }
    
    // Remove from active list
    activeCueIds_.removeAll(cueId);
    
    // Now actually remove the cue
    cues_.removeAt(index);
    
    markUnsaved();
    renumberAllCues();
    
    emit cueCountChanged(cues_.size());
    emit selectionChanged();
    
    qDebug() << "Removed cue:" << cueId;
    return true;
}

void CueManager::removeCueWithoutSignals(const QString& cueId)
{
    int index = getCueIndex(cueId);
    if (index >= 0) {
        // Just remove from list, don't emit signals
        cues_.removeAt(index);
        selectedCueIds_.removeAll(cueId);
        activeCueIds_.removeAll(cueId);
        
        // Clear standby if this was it
        if (standByCue_ && standByCue_->id() == cueId) {
            standByCue_ = nullptr;
        }
    }
}

Cue* CueManager::getCue(const QString& cueId) const
{
    for (const auto& cuePtr : cues_) {
        if (cuePtr && cuePtr->id() == cueId) {
            return cuePtr.get();
        }
    }
    return nullptr;
}

int CueManager::getCueIndex(const QString& cueId) const
{
    for (int i = 0; i < cues_.size(); ++i) {
        if (cues_[i] && cues_[i]->id() == cueId) {
            return i;
        }
    }
    return -1;
}

bool CueManager::moveCue(const QString& cueId, int newIndex)
{
    int oldIndex = getCueIndex(cueId);
    if (oldIndex < 0 || newIndex < 0 || newIndex >= cues_.size()) {
        return false;
    }
    
    if (oldIndex == newIndex) {
        return true;
    }
    
    auto cue = cues_[oldIndex];
    cues_.removeAt(oldIndex);
    
    if (newIndex > oldIndex) {
        newIndex--;
    }
    
    cues_.insert(newIndex, cue);
    
    markUnsaved();
    renumberAllCues();
    
    emit cueMoved(cueId, oldIndex, newIndex);
    
    qDebug() << "Moved cue" << cueId << "from" << oldIndex << "to" << newIndex;
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

Cue* CueManager::duplicateCue(const QString& cueId)
{
    Cue* original = getCue(cueId);
    if (!original) {
        return nullptr;
    }
    
    int index = getCueIndex(cueId);
    Cue* duplicate = createCue(original->type(), index + 1);
    
    if (duplicate) {
        QJsonObject json = original->toJson();
        duplicate->fromJson(json);
        duplicate->setName(original->name() + " Copy");
        
        qDebug() << "Duplicated cue:" << original->number();
    }
    
    return duplicate;
}

// ============================================================================
// Standby and Playback
// ============================================================================

Cue* CueManager::standByCue() const
{
    return standByCue_.data();
}

void CueManager::setStandByCue(const QString& cueId)
{
    Cue* cue = getCue(cueId);
    setStandByCue(cue);
}

void CueManager::setStandByCue(Cue* cue)
{
    if (standByCue_ == cue) {
        return;
    }
    
    standByCue_ = cue;
    emit standByCueChanged(cue ? cue->id() : QString());
    
    qDebug() << "Standby cue set to:" << (cue ? cue->number() : "None");
}

QStringList CueManager::activeCueIds() const
{
    return activeCueIds_;
}

bool CueManager::go()
{
    if (!standByCue_) {
        // Find first cue if no standby set
        if (cues_.isEmpty()) {
            emit warning("No cues to execute");
            return false;
        }
        standByCue_ = cues_.first().get();
    }
    
    if (!standByCue_) {
        return false;
    }
    
    // Execute the cue
    standByCue_->execute();
    
    // Add to active list
    if (!activeCueIds_.contains(standByCue_->id())) {
        activeCueIds_.append(standByCue_->id());
    }
    
    emit playbackStateChanged();
    
    // Move standby to next cue
    int currentIndex = getCueIndex(standByCue_->id());
    if (currentIndex >= 0 && currentIndex < cues_.size() - 1) {
        standByCue_ = cues_[currentIndex + 1].get();
        emit standByCueChanged(standByCue_ ? standByCue_->id() : QString());
    } else {
        standByCue_ = nullptr;
        emit standByCueChanged(QString());
    }
    
    qDebug() << "GO executed";
    return true;
}

void CueManager::stop()
{
    // Stop all active cues
    for (const QString& cueId : activeCueIds_) {
        Cue* cue = getCue(cueId);
        if (cue) {
            cue->stop();
        }
    }
    
    activeCueIds_.clear();
    emit playbackStateChanged();
    
    qDebug() << "All cues stopped";
}

void CueManager::pause()
{
    // Pause all active cues
    for (const QString& cueId : activeCueIds_) {
        Cue* cue = getCue(cueId);
        if (cue) {
            if (cue->status() == CueStatus::Running) {
                cue->pause();
            } else if (cue->status() == CueStatus::Paused) {
                cue->resume();
            }
        }
    }
    
    emit playbackStateChanged();
    
    qDebug() << "Pause/Resume toggled";
}

void CueManager::panic()
{
    // Emergency stop - stop everything immediately
    for (auto& cuePtr : cues_) {
        if (cuePtr) {
            cuePtr->stop();
        }
    }
    
    activeCueIds_.clear();
    emit playbackStateChanged();
    emit error("PANIC STOP");
    
    qDebug() << "PANIC STOP executed";
}

void CueManager::previousCue()
{
    if (!standByCue_) {
        // Set to last cue
        if (!cues_.isEmpty()) {
            standByCue_ = cues_.last().get();
            emit standByCueChanged(standByCue_->id());
        }
        return;
    }
    
    int currentIndex = getCueIndex(standByCue_->id());
    if (currentIndex > 0) {
        standByCue_ = cues_[currentIndex - 1].get();
        emit standByCueChanged(standByCue_->id());
    }
    
    qDebug() << "Previous cue selected";
}

void CueManager::nextCue()
{
    if (!standByCue_) {
        // Set to first cue
        if (!cues_.isEmpty()) {
            standByCue_ = cues_.first().get();
            emit standByCueChanged(standByCue_->id());
        }
        return;
    }
    
    int currentIndex = getCueIndex(standByCue_->id());
    if (currentIndex >= 0 && currentIndex < cues_.size() - 1) {
        standByCue_ = cues_[currentIndex + 1].get();
        emit standByCueChanged(standByCue_->id());
    }
    
    qDebug() << "Next cue selected";
}

// ============================================================================
// Group Operations
// ============================================================================

Cue* CueManager::createGroupFromSelection(const QString& groupName)
{
    if (selectedCueIds_.isEmpty()) {
        emit warning("No cues selected to group");
        return nullptr;
    }

    if (selectedCueIds_.size() < 2) {
        emit warning("Select at least 2 cues to create a group");
        return nullptr;
    }

    // Get the indices of selected cues and sort them
    QList<int> indices;
    for (const QString& id : selectedCueIds_) {
        int idx = getCueIndex(id);
        if (idx >= 0) {
            indices.append(idx);
        }
    }

    if (indices.isEmpty()) {
        return nullptr;
    }

    std::sort(indices.begin(), indices.end());

    // Get the first index - this is where the group will be inserted
    int firstIndex = indices.first();

    // Create the group cue at the first position
    GroupCue* group = static_cast<GroupCue*>(createCue(CueType::Group, firstIndex));
    if (!group) {
        return nullptr;
    }

    group->setName(groupName);

    // Collect the cue IDs to move (work with IDs, not pointers)
    QStringList idsToMove;
    for (int i = 0; i < indices.size(); ++i) {
        int originalIndex = indices[i];
        // Adjust index because we inserted the group
        int adjustedIndex = originalIndex + 1 - i;

        if (adjustedIndex >= 0 && adjustedIndex < cues_.size()) {
            Cue* cue = cues_[adjustedIndex].get();
            if (cue && cue != group) {
                idsToMove.append(cue->id());
            }
        }
    }

    // Now move each cue into the group
    for (const QString& id : idsToMove) {
        int idx = getCueIndex(id);
        if (idx > firstIndex && idx < cues_.size()) {
            // Take ownership of the shared_ptr
            Cue::CuePtr cuePtr = cues_[idx];

            // Remove from main list
            cues_.removeAt(idx);

            // Add to group
            group->addChild(cuePtr);
        }
    }

    // Clear selection and select the new group
    clearSelection();
    selectCue(group->id());

    markUnsaved();
    renumberAllCues();

    emit cueUpdated(group);

    qDebug() << "Created group:" << groupName << "with" << group->childCount() << "children";
    return group;
}

bool CueManager::ungroupCue(const QString& groupCueId)
{
    Cue* cue = getCue(groupCueId);
    if (!cue || cue->type() != CueType::Group) {
        return false;
    }

    GroupCue* group = static_cast<GroupCue*>(cue);
    int groupIndex = getCueIndex(groupCueId);

    if (groupIndex < 0) {
        return false;
    }

    // Get all children first (before modifying anything)
    QList<Cue::CuePtr> children;
    int childCount = group->childCount();

    for (int i = 0; i < childCount; ++i) {
        Cue* child = group->getChildAt(i);
        if (child) {
            // Create a new shared_ptr that shares ownership
            children.append(Cue::CuePtr(child, [](Cue*) {})); // Empty deleter, ownership stays with group temporarily
        }
    }

    // Now we need to properly transfer ownership
    // First, remove the group (this will trigger model update)
    // But keep children alive by temporarily storing shared_ptrs
    QList<Cue::CuePtr> childrenToInsert;
    for (int i = childCount - 1; i >= 0; --i) {
        Cue::CuePtr childPtr = group->removeChild(i);
        if (childPtr) {
            childrenToInsert.prepend(childPtr);
        }
    }

    // Now remove the empty group
    emit cueRemoved(groupCueId);  // Signal FIRST
    cues_.removeAt(groupIndex);   // Then remove

    // Insert all children at the old group position
    for (int i = 0; i < childrenToInsert.size(); ++i) {
        cues_.insert(groupIndex + i, childrenToInsert[i]);
        emit cueAdded(childrenToInsert[i].get(), groupIndex + i);
    }

    markUnsaved();
    renumberAllCues();

    qDebug() << "Ungrouped:" << groupCueId << "with" << childrenToInsert.size() << "children";
    return true;
}

// ============================================================================
// Selection
// ============================================================================

void CueManager::selectCue(const QString& cueId)
{
    clearSelection();
    selectedCueIds_.append(cueId);
    emit selectionChanged();
}

void CueManager::addToSelection(const QString& cueId)
{
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

void CueManager::clearSelection()
{
    if (!selectedCueIds_.isEmpty()) {
        selectedCueIds_.clear();
        emit selectionChanged();
    }
}

void CueManager::selectAll()
{
    selectedCueIds_.clear();
    for (const auto& cuePtr : cues_) {
        if (cuePtr) {
            selectedCueIds_.append(cuePtr->id());
        }
    }
    emit selectionChanged();
}

// ============================================================================
// Clipboard Operations
// ============================================================================

void CueManager::copy()
{
    clipboard_.clear();
    
    if (selectedCueIds_.isEmpty()) {
        return;
    }
    
    // Get actual cue pointers from IDs
    QList<Cue*> selected;
    for (const QString& id : selectedCueIds_) {
        Cue* cue = getCue(id);
        if (cue) {
            selected.append(cue);
        }
    }
    
    if (selected.isEmpty()) {
        return;
    }
    
    // Sort by index
    std::sort(selected.begin(), selected.end(), [this](Cue* a, Cue* b) {
        return getCueIndex(a->id()) < getCueIndex(b->id());
    });
    
    // Convert to JSON
    for (Cue* cue : selected) {
        clipboard_.append(cue->toJson());
    }
    
    qDebug() << "Copied" << clipboard_.size() << "cues to clipboard";
}

void CueManager::cut()
{
    copy();
    
    // Copy the IDs before removing (removeCue modifies selectedCueIds_)
    QStringList ids = selectedCueIds_;
    
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
        } else {
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
// Workspace Management
// ============================================================================

void CueManager::newWorkspace()
{
    cues_.clear();
    selectedCueIds_.clear();
    activeCueIds_.clear();
    clipboard_.clear();
    standByCue_ = nullptr;
    currentWorkspacePath_.clear();
    hasUnsavedChanges_ = false;
    
    emit workspaceCleared();
    emit cueCountChanged(0);
    emit selectionChanged();
    emit standByCueChanged(QString());
    emit unsavedChangesChanged(false);
    
    qDebug() << "New workspace created";
}

bool CueManager::loadWorkspace(const QJsonObject& workspace)
{
    newWorkspace();

    QJsonArray cuesArray = workspace["cues"].toArray();

    // PASS 1: Create all cues (but don't load children yet)
    QList<QPair<Cue*, QJsonObject>> cuesToProcess;

    for (const QJsonValue& value : cuesArray) {
        QJsonObject cueObj = value.toObject();
        QString typeStr = cueObj["type"].toString();
        CueType type = stringToCueType(typeStr);

        Cue* cue = createCue(type);
        if (cue) {
            // Load basic properties only (not children)
            cue->fromJson(cueObj);

            // Store for second pass if it's a group
            if (type == CueType::Group) {
                cuesToProcess.append(qMakePair(cue, cueObj));
            }
        }
    }

    // PASS 2: Now load group children
    for (const auto& pair : cuesToProcess) {
        Cue* groupCue = pair.first;
        QJsonObject groupObj = pair.second;

        if (groupCue->type() == CueType::Group) {
            GroupCue* group = static_cast<GroupCue*>(groupCue);
            QJsonArray childrenArray = groupObj["children"].toArray();

            for (const QJsonValue& childValue : childrenArray) {
                QJsonObject childObj = childValue.toObject();
                QString childTypeStr = childObj["type"].toString();
                CueType childType = stringToCueType(childTypeStr);

                // Create child cue (but don't add to main list)
                Cue* childCue = nullptr;

                switch (childType) {
                case CueType::Audio:
                    childCue = new AudioCue(this);
                    break;
                case CueType::Group:
                    childCue = new GroupCue(this);
                    break;
                case CueType::Wait:
                    childCue = new WaitCue(this);
                    break;
                case CueType::Start:
                case CueType::Stop:
                case CueType::Pause:
                case CueType::Goto:
                    childCue = new ControlCue(childType, this);
                    break;
                default:
                    break;
                }

                if (childCue) {
                    childCue->fromJson(childObj);
                    group->addChild(Cue::CuePtr(childCue));
                }
            }
        }
    }

    // Restore standby cue if saved
    QString standbyCueId = workspace["standbyCue"].toString();
    if (!standbyCueId.isEmpty()) {
        setStandByCue(standbyCueId);
    }

    hasUnsavedChanges_ = false;
    emit unsavedChangesChanged(false);

    qDebug() << "Loaded workspace with" << cues_.size() << "cues";
    return true;
}

QJsonObject CueManager::saveWorkspace() const
{
    QJsonObject workspace;
    
    QJsonArray cuesArray;
    for (const auto& cuePtr : cues_) {
        if (cuePtr) {
            cuesArray.append(cuePtr->toJson());
        }
    }
    
    workspace["cues"] = cuesArray;
    workspace["version"] = "2.0.0";
    
    if (standByCue_) {
        workspace["standbyCue"] = standByCue_->id();
    }
    
    qDebug() << "Saved workspace with" << cues_.size() << "cues";
    return workspace;
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
// Utility Methods
// ============================================================================

void CueManager::renumberAllCues()
{
    for (int i = 0; i < cues_.size(); ++i) {
        if (cues_[i]) {
            cues_[i]->setNumber(QString::number(i + 1));
        }
    }
}

} // namespace CueForge