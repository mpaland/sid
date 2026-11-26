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
// \brief sid tests: the level meter (ext/meter.h)
//        The sources are constant values of a known length, so every peak is known: the frames pass through
//        as they are, the linear and the logarithmic scale (against decibels in floating point), the switch
//        between them, the fall of the level and the end of the source.
//
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <vector>

#include "sid/src/tone.h"
#include "sid/src/ext/meter.h"

#include "test.h"


namespace {

// a source of constant values, one after the other, which ends after the last
class levels : public sid::source
{
private:
  typedef struct tag_step {
    std::int16_t    left;
    std::int16_t    right;
    std::size_t     frames;
  } step;

  std::vector<step>   steps_;
  std::size_t         index_;

public:
  levels()
    : index_(0U)
  { }

  levels(int left, int right, std::size_t frames)
    : index_(0U)
  { (void)add(left, right, frames); }

  levels& add(int left, int right, std::size_t frames)
  {
    const step s = { static_cast<std::int16_t>(left), static_cast<std::int16_t>(right), frames };
    steps_.push_back(s);
    return *this;
  }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    while ((n < frames) && (index_ < steps_.size())) {
      step& s = steps_[index_];
      if (s.frames == 0U) {
        ++index_;
        continue;
      }
      stereo[n * 2U]      = s.left;
      stereo[n * 2U + 1U] = s.right;
      --s.frames;
      ++n;
    }
    return n;
  }
};

// render frames of a source in chunks; returns the frames it gave
std::size_t run(sid::source& src, std::size_t frames, std::size_t chunk = SID_CHUNK_FRAMES)
{
  std::vector<std::int16_t> buffer(chunk * 2U);
  std::size_t done = 0U;
  while (done < frames) {
    const std::size_t want = ((frames - done) < chunk) ? (frames - done) : chunk;
    const std::size_t got  = src.render(&buffer[0], want);
    done += got;
    if (got < want) {
      break;
    }
  }
  return done;
}

// what a meter reads for one sample on the left channel
std::uint32_t reading(int sample, sid::meter_type scale, std::uint32_t range = 48U)
{
  levels src(sample, 0, 1U);
  sid::meter vu(48000U, src, scale);
  vu.set_range(range);
  std::int16_t frame[2] = { 0, 0 };
  (void)vu.render(frame, 1U);
  return vu.left();
}

// what the logarithmic scale is to read for an amplitude, in floating point
int log_level(double amplitude, double range)
{
  if (amplitude < 1.0) {
    return 0;
  }
  const double down  = -20.0 * std::log10(amplitude / 32768.0);
  const double level = 255.0 - ((255.0 * down) / range);
  return (level <= 0.0) ? 0 : static_cast<int>(level + 0.5);
}

bool close_to(std::uint32_t value, int expected, int tolerance)
{
  const int diff = static_cast<int>(value) - expected;
  return (diff >= -tolerance) && (diff <= tolerance);
}

} // namespace


TEST_CASE(sid_meter_through)
{
  // the frames of a tone through a meter are the frames of the tone, in chunks of any size
  static const std::size_t chunks[] = { 1U, 37U, 64U, 100U, 1000U };
  for (std::size_t c = 0U; c < (sizeof(chunks) / sizeof(chunks[0])); ++c) {
    sid::tone plain(22050U);
    sid::tone inside(22050U);
    plain.set(440U, 50U);
    inside.set(440U, 50U);
    sid::meter vu(22050U, inside);

    std::vector<std::int16_t> a(chunks[c] * 2U);
    std::vector<std::int16_t> b(chunks[c] * 2U);
    std::size_t frames = 0U;
    bool same = true;
    bool loud = false;
    for (;;) {
      const std::size_t na = plain.render(&a[0], chunks[c]);
      const std::size_t nb = vu.render(&b[0], chunks[c]);
      same = same && (na == nb);
      for (std::size_t i = 0U; same && (i < (na * 2U)); ++i) {
        same = (a[i] == b[i]);
      }
      frames += nb;
      if ((na < chunks[c]) || (nb < chunks[c])) {
        break;
      }
      loud = loud || (vu.level() > 200U);
    }
    CHECK(same);
    CHECK_EQ(frames, 1102U);          // 50 ms
    CHECK(loud);                      // level 128 of the tone: -6 dB
    CHECK_EQ(vu.level(), 0U);         // the tone has ended
  }

  // nothing asked, nothing changed
  levels src(1000, 1000, 10U);
  sid::meter vu(22050U, src);
  std::int16_t frame[2] = { 0, 0 };
  CHECK_EQ(vu.render(frame, 0U), 0U);
  CHECK_EQ(vu.render(frame, 1U), 1U);
  CHECK(vu.level() != 0U);
  CHECK_EQ(vu.render(frame, 0U), 0U);
  CHECK(vu.level() != 0U);            // no frame asked is not the end
}


TEST_CASE(sid_meter_linear)
{
  // the level is the amplitude, of both signs
  CHECK_EQ(reading(0, sid::meter_linear), 0U);
  CHECK_EQ(reading(64, sid::meter_linear), 0U);
  CHECK_EQ(reading(65, sid::meter_linear), 1U);
  CHECK_EQ(reading(8192, sid::meter_linear), 64U);
  CHECK_EQ(reading(16384, sid::meter_linear), 128U);
  CHECK_EQ(reading(-16384, sid::meter_linear), 128U);
  CHECK_EQ(reading(32767, sid::meter_linear), 255U);
  CHECK_EQ(reading(-32768, sid::meter_linear), 255U);

  // the range of the logarithmic scale is nothing to the linear one
  CHECK_EQ(reading(16384, sid::meter_linear, 6U), 128U);

  // the channels apart, the level is the higher one
  levels src(100, -20000, 10U);
  sid::meter vu(48000U, src, sid::meter_linear);
  CHECK_EQ(run(vu, 5U), 5U);
  CHECK_EQ(vu.left(), 1U);
  CHECK_EQ(vu.right(), 156U);
  CHECK_EQ(vu.level(), 156U);

  // the peak of the frames counts, wherever it is in them
  levels peak;
  peak.add(1000, 0, 10U).add(30000, 0, 1U).add(1000, 0, 10U).add(0, -30000, 1U).add(0, 0, 10U);
  sid::meter vu2(48000U, peak, sid::meter_linear);
  CHECK_EQ(run(vu2, 32U), 32U);
  CHECK(close_to(vu2.left(), 233, 1));
  CHECK(close_to(vu2.right(), 233, 1));
}


TEST_CASE(sid_meter_log)
{
  // 255 is full scale, 0 is the range below it: 48 dB
  CHECK_EQ(reading(-32768, sid::meter_log), 255U);
  CHECK_EQ(reading(32767, sid::meter_log), 255U);
  CHECK_EQ(reading(16384, sid::meter_log), 223U);       //  -6.02 dB
  CHECK_EQ(reading(-16384, sid::meter_log), 223U);
  CHECK_EQ(reading(2048, sid::meter_log), 127U);        // -24.08 dB
  CHECK_EQ(reading(256, sid::meter_log), 31U);          // -42.14 dB
  CHECK_EQ(reading(128, sid::meter_log), 0U);           // -48.16 dB
  CHECK_EQ(reading(1, sid::meter_log), 0U);
  CHECK_EQ(reading(0, sid::meter_log), 0U);

  // every amplitude against decibels in floating point; a higher amplitude never reads lower
  static const std::uint32_t ranges[] = { 1U, 6U, 24U, 48U, 60U, 96U };
  for (std::size_t r = 0U; r < (sizeof(ranges) / sizeof(ranges[0])); ++r) {
    std::size_t   wrong   = 0U;
    std::size_t   falling = 0U;
    std::uint32_t before  = 0U;
    for (int sample = 0; sample <= 32768; ++sample) {
      levels one(-sample, 0, 2U);
      sid::meter m(48000U, one);
      m.set_range(ranges[r]);
      std::int16_t frame[2] = { 0, 0 };
      (void)m.render(frame, 1U);
      const std::uint32_t level = m.left();
      wrong   += close_to(level, log_level(sample, ranges[r]), 1) ? 0U : 1U;
      falling += (level < before) ? 1U : 0U;
      before   = level;
    }
    CHECK_EQ(wrong, 0U);
    CHECK_EQ(falling, 0U);
    CHECK_EQ(before, 255U);
  }

  // other ranges
  CHECK_EQ(reading(16384, sid::meter_log, 24U), 191U);
  CHECK_EQ(reading(2048, sid::meter_log, 24U), 0U);
  CHECK_EQ(reading(1, sid::meter_log, 96U), 15U);       // -90.3 dB

  // the range is limited to 1 .. 96 dB
  CHECK_EQ(reading(1, sid::meter_log, 1000U), 15U);
  CHECK_EQ(reading(30000, sid::meter_log, 0U), reading(30000, sid::meter_log, 1U));
  CHECK(reading(30000, sid::meter_log, 1U) != 0U);      // -0.77 dB
  CHECK_EQ(reading(29000, sid::meter_log, 1U), 0U);     // -1.06 dB
}


TEST_CASE(sid_meter_switch)
{
  levels src(16384, 4096, 100U);
  sid::meter vu(48000U, src);
  CHECK(vu.scale() == sid::meter_log);                  // the scale a meter has if none is given
  CHECK_EQ(run(vu, 1U), 1U);
  CHECK_EQ(vu.left(), 223U);
  CHECK_EQ(vu.right(), 159U);                           // -18.06 dB

  // the same peak in the other scale, without a frame in between
  vu.set_scale(sid::meter_linear);
  CHECK(vu.scale() == sid::meter_linear);
  CHECK_EQ(vu.left(), 128U);
  CHECK_EQ(vu.right(), 32U);
  CHECK_EQ(vu.level(), 128U);

  // and back, also while it plays
  CHECK_EQ(run(vu, 10U), 10U);
  vu.set_scale(sid::meter_log);
  CHECK_EQ(vu.left(), 223U);
  CHECK_EQ(vu.level(), 223U);

  // a meter that is linear from the start
  levels src2(16384, 0, 100U);
  sid::meter linear(48000U, src2, sid::meter_linear);
  CHECK_EQ(run(linear, 1U), 1U);
  CHECK_EQ(linear.left(), 128U);
}


TEST_CASE(sid_meter_fall)
{
  const std::uint32_t rate = 48000U;

  // after a burst the level falls by 20 dB in 300 ms, and by 10 dB in half of it, in chunks of any size
  static const std::size_t chunks[] = { 1U, 7U, 64U, 100U, 4096U };
  for (std::size_t c = 0U; c < (sizeof(chunks) / sizeof(chunks[0])); ++c) {
    levels src;
    src.add(32000, 0, 64U).add(0, 0, 100000U);
    sid::meter vu(rate, src);
    vu.set_range(96U);
    CHECK_EQ(run(vu, 64U, chunks[c]), 64U);
    CHECK_EQ(vu.left(), static_cast<std::uint32_t>(log_level(32000.0, 96.0)));
    CHECK_EQ(run(vu, 7200U, chunks[c]), 7200U);
    CHECK(close_to(vu.left(), log_level(32000.0 / std::sqrt(10.0), 96.0), 1));
    CHECK_EQ(run(vu, 7200U, chunks[c]), 7200U);
    CHECK(close_to(vu.left(), log_level(3200.0, 96.0), 1));
    vu.set_scale(sid::meter_linear);
    CHECK(close_to(vu.left(), 25, 1));                  // a tenth of 249
    CHECK_EQ(vu.right(), 0U);
  }

  // it rises at once, and what is below the level does not lift it
  {
    levels src;
    src.add(32000, 0, 64U).add(0, 0, 14400U).add(100, 0, 1U).add(20000, 0, 1U).add(0, 0, 100U);
    sid::meter vu(rate, src);
    vu.set_range(96U);
    CHECK_EQ(run(vu, 64U + 14400U), 64U + 14400U);
    const std::uint32_t low = vu.left();
    CHECK(close_to(low, log_level(3200.0, 96.0), 1));
    CHECK_EQ(run(vu, 1U), 1U);
    CHECK_EQ(vu.left(), low);
    CHECK_EQ(run(vu, 1U), 1U);
    CHECK_EQ(vu.left(), static_cast<std::uint32_t>(log_level(20000.0, 96.0)));
  }

  // another time of the fall: 20 dB in 50 ms; the channels fall on their own
  {
    levels src;
    src.add(30000, 30000, 100U).add(0, 30000, 2400U).add(0, 0, 10U);
    sid::meter vu(rate, src);
    vu.set_range(96U);
    vu.set_fall(50U);
    CHECK_EQ(run(vu, 2500U), 2500U);
    CHECK(close_to(vu.left(), log_level(3000.0, 96.0), 1));
    CHECK_EQ(vu.right(), static_cast<std::uint32_t>(log_level(30000.0, 96.0)));
    CHECK_EQ(vu.level(), vu.right());
  }

  // a long fall: hardly anything in 300 ms (0.6 dB)
  {
    levels src;
    src.add(32000, 0, 64U).add(0, 0, 100000U);
    sid::meter vu(rate, src);
    vu.set_range(96U);
    vu.set_fall(10000U);
    CHECK_EQ(run(vu, 64U + 14400U), 64U + 14400U);
    CHECK(close_to(vu.left(), log_level(32000.0 / std::pow(10.0, 0.03), 96.0), 1));
  }

  // no fall: the level is the peak of the last frames
  {
    levels src;
    src.add(32000, 0, 64U).add(0, 0, 1U).add(500, 0, 100U);
    sid::meter vu(rate, src, sid::meter_linear);
    vu.set_fall(0U);
    CHECK_EQ(run(vu, 64U), 64U);
    CHECK_EQ(vu.left(), 249U);
    CHECK_EQ(run(vu, 1U), 1U);
    CHECK_EQ(vu.left(), 0U);
    CHECK_EQ(run(vu, 50U), 50U);
    CHECK_EQ(vu.left(), 4U);
  }

  // without a sample rate there is no time: no fall either
  {
    levels src;
    src.add(32000, 0, 64U).add(0, 0, 100U);
    sid::meter vu(0U, src, sid::meter_linear);
    CHECK_EQ(run(vu, 64U), 64U);
    CHECK_EQ(vu.left(), 249U);
    CHECK_EQ(run(vu, 1U), 1U);
    CHECK_EQ(vu.left(), 0U);
  }

  // reset, and the end of the source: the level is 0
  {
    levels src(20000, 20000, 164U);
    sid::meter vu(rate, src);
    CHECK_EQ(run(vu, 10U), 10U);
    CHECK(vu.level() > 200U);
    vu.reset();
    CHECK_EQ(vu.level(), 0U);
    CHECK_EQ(run(vu, 90U), 90U);
    CHECK(vu.level() > 200U);

    std::int16_t buffer[SID_CHUNK_FRAMES * 2U];
    CHECK_EQ(vu.render(buffer, SID_CHUNK_FRAMES), static_cast<std::size_t>(SID_CHUNK_FRAMES));
    CHECK(vu.level() > 200U);         // its last frames, but the source has not said that it has ended
    CHECK_EQ(vu.render(buffer, SID_CHUNK_FRAMES), 0U);
    CHECK_EQ(vu.left(), 0U);
    CHECK_EQ(vu.right(), 0U);
  }
}
