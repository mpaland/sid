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
// \brief sid device: the sound output of a Windows PC (waveOut)
//        Plays on the default sound output. Eight buffers of 20 ms are kept queued at Windows - with fewer,
//        Windows hands the buffers back in batches and the output runs dry in between. So the sound follows a
//        write after about 160 ms, and write() waits while all buffers are queued: the caller is paced by the
//        sound. No thread of its own. stop() plays out what was written.
//        Sample rates of 8 .. 96 kHz. The volume is applied to the samples, the mixer of the PC is not touched.
//        Link winmm (done here for MSVC; MinGW: -lwinmm, the CMake target of sid does it).
//
//          sid::port::win32 pc;
//          sid::output out(pc);
//          if (out.open(22050U)) {         // false on a PC without a sound output
//            sid::beep(out, 880U, 200U);
//            out.close();
//          }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_PORT_WIN32_H_
#define _SID_PORT_WIN32_H_

#if !defined(_WIN32)
#error "sid/src/port/win32.h is the device of Windows"
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <cstring>

#include "../source.h"

#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif


namespace sid {
namespace port {

class win32 : public device
{
public:
  win32()
    : out_(NULL)
    , event_(::CreateEventA(NULL, FALSE, FALSE, NULL))
    , frames_(0U)
    , fill_(0U)
    , index_(0U)
    , volume_(255U)
    , dry_(0U)
    , playing_(false)
  { }

  ~win32()
  {
    stop();
    if (event_ != NULL) {
      (void)::CloseHandle(event_);
    }
  }

  virtual bool start(std::uint32_t sample_rate)
  {
    stop();
    if ((sample_rate < rate_min) || (sample_rate > rate_max) || (event_ == NULL)) {
      return false;
    }
    WAVEFORMATEX format;
    std::memset(&format, 0, sizeof(format));
    format.wFormatTag      = WAVE_FORMAT_PCM;
    format.nChannels       = 2U;
    format.nSamplesPerSec  = sample_rate;
    format.wBitsPerSample  = 16U;
    format.nBlockAlign     = 4U;
    format.nAvgBytesPerSec = sample_rate * 4U;
    if (::waveOutOpen(&out_, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(event_), 0U, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
      out_ = NULL;
      return false;       // no sound output
    }
    frames_ = (sample_rate * buffer_ms) / 1000U;
    for (std::uint32_t i = 0U; i < buffer_count; ++i) {
      std::memset(&header_[i], 0, sizeof(WAVEHDR));
      header_[i].lpData         = reinterpret_cast<LPSTR>(data_[i]);
      header_[i].dwBufferLength = frames_ * 4U;
      (void)::waveOutPrepareHeader(out_, &header_[i], sizeof(WAVEHDR));
    }
    fill_    = 0U;
    index_   = 0U;
    playing_ = false;
    return true;
  }

  virtual bool write(const std::int16_t* stereo, std::size_t frames)
  {
    if (out_ == NULL) {
      return false;
    }
    while (frames != 0U) {
      if ((fill_ == 0U) && !wait_free()) {
        return false;       // Windows still holds the buffer (a write after a timeout)
      }
      // into the buffer that is being filled; it goes to Windows when it is full
      const std::uint32_t room = frames_ - fill_;
      const std::uint32_t n    = (frames < room) ? static_cast<std::uint32_t>(frames) : room;
      std::int16_t* const to   = &data_[index_][fill_ * 2U];
      for (std::uint32_t s = 0U; s < (n * 2U); ++s) {
        to[s] = (volume_ >= 255U) ? stereo[s]
              : static_cast<std::int16_t>((static_cast<std::int32_t>(stereo[s]) * static_cast<std::int32_t>(volume_)) / 255);
      }
      stereo += n * 2U;
      frames -= n;
      fill_  += n;
      if ((fill_ == frames_) && !submit()) {
        return false;
      }
    }
    return true;
  }

  virtual void stop()
  {
    if (out_ == NULL) {
      return;
    }
    if (fill_ != 0U) {
      (void)submit();     // the rest
    }
    // the queued buffers play out
    for (std::uint32_t waited = 0U; (queued() != 0U) && (waited < timeout_ms); waited += poll_ms) {
      (void)::WaitForSingleObject(event_, poll_ms);
    }
    (void)::waveOutReset(out_);
    for (std::uint32_t i = 0U; i < buffer_count; ++i) {
      (void)::waveOutUnprepareHeader(out_, &header_[i], sizeof(WAVEHDR));
    }
    (void)::waveOutClose(out_);
    out_     = NULL;
    fill_    = 0U;
    playing_ = false;
  }

  virtual bool set_volume(std::uint32_t volume)
  {
    if (volume > 255U) {
      return false;
    }
    volume_ = volume;
    return true;
  }

  // times the output ran dry in the middle of the sound: the writer was too slow
  std::uint32_t dry() const
  { return dry_; }

private:
  win32(const win32&);
  win32& operator=(const win32&);

  static const std::uint32_t buffer_count = 8U;       // buffers in rotation
  static const std::uint32_t buffer_ms    = 20U;      // sound in one buffer
  static const std::uint32_t rate_min     = 8000U;    // [Hz]
  static const std::uint32_t rate_max     = 96000U;   // [Hz]
  static const std::uint32_t frames_max   = (rate_max * buffer_ms) / 1000U;
  static const std::uint32_t poll_ms      = 5U;
  static const std::uint32_t timeout_ms   = 2000U;    // longest wait for Windows to hand a buffer back

  // buffers Windows still holds
  std::uint32_t queued() const
  {
    std::uint32_t count = 0U;
    for (std::uint32_t i = 0U; i < buffer_count; ++i) {
      count += ((header_[i].dwFlags & WHDR_INQUEUE) != 0U) ? 1U : 0U;
    }
    return count;
  }

  // hand the buffer that was filled to Windows and wait for the next one to be free
  bool submit()
  {
    if (playing_ && (queued() == 0U)) {
      ++dry_;
    }
    header_[index_].dwBufferLength = fill_ * 4U;
    if (::waveOutWrite(out_, &header_[index_], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
      return false;
    }
    playing_ = true;
    fill_    = 0U;
    index_   = (index_ + 1U) % buffer_count;
    return wait_free();
  }

  // wait until Windows has handed back the buffer to fill next; false after the timeout
  bool wait_free()
  {
    for (std::uint32_t waited = 0U; (header_[index_].dwFlags & WHDR_INQUEUE) != 0U; waited += poll_ms) {
      if (waited >= timeout_ms) {
        return false;
      }
      (void)::WaitForSingleObject(event_, poll_ms);
    }
    return true;
  }

  HWAVEOUT        out_;         // NULL = off
  HANDLE          event_;       // Windows has finished a buffer
  WAVEHDR         header_[buffer_count];
  std::int16_t    data_[buffer_count][frames_max * 2U];
  std::uint32_t   frames_;      // frames per buffer at the current rate
  std::uint32_t   fill_;        // frames in the buffer that is being filled
  std::uint32_t   index_;       // the buffer that is being filled
  std::uint32_t   volume_;
  std::uint32_t   dry_;
  bool            playing_;     // a buffer was handed over since start()
};

} // namespace port
} // namespace sid

#endif  // _SID_PORT_WIN32_H_
