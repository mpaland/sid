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
// \brief sid tests: the RTTTL player (ext/rtttl.h)
//        What parse() takes and refuses, the frequencies of the notes against the note names of tone.h, the
//        notes of a text (durations, dots, sharps, octaves, pauses, entries that are no notes), the length of
//        a tune, the player (pitch, frames, loops, a tune without notes) and play_rtttl().
//
///////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <string>
#include <vector>

#include "sid/src/ext/rtttl.h"

#include "test.h"
#include "memory_device.h"


namespace {

const std::uint32_t RATE = 22050U;

typedef std::vector<std::int16_t> sound;

// all frames of a source, rendered in chunks of an odd size (chunk borders must not show)
sound render(sid::source& src, std::size_t chunk = 37U)
{
  sound all;
  sound part(chunk * 2U);
  for (;;) {
    const std::size_t n = src.render(&part[0], chunk);
    all.insert(all.end(), part.begin(), part.begin() + static_cast<std::ptrdiff_t>(n * 2U));
    if (n < chunk) {
      return all;
    }
  }
}

// rising zero crossings of the left channel in the frames first .. last - 1
std::size_t crossings(const sound& s, std::size_t first, std::size_t last)
{
  std::size_t count = 0U;
  for (std::size_t i = first + 1U; (i < last) && ((i * 2U) < s.size()); ++i) {
    if ((s[(i - 1U) * 2U] < 0) && (s[i * 2U] >= 0)) {
      ++count;
    }
  }
  return count;
}

int peak(const sound& s, std::size_t first, std::size_t last)
{
  int p = 0;
  for (std::size_t i = first * 2U; (i < (last * 2U)) && (i < s.size()); ++i) {
    const int a = (s[i] < 0) ? -s[i] : s[i];
    p = (a > p) ? a : p;
  }
  return p;
}

// the tune of a text that must be RTTTL
sid::rtttl::info tune_of(const char* text)
{
  sid::rtttl::info tune = { nullptr, 0U, "", 4U, 6U, 63U };
  CHECK_MSG(sid::rtttl::parse(text, tune), text);
  return tune;
}

// frames of a tune
std::size_t frames_of(const char* text, std::uint32_t loops = 1U)
{
  const sid::rtttl::info tune = tune_of(text);
  sid::rtttl::player music(RATE, tune, loops);
  return render(music).size() / 2U;
}

} // namespace


TEST_CASE(sid_rtttl_parse)
{
  sid::rtttl::info tune;
  const char* const scale = "scale:d=8,o=5,b=120:c,d,e";
  REQUIRE(sid::rtttl::parse(scale, tune));
  CHECK(std::string(tune.name, tune.name_length) == "scale");
  CHECK_EQ(tune.duration, 8U);
  CHECK_EQ(tune.octave, 5U);
  CHECK_EQ(tune.tempo, 120U);
  CHECK(tune.notes == (scale + 20));
  CHECK(std::string(tune.notes) == "c,d,e");

  // the defaults of the defaults
  REQUIRE(sid::rtttl::parse("x::c", tune));
  CHECK_EQ(tune.duration, 4U);
  CHECK_EQ(tune.octave, 6U);
  CHECK_EQ(tune.tempo, 63U);
  REQUIRE(sid::rtttl::parse(":b=200:", tune));          // no name, no notes
  CHECK_EQ(tune.name_length, 0U);
  CHECK_EQ(tune.tempo, 200U);
  CHECK_EQ(tune.duration, 4U);
  CHECK_EQ(tune.notes[0], '\0');

  // any order, upper case, blanks, settings that are not used
  REQUIRE(sid::rtttl::parse("  My Tune : B = 90 ,O=4, s=n ,D = 16,l=15 :\n c", tune));
  CHECK(std::string(tune.name, tune.name_length) == "My Tune");
  CHECK_EQ(tune.duration, 16U);
  CHECK_EQ(tune.octave, 4U);
  CHECK_EQ(tune.tempo, 90U);

  // all durations, the limits of octave and tempo
  static const char* const good[] = {
    "t:d=1:c", "t:d=2:c", "t:d=4:c", "t:d=8:c", "t:d=16:c", "t:d=32:c", "t:o=0:c", "t:o=9:c", "t:b=1:c", "t:b=999:c"
  };
  for (std::size_t i = 0U; i < (sizeof(good) / sizeof(good[0])); ++i) {
    CHECK_MSG(sid::rtttl::parse(good[i], tune), good[i]);
  }

  // no RTTTL
  static const char* const bad[] = {
    "", "name", "name:", "name:d=4", "name:d=4,o=5,b=100", "t:d=0:c", "t:d=3:c", "t:d=64:c", "t:d=400000000000:c",
    "t:o=10:c", "t:b=0:c", "t:b=1000:c", "t:d4:c", "t:d=:c", "t:d=x:c", "t:b=:c", "t:o:c", "t:d=4x:c"
  };
  for (std::size_t i = 0U; i < (sizeof(bad) / sizeof(bad[0])); ++i) {
    CHECK_MSG(!sid::rtttl::parse(bad[i], tune), bad[i]);
  }
  CHECK(!sid::rtttl::parse(nullptr, tune));
}


TEST_CASE(sid_rtttl_frequency)
{
  // the notes are those of the note names of tone.h
  static const std::uint16_t names[4][12] = {
    { sid::c3, sid::cs3, sid::d3, sid::ds3, sid::e3, sid::f3, sid::fs3, sid::g3, sid::gs3, sid::a3, sid::as3, sid::b3 },
    { sid::c4, sid::cs4, sid::d4, sid::ds4, sid::e4, sid::f4, sid::fs4, sid::g4, sid::gs4, sid::a4, sid::as4, sid::b4 },
    { sid::c5, sid::cs5, sid::d5, sid::ds5, sid::e5, sid::f5, sid::fs5, sid::g5, sid::gs5, sid::a5, sid::as5, sid::b5 },
    { sid::c6, sid::cs6, sid::d6, sid::ds6, sid::e6, sid::f6, sid::fs6, sid::g6, sid::gs6, sid::a6, sid::as6, sid::b6 }
  };
  for (std::uint32_t octave = 3U; octave <= 6U; ++octave) {
    for (std::uint32_t semitone = 0U; semitone < 12U; ++semitone) {
      CHECK_EQ(sid::rtttl::detail::frequency(semitone, octave), names[octave - 3U][semitone]);
    }
  }
  CHECK_EQ(sid::rtttl::detail::frequency(9U, 0U), 28U);         // a0: 27.5 Hz
  CHECK_EQ(sid::rtttl::detail::frequency(9U, 7U), 3520U);
  CHECK_EQ(sid::rtttl::detail::frequency(9U, 10U), 28160U);
  CHECK_EQ(sid::rtttl::detail::frequency(0U, 10U), 16744U);
}


TEST_CASE(sid_rtttl_notes)
{
  typedef struct tag_expected {
    unsigned  freq;
    unsigned  duration;
    bool      dotted;
  } expected;

  const sid::rtttl::info tune = tune_of("t:d=4,o=5,b=100: 8c#6,4a.,a.5,2P,h,b#,x,16zz,3c,c5., E ,32g#.7,,f#");
  static const expected notes[] = {
    { sid::cs6, 8U,  false },     // 8c#6
    { sid::a5,  4U,  true  },     // 4a.      a dot
    { sid::a5,  4U,  true  },     // a.5      the dot before the octave
    { 0U,       2U,  false },     // 2P       a pause
    { sid::b5,  4U,  false },     // h        is b
    { sid::c6,  4U,  false },     // b#       the c of the next octave
                                  // x        no note
                                  // 16zz     no note
                                  // 3c       no duration
    { sid::c5,  4U,  true  },     // c5.      the dot behind the octave
    { sid::e5,  4U,  false },     // E
    { 3323U,    32U, true  },     // 32g#.7
    { sid::fs5, 4U,  false }      // f#       behind an empty entry
  };
  const char* text = tune.notes;
  sid::rtttl::detail::event note = { 0U, 0U, false };
  for (std::size_t i = 0U; i < (sizeof(notes) / sizeof(notes[0])); ++i) {
    REQUIRE(sid::rtttl::detail::next(text, tune, note));
    CHECK_EQ(note.freq, notes[i].freq);
    CHECK_EQ(note.duration, notes[i].duration);
    CHECK_EQ(note.dotted, notes[i].dotted);
  }
  CHECK(!sid::rtttl::detail::next(text, tune, note));
  CHECK(!sid::rtttl::detail::next(text, tune, note));       // and it stays at the end
  CHECK_EQ(*text, '\0');

  // the default octave of the tune
  const sid::rtttl::info low = tune_of("t:o=3:a");
  text = low.notes;
  REQUIRE(sid::rtttl::detail::next(text, low, note));
  CHECK_EQ(note.freq, 220U);
}


TEST_CASE(sid_rtttl_length)
{
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=4,o=5,b=120:c,c,c,c")), 2000U);
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=4,o=5,b=60:1c,2c,4c,8c,16c,32c")), 7875U);
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=4,o=5,b=120:2c.,8p,16c,32c,1c")), 3937U);     // 3937.5
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=4,o=5,b=120:")), 0U);
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=4,o=5,b=120:x,y,,")), 0U);
  // an eighth at 140 beats is 214.29 ms: seven of them are 1500 ms, the parts of a ms are not lost
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=8,o=5,b=140:c,d,e,f,g,a,b")), 1500U);
  CHECK_EQ(sid::rtttl::length(tune_of("t:d=8,o=5,b=140:c")), 214U);
  // the slowest and longest note, the fastest and shortest
  CHECK_EQ(sid::rtttl::length(tune_of("t:b=1:1c.")), 360000U);
  CHECK_EQ(sid::rtttl::length(tune_of("t:b=999:32c")), 7U);                               // 7.5
}


TEST_CASE(sid_rtttl_player)
{
  // one second of a4, of c6 and of a pause
  {
    const sid::rtttl::info tune = tune_of("pitch:d=1,o=4,b=240:a,c6,p");
    sid::rtttl::player music(RATE, tune);
    CHECK(!music.ended());
    const sound s = render(music);
    CHECK(music.ended());
    REQUIRE(s.size() == (3U * RATE * 2U));
    const std::size_t a4 = crossings(s, 0U, RATE);
    const std::size_t c6 = crossings(s, RATE, 2U * RATE);
    CHECK_MSG((a4 >= 439U) && (a4 <= 441U), "a4");
    CHECK_MSG((c6 >= 1046U) && (c6 <= 1048U), "c6");
    CHECK_EQ(peak(s, 2U * RATE, 3U * RATE), 0);
    // the level: 128 of 255 by default
    const int level = peak(s, 0U, RATE);
    CHECK_MSG((level > 16300) && (level < 16500), "level");
    // it stays ended
    std::int16_t buffer[32];
    CHECK_EQ(music.render(buffer, 16U), 0U);
  }

  // waveform and level
  {
    const sid::rtttl::info tune = tune_of("square:d=4,o=5,b=120:a");
    sid::rtttl::player music(RATE, tune, 1U, sid::wave_square, 255U);
    const sound s = render(music);
    REQUIRE(s.size() == (11025U * 2U));
    CHECK_EQ(peak(s, 0U, 11025U), 32767);
    CHECK_EQ(s[2000], s[2001]);                 // the same on both channels
    const std::size_t a5 = crossings(s, 0U, 11025U);
    CHECK_MSG((a5 >= 439U) && (a5 <= 441U), "a5");
  }

  // the frames of a tune are those of its notes
  CHECK_EQ(frames_of("t:d=4,o=5,b=120:c,d,e,f"), 44100U);
  CHECK_EQ(frames_of("t:d=8,o=5,b=140:c,d,e,f,g,a,b"), (2U * 4740U) + (5U * 4718U));     // two of 215 ms, five of 214 ms
  CHECK_EQ(frames_of("t:d=4,o=5,b=120:c,x,d,,e"), 3U * 11025U);

  // loops
  CHECK_EQ(frames_of("t:d=4,o=5,b=120:c,d,e,f", 3U), 3U * 44100U);
  {
    const sid::rtttl::info tune = tune_of("t:d=4,o=5,b=120:c,d");
    sid::rtttl::player music(RATE, tune, 0U);       // for ever
    sound buffer(2000U);
    std::size_t frames = 0U;
    for (std::size_t i = 0U; i < 200U; ++i) {
      frames += music.render(&buffer[0], 1000U);
    }
    CHECK_EQ(frames, 200000U);                      // 9 rounds
    CHECK(!music.ended());
  }

  // a tune without a note ends at once, also one that loops for ever
  static const char* const empty[] = { "t:d=4,o=5,b=120:", "t:d=4,o=5,b=120: ,x, 3c ,,", "t::" };
  for (std::size_t i = 0U; i < (sizeof(empty) / sizeof(empty[0])); ++i) {
    CHECK_EQ(frames_of(empty[i], 1U), 0U);
    CHECK_EQ(frames_of(empty[i], 0U), 0U);
  }
}


TEST_CASE(sid_rtttl_play)
{
  memory_device dev;
  sid::output out(dev);
  const char* const tune = "beep:d=4,o=5,b=120:a,p,8a6";

  CHECK(!sid::play_rtttl(out, tune));               // the output is not open
  REQUIRE(out.open(RATE));
  CHECK(!sid::play_rtttl(out, "no tune"));
  CHECK(!sid::play_rtttl(out, nullptr));
  CHECK_EQ(dev.frames(), 0U);

  CHECK(sid::play_rtttl(out, tune));
  CHECK_EQ(dev.frames(), 11025U + 11025U + 5512U);
  const sound sine = dev.samples();
  CHECK(sid::play_rtttl(out, tune, sid::wave_square, 200U));
  CHECK_EQ(dev.frames(), 2U * (11025U + 11025U + 5512U));
  out.close();
  const sound both = dev.samples();
  CHECK_EQ(peak(both, 27562U, 27562U + 11025U), (32767 * 200) / 255);
  CHECK_EQ(peak(sine, 11025U, 22050U), 0);          // the pause

  dev.fail_write(true);
  REQUIRE(out.open(RATE));
  CHECK(!sid::play_rtttl(out, tune));               // the device does not take the frames
  out.close();
}
