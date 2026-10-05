#include "PluginEditor.h"
#include "HaloArtwork.h"
void HaloModeLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour&, bool hover, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(1);
    const bool selected = button.getToggleState();
    g.setGradientFill(juce::ColourGradient(juce::Colour(selected ? 0xfffff4d9 : 0xfffffcf4), bounds.getCentreX(), bounds.getY(),
        juce::Colour(selected ? 0xffead7af : 0xfff2ece0), bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle(bounds, 5);
    g.setColour(juce::Colour(0xffc4aa77).withAlpha(selected ? .8f : (hover || down ? .6f : .28f)));
    g.drawRoundedRectangle(bounds, 5, selected ? 1.0f : .6f);
}
void HaloModeLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 10.0f,
        button.getToggleState() ? juce::Font::bold : juce::Font::plain)));
    g.setColour(juce::Colour(button.getToggleState() ? 0xff715832 : 0xff9a8663));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}
HaloEditor::HaloEditor(HaloProcessor& p) : AudioProcessorEditor(p), orb(p), attachment(p.parameters, "halo", orb)
{
    setName("YOUR HALO · VoxxLab");
    logo = juce::ImageCache::getFromMemory(HaloArtwork::haloLogo_png, HaloArtwork::haloLogo_pngSize);
    addAndMakeVisible(orb);
    const char* names[] { "AURA", "GLOW", "SHINE", "ASCEND" };
    for (size_t i = 0; i < modeButtons.size(); ++i)
    {
        auto& button = modeButtons[i];
        button.setButtonText(names[i]); button.setName(names[i]); button.setLookAndFeel(&modeLook);
        button.setTooltip("Select " + juce::String(names[i]) + " processing character");
        button.onClick = [this, i] { modeAttachment->setValueAsCompleteGesture(static_cast<float>(i)); };
        addAndMakeVisible(button);
    }
    modeAttachment = std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("mode"), [this](float value)
    {
        const int selected = juce::jlimit(0, 3, juce::roundToInt(value));
        for (size_t i = 0; i < modeButtons.size(); ++i)
            modeButtons[i].setToggleState(static_cast<int>(i) == selected, juce::dontSendNotification);
    }, nullptr);
    modeAttachment->sendInitialUpdate();
    setResizable(false, false);
    setSize(500, 600);
}
void HaloEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffffdf8), 250, 0,
                                          juce::Colour(0xfff3eedf), 250, 600, false));
    g.fillRect(area);
    g.setColour(juce::Colours::white.withAlpha(0.8f)); g.drawRect(area.reduced(12), 1.0f);
    g.setColour(juce::Colour(0xffded4bf)); g.drawRect(area.reduced(13), 0.5f);
    g.setColour(juce::Colour(0xff88704d));
    g.setFont(juce::Font(juce::FontOptions("Georgia", 13.0f, juce::Font::plain)));
    g.drawText("Y O U R", 0, 28, getWidth(), 21, juce::Justification::centred);
    if (logo.isValid())
    {
        // Show the original wordmark region, preserving the supplied image and
        // its aspect ratio. The source is embedded; no runtime file access.
        g.setOpacity(0.94f);
        g.drawImage(logo, 69, 49, 362, 115, 0, 280, 1448, 460);
        g.setOpacity(1.0f);
        // Feather the artwork's clouds into the panel with drawing overlays.
        const auto feather = [&g](juce::Colour colour, juce::Rectangle<float> rectangle, juce::Point<float> from, juce::Point<float> to)
        {
            g.setGradientFill(juce::ColourGradient(colour, from.x, from.y, colour.withAlpha(0.0f), to.x, to.y, false));
            g.fillRect(rectangle);
        };
        feather(juce::Colour(0xfffdfbf5), { 69, 49, 362, 22 }, { 250, 49 }, { 250, 71 });
        feather(juce::Colour(0xfffcf9f0), { 69, 145, 362, 19 }, { 250, 164 }, { 250, 145 });
        feather(juce::Colour(0xfffdfbf5), { 69, 49, 19, 115 }, { 69, 100 }, { 88, 100 });
        feather(juce::Colour(0xfffdfbf5), { 412, 49, 19, 115 }, { 431, 100 }, { 412, 100 });
    }
    g.setColour(juce::Colour(0xffa39274));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 10.0f, juce::Font::plain)));
    g.drawText("V O C A L   E N H A N C E R", 0, 164, getWidth(), 18, juce::Justification::centred);
    g.setColour(juce::Colour(0xff827054));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 12.0f, juce::Font::plain)));
    g.drawText("V O X X L A B", 0, 566, getWidth(), 20, juce::Justification::centred);
}
void HaloEditor::resized()
{
    orb.setBounds(20, 155, 460, 390);
    for (size_t i = 0; i < modeButtons.size(); ++i)
        modeButtons[i].setBounds(70 + static_cast<int>(i) * 92, 539, 84, 22);
}
