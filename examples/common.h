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
// \brief sid examples: where an example plays
//        Every example is one function that plays on a sid::output. example::run() gives it the device the
//        command line asks for:
//          (nothing)   a WAV file <name>.wav in the current folder - works everywhere, no sound hardware needed
//          --play      the sound output of the PC, live (Windows)
//          --stdout    raw samples on stdout for a pipe into a player (Linux, macOS), e.g.
//                        ./sid_example_12_song --stdout | aplay -f S16_LE -c 2 -r 22050
//        Messages go to stderr, so they do not get into the samples.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXAMPLE_COMMON_H_
#define _SID_EXAMPLE_COMMON_H_

#include <cstdio>
#include <cstring>
#include <string>

#include "sid/src/source.h"
#include "sid/src/port/wav_file.h"
#include "sid/src/port/pcm_stream.h"
#if defined(_WIN32)
#include "sid/src/port/win32.h"
#endif


namespace example {

// sample rate of the examples [Hz]: enough for this kind of sound, and what a small MCU output can make
const std::uint32_t rate = 22050U;

/**
 * Run an example on the device the command line asks for
 * \param argc, argv The command line
 * \param name Name of the example: the WAV file is <name>.wav
 * \param play The example: plays on the output it gets, returns false if something failed
 * \return Exit code of the program: 0 = played
 */
inline int run(int argc, char** argv, const char* name, bool (*play)(sid::output& out))
{
  bool live = false;
  bool pipe = false;
  for (int i = 1; i < argc; ++i) {
    live = live || (std::strcmp(argv[i], "--play") == 0);
    pipe = pipe || (std::strcmp(argv[i], "--stdout") == 0);
  }

  if (pipe) {
    sid::port::pcm_stream raw(stdout);
    sid::output out(raw);
    const bool ok = play(out);
    out.close();
    std::fprintf(stderr, "%s: %s (raw samples, signed 16 bit little endian, stereo, %u Hz)\n", name,
                 ok ? "played to stdout" : "FAILED", static_cast<unsigned>(raw.rate()));
    return ok ? 0 : 1;
  }

  if (live) {
#if defined(_WIN32)
    sid::port::win32 pc;
    sid::output out(pc);
    const bool ok = play(out);
    out.close();
    std::fprintf(stderr, "%s: %s\n", name, ok ? "played" : "FAILED (no sound output?)");
    if (pc.dry() != 0U) {
      std::fprintf(stderr, "%s: the output ran dry %u times (the PC was too busy)\n", name, static_cast<unsigned>(pc.dry()));
    }
    return ok ? 0 : 1;
#else
    std::fprintf(stderr, "%s: --play is for Windows; here pipe the samples into a player:\n"
                         "  %s --stdout | aplay -f S16_LE -c 2 -r %u\n", name, argv[0], static_cast<unsigned>(rate));
    return 2;
#endif
  }

  const std::string path = std::string(name) + ".wav";
  sid::port::wav_file file(path.c_str(), 1U);       // mono: the examples have the same sound on both channels
  sid::output out(file);
  const bool played = play(out);
  out.close();
  const std::uint32_t frames = file.frames();
  const bool ok = file.close() && played;
  std::fprintf(stderr, "%s: %s %s, %u frames\n", name, ok ? "wrote" : "FAILED to write", path.c_str(),
               static_cast<unsigned>(frames));
  return ok ? 0 : 1;
}

} // namespace example

#endif  // _SID_EXAMPLE_COMMON_H_
