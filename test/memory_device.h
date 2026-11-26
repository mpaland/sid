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
// \brief sid tests: a device that keeps what is played in memory
//        The test double of the sound output: it takes sample rates of 8 .. 96 kHz, stores every frame and
//        counts the calls; a test can let start() or write() fail.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_TEST_MEMORY_DEVICE_H_
#define _SID_TEST_MEMORY_DEVICE_H_

#include <vector>

#include "sid/src/source.h"


class memory_device : public sid::device
{
public:
  memory_device()
    : rate_(0U)
    , volume_(255U)
    , starts_(0U)
    , stops_(0U)
    , writes_(0U)
    , running_(false)
    , fail_start_(false)
    , fail_write_(false)
  { }

  virtual bool start(std::uint32_t sample_rate)
  {
    ++starts_;
    if (fail_start_ || (sample_rate < 8000U) || (sample_rate > 96000U)) {
      return false;
    }
    rate_    = sample_rate;
    running_ = true;
    return true;
  }

  virtual bool write(const std::int16_t* stereo, std::size_t frames)
  {
    ++writes_;
    if (!running_ || fail_write_) {
      return false;
    }
    samples_.insert(samples_.end(), stereo, stereo + (frames * 2U));
    return true;
  }

  virtual void stop()
  {
    ++stops_;
    running_ = false;
  }

  virtual bool set_volume(std::uint32_t volume)
  {
    if (volume > 255U) {
      return false;
    }
    volume_ = volume;
    return true;
  }

  // what a test sets
  void fail_start(bool fail)        { fail_start_ = fail; }
  void fail_write(bool fail)        { fail_write_ = fail; }

  // what a test looks at
  const std::vector<std::int16_t>& samples() const    { return samples_; }      // L, R, L, R ...
  std::size_t   frames() const      { return samples_.size() / 2U; }
  std::uint32_t rate() const        { return rate_; }
  std::uint32_t volume() const      { return volume_; }
  bool          running() const     { return running_; }
  std::uint32_t start_count() const { return starts_; }
  std::uint32_t stop_count() const  { return stops_; }
  std::uint32_t write_count() const { return writes_; }

private:
  std::vector<std::int16_t>   samples_;
  std::uint32_t               rate_;
  std::uint32_t               volume_;
  std::uint32_t               starts_;
  std::uint32_t               stops_;
  std::uint32_t               writes_;
  bool                        running_;
  bool                        fail_start_;
  bool                        fail_write_;
};

#endif  // _SID_TEST_MEMORY_DEVICE_H_
