///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.design)
//             2026, PALANDesign, LLC
//
// \license The MIT License (MIT)
//
// This file is part of the sid library.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief sid tests: the library on a microcontroller
//        Not a program: this file is compiled for a bare Cortex-M (no operating system, no exceptions, no run
//        time type information) to show that the library needs nothing of a host, and to measure it. CI does:
//          arm-none-eabi-g++ -std=c++11 -mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections
//                            -fno-exceptions -fno-rtti -Wall -Wextra -Wshadow -Wpedantic -Werror
//                            -DSID_EMBEDDED_PART=<n> -c test/embedded.cpp
//          arm-none-eabi-size embedded.o       code of the part (text + data), without the divisions and 64 bit
//                                              multiplications a Cortex-M0 takes from the compiler library
//        SID_EMBEDDED_PART: 1 = tones and melodies, 2 = WAV clips, 3 = synthesizer, 4 = synthesizer with the
//        sequencer, 0 = all of it. The extensions are not in 'all of it': 5 = the MOD player (ext/mod.h),
//        6 = the mixer with two tones (ext/mixer.h), 7 = the RTTTL player (ext/rtttl.h), 8 = two sound effects
//        (ext/sfx.h), 9 = the level meter around a tone (ext/meter.h). With -DSID_EMBEDDED_SIZES the sizes of the
//        objects (RAM) are in the symbols size_of_... (tools/embedded_size.sh prints all of it as a table).
//
///////////////////////////////////////////////////////////////////////////////

#include "../src/sid.h"
#include "../src/ext/mod.h"
#include "../src/ext/meter.h"
#include "../src/ext/mixer.h"
#include "../src/ext/rtttl.h"
#include "../src/ext/sfx.h"

#ifndef SID_EMBEDDED_PART
#define SID_EMBEDDED_PART   0
#endif

#if defined(SID_EMBEDDED_SIZES)
// RAM of the objects: the size of each array is the size of the class (arm-none-eabi-nm -S)
extern const char size_of_output[sizeof(sid::output)];
extern const char size_of_tone[sizeof(sid::tone)];
extern const char size_of_melody[sizeof(sid::melody)];
extern const char size_of_pcm[sizeof(sid::pcm)];
extern const char size_of_synth[sizeof(sid::synth)];
extern const char size_of_sequencer[sizeof(sid::sequencer)];
extern const char size_of_patch[sizeof(sid::patch)];
extern const char size_of_mod_player[sizeof(sid::mod::player)];
extern const char size_of_mixer_4[sizeof(sid::mixer<4>)];
extern const char size_of_rtttl_player[sizeof(sid::rtttl::player)];
extern const char size_of_sfx_player[sizeof(sid::sfx::player)];
extern const char size_of_sfx_effect[sizeof(sid::sfx::effect)];
extern const char size_of_meter[sizeof(sid::meter)];
const char size_of_output[sizeof(sid::output)]       = { 0 };
const char size_of_tone[sizeof(sid::tone)]           = { 0 };
const char size_of_melody[sizeof(sid::melody)]       = { 0 };
const char size_of_pcm[sizeof(sid::pcm)]             = { 0 };
const char size_of_synth[sizeof(sid::synth)]         = { 0 };
const char size_of_sequencer[sizeof(sid::sequencer)] = { 0 };
const char size_of_patch[sizeof(sid::patch)]         = { 0 };
const char size_of_mod_player[sizeof(sid::mod::player)] = { 0 };
const char size_of_mixer_4[sizeof(sid::mixer<4>)]       = { 0 };
const char size_of_rtttl_player[sizeof(sid::rtttl::player)] = { 0 };
const char size_of_sfx_player[sizeof(sid::sfx::player)] = { 0 };
const char size_of_sfx_effect[sizeof(sid::sfx::effect)] = { 0 };
const char size_of_meter[sizeof(sid::meter)]           = { 0 };
#endif

// what an application has: a device, a clip, patches and a song
extern const std::uint8_t   embedded_clip[];
extern const std::size_t    embedded_clip_size;
extern const sid::note      embedded_notes[];
extern const sid::patch     embedded_patches[];
extern const sid::song      embedded_song;
extern const std::uint8_t   embedded_mod[];
extern const std::size_t    embedded_mod_size;
extern const char           embedded_rtttl[];
extern std::uint32_t        embedded_level;       // what the display of the application shows

bool sid_embedded(sid::device& dev);

bool sid_embedded(sid::device& dev)
{
  sid::output out(dev);
  bool ok = out.open(22050U);
#if (SID_EMBEDDED_PART == 0) || (SID_EMBEDDED_PART == 1)
  ok = ok && sid::beep(out, 880U, 150U);
  ok = ok && sid::play(out, embedded_notes, 8U, sid::wave_triangle);
#endif
#if (SID_EMBEDDED_PART == 0) || (SID_EMBEDDED_PART == 2)
  ok = ok && sid::play_wav(out, embedded_clip, embedded_clip_size);
#endif
#if (SID_EMBEDDED_PART == 0) || (SID_EMBEDDED_PART == 3)
  sid::synth synth(out.rate());
  synth.note_on(0U, embedded_patches[0], sid::key::a4);
  ok = ok && out.play(synth, 500U);
#endif
#if (SID_EMBEDDED_PART == 0) || (SID_EMBEDDED_PART == 4)
  ok = ok && sid::play(out, embedded_song);
#endif
#if (SID_EMBEDDED_PART == 5)
  ok = ok && sid::play_mod(out, embedded_mod, embedded_mod_size);
#endif
#if (SID_EMBEDDED_PART == 6)
  sid::tone low(out.rate());
  sid::tone high(out.rate());
  low.set(440U, 500U);
  high.set(660U, 300U);
  sid::mixer<2> mix;
  ok = ok && mix.add(low, 200U) && mix.add(high) && out.play(mix);
#endif
#if (SID_EMBEDDED_PART == 7)
  ok = ok && sid::play_rtttl(out, embedded_rtttl);
#endif
#if (SID_EMBEDDED_PART == 8)
  ok = ok && sid::play_sfx(out, sid::sfx::coin) && sid::play_sfx(out, sid::sfx::explosion);
#endif
#if (SID_EMBEDDED_PART == 9)
  sid::tone note(out.rate());
  note.set(440U, 500U);
  sid::meter vu(out.rate(), note);
  ok = ok && out.play(vu, 100U);
  embedded_level = vu.left() + vu.right();      // in decibels
  vu.set_scale(sid::meter_linear);
  embedded_level += vu.level();                 // as the amplitude
#endif
  out.close();
  return ok;
}
