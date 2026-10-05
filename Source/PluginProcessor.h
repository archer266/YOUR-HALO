#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/HaloEngine.h"
#include "Metering/HaloAudioMeter.h"
class HaloProcessor final : public juce::AudioProcessor
{
public:
    HaloProcessor();
    const juce::String getName() const override { return "YOUR HALO"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    voxxlab::MeterSnapshot getVisualMeters() const noexcept { return visualMeter.read(); }
private:
    voxxlab::HaloEngine engine;
    voxxlab::HaloAudioMeter visualMeter;
    std::atomic<float>* amount = nullptr;
    std::atomic<float>* mode = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HaloProcessor)
};
