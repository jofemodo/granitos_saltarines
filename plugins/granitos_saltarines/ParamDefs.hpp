/*
 * Granitos saltarines - parameters and port groups, shared by DSP (Plugin.cpp) and GUI (UI.cpp).
 * Symbols are stable API (presets, snapshots, MIDI learn, zynthian widget): don't rename them lightly.
 * Control pages in zynthian = port groups (4 controls per page).
 */

#pragma once

#include "DistrhoDetails.hpp"

START_NAMESPACE_DISTRHO

enum ParamId : uint32_t {
    kParamTime = 0,
    kParamFeedback = 1,
    kParamMix = 2,
    kParamSpread = 3,
    kParamGrainSize = 4,
    kParamDensity = 5,
    kParamPitch = 6,
    kParamJitter = 7,
    kParamHop = 8,
    kParamGravity = 9,
    kParamReverse = 10,
    kParamFreeze = 11,
    kParamGrainCount = 12,  // output
    kParamReadPos = 13,  // output
    kParamHopHeight = 14,  // output
    kParamCount
};

enum GroupId : uint32_t {
    kGroupDelay = 0,
    kGroupGrains = 1,
    kGroupHop = 2,
    kGroupViz = 3,
    kGroupCount
};

struct GroupDef {
    const char* symbol;
    const char* name;
};

struct ParamDef {
    const char* symbol;
    const char* name;
    const char* unit;
    float min, max, def;
    uint32_t hints;
    int32_t group;  // GroupId or -1
    const char* const* enumLabels;
    uint32_t enumCount;
};


static const GroupDef kGroups[kGroupCount > 0 ? static_cast<uint32_t>(kGroupCount) : 1u] = {
    { "delay", "Delay" },
    { "grains", "Grains" },
    { "hop", "Hop" },
    { "viz", "Viz" },
};

static const ParamDef kParams[kParamCount] = {
    { "time", "Time", "ms", 10.0f, 2000.0f, 300.0f, kParameterIsAutomatable, 0, nullptr, 0 },
    { "feedback", "Feedback", "%", 0.0f, 95.0f, 30.0f, kParameterIsAutomatable, 0, nullptr, 0 },
    { "mix", "Mix", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 0, nullptr, 0 },
    { "spread", "Stereo spread", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 0, nullptr, 0 },
    { "grain_size", "Grain size", "ms", 5.0f, 500.0f, 80.0f, kParameterIsAutomatable, 1, nullptr, 0 },
    { "density", "Density", "grains/s", 1.0f, 50.0f, 12.0f, kParameterIsAutomatable, 1, nullptr, 0 },
    { "pitch", "Pitch", "st", -24.0f, 24.0f, 0.0f, kParameterIsAutomatable, 1, nullptr, 0 },
    { "jitter", "Jitter", "%", 0.0f, 100.0f, 20.0f, kParameterIsAutomatable, 1, nullptr, 0 },
    { "hop", "Hop", "%", 0.0f, 100.0f, 0.0f, kParameterIsAutomatable, 2, nullptr, 0 },
    { "gravity", "Gravity", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 2, nullptr, 0 },
    { "reverse", "Reverse", "%", 0.0f, 100.0f, 0.0f, kParameterIsAutomatable, 2, nullptr, 0 },
    { "freeze", "Freeze", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsBoolean, 2, nullptr, 0 },
    { "grain_count", "Grains", "", 0.0f, 64.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 3, nullptr, 0 },
    { "read_pos", "Read position", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 3, nullptr, 0 },
    { "hop_height", "Hop height", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 3, nullptr, 0 },
};

// Outputs filled by the demo meters in Plugin.cpp (peak dB of the audio outputs)
static const uint32_t kMeterCount = 0;
static const uint32_t kMeterParams[kMeterCount > 0 ? kMeterCount : 1] = { 0 };

END_NAMESPACE_DISTRHO
