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
// \brief sid tests: the mixer (ext/mixer.h)
//        The inputs are sources of a constant value and a known length, so every frame of the sum is known:
//        the sum and its length, the levels, clipping, the inputs (full, twice, ended, removed), the endless
//        mixer, and a mixer of tones on an output.
//
///////////////////////////////////////////////////////////////////////////////

#include <vector>

#include "sid/src/tone.h"
#include "sid/src/ext/mixer.h"

#include "test.h"
#include "memory_device.h"


namespace {

typedef std::vector<std::int16_t> sound;

// a source of a constant value on each channel, which ends after its frames
class constant : public sid::source
{
private:
  std::int16_t    left_;
  std::int16_t    right_;
  std::size_t     frames_;      // frames left
  std::size_t     calls_;       // calls of render()

public:
  constant(int left, int right, std::size_t frames)
    : left_(static_cast<std::int16_t>(left))
    , right_(static_cast<std::int16_t>(right))
    , frames_(frames)
    , calls_(0U)
  { }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    ++calls_;
    std::size_t n = 0U;
    for (; (n < frames) && (frames_ != 0U); ++n, --frames_) {
      stereo[n * 2U]      = left_;
      stereo[n * 2U + 1U] = right_;
    }
    return n;
  }

  std::size_t calls() const
  { return calls_; }
};

// all frames of a source, rendered in chunks of a size
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

// true if the frames first .. last - 1 all have these values
bool frames_are(const sound& s, std::size_t first, std::size_t last, int left, int right)
{
  if ((last * 2U) > s.size()) {
    return false;
  }
  for (std::size_t i = first; i < last; ++i) {
    if ((s[i * 2U] != left) || (s[i * 2U + 1U] != right)) {
      return false;
    }
  }
  return true;
}

} // namespace


TEST_CASE(sid_mixer_sum)
{
  // chunks smaller and larger than the pieces the mixer works in, and of their size
  static const std::size_t chunks[] = { 1U, 37U, 64U, 100U, 1000U };
  for (std::size_t c = 0U; c < (sizeof(chunks) / sizeof(chunks[0])); ++c) {
    constant a(1000, -2000, 100U);
    constant b(30, 40, 250U);
    constant d(-5, 7, 180U);
    sid::mixer<3> mix;
    CHECK(mix.add(a));
    CHECK(mix.add(b));
    CHECK(mix.add(d));
    const sound s = render(mix, chunks[c]);
    CHECK_EQ(s.size() / 2U, 250U);                      // as long as its longest input
    CHECK(frames_are(s, 0U, 100U, 1025, -1953));        // all three
    CHECK(frames_are(s, 100U, 180U, 25, 47));           // a has ended
    CHECK(frames_are(s, 180U, 250U, 30, 40));           // only b is left
    CHECK_EQ(mix.inputs(), 0U);
  }

  // without an input the mixer has ended
  sid::mixer<2> empty;
  std::int16_t buffer[16] = { 0 };
  CHECK_EQ(empty.render(buffer, 8U), 0U);
  CHECK_EQ(empty.render(buffer, 0U), 0U);

  // an input of exactly one piece: the mixer ends with it
  constant piece(9, 9, SID_CHUNK_FRAMES);
  sid::mixer<1> one;
  CHECK(one.add(piece));
  CHECK_EQ(render(one, SID_CHUNK_FRAMES).size() / 2U, static_cast<std::size_t>(SID_CHUNK_FRAMES));
}


TEST_CASE(sid_mixer_level)
{
  // full scale changes nothing, also not the most negative value
  {
    constant a(32767, -32768, 50U);
    sid::mixer<1> mix;
    CHECK(mix.add(a, 255U));
    CHECK(frames_are(render(mix), 0U, 50U, 32767, -32768));
  }
  // a level above full scale is full scale
  {
    constant a(12345, -12345, 50U);
    sid::mixer<1> mix;
    CHECK(mix.add(a, 1000U));
    CHECK(frames_are(render(mix), 0U, 50U, 12345, -12345));
  }
  // level 0 is silence, but the input plays and ends
  {
    constant a(20000, -20000, 50U);
    sid::mixer<1> mix;
    CHECK(mix.add(a, 0U));
    const sound s = render(mix);
    CHECK_EQ(s.size() / 2U, 50U);
    CHECK(frames_are(s, 0U, 50U, 0, 0));
  }
  // the levels between: in proportion
  for (std::uint32_t level = 1U; level < 255U; level += 23U) {
    constant a(25500, -25500, 20U);
    sid::mixer<1> mix;
    CHECK(mix.add(a, level));
    const sound s = render(mix);
    REQUIRE(s.size() == 40U);
    const int expected = static_cast<int>(level) * 100;
    CHECK_MSG((s[0] >= (expected - 100)) && (s[0] <= (expected + 100)), "left");
    CHECK_EQ(s[1], -s[0]);
  }
  // each input has its own level, and it can be changed while it plays
  {
    constant a(10000, 10000, 100U);
    constant b(256, 256, 100U);
    sid::mixer<2> mix;
    CHECK(mix.add(a, 0U));
    CHECK(mix.add(b, 255U));
    std::int16_t buffer[80];
    CHECK_EQ(mix.render(buffer, 40U), 40U);
    CHECK_EQ(buffer[0], 256);
    CHECK(mix.set_level(a, 255U));
    CHECK(mix.set_level(b, 0U));
    CHECK_EQ(mix.render(buffer, 40U), 40U);
    CHECK_EQ(buffer[0], 10000);
    constant other(1, 1, 1U);
    CHECK(!mix.set_level(other, 255U));       // no input
  }
}


TEST_CASE(sid_mixer_clip)
{
  constant a(30000, -30000, 30U);
  constant b(20000, -20000, 30U);
  constant c(-1000, 1000, 30U);
  sid::mixer<3> mix;
  CHECK(mix.add(a));
  CHECK(mix.add(b));
  CHECK(mix.add(c));
  // clipped at full scale, not wrapped; what comes behind is added to the clipped sum
  CHECK(frames_are(render(mix), 0U, 30U, 31767, -31768));
}


TEST_CASE(sid_mixer_inputs)
{
  constant a(1, 1, 100U);
  constant b(2, 2, 100U);
  constant c(4, 4, 300U);
  sid::mixer<2> mix;
  CHECK_EQ(mix.inputs(), 0U);
  CHECK(!mix.playing(a));
  CHECK(mix.add(a));
  CHECK(mix.add(b));
  CHECK(!mix.add(c));                         // full
  CHECK_EQ(mix.inputs(), 2U);
  CHECK(mix.playing(a) && mix.playing(b) && !mix.playing(c));

  // an input that is added again is one input: it plays once, with the new level
  CHECK(mix.add(a, 0U));
  CHECK_EQ(mix.inputs(), 2U);
  std::int16_t buffer[128];
  CHECK_EQ(mix.render(buffer, 50U), 50U);
  CHECK_EQ(a.calls(), 1U);
  CHECK_EQ(buffer[0], 2);

  // removed: silent at once, the place is free
  mix.remove(b);
  CHECK(!mix.playing(b));
  CHECK(mix.add(c));
  CHECK_EQ(mix.render(buffer, 20U), 20U);
  CHECK_EQ(buffer[0], 4);
  CHECK_EQ(b.calls(), 1U);
  mix.remove(b);                              // no input: nothing happens
  CHECK_EQ(mix.inputs(), 2U);

  // an input that has ended is taken out: a has 30 frames left
  CHECK_EQ(mix.render(buffer, 64U), 64U);
  CHECK(!mix.playing(a));
  CHECK(mix.playing(c));
  CHECK_EQ(mix.inputs(), 1U);
  constant d(8, 8, 10U);
  CHECK(mix.add(d));                          // in its place
  CHECK_EQ(mix.render(buffer, 20U), 20U);
  CHECK_EQ(buffer[0], 12);
  CHECK_EQ(buffer[38], 4);                    // d has ended
  CHECK(!mix.playing(d));

  mix.clear();
  CHECK_EQ(mix.inputs(), 0U);
  CHECK_EQ(mix.render(buffer, 20U), 0U);
}


TEST_CASE(sid_mixer_endless)
{
  sid::mixer<2> mix(true);
  std::int16_t buffer[400];
  for (std::size_t i = 0U; i < 400U; ++i) {
    buffer[i] = 1234;
  }
  // silence while there is no input
  CHECK_EQ(mix.render(buffer, 200U), 200U);
  bool silent = true;
  for (std::size_t i = 0U; i < 400U; ++i) {
    silent = silent && (buffer[i] == 0);
  }
  CHECK(silent);

  // an input sounds, ends and leaves silence
  constant a(500, 600, 70U);
  CHECK(mix.add(a));
  CHECK_EQ(mix.render(buffer, 200U), 200U);
  const sound s(buffer, buffer + 400);
  CHECK(frames_are(s, 0U, 70U, 500, 600));
  CHECK(frames_are(s, 70U, 200U, 0, 0));
  CHECK(!mix.playing(a));
  CHECK_EQ(mix.render(buffer, 200U), 200U);
  CHECK_EQ(buffer[0], 0);
}


TEST_CASE(sid_mixer_output)
{
  memory_device dev;
  sid::output out(dev);
  REQUIRE(out.open(22050U));

  // a tone alone, and the same tone with a second one of twice its length on top
  sid::tone alone(out.rate());
  alone.set(440U, 100U, sid::wave_square, 100U);
  CHECK(out.play(alone));
  const sound first = dev.samples();
  REQUIRE(first.size() == (2205U * 2U));

  sid::tone low(out.rate());
  sid::tone high(out.rate());
  low.set(440U, 100U, sid::wave_square, 100U);
  high.set(1320U, 200U, sid::wave_sine, 60U);
  sid::mixer<2> mix;
  CHECK(mix.add(low));
  CHECK(mix.add(high));
  CHECK(out.play(mix));
  out.close();
  const sound all = dev.samples();
  REQUIRE(all.size() == ((2205U + 4410U) * 2U));

  // the sum is the two tones: what is left without the first one is the second, a sine of 60 / 255 of full scale
  sid::tone again(22050U);
  again.set(1320U, 200U, sid::wave_sine, 60U);
  const sound second = render(again);
  REQUIRE(second.size() == (4410U * 2U));
  bool equal = true;
  for (std::size_t i = 0U; i < second.size(); ++i) {
    const int rest = all[first.size() + i] - ((i < first.size()) ? first[i] : 0);
    equal = equal && (rest == second[i]);
  }
  CHECK(equal);
}
