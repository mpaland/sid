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
// \brief sid tests: the MOD player (ext/mod.h)
//        The MOD files are made here, in memory: a looped square of 32 bytes as the sample, notes and effects
//        written into the cells. Checked: what parse() takes and refuses, cut off files, the pitch of a period
//        and of the finetune, arpeggio and portamento, volume and its effects, the length of a song by speed,
//        tempo, pattern break, position jump and loops, the stereo, and play_mod().
//
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "sid/src/ext/mod.h"

#include "test.h"
#include "memory_device.h"


namespace {

const std::uint32_t RATE = 22050U;

typedef std::vector<std::int16_t> sound;

// a MOD file of 4 channels and 31 samples, made cell by cell
class mod_file
{
public:
  // \param patterns Patterns in the file, \param order The positions of the song
  mod_file(std::size_t patterns, const std::vector<std::uint8_t>& order)
    : bytes_(1084U + (patterns * 1024U), 0U)
    , samples_(31U)
  {
    const char name[] = "test song";
    for (std::size_t i = 0U; i < sizeof(name) - 1U; ++i) {
      bytes_[i] = static_cast<std::uint8_t>(name[i]);
    }
    bytes_[950] = static_cast<std::uint8_t>(order.size());
    bytes_[951] = 127U;
    for (std::size_t i = 0U; i < order.size(); ++i) {
      bytes_[952U + i] = order[i];
    }
    bytes_[1080] = 'M';  bytes_[1081] = '.';  bytes_[1082] = 'K';  bytes_[1083] = '.';
  }

  // the pattern of a position of the order
  void order_entry(std::size_t position, std::uint8_t pattern)
  {
    bytes_[952U + position] = pattern;
  }

  // a sample: \param number 1 .. 31, lengths and loop in bytes (even)
  void sample(std::size_t number, const std::vector<std::int8_t>& data, std::uint8_t volume = 64U,
              std::uint32_t loop_start = 0U, std::uint32_t loop_length = 0U, std::uint8_t finetune = 0U)
  {
    std::uint8_t* const h = &bytes_[20U + ((number - 1U) * 30U)];
    h[22] = static_cast<std::uint8_t>((data.size() / 2U) >> 8U);
    h[23] = static_cast<std::uint8_t>(data.size() / 2U);
    h[24] = finetune;
    h[25] = volume;
    h[26] = static_cast<std::uint8_t>((loop_start / 2U) >> 8U);
    h[27] = static_cast<std::uint8_t>(loop_start / 2U);
    h[28] = static_cast<std::uint8_t>((loop_length / 2U) >> 8U);
    h[29] = static_cast<std::uint8_t>((loop_length / 2U) != 0U ? (loop_length / 2U) : 1U);
    samples_[number - 1U] = data;
  }

  // a cell: \param number sample 0 .. 31, \param period 0 = no note
  void cell(std::size_t pattern, std::size_t row, std::size_t channel, std::uint32_t number, std::uint32_t period,
            std::uint32_t effect = 0U, std::uint32_t param = 0U)
  {
    std::uint8_t* const c = &bytes_[1084U + (pattern * 1024U) + (row * 16U) + (channel * 4U)];
    c[0] = static_cast<std::uint8_t>((number & 0xF0U) | ((period >> 8U) & 0x0FU));
    c[1] = static_cast<std::uint8_t>(period);
    c[2] = static_cast<std::uint8_t>(((number & 0x0FU) << 4U) | (effect & 0x0FU));
    c[3] = static_cast<std::uint8_t>(param);
  }

  // the file: the patterns and the samples behind them
  std::vector<std::uint8_t> bytes() const
  {
    std::vector<std::uint8_t> all = bytes_;
    for (std::size_t i = 0U; i < samples_.size(); ++i) {
      for (std::size_t s = 0U; s < samples_[i].size(); ++s) {
        all.push_back(static_cast<std::uint8_t>(samples_[i][s]));
      }
    }
    return all;
  }

private:
  std::vector<std::uint8_t>                 bytes_;
  std::vector<std::vector<std::int8_t> >    samples_;
};

// a square of 32 bytes, +-100
std::vector<std::int8_t> square()
{
  std::vector<std::int8_t> s(32U);
  for (std::size_t i = 0U; i < s.size(); ++i) {
    s[i] = static_cast<std::int8_t>((i < 16U) ? 100 : -100);
  }
  return s;
}

// a file of one pattern with the looped square as sample 1
mod_file one_pattern()
{
  mod_file f(1U, std::vector<std::uint8_t>(1U, 0U));
  f.sample(1U, square(), 64U, 0U, 32U);
  return f;
}

// all frames of a source that ends; the left channel, the right one in 'right'
sound all_of(sid::source& src, sound* right = nullptr, std::size_t limit = 20000000U)
{
  sound all;
  std::vector<std::int16_t> part(2U * 61U);
  while (all.size() < limit) {
    const std::size_t n = src.render(&part[0], 61U);
    for (std::size_t i = 0U; i < n; ++i) {
      all.push_back(part[i * 2U]);
      if (right != nullptr) {
        right->push_back(part[(i * 2U) + 1U]);
      }
    }
    if (n < 61U) {
      break;
    }
  }
  return all;
}

// frames of one tick at speed 6, tempo 125
const std::size_t TICK = (RATE * 5U) / 250U;      // 441

// part of a sound [frames]
sound cut(const sound& s, std::size_t from, std::size_t to)
{
  from = (from < s.size()) ? from : s.size();
  to   = (to < s.size()) ? to : s.size();
  return sound(s.begin() + static_cast<std::ptrdiff_t>(from), s.begin() + static_cast<std::ptrdiff_t>(to));
}

int peak(const sound& s)
{
  int p = 0;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    const int a = (s[i] < 0) ? -s[i] : s[i];
    p = (a > p) ? a : p;
  }
  return p;
}

// frequency [Hz] from the first to the last rising zero crossing
double frequency(const sound& s)
{
  double first = 0.0;
  double last  = 0.0;
  std::size_t count = 0U;
  for (std::size_t i = 1U; i < s.size(); ++i) {
    if ((s[i - 1U] < 0) && (s[i] >= 0)) {
      const double at = static_cast<double>(i - 1U)
                      + (static_cast<double>(-s[i - 1U]) / static_cast<double>(s[i] - s[i - 1U]));
      if (count == 0U) {
        first = at;
      }
      last = at;
      ++count;
    }
  }
  return (count < 2U) ? 0.0 : ((static_cast<double>(count - 1U) * static_cast<double>(RATE)) / (last - first));
}

bool near(double value, double expected, double tolerance)
{
  return std::fabs(value - expected) <= tolerance;
}

// the frequency of the square at a period: clock / period bytes per second, 32 bytes per period
double square_hz(double period)
{
  return 3546895.0 / period / 32.0;
}

// the sound of a file, played once
sound play(const std::vector<std::uint8_t>& file, std::uint32_t loops = 1U, sound* right = nullptr, std::uint32_t separation = 128U)
{
  sid::mod::info song;
  if (!sid::mod::parse(&file[0], file.size(), song)) {
    return sound();
  }
  sid::mod::player p(RATE, song, loops, separation);
  return all_of(p, right);
}

} // namespace


TEST_CASE(sid_mod_parse)
{
  mod_file f = one_pattern();
  std::vector<std::uint8_t> file = f.bytes();
  sid::mod::info song;
  REQUIRE(sid::mod::parse(&file[0], file.size(), song));
  CHECK_EQ(song.length, 1U);
  CHECK_EQ(song.patterns, 1U);
  CHECK(song.file == &file[0]);
  char text[21];
  sid::mod::title(song, text);
  CHECK(std::strcmp(text, "test song") == 0);

  const sid::mod::sample s = sid::mod::sample_of(song, 1U);
  CHECK_EQ(s.length, 32U);
  CHECK_EQ(s.loop_start, 0U);
  CHECK_EQ(s.loop_length, 32U);
  CHECK_EQ(s.volume, 64U);
  CHECK(s.data == reinterpret_cast<const std::int8_t*>(&file[1084U + 1024U]));
  CHECK_EQ(sid::mod::sample_of(song, 2U).length, 0U);      // empty
  CHECK_EQ(sid::mod::sample_of(song, 0U).length, 0U);      // no sample
  CHECK_EQ(sid::mod::sample_of(song, 32U).length, 0U);

  // the other signatures of 4 channels
  const char* const ids[3] = { "M!K!", "FLT4", "4CHN" };
  for (std::size_t i = 0U; i < 3U; ++i) {
    std::vector<std::uint8_t> other = file;
    std::memcpy(&other[1080], ids[i], 4U);
    CHECK(sid::mod::parse(&other[0], other.size(), song));
  }

  // refused: 8 channels, no signature (15 samples), no positions, patterns cut off, too short, nothing
  std::vector<std::uint8_t> bad = file;
  std::memcpy(&bad[1080], "8CHN", 4U);
  CHECK(!sid::mod::parse(&bad[0], bad.size(), song));
  bad = file;
  std::memcpy(&bad[1080], "\0\0\0\0", 4U);
  CHECK(!sid::mod::parse(&bad[0], bad.size(), song));
  bad = file;
  bad[950] = 0U;
  CHECK(!sid::mod::parse(&bad[0], bad.size(), song));
  bad = file;
  bad[952U + 5U] = 3U;      // an unused position names pattern 3: four patterns, one is in the file
  CHECK(!sid::mod::parse(&bad[0], bad.size(), song));
  CHECK(!sid::mod::parse(&file[0], 1084U + 1000U, song));
  CHECK(!sid::mod::parse(&file[0], 1083U, song));
  CHECK(!sid::mod::parse(nullptr, file.size(), song));

  // a file whose sample is cut off plays as far as it goes; the loop is shortened
  REQUIRE(sid::mod::parse(&file[0], file.size() - 10U, song));
  const sid::mod::sample cut_off = sid::mod::sample_of(song, 1U);
  CHECK_EQ(cut_off.length, 22U);
  CHECK_EQ(cut_off.loop_length, 22U);
  REQUIRE(sid::mod::parse(&file[0], 1084U + 1024U, song));
  CHECK_EQ(sid::mod::sample_of(song, 1U).length, 0U);
  CHECK(sid::mod::sample_of(song, 1U).data == nullptr);
}


TEST_CASE(sid_mod_pitch)
{
  // C-2 (period 428) and the octave above, C-3 (214)
  mod_file f = one_pattern();
  f.cell(0U, 0U, 0U, 1U, 428U);
  f.cell(0U, 8U, 0U, 0U, 214U);
  const sound s = play(f.bytes());
  REQUIRE(s.size() > (16U * 6U * TICK));
  CHECK(near(frequency(cut(s, 100U, 8U * 6U * TICK)), square_hz(428.0), 0.5));
  CHECK(near(frequency(cut(s, (8U * 6U * TICK) + 100U, 16U * 6U * TICK)), square_hz(214.0), 1.0));

  // finetune -8 is a semitone lower, +4 half a semitone higher
  mod_file low = one_pattern();
  low.sample(1U, square(), 64U, 0U, 32U, 8U);
  low.cell(0U, 0U, 0U, 1U, 428U);
  CHECK(near(frequency(cut(play(low.bytes()), 100U, 6U * 6U * TICK)), square_hz(428.0) / 1.0594631, 0.5));
  mod_file high = one_pattern();
  high.sample(1U, square(), 64U, 0U, 32U, 4U);
  high.cell(0U, 0U, 0U, 1U, 428U);
  CHECK(near(frequency(cut(play(high.bytes()), 100U, 6U * 6U * TICK)), square_hz(428.0) * 1.0293022, 0.6));

  // arpeggio 047: the note, 4 and 7 semitones above it, one tick each
  mod_file arp = one_pattern();
  arp.cell(0U, 0U, 0U, 1U, 428U, 0x0U, 0x47U);
  const sound a = play(arp.bytes());
  CHECK(near(frequency(cut(a, 100U, TICK - 20U)), square_hz(428.0), 1.5));
  CHECK(near(frequency(cut(a, TICK + 100U, (2U * TICK) - 20U)), square_hz(340.0), 1.5));
  CHECK(near(frequency(cut(a, (2U * TICK) + 100U, (3U * TICK) - 20U)), square_hz(285.0), 2.0));

  // portamento up by 10 per tick: after the row the period is 428 - 5 * 10
  mod_file up = one_pattern();
  up.cell(0U, 0U, 0U, 1U, 428U, 0x1U, 10U);
  const sound u = play(up.bytes());
  CHECK(near(frequency(cut(u, (6U * TICK) + 50U, 12U * TICK)), square_hz(378.0), 1.0));

  // tone portamento: from 428 towards 214 by 50 per tick, then held there
  mod_file tone = one_pattern();
  tone.cell(0U, 0U, 0U, 1U, 428U);
  tone.cell(0U, 1U, 0U, 0U, 214U, 0x3U, 50U);
  tone.cell(0U, 2U, 0U, 0U, 0U, 0x3U, 0U);
  const sound t = play(tone.bytes());
  CHECK(near(frequency(cut(t, (7U * TICK) + 50U, 8U * TICK)), square_hz(378.0), 3.0));
  CHECK(near(frequency(cut(t, (13U * TICK) + 50U, 24U * TICK)), square_hz(214.0), 1.0));

  // vibrato moves the pitch around the note, and the note stays
  mod_file vib = one_pattern();
  vib.cell(0U, 0U, 0U, 1U, 428U, 0x4U, 0xFFU);      // speed 15: one row reaches both sides (the first half lowers)
  const sound v = play(vib.bytes());
  double lowest  = 1.0e9;
  double highest = 0.0;
  for (std::size_t tick = 1U; tick < 6U; ++tick) {
    const double hz = frequency(cut(v, (tick * TICK) + 50U, ((tick + 1U) * TICK) - 10U));
    lowest  = (hz < lowest) ? hz : lowest;
    highest = (hz > highest) ? hz : highest;
  }
  CHECK(highest > (square_hz(428.0) + 3.0));
  CHECK(lowest < (square_hz(428.0) - 3.0));
  CHECK(near(frequency(cut(v, (6U * TICK) + 50U, 12U * TICK)), square_hz(428.0), 0.5));     // the next row: the note
}


TEST_CASE(sid_mod_volume)
{
  mod_file f = one_pattern();
  f.cell(0U, 0U, 0U, 1U, 428U);
  f.cell(0U, 2U, 0U, 0U, 0U, 0xCU, 32U);            // half
  f.cell(0U, 4U, 0U, 0U, 0U, 0xAU, 0x04U);          // down by 4 per tick: 32 - 5 * 4 = 12
  f.cell(0U, 6U, 0U, 0U, 0U, 0xEU, 0xA8U);          // fine up by 8: 20
  f.cell(0U, 8U, 0U, 0U, 0U, 0xEU, 0xC3U);          // cut at tick 3
  f.cell(0U, 10U, 0U, 1U, 0U);                      // the sample number alone: its volume again
  const sound s = play(f.bytes(), 1U, nullptr, 0U);   // mono: channel 0 is a quarter of full scale in both
  const std::size_t row = 6U * TICK;
  const int full = peak(cut(s, 10U, 2U * row));
  CHECK(near(full, 100 * 256 / 4, 200));
  CHECK(near(peak(cut(s, (2U * row) + 10U, 4U * row)), full / 2, 100));
  CHECK(near(peak(cut(s, (5U * row) + 10U, 6U * row)), (full * 12) / 64, 100));
  CHECK(near(peak(cut(s, (6U * row) + 10U, 8U * row)), (full * 20) / 64, 100));
  CHECK(near(peak(cut(s, (8U * row) + 10U, (8U * row) + (3U * TICK))), (full * 20) / 64, 100));
  CHECK_EQ(peak(cut(s, (8U * row) + (3U * TICK) + 10U, 10U * row)), 0);
  CHECK(near(peak(cut(s, (10U * row) + 10U, 12U * row)), full, 100));

  // a sample without a loop plays once and ends
  mod_file once(1U, std::vector<std::uint8_t>(1U, 0U));
  once.sample(1U, square());
  once.cell(0U, 0U, 0U, 1U, 428U);
  const sound o = play(once.bytes());
  CHECK(peak(cut(o, 0U, 100U)) > 1000);
  CHECK_EQ(peak(cut(o, 200U, 2U * row)), 0);

  // note delay: the note starts at tick 2
  mod_file late = one_pattern();
  late.cell(0U, 0U, 0U, 1U, 428U, 0xEU, 0xD2U);
  const sound d = play(late.bytes());
  CHECK_EQ(peak(cut(d, 0U, 2U * TICK)), 0);
  CHECK(peak(cut(d, 2U * TICK, 3U * TICK)) > 1000);

  // sample offset: starts 256 bytes into a long sample whose first half is silent
  std::vector<std::int8_t> half(512U, 0);
  for (std::size_t i = 256U; i < half.size(); ++i) {
    half[i] = static_cast<std::int8_t>(((i / 16U) % 2U) == 0U ? 100 : -100);
  }
  mod_file off(1U, std::vector<std::uint8_t>(1U, 0U));
  off.sample(1U, half);
  off.cell(0U, 0U, 0U, 1U, 428U, 0x9U, 1U);
  CHECK(peak(cut(play(off.bytes()), 0U, 100U)) > 1000);
}


TEST_CASE(sid_mod_song_length)
{
  // one pattern at speed 6, tempo 125: 64 rows of 6 ticks of 20 ms, and the fade of 10 ms
  const std::size_t fade = RATE / 100U;
  CHECK_EQ(play(one_pattern().bytes()).size(), (64U * 6U * TICK) + fade);

  // speed 3 and tempo 150 (a tick of 367.5 frames: 367 and 368 in turn)
  mod_file fast = one_pattern();
  fast.cell(0U, 0U, 1U, 0U, 0U, 0xFU, 3U);
  fast.cell(0U, 0U, 2U, 0U, 0U, 0xFU, 150U);
  CHECK_EQ(play(fast.bytes()).size(), ((64U * 3U * RATE * 5U) / 300U) + fade);

  // F00 stops the song after its row
  mod_file stop = one_pattern();
  stop.cell(0U, 9U, 0U, 0U, 0U, 0xFU, 0U);
  CHECK_EQ(play(stop.bytes()).size(), (10U * 6U * TICK) + fade);

  // two positions; D10 in row 3 of the first goes to row 10 of the second
  mod_file brk(2U, std::vector<std::uint8_t>(2U, 0U));
  brk.order_entry(1U, 1U);
  brk.cell(0U, 3U, 0U, 0U, 0U, 0xDU, 0x10U);
  CHECK_EQ(play(brk.bytes()).size(), ((4U + 54U) * 6U * TICK) + fade);

  // B00 in row 7: a jump back to the start is the end of the song; with 3 loops it plays three times, with 0
  // loops for ever
  mod_file loop = one_pattern();
  loop.cell(0U, 7U, 0U, 0U, 0U, 0xBU, 0U);
  CHECK_EQ(play(loop.bytes()).size(), (8U * 6U * TICK) + fade);
  CHECK_EQ(play(loop.bytes(), 3U).size(), (3U * 8U * 6U * TICK) + fade);
  {
    const std::vector<std::uint8_t> file = loop.bytes();
    sid::mod::info song;
    REQUIRE(sid::mod::parse(&file[0], file.size(), song));
    sid::mod::player forever(RATE, song, 0U);
    const sound s = all_of(forever, nullptr, 50U * 8U * 6U * TICK);
    CHECK(s.size() >= (50U * 8U * 6U * TICK));
    CHECK(!forever.ended());
  }

  // B and D in one row: position 1, row 5
  mod_file both(2U, std::vector<std::uint8_t>(3U, 0U));
  both.order_entry(1U, 1U);
  both.order_entry(2U, 0U);
  both.cell(0U, 0U, 0U, 0U, 0U, 0xBU, 2U);
  both.cell(0U, 0U, 1U, 0U, 0U, 0xDU, 0x05U);
  CHECK_EQ(play(both.bytes()).size(), ((1U + 59U) * 6U * TICK) + fade);

  // where the player is
  mod_file where(2U, std::vector<std::uint8_t>(2U, 0U));
  where.order_entry(1U, 1U);
  const std::vector<std::uint8_t> file = where.bytes();
  sid::mod::info song;
  REQUIRE(sid::mod::parse(&file[0], file.size(), song));
  sid::mod::player p(RATE, song);
  std::vector<std::int16_t> buffer(2U * TICK);
  for (std::size_t i = 0U; i < ((64U + 3U) * 6U); ++i) {
    CHECK_EQ(p.render(&buffer[0], TICK), TICK);
  }
  CHECK_EQ(p.order(), 1U);
  CHECK_EQ(p.row(), 2U);      // the third row of the second position has played; the fourth comes next
  CHECK(!p.ended());
}


TEST_CASE(sid_mod_stereo)
{
  // channel 0 is left on the Amiga, channel 1 right
  mod_file f = one_pattern();
  f.cell(0U, 0U, 0U, 1U, 428U);
  sound right;
  sound left = play(f.bytes(), 1U, &right, 255U);
  CHECK(peak(left) > 12000);
  CHECK(peak(right) < 100);
  right.clear();
  left = play(f.bytes(), 1U, &right, 0U);
  CHECK(left == right);
  CHECK(near(peak(left), 100 * 256 / 4, 200));

  mod_file g = one_pattern();
  g.cell(0U, 0U, 1U, 1U, 428U);
  right.clear();
  left = play(g.bytes(), 1U, &right, 255U);
  CHECK(peak(left) < 100);
  CHECK(peak(right) > 12000);

  // all four channels at full scale do not clip
  std::vector<std::int8_t> loud(32U);
  for (std::size_t i = 0U; i < loud.size(); ++i) {
    loud[i] = static_cast<std::int8_t>((i < 16U) ? 127 : -128);
  }
  mod_file h(1U, std::vector<std::uint8_t>(1U, 0U));
  h.sample(1U, loud, 64U, 0U, 32U);
  for (std::size_t c = 0U; c < 4U; ++c) {
    h.cell(0U, 0U, c, 1U, 428U);
  }
  right.clear();
  left = play(h.bytes(), 1U, &right, 255U);
  CHECK(peak(left) <= 32767);
  CHECK(peak(left) > 32000);
}


TEST_CASE(sid_mod_play)
{
  mod_file f = one_pattern();
  f.cell(0U, 0U, 0U, 1U, 428U);
  f.cell(0U, 3U, 0U, 0U, 0U, 0xFU, 0U);
  const std::vector<std::uint8_t> file = f.bytes();

  memory_device dev;
  sid::output out(dev);
  CHECK(!sid::play_mod(out, &file[0], file.size()));      // not open
  REQUIRE(out.open(RATE));
  CHECK(!sid::play_mod(out, &file[0], 100U));             // no MOD file
  CHECK(sid::play_mod(out, &file[0], file.size()));
  CHECK_EQ(dev.frames(), (4U * 6U * TICK) + (RATE / 100U));
  // the end fades out
  const std::vector<std::int16_t>& s = dev.samples();
  CHECK(std::abs(s[s.size() - 2U]) < 200);
  out.close();

  // any sample rate
  memory_device other;
  sid::output out2(other);
  REQUIRE(out2.open(44100U));
  CHECK(sid::play_mod(out2, &file[0], file.size()));
  CHECK_EQ(other.frames(), (4U * 6U * 882U) + 441U);
}
