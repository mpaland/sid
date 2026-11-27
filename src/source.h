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
// \brief sid - Sound Interface Driver library: the sound source, the device and the output
//        A source produces sound when asked (pull): render() fills stereo frames (std::int16_t L, R) and returns
//        fewer frames than asked for when it has ended. Tones, melodies, PCM clips and the synthesizer are sources.
//        A device is the sound output of the platform: a DAC, a codec, a sound card, a file. It is the only thing
//        to port, three functions: start(sample rate), write(frames), stop(). Ready devices are in port/.
//        The output plays sources on a device: open(sample rate), play(source) returns when the source has ended
//        and all its frames are written (play(source, ms) after that time at the latest), close() lets the sound
//        play out and switches the device off. Sources are made for the sample rate of the output
//        (output::rate()); nothing is resampled.
//        One thread or task per output. No heap; play() needs SID_CHUNK_FRAMES * 4 bytes of stack.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_SOURCE_H_
#define _SID_SOURCE_H_

#include <cstdint>
#include <cstddef>

// frames rendered and written in one step (the stack buffer of output::play)
#ifndef SID_CHUNK_FRAMES
#define SID_CHUNK_FRAMES        64U
#endif


namespace sid {

// a sound source
class source
{
public:
  /**
   * Produce the next frames
   * \param stereo Buffer for frames * 2 samples: L, R, L, R ...
   * \param frames Frames to produce
   * \return Frames written; fewer than 'frames' (also 0): the source has ended
   */
  virtual std::size_t render(std::int16_t* stereo, std::size_t frames) = 0;

protected:
  // sources are never deleted through a source pointer
  ~source()
  { }
};


// the sound output of the platform: implement it for your hardware, or take one of port/
class device
{
public:
  /**
   * Switch the output on
   * \param sample_rate Frames per second [Hz]
   * \return False if there is no output or it cannot make the rate
   */
  virtual bool start(std::uint32_t sample_rate) = 0;

  /**
   * Take frames to play. Returns when all of them are taken; waits while the output is full
   * \param stereo frames * 2 samples: L, R, L, R ... (signed 16 bit); a mono output plays the mean of L and R
   * \param frames Number of frames
   * \return False if the output did not take them (it does not run)
   */
  virtual bool write(const std::int16_t* stereo, std::size_t frames) = 0;

  /**
   * Let the frames written so far play out, then switch the output off
   */
  virtual void stop() = 0;

  /**
   * Set the volume (optional)
   * \param volume 0 = mute .. 255 = full scale
   * \return False if the device has no volume
   */
  virtual bool set_volume(std::uint32_t volume)
  {
    (void)volume;
    return false;
  }

protected:
  // devices are never deleted through a device pointer
  ~device()
  { }
};


// plays sources on a device
class output
{
public:
  explicit output(device& dev)
    : dev_(dev)
    , rate_(0U)
    , frames_(0U)
  { }

  /**
   * Switch the output on with a sample rate. An output that is open with another rate plays out first
   * \param sample_rate Frames per second [Hz]
   * \return False if the device cannot make the rate or has no output
   */
  bool open(std::uint32_t sample_rate)
  {
    if ((rate_ != 0U) && (rate_ == sample_rate)) {
      return true;
    }
    close();
    if ((sample_rate == 0U) || !dev_.start(sample_rate)) {
      return false;
    }
    rate_   = sample_rate;
    frames_ = 0U;
    return true;
  }

  /**
   * Let the sound play out, then switch the output off
   */
  void close()
  {
    if (rate_ != 0U) {
      dev_.stop();
      rate_ = 0U;
    }
  }

  // sample rate of the open output [Hz], 0 = not open
  std::uint32_t rate() const
  { return rate_; }

  // frames written to the device since open(): the position of the sound, frames() / rate() seconds of it are
  // written. What is heard is behind that by what the device has not played yet
  std::uint64_t frames() const
  { return frames_; }

  // volume of the device: 0 = mute .. 255 = full scale; false if the device has none
  bool set_volume(std::uint32_t volume)
  { return dev_.set_volume(volume); }

  // the device
  device& dev()
  { return dev_; }

  /**
   * Play a source to its end: returns when all its frames are written (the sound ends a little later)
   * \return False if the output is not open or the device did not take the frames
   */
  bool play(source& src)
  {
    if (rate_ == 0U) {
      return false;
    }
    std::int16_t chunk[SID_CHUNK_FRAMES * 2U];
    for (;;) {
      const std::size_t frames = src.render(chunk, SID_CHUNK_FRAMES);
      if ((frames != 0U) && !dev_.write(chunk, frames)) {
        return false;
      }
      frames_ += frames;
      if (frames < SID_CHUNK_FRAMES) {
        return true;
      }
    }
  }

  /**
   * Play a source for a time: for sources that never end by themselves (a synthesizer played by hand, a song
   * that loops). Returns when the frames of that time are written, or when the source has ended before
   * \param src The source
   * \param ms Duration [ms]
   * \return False if the output is not open or the device did not take the frames
   */
  bool play(source& src, std::uint32_t ms)
  {
    if (rate_ == 0U) {
      return false;
    }
    std::int16_t chunk[SID_CHUNK_FRAMES * 2U];
    std::uint32_t left = static_cast<std::uint32_t>((static_cast<std::uint64_t>(rate_) * ms) / 1000U);
    while (left != 0U) {
      const std::size_t want   = (left < SID_CHUNK_FRAMES) ? left : SID_CHUNK_FRAMES;
      const std::size_t frames = src.render(chunk, want);
      if ((frames != 0U) && !dev_.write(chunk, frames)) {
        return false;
      }
      frames_ += frames;
      if (frames < want) {
        break;      // the source has ended
      }
      left -= static_cast<std::uint32_t>(frames);
    }
    return true;
  }

  /**
   * Play silence
   * \param ms Duration [ms]
   */
  bool pause(std::uint32_t ms)
  {
    if (rate_ == 0U) {
      return false;
    }
    std::int16_t chunk[SID_CHUNK_FRAMES * 2U] = { };
    std::uint32_t frames = static_cast<std::uint32_t>((static_cast<std::uint64_t>(rate_) * ms) / 1000U);
    while (frames != 0U) {
      const std::uint32_t n = (frames < SID_CHUNK_FRAMES) ? frames : SID_CHUNK_FRAMES;
      if (!dev_.write(chunk, n)) {
        return false;
      }
      frames_ += n;
      frames  -= n;
    }
    return true;
  }

private:
  device&         dev_;
  std::uint32_t   rate_;    // 0 = not open
  std::uint64_t   frames_;  // written since open()
};

} // namespace sid

#endif  // _SID_SOURCE_H_
