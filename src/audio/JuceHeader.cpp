// ============================================================================
// JuceHeader.cpp - JUCE compilation symbols
// CueForge Qt6 - Professional show control software
// ============================================================================

#include <juce_core/juce_core.h>

// JUCE requires these symbols for build tracking
namespace juce
{
    const char* juce_compilationDate = __DATE__;
    const char* juce_compilationTime = __TIME__;
}

// Note: juce_isRunningUnderDebugger is already defined in juce_core.cpp
// We don't need to redefine it here