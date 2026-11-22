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
    kParamGrainSize = 3,
    kParamDensity = 4,
    kParamJitter = 5,
    kParamHop = 6,
    kParamGravity = 7,
    kParamFreeze = 8,
    kParamPitch = 9,
    kParamReverse = 10,
    kParamSpread = 11,
    kParamBypass = 12,
    kParamGrainCount = 13,  // output
    kParamReadPos = 14,  // output
    kParamHopHeight = 15,  // output
    kParamGrainPos0 = 16,  // 16 outputs: grain_pos_1 .. grain_pos_16 (kGrainPosCount)
    kParamGrainEnv0 = 32,  // 16 outputs: grain_env_1 .. grain_env_16 (kGrainPosCount)
    kParamCount = 48
};

// Read position (0..1 of the buffer) of grain pool slots 0..15, or -1 when the slot is idle,
// and the level (Hann window, 0..1) of each of those grains: the widget sizes the dots with it
static const uint32_t kGrainPosCount = 16;

enum GroupId : uint32_t {
    kGroupDelay = 0,
    kGroupGrains = 1,
    kGroupHop = 2,
    kGroupVariation = 3,
    kGroupViz = 4,
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
    ParameterDesignation designation;  // kParameterDesignationBypass: 1 = bypassed (LV2 port lv2_enabled)
};


static const GroupDef kGroups[kGroupCount > 0 ? static_cast<uint32_t>(kGroupCount) : 1u] = {
    { "delay", "Delay" },
    { "grains", "Grains" },
    { "hop", "Hop" },
    { "variation", "Variation" },
    { "viz", "Viz" },
};

static const ParamDef kParams[kParamCount] = {
    { "time", "Time", "ms", 10.0f, 2000.0f, 300.0f, kParameterIsAutomatable, 0, nullptr, 0, kParameterDesignationNull },
    { "feedback", "Feedback", "%", 0.0f, 95.0f, 30.0f, kParameterIsAutomatable, 0, nullptr, 0, kParameterDesignationNull },
    { "mix", "Mix", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 0, nullptr, 0, kParameterDesignationNull },
    { "grain_size", "Grain size", "ms", 5.0f, 500.0f, 80.0f, kParameterIsAutomatable, 1, nullptr, 0, kParameterDesignationNull },
    { "density", "Density", "grains/s", 1.0f, 50.0f, 12.0f, kParameterIsAutomatable, 1, nullptr, 0, kParameterDesignationNull },
    { "jitter", "Jitter", "%", 0.0f, 100.0f, 20.0f, kParameterIsAutomatable, 1, nullptr, 0, kParameterDesignationNull },
    { "hop", "Hop", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 2, nullptr, 0, kParameterDesignationNull },
    { "gravity", "Gravity", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 2, nullptr, 0, kParameterDesignationNull },
    { "freeze", "Freeze", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsBoolean, 2, nullptr, 0, kParameterDesignationNull },
    { "pitch", "Pitch", "st", -24.0f, 24.0f, 0.0f, kParameterIsAutomatable, 3, nullptr, 0, kParameterDesignationNull },
    { "reverse", "Reverse", "%", 0.0f, 100.0f, 0.0f, kParameterIsAutomatable, 3, nullptr, 0, kParameterDesignationNull },
    { "spread", "Stereo spread", "%", 0.0f, 100.0f, 50.0f, kParameterIsAutomatable, 3, nullptr, 0, kParameterDesignationNull },
    { "bypass", "Bypass", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsBoolean, 0, nullptr, 0, kParameterDesignationBypass },
    { "grain_count", "Grains", "", 0.0f, 64.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "read_pos", "Read position", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "hop_height", "Hop height", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_1", "Grain 1 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_2", "Grain 2 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_3", "Grain 3 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_4", "Grain 4 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_5", "Grain 5 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_6", "Grain 6 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_7", "Grain 7 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_8", "Grain 8 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_9", "Grain 9 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_10", "Grain 10 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_11", "Grain 11 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_12", "Grain 12 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_13", "Grain 13 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_14", "Grain 14 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_15", "Grain 15 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_pos_16", "Grain 16 position", "", -1.0f, 1.0f, -1.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_1", "Grain 1 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_2", "Grain 2 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_3", "Grain 3 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_4", "Grain 4 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_5", "Grain 5 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_6", "Grain 6 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_7", "Grain 7 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_8", "Grain 8 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_9", "Grain 9 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_10", "Grain 10 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_11", "Grain 11 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_12", "Grain 12 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_13", "Grain 13 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_14", "Grain 14 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_15", "Grain 15 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
    { "grain_env_16", "Grain 16 level", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, 4, nullptr, 0, kParameterDesignationNull },
};

// Bypass parameter (kParamCount if none): Plugin.cpp crossfades to the dry input while bypassed
static const uint32_t kBypassParam = kParamBypass;

END_NAMESPACE_DISTRHO
