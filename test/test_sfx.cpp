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
// \brief sid tests: the sound effects (ext/sfx.h)
//        Every effect of the file sounds, does not clip and ends. The notes of an effect (pitch and time,
//        transposed too) and the voices (four effects at once, the oldest ends for a fifth, a voice without an
//        effect is taken first) are measured with effects made here: plain squares that are held. The player
//        as a source that never ends, stop() and play_sfx().
//
///////////////////////////////////////////////////////////////////////////////

#include <vector>

#include "sid/src/ext/sfx.h"

#include "test.h"
#include "memory_device.h"


namespace {

const std::uint32_t RATE = 22050U;

typedef std::vector<std::int16_t> sound;

// frames of a time
std::size_t frames_of(std::uint32_t ms)
{
  return (static_cast<std::size_t>(RATE) * ms) / 1000U;
}

// the next ms of a source that never ends, behind what is in 'to'
void play(sid::source& src, std::uint32_t ms, sound& to)
{
  const std::size_t frames = frames_of(ms);
  const std::size_t before = to.size();
  to.resize(before + (frames * 2U));
  std::size_t done = 0U;
  bool        all  = true;
  while (done < frames) {
    const std::size_t part = ((frames - done) < 37U) ? (frames - done) : 37U;      // chunk borders must not show
    all   = all && (src.render(&to[before + (done * 2U)], part) == part);
    done += part;
  }
  CHECK_MSG(all, "the source has ended");
}

// an effect to its end, 20 s at most; the frames are a multiple of 10 ms
sound to_the_end(sid::sfx::player& fx)
{
  sound all;
  for (std::uint32_t ms = 0U; !fx.silent() && (ms < 20000U); ms += 10U) {
    play(fx, 10U, all);
  }
  return all;
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

bool close_to(std::size_t value, std::size_t expected, std::size_t tolerance)
{
  return (value + tolerance >= expected) && (value <= expected + tolerance);
}

// a square that is held at its level, and ends 10 ms after its last note
sid::patch square(std::uint32_t level)
{
  sid::patch p = { };
  p.wave    = sid::osc_pulse;
  p.level   = static_cast<std::uint8_t>(level);
  p.sustain = 255U;
  p.release = 10U;
  p.pulse   = 128U;
  return p;
}

// an effect of up to two notes of a square
sid::sfx::effect notes(std::uint32_t level, std::uint8_t key0, std::uint16_t ms0, std::uint8_t key1 = 0U, std::uint16_t ms1 = 0U)
{
  sid::sfx::effect e = { square(level), { { key0, ms0 }, { key1, ms1 } } };
  return e;
}

const sid::sfx::effect two_notes = notes(100U, sid::key::a4, 500U, sid::key::a5, 250U);
const sid::sfx::effect long_tone = notes(40U,  sid::key::a4, 2000U);
const sid::sfx::effect short_one = notes(40U,  sid::key::a4, 30U);
const sid::sfx::effect mute      = notes(0U,   sid::key::a4, 2000U);

} // namespace


TEST_CASE(sid_sfx_effects)
{
  typedef struct tag_entry {
    const char*               name;
    const sid::sfx::effect*   what;
  } entry;
  static const entry all[] = {
    { "click",     &sid::sfx::click     }, { "blip",     &sid::sfx::blip     }, { "confirm", &sid::sfx::confirm },
    { "error",     &sid::sfx::error     }, { "alarm",    &sid::sfx::alarm    }, { "coin",    &sid::sfx::coin    },
    { "jump",      &sid::sfx::jump      }, { "laser",    &sid::sfx::laser    }, { "hit",     &sid::sfx::hit     },
    { "explosion", &sid::sfx::explosion }, { "power_up", &sid::sfx::power_up }, { "win",     &sid::sfx::win     },
    { "lose",      &sid::sfx::lose      }
  };

  for (std::size_t i = 0U; i < (sizeof(all) / sizeof(all[0])); ++i) {
    const sid::sfx::effect& e = *all[i].what;
    std::uint32_t ms = 0U;
    for (std::size_t n = 0U; n < sid::sfx::parts; ++n) {
      ms += e.notes[n].ms;
    }
    CHECK_MSG(ms != 0U, all[i].name);

    sid::sfx::player fx(RATE);
    CHECK(fx.silent());
    fx.play(e);
    CHECK_MSG(!fx.silent(), all[i].name);
    const sound s = to_the_end(fx);
    CHECK_MSG(fx.silent(), all[i].name);
    const std::size_t frames = s.size() / 2U;

    // it lasts as long as its notes, and its end fades within 2 s
    CHECK_MSG(frames >= frames_of(ms), all[i].name);
    CHECK_MSG(frames <= frames_of(ms + 2000U), all[i].name);
    // it starts at once, is loud enough to be heard and does not clip
    CHECK_MSG(peak(s, 0U, frames_of(20U)) > 1000, all[i].name);
    CHECK_MSG(peak(s, 0U, frames) > 8000, all[i].name);
    CHECK_MSG(peak(s, 0U, frames) < 32767, all[i].name);
    // the same on both channels
    bool mono = true;
    for (std::size_t f = 0U; f < frames; ++f) {
      mono = mono && (s[f * 2U] == s[f * 2U + 1U]);
    }
    CHECK_MSG(mono, all[i].name);
  }
}


TEST_CASE(sid_sfx_notes)
{
  // 500 ms of a4, then 250 ms of a5
  {
    sid::sfx::player fx(RATE);
    fx.play(two_notes);
    const sound s = to_the_end(fx);
    const std::size_t change = frames_of(500U);
    const std::size_t end    = frames_of(750U);
    CHECK_MSG(close_to(crossings(s, 0U, change), 220U, 1U), "a4");
    CHECK_MSG(close_to(crossings(s, change, end), 220U, 1U), "a5");
    // held to the end of the last note, silent soon after it
    CHECK(peak(s, end - 100U, end) > 10000);
    CHECK_MSG((s.size() / 2U) >= end, "length");
    CHECK_MSG((s.size() / 2U) <= frames_of(850U), "length");
  }
  // an octave up and an octave down
  {
    sid::sfx::player fx(RATE);
    fx.play(two_notes, 12);
    const sound up = to_the_end(fx);
    CHECK_MSG(close_to(crossings(up, 0U, frames_of(500U)), 440U, 1U), "a5");
    CHECK_MSG(close_to(crossings(up, frames_of(500U), frames_of(750U)), 440U, 1U), "a6");
    fx.play(two_notes, -12);
    const sound down = to_the_end(fx);
    CHECK_MSG(close_to(crossings(down, 0U, frames_of(500U)), 110U, 1U), "a3");
  }
  // a transposition out of the keys is limited
  {
    sid::sfx::player fx(RATE);
    fx.play(two_notes, -1000);
    const sound low = to_the_end(fx);
    CHECK(peak(low, 0U, low.size() / 2U) > 10000);
    fx.play(two_notes, 1000);
    const sound high = to_the_end(fx);
    CHECK((high.size() / 2U) >= frames_of(750U));
  }
  // an effect that starts while the same one plays: it plays twice
  {
    sid::sfx::player fx(RATE);
    sound s;
    fx.play(two_notes);
    play(fx, 600U, s);                                // the first one is at its a5
    fx.play(two_notes);                               // the second at its a4
    play(fx, 100U, s);
    CHECK(peak(s, frames_of(620U), frames_of(700U)) > peak(s, frames_of(520U), frames_of(600U)));
  }
}


TEST_CASE(sid_sfx_voices)
{
  // one voice
  int one = 0;
  {
    sid::sfx::player fx(RATE);
    sound s;
    fx.play(long_tone);
    play(fx, 200U, s);
    one = peak(s, frames_of(100U), frames_of(200U));
    CHECK_MSG((one > 4000) && (one < 6500), "level 40 of 255");
  }

  // four effects that start together are four voices; a fifth takes the voice of the oldest
  {
    sid::sfx::player fx(RATE);
    sound s;
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      fx.play(long_tone);
    }
    play(fx, 200U, s);
    const int all = peak(s, frames_of(100U), frames_of(200U));
    const int expected = one * static_cast<int>(SID_VOICES);
    CHECK_MSG((all > ((expected * 98) / 100)) && (all < ((expected * 102) / 100)), "all voices");

    for (int left = static_cast<int>(SID_VOICES) - 1; left >= 0; --left) {
      fx.play(mute);
      s.clear();
      play(fx, 200U, s);
      const int now = peak(s, frames_of(100U), frames_of(200U));
      CHECK_MSG((now >= (((one * left) * 98) / 100)) && (now <= (((one * left) * 102) / 100)), "a voice less");
    }
  }

  // a voice without an effect is taken before one that plays
  {
    sid::sfx::player fx(RATE);
    sound s;
    fx.play(long_tone);
    fx.play(short_one);
    play(fx, 200U, s);                                // the short one has ended
    for (std::size_t i = 1U; i < SID_VOICES; ++i) {
      fx.play(mute);
    }
    s.clear();
    play(fx, 200U, s);
    const int still = peak(s, frames_of(100U), frames_of(200U));
    CHECK_MSG((still > ((one * 98) / 100)) && (still < ((one * 102) / 100)), "the long tone goes on");
    fx.play(mute);                                    // all voices are busy: the long tone is the oldest
    s.clear();
    play(fx, 200U, s);
    CHECK_EQ(peak(s, frames_of(100U), frames_of(200U)), 0);
  }
}


TEST_CASE(sid_sfx_player)
{
  // a source that never ends: silence when nothing plays
  sid::sfx::player fx(RATE);
  sound s;
  play(fx, 100U, s);
  CHECK_EQ(peak(s, 0U, frames_of(100U)), 0);
  CHECK(fx.silent());
  std::int16_t buffer[2] = { 77, 77 };
  CHECK_EQ(fx.render(buffer, 0U), 0U);
  CHECK_EQ(buffer[0], 77);

  // stop() ends an effect: the alarm of 1.5 s is silent after its release
  fx.play(sid::sfx::alarm);
  fx.play(sid::sfx::explosion);
  s.clear();
  play(fx, 100U, s);
  CHECK(peak(s, 0U, frames_of(100U)) > 8000);
  CHECK(!fx.silent());
  fx.stop();
  const sound rest = to_the_end(fx);
  CHECK(fx.silent());
  CHECK_MSG((rest.size() / 2U) <= frames_of(1000U), "release");
  fx.stop();                                          // nothing plays: nothing happens
  CHECK(fx.silent());

  // and it plays again
  fx.play(sid::sfx::blip);
  CHECK(!fx.silent());
  const sound again = to_the_end(fx);
  CHECK(peak(again, 0U, again.size() / 2U) > 8000);
}


TEST_CASE(sid_sfx_play)
{
  memory_device dev;
  sid::output out(dev);
  CHECK(!sid::play_sfx(out, sid::sfx::coin));         // the output is not open
  REQUIRE(out.open(RATE));

  CHECK(sid::play_sfx(out, sid::sfx::coin));
  const std::size_t coin = dev.frames();
  CHECK_MSG(coin >= frames_of(530U), "the notes of the coin");
  CHECK_MSG(coin <= frames_of(1500U), "and its end");
  CHECK_EQ(coin % frames_of(10U), 0U);                // played in slices of 10 ms
  CHECK(peak(dev.samples(), 0U, coin) > 8000);

  // transposed: the first note of the coin (b5, 988 Hz for 80 ms) an octave lower
  CHECK(sid::play_sfx(out, sid::sfx::coin, -12));
  const sound both = dev.samples();
  CHECK_MSG(close_to(crossings(both, 0U, frames_of(80U)), 79U, 2U), "b5");
  CHECK_MSG(close_to(crossings(both, coin, coin + frames_of(80U)), 40U, 2U), "b4");
  out.close();

  dev.fail_write(true);
  REQUIRE(out.open(RATE));
  CHECK(!sid::play_sfx(out, sid::sfx::coin));         // the device does not take the frames
  out.close();
}
