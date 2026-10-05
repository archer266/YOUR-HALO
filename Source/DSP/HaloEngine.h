#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
namespace voxxlab
{
// Fixed maximum-boost filters blended with their input. Coefficients never change
// during automation: this avoids time-varying IIR state discontinuities.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void bell(double rate, double frequency, double q, double gainDb);
    void shelf(double rate, double frequency, double gainDb);
    void bandpass(double rate, double frequency, double q);
    void highpass(double rate, double frequency);
    void lowpass(double rate, double frequency);
    float tick(float x) noexcept;
    void reset() noexcept { z1 = z2 = 0; }
};
class HaloEngine
{
public:
    void prepare(double sampleRate, int channels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float amount, int mode = 0) noexcept;
    // Audio-thread telemetry, also used by the offline measurement harness.
    float getLastGainReductionDb() const noexcept { return lastReduction; }
private:
    juce::SmoothedValue<float> halo;
    std::array<juce::SmoothedValue<float>, 4> modeWeights;
    std::array<Biquad, 2> body, cleanup, presence, air, compressorHighpass;
    std::array<Biquad, 2> saturationLowpass, saturationResidue, excitementHighpass, excitementLowpass;
    std::array<Biquad, 2> detector, protection;
    double attackPole = 0, releasePole = 0, bandEnergy = 0, fullEnergy = 0;
    double detectorPole = 0, compressorEnergy = 0, parallelAttack = 0, parallelRelease = 0;
    double rate = 48000;
    float reduction = 0, parallelReduction = 0, lastReduction = 0;
    float protectionMix = 0;
    Biquad sideHighpass;
};
}
