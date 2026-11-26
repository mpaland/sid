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
// \brief sid device: a WAV file
//        Writes what is played into a WAV file (PCM, 16 bit). Works wherever there is a file system, so it is
//        the device of the examples and the tests - and a way to listen to a song without any sound hardware.
//        A WAV file has one sample rate: a start() with another rate than the first is refused.
//        The file is complete when the device is closed (close() or the end of its life).
//
//          sid::port::wav_file file("song.wav");
//          sid::output out(file);
//          out.open(22050U);
//          sid::beep(out, 880U, 200U);
//          out.close();
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_PORT_WAV_FILE_H_
#define _SID_PORT_WAV_FILE_H_

#include <cstdio>

#include "../source.h"


namespace sid {
namespace port {

class wav_file : public device
{
public:
  /**
   * \param path Name of the file; it is created or overwritten by the first start()
   * \param channels 2 = stereo as played, 1 = mono (the mean of left and right; half the size)
   */
  explicit wav_file(const char* path, std::uint32_t channels = 2U)
    : path_(path)
    , channels_((channels == 1U) ? 1U : 2U)
    , file_(nullptr)
    , rate_(0U)
    , frames_(0U)
    , volume_(255U)
  { }

  ~wav_file()
  { (void)close(); }

  virtual bool start(std::uint32_t sample_rate)
  {
    if (file_ != nullptr) {
      return sample_rate == rate_;      // one file, one rate
    }
    if ((sample_rate == 0U) || (rate_ != 0U)) {
      return false;                     // no rate, or the file is closed already
    }
#if defined(_MSC_VER)
    if (fopen_s(&file_, path_, "wb") != 0) {
      file_ = nullptr;
    }
#else
    file_ = std::fopen(path_, "wb");
#endif
    if (file_ == nullptr) {
      return false;
    }
    rate_ = sample_rate;
    return header();
  }

  virtual bool write(const std::int16_t* stereo, std::size_t frames)
  {
    if ((file_ == nullptr) || (frames > (frames_max() - frames_))) {
      return false;     // not open, or the sizes in the header would overflow (4 GB)
    }
    for (std::size_t n = 0U; n < frames; ++n) {
      const std::int32_t left  = scale(stereo[n * 2U]);
      const std::int32_t right = scale(stereo[n * 2U + 1U]);
      if (channels_ == 1U) {
        put16(static_cast<std::uint32_t>((left + right) / 2));
      }
      else {
        put16(static_cast<std::uint32_t>(left));
        put16(static_cast<std::uint32_t>(right));
      }
    }
    frames_ += static_cast<std::uint32_t>(frames);
    return std::ferror(file_) == 0;
  }

  // the file stays open for more sound of the same rate
  virtual void stop()
  { }

  virtual bool set_volume(std::uint32_t volume)
  {
    if (volume > 255U) {
      return false;
    }
    volume_ = volume;
    return true;
  }

  /**
   * Finish the file: the sizes go into its header
   * \return False if there is no open file or it could not be written
   */
  bool close()
  {
    if (file_ == nullptr) {
      return false;
    }
    bool ok = (std::fseek(file_, 0L, SEEK_SET) == 0) && header();
    ok = (std::fclose(file_) == 0) && ok;
    file_ = nullptr;
    return ok;
  }

  // frames written so far
  std::uint32_t frames() const
  { return frames_; }

private:
  wav_file(const wav_file&);                // one file, one device
  wav_file& operator=(const wav_file&);

  // most frames a WAV file can hold: the RIFF size (36 + data bytes) is 32 bit
  std::uint32_t frames_max() const
  { return (0xFFFFFFFFUL - 36UL) / (channels_ * 2U); }

  std::int32_t scale(std::int16_t sample) const
  {
    return (volume_ >= 255U) ? sample : ((static_cast<std::int32_t>(sample) * static_cast<std::int32_t>(volume_)) / 255);
  }

  void put16(std::uint32_t value)
  {
    (void)std::fputc(static_cast<int>(value & 0xFFU), file_);
    (void)std::fputc(static_cast<int>((value >> 8U) & 0xFFU), file_);
  }

  void put32(std::uint32_t value)
  {
    put16(value & 0xFFFFU);
    put16(value >> 16U);
  }

  // the 44 bytes in front of the samples
  bool header()
  {
    const std::uint32_t bytes = frames_ * channels_ * 2U;
    (void)std::fputs("RIFF", file_);
    put32(36U + bytes);
    (void)std::fputs("WAVEfmt ", file_);
    put32(16U);
    put16(1U);                          // PCM
    put16(channels_);
    put32(rate_);
    put32(rate_ * channels_ * 2U);
    put16(channels_ * 2U);
    put16(16U);
    (void)std::fputs("data", file_);
    put32(bytes);
    return std::ferror(file_) == 0;
  }

  const char*     path_;
  std::uint32_t   channels_;
  std::FILE*      file_;
  std::uint32_t   rate_;        // 0 until the first start()
  std::uint32_t   frames_;
  std::uint32_t   volume_;
};

} // namespace port
} // namespace sid

#endif  // _SID_PORT_WAV_FILE_H_
