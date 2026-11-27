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
// \brief sid - Sound Interface Driver library: the sequencer, which plays a song on the synthesizer (synth.h)
//        A song is written the way the tunes of the 80s were: in patterns. A pattern is a short line for one
//        voice, an array of steps (16 as a rule, one bar in sixteenth notes). A step starts a note with a patch,
//        ends the running note (key::off) or lets it go on (key::hold). The order of the song has one row per
//        bar; a row names the pattern of every track (track n plays on voice n) and a transposition, so one bass
//        pattern serves all chords. All of it is constant data for the flash:
//          const sid::step bass[16] = { { sid::key::a2, 0U }, { sid::key::hold, 0U }, { sid::key::a3, 0U }, ... };
//          const sid::step* const patterns[] = { bass, lead };
//          const sid::slot order[] = { { 0U, 0 }, { 1U, 0 },   { 0U, -4 }, { 1U, 0 } };     // 2 rows of 2 tracks
//          const sid::song tune = { patches, patterns, order, 2U, 2U, 16U, 125U };
//          sid::play(out, tune);
//        The sequencer is a source: it ends after the last row, when the last notes have faded.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_SEQUENCER_H_
#define _SID_SEQUENCER_H_

#include "synth.h"

// longest time the last notes of a song may take to fade [ms]: the song ends then
#ifndef SID_SONG_TAIL_MS
#define SID_SONG_TAIL_MS        2000U
#endif


namespace sid {

namespace key {
const std::uint8_t hold = 0U;       // step: nothing happens, a running note goes on
const std::uint8_t off  = 1U;       // step: the running note ends (release)
} // namespace key

// slot: the track plays no pattern in this row (a running note goes on)
const std::uint8_t no_pattern = 0xFFU;

// one step of a pattern
typedef struct tag_step {
  std::uint8_t    key;          // key of the note that starts, key::hold or key::off
  std::uint8_t    patch;        // index of its patch in the patches of the song
} step;

// what one track plays in one row of the song
typedef struct tag_slot {
  std::uint8_t    pattern;      // index in the patterns of the song, or no_pattern
  std::int8_t     transpose;    // semitones added to the keys of the pattern
} slot;

// a song
typedef struct tag_song {
  const patch*          patches;      // the instruments
  const step* const*    patterns;     // the patterns: arrays of 'steps' steps
  const slot*           order;        // the song: rows * tracks slots, row after row
  std::uint16_t         rows;         // number of rows
  std::uint8_t          tracks;       // number of tracks; track n plays on voice n (tracks above SID_VOICES are mute)
  std::uint8_t          steps;        // steps per pattern
  std::uint16_t         step_ms;      // duration of a step [ms]: 125 = sixteenth notes at 120 beats per minute
} song;


// plays a song: a source that ends after the last row (never with loops = 0)
class sequencer : public source
{
  synth           synth_;
  const song&     song_;
  std::uint32_t   loops_;       // rounds left, 0 = for ever
  std::size_t     row_;         // position of the next step
  std::size_t     step_;
  std::size_t     at_row_;      // position of the step that plays
  std::size_t     at_step_;
  std::uint32_t   left_;        // frames left of the current step
  std::uint32_t   fraction_;    // [1 / 1000 frame] carried to the next step
  std::uint32_t   tail_;        // [ms] of the fading end so far
  bool            ended_;       // all steps are played

public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   * \param tune The song; it and all it points to must stay valid while it plays
   * \param loops Times the song plays, 0 = for ever
   */
  sequencer(std::uint32_t sample_rate, const song& tune, std::uint32_t loops = 1U)
    : synth_(sample_rate)
    , song_(tune)
    , loops_(loops)
    , row_(0U)
    , step_(0U)
    , at_row_(0U)
    , at_step_(0U)
    , left_(0U)
    , fraction_(0U)
    , tail_(0U)
    , ended_((tune.rows == 0U) || (tune.steps == 0U) || (tune.step_ms == 0U))
  { }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    while (n < frames) {
      if ((left_ == 0U) && !next()) {
        break;
      }
      const std::size_t part = ((frames - n) < left_) ? (frames - n) : left_;
      (void)synth_.render(&stereo[n * 2U], part);
      n     += part;
      left_ -= static_cast<std::uint32_t>(part);
    }
    return n;
  }

  // the synth of the sequencer: voices above the tracks of the song are free for notes of the app
  synth& voices()
  { return synth_; }

  // row of the song that plays: 0 .. rows - 1 (the row of the step rendered last; 0 before the first)
  std::size_t row() const
  { return at_row_; }

  // step of that row that plays: 0 .. steps - 1
  std::size_t step() const
  { return at_step_; }

  // true once the last step of the song has started: no step follows it, its notes fade and the source ends
  bool ended() const
  { return ended_; }

private:
  // the next piece of the song: a step, or 10 ms of the fading end; \return false when the song has ended
  bool next()
  {
    if (ended_) {
      // the last notes end and fade
      if (tail_ == 0U) {
        synth_.all_off();
      }
      if (synth_.silent() || (tail_ >= SID_SONG_TAIL_MS)) {
        return false;
      }
      tail_ += 10U;
      left_  = synth_.rate() / 100U;
      return true;
    }

    // the notes of the step
    at_row_  = row_;
    at_step_ = step_;
    for (std::size_t track = 0U; track < song_.tracks; ++track) {
      const slot& where = song_.order[(row_ * song_.tracks) + track];
      if (where.pattern != no_pattern) {
        const sid::step& now = song_.patterns[where.pattern][step_];     // sid::step: step() is the position
        if (now.key == key::off) {
          synth_.note_off(track);
        }
        else if (now.key != key::hold) {
          const std::int32_t number = static_cast<std::int32_t>(now.key) + where.transpose;
          synth_.note_on(track, song_.patches[now.patch], static_cast<std::uint32_t>((number < 0) ? 0 : number));
        }
      }
    }

    // its duration in frames; the fractions add up, so a song is as long as its steps say
    fraction_ += static_cast<std::uint32_t>(song_.step_ms) * (synth_.rate() % 1000U);
    left_      = (static_cast<std::uint32_t>(song_.step_ms) * (synth_.rate() / 1000U)) + (fraction_ / 1000U);
    fraction_ %= 1000U;

    // the step after it
    if (++step_ >= song_.steps) {
      step_ = 0U;
      if (++row_ >= song_.rows) {
        row_ = 0U;
        if ((loops_ != 0U) && (--loops_ == 0U)) {
          ended_ = true;      // after this step
        }
      }
    }
    return true;
  }
};


/**
 * Play a song on an open output; returns when it is queued
 * \param out The output
 * \param tune The song
 * \param loops Times the song plays (0 = for ever: never returns)
 */
inline bool play(output& out, const song& tune, std::uint32_t loops = 1U)
{
  sequencer s(out.rate(), tune, loops);
  return out.play(s);
}

} // namespace sid

#endif  // _SID_SEQUENCER_H_
