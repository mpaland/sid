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
// \brief sid example 02: melodies
//        A melody is an array of notes { frequency [Hz], duration [ms] }; sid::play() plays it with one waveform.
//        The note names sid::c3 .. sid::b6 are frequencies, sid::rest is silence. The array is constant data,
//        so on a microcontroller it stays in the flash.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/tone.h"

#include "common.h"


// one octave up
static const sid::note scale[] = {
  { sid::c4, 150U }, { sid::d4, 150U }, { sid::e4, 150U }, { sid::f4, 150U },
  { sid::g4, 150U }, { sid::a4, 150U }, { sid::b4, 150U }, { sid::c5, 300U }, { sid::rest, 150U }
};

// a little fanfare
static const sid::note fanfare[] = {
  { sid::g4, 140U }, { sid::c5, 140U }, { sid::e5, 140U }, { sid::g5, 280U }, { sid::rest, 60U },
  { sid::e5, 140U }, { sid::g5, 420U }, { sid::rest, 120U },
  { sid::a4, 140U }, { sid::d5, 140U }, { sid::f5, 140U }, { sid::a5, 280U }, { sid::rest, 60U },
  { sid::f5, 140U }, { sid::a5, 420U }, { sid::rest, 120U },
  { sid::g5, 140U }, { sid::e5, 140U }, { sid::c5, 140U }, { sid::g4, 140U }, { sid::c5, 560U }, { sid::rest, 200U }
};


static bool play(sid::output& out)
{
  bool ok = out.open(example::rate);

  // the scale in the three waveforms
  ok = ok && sid::play(out, scale, sizeof(scale) / sizeof(scale[0]), sid::wave_sine);
  ok = ok && sid::play(out, scale, sizeof(scale) / sizeof(scale[0]), sid::wave_triangle);
  ok = ok && sid::play(out, scale, sizeof(scale) / sizeof(scale[0]), sid::wave_square, 48U);

  // the fanfare, a bit louder
  ok = ok && sid::play(out, fanfare, sizeof(fanfare) / sizeof(fanfare[0]), sid::wave_triangle, 160U);

  // a melody is a source like any other: it can be made first and played later
  sid::melody again(out.rate(), fanfare, sizeof(fanfare) / sizeof(fanfare[0]), sid::wave_square, 40U);
  ok = ok && out.play(again);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "02_melody", &play);
}
