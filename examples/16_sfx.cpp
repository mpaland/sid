///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.design)
//             2010-2026, PALANDesign, LLC
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
// \brief sid example 16: sound effects, ready to play
//        ext/sfx.h has the effects a device and a game need - a click, a confirmation, an error, a coin, a
//        laser, an explosion ... - as constants: a patch of the synthesizer and up to four notes. Example 11
//        shows how such patches are made; here they are just played.
//        First every effect once. Then what a game does: the same effect at other pitches, and several
//        effects at the same time on the voices of one sfx::player. An effect of your own is one more constant.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/ext/sfx.h"

#include "common.h"


// an effect of your own: a patch (here a bell, a triangle with ring modulation) and its notes, a door chime
static sid::patch make_bell()
{
  sid::patch p = { };
  p.wave      = sid::osc_triangle;
  p.level     = 140U;
  p.decay     = 900U;
  p.release   = 400U;
  p.mod       = sid::mod_ring;
  p.mod_pitch = 19;
  return p;
}

static const sid::sfx::effect chime = { make_bell(), { { sid::key::e5, 350U }, { sid::key::c5, 900U } } };


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  bool ok = true;

  // every effect once: for a user interface, then for a game
  static const sid::sfx::effect* const all[] = {
    &sid::sfx::click, &sid::sfx::blip, &sid::sfx::confirm, &sid::sfx::error, &sid::sfx::alarm,
    &sid::sfx::coin, &sid::sfx::jump, &sid::sfx::laser, &sid::sfx::hit, &sid::sfx::explosion, &sid::sfx::power_up,
    &sid::sfx::win, &sid::sfx::lose
  };
  for (std::size_t i = 0U; ok && (i < (sizeof(all) / sizeof(all[0]))); ++i) {
    ok = ok && sid::play_sfx(out, *all[i]) && out.pause(350U);      // to its end, then a moment of silence
  }
  ok = ok && sid::play_sfx(out, chime) && out.pause(600U);

  // a player keeps its voices: effects start while others still sound. It is played for a time, which is what
  // the loop of an app does between its events
  sid::sfx::player fx(out.rate());

  // five coins, each a little higher than the one before
  static const std::int8_t steps[] = { 0, 2, 4, 5, 7 };
  for (std::size_t i = 0U; ok && (i < sizeof(steps)); ++i) {
    fx.play(sid::sfx::coin, steps[i]);
    ok = ok && out.play(fx, 140U);
  }
  ok = ok && out.play(fx, 700U);

  // a fight: shots over an explosion, hits, and the power up at the end
  fx.play(sid::sfx::explosion);
  for (std::uint32_t shot = 0U; ok && (shot < 6U); ++shot) {
    fx.play(sid::sfx::laser, -static_cast<std::int32_t>(shot));
    ok = ok && out.play(fx, 130U);
    if ((shot % 2U) == 1U) {
      fx.play(sid::sfx::hit);
    }
    ok = ok && out.play(fx, 90U);
  }
  fx.play(sid::sfx::power_up);
  while (ok && !fx.silent()) {
    ok = ok && out.play(fx, 10U);
  }

  // the alarm sounds until it is switched off
  fx.play(sid::sfx::alarm);
  ok = ok && out.play(fx, 600U);
  fx.stop();
  ok = ok && out.play(fx, 300U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "16_sfx", &play);
}
