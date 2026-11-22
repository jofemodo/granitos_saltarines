# Granitos saltarines

A granular delay whose grains hop around the buffer like bouncing balls.

LV2 plugin built with [DPF](https://github.com/DISTRHO/DPF), with a native X11 GUI (Cairo) and a
[zynthian](https://zynthian.org) UI widget (`zyngui:widget`) in the same bundle.

- URI: `https://zynthian.org/plugins/granitos_saltarines`
- Maker: Jofemodo · License: GPL-3.0-or-later
- Delay effect, stereo in / stereo out, no MIDI

## What it does

The input is recorded continuously into a 3 s stereo circular buffer. The sound comes back as grains: short,
Hann-windowed slices read from the buffer, `Density` new grains per second, up to 64 at once. Each grain starts
`Time` behind the write head and has its own pitch, direction, pan and start offset. The grains are summed,
mixed with the dry signal (equal power) and fed back into the buffer.

### The bouncing ball

One ball bounces in real time. It is thrown, and each bounce lasts a fraction `e` of the previous one and is `e²`
as high, with `e = 0.9 − 0.6 × Gravity`. The first bounce lasts 0.7 s. When the ball has settled it rests for
0.3 s and is thrown again.

- **Gravity** sets how fast the ball settles: at 0 % it keeps bouncing for about 6 s, at 50 % for about 1.5 s, at
  100 % for about 1 s.
- **Hop** sets how far the grains follow the ball. A grain born while the ball is high starts closer to the write
  head: its delay is `Time × (1 − 0.5 × Hop × ball height)`. The echoes jump with the ball, by up to half the
  delay time at Hop 100 %.
- The ball bounces whatever Hop is. At Hop 0 % the grains ignore it, so Gravity has no audible effect.

## Controls

Four pages of controls, as zynthian shows them.

| Page | Control | Range | Default | What it does |
|---|---|---|---|---|
| Delay | Time | 10 – 2000 ms | 300 | How far behind the write head the grains start |
| Delay | Feedback | 0 – 95 % | 30 | How much of the wet signal goes back into the buffer |
| Delay | Mix | 0 – 100 % | 50 | Dry / wet (equal power) |
| Delay | Bypass | off / on | off | Passes the input through untouched |
| Grains | Grain size | 5 – 500 ms | 80 | Length of each grain |
| Grains | Density | 1 – 50 grains/s | 12 | Grains started per second (spacing randomised by up to ±50 %) |
| Grains | Jitter | 0 – 100 % | 20 | Randomises each grain's start position (up to ± half a grain) and pitch (up to ±12 st) |
| Hop | Hop | 0 – 100 % | 50 | How far the grains follow the bouncing ball |
| Hop | Gravity | 0 – 100 % | 50 | How fast the ball settles |
| Hop | Freeze | off / on | off | Stops recording: the buffer keeps looping and the grains keep hopping over it |
| Variation | Pitch | −24 – +24 st | 0 | Transposes the grains |
| Variation | Reverse | 0 – 100 % | 0 | Chance that a grain plays backwards |
| Variation | Stereo spread | 0 – 100 % | 50 | How far each grain's random pan reaches from the centre |

Bypass is the plugin's native bypass: it is the LV2 port `lv2_enabled` (designated `lv2:enabled`, which DPF
inverts for the plugin), so zynthian uses it as the processor's bypass instead of its own routing.

The number of grains sounding at once is about `Density × Grain size`: 1 at the defaults, about 25 at 50 grains/s
with 500 ms grains.

## Behaviour notes

- Changing Time, Mix or Feedback glides over about 50 ms, so there are no clicks.
- Bypass crossfades to the dry input in 10 ms, with no clicks. While bypassed the input passes through bit-exact and
  the DSP is not run at all, so a bypassed plugin costs almost nothing. The buffer is cleared, so when the bypass is
  released the delay starts empty, with its recording faded in over the same 10 ms.
- Grains read as a delay behind the write head, kept between 64 samples and the oldest recorded sample: a grain
  that is pitched up or reversed never reads audio from a whole buffer ago. When Freeze is on, reads wrap around
  the buffer instead, so the frozen material loops.
- Feedback can't run away, even when the grains are correlated (same material, no jitter): the feedback path is
  levelled by the grain overlap and soft-clipped.
- The output is levelled by the grain overlap, so raising Density or Grain size doesn't make the wet signal
  louder.
- The DSP is real-time safe: everything is allocated outside `run()`, and the plugin works without any UI.

## Zynthian widget

Half height, drawn from the plugin's output ports. A strip shows the 3 s circular buffer:

- a line at the point where the grains read by default (the write head minus `Time`), moving along the strip and
  wrapping at the end, like the buffer does;
- one dot per sounding grain (up to 16), at the position it is really reading. Its size follows the grain's own
  level (its Hann window): pin-sized when the grain starts and ends, as big as its lane in the middle of its life;
- a ball above the strip, following the bouncing ball. Its size follows the Hop control: the bigger the ball, the
  harder it pulls the grains;
- a bar of 16 segments under the strip, 2 grains per segment, with how many grains sound at once.

The native GUI shows the same view, with the four control pages below it as columns of bars you can drag or scroll.

### Output ports

| Port | Range | Meaning |
|---|---|---|
| `grain_count` | 0 – 64 | Grains sounding |
| `read_pos` | 0 – 1 | Write head minus `Time`, as a fraction of the buffer |
| `hop_height` | 0 – 1 | Height of the bouncing ball |
| `grain_pos_1` … `grain_pos_16` | −1 – 1 | Read position (0 – 1 of the buffer) of grain slots 0 – 15, or −1 when idle |
| `grain_env_1` … `grain_env_16` | 0 – 1 | Level (Hann window) of those grains, 0 when idle |

## Build

```
git submodule update --init --recursive
make            # -> bin/granitos_saltarines.lv2
make install    # PREFIX=/usr/local (LV2_DIR=$PREFIX/lib/lv2)
```

Build dependencies: build-essential, pkg-config, libcairo2-dev, libx11-dev, libxext-dev, libxrandr-dev,
libxcursor-dev, libgl-dev. The TTL is generated by running the plugin (`make gen`), so build on the target
architecture.

## Continuous build

Every push and pull request builds the bundle on GitHub Actions for x86_64 and arm64, inside Debian trixie (the
zynthian OS base, so the same glibc and libraries as the device). The build checks that the bundle is complete
(manifest, TTL, both binaries, the widget declared) and keeps the bundles as workflow artifacts.

- A push to the default branch publishes the bundles in the rolling `latest` prerelease.
- A tag `vX.Y.Z` (matching the version in the code) publishes a release with that name.

The binary packages are on the [releases page](https://github.com/jofemodo/granitos_saltarines/releases): the
rolling `latest` prerelease (marked as a pre-release, so GitHub doesn't show it in the repository sidebar) and the
`vX.Y.Z` releases, each with `granitos_saltarines-lv2-arm64.tar.gz` (for the zynthian) and
`granitos_saltarines-lv2-x86_64.tar.gz`.

To install a downloaded bundle on a zynthian:

```
tar -xzf granitos_saltarines-lv2-arm64.tar.gz -C /zynthian/zynthian-plugins/lv2
```

DPF is a git submodule pinned to a tested commit: update the pin only deliberately, after building and testing.

## Layout

| Path | Contents |
|---|---|
| `plugins/granitos_saltarines/ParamDefs.hpp` | Parameters and port groups (shared by DSP and GUI) |
| `plugins/granitos_saltarines/Plugin.cpp` | DSP |
| `plugins/granitos_saltarines/UI.cpp` | Native GUI |
| `zyngui/` | zynthian widget, help page, `zyngui:widget` declaration, port extras |
| `tools/zyngui_bundle.py` | Adds the zyngui/ files to the bundle after DPF generates it |
| `SPEC.md` | Specification |
