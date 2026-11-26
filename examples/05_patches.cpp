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
// \brief sid example 05: the synthesizer and its patches
//        A patch is the sound of a note: waveform, envelope and what moves while the note sounds. This example
//        plays the synthesizer by hand - note_on(voice, patch, key), then the output plays the synth for a time -
//        with a tour of typical patches: a lead, a bass, a pluck, a pad, and drums.
//        The patches are built field by field here, to name every value. In a product they are constant tables
//        (see data/invention13.h), so they need no RAM.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/synth.h"

#include "common.h"


// a singing lead: a pulse that is not quite square, a little width modulation, vibrato after a moment
static sid::patch make_lead()
{
  sid::patch p = { };
  p.wave      = sid::osc_pulse;
  p.level     = 110U;
  p.attack    = 8U;         // [ms]
  p.decay     = 200U;
  p.sustain   = 170U;       // of 255
  p.release   = 180U;
  p.pulse     = 100U;       // of 256
  p.pwm       = 24U;
  p.lfo_rate  = 55U;        // 5.5 Hz
  p.lfo_delay = 18U;        // the vibrato sets in after 180 ms
  p.vibrato   = 22U;        // [cents]
  p.glide     = 40U;        // slides to the next note in 40 ms
  return p;
}

// a bass: sawtooth through the low pass, which closes with every note
static sid::patch make_bass()
{
  sid::patch p = { };
  p.wave       = sid::osc_saw;
  p.level      = 150U;
  p.attack     = 2U;
  p.decay      = 160U;
  p.sustain    = 110U;
  p.release    = 60U;
  p.filter     = sid::filter_low;
  p.resonance  = 170U;
  p.cutoff     = 2200U;     // [Hz] at the start of the note
  p.cutoff_end = 350U;      // where it goes
  p.cutoff_ms  = 140U;      // in this time
  return p;
}

// a pluck: no sustain, the note ends by itself
static sid::patch make_pluck()
{
  sid::patch p = { };
  p.wave    = sid::osc_triangle;
  p.level   = 200U;
  p.attack  = 1U;
  p.decay   = 350U;
  p.sustain = 0U;
  p.release = 100U;
  return p;
}

// a pad: a whole chord on one voice by a fast arpeggio (the note, 4 and 7 semitones above it), slow width modulation
static sid::patch make_pad()
{
  sid::patch p = { };
  p.wave     = sid::osc_pulse;
  p.level    = 90U;
  p.attack   = 60U;
  p.decay    = 400U;
  p.sustain  = 150U;
  p.release  = 400U;
  p.pulse    = 64U;
  p.pwm      = 48U;
  p.lfo_rate = 8U;          // 0.8 Hz
  p.arp[0]   = 4U;
  p.arp[1]   = 7U;
  p.arp_ms   = 20U;
  return p;
}

// drums: a triangle that falls fast (kick), noise that fades (snare), a tick of high noise (hat)
static sid::patch make_kick()
{
  sid::patch p = { };
  p.wave    = sid::osc_triangle;
  p.level   = 230U;
  p.decay   = 140U;
  p.release = 40U;
  p.sweep   = -300;         // [semitones / s]
  return p;
}

static sid::patch make_snare()
{
  sid::patch p = { };
  p.wave    = sid::osc_noise;
  p.level   = 140U;
  p.decay   = 110U;
  p.release = 40U;
  p.sweep   = -60;
  return p;
}

static sid::patch make_hat()
{
  sid::patch p = { };
  p.wave    = sid::osc_noise;
  p.level   = 70U;
  p.decay   = 35U;
  p.release = 20U;
  return p;
}

// a patch must live as long as its notes sound
static const sid::patch lead  = make_lead();
static const sid::patch bass  = make_bass();
static const sid::patch pluck = make_pluck();
static const sid::patch pad   = make_pad();
static const sid::patch kick  = make_kick();
static const sid::patch snare = make_snare();
static const sid::patch hat   = make_hat();


// notes one after the other on voice 0: each held for 'on' ms, then released for 'off' ms
static bool phrase(sid::output& out, sid::synth& synth, const sid::patch& sound, const std::uint8_t* keys,
                   std::size_t count, std::uint32_t on, std::uint32_t off)
{
  bool ok = true;
  for (std::size_t i = 0U; i < count; ++i) {
    synth.note_on(0U, sound, keys[i]);
    ok = ok && out.play(synth, on);       // the synth never ends: it is played for a time
    synth.note_off(0U);
    ok = ok && out.play(synth, off);
  }
  return ok;
}


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::synth synth(out.rate());
  bool ok = true;

  static const std::uint8_t melody[] = { sid::key::e5, sid::key::g5, sid::key::a5, sid::key::g5, sid::key::e5, sid::key::c5, sid::key::d5 };
  ok = ok && phrase(out, synth, lead, melody, sizeof(melody), 320U, 30U);
  ok = ok && out.play(synth, 500U);

  static const std::uint8_t line[] = { sid::key::a2, sid::key::a3, sid::key::a2, sid::key::e3, sid::key::g2, sid::key::g3, sid::key::c3, sid::key::e3 };
  ok = ok && phrase(out, synth, bass, line, sizeof(line), 200U, 20U);
  ok = ok && out.play(synth, 400U);

  static const std::uint8_t harp[] = { sid::key::c4, sid::key::e4, sid::key::g4, sid::key::c5, sid::key::e5, sid::key::g5, sid::key::c6 };
  ok = ok && phrase(out, synth, pluck, harp, sizeof(harp), 140U, 0U);
  ok = ok && out.play(synth, 800U);

  static const std::uint8_t chords[] = { sid::key::a3, sid::key::f3, sid::key::c4, sid::key::g3 };
  ok = ok && phrase(out, synth, pad, chords, sizeof(chords), 900U, 100U);
  ok = ok && out.play(synth, 800U);

  // drums: the key is the pitch of the kick and the colour of the noise
  for (std::uint32_t beat = 0U; ok && (beat < 8U); ++beat) {
    synth.note_on(0U, ((beat % 2U) == 0U) ? kick : snare, ((beat % 2U) == 0U) ? sid::key::g3 : sid::key::c5);
    ok = ok && out.play(synth, 200U);
    synth.note_on(0U, hat, sid::key::e7);
    ok = ok && out.play(synth, 200U);
  }
  ok = ok && out.play(synth, 300U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "05_patches", &play);
}
