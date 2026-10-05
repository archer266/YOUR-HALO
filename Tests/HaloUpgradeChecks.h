// Included inside the regression harness namespace to reuse its assertions and
// fixture generator. Measurements are reproducible synthetic audio, not LUFS.
void upgradeChecks(const juce::File& docs)
{
    const char* names[] { "AURA", "GLOW", "SHINE", "ASCEND" };
    juce::String csv("Mode,HALO,RMS_change_dB,peak_change_dB,mean_main_GR_dB,max_main_GR_dB,crest_change_dB,gain_normalized_residual,quiet_vs_loud_gain_dB\n");
    std::array<std::vector<float>, 4> modeAudio;
    for (int mode = 0; mode < 4; ++mode)
    for (float amount : { .5f, 1.0f })
    {
        voxxlab::HaloEngine engine; engine.prepare(48000, 1);
        juce::AudioBuffer<float> wave(1, 480000), dry(1, 480000);
        double inPower = 0, outPower = 0, cross = 0, gr = 0, quietIn = 0, quietOut = 0, loudIn = 0, loudOut = 0;
        float inPeak = 0, outPeak = 0, maxGr = 0; int samples = 0, blocks = 0;
        for (int position = 0; position < 480000; position += 64)
        {
            juce::AudioBuffer<float> b(1, 64);
            for (int i = 0; i < 64; ++i)
            {
                const double t = (position + i) / 48000.0;
                const float strength = t < 2 ? .7f : (t < 4 ? .08f : (t < 6 ? 1.8f : (t < 8 ? .25f : 1.0f)));
                const float input = vocal(t, 0, strength);
                b.setSample(0, i, input); dry.setSample(0, position + i, input);
            }
            engine.process(b, amount, mode);
            wave.copyFrom(0, position, b, 0, 0, 64);
            if (position >= 48000)
            {
                gr += engine.getLastGainReductionDb(); ++blocks;
                maxGr = std::max(maxGr, engine.getLastGainReductionDb());
                for (int i = 0; i < 64; ++i)
                {
                    const float x = dry.getSample(0, position + i), y = b.getSample(0, i);
                    inPower += x * x; outPower += y * y; cross += x * y;
                    inPeak = std::max(inPeak, std::abs(x)); outPeak = std::max(outPeak, std::abs(y)); ++samples;
                    const double t = (position + i) / 48000.0;
                    if (t >= 3 && t < 4) { quietIn += x * x; quietOut += y * y; }
                    if (t >= 5 && t < 6) { loudIn += x * x; loudOut += y * y; }
                }
            }
        }
        const double rmsChange = 10 * std::log10(outPower / inPower);
        const double peakChange = 20 * std::log10(outPeak / inPeak);
        const double residual = std::sqrt(std::max(0.0, outPower - cross * cross / inPower) / outPower);
        const double density = 10 * std::log10((quietOut / quietIn) / (loudOut / loudIn));
        require(outPeak <= 1.0f - .05f * amount + .00001f && std::isfinite(rmsChange), "A/B levels uncontrolled");
        require(residual > .035, "mode only adds gain without sufficient tone/dynamic change");
        require(density > .5, "quiet details not denser relative to loud phrases");
        require(maxGr > .5f, "mode compressor never becomes meaningful");
        csv << names[mode] << ',' << amount * 100 << ',' << rmsChange << ',' << peakChange << ','
            << gr / blocks << ',' << maxGr << ',' << peakChange - rmsChange << ',' << residual << ',' << density << '\n';
        std::cout << names[mode] << " " << amount * 100 << "%: RMS " << rmsChange << " dB, peak " << peakChange
            << " dB, mean/max GR " << gr / blocks << '/' << maxGr << " dB, crest " << peakChange - rmsChange
            << " dB, gain-normalized residual " << residual << ", quiet/loud density " << density << " dB\n";
        const auto write = [](const juce::AudioBuffer<float>& audio, const juce::File& file)
        {
            auto stream = file.createOutputStream(); require(stream != nullptr, "A/B WAV open failed");
            stream->setPosition(0); stream->truncate();
            juce::WavAudioFormat format;
            std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(), 48000, 1, 24, {}, 0));
            require(writer != nullptr && writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()), "A/B WAV write failed");
        };
        write(wave, docs.getChildFile(juce::String(names[mode]) + "-" + juce::String(static_cast<int>(amount * 100)) + ".wav"));
        if (mode == 0 && amount == .5f) write(dry, docs.getChildFile("Dry.wav"));
        if (amount == 1)
            modeAudio[static_cast<size_t>(mode)].assign(wave.getReadPointer(0), wave.getReadPointer(0) + wave.getNumSamples());
    }
    require(docs.getChildFile("Loudness.csv").replaceWithText(csv), "loudness report write failed");
    for (size_t a = 0; a < 4; ++a)
    for (size_t b = a + 1; b < 4; ++b)
    {
        double aa = 0, bb = 0, ab = 0;
        for (size_t i = 48000; i < modeAudio[a].size(); ++i)
        { aa += modeAudio[a][i] * modeAudio[a][i]; bb += modeAudio[b][i] * modeAudio[b][i]; ab += modeAudio[a][i] * modeAudio[b][i]; }
        const double difference = std::sqrt(std::max(0.0, bb - ab * ab / aa) / bb);
        require(difference > .035, "two modes differ only in gain or are too similar");
        std::cout << "Gain-normalized " << names[a] << '/' << names[b] << " difference: " << difference << ": PASS\n";
    }
    // Isolate the first sample at a mode switch from the underlying waveform.
    float worstSwitch = 0;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    for (int channels : { 1, 2 })
    for (int from = 0; from < 4; ++from)
    for (int to = 0; to < 4; ++to)
    {
        voxxlab::HaloEngine engine; engine.prepare(rate, channels);
        for (int block = 0; block < 500; ++block)
        {
            juce::AudioBuffer<float> b(channels, 64);
            for (int ch = 0; ch < channels; ++ch)
                for (int i = 0; i < 64; ++i) b.setSample(ch, i, vocal((block * 64 + i) / rate, ch));
            engine.process(b, 1, from);
        }
        auto uninterrupted = engine;
        juce::AudioBuffer<float> switched(channels, 64), reference(channels, 64);
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < 64; ++i) switched.setSample(ch, i, vocal((32000 + i) / rate, ch));
        reference.makeCopyOf(switched); engine.process(switched, 1, to); uninterrupted.process(reference, 1, from);
        for (int ch = 0; ch < channels; ++ch)
            worstSwitch = std::max(worstSwitch, std::abs(switched.getSample(ch, 0) - reference.getSample(ch, 0)));
    }
    require(worstSwitch < .0005f, "mode switch produces an instantaneous pop");
    std::cout << "All 16 mode transitions across four rates and mono/stereo, first-sample control discontinuity: " << worstSwitch << ": PASS\n";
    juce::String performance("Rate,Mode,median_callback_us,p95_callback_us,thread_CPU_duty_percent\n");
    const auto threadMs = []
    {
        FILETIME creation {}, exit {}, kernel {}, user {};
        require(GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user) != 0, "thread CPU timing unavailable");
        ULARGE_INTEGER k {}, u {}; k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
        return static_cast<double>(k.QuadPart + u.QuadPart) * .0001;
    };
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    for (int mode = 0; mode < 4; ++mode)
    {
        HaloProcessor processor; processor.prepareToPlay(rate, 64); setHalo(processor, 100); setMode(processor, mode);
        juce::AudioBuffer<float> b(2, 64), source(2, 64); juce::MidiBuffer midi;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i) source.setSample(ch, i, vocal(i / rate, ch));
        for (int warm = 0; warm < 300; ++warm) { b.makeCopyOf(source, true); processor.processBlock(b, midi); }
        std::vector<double> timings; timings.reserve(8000);
        const double startCPU = threadMs();
        for (int block = 0; block < 8000; ++block)
        {
            b.makeCopyOf(source, true);
            const auto start = std::chrono::steady_clock::now();
            processor.processBlock(b, midi);
            timings.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count());
        }
        const double cpuMs = threadMs() - startCPU;
        std::sort(timings.begin(), timings.end());
        const double duty = cpuMs / (8000.0 * 64 / rate * 1000) * 100;
        performance << rate << ',' << names[mode] << ',' << timings[4000] << ',' << timings[7600] << ',' << duty << '\n';
        require(timings[7600] < 64.0 / rate * 1000000 * .5, "audio callback approaches its realtime deadline");
    }
    require(docs.getChildFile("CPU.csv").replaceWithText(performance), "CPU report write failed");
    std::cout << "DSP-only full-macro CPU/callback benchmarks: all four modes at four rates: PASS\n";
}
