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
// \brief sid example 03: a recorded clip
//        Recorded sound is a WAV file in memory: tools/wav2h.py turns a file into a header with a byte array
//        (data/clip.h, made by data/make_clip.py), so the clip lives in the flash of a microcontroller.
//        sid::play_wav() opens the output with the sample rate of the file and plays it; sid::wav::parse() and
//        sid::pcm do the same in two steps. PCM of 8 or 16 bit, mono or stereo; nothing is resampled.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/player.h"

#include "common.h"
#include "data/clip.h"


static bool play(sid::output& out)
{
  // the short way
  bool ok = sid::play_wav(out, clip, sizeof(clip));
  ok = ok && out.pause(400U);

  // step by step: look at the file, then play its samples as a source
  sid::wav::info info;
  if (!sid::wav::parse(clip, sizeof(clip), info)) {
    return false;
  }
  std::fprintf(stderr, "clip: %u Hz, %u bit, %u channel(s), %u frames\n", static_cast<unsigned>(info.rate),
               static_cast<unsigned>(info.bits), static_cast<unsigned>(info.channels), static_cast<unsigned>(info.frames));
  ok = ok && out.open(info.rate);
  sid::pcm samples(info);
  ok = ok && out.play(samples, 600U);       // the first 0.6 s only ...
  ok = ok && out.pause(200U);
  ok = ok && out.play(samples);             // ... and the rest

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "03_wav_clip", &play);
}
