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
// \brief sid example 09: chords on one voice
//        With three voices there is no room for chords, so the tunes of the 80s played them as arpeggios: one
//        voice runs through the notes of the chord so fast that the ear hears them together. A patch does it by
//        itself: arp[0] and arp[1] are the semitones of the second and third note above the key, arp_ms the time
//        of a step. A chord progression on voice 0, a bass on voice 1 - the whole harmony on two voices.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/synth.h"

#include "common.h"


static sid::patch chord(std::uint8_t second, std::uint8_t third, std::uint8_t step_ms)
{
  sid::patch p = { };
  p.wave     = sid::osc_pulse;
  p.level    = 90U;
  p.attack   = 4U;
  p.decay    = 500U;
  p.sustain  = 120U;
  p.release  = 250U;
  p.pulse    = 72U;
  p.pwm      = 40U;
  p.lfo_rate = 6U;
  p.arp[0]   = second;
  p.arp[1]   = third;
  p.arp_ms   = step_ms;
  return p;
}

static sid::patch make_bass()
{
  sid::patch p = { };
  p.wave    = sid::osc_triangle;
  p.level   = 150U;
  p.attack  = 3U;
  p.decay   = 300U;
  p.sustain = 140U;
  p.release = 100U;
  return p;
}

static const sid::patch chord_minor = chord(3U, 7U, 20U);     // root, minor third, fifth; 50 notes per second
static const sid::patch chord_major = chord(4U, 7U, 20U);
static const sid::patch broken_minor = chord(3U, 7U, 110U);     // slow: a broken chord, the notes are heard one by one
static const sid::patch broken_major = chord(4U, 7U, 110U);
static const sid::patch octaves    = chord(12U, 24U, 30U);    // not a chord: three octaves, a shimmer
static const sid::patch bass       = make_bass();

// A minor, F, C, G
typedef struct tag_bar {
  std::uint8_t  key;
  bool          is_minor;
  std::uint8_t  bass_key;
} bar;

static const bar bars[] = {
  { sid::key::a4, true,  sid::key::a2 },
  { sid::key::f4, false, sid::key::f2 },
  { sid::key::c5, false, sid::key::c3 },
  { sid::key::g4, false, sid::key::g2 }
};


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::synth synth(out.rate());
  bool ok = true;

  // the progression as fast arpeggios, then as broken chords, then the roots in octaves
  for (std::uint32_t round = 0U; ok && (round < 3U); ++round) {
    for (std::size_t i = 0U; ok && (i < (sizeof(bars) / sizeof(bars[0]))); ++i) {
      const sid::patch& sound = (round == 2U) ? octaves
                              : (round == 1U) ? (bars[i].is_minor ? broken_minor : broken_major)
                              :                 (bars[i].is_minor ? chord_minor : chord_major);
      synth.note_on(0U, sound, bars[i].key);
      synth.note_on(1U, bass, bars[i].bass_key);
      ok = ok && out.play(synth, 1300U);
    }
  }
  synth.all_off();
  ok = ok && out.play(synth, 600U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "09_arpeggio_chords", &play);
}
