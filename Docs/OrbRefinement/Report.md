# YOUR HALO orb refinement

Visual refinement completed and validated on Windows x64, October 4, 2026. The official embedded HALO artwork remains unchanged. This is a development build, retaining version 1.0.0 and all existing identifiers; the original release package remains in `dist`.

## Changed files

- `Source/UI/HaloOrbComponent.cpp`: cached spherical lighting, dimensional shell, layered inner core, drifting pearl light, curved refractive bands, surface highlights, lower reflection and softer animated aura.
- `Source/UI/HaloOrbComponent.h`: lighting caches and reusable highlight path.
- `Source/UI/HaloOrbAnimation.h`: four independent motion phases and a separately smoothed expansion envelope.
- `Tests/HaloVisualTests.cpp`: updated expansion bound; two-minute phase/pulse continuity check and real rendered idle-motion check.
- `README.md` and this documentation folder: updated build pointers, captures, previews and validation evidence.

## Visual result and motion

The sphere now has shaded edge falloff, a bright warm core, upper-left curved key highlights, an illuminated glass rim and subtle reflected light below. Muted lavender, cyan and peach pools move inside the pearl/champagne palette. Two translucent curved bands shift independently to suggest light traveling through a thick shell. Layered soft bloom and faint traveling aura arcs respond to input and HALO without adding controls or overlays.

The timer remains at 30 FPS. Four separately wrapped phases avoid discontinuities when a cycle wraps. At maximum HALO their idle-to-full-input speeds are 0.60–1.22, 0.43–0.79, 0.32–0.55 and 0.26–0.46 radians/second respectively. Internal pools, bands, sheen and aura therefore move at different rates, accelerating smoothly with audio.

Maximum radius expansion increased from about 2.55% to 4.4%, including a 0–0.5% slow idle breath. Size uses 150 ms attack / 380 ms release smoothing; light activity retains 65 ms attack / 320 ms release. Sudden full input stays below a 0.8% radius step per frame in the 30 FPS continuity test. The two-minute test reached 4.4% expansion. Two silent captures two seconds apart show a mean internal RGB change of 1.55544/255, confirming idle light movement without audio.

## Build and validation

Release build succeeded for the VST3 and test executable. Core CTest passed (1/1, 4.33 seconds; 40,401,140 assertions). The separately run extended visual/host suite passed:

- Actual retained 1.0.0 versus refined VST3: **4,869,360 bit-identical audio samples**, across 40 setting/rate/layout combinations plus automation, silence, buffer variations and editor open/close/reopen.
- Original state recall, parameter identity, plugin identifiers, automation and zero reported latency preserved.
- VST3 scan, instantiate, mono/stereo processing, automation, unload/reopen, state and editor checks passed in the JUCE host harness.
- Mouse drag, wheel, arrow keys, double-click reset, attachment and hit testing passed.
- 20 real-audio visual states, four DPI render sizes, 120 animation frames, silence and stopped-host recovery passed.
- Two-minute four-phase continuity, smooth bounded expansion and rendered internal idle motion passed.
- Concurrent 64-sample processing and actual GUI timer passed with no crashes or detected repaint/lifecycle failures. Core realtime heap-allocation check remained zero.

The [preservation audit](Preservation-audit.json) confirms byte-identical DSP, processor, metering, editor, CMake, logo and original release binary/ZIP against pre-refinement hashes. The visual changes have no feedback path to audio or parameter state.

## Performance

| Measurement | Previous orb | Refined orb |
|---|---:|---:|
| Observed GUI FPS | approximately 30 | 29.375 |
| Median full-editor offscreen paint | 1.9485 ms | 2.1470 ms |
| Refined p95 paint | — | 3.2501 ms |
| Median 64-sample audio callback | 4.9 µs | 4.9 µs |
| Refined p95 audio callback | — | 6.9 µs |
| Unchanged engine median | 4.2 µs | 4.2 µs |

The median render cost rose about 0.20 ms (10.2%). At 30 FPS this paint-only wall-time estimate is 6.44% of one core, compared with 5.85% before. It is not a measured CPU/GPU utilization percentage. Sphere lighting, bloom and refractive bands are cached when size/DPI changes; animation reuses images and paths on the message thread. No audio-thread code changed. Native DAW/GPU scheduling may differ from the offscreen JUCE image backend; interactive FL Studio listening and native per-monitor DPI acceptance remain manual checks.

## Outputs and evidence

Updated bundle: `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3` (copy the entire bundle with its `Contents` folder).

- [Before/after](BeforeAfter.html), with matched HALO/input captures.
- [Interactive capture and animation preview](Preview.html), including the 120-frame silent sequence driven by the actual processor meter.
- [Strong-input screenshot](Orb-HALO-100-3.png), [idle capture A](Idle-motion-A.png), [idle capture B](Idle-motion-B.png).
- [Core output](Core-test-output.log), [extended visual/host output](Visual-test-output.log), [performance measurements](UI-performance.txt), [previous measurements](Before-performance.txt).

No new final release was packaged or installed.
