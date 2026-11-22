# Granitos saltarines: specification

Living document: each decision made in the chat is written here before it's implemented.
Status of each section: TODO / AGREED / DONE (implemented and tested on a zynthian).

## Identity — AGREED
- Name: Granitos saltarines · symbol: `granitos_saltarines` · URI: `https://zynthian.org/plugins/granitos_saltarines`
- Maker: Jofemodo · license: GPL-3.0-or-later · LV2 category: lv2:DelayPlugin
- Purpose: Granular delay whose grains hop around the buffer like bouncing balls

## I/O — AGREED
- Audio: 2 in, 2 out
- MIDI: no input, no output
- Effect (not a synth)

## Controls (LV2 input ports) — AGREED
Pages = port groups, 4 controls per page in zynthian. Symbols are stable API. 13 controls, 4 pages (4 + 3 + 3 + 3):
the bypass takes the last slot of the page with Mix, and the rest is grouped by what each control does, with no
page of 1 or 2 controls.

| Page | Symbol | Name | Range | Default | Unit | Type | Designation / notes |
|---|---|---|---|---|---|---|---|
| Delay | `time` | Time | 10 .. 2000 | 300 | ms | float |  |
| Delay | `feedback` | Feedback | 0 .. 95 | 30 | % | float |  |
| Delay | `mix` | Mix | 0 .. 100 | 50 | % | float |  |
| Delay | `bypass` | Bypass | 0 .. 1 | 0 |  | boolean | `kParameterDesignationBypass`: LV2 port `lv2_enabled` (inverted by DPF), the processor's native bypass in zynthian |
| Grains | `grain_size` | Grain size | 5 .. 500 | 80 | ms | float |  |
| Grains | `density` | Density | 1 .. 50 | 12 | grains/s | float |  |
| Grains | `jitter` | Jitter | 0 .. 100 | 20 | % | float |  |
| Hop | `hop` | Hop | 0 .. 100 | 50 | % | float | The signature control: how far the grains follow the ball. Default 50 so the plugin shows its character out of the box |
| Hop | `gravity` | Gravity | 0 .. 100 | 50 | % | float |  |
| Hop | `freeze` | Freeze | 0 .. 1 | 0 |  | boolean |  |
| Variation | `pitch` | Pitch | -24 .. 24 | 0 | st | float |  |
| Variation | `reverse` | Reverse | 0 .. 100 | 0 | % | float |  |
| Variation | `spread` | Stereo spread | 0 .. 100 | 50 | % | float |  |

Note: the zynthian's own level meters are used, so there are no peak-level outputs.

## Outputs (LV2 output ports → widget monitors, ~30 Hz) — AGREED (revised after review)

| Symbol | Name | Range | Meaning / how it's computed |
|---|---|---|---|
| `grain_count` | Grains | 0 .. 64 | Number of grains sounding at the end of the block. About `density × grain_size`: 1 at the defaults, ~25 at 50 grains/s with 500 ms grains. |
| `read_pos` | Read position | 0 .. 1 | Where the grains read by default: write head − `time`, as a fraction of the buffer. Always defined, moves smoothly forward and wraps at the end of the strip. |
| `hop_height` | Hop height | 0 .. 1 | Height of the bouncing ball. The ball runs on its own, whatever `hop` is, so `gravity` is always visible. |
| `grain_pos_1` .. `grain_pos_16` | Grain N position | -1 .. 1 | Real read position (0..1 of the buffer) of grain pool slots 0..15, or -1 when the slot is idle. Grains take the lowest free slot, so up to 16 simultaneous grains are all shown. |

| `grain_env_1` .. `grain_env_16` | Grain N level | 0 .. 1 | Level of the same grains: their Hann window value (0 at the ends, 1 in the middle), 0 when the slot is idle. |

16 position ports instead of one cloud of invented dots: the widget draws each dot where the grain really reads. Sizes carry information too: a dot's radius follows its grain's level, and the ball's radius follows the Hop control.

## DSP — DONE (host-tested locally and built on the zynthian)
- Buffer: stereo circular buffer of 3 s at the host sample rate (time max 2 s + jitter 0.25 s + longest grain 0.5 s). The input plus the feedback signal is written every sample, unless `freeze` is on.
- Grain scheduler: a new grain starts every `1/density` seconds, with a random offset of up to ±50 % of that interval.
- Grain pool: up to 64 preallocated grains, taking the lowest free slot. When full, the oldest grain is stolen.
- Grain parameters:
  - Start position: `write_pos − time`, plus a random offset scaled by `jitter`.
  - Length: `grain_size`, shaped by a Hann window.
  - Rate: `2^((pitch + random detune × jitter) / 12)`.
  - Direction: backwards with probability `reverse`.
  - Pan: random per grain within `spread`, equal power.
- Reading is done as a delay behind the write head. Unless `freeze` is on, the delay is kept between 64 samples and the oldest sample, so a pitched-up or reversed grain never reads audio from a full buffer ago. When frozen, reads wrap around the buffer and the material keeps looping.
- Hop and gravity (revised after review: bouncing inside a grain of 80 ms can't be seen or heard): one bouncing ball runs in real time. It is thrown, and each bounce lasts `e` times the previous one and is `e²` as high, with `e = 0.9 − 0.6 × gravity`: more gravity = it settles sooner (gravity 0 keeps bouncing for ~6 s, gravity 100 settles in ~1 s). The first bounce lasts 0.7 s. When it settles it rests 0.3 s and is thrown again. A grain born while the ball is high starts closer to the write head: its delay is `time × (1 − 0.5 × hop × ball height)`. So the echoes jump with the ball, and `hop` is how far they follow it. With `hop` 0 the ball still bounces but nothing follows it.
- Feedback goes back into the buffer, so the grains feed the next repeat. The output is levelled by `1/sqrt(overlap)` (overlap = density × grain length / 2), the feedback path by `1/overlap` with a soft clip (`tanh`): even correlated grains give a loop gain below the feedback setting, so it can't run away.
- Changing `time` glides the read position over about 50 ms, so there are no clicks.
- Mix: dry/wet with equal power.
- Bypass: the processed signal is crossfaded to the dry input in 10 ms, so there are no clicks. The input is recorded with the same 10 ms ramp, so that a buffer that starts empty has no hard edge. Once fully bypassed the DSP is skipped (the plugin costs almost nothing), the input passes through bit-exact, and the buffer, the grains and the ball are reset: when the bypass is released the delay starts empty. The outputs of the visualization go idle (no grains, ball down).
- RT rules: no allocation, locks, I/O or logging in `run()`. Denormals in the buffer are flushed to zero.

## State & presets — AGREED
- No extra saved state (the buffer is runtime-only).
- No factory presets for now.

## Zynthian widget — DONE (built, installed, check passes; not yet seen on the display)
- Half height: the lower half of the screen, with the page list visible above it.
- A strip for the 3 s circular buffer, with 2 % side margins, and a line at `read_pos`.
- One dot per sounding grain (up to 16), at the position it is really reading (`grain_pos_N`). The lane (vertical position) is fixed by the grain slot. The dot's radius follows the grain's level (`grain_env_N`): a pin at the ends of the grain, as big as the lane in the middle.
- A ball above the strip at `read_pos`, its height following `hop_height` and its radius following the Hop control (the widget reads the control itself): it bounces whatever Hop is, and gravity changes how fast it settles. With Hop up, the grain dots are shifted toward the write head while the ball is high.
- A bar of 16 segments under the strip, 2 grains per segment (full = 32 grains), from `grain_count`.
- The strip is a ring: near the seam, some grains can already be back at the start while the marker is still at the end.
- Colours from the zynthian theme. No knob or switch handling: the widget only draws.

## Native GUI (X11, Cairo) — DONE (built on the zynthian; not yet seen on the VNC display)
- Window about 640×320, shown on the VNC display only.
- The four pages of controls as columns, in the order of the port groups (Delay, Grains, Hop, Variation): the name on top, the bar with its value inside.
- Larger version of the buffer view as the main visual: the same elements as the widget (buffer strip, read marker, grain dots, bouncing ball), at a larger size.
- The GUI is never needed for computation: the plugin works fully without it.

## Help page — DONE (written; not yet seen in the UI)

## Open questions
- None yet.
