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
// \brief sid device: raw PCM on a stream
//        Writes what is played as raw samples (signed 16 bit, little endian, stereo) to a C stream, e.g. stdout.
//        Piped into a player this is live sound on Linux and macOS without any library:
//          ./song --stdout | aplay -f S16_LE -c 2 -r 22050
//          ./song --stdout | ffplay -f s16le -ch_layout stereo -ar 22050 -nodisp -autoexit -
//          ./song --stdout | play -t raw -e signed -b 16 -c 2 -r 22050 -          (sox)
//        A raw stream carries no sample rate, the player must be told. So a start() with another rate than the
//        first is refused. On Windows the stream is switched to binary mode.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_PORT_PCM_STREAM_H_
#define _SID_PORT_PCM_STREAM_H_

#include <cstdio>
#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif

#include "../source.h"


namespace sid {
namespace port {

class pcm_stream : public device
{
public:
  /**
   * \param stream The stream to write to, open for writing (stdout, a pipe, a file)
   */
  explicit pcm_stream(std::FILE* stream)
    : stream_(stream)
    , rate_(0U)
    , volume_(255U)
  {
#if defined(_WIN32)
    if (stream_ != nullptr) {
      (void)_setmode(_fileno(stream_), _O_BINARY);    // no line end translation in the samples
    }
#endif
  }

  virtual bool start(std::uint32_t sample_rate)
  {
    if ((stream_ == nullptr) || (sample_rate == 0U) || ((rate_ != 0U) && (rate_ != sample_rate))) {
      return false;
    }
    rate_ = sample_rate;
    return true;
  }

  virtual bool write(const std::int16_t* stereo, std::size_t frames)
  {
    if ((stream_ == nullptr) || (rate_ == 0U)) {
      return false;
    }
    for (std::size_t n = 0U; n < (frames * 2U); ++n) {
      const std::int32_t sample = (volume_ >= 255U) ? stereo[n]
                                : ((static_cast<std::int32_t>(stereo[n]) * static_cast<std::int32_t>(volume_)) / 255);
      const std::uint32_t value = static_cast<std::uint32_t>(sample);
      (void)std::fputc(static_cast<int>(value & 0xFFU), stream_);
      (void)std::fputc(static_cast<int>((value >> 8U) & 0xFFU), stream_);
    }
    return std::ferror(stream_) == 0;
  }

  virtual void stop()
  {
    if (stream_ != nullptr) {
      (void)std::fflush(stream_);
    }
  }

  virtual bool set_volume(std::uint32_t volume)
  {
    if (volume > 255U) {
      return false;
    }
    volume_ = volume;
    return true;
  }

  // sample rate of the stream [Hz], 0 until the first start()
  std::uint32_t rate() const
  { return rate_; }

private:
  std::FILE*      stream_;
  std::uint32_t   rate_;
  std::uint32_t   volume_;
};

} // namespace port
} // namespace sid

#endif  // _SID_PORT_PCM_STREAM_H_
