#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "HaloOrbAnimation.h"

class HaloProcessor;
class HaloOrbComponent final : public juce::Slider, private juce::Timer
{
public:
    explicit HaloOrbComponent(HaloProcessor&);
    ~HaloOrbComponent() override;
    void paint(juce::Graphics&) override;
    bool hitTest(int x, int y) override;
    // The same update used by the timer also supports deterministic rendering
    // and verification without injecting fake values into the audio meter.
    void advanceVisuals(double seconds, voxxlab::MeterSnapshot);
    const voxxlab::OrbVisualState& getVisualState() const noexcept { return animation.getState(); }
    std::uint64_t getFrameCount() const noexcept { return frameCount; }
    static constexpr int refreshRate = 60;
    // Allows the rendering harness to compare supported cadences; GUI only.
    void setVisualRefreshRate(int rate) { startTimerHz(juce::jlimit(30, 60, rate)); }
private:
    void timerCallback() override;
    void ensureLightingCache(float pixelScale);
    juce::Point<float> orbCentre() const noexcept;
    float baseRadius() const noexcept;
    HaloProcessor& processor;
    voxxlab::HaloOrbAnimation animation;
    juce::Image pearlSurface, bloom, refractionBand, softBand, energyCloud, coreCloud;
    int cachedDiameter = 0;
    juce::Path sphereClip, auraArc, highlightPath;
    std::uint64_t frameCount = 0;
    double lastTickMs = 0.0;
    std::atomic<float>* modeParameter = nullptr;
    std::array<float, 4> modeLight { 1, 0, 0, 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HaloOrbComponent)
};
