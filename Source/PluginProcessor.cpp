#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>
HaloProcessor::HaloProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "YOUR_HALO_STATE", {
          std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "halo", 1 }, "HALO",
              juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 0.0f,
              juce::AudioParameterFloatAttributes().withLabel("%")),
          std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "mode", 1 }, "MODE",
              juce::StringArray { "AURA", "GLOW", "SHINE", "ASCEND" }, 0) })
{ amount = parameters.getRawParameterValue("halo"); mode = parameters.getRawParameterValue("mode"); setLatencySamples(0); }
bool HaloProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && output == layout.getMainInputChannelSet();
}
void HaloProcessor::prepareToPlay(double rate, int) { engine.prepare(rate, getTotalNumInputChannels()); visualMeter.prepare(rate); }
void HaloProcessor::releaseResources() { engine.reset(); visualMeter.reset(); }
void HaloProcessor::reset() { engine.reset(); visualMeter.reset(); }
void HaloProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch) buffer.clear(ch, 0, buffer.getNumSamples());
    visualMeter.observe(buffer, getTotalNumInputChannels());
    engine.process(buffer, amount->load(std::memory_order_relaxed) * 0.01f,
                   juce::roundToInt(mode->load(std::memory_order_relaxed)));
}
juce::AudioProcessorEditor* HaloProcessor::createEditor() { return new HaloEditor(*this); }
void HaloProcessor::getStateInformation(juce::MemoryBlock& data)
{
    const auto xml = parameters.copyState().createXml();
    copyXmlToBinary(*xml, data);
}
void HaloProcessor::setStateInformation(const void* data, int size)
{
    if (data == nullptr || size <= 0) return;
    if (const auto xml = getXmlFromBinary(data, size))
    {
        if (!xml->hasTagName(parameters.state.getType())) return;
        auto state = juce::ValueTree::fromXml(*xml);
        bool hasHalo = false, hasMode = false;
        for (const auto& child : state)
            if (child["id"].toString() == "halo" || child["id"].toString() == "mode")
            {
                if (!child.hasProperty("value")) return;
                const auto text = child["value"].toString().trim();
                const auto* begin = text.toRawUTF8(); char* end = nullptr;
                const double value = std::strtod(begin, &end);
                if (end == begin || *end != '\0') return;
                const bool isMode = child["id"].toString() == "mode";
                if (!std::isfinite(value) || value < 0 || value > (isMode ? 3.0 : 100.0)
                    || (isMode && std::floor(value) != value)) return;
                if (isMode) { if (hasMode) return; hasMode = true; }
                else { if (hasHalo) return; hasHalo = true; }
            }
        if (!hasHalo) return;
        if (!hasMode)
        {
            juce::ValueTree legacyDefault("PARAM");
            legacyDefault.setProperty("id", "mode", nullptr);
            legacyDefault.setProperty("value", 0.0f, nullptr);
            state.addChild(legacyDefault, -1, nullptr);
        }
        parameters.replaceState(state);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HaloProcessor(); }
