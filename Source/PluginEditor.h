#pragma once
#include "PluginProcessor.h"
#include "UI/HaloOrbComponent.h"
class HaloModeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
};
class HaloEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HaloEditor(HaloProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    HaloOrbComponent orb;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    HaloModeLookAndFeel modeLook;
    std::array<juce::TextButton, 4> modeButtons;
    std::unique_ptr<juce::ParameterAttachment> modeAttachment;
    juce::TooltipWindow tooltip { this, 800 };
    juce::Image logo;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HaloEditor)
};
