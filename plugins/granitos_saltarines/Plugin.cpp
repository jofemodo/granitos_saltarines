/*
 * Granitos saltarines - DSP
 * Granular delay whose grains hop around the buffer like bouncing balls
 *
 * Copyright (C) 2026 Jofemodo
 * License: GPL-3.0-or-later
 *
 * Algorithm: see SPEC.md (DSP section).
 */

#include "DistrhoPlugin.hpp"
#include "ParamDefs.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

START_NAMESPACE_DISTRHO

namespace GranitosDsp {

constexpr uint32_t kMaxGrains = 64;     // preallocated grain pool
constexpr uint32_t kBounces = 4;        // bounces per grain when hop > 0
constexpr uint32_t kHannSize = 1024;    // window lookup table
constexpr double kBufferSeconds = 2.5;  // time max (2 s) + jitter and hop margin
constexpr double kSmoothSeconds = 0.05; // glide time for time / mix / feedback
constexpr double kMinDelaySamples = 64.0;
constexpr double kPi = 3.14159265358979323846;

// Small xorshift PRNG: RT-safe, no allocation, no locks
struct Rng {
    uint32_t s = 0x9E3779B9u;
    // uniform in [-1, 1)
    float bipolar()
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<float>(s) * (2.0f / 4294967296.0f) - 1.0f;
    }
    // uniform in [0, 1)
    float unit()
    {
        return 0.5f * (bipolar() + 1.0f);
    }
};

struct Grain {
    bool active = false;
    uint32_t age = 0;      // samples since the grain started
    uint32_t len = 1;      // total length in samples
    uint64_t stamp = 0;    // start order, used to steal the oldest grain
    double start = 0.0;    // read index (buffer samples) at age 0
    double rate = 1.0;     // playback rate (pitch)
    double sign = 1.0;     // +1 forward, -1 reverse
    float gainL = 0.0f;    // equal-power pan gains
    float gainR = 0.0f;
    double hopAmp = 0.0;   // max forward shift of the first bounce, in samples
    double r = 0.5;        // bounce decay (from gravity)
    uint32_t bounce = 0;   // current bounce index
    double bStart = 0.0;   // phase where the current bounce starts (0..1)
    double bLen = 1.0;     // phase length of the current bounce
    float bHeight = 0.0f;  // relative height of the current bounce
    float lastHop = 0.0f;  // hop height at the last processed sample (for the widget)
    double lastIdx = 0.0;  // read index at the last processed sample (for the widget)
};

inline double wrapIndex(double idx, double len)
{
    idx = std::fmod(idx, len);
    return idx < 0.0 ? idx + len : idx;
}

} // namespace GranitosDsp

using namespace GranitosDsp;

class GranitosSaltarinesPlugin : public Plugin
{
public:
    GranitosSaltarinesPlugin()
        : Plugin(kParamCount, 0, 0)  // parameters, programs, states
    {
        for (uint32_t i = 0; i < kParamCount; ++i)
            fValues[i] = kParams[i].def;

        for (uint32_t i = 0; i < kHannSize; ++i)
            fHann[i] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * kPi * i / kHannSize));

        if (getSampleRate() > 0.0)
            sampleRateChanged(getSampleRate());
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information

    const char* getLabel() const override { return "granitos_saltarines"; }
    const char* getDescription() const override { return "Granular delay whose grains hop around the buffer like bouncing balls"; }
    const char* getMaker() const override { return "Jofemodo"; }
    const char* getHomePage() const override { return "https://zynthian.org"; }
    const char* getLicense() const override { return "GPL-3.0-or-later"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

    // ----------------------------------------------------------------------------------------------------------------
    // Ports, parameters and groups (from ParamDefs.hpp)

    void initPortGroup(const uint32_t groupId, PortGroup& portGroup) override
    {
        if (groupId < kGroupCount)
        {
            portGroup.name = kGroups[groupId].name;
            portGroup.symbol = kGroups[groupId].symbol;
        }
    }

    void initParameter(const uint32_t index, Parameter& parameter) override
    {
        if (index >= kParamCount)
            return;
        const ParamDef& p(kParams[index]);
        parameter.hints = p.hints;
        parameter.name = p.name;
        parameter.symbol = p.symbol;
        parameter.unit = p.unit;
        parameter.ranges.min = p.min;
        parameter.ranges.max = p.max;
        parameter.ranges.def = p.def;
        parameter.groupId = p.group >= 0 ? static_cast<uint32_t>(p.group) : kPortGroupNone;
    }

    float getParameterValue(const uint32_t index) const override
    {
        return index < kParamCount ? fValues[index] : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount && (kParams[index].hints & kParameterIsOutput) == 0)
            fValues[index] = value;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Processing

    void sampleRateChanged(const double sampleRate) override
    {
        // Everything sized by the sample rate is allocated here, never in run()
        fSampleRate = sampleRate;
        const size_t len = static_cast<size_t>(std::ceil(kBufferSeconds * sampleRate)) + 1;
        fBufL.assign(len, 0.0f);
        fBufR.assign(len, 0.0f);
        fSmoothCoef = static_cast<float>(1.0 - std::exp(-1.0 / (kSmoothSeconds * sampleRate)));
        fReset = true;
    }

    void activate() override
    {
        fReset = true;
    }

    // RT-safe: no allocation, locks, I/O or logging here.
    void run(const float** const inputs, float** const outputs, const uint32_t frames) override
    {
        if (fBufL.empty())
            return;
        if (fReset)
        {
            std::fill(fBufL.begin(), fBufL.end(), 0.0f);
            std::fill(fBufR.begin(), fBufR.end(), 0.0f);
            for (Grain& g : fGrains)
                g.active = false;
            fWetL = fWetR = 0.0f;
            fTime = fValues[kParamTime] * 0.001 * fSampleRate;
            fFeedback = fValues[kParamFeedback] * 0.01f;
            fMix = fValues[kParamMix] * 0.01f;
            fReset = false;
        }

        const double bufLen = static_cast<double>(fBufL.size());

        // Block-rate snapshot of the parameters
        const double targetTime = std::max(kMinDelaySamples, fValues[kParamTime] * 0.001 * fSampleRate);
        const float targetFeedback = fValues[kParamFeedback] * 0.01f;
        const float targetMix = fValues[kParamMix] * 0.01f;
        const float spread = fValues[kParamSpread] * 0.01f;
        const double grainLen = std::max(16.0, fValues[kParamGrainSize] * 0.001 * fSampleRate);
        const double interval = fSampleRate / std::max(1.0f, fValues[kParamDensity]);
        const float jitter = fValues[kParamJitter] * 0.01f;
        const float pitch = fValues[kParamPitch];
        const double hop = fValues[kParamHop] * 0.01;
        const double gravity = fValues[kParamGravity] * 0.01;
        const float pReverse = fValues[kParamReverse] * 0.01f;
        const bool freeze = fValues[kParamFreeze] > 0.5f;

        // Grain decay per bounce: 0 = very bouncy, 1 = no bounce; kept well inside (0, 1)
        const double r = 0.25 + 0.6 * gravity;
        const double bounceC = (1.0 - r) / (1.0 - std::pow(r, kBounces));

        // Output level compensation: the Hann grains sum to about density * grain_length / 2
        const double overlap = std::max(1.0, fValues[kParamDensity] * (grainLen / fSampleRate) * 0.5);
        const float wetNorm = static_cast<float>(1.0 / std::sqrt(overlap));

        const float* const inL = inputs[0];
        const float* const inR = inputs[1];
        float* const outL = outputs[0];
        float* const outR = outputs[1];

        for (uint32_t i = 0; i < frames; ++i)
        {
            // Glide time, mix and feedback so changes don't click
            fTime += (targetTime - fTime) * fSmoothCoef;
            fFeedback += (targetFeedback - fFeedback) * fSmoothCoef;
            fMix += (targetMix - fMix) * fSmoothCoef;

            // Record the input plus feedback, unless frozen
            if (!freeze)
            {
                fBufL[fWrite] = inL[i] + fFeedback * fWetL;
                fBufR[fWrite] = inR[i] + fFeedback * fWetR;
                fWrite = (fWrite + 1) % fBufL.size();
            }

            // Grain scheduler: one grain every 1/density seconds, timing randomised by ±50 %
            fCountdown -= 1.0;
            if (fCountdown <= 0.0)
            {
                spawnGrain(fTime, grainLen, jitter, pitch, spread, pReverse, hop, r, bounceC);
                fCountdown += interval * (1.0 + 0.5 * fRng.bipolar());
            }

            // Sum all active grains
            float wetL = 0.0f, wetR = 0.0f;
            for (Grain& g : fGrains)
            {
                if (!g.active)
                    continue;

                const double phase = static_cast<double>(g.age) / static_cast<double>(g.len);
                const float w = fHann[std::min(kHannSize - 1, static_cast<uint32_t>(phase * kHannSize))];

                // Bouncing ball: advance to the bounce that contains this phase
                while (g.bounce < kBounces && phase >= g.bStart + g.bLen)
                {
                    g.bStart += g.bLen;
                    g.bLen *= g.r;
                    g.bHeight = static_cast<float>(g.bHeight * g.r);
                    ++g.bounce;
                }
                float hopH = 0.0f;
                if (g.bounce < kBounces)
                {
                    const double q = std::min(1.0, std::max(0.0, (phase - g.bStart) / g.bLen));
                    hopH = g.bHeight * static_cast<float>(4.0 * q * (1.0 - q));
                }

                // Read index: start + pitch drift + forward shift of the hop (towards the write head)
                const double idx = g.start + g.sign * g.rate * static_cast<double>(g.age) + g.hopAmp * hopH;
                const double pos = wrapIndex(idx, bufLen);
                const size_t i0 = static_cast<size_t>(pos);
                const size_t i1 = (i0 + 1) % fBufL.size();
                const float frac = static_cast<float>(pos - static_cast<double>(i0));
                const float sL = fBufL[i0] + frac * (fBufL[i1] - fBufL[i0]);
                const float sR = fBufR[i0] + frac * (fBufR[i1] - fBufR[i0]);

                wetL += w * g.gainL * sL;
                wetR += w * g.gainR * sR;

                g.lastIdx = pos;
                g.lastHop = hopH;
                ++g.age;
                if (g.age >= g.len)
                    g.active = false;
            }

            fWetL = wetL * wetNorm;
            fWetR = wetR * wetNorm;

            // Equal-power dry/wet
            const float dryG = std::cos(fMix * static_cast<float>(kPi) * 0.5f);
            const float wetG = std::sin(fMix * static_cast<float>(kPi) * 0.5f);
            outL[i] = dryG * inL[i] + wetG * fWetL;
            outR[i] = dryG * inR[i] + wetG * fWetR;
        }

        // Values for the widget (LV2 output ports): newest active grain is the reference
        uint32_t count = 0;
        const Grain* newest = nullptr;
        for (const Grain& g : fGrains)
        {
            if (!g.active)
                continue;
            ++count;
            if (newest == nullptr || g.stamp > newest->stamp)
                newest = &g;
        }
        fValues[kParamGrainCount] = static_cast<float>(count);
        fValues[kParamReadPos] = newest ? static_cast<float>(newest->lastIdx / bufLen) : 0.0f;
        fValues[kParamHopHeight] = newest ? newest->lastHop : 0.0f;
    }

private:
    void spawnGrain(double delay, double grainLen, float jitter, float pitch, float spread,
                    float pReverse, double hop, double r, double bounceC)
    {
        // Pick a free slot, or steal the oldest grain
        Grain* slot = nullptr;
        for (Grain& g : fGrains)
        {
            if (!g.active)
            {
                slot = &g;
                break;
            }
        }
        if (slot == nullptr)
        {
            slot = &fGrains[0];
            for (Grain& g : fGrains)
                if (g.stamp < slot->stamp)
                    slot = &g;
        }

        const double len = grainLen;
        const bool reverse = fRng.unit() < pReverse;
        const double rate = std::pow(2.0, (pitch + fRng.bipolar() * 12.0 * jitter) / 12.0);
        const double d0 = std::max(kMinDelaySamples, delay + fRng.bipolar() * jitter * 0.5 * len);
        const double pan = 0.5 + 0.5 * fRng.bipolar() * spread;

        Grain& g = *slot;
        g.active = true;
        g.age = 0;
        g.len = static_cast<uint32_t>(len);
        g.stamp = ++fStampCounter;
        g.rate = rate;
        g.sign = reverse ? -1.0 : 1.0;
        // Reverse grains start at their most recent point and read backwards
        g.start = static_cast<double>(fWrite) - d0 + (reverse ? rate * len : 0.0);
        g.gainL = static_cast<float>(std::cos(pan * kPi * 0.5));
        g.gainR = static_cast<float>(std::sin(pan * kPi * 0.5));
        g.hopAmp = hop * 0.5 * d0;
        g.r = r;
        g.bounce = 0;
        g.bStart = 0.0;
        g.bLen = bounceC;
        g.bHeight = 1.0f;
        g.lastHop = 0.0f;
        g.lastIdx = 0.0;
    }

    float fValues[kParamCount];

    double fSampleRate = 48000.0;
    float fSmoothCoef = 0.0f;
    bool fReset = true;

    std::vector<float> fBufL;
    std::vector<float> fBufR;
    size_t fWrite = 0;

    Grain fGrains[kMaxGrains];
    uint64_t fStampCounter = 0;
    double fCountdown = 0.0;
    Rng fRng;

    float fHann[kHannSize];

    double fTime = 0.0;
    float fFeedback = 0.0f;
    float fMix = 0.0f;
    float fWetL = 0.0f;
    float fWetR = 0.0f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranitosSaltarinesPlugin)
};

Plugin* createPlugin()
{
    return new GranitosSaltarinesPlugin();
}

END_NAMESPACE_DISTRHO
