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
// \brief sid example 04: sources of your own
//        Everything that makes sound is a sid::source: one function, render(), fills stereo frames when asked and
//        returns fewer frames than asked for when the sound has ended. Two sources are written here:
//          siren   a generator: a square wave whose pitch goes up and down
//          echo    an effect: it takes another source and adds a delayed, quieter copy of it
//        The output plays them like the sources of the library.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/tone.h"

#include "common.h"


// a two tone siren: a square wave that slides between two pitches
class siren : public sid::source
{
public:
  siren(std::uint32_t sample_rate, std::uint32_t low_hz, std::uint32_t high_hz, std::uint32_t ms)
    : rate_(sample_rate)
    , low_(low_hz)
    , span_(high_hz - low_hz)
    , left_(static_cast<std::uint32_t>((static_cast<std::uint64_t>(sample_rate) * ms) / 1000U))
    , phase_(0U)
    , sweep_(0U)
  { }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    for (; (n < frames) && (left_ != 0U); ++n, --left_) {
      // the sweep is a triangle of 1.5 Hz: 0 .. 65535 .. 0
      sweep_ += static_cast<std::uint32_t>((static_cast<std::uint64_t>(3U) << 31U) / rate_);
      const std::uint32_t ramp = (sweep_ >> 15U) & 0xFFFFU;
      const std::uint32_t up   = ((sweep_ & 0x80000000UL) != 0U) ? (0xFFFFU - ramp) : ramp;
      const std::uint32_t hz   = low_ + ((span_ * up) >> 16U);
      // 2^32 is one period of the square
      phase_ += static_cast<std::uint32_t>((static_cast<std::uint64_t>(hz) << 32U) / rate_);
      const std::int16_t sample = ((phase_ & 0x80000000UL) != 0U) ? -6000 : 6000;
      stereo[n * 2U]      = sample;
      stereo[n * 2U + 1U] = sample;
    }
    return n;
  }

private:
  std::uint32_t   rate_;
  std::uint32_t   low_;
  std::uint32_t   span_;
  std::uint32_t   left_;      // frames to go
  std::uint32_t   phase_;
  std::uint32_t   sweep_;
};


// an echo: the sound of another source plus a copy of it some time later; it ends when the copy has faded
class echo : public sid::source
{
public:
  echo(sid::source& input, std::uint32_t sample_rate, std::uint32_t delay_ms)
    : input_(input)
    , delay_((sample_rate * delay_ms) / 1000U)
    , at_(0U)
    , tail_(0U)
    , ended_(false)
  {
    if (delay_ < 1U) {
      delay_ = 1U;
    }
    if (delay_ > size) {
      delay_ = size;
    }
    for (std::uint32_t i = 0U; i < size; ++i) {
      line_[i] = 0;
    }
  }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = ended_ ? 0U : input_.render(stereo, frames);
    if (n < frames) {
      // the input has ended: silence goes in until the echoes are gone (three rounds of the delay)
      ended_ = true;
      for (; (n < frames) && (tail_ < (delay_ * 3U)); ++n, ++tail_) {
        stereo[n * 2U]      = 0;
        stereo[n * 2U + 1U] = 0;
      }
    }
    for (std::size_t i = 0U; i < n; ++i) {
      // the left channel is taken for both
      const std::int32_t mixed = stereo[i * 2U] + (line_[at_] / 2);
      line_[at_] = static_cast<std::int16_t>((mixed > 32767) ? 32767 : ((mixed < -32767) ? -32767 : mixed));
      at_ = (at_ + 1U) % delay_;
      stereo[i * 2U]      = line_[(at_ + delay_ - 1U) % delay_];
      stereo[i * 2U + 1U] = stereo[i * 2U];
    }
    return n;
  }

private:
  static const std::uint32_t size = 8192U;      // longest delay [frames]

  sid::source&    input_;
  std::uint32_t   delay_;         // [frames]
  std::uint32_t   at_;            // place in the delay line
  std::uint32_t   tail_;          // frames of silence fed in after the input
  bool            ended_;         // the input has ended
  std::int16_t    line_[size];    // the delay line
};


static bool play(sid::output& out)
{
  bool ok = out.open(example::rate);

  // the siren for 4 s
  siren alarm(out.rate(), 600U, 1200U, 4000U);
  ok = ok && out.play(alarm);
  ok = ok && out.pause(300U);

  // a melody of the library through the echo
  static const sid::note notes[] = {
    { sid::e5, 150U }, { sid::rest, 250U }, { sid::g5, 150U }, { sid::rest, 250U },
    { sid::b5, 150U }, { sid::rest, 250U }, { sid::e6, 300U }
  };
  sid::melody tune(out.rate(), notes, sizeof(notes) / sizeof(notes[0]), sid::wave_triangle, 150U);
  echo hall(tune, out.rate(), 180U);
  ok = ok && out.play(hall);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "04_custom_source", &play);
}
