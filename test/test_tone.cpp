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
// \brief sid tests: tones, melodies, the WAV reader, the PCM source and the output
//        Tones against the mathematical waveforms (frequency by zero crossings, amplitude, duration, fades),
//        melodies, the WAV reader on files built here (and on broken ones), the PCM source, the output on a
//        device that keeps the samples.
//
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <vector>

#include "sid/src/tone.h"
#include "sid/src/player.h"

#include "test.h"
#include "memory_device.h"


namespace {

// all frames of a source, rendered in chunks of an odd size (chunk borders must not show)
std::vector<std::int16_t> render(sid::source& src, std::size_t chunk = 37U)
{
  std::vector<std::int16_t> all;
  std::vector<std::int16_t> part(chunk * 2U);
  for (;;) {
    const std::size_t n = src.render(&part[0], chunk);
    all.insert(all.end(), part.begin(), part.begin() + static_cast<std::ptrdiff_t>(n * 2U));
    if (n < chunk) {
      return all;
    }
  }
}

// rising zero crossings of the left channel
std::size_t crossings(const std::vector<std::int16_t>& s)
{
  std::size_t count = 0U;
  for (std::size_t i = 2U; i < s.size(); i += 2U) {
    if ((s[i - 2U] < 0) && (s[i] >= 0)) {
      ++count;
    }
  }
  return count;
}

int peak(const std::vector<std::int16_t>& s)
{
  int p = 0;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    const int a = (s[i] < 0) ? -s[i] : s[i];
    p = (a > p) ? a : p;
  }
  return p;
}

bool both_channels_equal(const std::vector<std::int16_t>& s)
{
  for (std::size_t i = 0U; i + 1U < s.size(); i += 2U) {
    if (s[i] != s[i + 1U]) {
      return false;
    }
  }
  return true;
}

void put16(std::vector<std::uint8_t>& f, std::uint32_t v)
{
  f.push_back(static_cast<std::uint8_t>(v));
  f.push_back(static_cast<std::uint8_t>(v >> 8U));
}

void put32(std::vector<std::uint8_t>& f, std::uint32_t v)
{
  put16(f, v & 0xFFFFU);
  put16(f, v >> 16U);
}

void put_tag(std::vector<std::uint8_t>& f, const char* id)
{
  f.insert(f.end(), id, id + 4);
}

// a WAV file: 'fmt ' (16 bytes), optionally a chunk of odd size in between, 'data'
std::vector<std::uint8_t> wav_file(std::uint32_t coding, std::uint32_t channels, std::uint32_t rate, std::uint32_t bits,
                                   const std::vector<std::uint8_t>& samples, bool extra_chunk = false)
{
  std::vector<std::uint8_t> f;
  put_tag(f, "RIFF");
  put32(f, 0U);
  put_tag(f, "WAVE");
  put_tag(f, "fmt ");
  put32(f, 16U);
  put16(f, coding);
  put16(f, channels);
  put32(f, rate);
  put32(f, rate * channels * (bits / 8U));
  put16(f, channels * (bits / 8U));
  put16(f, bits);
  if (extra_chunk) {
    put_tag(f, "LIST");
    put32(f, 5U);
    f.insert(f.end(), 6U, 0x55U);     // 5 bytes and the pad byte
  }
  put_tag(f, "data");
  put32(f, static_cast<std::uint32_t>(samples.size()));
  f.insert(f.end(), samples.begin(), samples.end());
  const std::uint32_t riff = static_cast<std::uint32_t>(f.size() - 8U);
  f[4] = static_cast<std::uint8_t>(riff);
  f[5] = static_cast<std::uint8_t>(riff >> 8U);
  return f;
}

std::vector<std::uint8_t> bytes(std::size_t n)
{
  std::vector<std::uint8_t> b(n);
  for (std::size_t i = 0U; i < n; ++i) {
    b[i] = static_cast<std::uint8_t>(i * 7U + 3U);
  }
  return b;
}

// a source of 'frames' frames with the values 1, 2, 3 ... on the left and their negatives on the right
class count_source : public sid::source
{
public:
  explicit count_source(std::size_t frames)
    : left_(frames)
    , value_(0)
  { }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    for (; (n < frames) && (left_ != 0U); ++n, --left_) {
      ++value_;
      stereo[n * 2U]      = value_;
      stereo[n * 2U + 1U] = static_cast<std::int16_t>(-value_);
    }
    return n;
  }

private:
  std::size_t   left_;
  std::int16_t  value_;
};

} // namespace


TEST_CASE(sid_sine_table)
{
  // the interpolated table against sin(): 256 steps leave an error of about 2.5 of 32767
  int worst = 0;
  for (std::uint32_t i = 0U; i < 8192U; ++i) {
    const std::uint32_t phase = i << 19U;
    const double exact = 32767.0 * std::sin(6.283185307179586 * static_cast<double>(i) / 8192.0);
    const int error = static_cast<int>(std::fabs(exact - static_cast<double>(sid::detail::sine(phase))) + 0.5);
    worst = (error > worst) ? error : worst;
  }
  CHECK(worst <= 4);
  CHECK_EQ(sid::detail::sine(0U), 0);
  CHECK_EQ(sid::detail::sine(0x40000000UL), 32767);
  CHECK_EQ(sid::detail::sine(0x80000000UL), 0);
  CHECK_EQ(sid::detail::sine(0xC0000000UL), -32767);
  CHECK(sid::detail::sine(0xFFFFFFFFUL) < 0);       // the last step runs back to 0, not to the peak
  CHECK(sid::detail::sine(0xFFFFFFFFUL) > -900);
}


TEST_CASE(sid_tone_sine)
{
  sid::tone t(48000U);
  CHECK(t.ended());                                 // no note yet
  t.set(1000U, 1000U, sid::wave_sine, 255U);
  CHECK(!t.ended());
  const std::vector<std::int16_t> s = render(t);
  CHECK(t.ended());
  CHECK_EQ(s.size(), 96000U);                       // 1 s of stereo
  CHECK(both_channels_equal(s));
  const std::size_t n = crossings(s);
  CHECK((n >= 999U) && (n <= 1000U));               // 1000 periods, the first starts at the crossing
  CHECK((peak(s) >= 32700) && (peak(s) <= 32767));
  // the fades: silent at both ends, full level after 4 ms
  CHECK_EQ(s[0], 0);
  CHECK_EQ(s[s.size() - 2U], 0);
  std::vector<std::int16_t> start(s.begin(), s.begin() + 96);       // the first ms
  CHECK(peak(start) < 32767 / 3);
  std::vector<std::int16_t> middle(s.begin() + 384, s.begin() + 480);   // the fifth ms
  CHECK(peak(middle) >= 32700);
  std::vector<std::int16_t> end(s.end() - 96, s.end());
  CHECK(peak(end) < 32767 / 3);

  // half the level, another rate: 440 Hz for 250 ms at 22050 Hz
  sid::tone u(22050U);
  u.set(440U, 250U, sid::wave_sine, 128U);
  const std::vector<std::int16_t> v = render(u, 64U);
  CHECK_EQ(v.size(), 2U * 5512U);                   // 22050 * 0.25, rounded down
  CHECK((crossings(v) >= 109U) && (crossings(v) <= 110U));
  CHECK((peak(v) >= 16380) && (peak(v) <= 16448));  // 32767 * 128 / 255
}


TEST_CASE(sid_tone_shapes)
{
  // square: only the two levels (and the fades), the frequency
  sid::tone t(32000U);
  t.set(500U, 500U, sid::wave_square, 255U);
  std::vector<std::int16_t> s = render(t);
  CHECK_EQ(s.size(), 32000U);
  CHECK((crossings(s) >= 249U) && (crossings(s) <= 250U));
  CHECK_EQ(peak(s), 32767);
  bool two_levels = true;
  for (std::size_t i = 2U * 200U; i < s.size() - 2U * 200U; i += 2U) {    // outside the fades (128 frames)
    two_levels = two_levels && ((s[i] == 32767) || (s[i] == -32767));
  }
  CHECK(two_levels);

  // triangle: the same slope everywhere, peaks at full scale
  t.set(250U, 500U, sid::wave_triangle, 255U);
  s = render(t);
  CHECK((crossings(s) >= 124U) && (crossings(s) <= 126U));    // 125 periods; the fade ends at 0 from below: one more
  CHECK((peak(s) >= 32000) && (peak(s) <= 32768));
  bool slope = true;
  for (std::size_t i = 2U * 200U; i < s.size() - 2U * 200U; i += 2U) {
    const int d = s[i] - s[i - 2U];
    // 65536 * 2 per period of 128 frames = 1024 per frame; less at a turn
    slope = slope && (d <= 1024) && (d >= -1024);
  }
  CHECK(slope);

  // a rest, a level of 0 and a frequency above half the sample rate are silent but last their time
  t.set(sid::rest, 100U);
  s = render(t);
  CHECK_EQ(s.size(), 6400U);
  CHECK_EQ(peak(s), 0);
  t.set(1000U, 100U, sid::wave_square, 0U);
  CHECK_EQ(peak(render(t)), 0);
  t.set(16000U, 100U, sid::wave_square, 255U);
  s = render(t);
  CHECK_EQ(s.size(), 6400U);
  CHECK_EQ(peak(s), 0);
  t.set(15999U, 100U, sid::wave_square, 255U);
  CHECK(peak(render(t)) > 0);

  // a level above 255 is 255; a note of 0 ms has no frames; a very short note is all fade and ends silent
  t.set(1000U, 100U, sid::wave_square, 1000U);
  CHECK_EQ(peak(render(t)), 32767);
  t.set(1000U, 0U);
  CHECK_EQ(render(t).size(), 0U);
  t.set(4000U, 2U, sid::wave_square, 255U);
  s = render(t);
  CHECK_EQ(s.size(), 128U);
  CHECK_EQ(s[0], 0);
  CHECK_EQ(s[s.size() - 2U], 0);
  CHECK(peak(s) > 20000);
}


TEST_CASE(sid_melody)
{
  static const sid::note notes[] = { { sid::a4, 100U }, { sid::rest, 50U }, { sid::a5, 200U } };
  sid::melody m(16000U, notes, 3U, sid::wave_sine, 255U);
  const std::vector<std::int16_t> s = render(m, 61U);
  CHECK_EQ(s.size(), 2U * (1600U + 800U + 3200U));
  const std::vector<std::int16_t> first(s.begin(), s.begin() + 3200);
  const std::vector<std::int16_t> gap(s.begin() + 3200, s.begin() + 4800);
  const std::vector<std::int16_t> second(s.begin() + 4800, s.end());
  CHECK((crossings(first) >= 43U) && (crossings(first) <= 44U));      // 440 Hz * 0.1 s
  CHECK_EQ(peak(gap), 0);
  CHECK((crossings(second) >= 175U) && (crossings(second) <= 176U));  // 880 Hz * 0.2 s
  CHECK(peak(second) >= 32700);

  // no notes: ends at once
  sid::melody none(16000U, notes, 0U);
  CHECK_EQ(render(none).size(), 0U);
}


TEST_CASE(sid_wav_parse)
{
  sid::wav::info info = { };

  // 8 bit mono
  std::vector<std::uint8_t> f = wav_file(1U, 1U, 11025U, 8U, bytes(100U));
  REQUIRE(sid::wav::parse(&f[0], f.size(), info));
  CHECK_EQ(info.rate, 11025U);
  CHECK_EQ(info.channels, 1U);
  CHECK_EQ(info.bits, 8U);
  CHECK_EQ(info.frames, 100U);
  CHECK(info.data == &f[44]);

  // 16 bit stereo behind a chunk of odd size (padded)
  f = wav_file(1U, 2U, 44100U, 16U, bytes(400U), true);
  REQUIRE(sid::wav::parse(&f[0], f.size(), info));
  CHECK_EQ(info.rate, 44100U);
  CHECK_EQ(info.channels, 2U);
  CHECK_EQ(info.bits, 16U);
  CHECK_EQ(info.frames, 100U);
  CHECK(info.data == &f[44 + 14]);

  // the samples cut off: plays as far as the file goes, whole frames
  f = wav_file(1U, 2U, 44100U, 16U, bytes(400U));
  REQUIRE(sid::wav::parse(&f[0], f.size() - 10U, info));
  CHECK_EQ(info.frames, 97U);

  // extensible format with the PCM sub format
  f = wav_file(0xFFFEU, 1U, 8000U, 16U, bytes(20U));
  std::vector<std::uint8_t> ext(f.begin(), f.begin() + 36);
  ext[16] = 40U;                                  // 'fmt ' is 40 bytes
  put16(ext, 22U);                                // size of the extension
  put16(ext, 16U);                                // valid bits
  put32(ext, 4U);                                 // channel mask
  put16(ext, 1U);                                 // sub format: PCM ...
  ext.insert(ext.end(), 14U, 0U);                 // ... the rest of its GUID
  ext.insert(ext.end(), f.begin() + 36, f.end());
  REQUIRE(sid::wav::parse(&ext[0], ext.size(), info));
  CHECK_EQ(info.rate, 8000U);
  CHECK_EQ(info.frames, 10U);
  ext[36 + 8] = 3U;                               // sub format: IEEE float
  CHECK(!sid::wav::parse(&ext[0], ext.size(), info));

  // refused: no file, too short, not RIFF / WAVE, compressed, 24 bit, 3 channels, rate 0, a wrong block size
  f = wav_file(1U, 1U, 8000U, 8U, bytes(16U));
  CHECK(!sid::wav::parse(nullptr, f.size(), info));
  CHECK(!sid::wav::parse(&f[0], 11U, info));
  CHECK(!sid::wav::parse(&f[0], 40U, info));      // ends inside the header of 'data'
  CHECK(sid::wav::parse(&f[0], 44U, info));       // no samples left: a clip of no frames
  CHECK_EQ(info.frames, 0U);
  std::vector<std::uint8_t> bad = f;
  bad[0] = 'X';
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = f;
  bad[8] = 'X';
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = wav_file(2U, 1U, 8000U, 8U, bytes(16U));        // ADPCM
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = wav_file(1U, 1U, 8000U, 24U, bytes(18U));
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = wav_file(1U, 3U, 8000U, 8U, bytes(18U));
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = wav_file(1U, 1U, 0U, 8U, bytes(16U));
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = f;
  bad[32] = 2U;                                         // block size 2 for 8 bit mono
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  bad = f;
  bad[16] = 14U;                                        // 'fmt ' too short
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  // 'data' before 'fmt ', and a file without samples
  bad.assign(f.begin(), f.begin() + 12);
  bad.insert(bad.end(), f.begin() + 36, f.end());
  bad.insert(bad.end(), f.begin() + 12, f.begin() + 36);
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
  CHECK(!sid::wav::parse(&f[0], 36U, info));
  // a chunk that claims to be longer than the file
  bad = wav_file(1U, 1U, 8000U, 8U, bytes(16U), true);
  bad[40] = 200U;
  CHECK(!sid::wav::parse(&bad[0], bad.size(), info));
}


TEST_CASE(sid_pcm)
{
  // 8 bit mono: unsigned, to both channels
  static const std::uint8_t mono8[] = { 0U, 128U, 255U, 129U };
  sid::pcm a(mono8, 4U, 1U, 8U);
  CHECK_EQ(a.remaining(), 4U);
  std::vector<std::int16_t> s = render(a, 3U);
  REQUIRE(s.size() == 8U);
  CHECK_EQ(a.remaining(), 0U);
  CHECK_EQ(s[0], -32768);
  CHECK_EQ(s[1], -32768);
  CHECK_EQ(s[2], 0);
  CHECK_EQ(s[4], 32512);
  CHECK_EQ(s[6], 256);
  CHECK_EQ(s[7], 256);

  // 16 bit stereo, little endian, at an odd address
  static const std::uint8_t stereo16[] = { 0xEEU, 0x34U, 0x12U, 0xFEU, 0xFFU, 0x00U, 0x80U, 0xFFU, 0x7FU };
  sid::pcm b(&stereo16[1], 2U, 2U, 16U);
  s = render(b);
  REQUIRE(s.size() == 4U);
  CHECK_EQ(s[0], 0x1234);
  CHECK_EQ(s[1], -2);
  CHECK_EQ(s[2], -32768);
  CHECK_EQ(s[3], 32767);

  // 8 bit stereo and 16 bit mono
  static const std::uint8_t stereo8[] = { 0U, 255U, 128U, 127U };
  sid::pcm c(stereo8, 2U, 2U, 8U);
  s = render(c);
  REQUIRE(s.size() == 4U);
  CHECK_EQ(s[0], -32768);
  CHECK_EQ(s[1], 32512);
  CHECK_EQ(s[3], -256);
  sid::pcm d(&stereo16[1], 4U, 1U, 16U);
  s = render(d);
  REQUIRE(s.size() == 8U);
  CHECK_EQ(s[0], 0x1234);
  CHECK_EQ(s[1], 0x1234);
  CHECK_EQ(s[6], 32767);

  // formats the source cannot play, and no data: no frames
  sid::pcm e(mono8, 4U, 3U, 8U);
  CHECK_EQ(render(e).size(), 0U);
  sid::pcm g(mono8, 4U, 1U, 24U);
  CHECK_EQ(render(g).size(), 0U);
  sid::pcm h(nullptr, 4U, 1U, 8U);
  CHECK_EQ(render(h).size(), 0U);

  // from a WAV file
  const std::vector<std::uint8_t> f = wav_file(1U, 1U, 8000U, 8U, std::vector<std::uint8_t>(mono8, mono8 + 4));
  sid::wav::info info = { };
  REQUIRE(sid::wav::parse(&f[0], f.size(), info));
  sid::pcm w(info);
  s = render(w);
  REQUIRE(s.size() == 8U);
  CHECK_EQ(s[4], 32512);
}


TEST_CASE(sid_output)
{
  memory_device dev;
  sid::output out(dev);
  CHECK_EQ(out.rate(), 0U);
  CHECK(&out.dev() == &dev);

  // not open: nothing plays
  count_source early(10U);
  CHECK(!out.play(early));
  CHECK(!out.pause(10U));
  CHECK_EQ(dev.write_count(), 0U);
  out.close();
  CHECK_EQ(dev.stop_count(), 0U);

  // a rate the device cannot make, a device without an output, no rate at all
  CHECK(!out.open(1000U));
  CHECK_EQ(out.rate(), 0U);
  dev.fail_start(true);
  CHECK(!out.open(16000U));
  CHECK_EQ(out.rate(), 0U);
  dev.fail_start(false);
  CHECK(!out.open(0U));

  REQUIRE(out.open(16000U));
  CHECK_EQ(out.rate(), 16000U);
  CHECK_EQ(dev.rate(), 16000U);
  CHECK(dev.running());

  // every frame of a source arrives, in order, also when it ends on a chunk border or is empty
  count_source src(1000U);
  REQUIRE(out.play(src));
  REQUIRE(dev.frames() == 1000U);
  CHECK_EQ(dev.samples()[0], 1);
  CHECK_EQ(dev.samples()[1], -1);
  CHECK_EQ(dev.samples()[1998], 1000);
  CHECK_EQ(dev.samples()[1999], -1000);
  count_source border(SID_CHUNK_FRAMES * 2U);
  REQUIRE(out.play(border));
  CHECK_EQ(dev.frames(), 1000U + SID_CHUNK_FRAMES * 2U);
  count_source empty(0U);
  REQUIRE(out.play(empty));
  CHECK_EQ(dev.frames(), 1000U + SID_CHUNK_FRAMES * 2U);

  // a pause is silence of its length
  const std::size_t before = dev.frames();
  REQUIRE(out.pause(25U));
  CHECK_EQ(dev.frames(), before + 400U);
  CHECK_EQ(dev.samples()[dev.samples().size() - 1U], 0);

  // a source played for a time: so many frames of it, the rest later; it may end before the time is over
  count_source timed(1000U);
  const std::size_t start = dev.frames();
  REQUIRE(out.play(timed, 25U));
  CHECK_EQ(dev.frames(), start + 400U);
  CHECK_EQ(dev.samples()[dev.samples().size() - 2U], 400);
  REQUIRE(out.play(timed, 0U));
  CHECK_EQ(dev.frames(), start + 400U);
  REQUIRE(out.play(timed, 1000U));
  CHECK_EQ(dev.frames(), start + 1000U);
  CHECK_EQ(dev.samples()[dev.samples().size() - 2U], 1000);
  REQUIRE(out.play(timed, 1000U));
  CHECK_EQ(dev.frames(), start + 1000U);

  // the volume goes to the device
  CHECK(out.set_volume(77U));
  CHECK_EQ(dev.volume(), 77U);
  CHECK(!out.set_volume(256U));

  // open again with the same rate: the device goes on. With another rate: it plays out and starts anew, and
  // the output adds nothing to the sound
  const std::uint32_t starts = dev.start_count();
  const std::size_t   played = dev.frames();
  REQUIRE(out.open(16000U));
  CHECK_EQ(dev.start_count(), starts);
  CHECK_EQ(dev.stop_count(), 0U);
  REQUIRE(out.open(8000U));
  CHECK_EQ(dev.stop_count(), 1U);
  CHECK_EQ(dev.start_count(), starts + 1U);
  CHECK_EQ(dev.rate(), 8000U);
  CHECK_EQ(out.rate(), 8000U);
  CHECK_EQ(dev.frames(), played);

  // a rate the device refuses while it is open: the output is closed then
  CHECK(!out.open(500U));
  CHECK_EQ(out.rate(), 0U);
  CHECK(!dev.running());
  REQUIRE(out.open(8000U));

  // close: the device plays out and stops; a closed output plays nothing, closing it again does nothing
  const std::uint32_t stops = dev.stop_count();
  out.close();
  CHECK(!dev.running());
  CHECK_EQ(dev.stop_count(), stops + 1U);
  CHECK_EQ(out.rate(), 0U);
  count_source late(10U);
  CHECK(!out.play(late));
  CHECK(!out.play(late, 10U));
  out.close();
  CHECK_EQ(dev.stop_count(), stops + 1U);
}


TEST_CASE(sid_output_write_fails)
{
  // a device that does not take the frames: play and pause fail, and go on when it takes them again
  memory_device dev;
  sid::output out(dev);
  REQUIRE(out.open(48000U));
  dev.fail_write(true);
  count_source src(1000U);
  CHECK(!out.play(src));
  CHECK(!out.play(src, 10U));
  CHECK(!out.pause(10U));
  CHECK_EQ(dev.frames(), 0U);
  dev.fail_write(false);
  count_source again(100U);
  CHECK(out.play(again));
  CHECK_EQ(dev.frames(), 100U);
}


TEST_CASE(sid_beep_and_play_wav)
{
  memory_device dev;
  sid::output out(dev);
  REQUIRE(out.open(24000U));
  REQUIRE(sid::beep(out, 1000U, 100U, sid::wave_square, 255U));
  CHECK_EQ(dev.frames(), 2400U);
  CHECK((crossings(dev.samples()) >= 99U) && (crossings(dev.samples()) <= 100U));

  static const sid::note notes[] = { { sid::c5, 50U }, { sid::g5, 50U } };
  REQUIRE(sid::play(out, notes, 2U));
  CHECK_EQ(dev.frames(), 2400U + 2400U);

  // a WAV file of another rate: the output changes to it
  static const std::uint8_t mono8[] = { 0U, 128U, 255U };
  const std::vector<std::uint8_t> f = wav_file(1U, 1U, 11025U, 8U, std::vector<std::uint8_t>(mono8, mono8 + 3));
  const std::size_t before = dev.frames();
  REQUIRE(sid::play_wav(out, &f[0], f.size()));
  CHECK_EQ(dev.rate(), 11025U);
  CHECK_EQ(out.rate(), 11025U);
  REQUIRE(dev.frames() == before + 3U);
  CHECK_EQ(dev.samples()[dev.samples().size() - 6U], -32768);
  CHECK_EQ(dev.samples()[dev.samples().size() - 1U], 32512);

  // no WAV file, and a rate the device cannot make
  CHECK(!sid::play_wav(out, mono8, sizeof(mono8)));
  const std::vector<std::uint8_t> fast = wav_file(1U, 1U, 192000U, 8U, std::vector<std::uint8_t>(mono8, mono8 + 3));
  CHECK(!sid::play_wav(out, &fast[0], fast.size()));
}
