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
// \brief sid - tones and melodies
//        tone: one note of a sine, square or triangle wave (or a rest) with a short fade in and out against
//        clicks. melody: an array of notes, one after the other. beep() and play() are the short forms.
//        Integer only: a 32 bit phase accumulator, the sine from a table of 256 values with interpolation.
//
//          sid::output out(device);
//          out.open(22050U);
//          sid::beep(out, 880U, 150U);
//          static const sid::note tune[] = { { sid::c5, 200U }, { sid::e5, 200U }, { sid::g5, 400U } };
//          sid::play(out, tune, 3U);
//          out.close();
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_TONE_H_
#define _SID_TONE_H_

#include "source.h"


namespace sid {

// waveform of a tone
typedef enum tag_wave_type {
  wave_sine = 0,
  wave_square,
  wave_triangle
} wave_type;

// one note of a melody
typedef struct tag_note {
  std::uint16_t   freq;     // frequency [Hz], 0 = rest
  std::uint16_t   ms;       // duration [ms]
} note;

// note frequencies [Hz], equal temperament, a4 = 440 Hz ('s' = sharp); 'rest' is silence
const std::uint16_t rest = 0U;
const std::uint16_t c3 = 131U, cs3 = 139U, d3 = 147U, ds3 = 156U, e3 = 165U, f3 = 175U,
                    fs3 = 185U, g3 = 196U, gs3 = 208U, a3 = 220U, as3 = 233U, b3 = 247U;
const std::uint16_t c4 = 262U, cs4 = 277U, d4 = 294U, ds4 = 311U, e4 = 330U, f4 = 349U,
                    fs4 = 370U, g4 = 392U, gs4 = 415U, a4 = 440U, as4 = 466U, b4 = 494U;
const std::uint16_t c5 = 523U, cs5 = 554U, d5 = 587U, ds5 = 622U, e5 = 659U, f5 = 698U,
                    fs5 = 740U, g5 = 784U, gs5 = 831U, a5 = 880U, as5 = 932U, b5 = 988U;
const std::uint16_t c6 = 1047U, cs6 = 1109U, d6 = 1175U, ds6 = 1245U, e6 = 1319U, f6 = 1397U,
                    fs6 = 1480U, g6 = 1568U, gs6 = 1661U, a6 = 1760U, as6 = 1865U, b6 = 1976U;


namespace detail {

// one period of a sine, 256 steps, full scale 32767
inline std::int16_t sine(std::uint32_t phase)
{
  // the first quarter period (0 .. 90 degrees) in 64 steps, the value at 90 degrees included
  static const std::int16_t quarter[65] = {
        0,   804,  1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767
  };
  const std::uint32_t index    = phase >> 24U;                  // 0 .. 255
  const std::int32_t  fraction = (phase >> 8U) & 0xFFFFU;       // between this step and the next
  const std::uint32_t next     = index + 1U;                    // 1 .. 256
  const std::uint32_t q0 = index & 0x3FU;
  const std::uint32_t q1 = next & 0x3FU;
  // the quarter of a step: 0 = rising from 0, 1 = falling from the peak, 2, 3 = the same, negative
  const std::int32_t a0 = ((index & 0x40U) != 0U) ? quarter[64U - q0] : quarter[q0];
  const std::int32_t a1 = ((next  & 0x40U) != 0U) ? quarter[64U - q1] : quarter[q1];
  const std::int32_t s0 = ((index & 0x80U) != 0U) ? -a0 : a0;
  const std::int32_t s1 = ((next  & 0x80U) != 0U) ? -a1 : a1;
  return static_cast<std::int16_t>(s0 + (((s1 - s0) * fraction) / 65536));
}

} // namespace detail


// one note: a source that ends after its duration
class tone : public source
{
private:
  std::uint32_t   rate_;        // [Hz]
  wave_type       wave_;
  std::uint32_t   phase_;       // 2^32 = one period
  std::uint32_t   step_;        // phase per frame
  std::int32_t    level_;       // 0 .. 255
  std::uint32_t   length_;      // [frames]
  std::uint32_t   position_;    // [frames]
  std::uint32_t   fade_;        // [frames]

public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   */
  explicit tone(std::uint32_t sample_rate)
    : rate_(sample_rate)
    , wave_(wave_sine)
    , phase_(0U)
    , step_(0U)
    , level_(0)
    , length_(0U)
    , position_(0U)
    , fade_(0U)
  { }

  
  /**
   * Start a note
   * \param freq Frequency [Hz], 0 = rest (silence); below half the sample rate
   * \param ms Duration [ms]
   * \param wave Waveform
   * \param level Loudness: 0 .. 255 (full scale)
   */
  void set(std::uint32_t freq, std::uint32_t ms, wave_type wave = wave_sine, std::uint32_t level = 128U)
  {
    wave_     = wave;
    phase_    = 0U;
    step_     = (rate_ != 0U) ? static_cast<std::uint32_t>((static_cast<std::uint64_t>(freq) << 32U) / rate_) : 0U;
    level_    = static_cast<std::int32_t>((level > 255U) ? 255U : level);
    length_   = static_cast<std::uint32_t>((static_cast<std::uint64_t>(rate_) * ms) / 1000U);
    position_ = 0U;
    fade_     = rate_ / 250U;             // 4 ms in and out
    if (fade_ > (length_ / 2U)) {
      fade_ = length_ / 2U;               // a very short note is all fade
    }
    if ((freq == 0U) || (freq >= (rate_ / 2U))) {
      level_ = 0;                         // rest; a frequency the sample rate cannot carry is silent
    }
  }

  
  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    for (; (n < frames) && (position_ < length_); ++n, ++position_) {
      std::int32_t sample = 0;
      if (level_ != 0) {
        switch (wave_) {
          case wave_square :
            sample = ((phase_ & 0x80000000UL) != 0U) ? -32767 : 32767;
            break;
          case wave_triangle : {
            // up from -32768 to 32767 in the first half of the period, down in the second
            const std::int32_t ramp = static_cast<std::int32_t>((phase_ >> 15U) & 0xFFFFU);    // 0 .. 65535 per half
            sample = ((phase_ & 0x80000000UL) != 0U) ? (32767 - ramp) : (ramp - 32768);
            break;
          }
          default :
            sample = detail::sine(phase_);
            break;
        }
        sample = (sample * level_) / 255;
        const std::uint32_t left = length_ - position_;     // frames to the end, this one included
        if (position_ < fade_) {
          sample = (sample * static_cast<std::int32_t>(position_)) / static_cast<std::int32_t>(fade_);
        }
        else if (left <= fade_) {
          sample = (sample * static_cast<std::int32_t>(left - 1U)) / static_cast<std::int32_t>(fade_);
        }
        phase_ += step_;
      }
      stereo[n * 2U]      = static_cast<std::int16_t>(sample);
      stereo[n * 2U + 1U] = static_cast<std::int16_t>(sample);
    }
    return n;
  }


  // true when the note has ended
  bool ended() const
  { return position_ >= length_; }
};


// notes one after the other: a source that ends after the last note
class melody : public source
{
private:
  tone            tone_;
  const note*     notes_;
  std::size_t     count_;
  std::size_t     next_;      // index of the note after the one that plays
  wave_type       wave_;
  std::uint32_t   level_;

public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   * \param notes The notes; the array must stay valid while the melody plays
   * \param count Number of notes
   * \param wave Waveform of all notes
   * \param level Loudness: 0 .. 255 (full scale)
   */
  melody(std::uint32_t sample_rate, const note* notes, std::size_t count, wave_type wave = wave_sine, std::uint32_t level = 128U)
    : tone_(sample_rate)
    , notes_(notes)
    , count_(count)
    , next_(0U)
    , wave_(wave)
    , level_(level)
  { }


  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    while (n < frames) {
      n += tone_.render(&stereo[n * 2U], frames - n);
      if (n < frames) {
        // the note has ended: the next one
        if (next_ >= count_) {
          break;
        }
        tone_.set(notes_[next_].freq, notes_[next_].ms, wave_, level_);
        ++next_;
      }
    }
    return n;
  }
};


/**
 * Play one tone on an open output; returns when it is queued
 * \param out The output
 * \param freq Frequency [Hz]
 * \param ms Duration [ms]
 * \param wave Waveform
 * \param level Loudness: 0 .. 255 (full scale)
 */
inline bool beep(output& out, std::uint32_t freq, std::uint32_t ms, wave_type wave = wave_sine, std::uint32_t level = 128U)
{
  tone t(out.rate());
  t.set(freq, ms, wave, level);
  return out.play(t);
}


/**
 * Play a melody on an open output; returns when it is queued
 * \param out The output
 * \param notes The notes
 * \param count Number of notes
 * \param wave Waveform of all notes
 * \param level Loudness: 0 .. 255 (full scale)
 */
inline bool play(output& out, const note* notes, std::size_t count, wave_type wave = wave_sine, std::uint32_t level = 128U)
{
  melody m(out.rate(), notes, count, wave, level);
  return out.play(m);
}

} // namespace sid

#endif  // _SID_TONE_H_
