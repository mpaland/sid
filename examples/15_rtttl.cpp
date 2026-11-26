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
// \brief sid example 15: ring tones as text (RTTTL)
//        The ring tones of the mobile phones of the 90s were lines of text: a name, the defaults (duration,
//        octave, beats per minute) and the notes. ext/rtttl.h plays such a line from where it is - a tune is a
//        string constant, and thousands of them can be found. Here: three pieces everybody knows, written as
//        RTTTL for this example, each in another waveform.
//        The pieces are in the public domain (Beethoven: Fuer Elise and the Ode to Joy; a minuet of the
//        notebook for Anna Magdalena Bach, by Christian Petzold).
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/ext/rtttl.h"

#include "common.h"


// the defaults: sixteenth notes of the piece are written as eighths (d=8), an octave above the score (o=6)
static const char elise[] =
  "Fuer Elise:d=8,o=6,b=140:"
  "e,d#,e,d#,e,b5,d,c,4a5,p,c5,e5,a5,4b5,p,e5,g#5,b5,4c,p,e5,"
  "e,d#,e,d#,e,b5,d,c,4a5,p,c5,e5,a5,4b5,p,e5,c,b5,2a5,4p";

static const char ode[] =
  "Ode to Joy:d=4,o=5,b=150:"
  "e,e,f,g,g,f,e,d,c,c,d,e,e.,8d,2d,"
  "e,e,f,g,g,f,e,d,c,c,d,e,d.,8c,2c,p";

static const char minuet[] =
  "Minuet in G:d=8,o=5,b=140:"
  "4d6,g,a,b,c6,4d6,4g,4g,4e6,c6,d6,e6,f#6,4g6,4g,4g,"
  "4c6,d6,c6,b,a,4b,c6,b,a,g,4f#,g,a,b,g,2a.";


// a tune with its name and its length on the console, then its sound
static bool ring(sid::output& out, const char* text, sid::wave_type wave, std::uint32_t level)
{
  sid::rtttl::info tune;
  if (!sid::rtttl::parse(text, tune)) {
    return false;       // no RTTTL
  }
  std::fprintf(stderr, "  %.*s: %u.%u s, %u beats per minute\n", static_cast<int>(tune.name_length), tune.name,
               static_cast<unsigned>(sid::rtttl::length(tune) / 1000U),
               static_cast<unsigned>((sid::rtttl::length(tune) % 1000U) / 100U), static_cast<unsigned>(tune.tempo));
  sid::rtttl::player music(out.rate(), tune, 1U, wave, level);      // a source, like a melody
  return out.play(music) && out.pause(400U);
}


static bool play(sid::output& out)
{
  bool ok = out.open(example::rate);

  ok = ok && ring(out, elise, sid::wave_triangle, 160U);
  ok = ok && ring(out, ode, sid::wave_square, 48U);         // the beeper of a phone
  ok = ok && ring(out, minuet, sid::wave_sine, 140U);

  // the short form: a text, played. The text of a ring: twice, as long as nobody answers
  ok = ok && sid::play_rtttl(out, "ring:d=32,o=6,b=120:c7,g,c7,g,c7,g,c7,g,c7,g,c7,g,4p,c7,g,c7,g,c7,g,c7,g,c7,g,c7,g,2p");

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "15_rtttl", &play);
}
