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
// \brief sid example 17: a mixer - music, sound effects and a tune at the same time
//        The output plays one source after the other. A sid::mixer (ext/mixer.h) is a source that plays
//        several at once, each with its level. Example 13 plays effects on a voice the song leaves free; the
//        song of this example needs all four voices of its synthesizer, so the effects are a source of their
//        own (ext/sfx.h) and the mixer adds them up.
//        A scene of a game: the music of example 12 in the background, effects when something happens, and a
//        ring tone (ext/rtttl.h) that comes in - the music goes quiet while it plays. At the end the music
//        fades out.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/sequencer.h"
#include "sid/src/ext/mixer.h"
#include "sid/src/ext/rtttl.h"
#include "sid/src/ext/sfx.h"

#include "common.h"
#include "data/invention13.h"


// what happens in the scene: an effect at a time [slices of 100 ms]
typedef struct tag_event {
  std::uint32_t             slice;
  const sid::sfx::effect*   what;
  std::int8_t               transpose;
} event;

static const event events[] = {
  {  15U, &sid::sfx::coin,      0 }, {  18U, &sid::sfx::coin,     2 }, {  21U, &sid::sfx::coin,  4 },
  {  32U, &sid::sfx::jump,      0 }, {  40U, &sid::sfx::laser,    0 }, {  42U, &sid::sfx::laser, 0 },
  {  44U, &sid::sfx::explosion, 0 }, {  60U, &sid::sfx::jump,     3 }, {  64U, &sid::sfx::hit,   0 },
  { 130U, &sid::sfx::power_up,  0 }, { 150U, &sid::sfx::coin,     7 }, { 165U, &sid::sfx::win,   0 }
};

static const char message[] = "message:d=16,o=6,b=160:e,g,b,8e7,p,b,8e7,4p,e,g,b,8e7,p,b,4e7";

// levels of the inputs: the music leaves room for what comes on top
static const std::uint32_t music_level = 140U;
static const std::uint32_t quiet_level = 50U;      // while the ring tone plays
static const std::uint32_t fx_level    = 210U;


static bool play(sid::output& out)
{
  sid::rtttl::info tune;
  if (!out.open(example::rate) || !sid::rtttl::parse(message, tune)) {
    return false;
  }

  sid::sequencer      music(out.rate(), bach::song, 0U);                          // loops for ever
  sid::sfx::player    fx(out.rate());                                             // never ends, silent until an effect plays
  sid::rtttl::player  ring(out.rate(), tune, 1U, sid::wave_triangle, 150U);
  sid::mixer<3>       mix;
  bool ok = mix.add(music, music_level) && mix.add(fx, fx_level);

  // 20 s in slices of 100 ms; a real app would look at its buttons and events between the slices
  for (std::uint32_t slice = 0U; ok && (slice < 200U); ++slice) {
    for (std::size_t i = 0U; i < (sizeof(events) / sizeof(events[0])); ++i) {
      if (events[i].slice == slice) {
        fx.play(*events[i].what, events[i].transpose);
      }
    }

    // the message comes in: the music goes quiet for it, and comes back when the tune has ended
    if (slice == 85U) {
      ok = ok && mix.add(ring) && mix.set_level(music, quiet_level);
    }
    if ((slice > 85U) && (slice < 180U) && !mix.playing(ring)) {
      (void)mix.set_level(music, music_level);
    }

    // the end: the music fades out
    if (slice >= 180U) {
      (void)mix.set_level(music, (music_level * (199U - slice)) / 20U);
    }
    ok = ok && out.play(mix, 100U);
  }

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "17_mixer", &play);
}
