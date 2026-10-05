#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>

namespace voxxlab
{
struct MeterSnapshot
{
    float rms = 0.0f;
    float peak = 0.0f;
    std::uint32_t sequence = 0;
};

// A read-only observer of input audio, outside the enhancement chain. Audio is
// the sole writer; the GUI only loads independent, lock-free scalar values.
// Slightly different sample ages in a GUI snapshot are harmless for light motion.
class HaloAudioMeter
{
public:
    void prepare(double sampleRate) noexcept
    {
        powerPole = std::exp(-1.0 / (0.025 * sampleRate));
        peakPole = std::exp(-1.0 / (0.12 * sampleRate));
        reset();
    }
    void reset() noexcept
    {
        power = heldPeak = 0.0;
        rmsValue.store(0.0f, std::memory_order_relaxed);
        peakValue.store(0.0f, std::memory_order_relaxed);
        sequence.fetch_add(1, std::memory_order_relaxed);
    }
    void observe(const juce::AudioBuffer<float>& input, int inputChannels) noexcept
    {
        const int channels = juce::jlimit(0, 2, std::min(inputChannels, input.getNumChannels()));
        if (channels == 0 || input.getNumSamples() == 0) return;
        std::array<const float*, 2> samples {};
        for (int ch = 0; ch < channels; ++ch) samples[static_cast<size_t>(ch)] = input.getReadPointer(ch);
        for (int i = 0; i < input.getNumSamples(); ++i)
        {
            double samplePower = 0.0, samplePeak = 0.0;
            for (int ch = 0; ch < channels; ++ch)
            {
                const float raw = samples[static_cast<size_t>(ch)][i];
                const double level = std::isfinite(raw) ? std::min(4.0, std::abs(static_cast<double>(raw))) : 0.0;
                samplePower += level * level;
                samplePeak = std::max(samplePeak, level);
            }
            power = powerPole * power + (1.0 - powerPole) * samplePower / channels;
            heldPeak = std::max(samplePeak, heldPeak * peakPole);
        }
        if (power < 1.0e-24) power = 0.0;
        if (heldPeak < 1.0e-12) heldPeak = 0.0;
        rmsValue.store(static_cast<float>(std::sqrt(power)), std::memory_order_relaxed);
        peakValue.store(static_cast<float>(heldPeak), std::memory_order_relaxed);
        sequence.fetch_add(1, std::memory_order_relaxed);
    }
    MeterSnapshot read() const noexcept
    {
        return { rmsValue.load(std::memory_order_relaxed), peakValue.load(std::memory_order_relaxed),
                 sequence.load(std::memory_order_relaxed) };
    }
private:
    static_assert(std::atomic<float>::is_always_lock_free, "Visual meters must be lock-free");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free, "Visual freshness counter must be lock-free");
    std::atomic<float> rmsValue { 0.0f }, peakValue { 0.0f };
    std::atomic<std::uint32_t> sequence { 0 };
    double powerPole = 0.0, peakPole = 0.0, power = 0.0, heldPeak = 0.0;
};
}
