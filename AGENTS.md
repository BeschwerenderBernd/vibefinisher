# VibeFinisher

A multi-saturation mix bus vibe finisher for 90s house/trance production. Models the combined character of overdrive, tape saturation, and vinyl coloration with gated noise injection — all driven by a single macro "Vibe" knob.

## Build & Install

```bash
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Plugins are auto-copied after build to:
- `~/Library/Audio/Plug-Ins/VST3/VibeFinisher.vst3`
- `~/Library/Audio/Plug-Ins/Components/VibeFinisher.component`

JUCE 9.0.0 is vendored at `third_party/JUCE`.

> **Note:** After every change to the code, update this file so it always reflects the current state of the plugin.

## Goal

A creative mix bus tool that combines four saturation/flavor modules into one character knob. Think of it as the combined sound of signal hitting a desk channel (overdrive), being recorded to tape (compression + head bump + HF rolloff), being pressed to vinyl (saturation + mono bass + warmth), all with subtle gated noise injection. The "Vibe" macro sweeps everything simultaneously — from clean through warm to pushed.

## Architecture

### Signal chain

```
Input → Input Gain → Overdrive → ┌─ Tape ─┐
                                → ├─ Blend ├ → Gated Noise → Output Gain → Wet/Dry
                                → └─ Vinyl ─┘
```

Dry capture happens after input gain (before overdrive) for the final wet/dry mix. The dry path is delay-compensated by the oversampling latency (4x IIR polyphase) via an integer delay line, and the latency is reported to the DAW via `setLatencySamples()`. Output gain is applied after the wet/dry mix so both paths track gain consistently.

A fixed −6 dB calibration pad attenuates the signal before the saturation chain and is made up afterwards, so the stages are not driven too hot. It can be disabled via the PAD button to drive the stages 6 dB hotter for more grit; the switch is smoothed (5ms) to avoid clicks, and the noise level scales with the pad so its perceived loudness stays constant.

### Bus layout

Single stereo bus (1 input, 1 output). Mono/stereo only — no surround or sidechain.

### DSP stages

#### OverdriveStage (`Source/DSP/OverdriveStage.h`)
Tanh soft-clipper with 4x oversampling. No asymmetry — just a smooth saturator that generates even/odd harmonics and rolls off naturally at ±1. The oversampling prevents alias foldback into the audible band. Drive input is per-sample smoothed (5ms) to eliminate zipper noise from parameter changes.

#### TapeStage (`Source/DSP/TapeStage.h`)
Three series processing steps applied to the wet path only, then blended with dry:
1. **Arctan saturation** — sharper knee than tanh, models tape hysteresis shoulder
2. **Peak-driven envelope compression** — BallisticsFilter (2ms attack, 80ms release) on per-sample peak magnitude drives gain reduction up to 25% when envelope > 0.04, mimicking tape's natural flux saturation limiting
3. **EQ curve** — low shelf +3 dB at 80 Hz (head bump), high shelf -2.5 dB at 9 kHz (HF rolloff from self-demagnetization)

Output: `dry * (1 - depth) + processed * depth`. Depth is per-sample smoothed (5ms).

#### VinylStage (`Source/DSP/VinylStage.h`)
Two steps applied in-place:
1. **Tanh saturation** — mild harmonic generation from cutting head/playback stylus nonlinearity
2. **Bass-only mono** — mid/side decomposition with the side channel low-pass filtered at 150 Hz (2nd-order Butterworth), then the low-frequency side content attenuated by `depth * 0.6`. High-frequency width is untouched, matching real vinyl mastering practice where only bass is summed to mono.

Output: `dry * (1 - depth) + saturated * depth`, followed by bass-only stereo narrowing. Depth is per-sample smoothed (5ms).

#### ParallelBlend (`Source/DSP/ParallelBlend.h`)
Linear crossfade between tape and vinyl outputs: `output = tape * (1 - blend) + vinyl * blend`. Blend is per-sample smoothed (5ms).

#### NoiseStage (`Source/DSP/NoiseStage.h`)
Four selectable noise generators, all gated behind a threshold:
- **Tape Hiss** — bandpass 2-8 kHz (modeled preamp hiss), mono/correlated across channels
- **Vinyl** — sparse impulse crackle (~13 pops/sec, sample-rate independent) shaped as a decaying noise burst (τ≈0.5ms) with low-level background texture. Pops are uncorrelated per channel for authentic stereo character.
- **Console** — pink noise (1/f) via Kellet economy filter, mono/correlated
- **Digital** — LP-filtered white noise at 3 kHz, mono/correlated

Gate: BallisticsFilter (10ms attack, 50ms release) smooths the input level, then compares against threshold. Gate opens with a fixed 2ms attack and closes with the configurable Decay time (50-5000ms) for natural fade-out. Noise level is per-sample smoothed (5ms).

### Parameters

#### Main

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Vibe | 0–100% | 50% | Master macro — drives all saturation depth, blend, noise |
| Drive | 0–18 dB | 5 dB | Overdrive gain, scaled by Vibe |
| Tp/Vnl (Blend) | 0–100% | 50% | Tape/Vinyl balance (0 = all tape, 100 = all vinyl) |
| Noise | 0–100% | 10% | Noise injection level, scaled by Vibe |
| Mix | 0–100% | 100% | Wet/dry blend (100 = full effect) |
| Gate | -60–0 dB | -40 dB | Noise gate threshold (RMS-smoothed input vs linear threshold) |
| Input | -24–+24 dB | 0 dB | Input gain before the chain |
| Output | -24–+6 dB | 0 dB | Output trim after the chain |
| Decay | 50–5000 ms | 500 ms | Noise gate fade-out time after signal drops below threshold |
| Noise Type | choice | Tape Hiss | Tape Hiss, Vinyl, Console, Digital |
| Pad | toggle | on | −6 dB headroom pad before the saturation chain (made up afterwards). Off drives the stages 6 dB hotter |

#### Advanced (toggle via ADV button)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Drive Trim | -50–+50% | 0% | Offset from Vibe baseline for overdrive |
| Tape Trim | -50–+50% | 0% | Offset from Vibe baseline for tape stage |
| Vinyl Trim | -50–+50% | 0% | Offset from Vibe baseline for vinyl stage |
| Noise Trim | -50–+50% | 0% | Offset from Vibe baseline for noise level |

### Macro scaling

All depth calculations follow: `depth = vibe * (1.0 + trim)`

So at Vibe=50% with neutral trim: 50% depth. At Vibe=100%, trim=+50%: 150% depth. At Vibe=0: everything bypasses.

Drive is scaled differently: `effectiveDrive = driveVal * vibe * (1.0 + driveTrim)` — the drive knob sets the base amount, Vibe and trim scale it.

Noise follows the same pattern: `noiseLevel = noiseLevelVal * vibe * (1.0 + noiseTrim)`.

All time-varying macro parameters (drive, depth, blend, noise level, mix) are per-sample smoothed with a 5ms linear ramp via `juce::SmoothedValue` to prevent zipper noise from parameter automation or macro sweeps. Non-time-varying parameters (gate threshold, decay, noise type) are applied at block rate.

### UI

Modern dark theme (burnt orange accent #FF7043). Two rows of 68px arc-style rotary knobs below a centered 96px Vibe knob. An input level meter (30 Hz refresh) spans the width above Vibe, showing post-input-gain RMS in dBFS with a gate threshold marker.

The header holds the PAD and ADV buttons. The input meter's reference tick marks the meter level that corresponds to −18 dBFS inside the saturation chain: −12 dBFS with the pad on, −18 dBFS with it off. Tooltips on the PAD button and INPUT knob describe the ideal range.

```
┌─────────────────────────────────────────┐
│  VIBEFINISHER                 [PAD][ADV]│
│  [=============METER=============] dB   │
│                                         │
│           ┌──────────────┐              │
│           │     VIBE     │              │
│           └──────────────┘              │
│                                         │
│   DRIVE  TP/VNL  NOISE  GATE   [ADV]   │
│                                         │
│   INPUT  OUTPUT  DECAY  [Type▼]  MIX   │
│  ─────────────────────────────────────  │
│   DRV TRIM  TAPE TRIM  VNL TRIM  NOI   │  ← hidden panel
└─────────────────────────────────────────┘
```

### Plugin identity

| Property | Value |
|----------|-------|
| CMake target | `VibeFinisher` |
| FORMATS | `AU VST3` |
| PRODUCT_NAME | `VibeFinisher` |
| MANUFACTURER_CODE | `ViFi` |
| PLUGIN_CODE | `Vfin` |
| VST3_CATEGORIES | `Fx\|Distortion` |
| BUNDLE_ID | `com.vibefinisher.plugin` |
| C++ standard | C++20 |
| macOS target | 12.0 |
