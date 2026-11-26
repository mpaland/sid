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
// \brief sid extension: sound effects for games and devices, ready to play
//        An effect is a patch of the synthesizer (synth.h) and up to four notes of it, one after the other:
//        constant data, no samples. The effects of this file:
//          for a user interface   click, blip, confirm, error, alarm
//          for a game             coin, jump, laser, hit, explosion, power_up, win, lose
//        Only the effects a program uses get into it. An effect of your own is a constant of sfx::effect.
//        Each effect reaches about half of full scale, so two of them, or one over music, fit without clipping.
//          sfx::player(rate)             plays effects: a synthesizer, so SID_VOICES (4) effects at the same time
//          play(effect, transpose)       starts an effect, some semitones higher or lower if wanted
//          silent()                      true when no effect sounds
//          sid::play_sfx(out, effect)    plays one effect on an open output, to its end
//        The player is a source that never ends, like the synthesizer: render() always fills all frames
//        (silence when nothing plays). It takes a voice without an effect for a new one; when all voices are
//        busy the oldest effect ends. Over music the player is an input of a mixer (ext/mixer.h).
//        Integer only, no heap. One task: play() and render() are called from the task that plays.
//
//          sid::sfx::player fx(out.rate());
//          fx.play(sid::sfx::coin);
//          while (!fx.silent()) {
//            out.play(fx, 10U);
//          }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXT_SFX_H_
#define _SID_EXT_SFX_H_

#include "../synth.h"


namespace sid {
namespace sfx {

// notes an effect can have
const std::size_t parts = 4U;

// one note of an effect
typedef struct tag_part {
  std::uint8_t    key;          // key of the note (sid::key)
  std::uint16_t   ms;           // time until the next note starts, or the last one ends [ms]; 0: no more notes
} part;

// an effect. It must stay valid while it sounds: a constant
typedef struct tag_effect {
  patch           sound;        // the sound of its notes
  part            notes[parts]; // the notes; after the last one the sound goes into its release
} effect;


//                           wave                  level attack decay sustain release pulse pwm lfo: rate delay vibrato sweep glide  arpeggio     ms  mod     pitch  filter    reson. cutoff end ms       notes: key, ms

// a key of a keyboard or a touch screen: a tick
const effect click     = { { osc_noise,              110U,   0U,   10U,    0U,   10U,    0U, 0U,       0U,   0U,     0U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c7, 20U } } };

// a step in a menu: a short high tone
const effect blip      = { { osc_pulse,              100U,   0U,   70U,    0U,   30U,  128U, 0U,       0U,   0U,     0U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::a5, 60U } } };

// accepted, done: two soft tones up
const effect confirm   = { { osc_triangle,           130U,   2U,  250U,    0U,   80U,    0U, 0U,       0U,   0U,     0U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::e5, 80U }, { key::b5, 200U } } };

// refused, wrong: two low buzzing tones down
const effect error     = { { osc_saw,                110U,   2U,  200U,  180U,   40U,    0U, 0U,       0U,   0U,     0U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::a2, 130U }, { key::f2, 260U } } };

// an alarm: a sawtooth with a deep, fast vibrato
const effect alarm     = { { osc_saw,                110U,   5U,    0U,  255U,   60U,    0U, 0U,      40U,   0U,   500U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::a4, 1500U } } };

// a coin: two high tones, the second rings
const effect coin      = { { osc_pulse,              130U,   0U,  450U,    0U,  100U,  128U, 0U,       0U,   0U,     0U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::b5, 80U }, { key::e6, 450U } } };

// a jump: a square that rises
const effect jump      = { { osc_pulse,              130U,   0U,  260U,    0U,   40U,  128U, 0U,       0U,   0U,     0U,    70,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c4, 280U } } };

// a laser: a pulse that falls four octaves in a quarter of a second
const effect laser     = { { osc_pulse,               90U,   0U,  250U,    0U,   50U,   64U, 0U,       0U,   0U,     0U,  -190,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c7, 280U } } };

// a hit, damage: a short burst of noise that gets darker
const effect hit       = { { osc_noise,              135U,   0U,  140U,    0U,   40U,    0U, 0U,       0U,   0U,     0U,   -80,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c6, 150U } } };

// an explosion: low noise that fades slowly and gets darker
const effect explosion = { { osc_noise,              140U,   2U, 1200U,    0U,  300U,    0U, 0U,       0U,   0U,     0U,   -18,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c4, 1500U } } };

// a power up: an arpeggio through three octaves that climbs
const effect power_up  = { { osc_triangle | osc_pulse, 105U, 2U,  900U,    0U,  150U,  128U, 0U,       0U,   0U,     0U,    14,   0U, { 7U, 12U }, 40U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c4, 1100U } } };

// won, a level is done: a chord, note by note up
const effect win       = { { osc_pulse,              110U,   2U,  400U,   90U,  150U,   96U, 0U,      60U,  15U,    30U,     0,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::c5, 100U }, { key::e5, 100U }, { key::g5, 100U }, { key::c6, 500U } } };

// lost, the game is over: a chord, note by note down, each one sinks
const effect lose      = { { osc_pulse,               85U,   2U,  500U,   90U,  250U,   64U, 0U,       0U,   0U,     0U,    -3,   0U, { 0U,  0U },  0U, mod_none,  0, filter_off,  0U,    0U, 0U, 0U },   { { key::g4, 200U }, { key::e4, 200U }, { key::c4, 200U }, { key::g3, 700U } } };


// plays effects: a source that never ends
class player : public source
{
private:
  typedef struct tag_slot {
    const effect*   what;         // effect of the voice, nullptr: none (its last note may still fade)
    std::uint32_t   left;         // frames to its next note
    std::uint32_t   started;      // number of the effect: the oldest ends when all voices are busy
    std::int8_t     transpose;    // [semitones]
    std::uint8_t    index;        // its next note
  } slot;

  synth           synth_;
  slot            slot_[SID_VOICES];    // slot n plays on voice n
  std::uint32_t   count_;               // effects started so far

public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   */
  explicit player(std::uint32_t sample_rate)
    : synth_(sample_rate)
    , count_(0U)
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      const slot empty = { nullptr, 0U, 0U, 0, 0U };
      slot_[i] = empty;
    }
  }


  /**
   * Start an effect. It takes a voice without an effect; when all are busy the oldest effect ends for it
   * \param what The effect; it must stay valid while it sounds
   * \param transpose Semitones added to the keys of its notes: -127 .. 127
   */
  void play(const effect& what, std::int32_t transpose = 0)
  {
    // a voice without an effect before a busy one, then the one that started the longest time ago
    std::size_t voice = 0U;
    for (std::size_t i = 1U; i < SID_VOICES; ++i) {
      const bool free_i = (slot_[i].what == nullptr);
      const bool free_v = (slot_[voice].what == nullptr);
      if ((free_i != free_v) ? free_i : ((count_ - slot_[i].started) > (count_ - slot_[voice].started))) {
        voice = i;
      }
    }
    ++count_;
    slot& s = slot_[voice];
    s.what      = &what;
    s.left      = 0U;           // its first note starts with the next frame
    s.started   = count_;
    s.transpose = static_cast<std::int8_t>((transpose > 127) ? 127 : ((transpose < -127) ? -127 : transpose));
    s.index     = 0U;
  }


  // end all effects: their notes go into the release
  void stop()
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      slot_[i].what = nullptr;
    }
    synth_.all_off();
  }


  // true when no effect sounds
  bool silent() const
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      if (slot_[i].what != nullptr) {
        return false;
      }
    }
    return synth_.silent();
  }


  /**
   * Produce the next frames; the player never ends
   * \return frames
   */
  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    while (n < frames) {
      // up to the next note of an effect
      std::size_t part_frames = frames - n;
      for (std::size_t i = 0U; i < SID_VOICES; ++i) {
        slot& s = slot_[i];
        if ((s.what != nullptr) && (s.left == 0U)) {
          next(i);
        }
        if ((s.what != nullptr) && (s.left < part_frames)) {
          part_frames = s.left;
        }
      }
      (void)synth_.render(&stereo[n * 2U], part_frames);
      n += part_frames;
      for (std::size_t i = 0U; i < SID_VOICES; ++i) {
        if (slot_[i].what != nullptr) {
          slot_[i].left -= static_cast<std::uint32_t>(part_frames);
        }
      }
    }
    return frames;
  }


private:
  // the next note of the effect of a voice, or its end
  void next(std::size_t voice)
  {
    slot& s = slot_[voice];
    if ((s.index >= parts) || (s.what->notes[s.index].ms == 0U)) {
      synth_.note_off(voice);
      s.what = nullptr;
      return;
    }
    const part&         now    = s.what->notes[s.index];
    const std::int32_t  number = static_cast<std::int32_t>(now.key) + s.transpose;
    synth_.note_on(voice, s.what->sound, static_cast<std::uint32_t>((number < 0) ? 0 : number));
    // at least one frame per ms: the synth takes no sample rate below 1000 Hz
    s.left = (static_cast<std::uint32_t>(now.ms) * (synth_.rate() / 1000U)) +
             ((static_cast<std::uint32_t>(now.ms) * (synth_.rate() % 1000U)) / 1000U);
    ++s.index;
  }
};

} // namespace sfx


/**
 * Play one effect on an open output, to its end; returns when it is queued
 * \param out The output
 * \param what The effect
 * \param transpose Semitones added to the keys of its notes
 * \return False if the output is not open or the device failed
 */
inline bool play_sfx(output& out, const sfx::effect& what, std::int32_t transpose = 0)
{
  if (out.rate() == 0U) {
    return false;
  }
  sfx::player fx(out.rate());
  fx.play(what, transpose);
  while (!fx.silent()) {
    if (!out.play(fx, 10U)) {
      return false;
    }
  }
  return true;
}

} // namespace sid

#endif  // _SID_EXT_SFX_H_
