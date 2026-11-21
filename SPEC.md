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
Pages = port groups, 4 controls per page in zynthian. Symbols are stable API.

| Page | Symbol | Name | Range | Default | Unit | Type | Designation / notes |
|---|---|---|---|---|---|---|---|
| Delay | `time` | Time | 10 .. 2000 | 300 | ms | float |  |
| Delay | `feedback` | Feedback | 0 .. 95 | 30 | % | float |  |
| Delay | `mix` | Mix | 0 .. 100 | 50 | % | float |  |
| Delay | `spread` | Stereo spread | 0 .. 100 | 50 | % | float |  |
| Grains | `grain_size` | Grain size | 5 .. 500 | 80 | ms | float |  |
| Grains | `density` | Density | 1 .. 50 | 12 | grains/s | float |  |
| Grains | `pitch` | Pitch | -24 .. 24 | 0 | st | float |  |
| Grains | `jitter` | Jitter | 0 .. 100 | 20 | % | float |  |
| Hop | `hop` | Hop | 0 .. 100 | 0 | % | float | The signature control |
| Hop | `gravity` | Gravity | 0 .. 100 | 50 | % | float |  |
| Hop | `reverse` | Reverse | 0 .. 100 | 0 | % | float |  |
| Hop | `freeze` | Freeze | 0 .. 1 | 0 |  | boolean |  |

Note: the zynthian's own level meters are used, so there are no peak-level outputs.

## Outputs (LV2 output ports → widget monitors, ~30 Hz) — AGREED

| Symbol | Name | Range | Unit | Meaning / how it's computed |
|---|---|---|---|---|
| `grain_count` | Grains | 0 .. 64 | | Number of grains currently sounding |
| `read_pos` | Read position | 0 .. 1 | | Read position of the grains in the buffer, as a fraction of the buffer length |
| `hop_height` | Hop height | 0 .. 1 | | Height of the current bounce (the hop curve value, normalised) |

## DSP — DONE (host-tested locally; not yet on the zynthian)
- Buffer: stereo circular buffer of 2 s at the host sample rate. The input plus the feedback signal is written every sample, unless `freeze` is on.
- Grain scheduler: a new grain starts every `1/density` seconds, with a random offset of up to ±50 % of that interval.
- Grain pool: up to 64 preallocated grains. When full, the oldest grain is stolen.
- Grain parameters:
  - Start position: `write_pos − time`, plus a random offset scaled by `jitter`.
  - Length: `grain_size`, shaped by a Hann window.
  - Rate: `2^((pitch + random detune × jitter) / 12)`.
  - Direction: backwards with probability `reverse`.
  - Pan: random per grain within `spread`, equal power.
- Hop: each grain bounces through its lifetime. At bounce `k` its read position jumps forward by `hop × step × r^k`, where `r` shrinks as `gravity` rises.
- Feedback goes back into the buffer, so the grains feed the next repeat.
- Changing `time` glides the read position over about 50 ms, so there are no clicks.
- Mix: dry/wet with equal power.
- RT rules: no allocation, locks, I/O or logging in `run()`.

## State & presets — AGREED
- No extra saved state (the buffer is runtime-only).
- No factory presets for now.

## Zynthian widget — AGREED
- Half height: the lower half of the screen, with the page list visible above it.
- Shows a horizontal strip for the 2 s buffer, with a marker at `read_pos`.
- A dot per active grain (`grain_count`) drawn at the marker.
- A small ball bouncing above the strip, its height following `hop_height`.
- Colours from the zynthian theme.
- No knob or switch handling: the widget only draws.
- Status: `spec.json` has `half_height: true`. The widget code isn't written yet.

## Native GUI (X11, Cairo) — AGREED
- Window about 640×320, shown on the VNC display only.
- Same three pages of controls as zynthian (Delay, Grains, Hop).
- Larger version of the buffer view as the main visual: the same elements as the widget (buffer strip, read marker, grain dots, bouncing ball), at a larger size.
- The GUI is never needed for computation: the plugin works fully without it.

## Help page — AGREED (text to be written)

## Open questions
- None yet.
