#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <new>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

// Catch ordinary C++ heap allocation in the processor's realtime callback.
static thread_local bool trackAllocations = false;
static std::atomic<int> realtimeAllocations { 0 };
void* operator new(std::size_t size)
{
    if (trackAllocations) ++realtimeAllocations;
    if (void* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace
{
int checks = 0;
void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void setHalo(HaloProcessor& p, float percent)
{
    p.parameters.getParameter("halo")->setValueNotifyingHost(percent * 0.01f);
}
void setMode(HaloProcessor& p, int mode)
{ p.parameters.getParameter("mode")->setValueNotifyingHost(mode / 3.0f); }
void layout(juce::AudioProcessor& p, int channels)
{
    juce::AudioProcessor::BusesLayout buses;
    const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    buses.inputBuses.add(set); buses.outputBuses.add(set);
    require(p.setBusesLayout(buses), "mono/stereo layout rejected");
}
float vocal(double time, int channel, float loudness = 1)
{
    const double pi2 = juce::MathConstants<double>::twoPi;
    const double envelope = 0.65 + 0.35 * std::sin(pi2 * 2.3 * time);
    const double side = channel == 0 ? 1.0 : -1.0;
    return loudness * static_cast<float>(envelope * (0.23 * std::sin(pi2 * 180 * time)
        + 0.07 * std::sin(pi2 * 720 * time) + 0.035 * std::sin(pi2 * 3400 * time)
        + 0.025 * std::sin(pi2 * 11000 * time) + side * 0.012 * std::sin(pi2 * 4700 * time)));
}
void matrix()
{
    double worstGain = 0;
    float worstPeak = 0;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        for (int channels : { 1, 2 })
            for (int mode = 0; mode < 4; ++mode)
            for (float amount : { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f })
            {
                HaloProcessor p; layout(p, channels); setHalo(p, amount); setMode(p, mode);
                p.setRateAndBufferSizeDetails(rate, 64); p.prepareToPlay(rate, 64);
                require(p.getLatencySamples() == 0, "latency is not zero");
                double inPower = 0, outPower = 0, mean = 0, difference = 0;
                int position = 0, blockIndex = 0, measured = 0;
                constexpr int sizes[] = { 64, 1, 127, 512, 17, 256 };
                juce::MidiBuffer midi;
                while (position < static_cast<int>(rate * 2))
                {
                    const int size = std::min(sizes[blockIndex++ % 6], static_cast<int>(rate * 2) - position);
                    juce::AudioBuffer<float> b(channels, size), dry(channels, size);
                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < size; ++i)
                            b.setSample(ch, i, vocal((position + i) / rate, ch));
                    dry.makeCopyOf(b); p.processBlock(b, midi);
                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < size; ++i)
                        {
                            const float output = b.getSample(ch, i), input = dry.getSample(ch, i);
                            require(std::isfinite(output), "matrix NaN/Inf");
                            require(std::fpclassify(output) != FP_SUBNORMAL, "matrix denormal");
                            require(std::abs(output) < 1, "matrix clipping");
                            if (amount == 0) require(output == input, "zero is not bit-exact");
                            worstPeak = std::max(worstPeak, std::abs(output));
                            if (position + i > rate)
                            {
                                inPower += input * input; outPower += output * output;
                                mean += output; difference += std::abs(output - input); ++measured;
                            }
                        }
                    position += size;
                }
                const double gain = 10.0 * std::log10(outPower / inPower);
                require(gain > -2.0 && gain < 9.0, "normal vocal RMS change outside usable -2 to +9 dB range");
                require(std::abs(mean / measured) < 0.001, "unexpected vocal DC offset");
                if (amount > 0) require(difference / measured > 0.0001, "HALO makes no change");
                worstGain = std::max(worstGain, std::abs(gain));
                p.releaseResources();
            }
    std::cout << "160 mode/rate/layout/HALO cases, buffers 1/17/64/127/256/512: PASS; max RMS change "
              << worstGain << " dB, peak " << worstPeak << '\n';
}
void silenceAndLoud()
{
    juce::MidiBuffer midi;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        for (int channels : { 1, 2 })
        for (int mode = 0; mode < 4; ++mode)
        {
            HaloProcessor p; layout(p, channels); p.prepareToPlay(rate, 64); setHalo(p, 100); setMode(p, mode);
            juce::AudioBuffer<float> b(channels, 64); b.clear();
            for (int block = 0; block < 100; ++block)
            {
                p.processBlock(b, midi);
                require(b.getMagnitude(0, 64) == 0, "silence generates output");
            }
            for (int block = 0; block < 1500; ++block)
            {
                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < 64; ++i)
                        b.setSample(ch, i, vocal((block * 64 + i) / rate, ch, 5));
                p.processBlock(b, midi);
                require(b.getMagnitude(0, 64) <= 0.991f, "loud input exceeds safety ceiling");
                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < 64; ++i) require(std::isfinite(b.getSample(ch, i)), "loud input nonfinite");
            }
            for (int block = 0; block < 16000; ++block)
            {
                b.clear(); p.processBlock(b, midi);
                if (block > 1500) require(b.getMagnitude(0, 64) < 1.0e-7f, "tail fails to decay");
            }
            for (int ch = 0; ch < channels; ++ch)
                for (int i = 0; i < 64; ++i) require(std::fpclassify(b.getSample(ch, i)) != FP_SUBNORMAL, "silence denormal");
        }
    std::cout << "Silence, overload safety and long tail decay at all rates/layouts: PASS\n";
}
void stereo()
{
    juce::MidiBuffer midi;
    for (int mode = 0; mode < 4; ++mode)
    {
    HaloProcessor mono, stereoProcessor; layout(mono, 1); layout(stereoProcessor, 2);
    mono.prepareToPlay(48000, 64); stereoProcessor.prepareToPlay(48000, 64);
    setHalo(mono, 100); setHalo(stereoProcessor, 100); setMode(mono, mode); setMode(stereoProcessor, mode);
    for (int block = 0; block < 2000; ++block)
    {
        juce::AudioBuffer<float> m(1, 64), s(2, 64);
        for (int i = 0; i < 64; ++i)
        {
            const float v = vocal((block * 64 + i) / 48000.0, 0);
            m.setSample(0, i, v); s.setSample(0, i, v); s.setSample(1, i, v);
        }
        mono.processBlock(m, midi); stereoProcessor.processBlock(s, midi);
        for (int i = 0; i < 64; ++i)
        {
            require(s.getSample(0, i) == s.getSample(1, i), "width created on mono content");
            require(std::abs(s.getSample(0, i) - m.getSample(0, i)) < 2.0e-6f, "dual mono differs from mono");
        }
    }
    stereoProcessor.reset();
    for (int block = 0; block < 1000; ++block)
    {
        juce::AudioBuffer<float> s(2, 64);
        for (int i = 0; i < 64; ++i)
        {
            const float v = vocal((block * 64 + i) / 48000.0, 0);
            s.setSample(0, i, v); s.setSample(1, i, -v);
        }
        stereoProcessor.processBlock(s, midi);
        for (int i = 0; i < 64; ++i) require(std::abs(s.getSample(0, i) + s.getSample(1, i)) < 1.0e-7f, "side leaks into mono sum");
    }
    }
    std::cout << "All four modes: dual-mono consistency and antisymmetric stereo/mono cancellation: PASS\n";
}
std::vector<float> partitioned(int blockSize, int mode = 0)
{
    voxxlab::HaloEngine engine; engine.prepare(48000, 1);
    std::vector<float> output(48000);
    for (int position = 0; position < 48000; position += blockSize)
    {
        const int size = std::min(blockSize, 48000 - position);
        juce::AudioBuffer<float> b(1, size);
        for (int i = 0; i < size; ++i) b.setSample(0, i, vocal((position + i) / 48000.0, 0));
        engine.process(b, 0.75f, mode);
        std::copy_n(b.getReadPointer(0), size, output.begin() + position);
    }
    return output;
}
void automation()
{
    for (int mode = 0; mode < 4; ++mode)
    {
        require(partitioned(1, mode) == partitioned(64, mode), "processing depends on block partition");
        require(partitioned(64, mode) == partitioned(512, mode), "processing depends on large block partition");
    }
    HaloProcessor p; layout(p, 1); p.prepareToPlay(48000, 64);
    float lastInput = 0, lastOutput = 0, maxResidualStep = 0;
    float maximumControlStep = 0, previousAmount = 0;
    voxxlab::HaloEngine transitionEngine; transitionEngine.prepare(48000, 1);
    juce::MidiBuffer midi;
    for (int block = 0; block < 10000; ++block)
    {
        setHalo(p, (block % 20) < 10 ? 100.0f : 0.0f);
        juce::AudioBuffer<float> b(1, 64);
        for (int i = 0; i < 64; ++i)
            b.setSample(0, i, 0.2f * std::sin(static_cast<float>(juce::MathConstants<double>::twoPi * 220 * (block * 64 + i) / 48000)));
        auto dry = b;
        auto uninterrupted = transitionEngine;
        auto reference = b;
        uninterrupted.process(reference, previousAmount);
        auto transition = b;
        const float newAmount = (block % 20) < 10 ? 1.0f : 0.0f;
        transitionEngine.process(transition, newAmount);
        maximumControlStep = std::max(maximumControlStep, std::abs(reference.getSample(0, 0) - transition.getSample(0, 0)));
        previousAmount = newAmount;
        p.processBlock(b, midi);
        for (int i = 0; i < 64; ++i)
        {
            const float input = dry.getSample(0, i), output = b.getSample(0, i);
            maxResidualStep = std::max(maxResidualStep, std::abs((output - lastOutput) - (input - lastInput)));
            require(std::isfinite(output), "automation nonfinite");
            lastInput = input; lastOutput = output;
        }
    }
    require(maximumControlStep < 0.0005f, "HALO automation causes control discontinuity");
    setHalo(p, 0);
    for (int block = 0; block < 100; ++block)
    {
        juce::AudioBuffer<float> b(1, 64); b.clear(); b.setSample(0, 0, 0.1f);
        auto dry = b; p.processBlock(b, midi);
        if (block > 40) for (int i = 0; i < 64; ++i) require(b.getSample(0, i) == dry.getSample(0, i), "return to zero not transparent");
    }
    std::cout << "Partition invariance, rapid automation and return to zero: PASS; control-induced step " << maximumControlStep
              << "; waveform residual derivative (includes intentional gain/tone/distortion) " << maxResidualStep << '\n';
    // Exercise a return to zero with a strongly active level/protection envelope.
    p.reset(); setHalo(p, 100);
    int position = 0;
    for (int block = 0; block < 1500; ++block)
    {
        juce::AudioBuffer<float> b(1, 64);
        for (int i = 0; i < 64; ++i) b.setSample(0, i, 0.55f * static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * 11000 * (position + i) / 48000)));
        p.processBlock(b, midi); position += 64;
    }
    setHalo(p, 0);
    int rampPosition = 0;
    for (int block = 0; block < 40; ++block)
    {
        juce::AudioBuffer<float> b(1, 64);
        for (int i = 0; i < 64; ++i) b.setSample(0, i, 0.55f * static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * 11000 * (position + i) / 48000)));
        auto dry = b; p.processBlock(b, midi);
        for (int i = 0; i < 64; ++i)
            if (rampPosition + i >= 1676) require(std::abs(b.getSample(0, i) - dry.getSample(0, i)) < 0.002f, "bright return-to-zero endpoint jump");
        position += 64; rampPosition += 64;
    }
    std::cout << "Bright-signal return-to-zero endpoint continuity: PASS\n";
}
void stateAndUI(const juce::File& docs)
{
    HaloProcessor p;
    require(p.getParameters().size() == 2, "HALO/MODE parameter count mismatch");
    require(p.parameters.getParameter("halo")->isAutomatable(), "HALO not automatable");
    require(p.parameters.getParameter("mode")->isAutomatable(), "MODE not automatable");
    require(p.parameters.getRawParameterValue("mode")->load() == 0, "default mode not AURA");
    juce::AudioProcessor::BusesLayout invalid;
    invalid.inputBuses.add(juce::AudioChannelSet::mono()); invalid.outputBuses.add(juce::AudioChannelSet::stereo());
    require(!p.isBusesLayoutSupported(invalid), "mismatched layout accepted");
    invalid.inputBuses.set(0, juce::AudioChannelSet::create5point1()); invalid.outputBuses.set(0, juce::AudioChannelSet::create5point1());
    require(!p.isBusesLayoutSupported(invalid), "surround accepted");
    for (int mode = 0; mode < 4; ++mode)
    for (float amount : { 0.0f, 25.0f, 37.0f, 50.0f, 75.0f, 100.0f })
    {
        setHalo(p, amount); setMode(p, mode); juce::MemoryBlock saved; p.getStateInformation(saved);
        HaloProcessor recalled; recalled.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        require(std::abs(recalled.parameters.getRawParameterValue("halo")->load() - amount) < 0.001f, "state recall failed");
        require(recalled.parameters.getRawParameterValue("mode")->load() == mode, "MODE state recall failed");
        recalled.prepareToPlay(96000, 64);
        recalled.prepareToPlay(44100, 512); recalled.releaseResources(); recalled.prepareToPlay(48000, 1);
    }
    auto legacy = p.parameters.copyState();
    for (int i = legacy.getNumChildren() - 1; i >= 0; --i)
        if (legacy.getChild(i)["id"].toString() == "mode") legacy.removeChild(i, nullptr);
    auto legacyXml = legacy.createXml(); juce::MemoryBlock legacyData;
    juce::AudioProcessor::copyXmlToBinary(*legacyXml, legacyData);
    p.setStateInformation(legacyData.getData(), static_cast<int>(legacyData.getSize()));
    require(p.parameters.getRawParameterValue("mode")->load() == 0, "old state fails to default to AURA");
    p.setStateInformation(nullptr, 0); p.setStateInformation("bad data", 8);
    require(p.parameters.getRawParameterValue("halo")->load() == 100, "invalid state changed parameter");
    auto invalidState = p.parameters.copyState(); invalidState.getChild(0).setProperty("value", -1000, nullptr);
    auto xml = invalidState.createXml(); juce::MemoryBlock corrupt;
    juce::AudioProcessor::copyXmlToBinary(*xml, corrupt);
    p.setStateInformation(corrupt.getData(), static_cast<int>(corrupt.getSize()));
    require(p.parameters.getRawParameterValue("halo")->load() == 100, "out-of-range state accepted");
    setHalo(p, 42); setMode(p, 2);
    for (const char* invalidMode : { "-1", "4", "0.5", "nan", "inf", "garbage" })
    {
        auto bad = p.parameters.copyState();
        for (auto child : bad) if (child["id"].toString() == "mode") child.setProperty("value", invalidMode, nullptr);
        auto badXml = bad.createXml(); juce::MemoryBlock invalidData;
        juce::AudioProcessor::copyXmlToBinary(*badXml, invalidData);
        p.setStateInformation(invalidData.getData(), static_cast<int>(invalidData.getSize()));
        require(p.parameters.getRawParameterValue("mode")->load() == 2 && p.parameters.getRawParameterValue("halo")->load() == 42,
                "invalid mode state partially replaced parameters");
    }
    setMode(p, 0);
    docs.createDirectory();
    for (float amount : { 0.0f, 50.0f, 100.0f })
    {
        setHalo(p, amount);
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        require(editor->getWidth() == 500 && editor->getHeight() == 600, "editor dimensions");
        const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
        auto stream = docs.getChildFile("HALO-" + juce::String(static_cast<int>(amount)) + ".png").createOutputStream();
        if (stream != nullptr) { stream->setPosition(0); stream->truncate(); }
        require(stream != nullptr && juce::PNGImageFormat().writeImageToStream(image, *stream), "UI snapshot failed");
    }
    std::cout << "Automation metadata, layouts, state recall, rate changes, invalid state, editor/snapshots: PASS\n";
}
void longPlayback()
{
    HaloProcessor p; p.prepareToPlay(48000, 64);
    juce::MidiBuffer midi; juce::AudioBuffer<float> b(2, 64);
    const auto start = std::chrono::steady_clock::now();
    constexpr int blocks = 48000 * 120 / 64;
    for (int block = 0; block < blocks; ++block)
    {
        if (block % 500 == 0) { setHalo(p, static_cast<float>((block / 500 * 37) % 101)); setMode(p, (block / 500) % 4); }
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i) b.setSample(ch, i, vocal((block * 64 + i) / 48000.0, ch));
        p.processBlock(b, midi);
        require(b.getMagnitude(0, 64) < 1, "long playback overload");
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i) require(std::isfinite(b.getSample(ch, i)), "long playback nonfinite");
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "120 seconds stereo automation: PASS, rendered in " << elapsed << " seconds (includes generator/checks)\n";
}
void robustness()
{
    HaloProcessor p; p.prepareToPlay(48000, 64); setHalo(p, 100);
    juce::AudioBuffer<float> b(2, 64); juce::MidiBuffer midi;
    for (int block = 0; block < 2000; ++block)
    {
        b.clear(); b.setSample(0, 0, 0.25f); b.setSample(1, 0, -0.2f);
        if (block % 100 == 0) { setHalo(p, static_cast<float>(block % 101)); setMode(p, (block / 100) % 4); }
        trackAllocations = true; p.processBlock(b, midi); trackAllocations = false;
    }
    require(realtimeAllocations.load() == 0, "audio callback allocates on C++ heap");
    for (float amount : { 0.0f, 100.0f })
    {
        p.reset(); setHalo(p, amount); b.clear();
        b.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
        b.setSample(0, 1, std::numeric_limits<float>::infinity());
        b.setSample(1, 2, -std::numeric_limits<float>::max());
        p.processBlock(b, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i) require(std::isfinite(b.getSample(ch, i)), "nonfinite input poisons DSP");
    }
    const auto exercise = []
    {
        {
            HaloProcessor instance; instance.prepareToPlay(48000, 64);
            std::unique_ptr<juce::AudioProcessorEditor> editor(instance.createEditor());
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            juce::MemoryBlock saved; instance.getStateInformation(saved);
            instance.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            require(image.isValid(), "lifecycle image invalid");
        }
        // Deliver deferred GUI cleanup like a real host message loop.
        MSG message {};
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        { TranslateMessage(&message); DispatchMessage(&message); }
    };
    for (int i = 0; i < 20; ++i) exercise();
    const auto memoryUse = []
    {
        PROCESS_MEMORY_COUNTERS_EX counters {};
        require(GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)) != 0, "memory counter query failed");
        return counters.PrivateUsage;
    };
    const auto before = memoryUse();
    for (int i = 0; i < 200; ++i) exercise();
    const auto after = memoryUse();
    require(after <= before + 8 * 1024 * 1024, "repeated editor/processor lifecycle memory growth > 8 MiB");
    std::cout << "Realtime heap allocations: 0; nonfinite input recovery: PASS; 200 lifecycles private-memory delta "
              << (static_cast<long long>(after) - static_cast<long long>(before)) << " bytes: PASS\n";
}
double spectralRatio(float scale)
{
    HaloProcessor p; layout(p, 1); p.prepareToPlay(48000, 64); setHalo(p, 100);
    juce::MidiBuffer midi;
    double lowSin = 0, lowCos = 0, highSin = 0, highCos = 0;
    for (int block = 0; block < 3000; ++block)
    {
        juce::AudioBuffer<float> b(1, 64);
        for (int i = 0; i < 64; ++i)
        {
            const double t = (block * 64 + i) / 48000.0;
            b.setSample(0, i, scale * static_cast<float>(0.08 * std::sin(juce::MathConstants<double>::twoPi * 300 * t)
                + 0.2 * std::sin(juce::MathConstants<double>::twoPi * 7000 * t)));
        }
        p.processBlock(b, midi);
        if (block >= 1500)
            for (int i = 0; i < 64; ++i)
            {
                const double t = (block * 64 + i) / 48000.0, sample = b.getSample(0, i);
                lowSin += sample * std::sin(juce::MathConstants<double>::twoPi * 300 * t);
                lowCos += sample * std::cos(juce::MathConstants<double>::twoPi * 300 * t);
                highSin += sample * std::sin(juce::MathConstants<double>::twoPi * 7000 * t);
                highCos += sample * std::cos(juce::MathConstants<double>::twoPi * 7000 * t);
            }
    }
    return std::hypot(highSin, highCos) / std::hypot(lowSin, lowCos);
}
void harshness()
{
    const double quiet = spectralRatio(0.08f), loud = spectralRatio(1.0f);
    require(loud < quiet * 0.92, "strong HF does not receive dynamic protection");
    std::cout << "Strong 7 kHz tone relative to 300 Hz: " << 20 * std::log10(loud / quiet)
              << " dB reduction versus quiet signal: PASS\n";
}
#include "HaloUpgradeChecks.h"
void host(const juce::File& bundle)
{
    require(bundle.isDirectory(), "VST3 bundle not found");
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile(descriptions, bundle.getFullPathName());
    require(descriptions.size() == 1, "VST3 scan did not find one effect");
    require(descriptions[0]->manufacturerName == "VoxxLab", "VST3 manufacturer mismatch");
    require(descriptions[0]->name == "YOUR HALO", "VST3 name mismatch");
    juce::String error;
    auto plugin = format.createInstanceFromDescription(*descriptions[0], 48000, 64, error);
    if (plugin == nullptr) std::cerr << error << '\n';
    require(plugin != nullptr, "VST3 instantiate failed");
    require(plugin->getLatencySamples() == 0, "hosted VST3 latency");
    juce::AudioProcessorParameter* halo = nullptr;
    juce::AudioProcessorParameter* mode = nullptr;
    for (auto* parameter : plugin->getParameters()) { if (parameter->getName(64) == "HALO") halo = parameter; if (parameter->getName(64) == "MODE") mode = parameter; }
    require(halo != nullptr && halo->isAutomatable(), "hosted HALO parameter missing");
    require(mode != nullptr && mode->isAutomatable(), "hosted MODE parameter missing");
    juce::MidiBuffer midi;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    for (int channels : { 1, 2 })
    for (int modeIndex = 0; modeIndex < 4; ++modeIndex)
    for (float amount : { 0.0f, .25f, .5f, .75f, 1.0f })
    {
        layout(*plugin, channels); plugin->setRateAndBufferSizeDetails(rate, 64); plugin->prepareToPlay(rate, 64);
        halo->setValueNotifyingHost(amount); mode->setValueNotifyingHost(modeIndex / 3.0f);
        for (int block = 0; block < 100; ++block)
        {
            juce::AudioBuffer<float> b(channels, 64);
            for (int ch = 0; ch < channels; ++ch)
                for (int i = 0; i < 64; ++i) b.setSample(ch, i, vocal((block * 64 + i) / rate, ch));
            plugin->processBlock(b, midi);
            require(b.getMagnitude(0, 64) < 1, "hosted VST3 clipping");
            for (int ch = 0; ch < channels; ++ch)
                for (int i = 0; i < 64; ++i) require(std::isfinite(b.getSample(ch, i)), "hosted VST3 NaN");
        }
        plugin->releaseResources();
    }
    halo->setValueNotifyingHost(.75f); mode->setValueNotifyingHost(1);
    juce::MemoryBlock saved; plugin->getStateInformation(saved); plugin.reset();
    plugin = format.createInstanceFromDescription(*descriptions[0], 44100, 128, error);
    require(plugin != nullptr, "VST3 reopen failed");
    plugin->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    for (auto* parameter : plugin->getParameters())
        if (parameter->getName(64) == "HALO") require(std::abs(parameter->getValue() - 0.75f) < 0.001f, "hosted state recall failed");
        else if (parameter->getName(64) == "MODE") require(parameter->getValue() == 1.0f, "hosted MODE state recall failed");
    std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditor());
    require(editor != nullptr, "hosted VST3 editor failed");
    editor.reset(); plugin.reset();
    std::cout << "Actual VST3 scan, instantiate, mono/stereo, automation, unload/reopen/state and editor: PASS\n";
}
}
int runVisualTests(const juce::File&, const juce::File&, const juce::File&);
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    juce::ScopedNoDenormals noDenormals;
    if (argc == 5 && juce::String(argv[1]) == "--visual")
        return runVisualTests(juce::File(juce::String(argv[2])), juce::File(juce::String(argv[3])), juce::File(juce::String(argv[4])));
    try
    {
        require(argc == 3, "Usage: HaloTests <VST3 bundle> <snapshot directory>");
        matrix(); silenceAndLoud(); stereo(); automation();
        stateAndUI(juce::File(juce::String(argv[2]))); upgradeChecks(juce::File(juce::String(argv[2])));
        harshness(); robustness(); longPlayback(); host(juce::File(juce::String(argv[1])));
        std::cout << "PASS: " << checks << " assertions\n"; return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL after " << checks << " assertions: " << e.what() << '\n'; return 1; }
}
