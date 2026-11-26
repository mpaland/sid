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
// \brief sid example 01: beeps
//        The smallest use of the library: open the output, play single tones, close it.
//        A tone is a frequency, a duration, a waveform (sine, square, triangle) and a level (0 .. 255).
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/tone.h"

#include "common.h"


static bool play(sid::output& out)
{
  bool ok = out.open(example::rate);

  // one beep: 880 Hz for 150 ms (a sine of medium level)
  ok = ok && sid::beep(out, 880U, 150U);
  ok = ok && out.pause(300U);

  // the three waveforms on the same note
  ok = ok && sid::beep(out, 440U, 400U, sid::wave_sine);
  ok = ok && out.pause(100U);
  ok = ok && sid::beep(out, 440U, 400U, sid::wave_triangle);
  ok = ok && out.pause(100U);
  ok = ok && sid::beep(out, 440U, 400U, sid::wave_square, 64U);     // a square is loud: a lower level
  ok = ok && out.pause(300U);

  // "ok": two rising tones; "error": a low buzz; a door bell
  ok = ok && sid::beep(out, 1047U, 80U, sid::wave_square, 64U);
  ok = ok && sid::beep(out, 1568U, 120U, sid::wave_square, 64U);
  ok = ok && out.pause(400U);
  ok = ok && sid::beep(out, 110U, 350U, sid::wave_square, 80U);
  ok = ok && out.pause(400U);
  ok = ok && sid::beep(out, 659U, 500U, sid::wave_sine, 200U);
  ok = ok && sid::beep(out, 523U, 800U, sid::wave_sine, 200U);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "01_beep", &play);
}
