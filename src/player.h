///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.design)
//             2010 - 2026, PALANDesign, LLC
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
// \brief sid - player for recorded sound in memory (flash or RAM)
//        pcm: a source that plays samples of 8 bit (unsigned) or 16 bit (signed, little endian), mono or stereo;
//        mono goes to both channels. play_wav() plays a WAV file (sid/src/wav.h): it opens the output with the
//        sample rate of the file - nothing is resampled, the device must be able to make that rate.
//        A clip for the flash: tools/wav2h.py turns a WAV file into a header with a byte array.
//
//          #include "clip.h"       // const std::uint8_t clip[] = { ... }
//          sid::output out(device);
//          sid::play_wav(out, clip, sizeof(clip));
//          out.close();
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_PLAYER_H_
#define _SID_PLAYER_H_

#include "source.h"
#include "wav.h"


namespace sid {

// recorded samples: a source that ends after the last frame
class pcm : public source
{
private:
  // the next sample as signed 16 bit
  std::int16_t sample()
  {
    if (wide_) {
      const std::uint16_t value = static_cast<std::uint16_t>(data_[0] | (static_cast<std::uint16_t>(data_[1]) << 8U));
      data_ += 2;
      return static_cast<std::int16_t>(value);
    }
    const std::int32_t value = (static_cast<std::int32_t>(*data_) - 128) * 256;
    data_ += 1;
    return static_cast<std::int16_t>(value);
  }

  const std::uint8_t*   data_;      // the next sample
  std::uint32_t         frames_;    // frames left
  bool                  stereo_;
  bool                  wide_;      // 16 bit

public:
  /**
   * \param data The samples, interleaved (L, R) if stereo; must stay valid while the source plays
   * \param frames Number of frames
   * \param channels 1 = mono, 2 = stereo
   * \param bits 8 (unsigned) or 16 (signed, little endian)
   */
  pcm(const std::uint8_t* data, std::uint32_t frames, std::uint8_t channels, std::uint8_t bits)
    : data_(data)
    , frames_(((data != nullptr) && ((channels == 1U) || (channels == 2U)) && ((bits == 8U) || (bits == 16U))) ? frames : 0U)
    , stereo_(channels == 2U)
    , wide_(bits == 16U)
  { }

  explicit pcm(const wav::info& file)
    : data_(file.data)
    , frames_(file.frames)
    , stereo_(file.channels == 2U)
    , wide_(file.bits == 16U)
  { }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    for (; (n < frames) && (frames_ != 0U); ++n, --frames_) {
      const std::int16_t left = sample();
      stereo[n * 2U]      = left;
      stereo[n * 2U + 1U] = stereo_ ? sample() : left;
    }
    return n;
  }

  // frames left to play
  std::uint32_t remaining() const
  { return frames_; }
};


/**
 * Play a WAV file from memory; returns when it is queued. The output is opened with the sample rate of the file
 * \param out The output (open or not)
 * \param file The WAV file
 * \param size Size of the file [bytes]
 * \return False if the file is not a supported WAV file or the device cannot make its sample rate
 */
inline bool play_wav(output& out, const std::uint8_t* file, std::size_t size)
{
  wav::info info;
  if (!wav::parse(file, size, info) || !out.open(info.rate)) {
    return false;
  }
  pcm clip(info);
  return out.play(clip);
}

} // namespace sid

#endif  // _SID_PLAYER_H_
