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
// \brief sid extension: a mixer, several sources at once
//        Sources are played one after the other by the output. The mixer is a source that plays up to N others
//        at the same time: music with a sound effect, a clip with a tone on top.
//          mixer<N>            a mixer of up to N inputs
//          add(source, level)  the source plays from now on, with a level of 0 .. 255 (255: as it is)
//          remove(source)      takes it out again
//          playing(source)     true while it is an input
//        An input that has ended is taken out by the mixer, so its place is free again. The mixer itself ends
//        when its last input has ended. A mixer made as 'endless' never ends: it plays silence while it has no
//        input, the way the synthesizer does - for the loop of an app, which plays it in slices of time and
//        adds a sound when something happens.
//        The sum is clipped, not wrapped: the levels of inputs which are loud together should add up to
//        about 255. All inputs are made for the sample rate of the output, nothing is resampled.
//        The mixer does not own its inputs: a source must stay valid while it is an input.
//        Integer only, no heap; render() needs SID_CHUNK_FRAMES * 4 bytes of stack.
//        One task: add / remove and render() are called from the task that plays.
//
//          sid::sequencer music(out.rate(), song, 0U);       // loops for ever
//          sid::tone      beep(out.rate());
//          sid::mixer<2>  mix;
//          mix.add(music, 160U);
//          for (;;) {
//            if (button()) {
//              beep.set(880U, 100U);
//              mix.add(beep);                                // over the music
//            }
//            out.play(mix, 20U);
//          }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXT_MIXER_H_
#define _SID_EXT_MIXER_H_

#include "../source.h"


namespace sid {

// plays up to N sources at the same time
template<std::size_t N>
class mixer : public source
{
private:
  typedef struct tag_input {
    source*         src;        // nullptr: the place is free
    std::int32_t    gain;       // 0 .. 256 (as it is)
  } input;

  input           input_[N];
  bool            endless_;     // silence instead of the end while there is no input

public:
  /**
   * \param endless True: the mixer never ends, it plays silence while it has no input.
   *                False: it ends when its last input has ended
   */
  explicit mixer(bool endless = false)
    : endless_(endless)
  {
    static_assert(N > 0U, "a mixer has at least one input");
    clear();
  }


  /**
   * Play a source from now on. A source that is an input already goes on as it is, with the new level
   * \param src The source; it must stay valid while it is an input
   * \param level Loudness: 0 .. 255 (full scale, the source as it is)
   * \return False if all N inputs are taken
   */
  bool add(source& src, std::uint32_t level = 255U)
  {
    if (set_level(src, level)) {
      return true;
    }
    for (std::size_t i = 0U; i < N; ++i) {
      if (input_[i].src == nullptr) {
        input_[i].src  = &src;
        input_[i].gain = gain_of(level);
        return true;
      }
    }
    return false;
  }


  /**
   * Take a source out: it is silent at once
   * \param src The source; nothing happens if it is no input
   */
  void remove(const source& src)
  {
    for (std::size_t i = 0U; i < N; ++i) {
      if (input_[i].src == &src) {
        input_[i].src = nullptr;
      }
    }
  }


  // take all sources out
  void clear()
  {
    for (std::size_t i = 0U; i < N; ++i) {
      input_[i].src  = nullptr;
      input_[i].gain = 0;
    }
  }


  /**
   * Set the level of an input
   * \param src The source
   * \param level Loudness: 0 .. 255 (full scale, the source as it is)
   * \return False if the source is no input
   */
  bool set_level(const source& src, std::uint32_t level)
  {
    for (std::size_t i = 0U; i < N; ++i) {
      if (input_[i].src == &src) {
        input_[i].gain = gain_of(level);
        return true;
      }
    }
    return false;
  }


  // true while the source is an input: it was added and has not ended yet
  bool playing(const source& src) const
  {
    for (std::size_t i = 0U; i < N; ++i) {
      if (input_[i].src == &src) {
        return true;
      }
    }
    return false;
  }


  // number of inputs that play
  std::size_t inputs() const
  {
    std::size_t count = 0U;
    for (std::size_t i = 0U; i < N; ++i) {
      if (input_[i].src != nullptr) {
        ++count;
      }
    }
    return count;
  }


  /**
   * Produce the next frames: the sum of the inputs
   * \return Frames written; fewer than 'frames': the last input has ended (never if the mixer is endless)
   */
  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::int16_t chunk[SID_CHUNK_FRAMES * 2U];
    std::size_t  done = 0U;
    while (done < frames) {
      const std::size_t   part = ((frames - done) < SID_CHUNK_FRAMES) ? (frames - done) : SID_CHUNK_FRAMES;
      std::int16_t* const out  = &stereo[done * 2U];
      for (std::size_t s = 0U; s < (part * 2U); ++s) {
        out[s] = 0;
      }

      std::size_t longest = 0U;     // frames of the input that gave the most
      for (std::size_t i = 0U; i < N; ++i) {
        if (input_[i].src == nullptr) {
          continue;
        }
        const std::size_t  got  = input_[i].src->render(chunk, part);
        const std::int32_t gain = input_[i].gain;
        for (std::size_t s = 0U; s < (got * 2U); ++s) {
          const std::int32_t sum = out[s] + ((chunk[s] * gain) / 256);
          out[s] = static_cast<std::int16_t>((sum > 32767) ? 32767 : ((sum < -32768) ? -32768 : sum));
        }
        if (got < part) {
          input_[i].src = nullptr;      // it has ended: the place is free
        }
        longest = (got > longest) ? got : longest;
      }

      if (!endless_ && (longest < part)) {
        return done + longest;          // no input is left
      }
      done += part;
    }
    return frames;
  }


private:
  // level 0 .. 255 as a gain of 0 .. 256, so that full scale changes nothing
  static std::int32_t gain_of(std::uint32_t level)
  {
    const std::uint32_t limited = (level > 255U) ? 255U : level;
    return static_cast<std::int32_t>(limited + (limited >> 7U));
  }
};

} // namespace sid

#endif  // _SID_EXT_MIXER_H_
