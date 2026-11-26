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
// \brief sid example 12: a whole song
//        J. S. Bach, Invention 13 in A minor (BWV 784), the way the home computers of the 80s played Bach: the
//        right hand as a pulse lead with a quiet echo three steps behind it, the left hand as a sawtooth bass
//        through the resonant low pass, and drums. Four voices, 46 seconds, 3.3 KB of constant data.
//        The song is in data/invention13.h, made by data/make_invention13.py from the notes of the piece - that
//        script is the example of how to turn written music into a song for the sequencer.
//        The piece is in the public domain (notes: Mutopia Project edition, see the script).
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/sequencer.h"

#include "common.h"
#include "data/invention13.h"


static bool play(sid::output& out)
{
  const bool ok = out.open(example::rate) && sid::play(out, bach::song);
  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "12_song", &play);
}
