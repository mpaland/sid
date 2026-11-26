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
// \brief sid tests: the devices in port/
//        The WAV file device (the file it writes is read back and parsed by sid::wav), the raw PCM stream, and on
//        Windows the sound output of the PC with a short silence (no sound card: start() is false, that is fine).
//
///////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <vector>

#include "sid/src/tone.h"
#include "sid/src/player.h"
#include "sid/src/port/wav_file.h"
#include "sid/src/port/pcm_stream.h"
#if defined(_WIN32)
#include "sid/src/port/win32.h"
#endif

#include "test.h"
#include "memory_device.h"


namespace {

std::vector<std::uint8_t> read_file(const char* path)
{
  std::vector<std::uint8_t> bytes;
#if defined(_MSC_VER)
  std::FILE* f = nullptr;
  if (fopen_s(&f, path, "rb") != 0) {
    f = nullptr;
  }
#else
  std::FILE* f = std::fopen(path, "rb");
#endif
  if (f != nullptr) {
    int c;
    while ((c = std::fgetc(f)) != EOF) {
      bytes.push_back(static_cast<std::uint8_t>(c));
    }
    (void)std::fclose(f);
  }
  return bytes;
}

std::FILE* open_for_writing(const char* path)
{
#if defined(_MSC_VER)
  std::FILE* f = nullptr;
  return (fopen_s(&f, path, "wb") == 0) ? f : nullptr;
#else
  return std::fopen(path, "wb");
#endif
}

// a short piece: two notes, 100 ms at 16 kHz = 1600 frames
bool piece(sid::output& out)
{
  static const sid::note notes[] = { { sid::a4, 50U }, { sid::e5, 50U } };
  return out.open(16000U) && sid::play(out, notes, 2U, sid::wave_triangle, 200U);
}

} // namespace


TEST_CASE(port_wav_file)
{
  // what the file device gets is what a memory device gets
  memory_device memory;
  sid::output reference(memory);
  REQUIRE(piece(reference));
  REQUIRE(memory.frames() == 1600U);

  const char* const path = "sid_test_port.wav";
  {
    sid::port::wav_file file(path);
    CHECK(!file.write(&memory.samples()[0], 1U));       // not started
    sid::output out(file);
    REQUIRE(piece(out));
    CHECK_EQ(file.frames(), 1600U);
    // a WAV file has one sample rate
    CHECK(file.start(16000U));
    CHECK(!file.start(22050U));
    CHECK(!out.open(22050U));
    CHECK(file.close());
    CHECK(!file.close());
    CHECK(!file.start(16000U));                         // closed is closed
  }

  // the file is a WAV file the library itself can read: stereo, 16 bit, the same samples
  const std::vector<std::uint8_t> bytes = read_file(path);
  REQUIRE(bytes.size() == 44U + (1600U * 4U));
  sid::wav::info info;
  REQUIRE(sid::wav::parse(&bytes[0], bytes.size(), info));
  CHECK_EQ(info.rate, 16000U);
  CHECK_EQ(info.channels, 2U);
  CHECK_EQ(info.bits, 16U);
  CHECK_EQ(info.frames, 1600U);
  sid::pcm clip(info);
  std::vector<std::int16_t> back(3200U);
  CHECK_EQ(clip.render(&back[0], 1600U), 1600U);
  CHECK(back == memory.samples());
  CHECK_EQ(std::remove(path), 0);

  // mono: the mean of both channels, half the size; with a volume the samples are scaled
  {
    sid::port::wav_file file(path, 1U);
    CHECK(file.set_volume(128U));
    CHECK(!file.set_volume(256U));
    REQUIRE(file.start(8000U));
    static const std::int16_t frames[] = { 1000, 3000, -2000, -2000, 32767, 32767 };
    CHECK(file.write(frames, 3U));
  }                                                     // the end of its life closes the file
  const std::vector<std::uint8_t> mono = read_file(path);
  REQUIRE(mono.size() == 44U + 6U);
  REQUIRE(sid::wav::parse(&mono[0], mono.size(), info));
  CHECK_EQ(info.rate, 8000U);
  CHECK_EQ(info.channels, 1U);
  CHECK_EQ(info.frames, 3U);
  sid::pcm half(info);
  std::int16_t mixed[6];
  CHECK_EQ(half.render(mixed, 3U), 3U);
  CHECK_EQ(mixed[0], (501 + 1505) / 2);                 // 1000 and 3000 at volume 128 / 255
  CHECK_EQ(mixed[1], mixed[0]);                         // mono goes to both channels
  CHECK_EQ(mixed[2], -1003);
  CHECK_EQ(mixed[4], 16447);
  CHECK_EQ(std::remove(path), 0);

  // a file that cannot be made: no output
  sid::port::wav_file nowhere("no_such_folder_of_sid/x.wav");
  CHECK(!nowhere.start(16000U));
  CHECK(!nowhere.close());
}


TEST_CASE(port_pcm_stream)
{
  memory_device memory;
  sid::output reference(memory);
  REQUIRE(piece(reference));

  const char* const path = "sid_test_port.raw";
  std::FILE* const stream = open_for_writing(path);
  REQUIRE(stream != nullptr);
  {
    sid::port::pcm_stream raw(stream);
    CHECK_EQ(raw.rate(), 0U);
    CHECK(!raw.write(&memory.samples()[0], 1U));        // not started
    CHECK(!raw.start(0U));
    sid::output out(raw);
    REQUIRE(piece(out));
    CHECK_EQ(raw.rate(), 16000U);
    CHECK(raw.start(16000U));
    CHECK(!raw.start(8000U));                           // a raw stream has one sample rate
    out.close();
  }
  CHECK_EQ(std::fclose(stream), 0);

  // signed 16 bit, little endian, left then right, nothing else
  const std::vector<std::uint8_t> bytes = read_file(path);
  REQUIRE(bytes.size() == (memory.samples().size() * 2U));
  bool same = true;
  for (std::size_t i = 0U; same && (i < memory.samples().size()); ++i) {
    const std::uint16_t value = static_cast<std::uint16_t>(bytes[i * 2U] | (static_cast<std::uint16_t>(bytes[i * 2U + 1U]) << 8U));
    same = (static_cast<std::int16_t>(value) == memory.samples()[i]);
  }
  CHECK(same);
  CHECK_EQ(std::remove(path), 0);

  // no stream: no output
  sid::port::pcm_stream none(nullptr);
  CHECK(!none.start(16000U));
  CHECK(!none.write(&memory.samples()[0], 1U));
  none.stop();
}


#if defined(_WIN32)
TEST_CASE(port_win32)
{
  sid::port::win32 pc;
  CHECK(!pc.write(nullptr, 0U));                        // not started
  CHECK(!pc.start(1000U));                              // rates of 8 .. 96 kHz
  CHECK(!pc.start(200000U));
  CHECK(pc.set_volume(100U));
  CHECK(!pc.set_volume(256U));
  pc.stop();                                            // nothing to stop

  sid::output out(pc);
  if (!out.open(22050U)) {
    std::printf("port_win32: this PC has no sound output, the rest is skipped\n");
    return;
  }
  // 0.3 s of silence: more than the eight buffers hold, so write() has to wait for the sound - and it comes back
  CHECK(out.pause(300U));
  CHECK(out.open(48000U));                              // another rate: plays out, opens again
  CHECK(out.pause(50U));
  out.close();
  CHECK_EQ(pc.dry(), 0U);
  CHECK(!out.pause(10U));
}
#endif
