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
// \brief sid example 18: a level meter
//        ext/meter.h is a source around another source: it passes the sound on and keeps its level, for a VU
//        meter or a bar of LEDs. Here the sound effects of ext/sfx.h play through a meter, and the level is
//        printed as a bar five times a second - in both scales, which shows what the switch is for: the linear
//        bar follows the amplitude and is short for all but the loudest sound, the logarithmic bar follows
//        the loudness as the ear hears it, and the long tail of the explosion is seen as long as it is heard.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/ext/meter.h"
#include "sid/src/ext/sfx.h"

#include "common.h"


// what happens: an effect at a time [slices of 50 ms]
typedef struct tag_event {
  std::uint32_t             slice;
  const sid::sfx::effect*   what;
} event;

static const event events[] = {
  {  4U, &sid::sfx::coin }, { 16U, &sid::sfx::hit }, { 28U, &sid::sfx::explosion }, { 72U, &sid::sfx::power_up },
  { 100U, &sid::sfx::laser }
};

// a bar of 20 LEDs as text: the last one is on at full scale
static void bar(std::uint32_t level, char text[21])
{
  const std::uint32_t on = (level * 20U) / 255U;
  for (std::uint32_t i = 0U; i < 20U; ++i) {
    text[i] = (i < on) ? '#' : '.';
  }
  text[20] = '\0';
}


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }

  sid::sfx::player  fx(out.rate());           // never ends, silent until an effect plays
  sid::meter        vu(out.rate(), fx);       // plays the effects and measures them

  // 6 s in slices of 50 ms; a real app would show the level on its LEDs or its display after each slice
  bool ok = true;
  for (std::uint32_t slice = 0U; ok && (slice < 120U); ++slice) {
    for (std::size_t i = 0U; i < (sizeof(events) / sizeof(events[0])); ++i) {
      if (events[i].slice == slice) {
        fx.play(*events[i].what);
      }
    }
    ok = out.play(vu, 50U);

    // the level in both scales: the switch changes how the level is read, not what is measured
    if ((slice % 4U) == 3U) {
      char log_bar[21];
      char linear_bar[21];
      vu.set_scale(sid::meter_log);
      const unsigned decibels = static_cast<unsigned>(vu.level());
      bar(decibels, log_bar);
      vu.set_scale(sid::meter_linear);
      const unsigned amplitude = static_cast<unsigned>(vu.level());
      bar(amplitude, linear_bar);
      std::fprintf(stderr, "  %u.%u s   log |%s| %3u   linear |%s| %3u\n", static_cast<unsigned>((slice + 1U) / 20U),
                   static_cast<unsigned>(((slice + 1U) % 20U) / 2U), log_bar, decibels, linear_bar, amplitude);
    }
  }

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "18_meter", &play);
}
