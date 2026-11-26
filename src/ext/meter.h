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
// \brief sid extension: a level meter, for a VU meter or a bar of LEDs
//        One sample says nothing about the loudness: it swings around zero. The meter is a source around
//        another source. It passes the frames on as they are and keeps the peak of each channel, which rises
//        at once and falls slowly, the way the needle or the LEDs of a meter do.
//          meter(rate, source)     a meter around the source
//          left(), right()         the level of a channel: 0 .. 255 (full scale)
//          level()                 the higher of both, for a single bar
//          set_scale(scale)        meter_log (decibels) or meter_linear (amplitude), also while it plays
//          set_range(dB)           what the logarithmic scale shows: 48 dB below full scale by default
//          set_fall(ms)            the time in which the level falls by 20 dB: 300 ms by default
//        The two scales:
//          meter_linear            the level is the amplitude: half the amplitude reads 128. Quiet sound is
//                                  hardly seen, a bar moves at its top only
//          meter_log               the level is in decibels, as the ear hears: 255 = full scale, 0 = the range
//                                  below it or less. Half the amplitude (-6 dB) reads 223 at a range of 48 dB
//        Both read 0 .. 255, so what shows the level is the same for both: (level * leds) / 255 are the LEDs
//        that are on, the last one at full scale.
//        The level is read when it is shown, at any pace: reading does not change it, and no peak between two
//        readings is lost. When the source has ended the level is 0.
//        The meter sees the frames when they are written: the level is ahead of the sound by what the device
//        buffers. It is the level of the source, before the volume of the device (output::set_volume).
//        The meter does not own its source: the source must stay valid while the meter plays.
//        Integer only, no heap, 32 bytes on a 32 bit MCU. One task: the functions are called from the task
//        that plays; a level may be read from another one where reading 32 bit is atomic.
//
//          sid::sequencer music(out.rate(), song, 0U);       // loops for ever
//          sid::meter     vu(out.rate(), music);
//          for (;;) {
//            out.play(vu, 20U);
//            show_bar((vu.level() * 10U) / 255U);            // 10 LEDs, the last one is on at full scale
//          }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXT_METER_H_
#define _SID_EXT_METER_H_

#include "../source.h"


namespace sid {

// scale of a meter
typedef enum tag_meter_type {
  meter_linear = 0,   // the level is the amplitude
  meter_log           // the level is in decibels
} meter_type;


// measures the level of the source it plays
class meter : public source
{
private:
  source&         src_;
  std::uint32_t   rate_;        // [Hz]
  std::uint32_t   left_;        // peak that falls: 2^31 = full scale (a sample of 32768 << 16)
  std::uint32_t   right_;
  std::uint32_t   keep_;        // part of the level that is left after one frame: 2^32 = all of it
  std::uint32_t   range_;       // of the logarithmic scale [dB]
  meter_type      scale_;

public:
  /**
   * \param sample_rate Sample rate of the output [Hz], for the time of the fall
   * \param src The source to play and to measure; it must stay valid while the meter plays
   * \param scale meter_log: the level is in decibels, meter_linear: the level is the amplitude
   */
  meter(std::uint32_t sample_rate, source& src, meter_type scale = meter_log)
    : src_(src)
    , rate_(sample_rate)
    , left_(0U)
    , right_(0U)
    , keep_(0U)
    , range_(48U)
    , scale_(scale)
  {
    set_fall(300U);
  }


  /**
   * Switch the scale, also while the meter plays: the next reading is in the new scale
   * \param scale meter_log or meter_linear
   */
  void set_scale(meter_type scale)
  { scale_ = scale; }


  // the scale
  meter_type scale() const
  { return scale_; }


  /**
   * Set what the logarithmic scale shows
   * \param db Range [dB]: a level this far below full scale, or further, reads 0. 1 .. 96, 48 by default
   */
  void set_range(std::uint32_t db)
  { range_ = (db < 1U) ? 1U : ((db > 96U) ? 96U : db); }


  /**
   * Set how fast the level falls when the sound gets quieter. It always rises at once
   * \param ms Time in which the level falls by 20 dB, to a tenth of the amplitude [ms]; 300 by default.
   *           0: no fall, the level is the peak of the last frames (up to SID_CHUNK_FRAMES)
   */
  void set_fall(std::uint32_t ms)
  {
    // a fall to a tenth in n frames: per frame the part ln(10) / n goes, here as 2^32 * ln(10) / n
    const std::uint64_t frames = (static_cast<std::uint64_t>(rate_) * ms) / 1000U;
    const std::uint64_t part   = (frames != 0U) ? (9889527671ULL / frames) : 0xFFFFFFFFFFFFFFFFULL;
    keep_ = (part > 0xFFFFFFFFULL) ? 0U : ((part == 0U) ? 0xFFFFFFFFU : static_cast<std::uint32_t>(0U - part));
  }


  // level of the left channel in the scale of the meter: 0 .. 255 (full scale)
  std::uint32_t left() const
  { return scaled(left_); }


  // level of the right channel: 0 .. 255 (full scale)
  std::uint32_t right() const
  { return scaled(right_); }


  // the higher level of both channels: 0 .. 255 (full scale)
  std::uint32_t level() const
  { return scaled((left_ > right_) ? left_ : right_); }


  // set the level to 0
  void reset()
  {
    left_  = 0U;
    right_ = 0U;
  }


  /**
   * Produce the next frames: the frames of the source, as they are
   * \return Frames written; fewer than 'frames': the source has ended
   */
  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    const std::size_t got = src_.render(stereo, frames);
    std::size_t done = 0U;
    while (done < got) {
      const std::size_t part = ((got - done) < SID_CHUNK_FRAMES) ? (got - done) : SID_CHUNK_FRAMES;
      std::uint32_t l = 0U;
      std::uint32_t r = 0U;
      for (std::size_t i = done; i < (done + part); ++i) {
        const std::int32_t  sl = stereo[i * 2U];
        const std::int32_t  sr = stereo[(i * 2U) + 1U];
        const std::uint32_t al = static_cast<std::uint32_t>((sl < 0) ? -sl : sl);
        const std::uint32_t ar = static_cast<std::uint32_t>((sr < 0) ? -sr : sr);
        l = (al > l) ? al : l;
        r = (ar > r) ? ar : r;
      }

      // the level falls by the time of these frames; a peak above it lifts it at once
      const std::uint64_t keep = keep_of(part);
      left_  = static_cast<std::uint32_t>((left_  * keep) >> 32U);
      right_ = static_cast<std::uint32_t>((right_ * keep) >> 32U);
      left_  = ((l << 16U) > left_)  ? (l << 16U) : left_;
      right_ = ((r << 16U) > right_) ? (r << 16U) : right_;
      done += part;
    }
    if (got < frames) {
      reset();      // the source has ended: nothing lets the level fall any more
    }
    return got;
  }


private:
  // part of the level that is left after a number of frames: 2^32 = all of it
  std::uint64_t keep_of(std::size_t frames) const
  {
    std::uint64_t result = 1ULL << 32U;
    std::uint64_t factor = keep_;
    for (std::size_t n = frames; n != 0U; n >>= 1U) {
      if ((n & 1U) != 0U) {
        result = (result * factor) >> 32U;
      }
      factor = (factor * factor) >> 32U;
    }
    return result;
  }


  // a peak (2^31 = full scale) as a level of 0 .. 255 in the scale of the meter
  std::uint32_t scaled(std::uint32_t peak) const
  {
    const std::uint32_t sample = peak >> 16U;     // 0 .. 32768
    if (scale_ == meter_linear) {
      return ((sample * 255U) + 16384U) >> 15U;
    }
    if (sample == 0U) {
      return 0U;
    }

    // log2 of the sample [1 / 4096 octave]: the place of its highest bit, and the bits below it by a table
    // of log2(1 + i / 16) with the values between two entries on a line
    static const std::uint16_t log2_table[17] = {
      0U, 358U, 696U, 1016U, 1319U, 1607U, 1882U, 2145U, 2396U, 2637U, 2869U, 3092U, 3307U, 3514U, 3715U, 3908U, 4096U
    };
    std::uint32_t bit = 15U;
    while ((sample >> bit) == 0U) {
      --bit;
    }
    const std::uint32_t below  = (sample << (15U - bit)) - 32768U;       // 0 .. 32767: the bits below the highest
    const std::uint32_t index  = below >> 11U;
    const std::uint32_t rise   = static_cast<std::uint32_t>(log2_table[index + 1U] - log2_table[index]);
    const std::uint32_t octave = (bit * 4096U) + log2_table[index] + ((rise * (below & 2047U)) >> 11U);

    // an octave is 6.0206 dB: how far the sample is below full scale [1 / 1000 dB], and that on the range
    const std::uint32_t down = (((15U * 4096U) - octave) * 6021U) / 4096U;
    const std::uint32_t full = range_ * 1000U;
    return (down >= full) ? 0U : (255U - (((down * 255U) + (full / 2U)) / full));
  }
};

} // namespace sid

#endif  // _SID_EXT_METER_H_
