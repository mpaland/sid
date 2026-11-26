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
// \brief sid example 11: sound effects
//        The effects of the games of the 80s are single notes of patches that move: a pitch that falls or
//        rises (sweep), noise that fades, a fast arpeggio. An effect needs one voice for a moment, so it can be
//        played over music on a voice the song leaves free (see 13_loop_and_notes.cpp).
//        Laser, explosion, coin, jump, power up, alarm and an engine.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/synth.h"

#include "common.h"


// a laser: a pulse that falls four octaves in a quarter of a second
static sid::patch make_laser()
{
  sid::patch p = { };
  p.wave    = sid::osc_pulse;
  p.level   = 150U;
  p.decay   = 250U;
  p.release = 50U;
  p.pulse   = 64U;
  p.sweep   = -190;         // [semitones / s]
  return p;
}

// an explosion: low noise that fades slowly and gets darker
static sid::patch make_explosion()
{
  sid::patch p = { };
  p.wave    = sid::osc_noise;
  p.level   = 170U;
  p.attack  = 2U;
  p.decay   = 1200U;
  p.release = 300U;
  p.sweep   = -18;
  return p;
}

// a coin: two short high notes, the second rings
static sid::patch make_coin()
{
  sid::patch p = { };
  p.wave    = sid::osc_pulse;
  p.level   = 130U;
  p.decay   = 450U;
  p.release = 100U;
  p.pulse   = 128U;
  return p;
}

// a jump: a square that rises
static sid::patch make_jump()
{
  sid::patch p = { };
  p.wave    = sid::osc_pulse;
  p.level   = 130U;
  p.decay   = 260U;
  p.release = 40U;
  p.pulse   = 128U;
  p.sweep   = 70;
  return p;
}

// a power up: an arpeggio through three octaves that climbs
static sid::patch make_power_up()
{
  sid::patch p = { };
  p.wave    = sid::osc_triangle | sid::osc_pulse;
  p.level   = 150U;
  p.attack  = 2U;
  p.decay   = 900U;
  p.release = 150U;
  p.pulse   = 128U;
  p.arp[0]  = 7U;
  p.arp[1]  = 12U;
  p.arp_ms  = 40U;
  p.sweep   = 14;
  return p;
}

// an alarm: a sawtooth with a deep, fast vibrato
static sid::patch make_alarm()
{
  sid::patch p = { };
  p.wave     = sid::osc_saw;
  p.level    = 110U;
  p.attack   = 5U;
  p.sustain  = 255U;
  p.release  = 60U;
  p.lfo_rate = 40U;         // 4 Hz
  p.vibrato  = 500U;        // [cents]
  return p;
}

// an engine: a low, narrow pulse through the band pass, the pitch is the speed
static sid::patch make_engine()
{
  sid::patch p = { };
  p.wave      = sid::osc_pulse;
  p.level     = 130U;
  p.attack    = 30U;
  p.sustain   = 255U;
  p.release   = 300U;
  p.pulse     = 24U;
  p.glide     = 900U;       // the pitch slides from note to note: revving
  p.filter    = sid::filter_band;
  p.resonance = 120U;
  p.cutoff    = 700U;
  return p;
}

static const sid::patch laser     = make_laser();
static const sid::patch explosion = make_explosion();
static const sid::patch coin      = make_coin();
static const sid::patch jump      = make_jump();
static const sid::patch power_up  = make_power_up();
static const sid::patch alarm     = make_alarm();
static const sid::patch engine    = make_engine();


// one note of an effect on voice 0, then the time it needs
static bool effect(sid::output& out, sid::synth& synth, const sid::patch& sound, std::uint32_t note, std::uint32_t ms)
{
  synth.note_on(0U, sound, note);
  return out.play(synth, ms);
}


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::synth synth(out.rate());
  bool ok = true;

  for (std::uint32_t shot = 0U; ok && (shot < 3U); ++shot) {
    ok = ok && effect(out, synth, laser, sid::key::c7, 300U);
  }
  ok = ok && out.play(synth, 300U);

  ok = ok && effect(out, synth, explosion, sid::key::c4, 1800U);

  ok = ok && effect(out, synth, coin, sid::key::b5, 80U);
  ok = ok && effect(out, synth, coin, sid::key::e6, 700U);

  for (std::uint32_t hop = 0U; ok && (hop < 2U); ++hop) {
    ok = ok && effect(out, synth, jump, sid::key::c4, 500U);
  }

  ok = ok && effect(out, synth, power_up, sid::key::c4, 1300U);

  ok = ok && effect(out, synth, alarm, sid::key::a4, 1500U);
  synth.note_off(0U);
  ok = ok && out.play(synth, 400U);

  // the engine starts, revs up through the gears and stops
  static const std::uint8_t gears[] = { sid::key::c2, sid::key::g2, sid::key::d2, sid::key::a2, sid::key::e2, sid::key::c3 };
  for (std::size_t i = 0U; ok && (i < sizeof(gears)); ++i) {
    ok = ok && effect(out, synth, engine, gears[i], 900U);
  }
  synth.note_off(0U);
  ok = ok && out.play(synth, 600U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "11_sound_effects", &play);
}
