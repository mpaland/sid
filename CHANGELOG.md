# Changelog

## 1.0.0 (2026-11-26)

The first version as a project of its own. The library comes from the
[turnkeyboard](https://github.com/turnkeyboard/turnkeyboard) firmware framework, where it was written.

### Features

* **source:** `sid::source`, the interface of everything that makes sound; `sid::output` with `open`, `play`,
  `play` for a time, `pause`, `set_volume`, `close`
* **device:** `sid::device`, the sound output of the platform in three functions (`start`, `write`, `stop`)
* **tone:** tones and melodies in sine, square and triangle
* **wav, player:** WAV files (PCM, 8 or 16 bit, mono or stereo) from memory, `sid::pcm`, `sid::play_wav()`
* **synth:** four voices; sawtooth, pulse, triangle, noise and their combinations; envelope; LFO for vibrato and
  pulse width modulation; arpeggio, glide, pitch sweep; ring modulation and hard sync by a second oscillator or
  by the voice before; resonant low / band / high pass filter and their sums (notch); band limited sawtooth and
  pulse
* **sequencer:** songs as patterns with an order and a transposition per bar, loops
* **port:** WAV file, Windows sound output (waveOut), raw PCM stream, turnkeyboard audio drivers
* **ext:** `ext/mod.h`, a player for MOD files (ProTracker, 4 channels, 31 samples) from memory, `sid::play_mod()`
* **ext:** `ext/mixer.h`, `sid::mixer<N>`: several sources at the same time, each with its level
* **ext:** `ext/meter.h`, `sid::meter`: the level of a source for a VU meter or a bar of LEDs, in a logarithmic
  or a linear scale
* **ext:** `ext/rtttl.h`, a player for ring tones as text (RTTTL), `sid::play_rtttl()`
* **ext:** `ext/sfx.h`, 13 ready sound effects for user interfaces and games, `sid::sfx::player`, `sid::play_sfx()`
* **examples:** 19 programs, from a beep to a whole song, a MOD player, ring tones, sound effects, a mixer and a
  level meter
* **tools:** `wav2h.py`, `embedded_size.sh`
* **ci:** Linux (GCC, Clang), Windows (MSVC), macOS with C++11 and C++20; Cortex-M0 / M4 compile and size check;
  CodeQL; releases by release-please
