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
// \brief sid example 08: combined waveforms
//        A patch can select more than one waveform with |. The voice then gives the AND of their values, the
//        trick of the chip of the home computers of the 80s: sounds between the plain waveforms, hollow, nasal
//        or ragged. Triangle | pulse and sawtooth | pulse are the classic ones; with pulse width modulation
//        they move. The same phrase is played with each.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/synth.h"

#include "common.h"


static sid::patch voice(int waves, std::uint8_t pwm)
{
  sid::patch p = { };
  p.wave     = static_cast<std::uint8_t>(waves);
  p.level    = 170U;
  p.attack   = 5U;
  p.decay    = 300U;
  p.sustain  = 180U;
  p.release  = 150U;
  p.pulse    = 110U;
  p.pwm      = pwm;         // depth of the pulse width modulation
  p.lfo_rate = 12U;         // 1.2 Hz
  return p;
}

static const sid::patch patches[] = {
  voice(sid::osc_triangle, 0U),                             // for the ear: the plain ones first
  voice(sid::osc_pulse, 0U),
  voice(sid::osc_triangle | sid::osc_pulse, 0U),            // half a triangle: hollow
  voice(sid::osc_triangle | sid::osc_pulse, 90U),           // ... and moving
  voice(sid::osc_saw | sid::osc_pulse, 0U),                 // a cut sawtooth: nasal
  voice(sid::osc_saw | sid::osc_pulse, 90U),
  voice(sid::osc_triangle | sid::osc_saw, 0U),              // ragged and thin
  voice(sid::osc_noise | sid::osc_pulse, 60U)               // bursts of noise at the pitch of the note
};


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::synth synth(out.rate());
  bool ok = true;

  static const std::uint8_t phrase[] = { sid::key::a2, sid::key::e3, sid::key::a3, sid::key::c4, sid::key::e4 };
  for (std::size_t n = 0U; ok && (n < (sizeof(patches) / sizeof(patches[0]))); ++n) {
    for (std::size_t i = 0U; ok && (i < sizeof(phrase)); ++i) {
      synth.note_on(0U, patches[n], phrase[i]);
      ok = ok && out.play(synth, (i + 1U < sizeof(phrase)) ? 220U : 900U);
    }
    synth.note_off(0U);
    ok = ok && out.play(synth, 400U);
  }

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "08_combined_waveforms", &play);
}
