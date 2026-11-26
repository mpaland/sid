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
// \brief sid example 06: the filter
//        The synthesizer has one resonant filter for all voices that ask for it. A patch chooses the mode - low,
//        band or high pass, or a sum of them (low | high is a notch) - the resonance, and a cutoff that moves
//        from a start to an end value with every note: the classic "wow" of a bass.
//        A sawtooth with all its harmonics is played through each of them.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/synth.h"

#include "common.h"


// a sawtooth through the filter: mode, resonance and the way of the cutoff
static sid::patch filtered(std::uint8_t mode, std::uint8_t resonance, std::uint16_t from_hz, std::uint16_t to_hz, std::uint16_t ms)
{
  sid::patch p = { };
  p.wave       = sid::osc_saw;
  p.level      = 110U;
  p.attack     = 5U;
  p.sustain    = 255U;
  p.release    = 150U;
  p.filter     = mode;
  p.resonance  = resonance;
  p.cutoff     = from_hz;
  p.cutoff_end = to_hz;
  p.cutoff_ms  = ms;
  return p;
}

static const sid::patch plain      = filtered(sid::filter_off,  0U,   0U,    0U,    0U);
static const sid::patch low_down   = filtered(sid::filter_low,  0U,   5000U, 150U,  1500U);     // closes
static const sid::patch low_up     = filtered(sid::filter_low,  200U, 150U,  5000U, 1500U);     // opens, with resonance
static const sid::patch band_sweep = filtered(sid::filter_band, 220U, 200U,  4000U, 1500U);
static const sid::patch high_sweep = filtered(sid::filter_high, 100U, 100U,  4000U, 1500U);     // the body goes
static const sid::patch notch      = filtered(static_cast<std::uint8_t>(sid::filter_low | sid::filter_high), 180U, 300U, 4000U, 1500U);
static const sid::patch wow        = filtered(sid::filter_low,  190U, 2600U, 300U,  110U);      // per note: a bass


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::synth synth(out.rate());
  bool ok = true;

  // one long note through each filter
  const sid::patch* const sweeps[] = { &plain, &low_down, &low_up, &band_sweep, &high_sweep, &notch };
  for (std::size_t i = 0U; ok && (i < (sizeof(sweeps) / sizeof(sweeps[0]))); ++i) {
    synth.note_on(0U, *sweeps[i], sid::key::a2);
    ok = ok && out.play(synth, 1800U);
    synth.note_off(0U);
    ok = ok && out.play(synth, 400U);
  }

  // the cutoff moves with every note
  static const std::uint8_t line[] = {
    sid::key::e2, sid::key::e2, sid::key::e3, sid::key::e2, sid::key::g2, sid::key::g2, sid::key::g3, sid::key::a2,
    sid::key::e2, sid::key::e2, sid::key::e3, sid::key::e2, sid::key::d2, sid::key::d3, sid::key::b2, sid::key::d3
  };
  for (std::size_t i = 0U; ok && (i < sizeof(line)); ++i) {
    synth.note_on(0U, wow, line[i]);
    ok = ok && out.play(synth, 150U);
  }
  synth.note_off(0U);
  ok = ok && out.play(synth, 400U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "06_filter", &play);
}
