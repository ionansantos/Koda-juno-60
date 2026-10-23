# Voice Tuning: evidence and KODA integration

Reference: installed TAL-U-NO-LX-V2 5.1.5, arm64 slice SHA-256 `f42ba995938a5df3a0beed6d28c214b80207c919f692306650222a32b1ef55e0`. REA/Ghidra 12.1.4 read-only investigation is retained in `evidence/toolbar-vt/rea-20261023/evidence-bundle.json`. The session was exported and closed. These are static observations, not proof of runtime audio equivalence.

## Observed reference behavior

- `0x81a08`, retained `027-service-refresh-caller`, toggles a dedicated ServiceControlDialog and refreshes its entries. It does not replace the keyboard.
- Six tuning records are shared by voices modulo six: SynthVoice constructor `0x51814`, retained `011` (`ev_1f5c3eea978c8c5e5d78a14ca0189573aab02dd5b9371f1527f09e13a9183d40`). Each record has cutoff, detune, attack, decay, release and pan. Existing state loader `evidence/lx-effects/rea-20261019/026-state-load.cpp.txt` establishes their XML names/defaults and application-independent values.
- Cutoff and detune handlers `0x8b820`/`0x8b6a4` (`014-ui-cutoff`/`015-ui-detune`) map `x` to `0.5 * (2*x-1)^3`. This is an additive cutoff offset and a pitch offset in semitones. Neutral is 0.5; extrema are -0.5/+0.5. TAL additionally has small constructor-time per-voice random factors; KODA retains its existing oscillator behavior.
- Attack, decay and release multiply their stage durations by `1 + 0.5 * (2*x-1)^3`, range 0.5–1.5. The state loader, `019-ui-attack` and previously retained SynthVoice process show the mapping and consumption. Sustain has no VT knob.
- Pan handler `0x8b0ac`, retained `016-ui-pan` and **instruction** evidence `018-pan-asm`: `left=(1-x*x)*1.3333334`, `right=(1-(1-x)*(1-x))*1.3333334`. Neutral is exactly 1/1; endpoints are approximately 4/3 and 0. Ghidra's pseudocode incorrectly treats the two SIMD floats as a double subtraction; assembly `fmul v0.2S` followed by `fmls v1.2S` resolves it.
- Random action `0x8c5f0`, retained `026-service-construction`, calls VoiceTuningHandler::random with its intensity control then refreshes all rows. `0x408e0`, retained `013-random`, computes `clamp(.5 + intensity * (uniform-.5), 0, 1)` independently for cutoff, detune, attack, decay and release. Pan is untouched. KODA uses JUCE's random generator, not TAL's ambient `rand()` sequence; random tables are therefore intentionally not seed-identical.
- Existing TAL state loader leaves tuning records intact when its VT lock is set; KODA processor integration owns lock/preset/session semantics separately.

## Implementation contract

`Source/VoiceTuningParameters.h` owns 36 normalized parameters, default 0.5, ordered voice 1–6 then Cutoff/Detune/Attack/Decay/Release/Pan. IDs are `vt1Cutoff`, `vt1Detune`, … `vt6Pan`. `VoiceTunings` is an array of six `VoiceTuning` records with those named float members. `SynthDSP::setVoiceTuning(const VoiceTunings&) noexcept` caches bounded coefficients without allocation. Nonfinite inputs become neutral.

KODA applies pitch/cutoff offsets and per-stage duration factors to its existing reconstructed synth. Its envelope engine differs from the reference ARM64 engine, so implementing the recovered multipliers does not establish equal envelope waveforms or arrival times. PAN is applied after each voice filter/VCA and before voice summation and common chorus/delay/reverb. It enables the stereo summation path even with Unison off; mono oscillator/filter output is reused before panning. Stereo state transition ownership is shared with Unison through one helper.

`Source/VoiceTuningPanel.h` is a 640×460 dedicated overlay with 36 APVTS attachments, CLOSE, and RANDOM with an intensity slider (default 0.2). Double-click a voice knob to return to neutral. The random intensity is transient editor state, not an additional automatable synth parameter. The parent editor owns overlay visibility and the separate VT LOCK toolbar control.

## Verification

`tests/voice_tuning_test.cpp` renders real KODA DSP at 8, 44.1, 48 and 96 kHz, verifies each of six voice detune assignments, pan center/endpoints, cutoff response, A/D/R duration directions, bounded hostile input and finite output. Both ordinary and ASan/UBSan runs passed (`evidence/toolbar-vt/unit-sanitizers.txt`). The 8 kHz pitch assertion allows 0.3 semitone because zero-crossing interpolation at that sample rate is coarse; 44.1/48/96 kHz use 0.04 semitone.

A one-second six-note neutral render from the backed-up 0.5.1 DSP and the new DSP is byte-identical. Both interleaved float32 recordings have SHA-256 `0cb0d0512b48fec9f67136e646cd3c2716aa5dc3ab00e7e9cbae0e0545ba7392` (`evidence/toolbar-vt/neutral-old.f32`, `neutral-new.f32`). This establishes unchanged neutral behavior for that controlled render, not all synth patches. Toolbar integration, persistence/LOCK, real TAL AU comparisons and host validation are recorded by the overall toolbar report.

The VT runtime comparison exposed an existing shared-filter (`Save CPU`, p38) stereo bug: the right filter consumed the left filter's already normalized cutoff, so `Filter::update` normalized it a second time. The producing representation now keeps the unscaled cutoff in one local value and passes that same value to both channel owners. No gain compensation was added. A real-render regression mirrors pan in shared-filter mode at three cutoffs and four sample rates; the fixed implementation passes a 1.5% energy symmetry tolerance, while a temporary build restoring the previous field consumption fails that assertion. Evidence: `evidence/toolbar-vt/shared-filter-regression.json`.
