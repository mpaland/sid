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
// \brief sid extension: a player for RTTTL, the ring tones of the mobile phones of the 90s as text
//        A tune is one line of text - a name, the defaults and the notes, divided by colons:
//          "scale:d=4,o=5,b=120:c,d,e,f,g,a,b,2c6"
//          d   default duration: 1, 2, 4, 8, 16 or 32 (a whole note .. a 32nd; 4 if it is not given)
//          o   default octave (6 if it is not given); a4 is 440 Hz
//          b   beats per minute, a beat is a quarter note (63 if it is not given)
//        A note is [duration] note [#] [.] [octave]: 8c#6 is an eighth of c sharp in octave 6. The notes are
//        c d e f g a b (h is b too) and p for a pause, a dot makes a note half as long again (before or behind
//        the octave). Upper and lower case are the same, blanks are skipped.
//          rtttl::parse(text, tune)        checks the text and reads the defaults
//          rtttl::player(rate, tune)       the tune as a source: it ends after the last note
//          rtttl::length(tune)             its duration [ms]
//          sid::play_rtttl(out, text)      plays a text on an open output
//        The text is played from where it is (flash or RAM): nothing is copied, no table of notes is made.
//        The notes are those of sid::tone (tone.h), so a tune sounds as a sine, square or triangle wave.
//        An entry that is no note is left out. The octaves are 4 .. 7 by the standard, 0 .. 9 are taken.
//        Integer only, no heap.
//
//          sid::play_rtttl(out, "scale:d=4,o=5,b=120:c,d,e,f,g,a,b,2c6", sid::wave_square);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXT_RTTTL_H_
#define _SID_EXT_RTTTL_H_

#include "../tone.h"


namespace sid {
namespace rtttl {

// a tune, checked by parse()
typedef struct tag_info {
  const char*     name;         // name of the tune: 'name_length' characters, not terminated
  std::size_t     name_length;
  const char*     notes;        // the notes: the text behind the second colon
  std::uint8_t    duration;     // default duration: 1, 2, 4, 8, 16 or 32
  std::uint8_t    octave;       // default octave: 0 .. 9
  std::uint16_t   tempo;        // beats (quarter notes) per minute: 1 .. 999
} info;


namespace detail {

// one note of a tune
typedef struct tag_event {
  std::uint16_t   freq;         // [Hz], 0 = pause
  std::uint8_t    duration;     // 1, 2, 4, 8, 16 or 32
  bool            dotted;       // half as long again
} event;


inline bool space(char c)
{ return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'); }


inline char lower(char c)
{ return ((c >= 'A') && (c <= 'Z')) ? static_cast<char>(c - 'A' + 'a') : c; }


inline bool digit(char c)
{ return (c >= '0') && (c <= '9'); }


/**
 * Read a decimal number
 * \param text The text, moved behind the number
 * \param value The number, 65535 if it is greater; unchanged if there is none
 * \return False if the text does not start with a digit
 */
inline bool number(const char*& text, std::uint32_t& value)
{
  if (!digit(*text)) {
    return false;
  }
  value = 0U;
  for (; digit(*text); ++text) {
    value = (value > 6552U) ? 65535U : ((value * 10U) + static_cast<std::uint32_t>(*text - '0'));
  }
  return true;
}


// true for the durations of RTTTL: 1, 2, 4, 8, 16, 32
inline bool duration_ok(std::uint32_t value)
{ return (value != 0U) && (value <= 32U) && ((value & (value - 1U)) == 0U); }


/**
 * Frequency of a note
 * \param semitone 0 (c) .. 11 (b)
 * \param octave 0 .. 10; a of octave 4 is 440 Hz
 * \return Frequency [Hz], rounded
 */
inline std::uint16_t frequency(std::uint32_t semitone, std::uint32_t octave)
{
  // octave 10: the lower ones are halves of it
  static const std::uint16_t top[12] = {
    16744U, 17740U, 18795U, 19912U, 21096U, 22351U, 23680U, 25088U, 26580U, 28160U, 29834U, 31609U
  };
  const std::uint32_t shift = 10U - octave;
  return static_cast<std::uint16_t>((top[semitone] + ((1UL << shift) >> 1U)) >> shift);
}


/**
 * Read the next note of a tune. An entry that is no note is left out
 * \param text The text, moved behind the note
 * \param tune The tune, for its defaults
 * \param note The note
 * \return False if the text has ended
 */
inline bool next(const char*& text, const info& tune, event& note)
{
  // semitones of a b c d e f g h
  static const std::uint8_t semitones[8] = { 9U, 11U, 0U, 2U, 4U, 5U, 7U, 11U };

  for (;;) {
    while (space(*text) || (*text == ',')) {
      ++text;
    }
    if (*text == '\0') {
      return false;
    }

    std::uint32_t duration = tune.duration;
    bool          valid    = !number(text, duration) || duration_ok(duration);
    std::uint32_t semitone = 0U;
    std::uint32_t octave   = tune.octave;
    bool          pause    = false;
    bool          dotted   = false;
    const char    letter   = lower(*text);
    if ((letter >= 'a') && (letter <= 'h')) {
      semitone = semitones[letter - 'a'];
      ++text;
    }
    else if (letter == 'p') {
      pause = true;
      ++text;
    }
    else {
      valid = false;
    }
    if (valid) {
      if (*text == '#') {
        ++semitone;
        ++text;
      }
      if (*text == '.') {
        dotted = true;
        ++text;
      }
      if (digit(*text)) {
        octave = static_cast<std::uint32_t>(*text - '0');
        ++text;
      }
      if (*text == '.') {
        dotted = true;
        ++text;
      }
      if (semitone > 11U) {
        semitone = 0U;      // b sharp is the c of the next octave
        ++octave;
      }
    }

    // the rest of the entry
    while ((*text != '\0') && (*text != ',')) {
      ++text;
    }
    if (valid) {
      note.freq     = pause ? static_cast<std::uint16_t>(0U) : frequency(semitone, octave);
      note.duration = static_cast<std::uint8_t>(duration);
      note.dotted   = dotted;
      return true;
    }
  }
}


/**
 * Duration of a note. What is less than a ms is carried to the next note, so a tune is as long as its notes say
 * \param tune The tune, for its tempo
 * \param note The note
 * \param carry What the notes before have left [1 / tempo ms], and what this one leaves; 0 at the start
 * \return Duration [ms]
 */
inline std::uint32_t ms_of(const info& tune, const event& note, std::uint32_t& carry)
{
  // a whole note is 240 000 ms / tempo; in 1 / 64 of it, a dotted note is 3 / 2
  const std::uint32_t parts = (32U / note.duration) * (note.dotted ? 3U : 2U);
  const std::uint32_t total = (3750U * parts) + carry;
  carry = total % tune.tempo;
  return total / tune.tempo;
}

} // namespace detail


/**
 * Check a tune and read its name and its defaults
 * \param text The tune, terminated by 0; it must stay valid while the tune is used
 * \param result The tune, valid if true is returned
 * \return False if the text is no RTTTL: the two colons are not there, or a default has a value that is none
 */
inline bool parse(const char* text, info& result)
{
  if (text == nullptr) {
    return false;
  }
  while (detail::space(*text)) {
    ++text;
  }
  result.name = text;
  while ((*text != '\0') && (*text != ':')) {
    ++text;
  }
  if (*text != ':') {
    return false;
  }
  result.name_length = static_cast<std::size_t>(text - result.name);
  while ((result.name_length != 0U) && detail::space(result.name[result.name_length - 1U])) {
    --result.name_length;
  }
  ++text;

  result.duration = 4U;
  result.octave   = 6U;
  result.tempo    = 63U;
  for (;;) {
    while (detail::space(*text) || (*text == ',')) {
      ++text;
    }
    if (*text == ':') {
      break;
    }
    if (*text == '\0') {
      return false;
    }
    const char key = detail::lower(*text);
    ++text;
    while (detail::space(*text)) {
      ++text;
    }
    if (*text != '=') {
      return false;
    }
    ++text;
    while (detail::space(*text)) {
      ++text;
    }
    std::uint32_t value = 0U;
    if ((key != 'd') && (key != 'o') && (key != 'b')) {
      // another setting (style, loops): not used
      while ((*text != '\0') && (*text != ',') && (*text != ':')) {
        ++text;
      }
    }
    else if (!detail::number(text, value)) {
      return false;
    }
    else if (key == 'd') {
      if (!detail::duration_ok(value)) {
        return false;
      }
      result.duration = static_cast<std::uint8_t>(value);
    }
    else if (key == 'o') {
      if (value > 9U) {
        return false;
      }
      result.octave = static_cast<std::uint8_t>(value);
    }
    else {
      if ((value == 0U) || (value > 999U)) {
        return false;
      }
      result.tempo = static_cast<std::uint16_t>(value);
    }
  }
  result.notes = text + 1;
  return true;
}


/**
 * Duration of a tune
 * \param tune The tune
 * \return Duration [ms]
 */
inline std::uint32_t length(const info& tune)
{
  const char*   text  = tune.notes;
  std::uint32_t carry = 0U;
  std::uint32_t ms    = 0U;
  detail::event note;
  while (detail::next(text, tune, note)) {
    ms += detail::ms_of(tune, note, carry);
  }
  return ms;
}


// plays a tune: a source that ends after the last note (never with loops = 0)
class player : public source
{
private:
  tone            tone_;
  const info      tune_;        // a copy: four words
  const char*     next_;        // the text behind the note that plays
  std::uint32_t   carry_;       // of the durations [1 / tempo ms]
  std::uint32_t   loops_;       // rounds left, 0 = for ever
  wave_type       wave_;
  std::uint32_t   level_;
  bool            played_;      // this round had a note
  bool            ended_;

public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   * \param tune The tune of parse(); its text must stay valid while it plays
   * \param loops Times the tune plays, 0 = for ever
   * \param wave Waveform of all notes
   * \param level Loudness: 0 .. 255 (full scale)
   */
  player(std::uint32_t sample_rate, const info& tune, std::uint32_t loops = 1U, wave_type wave = wave_sine, std::uint32_t level = 128U)
    : tone_(sample_rate)
    , tune_(tune)
    , next_(tune.notes)
    , carry_(0U)
    , loops_(loops)
    , wave_(wave)
    , level_(level)
    , played_(false)
    , ended_(false)
  { }


  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    while (n < frames) {
      n += tone_.render(&stereo[n * 2U], frames - n);
      if ((n < frames) && !next()) {
        break;
      }
    }
    return n;
  }


  // true when the tune has ended
  bool ended() const
  { return ended_; }


private:
  // start the next note; \return false when the tune has ended
  bool next()
  {
    while (!ended_) {
      detail::event note;
      if (detail::next(next_, tune_, note)) {
        tone_.set(note.freq, detail::ms_of(tune_, note, carry_), wave_, level_);
        played_ = true;
        return true;
      }
      // the end of the text: once more, but a tune without a note ends at once
      if (!played_ || ((loops_ != 0U) && (--loops_ == 0U))) {
        ended_ = true;
      }
      next_   = tune_.notes;
      played_ = false;
    }
    return false;
  }
};

} // namespace rtttl


/**
 * Play a RTTTL tune on an open output; returns when it is queued
 * \param out The output
 * \param text The tune, terminated by 0
 * \param wave Waveform of all notes
 * \param level Loudness: 0 .. 255 (full scale)
 * \return False if the output is not open, the text is no RTTTL, or the device failed
 */
inline bool play_rtttl(output& out, const char* text, wave_type wave = wave_sine, std::uint32_t level = 128U)
{
  rtttl::info tune;
  if ((out.rate() == 0U) || !rtttl::parse(text, tune)) {
    return false;
  }
  rtttl::player music(out.rate(), tune, 1U, wave, level);
  return out.play(music);
}

} // namespace sid

#endif  // _SID_EXT_RTTTL_H_
