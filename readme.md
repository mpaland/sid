# Sound Interface Driver library

[![Release](https://img.shields.io/github/v/tag/mpaland/sid?label=release&sort=semver)](https://github.com/mpaland/sid/tags)
[![CI](https://github.com/mpaland/sid/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mpaland/sid/actions/workflows/ci.yml)
[![CodeQL](https://github.com/mpaland/sid/actions/workflows/codeql.yml/badge.svg?branch=main)](https://github.com/mpaland/sid/actions/workflows/codeql.yml)
[![C++11](https://img.shields.io/badge/C%2B%2B-11-blue.svg?logo=cplusplus)](https://en.cppreference.com/w/cpp/11)
[![Header only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)](#highlights)
[![No heap](https://img.shields.io/badge/heap-none-brightgreen.svg)](#highlights)
[![Compilers](<https://img.shields.io/badge/compilers-MSVC%20%7C%20GCC%20%7C%20Clang-informational.svg>)](.github/workflows/ci.yml)
[![Last commit](https://img.shields.io/github/last-commit/mpaland/sid.svg)](https://github.com/mpaland/sid/commits/main)
[![Stars](https://img.shields.io/github/stars/mpaland/sid.svg?style=social)](https://github.com/mpaland/sid/stargazers)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

### Sound for embedded systems in the style of home computers of the 80s: beeps, melodies, synthesizer and a sequencer - header-only C++11, no float, no heap

This library is a long standing one. Back in 2010 I was playing around with generic sound generation on embedded boards which should go a little further than putting a square wave on a beeper.
So the basics of this library where created. I looked at the famous "SID" chip of MOS used in the Commodore 64, which is able to produce rather cool sounds with minimum effort. Ideas of this part went in the `synth.h` module.
Simplicity first, so a tone generation should be rather easy like this:

```cpp
#include "sid/src/sid.h"

sid::output out(device);                        // the sound output of your platform
out.open(22050U);                               // sample rate, output on
sid::beep(out, 880U, 150U);                     // 880 Hz for 150 ms
sid::play(out, song);                           // a whole tune on four synthesizer voices
out.close();                                    // plays out, output off
```

Lately I needed an universal but simple sound library for making the turnkeyboard project audible and got back to my ancient SID sources.
I fix bugs some bugs, added test cases and some new modules like the sfx module.
And here we are:

## Highlights

- **Beeps and melodies** in one line: sine, square and triangle tones, melodies as arrays of notes
- **Recorded sound**: WAV files (PCM, 8 or 16 bit, mono or stereo) straight from memory or flash, with a tool that
  turns a file into a header
- **A synthesizer** with the building blocks of the sound chips of the 80s: sawtooth, pulse with variable width, triangle and noise - also combined - an envelope per voice, LFO for vibrato and pulse width modulation, arpeggio, glide, pitch sweeps, ring modulation, hard sync and a resonant low / band / high pass / notch filter
- **A sequencer** that plays songs written as patterns, the way the tunes of that time were made. A song is constant data: 46 seconds of four voice music take 3.3 KB of flash
- **Everything is a source**: tones, clips, the synthesizer and your own generators and effects all implement one function and are played the same way
- **Extensions**, each one header to include when it is needed: a mixer that plays several sources at once (music with sound effects), a level meter for a VU display or a bar of LEDs, ring tones as text (**RTTTL**), ready sound effects for games and devices, and **a player for MOD files**
- **One small interface to port**: three functions (`start`, `write`, `stop`) connect the library to a DAC, a codec, an I2S or PDM output, a sound card or a file. Ready devices: WAV file, Windows sound output, raw PCM for a pipe
- **HEADER ONLY**, no module compilation; platform and CPU independent, NO dependencies, NO heap, NO `float`, no exceptions, no run time type information - clean C++11
- Small: the whole library is 4.3 KB of code on a Cortex-M4, the synthesizer needs 440 bytes of RAM ([footprint](#footprint))
- Band limited sawtooth and pulse waves, so high notes stay clean at the low sample rates of small outputs
- The same sound everywhere: integer arithmetic only, so nothing depends on the floating point of a platform - the MSVC and the GCC build write identical files
- Tested: the sound is measured in the tests (pitch, loudness, spectrum), built and run with MSVC, GCC and Clang and cross compiled for Cortex-M on every commit
- MIT license

## What is SID _not_?

SID is not an emulation of the MOS 6581 / 8580 "SID" chip of the Commodore 64 and cannot play `.sid` files. Those are programs for a 6502 CPU that write the registers of the chip; this lib has no registers. It is a synthesizer of its own with the same kind of building blocks and a similar kind of sound, made to be programmed directly:

|                                  | MOS SID chip                                                                | SID library                                                           |
| -------------------------------- | --------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| Voices                           | 3                                                                           | 4 (`SID_VOICES`)                                                    |
| Waveforms                        | triangle, sawtooth, pulse, noise and their combinations                     | the same                                                              |
| Envelope                         | attack, decay, sustain, release in 16 fixed rates                           | the same four stages, times in ms                                     |
| Ring modulation, hard sync       | by the neighbouring voice                                                   | by the neighbouring voice, or by a second oscillator inside the voice |
| Filter                           | low, band, high pass and sums of them, resonance, one for all voices        | the same                                                              |
| Vibrato, arpeggio, glide, sweeps | not in the chip: the player routine rewrote the registers 50 times a second | built in, set in the patch                                            |
| Programming                      | 29 registers                                                                | `note_on()` / `note_off()` with a patch; songs as patterns        |
| Play recorded sound              | a trick with the volume register                                            | a regular PCM / WAV player                                            |
| Character                        | analogue filter with its distortion                                         | clean and band limited                                                |

SID is not a general audio framework either: one sample format (16 bit stereo), no resampling (the [mixer](#mixer-extension) adds sources of one sample rate), no decoders for compressed audio.

## Quick start

Take the [WAV file device](src/port/wav_file.h) to hear something without any sound hardware:

```cpp
#include "sid/src/sid.h"
#include "sid/src/port/wav_file.h"

// a bass: a sawtooth through the resonant low pass, which closes while the note sounds
static sid::patch make_bass()
{
  sid::patch p = { };
  p.wave       = sid::osc_saw;
  p.level      = 150U;
  p.attack     = 2U;        // [ms]
  p.decay      = 150U;
  p.sustain    = 200U;      // of 255
  p.release    = 200U;
  p.filter     = sid::filter_low;
  p.resonance  = 170U;
  p.cutoff     = 2200U;     // [Hz] when the note starts
  p.cutoff_end = 350U;      // where it moves to
  p.cutoff_ms  = 400U;      // in this time
  return p;
}

static const sid::patch patches[] = { make_bass() };

int main()
{
  sid::port::wav_file file("hello.wav");
  sid::output out(file);
  if (!out.open(22050U)) {
    return 1;
  }

  // a beep and a melody
  sid::beep(out, 880U, 150U);
  static const sid::note notes[] = { { sid::c5, 200U }, { sid::e5, 200U }, { sid::g5, 400U } };
  sid::play(out, notes, 3U, sid::wave_triangle);

  // a note on the synthesizer
  sid::synth synth(out.rate());
  synth.note_on(0U, patches[0], sid::key::a2);
  out.play(synth, 800U);                        // the synthesizer never ends: it is played for a time
  synth.note_off(0U);
  out.play(synth, 300U);

  out.close();
  return 0;
}
```

This is [examples/00_hello.cpp](examples/00_hello.cpp), which is built and run with every commit.

On Windows replace the device by `sid::port::win32` ([port/win32.h](src/port/win32.h)) and the same program plays on the sound output of the PC. On Linux and macOS write raw samples to stdout ([port/pcm_stream.h](src/port/pcm_stream.h)) and pipe them into `aplay`, `ffplay` or `sox`.

## Listen

The [examples](#examples) write WAV files. Build them, or download the artifact `sid-examples-wav` of the latest [CI run](https://github.com/mpaland/sid/actions/workflows/ci.yml):

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release           # runs the tests and every example: the WAV files are in build/examples/wav
```

Live:

```sh
build\examples\Release\sid_example_12_song.exe --play                           # Windows
build/examples/sid_example_12_song --stdout | aplay -f S16_LE -c 2 -r 22050     # Linux (ALSA)
build/examples/sid_example_12_song --stdout | ffplay -f s16le -ch_layout stereo -ar 22050 -nodisp -autoexit -
```

## Use it in your project

SID is header-only. The headers are included as `"sid/src/<name>.h"`.

**With CMake** - as a sub folder or fetched; the target `sid::sid` gives the include path whatever the folder is called:

```cmake
include(FetchContent)
FetchContent_Declare(sid GIT_REPOSITORY https://github.com/mpaland/sid.git GIT_TAG main)
FetchContent_MakeAvailable(sid)

target_link_libraries(app PRIVATE sid::sid)
```

**Without** - copy or clone the repository as a folder named `sid` into your project and put the folder above it on
the include path. Nothing to compile.

| Header                                | Content                                                                                                                                 |
| ------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| [`src/sid.h`](src/sid.h)             | all of the following in one include                                                                                                     |
| [`src/source.h`](src/source.h)       | `sid::source` (something that makes sound), `sid::device` (the output of the platform), `sid::output` (plays sources on a device) |
| [`src/tone.h`](src/tone.h)           | `sid::tone`, `sid::melody`, the note names `sid::c3` .. `sid::b6`, `sid::beep()`, `sid::play(out, notes, count)`            |
| [`src/wav.h`](src/wav.h)             | `sid::wav::parse()`: format and samples of a WAV file in memory                                                                       |
| [`src/player.h`](src/player.h)       | `sid::pcm` (recorded samples as a source), `sid::play_wav()`                                                                        |
| [`src/synth.h`](src/synth.h)         | `sid::synth`, `sid::patch`, the keys `sid::key::c1` .. `sid::key::b7`                                                           |
| [`src/sequencer.h`](src/sequencer.h) | `sid::sequencer`, `sid::song`, `sid::step`, `sid::slot`, `sid::play(out, song)`                                               |
| [`src/port/`](src/port)              | ready devices: `wav_file.h`, `win32.h`, `pcm_stream.h`, `turnkeyboard.h`                                                        |
| [`src/ext/mixer.h`](src/ext/mixer.h) | extension, not in `sid.h`: `sid::mixer<N>`, several sources at the same time                                                        |
| [`src/ext/meter.h`](src/ext/meter.h) | extension, not in `sid.h`: `sid::meter`, the level of a source for a VU meter or a bar of LEDs, logarithmic or linear               |
| [`src/ext/rtttl.h`](src/ext/rtttl.h) | extension, not in `sid.h`: `sid::rtttl::player`, ring tones as text (RTTTL), `sid::play_rtttl()`                                  |
| [`src/ext/sfx.h`](src/ext/sfx.h)     | extension, not in `sid.h`: `sid::sfx::player` and 13 ready sound effects, `sid::play_sfx()`                                       |
| [`src/ext/mod.h`](src/ext/mod.h)     | extension, not in `sid.h`: `sid::mod::player`, a player for MOD files (ProTracker, 4 channels), `sid::play_mod()`                 |

## Sources and the output

Everything that makes sound is a **source**: one function, `render(stereo, frames)`, fills frames of two `std::int16_t` (left, right) when it is asked and returns fewer frames than asked for when the sound has ended.
The **output** plays sources on a device:

| `sid::output`          |                                                                                                                                                               |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `open(sample_rate)`    | switches the device on. Open with another rate: the sound plays out first, then the device starts with the new rate. False if the device cannot make the rate |
| `play(source)`         | plays a source to its end; returns when all its frames are written (the device paces it)                                                                      |
| `play(source, ms)`     | plays a source for a time: for sources that never end (the synthesizer, a song that loops)                                                                    |
| `pause(ms)`            | silence                                                                                                                                                       |
| `set_volume(0 .. 255)` | volume of the device, false if it has none                                                                                                                    |
| `close()`              | lets the sound play out and switches the device off                                                                                                           |
| `rate()`, `dev()`    | the sample rate of the open output (0: not open), the device                                                                                                  |

A source is made for the sample rate of the output (`out.rate()`); nothing is resampled. A source of your own - a generator or an effect around another source - is a class with that one function, see [04_custom_source.cpp](examples/04_custom_source.cpp).

## Tones, melodies and recorded clips

```cpp
sid::beep(out, 440U, 400U, sid::wave_square, 64U);          // frequency [Hz], duration [ms], waveform, level 0 .. 255

static const sid::note fanfare[] = { { sid::g4, 140U }, { sid::c5, 140U }, { sid::e5, 140U }, { sid::g5, 280U }, { sid::rest, 60U } };
sid::play(out, fanfare, 5U, sid::wave_triangle, 160U);      // an array of { frequency, duration }

#include "clip.h"                                           // made by: python tools/wav2h.py clip.wav clip.h
sid::play_wav(out, clip, sizeof(clip));                     // opens the output with the sample rate of the file
```

Tones fade in and out over 4 ms, so they do not click. A frequency of half the sample rate or more is silent.
`sid::wav::parse()` reads RIFF / WAVE files with uncompressed PCM of 8 or 16 bit, mono or stereo; the samples are played from where they are, nothing is copied. `sid::pcm` is the source for samples without a file header.

## The synthesizer

`sid::synth` has `SID_VOICES` voices (4). `note_on(voice, patch, key)` starts a note, `note_off(voice)` lets it fade (its release). Keys count 12 per octave, `sid::key::a4` = 69 = 440 Hz. What a note sounds like is a **patch** - a plain struct, so the instruments of a tune are constant data:

| Field                                     | Unit            | What it does                                                                                                                                                                                         |
| ----------------------------------------- | --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `wave`                                  |                 | `osc_saw`, `osc_pulse`, `osc_triangle`, `osc_noise` (the key is its colour); several combined with `\|` give the AND of their values, the trick of the chip of the 80s                      |
| `level`                                 | 0 .. 255        | loudness                                                                                                                                                                                             |
| `attack`, `decay`, `release`        | ms              | time to rise to full level, to fall to the sustain level, to fall to silence after `note_off` (the falling parts are exponential)                                                                  |
| `sustain`                               | 0 .. 255        | level while the note is held; 0: the note ends by itself (drums, plucks)                                                                                                                             |
| `pulse`                                 | 8 .. 248 of 256 | width of the pulse, 128 = square; narrower or wider (also by `pwm`) is limited to 8 or 248                                                                                                         |
| `pwm`                                   | 0 .. 127 of 256 | depth of the pulse width modulation by the LFO                                                                                                                                                       |
| `lfo_rate`                              | 0.1 Hz          | rate of the LFO (a triangle)                                                                                                                                                                         |
| `lfo_delay`                             | 10 ms           | time until the vibrato sets in                                                                                                                                                                       |
| `vibrato`                               | cents           | depth of the pitch modulation by the LFO, up to 2400                                                                                                                                                 |
| `sweep`                                 | semitones / s   | pitch slide while the note sounds, negative = down (drums, lasers)                                                                                                                                   |
| `glide`                                 | ms              | time to slide from the last note of the voice to this one                                                                                                                                            |
| `arp[2]`, `arp_ms`                    | semitones, ms   | arpeggio: the note, `arp[0]` and `arp[1]` semitones above it, each for `arp_ms` - a chord on one voice                                                                                         |
| `mod`                                   |                 | `mod_ring`, `mod_sync`: ring modulation / hard sync by a second oscillator of the voice; `mod_ring_voice`, `mod_sync_voice`: by the oscillator of the voice before (voice 0: the last voice) |
| `mod_pitch`                             | semitones       | pitch of the second oscillator (`mod_ring`) or of the audible one (`mod_sync`) relative to the note                                                                                              |
| `filter`                                |                 | `filter_off`, `filter_low`, `filter_band`, `filter_high`; several combined with `\|` are summed, `filter_low \| filter_high` is a notch                                                    |
| `resonance`                             | 0 .. 255        | emphasis at the cutoff                                                                                                                                                                               |
| `cutoff`, `cutoff_end`, `cutoff_ms` | Hz, Hz, ms      | the cutoff when the note starts, where it moves to, and in which time (0: it stays)                                                                                                                  |

Things to know:

- There is one filter for all voices that ask for it; the last note that uses it sets it. The cutoff can go up to a third of the sample rate.
- The mix of the voices is clipped, not wrapped: the levels of the patches of voices that sound together should add up to about 255. A pulse swings around its mean, so a narrow one reaches higher than its level.
- A voice that modulates the voice behind it (`mod_ring_voice`, `mod_sync_voice`) must play a note; with a patch of level 0 it is not heard itself.
- Sawtooth and pulse are band limited. Combined waveforms have plain edges and are not as clean on high notes.
- Envelope, LFO and pitch are computed once per millisecond, the oscillators and the filter per frame.
- The synthesizer is a source that never ends: play it with `out.play(synth, ms)` between your `note_on` calls.
  `note_on` / `note_off` come from the thread or task that plays.

The examples [05](examples/05_patches.cpp) to [09](examples/09_arpeggio_chords.cpp) and [11](examples/11_sound_effects.cpp) are a tour of patches: leads, basses, pads, drums, bells, filters, effects.

## The sequencer

A song is written the way the tunes of the 80s were, in **patterns**. A pattern is one bar for one voice: an array of steps `{ key, patch }`. A step starts a note, ends the running one (`sid::key::off`) or lets it go on (`sid::key::hold`). The **order** of the song has one row per bar and names, for every track, the pattern it plays and a transposition - so a bass line is written once and moved through the chords. Track n plays on voice n.

```cpp
#include "sid/src/sequencer.h"

// the instruments (the bass of the quick start), one pattern of four steps, and the order: the bar in A, then in F
static const sid::patch patches[] = { make_bass() };
static const sid::step  line[4]   = { { sid::key::a2, 0U }, { sid::key::hold, 0U }, { sid::key::a3, 0U }, { sid::key::off, 0U } };
static const sid::step* const patterns[] = { line };
static const sid::slot  order[]   = { { 0U, 0 }, { 0U, -4 } };
static const sid::song  tune      = { patches, patterns, order, 2U, 1U, 4U, 250U };     // 2 rows, 1 track, 4 steps of 250 ms

sid::play(out, tune);                           // the song from its first to its last row
```

For a product the patches are a constant table, so they need no RAM and no start-up code - see [10_drum_pattern.cpp](examples/10_drum_pattern.cpp).

The sequencer is a source: it ends after the last row, when the last notes have faded (`SID_SONG_TAIL_MS` at most). `sid::sequencer(rate, song, loops)` plays a song several times, with 0 loops for ever - played in slices of time with `out.play(sequencer, ms)`, the app can start notes of its own in between on the voices the song leaves free (`sequencer::voices()`): music with sound effects, see [13_loop_and_notes.cpp](examples/13_loop_and_notes.cpp).

[`examples/data/make_invention13.py`](examples/data/make_invention13.py) shows how to turn written music into a song: it holds the notes of a two part invention by Bach in LilyPond notation, arranges them for four voices and writes the patterns.

## MOD player (extension)

[`src/ext/mod.h`](src/ext/mod.h) plays the music files of the Amiga: MOD files of ProTracker and its relatives with 4 channels and 31 samples (`M.K.`, `M!K!`, `FLT4`, `4CHN`). Like a WAV clip the file is played from where it is, in the flash or in RAM; nothing is copied. It is not part of `sid.h`, include it on its own:

```cpp
#include "sid/src/ext/mod.h"

sid::mod::info tune;
if (sid::mod::parse(file, size, tune) && out.open(22050U)) {      // false: no MOD file of 4 channels
  sid::mod::player music(out.rate(), tune);                       // a source; (rate, tune, loops, separation)
  out.play(music);                                                // or in slices: out.play(music, 100U)
}
sid::play_mod(out, file, size);                                   // the same in one line, on an open output
```

- **Effects**: `0` arpeggio, `1` / `2` portamento, `3` tone portamento, `4` vibrato, `5` / `6` those two with a volume slide, `9` sample offset, `A` volume slide, `B` position jump, `C` volume, `D` pattern break, `F` speed and tempo, `E1` / `E2` fine portamento, `EA` / `EB` fine volume slide, `EC` note cut, `ED` note delay. Others (tremolo, pattern loop, retrigger, ...) are ignored.
- **The end**: a song ends after its last position, or when it jumps back to a position that has played - the way most MOD files loop. `loops` plays it several times, 0 for ever; the sound fades out over 10 ms at the end.
- **Sound**: the samples are interpolated (linear); the channels are panned as on the Amiga (left, right, right, left), `separation` sets that from 0 (mono) to 255 (hard); 128 by default, easier on headphones. The mix never clips: two channels on a side are halved.
- `order()` and `row()` say where the song is, `mod::title()` and `mod::sample_of()` read the file for a display.
- A finetune other than 0 is computed, not taken from the 576 periods of ProTracker; a period can differ by 1 or 2 from its table (a few cents). Not supported: MOD files of 6, 8 or more channels and the old ones with 15 samples.

[14_mod_player.cpp](examples/14_mod_player.cpp) plays a file given on its command line (`--play` live on Windows); without one it plays a short tune it makes as a MOD file in memory.

## Mixer (extension)

The output plays one source after the other. [`src/ext/mixer.h`](src/ext/mixer.h) is a source that plays several at the same time, each with its level: music with sound effects, a clip with a tone on top.

```cpp
#include "sid/src/ext/mixer.h"

sid::sequencer music(out.rate(), tune, 0U);     // loops for ever
sid::tone      beep(out.rate());
sid::mixer<2>  mix;                             // up to 2 inputs
mix.add(music, 160U);                           // level 0 .. 255 (255: the source as it is)
for (;;) {
  if (button()) {
    beep.set(880U, 100U);
    mix.add(beep);                              // over the music; it leaves the mixer when it has ended
  }
  out.play(mix, 20U);
}
```

| `sid::mixer<N>`                 |                                                                                                                                       |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `add(source, level)`            | the source plays from now on. False if all N inputs are taken. A source that is an input already goes on as it is, with the new level |
| `remove(source)`, `clear()`   | takes it out, or all of them                                                                                                          |
| `set_level(source, level)`      | the level of an input, also while it plays: a fade, music that goes quiet for a message                                               |
| `playing(source)`, `inputs()` | true while the source is an input; the number of inputs                                                                               |

- **The end**: an input that has ended is taken out, so its place is free again. The mixer ends when its last input has ended. `sid::mixer<N> mix(true)` never ends: it plays silence while it has no input, for the loop of an app that adds a sound when something happens.
- **Levels**: the sum is clipped, not wrapped. The levels of inputs that are loud together should add up to about 255.
- The mixer does not own its inputs: a source must stay valid while it is an input. All inputs are made for the sample rate of the output. No heap: a mixer is N pointers and levels, `render()` needs `SID_CHUNK_FRAMES * 4` bytes of stack.

[17_mixer.cpp](examples/17_mixer.cpp) is a scene of a game: a song in the background, sound effects, and a ring tone for which the music goes quiet.

## Level meter (extension)

One sample says nothing about how loud the sound is: it swings around zero. [`src/ext/meter.h`](src/ext/meter.h) is a source around another source, which passes the sound on as it is and keeps its level: the peak of each channel, which rises at once and falls slowly - what a VU meter or a bar of LEDs shows.

```cpp
#include "sid/src/ext/meter.h"

sid::mixer<3>  mix(true);                       // or any other source: a song, a MOD player, the synthesizer
sid::meter     vu(out.rate(), mix);             // plays the mixer and measures it
for (;;) {
  out.play(vu, 20U);                            // the meter is played, not the mixer
  leds_left((vu.left() * 10U) / 255U);          // 10 LEDs per channel, the last one is on at full scale
  leds_right((vu.right() * 10U) / 255U);
}
```

| `sid::meter`                       |                                                                                      |
| ------------------------------------ | ------------------------------------------------------------------------------------ |
| `meter(rate, source, scale)`       | a meter around the source; the scale is `sid::meter_log` if none is given          |
| `left()`, `right()`, `level()` | the level of a channel, or the higher one of both: 0 .. 255 (full scale)             |
| `set_scale(scale)`, `scale()`    | the switch between `sid::meter_log` and `sid::meter_linear`, also while it plays |
| `set_range(dB)`                    | what the logarithmic scale shows: 48 dB by default, 1 .. 96                          |
| `set_fall(ms)`                     | the time in which the level falls by 20 dB: 300 ms by default, 0 = no fall           |
| `reset()`                          | the level is 0                                                                       |

- **Logarithmic** (`sid::meter_log`): the level is in decibels, as the ear hears. 255 is full scale and 0 is the range below it, so every step is the same number of decibels: with 48 dB and 8 LEDs, 6 dB per LED. Quiet sound and the tail of a note are seen as long as they are heard.
- **Linear** (`sid::meter_linear`): the level is the amplitude, half of it reads 128. Only loud sound moves the bar - right for a light that pulses with the beat.
  | amplitude  |        | linear | logarithmic, 48 dB |
  | ---------- | ------ | ------ | ------------------ |
  | full scale | 0 dB   | 255    | 255                |
  | 1/2        | -6 dB  | 128    | 223                |
  | 1/4        | -12 dB | 64     | 191                |
  | 1/16       | -24 dB | 16     | 127                |
  | 1/256      | -48 dB | 1      | 0                  |
- **The switch**: both scales read 0 .. 255 from the same measurement. `set_scale()` changes how the level is read, at once and without a new frame, so what shows the level stays the same.
- **Reading**: `left()`, `right()` and `level()` do not change the level, so it is read at the pace of the display, and no peak between two readings is lost. When the source has ended, the level is 0.
- **Timing**: the meter sees the frames when they are written, so the level is ahead of the sound by what the device buffers - a few milliseconds on a DAC, more on a PC.
- It is the level of the source: the volume of the device (`set_volume`) comes after it. The meter does not own its source, which must stay valid while the meter plays. No heap: a meter is 32 bytes.

[18_meter.cpp](examples/18_meter.cpp) plays sound effects through a meter and prints the level as a bar in both scales.

## Ring tones: RTTTL (extension)

[`src/ext/rtttl.h`](src/ext/rtttl.h) plays RTTTL, the ring tones of the mobile phones of the 90s. A tune is one line of text - a string constant, and thousands of them can be found. It is played from where it is; no table of notes is made.

```cpp
#include "sid/src/ext/rtttl.h"

sid::play_rtttl(out, "scale:d=4,o=5,b=120:c,d,e,f,g,a,b,2c6", sid::wave_square);     // on an open output

sid::rtttl::info tune;
if (sid::rtttl::parse(text, tune)) {                    // false: no RTTTL
  sid::rtttl::player music(out.rate(), tune);           // a source; (rate, tune, loops, wave, level)
  out.play(music);
}
```

- **The text**: `name:defaults:notes`. The defaults are `d` (duration: 1, 2, 4, 8, 16 or 32 for a whole note to a 32nd), `o` (octave, a4 is 440 Hz) and `b` (beats per minute, a beat is a quarter note); what is not given is 4, 6 and 63. A note is `[duration] note [#] [.] [octave]`: `8c#6` is an eighth of c sharp in octave 6, `p` is a pause, a dot makes a note half as long again. Upper and lower case are the same, blanks are skipped.
- An entry that is no note is left out; a text without the two colons or with a default that is none is refused by `parse()`. The standard has the octaves 4 to 7, the player takes 0 to 9.
- The notes are those of `sid::tone`: a tune sounds as a sine, square or triangle wave. `loops` plays it several times, 0 for ever (a ring until somebody answers).
- `rtttl::length(tune)` is its duration in ms, `tune.name` and `tune.name_length` its name for a display. The parts of a ms of the notes are carried on, so a tune is as long as its notes say.

[15_rtttl.cpp](examples/15_rtttl.cpp) plays three pieces written as RTTTL.

## Sound effects (extension)

[`src/ext/sfx.h`](src/ext/sfx.h) has the sounds a device and a game need, ready to play. An effect is a patch of the synthesizer and up to four notes of it: 50 bytes of constant data, no samples. Only the effects a program uses get into it.

| For              | Effects (`sid::sfx::`)                                                               |
| ---------------- | -------------------------------------------------------------------------------------- |
| a user interface | `click`, `blip`, `confirm`, `error`, `alarm`                                 |
| a game           | `coin`, `jump`, `laser`, `hit`, `explosion`, `power_up`, `win`, `lose` |

```cpp
#include "sid/src/ext/sfx.h"

sid::play_sfx(out, sid::sfx::confirm);          // one effect to its end, on an open output

sid::sfx::player fx(out.rate());                // a source: up to SID_VOICES (4) effects at the same time
fx.play(sid::sfx::coin);
fx.play(sid::sfx::laser, -5);                   // five semitones lower
out.play(fx, 20U);                              // in the loop of the app - or the player is an input of a mixer
```

- The player is a source that never ends, like the synthesizer it contains: it is silent when nothing plays, and `silent()` says so. `stop()` ends all effects.
- A new effect takes a voice without an effect; when all are busy, the oldest effect ends for it.
- Each effect reaches about half of full scale, so two of them, or one over music, fit without clipping.
- An effect of your own is one more constant - a patch and its notes `{ key, ms }`:
  `static const sid::sfx::effect chime = { bell, { { sid::key::e5, 350U }, { sid::key::c5, 900U } } };`

[16_sfx.cpp](examples/16_sfx.cpp) plays every effect, then several at once. How such patches are made is shown in [11_sound_effects.cpp](examples/11_sound_effects.cpp).

## Porting: the device

The only thing sid needs from a platform is a `sid::device`:

```cpp
class my_output : public sid::device
{
public:
  virtual bool start(std::uint32_t sample_rate);                        // output on; false: no output, or it cannot make the rate
  virtual bool write(const std::int16_t* stereo, std::size_t frames);   // take frames (L, R, L, R ...); wait while the output is full
  virtual void stop();                                                  // let the sound play out, then output off
  virtual bool set_volume(std::uint32_t volume);                        // optional: 0 .. 255
};
```

`write()` is where the pace comes from: it returns when the frames are taken, so a full output makes the caller wait. A mono output plays the mean of left and right.

| Ready device                                  | For                                                                            |
| --------------------------------------------- | ------------------------------------------------------------------------------ |
| [`port/wav_file.h`](src/port/wav_file.h)     | a WAV file - anywhere with a file system; the device of the examples and of CI |
| [`port/win32.h`](src/port/win32.h)           | the sound output of a Windows PC (waveOut), live                               |
| [`port/pcm_stream.h`](src/port/pcm_stream.h) | raw PCM on a C stream, e.g. stdout piped into `aplay`, `ffplay` or `sox` |

**On a microcontroller** the usual shape is a ring buffer between `write()` and the interrupt of the output. This is a sketch of it, not built or tested here - the names of the hardware functions are placeholders:

```cpp
class i2s_output : public sid::device
{
public:
  virtual bool start(std::uint32_t sample_rate)
  {
    return i2s_set_clock(sample_rate) && i2s_start_dma(dma_buffer_, dma_frames);    // circular DMA, two halves
  }

  virtual bool write(const std::int16_t* stereo, std::size_t frames)
  {
    while (frames != 0U) {
      const std::size_t n = ring_.push(stereo, frames);     // as much as fits
      stereo += n * 2U;
      frames -= n;
      if (frames != 0U) {
        wait_for_interrupt_or_delay();                      // an RTOS: block on a semaphore the interrupt gives
      }
    }
    return true;
  }

  virtual void stop()
  {
    while (!ring_.empty()) { wait_for_interrupt_or_delay(); }
    i2s_stop_dma();
  }

  // called by the DMA interrupt when a half of its buffer has been played: refill that half
  void on_half_played(std::int16_t* half)
  {
    const std::size_t n = ring_.pop(half, dma_frames / 2U);
    std::memset(&half[n * 2U], 0, ((dma_frames / 2U) - n) * 4U);      // silence when the ring is empty
  }
};
```

A 1 bit output (PDM, sigma-delta, PWM) is the same with another peripheral behind it.

## Options

Defined before the headers are included (compiler option or project configuration):

| Option               | Default |                                                                                                     |
| -------------------- | ------- | --------------------------------------------------------------------------------------------------- |
| `SID_CHUNK_FRAMES` | 64      | frames rendered and written in one step; `output::play()` needs four times that in bytes of stack |
| `SID_VOICES`       | 4       | voices of the synthesizer                                                                           |
| `SID_SONG_TAIL_MS` | 2000    | longest time the last notes of a song may take to fade                                              |

## Footprint

Measured with `arm-none-eabi-g++ -Os` 15.3, code and constant data of the library without the application's data ([`tools/embedded_size.sh`](tools/embedded_size.sh), also a job of CI, which prints the table for every commit):

| Part                       | Cortex-M0  | Cortex-M4  |
| -------------------------- | ---------- | ---------- |
| tones and melodies         | 848 bytes  | 830 bytes  |
| WAV clips                  | 804 bytes  | 698 bytes  |
| synthesizer                | 2762 bytes | 2426 bytes |
| synthesizer with sequencer | 3358 bytes | 2922 bytes |
| all of it                  | 4908 bytes | 4314 bytes |

A Cortex-M0 has no divider and no 64 bit multiplication; the routines of the compiler library for that come on top.

RAM, all of it in the objects you create (no heap, no static data): `sid::output` 8 bytes, `sid::tone` 36, `sid::melody` 60, `sid::pcm` 16, `sid::synth` 440, `sid::sequencer` 476 (it contains its synth); a patch is 34 bytes of constant data. `output::play()` needs 256 bytes of stack.

The extensions (mixer, level meter, RTTTL player, sound effects, MOD player) are not in "all of it": `tools/embedded_size.sh` (and CI) prints the code size of each as its own line, and the RAM of their objects.

The CPU load on a microcontroller has not been measured yet. The work per frame is small - per voice a phase accumulator, a waveform and one multiplication, plus the filter - and the output of the examples is at 22050 Hz.

## Examples

Each example is one small program in [`examples/`](examples). It writes `<name>.wav`; from 01 on it plays live with `--play` (Windows) and writes raw samples with `--stdout`.

| Example                                                    | Shows                                                                                                    |
| ---------------------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| [00_hello](examples/00_hello.cpp)                           | the quick start of this readme as a program                                                              |
| [01_beep](examples/01_beep.cpp)                             | the smallest use: single tones in three waveforms                                                        |
| [02_melody](examples/02_melody.cpp)                         | melodies as arrays of notes                                                                              |
| [03_wav_clip](examples/03_wav_clip.cpp)                     | a recorded clip from a header made by `tools/wav2h.py`                                                 |
| [04_custom_source](examples/04_custom_source.cpp)           | sources of your own: a siren (a generator) and an echo (an effect around another source)                 |
| [05_patches](examples/05_patches.cpp)                       | the synthesizer played by hand: lead, bass, pluck, pad, drums                                            |
| [06_filter](examples/06_filter.cpp)                         | low, band, high pass and notch, sweeps, the "wow" of a bass                                              |
| [07_ring_and_sync](examples/07_ring_and_sync.cpp)           | bells and a screaming lead; modulation by the voice before                                               |
| [08_combined_waveforms](examples/08_combined_waveforms.cpp) | the combined waveforms, still and moving                                                                 |
| [09_arpeggio_chords](examples/09_arpeggio_chords.cpp)       | a chord progression on one voice                                                                         |
| [10_drum_pattern](examples/10_drum_pattern.cpp)             | the sequencer: patterns, the order, a transposed bass line                                               |
| [11_sound_effects](examples/11_sound_effects.cpp)           | laser, explosion, coin, jump, power up, alarm, engine                                                    |
| [12_song](examples/12_song.cpp)                             | a whole piece on four voices: Bach, Invention 13 in A minor                                              |
| [13_loop_and_notes](examples/13_loop_and_notes.cpp)         | music that loops for ever with sound effects of the app on top                                           |
| [14_mod_player](examples/14_mod_player.cpp)                 | a MOD file from the command line, or a tune made in memory; title, samples and position while it plays   |
| [15_rtttl](examples/15_rtttl.cpp)                           | ring tones as text: three pieces in three waveforms, name and length of a tune                           |
| [16_sfx](examples/16_sfx.cpp)                               | the ready sound effects, one by one and several at once; an effect of your own                           |
| [17_mixer](examples/17_mixer.cpp)                           | a mixer: a song, sound effects and a ring tone at the same time, levels that change                      |
| [18_meter](examples/18_meter.cpp)                           | a level meter: sound effects through a meter, the level as a bar in the logarithmic and the linear scale |

## Tests

```sh
cmake -S . -B build && cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build/test/sid_test --filter sid_synth       # the cases whose name starts with that
```

55 test cases with about 10500 checks, in a harness of 100 lines without a framework ([`test/test.h`](test/test.h)).  
The sound is measured, not compared with stored samples: pitch by the distance of the zero crossings, loudness by the peak, colour by the power at single frequencies. Every example runs as a test too. CI builds and runs all of it on Linux (GCC, Clang), Windows (MSVC) and macOS with C++11 and C++20, and compiles the library for Cortex-M0 and Cortex-M4 without exceptions and run time type information.

## Tools

| Tool                                                                      |                                                                                         |
| ------------------------------------------------------------------------- | --------------------------------------------------------------------------------------- |
| [`tools/wav2h.py`](tools/wav2h.py)                                       | a WAV file as a C++ header with a byte array: `python tools/wav2h.py clip.wav clip.h` |
| [`tools/embedded_size.sh`](tools/embedded_size.sh)                       | compiles the library for Cortex-M and prints the footprint table                        |
| [`examples/data/make_invention13.py`](examples/data/make_invention13.py) | written music (LilyPond notation) to a song for the sequencer                           |
| [`examples/data/make_clip.py`](examples/data/make_clip.py)               | computes the recorded clip of the examples (plucked strings)                            |

## Releases

Releases are made by [release-please](https://github.com/googleapis/release-please): it reads the commit messages on `main` and keeps a pull request open with the next version and its part of the [changelog](CHANGELOG.md). Merging that pull request tags the version and creates the GitHub release.

| Commit message                               |                                             | Next version            |
| -------------------------------------------- | ------------------------------------------- | ----------------------- |
| `fix: ...`                                   | a bug fix                                   | patch: 1.0.0 to 1.0.1   |
| `feat: ...`                                  | a new feature                               | minor: 1.0.0 to 1.1.0   |
| `feat!: ...`, or a footer `BREAKING CHANGE:` | a change that breaks code using the library | major: 1.0.0 to 2.0.0   |

Other types (`docs:`, `style:`, `refactor:`, `test:`, `ci:`, `chore:`) make no release.

## Credits

The song of the examples is J. S. Bach's Invention 13 in A minor (BWV 784), which is in the public domain. Its notes are those of the [Mutopia Project](https://www.mutopiaproject.org) edition (Mutopia-2008/06/15-59), typeset by Allen Garvin after the Bach-Gesellschaft edition and placed in the public domain by the typesetter. The arrangement for four voices is part of this project.

The ring tones of example 15 are Beethoven's Fuer Elise and Ode to Joy and the Minuet in G of the notebook for Anna Magdalena Bach (Christian Petzold), all in the public domain. They were written as RTTTL for this project.

"SID", "Commodore" and "MOS" are named here to describe the kind of sound; this project is not affiliated with their owners.

## License

SID is written by Marco Paland (info@paland.design) and published under the [MIT license](LICENSE).
