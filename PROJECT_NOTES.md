# CueForge Qt6 - Project Notes & Architecture

## Architecture Overview

### Core Design Principles

1. **Separation of Concerns**
   - Core logic independent of UI
   - Clean interfaces between layers
   - Testable components

2. **Qt Best Practices**
   - Full use of signals/slots
   - Property system for bindings
   - Meta-object system
   - Smart pointer management

3. **Professional Standards**
   - Memory safety (no raw pointers in ownership)
   - Const-correctness
   - Comprehensive error handling
   - Full serialization support

### Layer Architecture
```
┌─────────────────────────────────────┐
│         UI Layer (Future)           │
│  MainWindow, CueList, Inspector     │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│         Model Layer (Future)        │
│      CueListModel, Delegates        │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│         Core Logic Layer            │
│   CueManager, ErrorHandler          │  ✅ COMPLETE
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│         Domain Layer                │
│   Cue Types, Audio System           │  ✅ 90% COMPLETE
└─────────────────────────────────────┘
```

## Key Classes

### CueManager
**Purpose**: Central orchestration of all cues
**Responsibilities**:
- Cue lifecycle (create, destroy, duplicate)
- Selection management
- Playback control
- Workspace save/load
- Clipboard operations

**Key Methods**:
- `createCue()` - Factory for cue creation
- `go()` - Execute standby cue
- `stop()` - Stop all playback
- `saveWorkspace()` - Serialize to JSON

### Cue (Abstract Base)
**Purpose**: Common interface for all cue types
**Responsibilities**:
- Core properties (number, name, duration)
- Execution interface
- Serialization
- Target system (for control cues)

**Key Methods**:
- `execute()` - Pure virtual, execute cue
- `stop()` - Stop execution
- `toJson()` / `fromJson()` - Serialization
- `clone()` - Deep copy

### ErrorHandler
**Purpose**: Centralized error management
**Responsibilities**:
- Error logging and categorization
- System health monitoring
- Automatic recovery
- Error history and analytics

## Cue Type System

### Type Hierarchy
```
Cue (Abstract Base)
├── AudioCue          ✅ Complete
├── VideoCue          ⏳ Header only
├── GroupCue          ✅ Complete
├── WaitCue           ✅ Complete
├── FadeCue           ⏳ Header only
├── ControlCue        ✅ Complete (base for all below)
│   ├── StartCue      ✅ Via ControlCue
│   ├── StopCue       ✅ Via ControlCue
│   ├── GotoCue       ✅ Via ControlCue
│   ├── PauseCue      ✅ Via ControlCue
│   ├── LoadCue       ✅ Via ControlCue
│   ├── ResetCue      ✅ Via ControlCue
│   ├── ArmCue        ✅ Via ControlCue
│   ├── DisarmCue     ✅ Via ControlCue
│   └── DevampCue     ✅ Via ControlCue
├── MidiCue           ❌ Not started
├── NetworkCue        ❌ Not started
├── LightCue          ❌ Not started
├── TextCue           ❌ Not started
└── MemoCue           ❌ Not started
```

### Cue Type Details

#### AudioCue
- File playback with validation
- Matrix routing system (8x8 crosspoints)
- Volume/pan/rate control
- Trim points and loops
- Gang controls for grouped crosspoints
- Output patch assignment

#### GroupCue
- Contains child cues
- Two modes: Sequential, Parallel
- Recursive structure (groups in groups)
- Automatic duration calculation

#### ControlCue
- Polymorphic design (one class, many types)
- Operates on target cues
- Fade time parameter
- Integrates with CueManager

## Data Flow

### Execution Flow
```
User Action (Go)
    ↓
CueManager::go()
    ↓
Get standBy cue
    ↓
Validate canExecute()
    ↓
CueManager::executeCue()
    ↓
Cue::execute() [virtual]
    ↓
[Cue-specific logic]
    ↓
emit executionStarted
    ↓
[Timer for duration]
    ↓
emit executionFinished
    ↓
CueManager::onCueFinished()
    ↓
Update status, cleanup
    ↓
If continue mode → advance standby
```

### Signal Flow
```
Cue Property Change
    ↓
emit propertyChanged()
    ↓
CueManager catches signal
    ↓
emit cueUpdated()
    ↓
UI refreshes (future)
```

## Serialization

### JSON Structure
```json
{
  "version": "2.0",
  "createdTime": "2025-01-15T10:30:00Z",
  "cueCount": 10,
  "cues": [
    {
      "id": "abc123",
      "type": "Audio",
      "number": "1",
      "name": "Background Music",
      "duration": 120.0,
      "preWait": 0.0,
      "postWait": 0.0,
      "continueMode": true,
      "color": "#64ff96",
      "filePath": "/path/to/audio.wav",
      "volume": 0.8,
      "pan": 0.0,
      "matrixRouting": {
        "0_0": -6.0,
        "1_1": -6.0
      }
    }
  ],
  "expandedGroups": ["group-id-1"],
  "standByCueId": "abc123"
}
```

## Memory Management

### Smart Pointer Strategy
```cpp
// Ownership via shared_ptr
using CuePtr = std::shared_ptr<Cue>;
using CueList = QList<CuePtr>;

// No raw pointer ownership
CueList cues_;  // Manager owns cues

// Raw pointers only for non-owning references
Cue* getCue(const QString& id) const;  // Doesn't transfer ownership
```

### Parent-Child Relationships
```cpp
// Qt parent-child system for signal cleanup
Cue* cue = new AudioCue(manager);  // manager is parent
// When manager destroyed, cue automatically deleted

// Shared pointers for flexible ownership
CuePtr cue = std::make_shared<AudioCue>();
manager->addCue(cue);  // Both have references
```

## Threading Considerations

**Current**: Single-threaded (main thread only)

**Future**:
- Audio playback on separate thread
- File I/O on separate thread
- Network operations on separate thread

**Qt Threading Tools**:
- `QThread` for worker threads
- `moveToThread()` for object threading
- Signals/slots are thread-safe
- `QMetaObject::invokeMethod()` for cross-thread calls

## Performance Optimization

### Already Optimized

1. **Efficient lookups**: Hash-based cue ID lookup
2. **Move semantics**: Used throughout
3. **Const correctness**: Enables compiler optimizations
4. **Reserve/preallocate**: For frequently resized containers

### Future Optimizations

1. **Lazy loading**: Don't load file info until needed
2. **Caching**: Cache serialization results
3. **Object pooling**: Reuse cue objects
4. **Batch updates**: Combine multiple UI updates

## Testing Strategy

### Unit Tests (Future)
```cpp
// Test cue creation
void testCueCreation();

// Test serialization
void testCueSerialization();

// Test playback
void testPlaybackControl();

// Test selection
void testSelection();
```

### Integration Tests (Future)
```cpp
// Test complete workflow
void testWorkspaceLifecycle();

// Test complex scenarios
void testNestedGroups();
```

## Known Limitations

1. **Audio Engine**: Placeholder implementation (just timers)
2. **Video Playback**: Not implemented
3. **MIDI**: Not implemented
4. **UI**: Not started
5. **File Loading**: No actual audio/video decoding yet

## Development Roadmap

### Phase 1: Core Foundation ✅ 90%
- [x] Build system
- [x] Base classes
- [x] CueManager
- [x] 4 cue types
- [x] Error handling
- [ ] Remaining cue type implementations

### Phase 2: UI Layer (Next)
- [ ] MainWindow
- [ ] CueListWidget
- [ ] InspectorWidget
- [ ] MatrixMixerWidget
- [ ] Transport controls
- [ ] Drag & drop

### Phase 3: Integration
- [ ] Audio engine (JUCE or Qt Multimedia)
- [ ] Video playback
- [ ] MIDI implementation
- [ ] Network protocols (OSC/Art-Net)

### Phase 4: Polish
- [ ] Preferences system
- [ ] Keyboard shortcuts
- [ ] Themes
- [ ] Help documentation
- [ ] Installers

## Code Style Guidelines

### Naming
```cpp
// Classes: PascalCase
class AudioCue;

// Methods: camelCase
void executeNextCue();

// Members: camelCase with trailing underscore
QString name_;
double volume_;

// Constants: UPPER_SNAKE_CASE
const int MAX_CUES = 1000;
```

### File Organization
```cpp
// Order in headers:
1. Public types/enums
2. Constructor/destructor
3. Public methods
4. Signals
5. Protected methods
6. Private slots
7. Private methods
8. Private members

// Order in implementation:
1. Constructor
2. Public methods (grouped logically)
3. Signals (don't implement)
4. Protected methods
5. Private slots
6. Private methods
```

### Comments
```cpp
// Use /// for public API documentation
/// Execute the cue
/// @return true if execution started successfully
bool execute();

// Use // for implementation notes
// Calculate total duration including waits
double total = preWait_ + duration_ + postWait_;
```

## Debugging Tips

### Enable Debug Output
```cpp
// In main.cpp
QLoggingCategory::setFilterRules("*.debug=true");
```

### Common Issues

1. **Signals not firing**: Check Q_OBJECT macro
2. **Memory leaks**: Check parent-child relationships
3. **Crashes on exit**: Check signal disconnection
4. **Properties not updating**: Check emit calls

## Resources

- Qt Documentation: https://doc.qt.io/qt-6/
- C++17 Reference: https://en.cppreference.com/
- CMake Documentation: https://cmake.org/documentation/
- JUCE (original): https://juce.com/

---

**Last Updated**: 2025
**Current Phase**: Phase 1 (90% complete)
**Lines of Code**: ~5,200
**Files**: 22