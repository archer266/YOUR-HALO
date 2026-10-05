#include "HaloEngine.h"
#include <cmath>
namespace voxxlab
{
void Biquad::bell(double rate, double frequency, double q, double gainDb)
{
    const double a = std::pow(10.0, gainDb / 40.0);
    const double w = juce::MathConstants<double>::twoPi * frequency / rate;
    const double alpha = std::sin(w) / (2.0 * q), c = std::cos(w), d = 1.0 + alpha / a;
    b0 = (1.0 + alpha * a) / d; b1 = -2.0 * c / d; b2 = (1.0 - alpha * a) / d;
    a1 = -2.0 * c / d; a2 = (1.0 - alpha / a) / d;
}
void Biquad::shelf(double rate, double frequency, double gainDb)
{
    const double a = std::pow(10.0, gainDb / 40.0);
    const double w = juce::MathConstants<double>::twoPi * frequency / rate;
    const double c = std::cos(w), beta = std::sin(w) * std::sqrt(2.0 * a);
    const double d = (a + 1.0) - (a - 1.0) * c + beta;
    b0 = a * ((a + 1.0) + (a - 1.0) * c + beta) / d;
    b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * c) / d;
    b2 = a * ((a + 1.0) + (a - 1.0) * c - beta) / d;
    a1 = 2.0 * ((a - 1.0) - (a + 1.0) * c) / d;
    a2 = ((a + 1.0) - (a - 1.0) * c - beta) / d;
}
void Biquad::bandpass(double rate, double frequency, double q)
{
    const double w = juce::MathConstants<double>::twoPi * frequency / rate;
    const double alpha = std::sin(w) / (2.0 * q), d = 1.0 + alpha;
    b0 = alpha / d; b1 = 0; b2 = -alpha / d;
    a1 = -2.0 * std::cos(w) / d; a2 = (1.0 - alpha) / d;
}
void Biquad::highpass(double rate, double frequency)
{
    const double w = juce::MathConstants<double>::twoPi * frequency / rate;
    const double c = std::cos(w), alpha = std::sin(w) / std::sqrt(2.0), d = 1.0 + alpha;
    b0 = (1.0 + c) * 0.5 / d; b1 = -(1.0 + c) / d; b2 = b0;
    a1 = -2.0 * c / d; a2 = (1.0 - alpha) / d;
}
float Biquad::tick(float x) noexcept
{
    const double y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y;
    if (std::abs(z1) < 1.0e-24) z1 = 0;
    if (std::abs(z2) < 1.0e-24) z2 = 0;
    return static_cast<float>(y);
}
void Biquad::lowpass(double sampleRate, double frequency)
{
    const double w = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const double c = std::cos(w), alpha = std::sin(w) / std::sqrt(2.0), d = 1.0 + alpha;
    b0 = (1.0 - c) * 0.5 / d; b1 = (1.0 - c) / d; b2 = b0;
    a1 = -2.0 * c / d; a2 = (1.0 - alpha) / d;
}
namespace
{
struct Personality
{
    float body, cleanup, presence, air, ratio, threshold, attack, release;
    float makeup, density, drive, saturation, excitement, width, harshness;
};
constexpr std::array<Personality, 4> personalities {{
    {0.0f, 1.4f, 3.2f, 3.8f, 3.0f, -24.0f, 22, 120, 6.0f, .22f, 3.4f, .36f, .10f, .13f, 5.5f},
    {2.5f, .4f, 1.8f, 1.8f, 3.5f, -25.0f, 30, 160, 6.5f, .35f, 4.8f, .58f, .04f, .08f, 4.5f},
    {0.0f, 2.0f, 4.5f, 6.0f, 2.7f, -23.0f, 12, 85, 5.5f, .18f, 3.0f, .28f, .24f, .20f, 7.5f},
    {1.0f, 1.0f, 4.0f, 5.5f, 4.0f, -27.0f, 16, 100, 8.0f, .50f, 4.4f, .52f, .20f, .30f, 8.0f}
}};
float softKneeReduction(float level, float threshold, float ratio) noexcept
{
    constexpr float knee = 6.0f;
    const float over = level - threshold, slope = 1.0f - 1.0f / ratio;
    if (over <= -knee * 0.5f) return 0;
    if (over >= knee * 0.5f) return slope * over;
    return slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
}
float gain(float db) noexcept { return std::pow(10.0f, db * 0.05f); }
}
void HaloEngine::prepare(double sampleRate, int)
{
    rate = sampleRate;
    halo.reset(rate, 0.035);
    for (auto& weight : modeWeights) weight.reset(rate, 0.060);
    detectorPole = std::exp(-1.0 / (0.008 * rate));
    attackPole = std::exp(-1.0 / (0.002 * rate));
    releasePole = std::exp(-1.0 / (0.080 * rate));
    parallelAttack = std::exp(-1.0 / (0.015 * rate));
    parallelRelease = std::exp(-1.0 / (0.180 * rate));
    for (auto& f : body) f.bell(rate, 240, .75, 3.0);
    for (auto& f : cleanup) f.bell(rate, 380, .8, -2.5);
    for (auto& f : presence) f.bell(rate, std::min(3400.0, rate * .2), .7, 5.0);
    for (auto& f : air) f.shelf(rate, std::min(9500.0, rate * .3), 6.5);
    for (auto& f : compressorHighpass) f.highpass(rate, 90);
    for (auto& f : saturationLowpass) f.lowpass(rate, 4200);
    for (auto& f : saturationResidue) f.lowpass(rate, std::min(10500.0, rate * .36));
    for (auto& f : excitementHighpass) f.highpass(rate, 3500);
    for (auto& f : excitementLowpass) f.lowpass(rate, std::min(11000.0, rate * .36));
    for (auto& f : detector) f.bandpass(rate, std::min(7000.0, rate * .25), .75);
    for (auto& f : protection) f.shelf(rate, std::min(5500.0, rate * .25), -8.0);
    sideHighpass.highpass(rate, 300);
    reset();
}
void HaloEngine::reset()
{
    halo.setCurrentAndTargetValue(0);
    for (size_t m = 0; m < modeWeights.size(); ++m) modeWeights[m].setCurrentAndTargetValue(m == 0 ? 1.0f : 0.0f);
    compressorEnergy = bandEnergy = fullEnergy = 0;
    reduction = parallelReduction = lastReduction = protectionMix = 0;
    for (auto* bank : { &body, &cleanup, &presence, &air, &compressorHighpass, &saturationLowpass,
                        &saturationResidue, &excitementHighpass, &excitementLowpass, &detector, &protection })
        for (auto& f : *bank) f.reset();
    sideHighpass.reset();
}
void HaloEngine::process(juce::AudioBuffer<float>& buffer, float amount, int mode) noexcept
{
    halo.setTargetValue(std::isfinite(amount) ? juce::jlimit(0.0f, 1.0f, amount) : 0.0f);
    mode = juce::jlimit(0, 3, mode);
    for (int m = 0; m < 4; ++m) modeWeights[static_cast<size_t>(m)].setTargetValue(m == mode ? 1.0f : 0.0f);
    const int channels = std::min(2, buffer.getNumChannels());
    // Mode-dependent poles are interpolated, while EQ coefficients remain fixed.
    // Every filter keeps its history, even at HALO zero; nothing resets on switch.
    std::array<double, 4> attacks {}, releases {};
    for (size_t m = 0; m < personalities.size(); ++m)
    {
        attacks[m] = std::exp(-1.0 / (personalities[m].attack * .001 * rate));
        releases[m] = std::exp(-1.0 / (personalities[m].release * .001 * rate));
    }
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float h = halo.getNextValue();
        const float dynamics = std::pow(h, .90f), toneCurve = std::pow(h, 1.10f);
        const float airCurve = std::pow(h, 1.35f), satCurve = std::pow(h, 1.15f);
        Personality p {}; double mainAttack = 0, mainRelease = 0;
        for (size_t m = 0; m < personalities.size(); ++m)
        {
            const float w = modeWeights[m].getNextValue();
            const auto& v = personalities[m];
            p.body += w * v.body; p.cleanup += w * v.cleanup; p.presence += w * v.presence;
            p.air += w * v.air; p.ratio += w * v.ratio; p.threshold += w * v.threshold;
            p.makeup += w * v.makeup; p.density += w * v.density; p.drive += w * v.drive;
            p.saturation += w * v.saturation; p.excitement += w * v.excitement;
            p.width += w * v.width; p.harshness += w * v.harshness;
            mainAttack += w * attacks[m]; mainRelease += w * releases[m];
        }
        std::array<float, 2> dry {}, wet {}; double linkedPower = 0, bandPower = 0, fullPower = 0;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<size_t>(ch); const float input = buffer.getSample(ch, i);
            dry[c] = std::isfinite(input) ? juce::jlimit(-32.0f, 32.0f, input) : 0;
            const float shaped = dry[c] + toneCurve * p.body / 3.0f * (body[c].tick(dry[c]) - dry[c]);
            wet[c] = shaped + toneCurve * p.cleanup / 2.5f * (cleanup[c].tick(shaped) - shaped);
            const double sidechain = compressorHighpass[c].tick(wet[c]);
            linkedPower = std::max(linkedPower, sidechain * sidechain);
            const double band = detector[c].tick(dry[c]);
            bandPower = std::max(bandPower, band * band);
            fullPower = std::max(fullPower, static_cast<double>(dry[c]) * dry[c]);
        }
        compressorEnergy = detectorPole * compressorEnergy + (1.0 - detectorPole) * linkedPower;
        if (compressorEnergy < 1.0e-24) compressorEnergy = 0;
        const float level = static_cast<float>(10.0 * std::log10(std::max(1.0e-12, compressorEnergy)));
        const float target = softKneeReduction(level, -10.0f + dynamics * (p.threshold + 10.0f),
                                              1.0f + dynamics * (p.ratio - 1.0f));
        const double mainPole = target > reduction ? mainAttack : mainRelease;
        reduction = static_cast<float>(mainPole * reduction + (1.0 - mainPole) * target);
        const float parallelTarget = softKneeReduction(level, -30.0f, 6.0f);
        const double parallelPole = parallelTarget > parallelReduction ? parallelAttack : parallelRelease;
        parallelReduction = static_cast<float>(parallelPole * parallelReduction + (1.0 - parallelPole) * parallelTarget);
        if (reduction < 1.0e-12f) reduction = 0;
        if (parallelReduction < 1.0e-12f) parallelReduction = 0;
        // Fade residual envelopes to unity with HALO, including a return to zero.
        const float levelGain = gain(std::pow(h, 1.20f) * p.makeup - dynamics * reduction)
            + p.density * std::pow(h, 1.10f) * gain(8.0f - parallelReduction);
        lastReduction = dynamics * reduction;
        const float drive = 1.0f + satCurve * (p.drive - 1.0f);
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<size_t>(ch);
            const float levelled = wet[c] * levelGain;
            const float low = saturationLowpass[c].tick(levelled);
            const float residual = saturationResidue[c].tick(std::tanh(drive * low) / drive - low);
            const float enriched = (levelled + p.saturation * satCurve * residual) * (1.0f + .12f * satCurve);
            const float present = enriched + toneCurve * p.presence / 5.0f * (presence[c].tick(enriched) - enriched);
            const float bright = present + airCurve * p.air / 6.5f * (air[c].tick(present) - present);
            const float high = excitementHighpass[c].tick(bright);
            const float sparkle = excitementLowpass[c].tick(high - std::tanh(2.8f * high) / 2.8f);
            wet[c] = bright + p.excitement * airCurve * sparkle;
        }
        const auto follow = [this](double value, double previous)
        {
            const double pole = value > previous ? attackPole : releasePole;
            const double result = pole * previous + (1.0 - pole) * value;
            return result < 1.0e-24 ? 0.0 : result;
        };
        bandEnergy = follow(bandPower, bandEnergy); fullEnergy = follow(fullPower, fullEnergy);
        const double ratio = std::sqrt(bandEnergy / std::max(1.0e-12, fullEnergy));
        const float severity = static_cast<float>(juce::jlimit(0.0, 1.0, (std::sqrt(bandEnergy) - .025) / .10)
            * juce::jlimit(0.0, 1.0, (ratio - .22) / .42));
        const double protectionPole = severity > protectionMix ? attackPole : releasePole;
        protectionMix = static_cast<float>(protectionPole * protectionMix + (1.0 - protectionPole) * severity);
        if (protectionMix < 1.0e-12f) protectionMix = 0;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<size_t>(ch);
            wet[c] += toneCurve * p.harshness / 8.0f * protectionMix * (protection[c].tick(wet[c]) - wet[c]);
        }
        if (channels == 2)
        {
            const float side = .5f * (wet[0] - wet[1]);
            const float extraSide = p.width * std::pow(h, 1.6f) * sideHighpass.tick(side);
            wet[0] += extraSide; wet[1] -= extraSide;
        }
        float peak = 0;
        for (int ch = 0; ch < channels; ++ch) peak = std::max(peak, std::abs(wet[static_cast<size_t>(ch)]));
        const float knee = 1.0f - .15f * h, margin = std::max(1.0e-9f, .10f * h);
        const float safety = peak > knee ? (knee + margin * std::tanh((peak - knee) / margin)) / peak : 1;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<size_t>(ch);
            buffer.setSample(ch, i, h == 0 ? dry[c] : wet[c] * safety);
        }
    }
}
}
