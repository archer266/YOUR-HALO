#include "HaloOrbComponent.h"
#include "PluginProcessor.h"
#include <cmath>

namespace
{
void wash(juce::Graphics& g, juce::Colour colour, juce::Point<float> centre, float radius, float opacity)
{
    g.setGradientFill(juce::ColourGradient(colour.withAlpha(opacity), centre.x, centre.y,
        colour.withAlpha(0.0f), centre.x + radius, centre.y, true));
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2, radius * 2);
}
}

HaloOrbComponent::HaloOrbComponent(HaloProcessor& p)
    : processor(p), animation(p.parameters.getRawParameterValue("halo")->load(std::memory_order_relaxed) * 0.01f)
{
    setName("HALO"); setTitle("HALO amount");
    setDescription("Drag the glowing orb to adjust vocal air, presence and polish from zero to one hundred percent");
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRange(0, 100, 0.01); setDoubleClickReturnValue(true, 0);
    setMouseDragSensitivity(260); setScrollWheelEnabled(true);
    setTextValueSuffix("%"); setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    setTooltip("HALO · Drag vertically or scroll to add glow. Double-click to reset. Arrow keys make fine changes.");
    sphereClip.preallocateSpace(256); auraArc.preallocateSpace(256); highlightPath.preallocateSpace(512);
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    modeParameter = p.parameters.getRawParameterValue("mode");
    startTimerHz(refreshRate);
}
HaloOrbComponent::~HaloOrbComponent() { stopTimer(); }
juce::Point<float> HaloOrbComponent::orbCentre() const noexcept
{ return { static_cast<float>(getWidth()) * 0.5f, static_cast<float>(getHeight()) * 0.45f }; }
float HaloOrbComponent::baseRadius() const noexcept
{ return std::min(static_cast<float>(getWidth()) * 0.29f, static_cast<float>(getHeight()) * 0.35f); }
bool HaloOrbComponent::hitTest(int x, int y)
{
    const auto point = juce::Point<float>(static_cast<float>(x), static_cast<float>(y));
    return point.getDistanceFrom(orbCentre()) < baseRadius() * 1.16f
        || juce::Rectangle<int>(getWidth() / 2 - 55, getHeight() - 67, 110, 53).contains(x, y);
}
void HaloOrbComponent::advanceVisuals(double seconds, voxxlab::MeterSnapshot meter)
{
    animation.advance(seconds, meter, static_cast<float>(getValue()) * 0.01f, isMouseOverOrDragging() || hasKeyboardFocus(true));
    const int mode = juce::jlimit(0, 3, juce::roundToInt(modeParameter->load(std::memory_order_relaxed)));
    const float ease = static_cast<float>(std::exp(-juce::jlimit(0.0, .1, seconds) / .15));
    for (size_t i = 0; i < modeLight.size(); ++i)
        modeLight[i] = (static_cast<int>(i) == mode ? 1.0f : 0.0f) * (1 - ease) + modeLight[i] * ease;
    ++frameCount;
}
void HaloOrbComponent::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = (now - lastTickMs) * 0.001;
    lastTickMs = now;
    if (!isShowing()) return;
    advanceVisuals(dt, processor.getVisualMeters());
    repaint(); // Only the orb's area; static branding stays outside it.
}
void HaloOrbComponent::ensureLightingCache(float pixelScale)
{
    const float diameter = baseRadius() * 2.0f;
    const int pixels = juce::jmax(2, juce::roundToInt(std::ceil(diameter * pixelScale)));
    if (pixels == cachedDiameter) return;
    cachedDiameter = pixels;
    pearlSurface = juce::Image(juce::Image::ARGB, pixels, pixels, true);
    // Cached spherical normals give real key-light falloff, a round terminator,
    // grazing shell reflection and a soft specular lobe. Built only at size/DPI
    // changes on the GUI thread; there is no 3D engine or audio involvement.
    {
        juce::Image::BitmapData bitmap(pearlSurface, juce::Image::BitmapData::writeOnly);
        const float half = static_cast<float>(pixels) * 0.5f;
        for (int y = 0; y < pixels; ++y)
            for (int x = 0; x < pixels; ++x)
            {
                const float nx = (static_cast<float>(x) + 0.5f - half) / half;
                const float ny = (static_cast<float>(y) + 0.5f - half) / half;
                const float radial = nx * nx + ny * ny;
                if (radial >= 1.0f) continue;
                const float nz = std::sqrt(1.0f - radial);
                const float key = std::max(0.0f, -0.43f * nx - 0.55f * ny + 0.715f * nz);
                const float shade = 0.68f + 0.22f * key + 0.14f * nz;
                const float grazing = std::pow(1.0f - nz, 5.0f);
                const float specular = std::pow(std::max(0.0f, -0.23f * nx - 0.29f * ny + 0.93f * nz), 38.0f);
                const float lowerReflection = std::pow(std::max(0.0f, ny), 4.0f) * (0.5f + 0.5f * nz);
                const auto channel = [](float value) { return static_cast<juce::uint8>(juce::jlimit(0, 255, juce::roundToInt(value))); };
                const float r = 250.0f * shade + 28.0f * grazing + 43.0f * specular + 22.0f * lowerReflection;
                const float green = 243.0f * shade + 26.0f * grazing + 46.0f * specular + 28.0f * lowerReflection;
                const float blue = 230.0f * shade + 24.0f * grazing + 52.0f * specular + 36.0f * lowerReflection;
                const float alpha = juce::jlimit(0.0f, 1.0f, (1.0f - std::sqrt(radial)) * half);
                bitmap.setPixelColour(x, y, juce::Colour(channel(r), channel(green), channel(blue)).withAlpha(alpha));
            }
    }
    // Broad, feathered caustic ribbon, cached once and transformed in paint.
    refractionBand = juce::Image(juce::Image::ARGB, pixels, pixels, true);
    {
        juce::Graphics g(refractionBand);
        const float d = static_cast<float>(pixels);
        juce::Path band;
        band.startNewSubPath(-d * 0.08f, d * 0.76f);
        band.cubicTo(d * 0.20f, d * 0.06f, d * 0.43f, d * 0.98f, d * 1.08f, d * 0.26f);
        for (int layer = 14; layer >= 1; --layer)
        {
            const float spread = static_cast<float>(layer);
            g.setColour(juce::Colour(0xffffe6bd).withAlpha(0.010f + 0.0015f * (14.0f - spread)));
            g.strokePath(band, juce::PathStrokeType((12.0f + spread * 5.5f) * pixelScale,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // A feathered aurora veil with no narrow stripe or geometric outline.
        g.setColour(juce::Colour(0xffe3d6f2).withAlpha(0.10f));
        g.strokePath(band, juce::PathStrokeType(30.0f * pixelScale));
        g.setColour(juce::Colour(0xffffffed).withAlpha(0.12f));
        g.strokePath(band, juce::PathStrokeType(18.0f * pixelScale));
    }
    // Rear-hemisphere ribbons are genuinely softer cached images. No blur,
    // image allocation or pixel generation is performed during steady paint.
    softBand = refractionBand.rescaled(juce::jmax(2, pixels / 5), juce::jmax(2, pixels / 5),
                                      juce::Graphics::highResamplingQuality);
    energyCloud = juce::Image(juce::Image::ARGB, pixels, pixels, true);
    coreCloud = juce::Image(juce::Image::ARGB, pixels, pixels, true);
    const float d = static_cast<float>(pixels);
    {
        juce::Graphics g(energyCloud);
        wash(g, juce::Colour(0xffffd68c), { d * 0.27f, d * 0.38f }, d * 0.40f, 0.64f);
        wash(g, juce::Colour(0xfffffae9), { d * 0.55f, d * 0.31f }, d * 0.29f, 0.85f);
        wash(g, juce::Colour(0xffd3bfe7), { d * 0.70f, d * 0.55f }, d * 0.32f, 0.60f);
        wash(g, juce::Colour(0xffcce9ea), { d * 0.56f, d * 0.75f }, d * 0.27f, 0.40f);
        wash(g, juce::Colour(0xfff5c9d5), { d * 0.36f, d * 0.65f }, d * 0.29f, 0.44f);
    }
    {
        juce::Graphics g(coreCloud);
        wash(g, juce::Colour(0xffffe6ad), { d * 0.39f, d * 0.58f }, d * 0.42f, 0.85f);
        wash(g, juce::Colour(0xfffffff2), { d * 0.56f, d * 0.36f }, d * 0.31f, 0.94f);
        wash(g, juce::Colour(0xffffe1d2), { d * 0.66f, d * 0.64f }, d * 0.23f, 0.44f);
    }
    const int bloomPixels = juce::roundToInt(static_cast<float>(pixels) * 1.65f);
    bloom = juce::Image(juce::Image::ARGB, bloomPixels, bloomPixels, true);
    {
        juce::Graphics g(bloom);
        const float half = static_cast<float>(bloomPixels) * 0.5f;
        auto light = juce::ColourGradient(juce::Colour(0xffffdfa0).withAlpha(0.03f), half, half,
            juce::Colour(0xffffdda0).withAlpha(0.0f), half * 2, half, true);
        light.addColour(0.40, juce::Colour(0xffffe4ac).withAlpha(0.40f));
        light.addColour(0.63, juce::Colour(0xffefd18e).withAlpha(0.34f));
        light.addColour(0.81, juce::Colour(0xffffe9ba).withAlpha(0.13f));
        g.setGradientFill(light); g.fillEllipse(0, 0, half * 2, half * 2);
    }
}
void HaloOrbComponent::paint(juce::Graphics& g)
{
    if (getWidth() < 2 || getHeight() < 2) return;
    const auto& visual = animation.getState();
    const float h = visual.halo, a = visual.activity;
    const auto centre = orbCentre();
    const float radius = baseRadius() * visual.radiusScale();
    const auto sphere = juce::Rectangle<float>(radius * 2, radius * 2).withCentre(centre);
    const float light = juce::jmin(1.0f, 0.12f + h * (0.65f + .07f * modeLight[3]) + a * (0.02f + 0.21f * h));
    ensureLightingCache(std::max(1.0f, g.getInternalContext().getPhysicalPixelScaleFactor()));

    // A soft ground reflection anchors the floating object without a hard base.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffbca277).withAlpha(0.065f), centre.x, centre.y + radius + 14,
        juce::Colour(0xffbca277).withAlpha(0.0f), centre.x + radius * 0.86f, centre.y + radius + 14, true));
    g.fillEllipse(centre.x - radius, centre.y + radius + 4, radius * 2, 20);
    wash(g, juce::Colour(0xffffe3a5), centre + juce::Point<float>(0, radius + 6), radius * 0.58f,
         0.025f + 0.065f * h + 0.025f * h * a);
    g.setOpacity(light);
    auto bloomArea = sphere.expanded(radius * (0.57f + a * 0.09f));
    // Let the bloom fade to zero before the component boundary instead of
    // clipping a bright radial gradient against the static header.
    bloomArea = bloomArea.withHeight(std::min(bloomArea.getHeight(), centre.y * 2.0f - 4.0f)).withCentre(centre);
    g.drawImage(bloom, bloomArea, juce::RectanglePlacement::stretchToFit);
    // Atmospheric bloom drifts and deforms independently of the stable shell.
    const float atmosphereAngle = static_cast<float>(visual.auraPhase);
    const float atmosphereScale = bloomArea.getWidth() / static_cast<float>(bloom.getWidth());
    const auto atmosphereCentre = centre + juce::Point<float>(std::cos(atmosphereAngle),
        std::sin(atmosphereAngle)) * radius * 0.07f;
    g.setOpacity(light * (0.22f + 0.18f * a));
    g.drawImageTransformed(bloom, juce::AffineTransform::translation(-bloom.getWidth() * 0.5f, -bloom.getHeight() * 0.5f)
        .scaled(atmosphereScale * 0.95f, atmosphereScale * 0.80f).rotated(atmosphereAngle)
        .translated(atmosphereCentre.x, atmosphereCentre.y));
    g.setOpacity(1);
    g.drawImage(pearlSurface, sphere, juce::RectanglePlacement::stretchToFit);

    sphereClip.clear(); sphereClip.addEllipse(sphere);
    {
        juce::Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(sphereClip);
        const auto composite = [&g, &visual](const juce::Image& image, juce::Point<float> location,
                                    float width, float height, float angle, float opacity)
        {
            g.setOpacity(juce::jlimit(0.0f, 1.0f, opacity));
            g.drawImageTransformed(image, juce::AffineTransform::translation(-image.getWidth() * 0.5f, -image.getHeight() * 0.5f)
                .scaled(width / image.getWidth(), height / image.getHeight())
                .sheared(.24f * static_cast<float>(std::sin(visual.phase + visual.innerPhase)),
                         .17f * static_cast<float>(std::cos(visual.sheenPhase - visual.phase)))
                .rotated(angle).translated(location.x, location.y));
        };
        const float slow = static_cast<float>(visual.phase), medium = static_cast<float>(visual.innerPhase);
        const float fast = static_cast<float>(visual.sheenPhase);
        const float energyVisibility = 0.22f + 0.57f * h + 0.16f * a * h;
        // Tilted orbits project depth onto screen position, size and brightness.
        // Near clouds sharpen/brighten; far clouds merge into the shaded shell.
        for (int layer = 0; layer < 3; ++layer)
        {
            const float theta = layer == 0 ? slow : (layer == 1 ? -medium + 2.1f : fast + 4.2f);
            const float z = std::sin(theta);
            const float front = 0.5f + 0.5f * z;
            const auto position = centre + juce::Point<float>(std::cos(theta) * 0.52f,
                std::sin(theta) * (layer == 1 ? -0.35f : 0.40f)) * radius;
            const float size = radius * (1.02f + 0.22f * front + 0.07f * a);
            composite(energyCloud, position, size, size * (0.78f + 0.14f * std::sin(medium + layer)),
                .90f * std::sin(theta) + .30f * std::cos(medium),
                energyVisibility * (0.25f + 0.70f * front) * (layer == 2 ? 0.75f : 1.0f));
        }
        // A changing luminous core is made of overlapping asymmetric clouds,
        // with different orbits and aspect deformation, rather than one disc.
        composite(coreCloud, centre + juce::Point<float>(std::cos(slow), std::sin(slow)) * radius * 0.21f,
            radius * (1.22f + 0.18f * std::sin(medium)), radius * (1.08f + .12f * std::cos(fast)), .28f * std::sin(medium),
            0.19f + h * 0.49f + h * a * 0.20f);
        composite(coreCloud, centre + juce::Point<float>(std::cos(-medium), std::sin(-medium)) * radius * 0.15f,
            radius * (0.85f + .12f * std::sin(slow)), radius * (0.86f + 0.16f * std::cos(fast)), .23f * std::cos(slow),
            0.16f + h * 0.38f + h * a * 0.23f);
        composite(coreCloud, centre + juce::Point<float>(std::cos(fast), std::sin(fast)) * radius * 0.12f,
            radius * (0.53f + .10f * std::cos(medium)), radius * 0.62f, .30f * std::sin(fast), .10f + h * .28f + h * a * .20f);
        g.setOpacity(1);
        wash(g, juce::Colour(0xffffcc78), centre, radius * .78f, .14f * h * modeLight[1]);
        wash(g, juce::Colour(0xffe2eeff), centre, radius * .75f, .13f * h * modeLight[2]);
        wash(g, juce::Colour(0xfffffff0), centre, radius * .69f, .12f * h * modeLight[3]);
        for (int layer = 0; layer < 2; ++layer)
        {
            const float theta = layer == 0 ? slow : -medium + 1.4f;
            const float front = 0.5f + 0.5f * std::sin(theta);
            const auto location = centre + juce::Point<float>(std::cos(theta) * 0.17f, std::sin(theta) * 0.21f) * radius;
            const float width = radius * (1.84f + 0.10f * std::sin(fast) + 0.10f * a);
            const float height = radius * (0.97f + 0.55f * front + 0.18f * a);
            // Continuous crossfade through depth: broad soft light at the rear,
            // a more defined caustic at the front. No hard hemisphere switch.
            const float flowAngle = .50f * std::sin(theta) + .12f * std::cos(fast);
            composite(softBand, location, width, height, flowAngle,
                energyVisibility * (0.20f + 0.48f * front));
            composite(refractionBand, location, width, height, flowAngle,
                energyVisibility * (0.10f + 0.75f * front * front));
        }
        g.setOpacity(1);
        // Two softly feathered curved speculars imply a thick glass shell.
        const float sheen = static_cast<float>(std::sin(visual.specularPhase));
        highlightPath.clear();
        highlightPath.startNewSubPath(centre.x - radius * 0.70f, centre.y - radius * (0.41f + 0.02f * sheen));
        highlightPath.cubicTo(centre.x - radius * 0.57f, centre.y - radius * 0.91f,
            centre.x - radius * 0.09f, centre.y - radius * 0.94f,
            centre.x + radius * 0.22f, centre.y - radius * (0.76f - 0.025f * sheen));
        highlightPath.applyTransform(juce::AffineTransform::rotation(static_cast<float>(visual.specularPhase), centre.x, centre.y));
        for (int layer = 4; layer >= 1; --layer)
        {
            g.setColour(juce::Colours::white.withAlpha(0.015f + h * 0.018f + h * a * 0.015f));
            g.strokePath(highlightPath, juce::PathStrokeType(1.2f + static_cast<float>(layer) * 2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour(juce::Colour(0xfffffff5).withAlpha(.045f + .04f * h + h * a * .025f));
        g.strokePath(highlightPath, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const auto lowerLight = centre + juce::Point<float>(radius * (0.10f + 0.06f * sheen), radius * 0.75f);
        wash(g, juce::Colour(0xfff1f8ff), lowerLight, radius * 0.32f, 0.12f + 0.14f * h);
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.72f), centre.x - radius * 0.35f, centre.y - radius * 0.60f,
            juce::Colour(0xffc5a66e).withAlpha(0.46f), centre.x + radius * 0.66f, centre.y + radius * 0.66f, false));
        g.drawEllipse(sphere.reduced(1.6f), 1.4f);
        g.setColour(juce::Colour(0xfffff9e9).withAlpha(0.20f + 0.16f * h));
        g.drawEllipse(sphere.reduced(5.0f), 0.8f);
    }

    const float auraBreath = static_cast<float>(0.5 + 0.5 * std::sin(visual.auraPhase));
    const float auraRadius = radius + 11 + 5 * a * h + 1.7f * auraBreath;
    const float auraAlpha = 0.035f + h * 0.23f + h * a * 0.15f + visual.hover * 0.12f;
    // Broad overlapping atmospheric pools replace defined rings/traveling arcs.
    for (int i = 0; i < 2; ++i)
    {
        const float start = static_cast<float>(visual.auraPhase) + static_cast<float>(i) * juce::MathConstants<float>::pi;
        const auto glowCentre = centre + juce::Point<float>(std::cos(start), std::sin(start)) * auraRadius * .74f;
        wash(g, juce::Colour(i == 0 ? 0xffffdfac : 0xffeee6fa), glowCentre, radius * .48f,
             auraAlpha * .12f);
    }
    if (visual.hover > 0.005f)
    {
        g.setColour(juce::Colour(0xffe0c692).withAlpha(visual.hover * 0.25f));
        g.drawEllipse(sphere.expanded(1.5f), 1.5f);
    }
    const auto valueArea = juce::Rectangle<int>(0, getHeight() - 67, getWidth(), 34);
    g.setColour(juce::Colour(0xff715c3d));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI Light", 28.0f, juce::Font::plain)));
    g.drawText(juce::String(juce::roundToInt(getValue())) + "%", valueArea, juce::Justification::centred);
    g.setColour(juce::Colour(0xffa08c69));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 10.0f, juce::Font::plain)));
    g.drawText("H A L O", valueArea.translated(0, 35).withHeight(17), juce::Justification::centred);
}
