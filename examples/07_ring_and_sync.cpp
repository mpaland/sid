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
// \brief sid example 07: ring modulation and hard sync
//        Two ways to get sounds no single waveform has:
//          ring modulation   the voice is multiplied by a second oscillator at another pitch: sums and
//                            differences of the two pitches, bells and metal
//          hard sync         a second oscillator at the note restarts the audible one, which runs higher: the
//                            note stays, the colour screams - more so when the vibrato moves it
//        The second oscillator is either part of the voice (mod_ring, mod_sync, its pitch in mod_pitch) or the
//        voice before it, as in the chip of the home computer (mod_ring_voice, mod_sync_voice). That voice plays
//        a note of its own; with a patch of level 0 it is not heard.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/synth.h"

#include "common.h"


// a bell: triangle times a triangle 7 semitones and a bit above, struck and fading
static sid::patch make_bell()
{
  sid::patch p = { };
  p.wave      = sid::osc_triangle;
  p.level     = 220U;
  p.attack    = 1U;
  p.decay     = 1200U;
  p.release   = 600U;
  p.mod       = sid::mod_ring;
  p.mod_pitch = 7;          // [semitones] of the second oscillator above the note
  return p;
}

// a sync lead: the sawtooth runs an octave above the note and is swept two more by the slow LFO
static sid::patch make_scream()
{
  sid::patch p = { };
  p.wave      = sid::osc_saw;
  p.level     = 120U;
  p.attack    = 6U;
  p.decay     = 250U;
  p.sustain   = 180U;
  p.release   = 200U;
  p.mod       = sid::mod_sync;
  p.mod_pitch = 12;
  p.lfo_rate  = 6U;         // 0.6 Hz
  p.vibrato   = 1200U;      // [cents]: with sync it moves the colour, not the note
  return p;
}

// the same two with the voice before as the second oscillator
static sid::patch make_ring_by_voice()
{
  sid::patch p = { };
  p.wave    = sid::osc_triangle;
  p.level   = 200U;
  p.attack  = 2U;
  p.sustain = 255U;
  p.release = 300U;
  p.mod     = sid::mod_ring_voice;
  return p;
}

static sid::patch make_sync_by_voice()
{
  sid::patch p = { };
  p.wave    = sid::osc_saw;
  p.level   = 120U;
  p.attack  = 4U;
  p.sustain = 255U;
  p.release = 200U;
  p.glide   = 600U;         // the audible oscillator slides, the voice before holds the note
  p.mod     = sid::mod_sync_voice;
  return p;
}

// the modulator: a voice that runs but is not heard. Its waveform is the colour of the ring modulation
static sid::patch make_silent(std::uint8_t wave)
{
  sid::patch p = { };
  p.wave    = wave;
  p.level   = 0U;
  p.sustain = 255U;
  p.pulse   = 128U;
  return p;
}

static const sid::patch bell            = make_bell();
static const sid::patch scream          = make_scream();
static const sid::patch ring_by_voice   = make_ring_by_voice();
static const sid::patch sync_by_voice   = make_sync_by_voice();
static const sid::patch silent_triangle = make_silent(sid::osc_triangle);
static const sid::patch silent_square   = make_silent(sid::osc_pulse);
static const sid::patch silent_saw      = make_silent(sid::osc_saw);


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::synth synth(out.rate());
  bool ok = true;

  // bells
  static const std::uint8_t chime[] = { sid::key::e5, sid::key::c5, sid::key::d5, sid::key::g4, sid::key::g4, sid::key::d5, sid::key::e5, sid::key::c5 };
  for (std::size_t i = 0U; ok && (i < sizeof(chime)); ++i) {
    synth.note_on(i % 2U, bell, chime[i]);      // two voices in turn, so a bell rings on under the next
    ok = ok && out.play(synth, 450U);
  }
  ok = ok && out.play(synth, 1500U);

  // the sync lead
  static const std::uint8_t tune[] = { sid::key::a3, sid::key::c4, sid::key::e4, sid::key::a4, sid::key::g4, sid::key::e4, sid::key::a3 };
  for (std::size_t i = 0U; ok && (i < sizeof(tune)); ++i) {
    synth.note_on(0U, scream, tune[i]);
    ok = ok && out.play(synth, 420U);
  }
  synth.note_off(0U);
  ok = ok && out.play(synth, 600U);

  // ring modulation by the voice before: voice 0 is the silent modulator, voice 1 is heard. First a soft
  // (triangle), then a hard (square) modulator, each moving through three pitches under a held note
  const sid::patch* const colours[] = { &silent_triangle, &silent_square };
  static const std::uint8_t under[] = { sid::key::e5, sid::key::g5, sid::key::b5 };
  for (std::size_t c = 0U; ok && (c < 2U); ++c) {
    synth.note_on(1U, ring_by_voice, sid::key::a4);
    for (std::size_t i = 0U; ok && (i < sizeof(under)); ++i) {
      synth.note_on(0U, *colours[c], under[i]);
      ok = ok && out.play(synth, 500U);
    }
    synth.all_off();
    ok = ok && out.play(synth, 500U);
  }

  // sync by the voice before: voice 0 holds a2, voice 1 slides from a2 up two octaves and is restarted by it
  synth.note_on(0U, silent_saw, sid::key::a2);
  synth.note_on(1U, sync_by_voice, sid::key::a2);
  ok = ok && out.play(synth, 300U);
  synth.note_on(1U, sync_by_voice, sid::key::a4);
  ok = ok && out.play(synth, 1200U);
  synth.note_on(1U, sync_by_voice, sid::key::a2);
  ok = ok && out.play(synth, 1200U);
  synth.all_off();
  ok = ok && out.play(synth, 500U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "07_ring_and_sync", &play);
}
