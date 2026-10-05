#pragma once
#include "Metering/HaloAudioMeter.h"
#include <cmath>

namespace voxxlab
{
struct OrbVisualState
{
    float halo = 0.0f, activity = 0.0f, pulseActivity = 0.0f, speedEnergy = 0.0f, hover = 0.0f;
    double phase = 0.0, innerPhase = 0.0, sheenPhase = 0.0, auraPhase = 0.0;
    double specularPhase = 0.0, breathPhase = 0.0;
    float speedMultiplier() const noexcept { return 1.0f + 1.1f * speedEnergy; }
    float radiusScale() const noexcept
    {
        const float idle = 0.005f * static_cast<float>(0.5 + 0.5 * std::sin(breathPhase));
        return 1.0f + idle + 0.039f * pulseActivity * (0.10f + 0.90f * halo);
    }
};

// Message-thread animation model. There is no feedback path to audio or APVTS.
// Time-based smoothing is independent of the timer's actual cadence.
class HaloOrbAnimation
{
public:
    explicit HaloOrbAnimation(float initialHalo = 0.0f) noexcept
    { state.halo = juce::jlimit(0.0f, 1.0f, initialHalo); }
    void advance(double seconds, MeterSnapshot meter, float haloAmount, bool hovered) noexcept
    {
        const double dt = juce::jlimit(0.0, 0.1, seconds);
        if (meter.sequence != lastSequence) { lastSequence = meter.sequence; staleSeconds = 0.0; }
        else staleSeconds += dt;
        const float rms = std::isfinite(meter.rms) ? juce::jlimit(0.0f, 4.0f, meter.rms) : 0.0f;
        const float peak = std::isfinite(meter.peak) ? juce::jlimit(0.0f, 4.0f, meter.peak) : 0.0f;
        const float level = 0.86f * rms + 0.14f * peak;
        const float target = staleSeconds > 0.35 || level < 0.00001f ? 0.0f
            : juce::jlimit(0.0f, 1.0f, (20.0f * std::log10(level) + 52.0f) / 46.0f);
        const auto ease = [dt](float current, float desired, double tau)
        { return desired + (current - desired) * static_cast<float>(std::exp(-dt / tau)); };
        state.activity = ease(state.activity, target, target > state.activity ? 0.065 : 0.32);
        // The shell moves more slowly than the light to soften sudden transients.
        state.pulseActivity = ease(state.pulseActivity, target, target > state.pulseActivity ? 0.15 : 0.38);
        state.speedEnergy = ease(state.speedEnergy, target, target > state.speedEnergy ? 0.25 : 0.70);
        state.halo = ease(state.halo, juce::jlimit(0.0f, 1.0f, haloAmount), 0.06);
        state.hover = ease(state.hover, hovered ? 1.0f : 0.0f, 0.08);
        if (state.activity < 1.0e-6f) state.activity = 0.0f;
        const auto movePhase = [dt](double phase, double speed)
        { return std::fmod(phase + dt * speed, juce::MathConstants<double>::twoPi); };
        // Independent wrapped phases keep every periodic motion continuous at
        // its seam; fractional multiples of one wrapped phase would jump.
        // HALO controls radiance; the separate smoothed input envelope controls
        // circulation. Every layer continues at its base speed in silence.
        const double energy = state.speedMultiplier();
        state.phase = movePhase(state.phase, 0.65 * energy);
        state.innerPhase = movePhase(state.innerPhase, 1.10 * energy);
        state.sheenPhase = movePhase(state.sheenPhase, 1.95 * energy);
        state.auraPhase = movePhase(state.auraPhase, 0.38 * (1.0 + 0.55 * state.speedEnergy));
        state.specularPhase = movePhase(state.specularPhase, 0.45 * (1.0 + 0.25 * state.speedEnergy));
        state.breathPhase = movePhase(state.breathPhase, 0.30);
    }
    const OrbVisualState& getState() const noexcept { return state; }
private:
    OrbVisualState state;
    std::uint32_t lastSequence = 0;
    double staleSeconds = 0.0;
};
}
