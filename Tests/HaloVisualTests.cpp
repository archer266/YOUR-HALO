#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <thread>
#include <chrono>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
void check(bool success, const char* message)
{
    if (!success) throw std::runtime_error(message);
}
std::unique_ptr<juce::AudioPluginInstance> load(juce::VST3PluginFormat& format, const juce::File& bundle,
                                              juce::PluginDescription& description)
{
    juce::OwnedArray<juce::PluginDescription> types;
    format.findAllTypesForFile(types, bundle.getFullPathName());
    check(types.size() == 1, "reference/current VST3 scan failed");
    description = *types[0];
    juce::String error;
    auto instance = format.createInstanceFromDescription(description, 48000, 64, error);
    check(instance != nullptr, "reference/current VST3 load failed");
    return instance;
}
juce::AudioProcessorParameter* haloParameter(juce::AudioProcessor& processor)
{
    for (auto* parameter : processor.getParameters())
        if (parameter->getName(64) == "HALO") return parameter;
    throw std::runtime_error("HALO parameter missing");
}
void setLayout(juce::AudioProcessor& p, int channels)
{
    juce::AudioProcessor::BusesLayout buses;
    auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    buses.inputBuses.add(set); buses.outputBuses.add(set);
    check(p.setBusesLayout(buses), "VST3 layout rejected");
}
float testSignal(int position, double rate, int channel)
{
    const double t = position / rate, twoPi = juce::MathConstants<double>::twoPi;
    const double gain = 0.4 + 0.6 * std::sin(twoPi * 1.7 * t) * std::sin(twoPi * 1.7 * t);
    return static_cast<float>(gain * (0.3 * std::sin(twoPi * 173 * t) + 0.1 * std::sin(twoPi * 3100 * t)
        + 0.13 * std::sin(twoPi * 7300 * t) + (channel == 0 ? 0.05 : -0.05) * std::sin(twoPi * 12300 * t)));
}
void preserveSound(const juce::File& currentBundle, const juce::File& referenceBundle, const juce::File& docs)
{
    juce::VST3PluginFormat format;
    juce::PluginDescription currentDescription, referenceDescription;
    auto reference = load(format, referenceBundle, referenceDescription);
    auto current = load(format, currentBundle, currentDescription);
    check(referenceDescription.uniqueId == currentDescription.uniqueId, "VST3 identifier changed");
    check(referenceDescription.manufacturerName == currentDescription.manufacturerName, "manufacturer changed");
    check(referenceDescription.version == currentDescription.version, "development version changed");
    check(reference->getParameters().size() + 1 == current->getParameters().size(), "MODE was not appended to existing parameters");
    auto* oldHalo = haloParameter(*reference);
    auto* newHalo = haloParameter(*current);
    oldHalo->setValueNotifyingHost(0.37f);
    juce::MemoryBlock legacyState;
    reference->getStateInformation(legacyState);
    current->setStateInformation(legacyState.getData(), static_cast<int>(legacyState.getSize()));
    check(std::abs(newHalo->getValue() - 0.37f) < 0.0001f, "1.0.0 VST3 state compatibility failed");
    // The fixture is captured from the retained release, never from the new build.
    auto fixture = docs.getParentDirectory().getParentDirectory().getChildFile("Tests/Fixtures/HALO-1.0.0-37.vst3state");
    if (!fixture.existsAsFile()) check(fixture.replaceWithData(legacyState.getData(), legacyState.getSize()), "legacy fixture write failed");
    juce::MemoryBlock persistedState;
    check(fixture.loadFileAsData(persistedState), "legacy fixture read failed");
    current->setStateInformation(persistedState.getData(), static_cast<int>(persistedState.getSize()));
    check(std::abs(newHalo->getValue() - 0.37f) < 0.0001f, "persisted 1.0.0 fixture failed");

    juce::MidiBuffer midi;
    std::uint64_t samples = 0;
    double differencePower = 0, referencePower = 0;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        for (int channels : { 1, 2 })
            for (float amount : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                setLayout(*reference, channels); setLayout(*current, channels);
                oldHalo->setValueNotifyingHost(amount); newHalo->setValueNotifyingHost(amount);
                reference->setRateAndBufferSizeDetails(rate, 512); current->setRateAndBufferSizeDetails(rate, 512);
                reference->prepareToPlay(rate, 512); current->prepareToPlay(rate, 512);
                check(current->getLatencySamples() == 0, "VST3 latency changed");
                std::unique_ptr<juce::AudioProcessorEditor> editor;
                int position = 0;
                for (int block = 0; block < 500; ++block)
                {
                    if (block == 50) editor.reset(current->createEditor());
                    if (block == 200) editor.reset();
                    if (block == 300) editor.reset(current->createEditor());
                    if (block == 400) editor.reset();
                    if (block >= 250 && block % 10 == 0)
                    {
                        const float target = static_cast<float>((block * 37) % 101) * 0.01f;
                        oldHalo->setValueNotifyingHost(target); newHalo->setValueNotifyingHost(target);
                    }
                    constexpr int sizes[] { 64, 1, 17, 127, 256, 512 };
                    const int size = sizes[block % 6];
                    juce::AudioBuffer<float> input(channels, size), output(channels, size);
                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < size; ++i)
                            input.setSample(ch, i, block % 67 == 0 ? 0.0f : testSignal(position + i, rate, ch));
                    output.makeCopyOf(input);
                    reference->processBlock(input, midi); current->processBlock(output, midi);
                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < size; ++i)
                        {
                            const float oldSample = input.getSample(ch, i), newSample = output.getSample(ch, i);
                            check(std::isfinite(newSample), "upgraded VST3 audio nonfinite");
                            if (amount == 0 && block < 250) check(oldSample == newSample, "dry endpoint compatibility changed");
                            differencePower += static_cast<double>(newSample - oldSample) * (newSample - oldSample);
                            referencePower += static_cast<double>(oldSample) * oldSample;
                            ++samples;
                        }
                    position += size;
                }
                reference->releaseResources(); current->releaseResources();
            }
    check(differencePower > referencePower * .01, "DSP upgrade remains too close to the subtle original");
    std::cout << "Retained 1.0.0 versus upgraded VST3: intentional audio change, preserved dry endpoint, old state, identifiers and zero latency across 40 settings/rates/layouts ("
              << samples << " samples); normalized output difference " << std::sqrt(differencePower / referencePower) << ": PASS\n";
}
void meterTests()
{
    juce::MidiBuffer midi;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        for (int channels : { 1, 2 })
        {
            HaloProcessor p; setLayout(p, channels); p.prepareToPlay(rate, 64);
            const auto initial = p.getVisualMeters();
            check(initial.rms == 0.0f && initial.peak == 0.0f, "meter not initialised to silence");
            for (int block = 0; block < 1000; ++block)
            {
                juce::AudioBuffer<float> audio(channels, 64);
                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < 64; ++i)
                        audio.setSample(ch, i, 0.3f * static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * 1000 * (block * 64 + i) / rate)));
                p.processBlock(audio, midi);
            }
            const auto active = p.getVisualMeters();
            check(std::abs(active.rms - 0.3f / std::sqrt(2.0f)) < 0.005f, "RMS meter inaccurate");
            check(active.peak > 0.295f && active.peak <= 0.301f, "peak meter inaccurate");
            check(active.sequence != initial.sequence, "meter freshness not advancing");
            juce::AudioBuffer<float> silence(channels, 64);
            for (int block = 0; block < 4000; ++block) { silence.clear(); p.processBlock(silence, midi); }
            const auto idle = p.getVisualMeters();
            check(idle.rms < 1.0e-6f && idle.peak < 1.0e-6f, "audio meter does not decay");
            p.releaseResources();
            check(p.getVisualMeters().rms == 0.0f && p.getVisualMeters().peak == 0.0f, "meter release not cleared");
        }
    std::cout << "Lock-free RMS/peak accuracy, freshness, silence decay and lifecycle at all rates/layouts: PASS\n";
}
HaloOrbComponent& findOrb(juce::AudioProcessorEditor& editor)
{
    for (auto* child : editor.getChildren())
        if (auto* orb = dynamic_cast<HaloOrbComponent*>(child)) return *orb;
    throw std::runtime_error("parameter-attached orb component missing");
}
void writePNG(const juce::Image& image, const juce::File& file)
{
    auto stream = file.createOutputStream();
    check(stream != nullptr, "preview file open failed");
    stream->setPosition(0); stream->truncate();
    check(juce::PNGImageFormat().writeImageToStream(image, *stream), "PNG preview export failed");
}
void feedFrame(HaloProcessor& p, HaloOrbComponent& orb, float scale, int& position)
{
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> audio(2, 64);
    const int frameSamples = 48000 / HaloOrbComponent::refreshRate;
    for (int remaining = frameSamples; remaining > 0; remaining -= 64)
    {
        const int count = juce::jmin(64, remaining);
        audio.setSize(2, count, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < count; ++i) audio.setSample(ch, i, scale * testSignal(position + i, 48000, ch));
        p.processBlock(audio, midi); position += count;
    }
    orb.advanceVisuals(1.0 / HaloOrbComponent::refreshRate, p.getVisualMeters());
}
void visualRenders(const juce::File& docs)
{
    juce::Image sheet(juce::Image::RGB, 1390, 1264, true);
    {
    juce::Graphics sheetGraphics(sheet); sheetGraphics.fillAll(juce::Colour(0xfffaf7ee));
    constexpr float amounts[] { 0, 25, 50, 75, 100 };
    constexpr float levels[] { 0, 0.035f, 0.7f, 1.7f };
    const char* labels[] { "Silence", "Quiet audio", "Normal vocal", "Strong vocal" };
    sheetGraphics.setFont(juce::Font(juce::FontOptions(15.0f)));
    sheetGraphics.setColour(juce::Colour(0xff715c3d));
    for (int column = 0; column < 5; ++column)
        sheetGraphics.drawText("HALO " + juce::String(amounts[column], 0) + "%", 140 + column * 250, 12, 250, 38, juce::Justification::centred);
    for (int row = 0; row < 4; ++row)
    {
        sheetGraphics.setColour(juce::Colour(0xff715c3d));
        sheetGraphics.drawText(labels[row], 8, 64 + row * 300, 124, 300, juce::Justification::centred);
        for (int column = 0; column < 5; ++column)
        {
            HaloProcessor p; p.prepareToPlay(48000, 64);
            p.parameters.getParameter("halo")->setValueNotifyingHost(amounts[column] * 0.01f);
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            auto& orb = findOrb(*editor);
            int position = 0;
    for (int frame = 0; frame < 3 * HaloOrbComponent::refreshRate; ++frame) feedFrame(p, orb, levels[row], position);
            const auto& state = orb.getVisualState();
            check(std::abs(state.halo - amounts[column] * 0.01f) < 0.001f, "orb HALO visual tracking failed");
            check(state.radiusScale() >= 1 && state.radiusScale() < 1.045f, "orb pulse exceeds 4.5 percent");
            if (row == 0) check(state.activity == 0, "silence falsely wakes orb");
            else check(state.activity > 0.05f, "real input audio does not wake orb");
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            const juce::String name = "Orb-HALO-" + juce::String(static_cast<int>(amounts[column])) + "-" + juce::String(row);
            writePNG(image, docs.getChildFile(name + ".png"));
            sheetGraphics.setOpacity(1); sheetGraphics.drawImage(image, { 140.0f + column * 250.0f, 64.0f + row * 300.0f, 250, 300 });
            if (column == 4 && row == 2)
                for (double scale : { 1.0, 1.25, 1.5, 2.0 })
                {
                    const auto hidpi = editor->createComponentSnapshot(editor->getLocalBounds(), true, static_cast<float>(scale));
                    check(hidpi.getWidth() == static_cast<int>(500 * scale) && hidpi.getHeight() == static_cast<int>(600 * scale), "HiDPI snapshot dimensions failed");
                    writePNG(hidpi, docs.getChildFile("Orb-DPI-" + juce::String(static_cast<int>(scale * 100)) + ".png"));
                }
        }
    }
    }
    writePNG(sheet, docs.getChildFile("Orb-contact-sheet.png"));
    const char* modeNames[] { "AURA", "GLOW", "SHINE", "ASCEND" };
    for (int mode = 0; mode < 4; ++mode)
    {
        HaloProcessor processor; processor.prepareToPlay(48000, 64);
        processor.parameters.getParameter("halo")->setValueNotifyingHost(.75f);
        processor.parameters.getParameter("mode")->setValueNotifyingHost(mode / 3.0f);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto& orb = findOrb(*editor); int position = 0;
        for (int frame = 0; frame < 3 * HaloOrbComponent::refreshRate; ++frame) feedFrame(processor, orb, .7f, position);
        writePNG(editor->createComponentSnapshot(editor->getLocalBounds()), docs.getChildFile(juce::String("Mode-") + modeNames[mode] + ".png"));
    }

    HaloProcessor p; p.prepareToPlay(48000, 64);
    p.parameters.getParameter("halo")->setValueNotifyingHost(0.9f);
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    auto& orb = findOrb(*editor);
    int position = 0;
    for (int frame = 0; frame < 2 * HaloOrbComponent::refreshRate; ++frame) feedFrame(p, orb, 0, position);
    const auto frames = docs.getChildFile("AnimationFrames"); frames.createDirectory();
    for (int frame = 0; frame < 24 * HaloOrbComponent::refreshRate; ++frame)
    {
        const double seconds = frame / static_cast<double>(HaloOrbComponent::refreshRate);
        const float audio = seconds < 6 ? 0 : (seconds < 10 ? 0.035f : (seconds < 14 ? 0.7f : (seconds < 18 ? 1.7f : 0)));
        feedFrame(p, orb, audio, position);
        writePNG(editor->createComponentSnapshot(editor->getLocalBounds(), true, 0.72f),
            frames.getChildFile("frame-" + juce::String(frame).paddedLeft('0', 4) + ".png"));
    }
    check(orb.getVisualState().activity < 0.05f, "orb fails to settle after audio ends");
    const double phase = orb.getVisualState().phase;
    for (int frame = 0; frame < 30; ++frame) feedFrame(p, orb, 0, position);
    check(orb.getVisualState().phase != phase, "idle movement stopped");
    // A stopped host can leave the last audio meter unchanged: stale detection
    // must calm the orb even without new processBlock calls.
    for (int frame = 0; frame < 30; ++frame) feedFrame(p, orb, 1, position);
    check(orb.getVisualState().activity > 0.4f, "orb not active before stale-host test");
    const auto stopped = p.getVisualMeters();
    for (int frame = 0; frame < 120; ++frame) orb.advanceVisuals(1.0 / 30, stopped);
    check(orb.getVisualState().activity < 0.01f, "stale host meter holds orb active");
    std::cout << "20 actual-audio visual states, four DPI renders, 24-second/1440-frame five-stage animation, micro-pulse, idle/silence/stopped-host recovery: PASS\n";
}
void interaction()
{
    HaloProcessor p;
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    auto& orb = findOrb(*editor);
    check(p.parameters.getParameter("halo")->paramID == "halo", "HALO ID changed");
    p.parameters.getParameter("halo")->setValueNotifyingHost(0.42f);
    check(std::abs(orb.getValue() - 42) < 0.001, "automation does not update orb");
    orb.setValue(50, juce::sendNotificationSync);
    check(std::abs(p.parameters.getRawParameterValue("halo")->load() - 50) < 0.001, "orb does not update parameter");
    check(orb.keyPressed(juce::KeyPress(juce::KeyPress::upKey)), "orb arrow key ignored");
    check(orb.getValue() > 50, "arrow key does not increase HALO");
    const auto source = juce::Desktop::getInstance().getMainMouseSource();
    const auto event = [&orb, source](juce::Point<float> point, juce::ModifierKeys modifiers, bool dragged, int clicks)
    {
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent(source, point, modifiers, 1.0f, 0, 0, 0, 0, &orb, &orb, now,
            { 230, 150 }, now, clicks, dragged);
    };
    orb.mouseWheelMove(event({ 230, 150 }, {}, false, 0), { 0, 0.2f, false, false, false });
    check(orb.getValue() > 50.01, "mouse wheel ignored");
    orb.setValue(50, juce::sendNotificationSync);
    orb.mouseDown(event({ 230, 150 }, juce::ModifierKeys::leftButtonModifier, false, 1));
    orb.mouseDrag(event({ 230, 98 }, juce::ModifierKeys::leftButtonModifier, true, 1));
    orb.mouseUp(event({ 230, 98 }, {}, true, 1));
    check(orb.getValue() > 55, "vertical drag ignored");
    orb.mouseDoubleClick(event({ 230, 150 }, {}, false, 2));
    check(orb.getValue() == 0 && p.parameters.getRawParameterValue("halo")->load() == 0, "double-click reset failed");
    check(orb.hitTest(230, 150) && !orb.hitTest(0, 0), "orb interactive area invalid");
    juce::Array<juce::TextButton*> buttons;
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child)) buttons.add(button);
    check(buttons.size() == 4, "four mode selectors missing");
    for (int mode = 0; mode < 4; ++mode)
    {
        buttons[mode]->onClick();
        check(p.parameters.getRawParameterValue("mode")->load() == mode, "mode selector does not update parameter");
        for (int i = 0; i < 4; ++i) check(buttons[i]->getToggleState() == (i == mode), "selected mode UI is incorrect");
        const int automated = (mode + 1) % 4;
        p.parameters.getParameter("mode")->setValueNotifyingHost(automated / 3.0f);
        check(buttons[automated]->getToggleState(), "mode automation does not update selection");
    }
    p.parameters.getParameter("mode")->setValueNotifyingHost(0);
    std::cout << "HALO/MODE IDs, automation attachment, mode selectors, arrow keys, wheel, vertical drag, double-click and hit testing: PASS\n";
}
void refinedMotion(const juce::File& docs)
{
    // Follow multiple complete phase wraps under varying input. This catches
    // discontinuities that a short phrase render cannot expose.
    voxxlab::HaloOrbAnimation model(1.0f);
    auto previous = model.getState();
    float maximumScale = 1.0f;
    float maximumSpeed = 1.0f;
    for (int cadence : { 30, 45, 60 })
    {
    model = voxxlab::HaloOrbAnimation(1.0f); previous = model.getState();
    for (std::uint32_t frame = 1; frame <= static_cast<std::uint32_t>(120 * cadence); ++frame)
    {
        const bool singing = (frame / (5 * cadence)) % 2 == 0;
        model.advance(1.0 / cadence, { singing ? 0.8f : 0.0f, singing ? 1.0f : 0.0f, frame }, 1.0f, false);
        const auto state = model.getState();
        const double oldPhases[] { previous.phase, previous.innerPhase, previous.sheenPhase, previous.auraPhase, previous.specularPhase, previous.breathPhase };
        const double newPhases[] { state.phase, state.innerPhase, state.sheenPhase, state.auraPhase, state.specularPhase, state.breathPhase };
        for (int layer = 0; layer < 6; ++layer)
        {
            const double angularStep = std::fmod(newPhases[layer] - oldPhases[layer] + juce::MathConstants<double>::twoPi,
                juce::MathConstants<double>::twoPi);
            check(angularStep > 0 && angularStep < 4.10 / cadence, "swirl motion phase has a discontinuity");
        }
        const float scale = state.radiusScale();
        check(std::isfinite(scale) && scale >= 1 && scale < 1.045f, "refined pulse outside 4.5-percent envelope");
        check(std::abs(scale - previous.radiusScale()) < 0.008f, "refined pulse jumps");
        maximumScale = std::max(maximumScale, scale);
        check(state.speedMultiplier() >= 1 && state.speedMultiplier() <= 2.101f, "audio speed multiplier out of range");
        check(std::abs(state.speedMultiplier() - previous.speedMultiplier()) < 0.14f, "audio speed jumps");
        maximumSpeed = std::max(maximumSpeed, state.speedMultiplier());
        previous = state;
    }
    }
    check(maximumSpeed > 2.09f, "strong input does not accelerate swirling");
    check(maximumScale > 1.035f, "strong vocal fails to reach expressive swell");
    check(std::abs(previous.phase - previous.innerPhase) > 0.05, "internal light phases move in lockstep");
    HaloProcessor p; p.prepareToPlay(48000, 64);
    p.parameters.getParameter("halo")->setValueNotifyingHost(0.75f);
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    auto& orb = findOrb(*editor);
    int position = 0;
    for (int frame = 0; frame < 3 * HaloOrbComponent::refreshRate; ++frame) feedFrame(p, orb, 0, position);
    const auto first = editor->createComponentSnapshot(editor->getLocalBounds());
    for (int frame = 0; frame < 2 * HaloOrbComponent::refreshRate; ++frame) feedFrame(p, orb, 0, position);
    const auto second = editor->createComponentSnapshot(editor->getLocalBounds());
    juce::Image::BitmapData firstPixels(first, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData secondPixels(second, juce::Image::BitmapData::readOnly);
    double difference = 0;
    for (int y = 250; y < 410; ++y)
        for (int x = 170; x < 330; ++x)
        {
            const auto c1 = firstPixels.getPixelColour(x, y), c2 = secondPixels.getPixelColour(x, y);
            difference += std::abs(c1.getRed() - c2.getRed()) + std::abs(c1.getGreen() - c2.getGreen()) + std::abs(c1.getBlue() - c2.getBlue());
        }
    const double meanChange = difference / (160 * 160 * 3);
    std::cout << "Rendered idle inner RGB change: " << meanChange << "/255\n";
    check(meanChange > 4.0, "idle inner-light movement is still too subtle");
    writePNG(first, docs.getChildFile("Idle-motion-A.png"));
    writePNG(second, docs.getChildFile("Idle-motion-B.png"));
    std::cout << "120-second six-phase continuity at 30/45/60 FPS, smooth expressive pulse and visible idle inner-light pixel motion: PASS; maximum expansion "
              << (maximumScale - 1) * 100 << "%; maximum speed " << maximumSpeed << "x; mean idle inner RGB change " << meanChange << "/255\n";
}
void dispatchMessages()
{
    MSG message {};
    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
    { TranslateMessage(&message); DispatchMessage(&message); }
}
void concurrentUI(const juce::File& docs, int cadence)
{
    using Clock = std::chrono::steady_clock;
    HaloProcessor p; p.prepareToPlay(48000, 64);
    p.parameters.getParameter("halo")->setValueNotifyingHost(0.75f);
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    // A real peer is required for JUCE isShowing()/timer behavior. Keep this
    // test window offscreen, without activation or a taskbar entry.
    editor->setTopLeftPosition(-16000, -16000);
    editor->addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowIgnoresMouseClicks
        | juce::ComponentPeer::windowIgnoresKeyPresses);
    editor->setVisible(true);
    auto& orb = findOrb(*editor);
    orb.setVisualRefreshRate(cadence);
    std::atomic<bool> stop { false }, failed { false };
    std::vector<double> callbackMs; callbackMs.reserve(5000);
    std::vector<double> unchangedEngineMs; unchangedEngineMs.reserve(5000);
    const auto start = Clock::now();
    std::thread audio([&]
    {
        juce::ScopedNoDenormals noDenormals;
        voxxlab::HaloEngine reference; reference.prepare(48000, 2);
        juce::AudioBuffer<float> b(2, 64), comparison(2, 64);
        juce::MidiBuffer midi;
        auto next = Clock::now();
        int position = 0;
        while (!stop.load(std::memory_order_relaxed))
        {
            const bool active = std::chrono::duration<double>(Clock::now() - start).count() < 1.3;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 64; ++i)
                {
                    const float input = active ? testSignal(position + i, 48000, ch) : 0;
                    b.setSample(ch, i, input); comparison.setSample(ch, i, input);
                }
            const auto begin = Clock::now(); p.processBlock(b, midi);
            const double duration = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
            if (callbackMs.size() < callbackMs.capacity()) callbackMs.push_back(duration);
            const auto engineBegin = Clock::now(); reference.process(comparison, 0.75f);
            if (unchangedEngineMs.size() < unchangedEngineMs.capacity())
                unchangedEngineMs.push_back(std::chrono::duration<double, std::milli>(Clock::now() - engineBegin).count());
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 64; ++i)
                    if (!std::isfinite(b.getSample(ch, i)) || b.getSample(ch, i) != comparison.getSample(ch, i)) failed.store(true);
            position += 64;
            next += std::chrono::nanoseconds(1333333);
            std::this_thread::sleep_until(next);
        }
    });
    struct Joiner { std::atomic<bool>& flag; std::thread& thread; ~Joiner() { flag.store(true); if (thread.joinable()) thread.join(); } } joiner { stop, audio };
    juce::Image canvas(juce::Image::RGB, 500, 600, true);
    std::vector<double> paints; paints.reserve(220);
    float highestActivity = 0;
    std::uint64_t lastFrame = orb.getFrameCount();
    while (std::chrono::duration<double>(Clock::now() - start).count() < 3.2)
    {
        dispatchMessages();
        if (lastFrame != orb.getFrameCount())
        {
            const auto begin = Clock::now();
            { juce::Graphics graphics(canvas); editor->paintEntireComponent(graphics, false); }
            paints.push_back(std::chrono::duration<double, std::milli>(Clock::now() - begin).count());
            lastFrame = orb.getFrameCount();
            highestActivity = std::max(highestActivity, orb.getVisualState().activity);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    stop.store(true); audio.join();
    check(!failed.load(), "GUI activity changed audio or generated NaN");
    check(paints.size() >= static_cast<size_t>(cadence * 3.2 * 0.75)
        && paints.size() <= static_cast<size_t>(cadence * 3.2 * 1.20), "JUCE timer cadence outside requested FPS range");
    check(highestActivity > 0.4f && orb.getVisualState().activity < 0.08f, "live timer does not wake/calm orb");
    check(!callbackMs.empty(), "concurrent audio did not run");
    std::sort(callbackMs.begin(), callbackMs.end()); std::sort(paints.begin(), paints.end());
    std::sort(unchangedEngineMs.begin(), unchangedEngineMs.end());
    const double medianPaint = paints[paints.size() / 2], p95Paint = paints[paints.size() * 95 / 100];
    const double medianCallback = callbackMs[callbackMs.size() / 2], p95Callback = callbackMs[callbackMs.size() * 95 / 100];
    const double medianEngine = unchangedEngineMs[unchangedEngineMs.size() / 2];
    const double framesPerSecond = paints.size() / 3.2;
    std::cout << "Concurrent 64-sample audio plus real JUCE timer at requested " << cadence << " FPS: PASS, " << framesPerSecond << " FPS; median/p95 full UI offscreen paint "
              << medianPaint << "/" << p95Paint << " ms; callback " << medianCallback << "/" << p95Callback << " ms; upgraded engine median " << medianEngine << " ms\n";
    juce::String metrics;
    metrics << "Measured FPS: " << framesPerSecond << "\nMedian full editor paint ms: " << medianPaint
            << "\nP95 full editor paint ms: " << p95Paint << "\nMedian 64-sample callback ms: " << medianCallback
            << "\nP95 callback ms: " << p95Callback << "\nApproximate core-time share at requested FPS (median paint only): "
            << medianPaint * cadence / 10.0 << "%\nUpgraded engine median ms: " << medianEngine
            << "\nProcessor-minus-engine median ms (meter and wrapper overhead): " << medianCallback - medianEngine
            << "\nOffscreen rendering using JUCE's default image backend; wall-time observations, not a CPU/GPU utilization measurement. Hardware/host scheduling may differ.\n";
    check(docs.getChildFile("UI-performance-" + juce::String(cadence) + ".txt").replaceWithText(metrics), "performance report failed");
    if (cadence == HaloOrbComponent::refreshRate)
        check(docs.getChildFile("UI-performance.txt").replaceWithText(metrics), "selected performance report failed");
}
}

int runVisualTests(const juce::File& current, const juce::File& reference, const juce::File& docs)
{
    try
    {
        docs.createDirectory();
        preserveSound(current, reference, docs);
        const auto previous = docs.getChildFile("PreviousBuild.vst3");
        if (previous.exists()) preserveSound(current, previous, docs);
        meterTests();
        interaction(); visualRenders(docs); refinedMotion(docs);
        for (int cadence : { 30, 45, 60 }) concurrentUI(docs, cadence);
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "VISUAL TEST FAIL: " << error.what() << '\n'; return 1; }
}
