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
// \brief sid extension: a small player for MOD files (ProTracker, 4 channels)
//        The music files of the Amiga: 31 samples of 8 bit, patterns of 64 rows for 4 channels and an order of
//        up to 128 positions. The file is played from where it is (flash or RAM), nothing is copied.
//          mod::parse(file, size, tune)    checks a file: 31 samples, 4 channels (M.K., M!K!, FLT4, 4CHN)
//          mod::player(rate, tune, loops)  the song as a source: it ends after the last position (0 loops: never)
//          sid::play_mod(out, file, size)  plays a file on an open output
//        The usual effects of the music of the time: 0 arpeggio, 1 / 2 portamento, 3 tone portamento,
//        4 vibrato, 5 / 6 those two with a volume slide, 9 sample offset, A volume slide, B position jump,
//        C volume, D pattern break, F speed / tempo, E1 / E2 fine portamento, EA / EB fine volume slide,
//        EC note cut, ED note delay. Other effects are ignored.
//        The four channels are panned as on the Amiga (left, right, right, left); the separation is set from
//        mono to that hard panning. The samples are interpolated (linear). A song ends after its last position,
//        or when it jumps back to a position that has played (the way most MOD files loop); the sound fades
//        out over 10 ms then.
//        Integer only, no heap. A finetune other than 0 is computed, not taken from the table of ProTracker:
//        its periods can differ by 1 or 2 (a few cents).
//
//          sid::mod::info tune;
//          if (sid::mod::parse(file, size, tune) && out.open(22050U)) {
//            sid::mod::player music(out.rate(), tune);
//            out.play(music);
//          }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXT_MOD_H_
#define _SID_EXT_MOD_H_

#include "../tone.h"


namespace sid {
namespace mod {

// channels of a MOD file this player takes
const std::size_t channels = 4U;

// rows of a pattern
const std::uint32_t rows = 64U;

// a MOD file, checked by parse()
typedef struct tag_info {
  const std::uint8_t*   file;       // the file
  std::size_t           size;       // its size [bytes]
  std::uint8_t          length;     // positions in the order: 1 .. 128
  std::uint8_t          patterns;   // patterns in the file
} info;

// a sample (an instrument) of a MOD file
typedef struct tag_sample {
  const std::int8_t*    data;         // the samples (8 bit, signed), in the file
  std::uint32_t         length;       // [bytes], 0 = no sample
  std::uint32_t         loop_start;   // [bytes]
  std::uint32_t         loop_length;  // [bytes], 0 = it plays once
  std::uint8_t          finetune;     // 0 .. 7, 8 .. 15 = -8 .. -1 [1 / 8 semitone]
  std::uint8_t          volume;       // 0 .. 64
} sample;


namespace detail {

// layout of a MOD file with 31 samples
const std::size_t sample_header = 20U;      // 31 headers of 30 bytes follow the title
const std::size_t song_length   = 950U;
const std::size_t order_table   = 952U;     // 128 positions
const std::size_t signature     = 1080U;
const std::size_t pattern_data  = 1084U;
const std::size_t pattern_size  = rows * channels * 4U;

inline std::uint32_t be16(const std::uint8_t* p)
{ return (static_cast<std::uint32_t>(p[0]) << 8U) | static_cast<std::uint32_t>(p[1]); }

inline bool tag(const std::uint8_t* p, const char* id)
{ return (p[0] == static_cast<std::uint8_t>(id[0])) && (p[1] == static_cast<std::uint8_t>(id[1])) &&
         (p[2] == static_cast<std::uint8_t>(id[2])) && (p[3] == static_cast<std::uint8_t>(id[3])); }

// the periods of ProTracker without finetune: C-1 .. B-3
const std::uint16_t periods[36] = {
  856U, 808U, 762U, 720U, 678U, 640U, 604U, 570U, 538U, 508U, 480U, 453U,
  428U, 404U, 381U, 360U, 340U, 320U, 302U, 285U, 269U, 254U, 240U, 226U,
  214U, 202U, 190U, 180U, 170U, 160U, 151U, 143U, 135U, 127U, 120U, 113U
};

// index of the period of the table nearest to a period
inline std::uint32_t note_of(std::uint32_t period)
{
  std::uint32_t best = 0U;
  std::uint32_t distance = 0xFFFFFFFFUL;
  for (std::uint32_t i = 0U; i < 36U; ++i) {
    const std::uint32_t d = (period > periods[i]) ? (period - periods[i]) : (periods[i] - period);
    if (d < distance) {
      distance = d;
      best     = i;
    }
  }
  return best;
}

// period of a note of the table with a finetune
inline std::uint32_t period_of(std::uint32_t note, std::uint32_t finetune)
{
  // 2^(-finetune / 96) [1 / 65536]: an eighth of a semitone per step, 8 .. 15 are -8 .. -1
  static const std::uint32_t factor[16] = {
    65536UL, 65065UL, 64596UL, 64132UL, 63670UL, 63212UL, 62757UL, 62306UL,
    69433UL, 68933UL, 68438UL, 67945UL, 67456UL, 66971UL, 66489UL, 66011UL
  };
  return ((static_cast<std::uint32_t>(periods[(note > 35U) ? 35U : note]) * factor[finetune & 15U]) + 32768UL) >> 16U;
}

// index of the note of the table a period with a finetune is nearest to
inline std::uint32_t note_in(std::uint32_t period, std::uint32_t finetune)
{
  if ((finetune & 15U) == 0U) {
    return note_of(period);
  }
  std::uint32_t best = 0U;
  std::uint32_t distance = 0xFFFFFFFFUL;
  for (std::uint32_t i = 0U; i < 36U; ++i) {
    const std::uint32_t p = period_of(i, finetune);
    const std::uint32_t d = (period > p) ? (period - p) : (p - period);
    if (d < distance) {
      distance = d;
      best     = i;
    }
  }
  return best;
}

// the period a note of a pattern plays with a finetune (finetune 0: the period as written)
inline std::uint32_t tune(std::uint32_t period, std::uint32_t finetune)
{
  return ((finetune & 15U) == 0U) ? period : period_of(note_of(period), finetune);
}

} // namespace detail


/**
 * Check a MOD file: 31 samples, 4 channels, all patterns in the file. A file whose samples are cut off plays as
 * far as it goes
 * \param file The file
 * \param size Size of the file [bytes]
 * \param result The song, valid if true is returned
 * \return False if this is no MOD file of 4 channels and 31 samples
 */
inline bool parse(const std::uint8_t* file, std::size_t size, info& result)
{
  if ((file == nullptr) || (size < detail::pattern_data)) {
    return false;
  }
  const std::uint8_t* const id = file + detail::signature;
  if (!detail::tag(id, "M.K.") && !detail::tag(id, "M!K!") && !detail::tag(id, "FLT4") && !detail::tag(id, "4CHN")) {
    return false;
  }
  const std::uint32_t length = file[detail::song_length];
  if ((length == 0U) || (length > 128U)) {
    return false;
  }
  // the patterns: as many as the highest number in the order (all 128 entries, as ProTracker counts them)
  std::uint32_t patterns = 0U;
  for (std::size_t i = 0U; i < 128U; ++i) {
    const std::uint32_t pattern = file[detail::order_table + i];
    patterns = (pattern >= patterns) ? (pattern + 1U) : patterns;
  }
  if (patterns > 128U) {
    return false;
  }
  if (((size - detail::pattern_data) / detail::pattern_size) < patterns) {
    return false;     // patterns cut off
  }
  result.file     = file;
  result.size     = size;
  result.length   = static_cast<std::uint8_t>(length);
  result.patterns = static_cast<std::uint8_t>(patterns);
  return true;
}


/**
 * A sample of a song
 * \param tune The song
 * \param number 1 .. 31
 * \return The sample; length 0 if there is none (or it is not in the file)
 */
inline sample sample_of(const info& tune, std::uint32_t number)
{
  sample result = { nullptr, 0U, 0U, 0U, 0U, 0U };
  if ((number == 0U) || (number > 31U)) {
    return result;
  }
  // the samples follow the patterns, one after the other
  std::size_t offset = detail::pattern_data + (static_cast<std::size_t>(tune.patterns) * detail::pattern_size);
  for (std::uint32_t i = 1U; i < number; ++i) {
    offset += static_cast<std::size_t>(detail::be16(tune.file + detail::sample_header + ((i - 1U) * 30U) + 22U)) * 2U;
  }
  const std::uint8_t* const header = tune.file + detail::sample_header + ((number - 1U) * 30U);
  result.finetune = static_cast<std::uint8_t>(header[24] & 15U);
  result.volume   = static_cast<std::uint8_t>((header[25] > 64U) ? 64U : header[25]);
  if (offset >= tune.size) {
    return result;
  }
  std::uint32_t length = detail::be16(header + 22) * 2U;
  if (length > (tune.size - offset)) {
    length = static_cast<std::uint32_t>(tune.size - offset);      // cut off: as far as it goes
  }
  result.data   = reinterpret_cast<const std::int8_t*>(tune.file + offset);
  result.length = length;
  // a loop of more than one word; one that reaches over the end is shortened
  const std::uint32_t loop_start  = detail::be16(header + 26) * 2U;
  std::uint32_t       loop_length = detail::be16(header + 28) * 2U;
  if ((loop_length > 2U) && (loop_start < length)) {
    loop_length        = ((length - loop_start) < loop_length) ? (length - loop_start) : loop_length;
    result.loop_start  = loop_start;
    result.loop_length = loop_length;
  }
  return result;
}


/**
 * The title of a song
 * \param tune The song
 * \param text Up to 20 characters and the terminating zero
 */
inline void title(const info& tune, char (&text)[21])
{
  for (std::size_t i = 0U; i < 20U; ++i) {
    const char c = static_cast<char>(tune.file[i]);
    text[i] = ((c == '\0') || ((c >= ' ') && (c <= '~'))) ? c : '.';
  }
  text[20] = '\0';
}


// plays a song: a source that ends after the last position (never with loops = 0)
class player : public source
{
public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   * \param tune The song (mod::parse()); the file must stay valid while it plays, the info is copied
   * \param loops Times the song plays, 0 = for ever
   * \param separation Stereo: 0 = mono .. 255 = the hard panning of the Amiga
   */
  player(std::uint32_t sample_rate, const info& tune, std::uint32_t loops = 1U, std::uint32_t separation = 128U)
    : song_(tune)
    , rate_((sample_rate < 1000U) ? 1000U : sample_rate)
    , scale_(static_cast<std::uint32_t>((static_cast<std::uint64_t>(clock) << 16U) / rate_))
    , loops_(loops)
    , separation_(static_cast<std::int32_t>((separation > 255U) ? 255U : separation))
    , left_(0U)
    , fraction_(0U)
    , fade_(rate_ / 100U)
    , order_(0U)
    , row_(0U)
    , tick_(0U)
    , speed_(6U)
    , tempo_(125U)
    , jump_(0U)
    , break_row_(0U)
    , state_(playing)
    , flow_(flow_none)
  {
    for (std::size_t i = 0U; i < channels; ++i) {
      channel empty = { };
      voice_[i] = empty;
    }
    for (std::size_t i = 0U; i < 16U; ++i) {
      played_[i] = 0U;
    }
    played_[0] = 1U;
  }

  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    std::size_t n = 0U;
    while (n < frames) {
      if ((left_ == 0U) && !next()) {
        break;
      }
      const std::size_t part = ((frames - n) < left_) ? (frames - n) : left_;
      for (std::size_t f = 0U; f < part; ++f) {
        // Amiga: channels 0 and 3 left, 1 and 2 right; the separation moves them to the middle
        const std::int32_t a   = (mix(voice_[0]) + mix(voice_[3])) / 2;
        const std::int32_t b   = (mix(voice_[1]) + mix(voice_[2])) / 2;
        const std::int32_t mid = (a + b) / 2;
        const std::int32_t side = (((a - b) / 2) * separation_) / 255;
        std::int32_t l = mid + side;
        std::int32_t r = mid - side;
        if (state_ == fading) {
          const std::int32_t gain = static_cast<std::int32_t>(left_ - f);
          l = (l * gain) / static_cast<std::int32_t>(fade_);
          r = (r * gain) / static_cast<std::int32_t>(fade_);
        }
        stereo[(n + f) * 2U]      = static_cast<std::int16_t>((l > 32767) ? 32767 : ((l < -32767) ? -32767 : l));
        stereo[(n + f) * 2U + 1U] = static_cast<std::int16_t>((r > 32767) ? 32767 : ((r < -32767) ? -32767 : r));
      }
      n     += part;
      left_ -= static_cast<std::uint32_t>(part);
    }
    return n;
  }

  // position in the order that plays: 0 .. length - 1
  std::uint32_t order() const
  { return order_; }

  // row of the pattern that plays: 0 .. 63
  std::uint32_t row() const
  { return row_; }

  // true when the song has ended
  bool ended() const
  { return state_ == done; }

private:
  // Amiga clock of the sound (PAL) [Hz]: a period of p plays clock / p samples per second
  static const std::uint32_t clock      = 3546895UL;
  static const std::uint32_t period_min = 113U;     // B-3: limits of the portamento
  static const std::uint32_t period_max = 856U;     // C-1

  typedef enum tag_state_type {
    playing = 0,
    fading,             // the song has ended, the sound fades
    done
  } state_type;

  typedef enum tag_flow_type {
    flow_none = 0,
    flow_jump,          // Bxx
    flow_break,         // Dxx
    flow_both,          // Bxx and Dxx in one row
    flow_stop           // F00
  } flow_type;

  typedef struct tag_channel {
    const std::int8_t*  data;           // the sample that sounds, nullptr: silent
    std::uint32_t       end;            // [bytes] where it ends or loops
    std::uint32_t       loop_start;     // [bytes]
    std::uint32_t       loop_length;    // [bytes], 0 = no loop
    std::uint32_t       pos;            // [bytes] in the sample
    std::uint32_t       fraction;       // [1 / 65536 byte]
    std::uint32_t       step;           // [1 / 65536 byte] per frame
    std::uint16_t       period;         // of the note
    std::uint16_t       target;         // tone portamento: the period to go to
    std::uint16_t       delayed;        // EDx: the period of the note that waits
    std::uint8_t        number;         // sample the notes play: 1 .. 31, 0 = none
    std::uint8_t        finetune;
    std::uint8_t        volume;         // 0 .. 64
    std::uint8_t        effect;         // of the row
    std::uint8_t        param;
    std::uint8_t        porta;          // speed of the tone portamento
    std::uint8_t        vibrato;        // speed << 4 | depth
    std::uint8_t        vibrato_pos;    // 0 .. 63 = one period
    std::uint8_t        offset;         // 9xx [256 bytes]
  } channel;

  // the next frame of a channel: -32768 .. 32767 at volume 64
  static std::int32_t mix(channel& c)
  {
    if (c.data == nullptr) {
      return 0;
    }
    const std::int32_t  now  = c.data[c.pos];
    const std::uint32_t next = c.pos + 1U;
    const std::int32_t  then = (next < c.end) ? c.data[next] : ((c.loop_length != 0U) ? c.data[c.loop_start] : now);
    const std::int32_t  value = (now * 256) + ((then - now) * static_cast<std::int32_t>(c.fraction >> 8U));
    c.fraction += c.step;
    c.pos      += c.fraction >> 16U;
    c.fraction &= 0xFFFFU;
    if (c.pos >= c.end) {
      if (c.loop_length != 0U) {
        c.pos = c.loop_start + ((c.pos - c.end) % c.loop_length);
      }
      else {
        c.data = nullptr;     // played to its end
      }
    }
    return (value * static_cast<std::int32_t>(c.volume)) / 64;
  }

  // the next tick; \return false when the song has ended
  bool next()
  {
    if (state_ != playing) {
      state_ = done;          // the fade has played
      return false;
    }
    if (tick_ >= speed_) {
      tick_ = 0U;
      if (!advance()) {
        state_ = fading;
        left_  = fade_;
        return left_ != 0U;
      }
    }
    for (std::size_t i = 0U; i < channels; ++i) {
      if (tick_ == 0U) {
        first_tick(voice_[i], song_.file + detail::pattern_data +
                       (static_cast<std::size_t>(song_.file[detail::order_table + order_]) * detail::pattern_size) +
                       (static_cast<std::size_t>(row_) * channels * 4U) + (i * 4U));
      }
      else {
        later_tick(voice_[i]);
      }
    }
    // the tick lasts 2.5 / tempo s; the fractions add up
    const std::uint32_t frames = rate_ * 5U;
    const std::uint32_t parts  = tempo_ * 2U;
    fraction_ += frames % parts;
    left_      = (frames / parts) + (fraction_ / parts);
    fraction_ %= parts;
    ++tick_;
    return true;
  }

  // to the next row after the last tick of a row; \return false when the song has ended
  bool advance()
  {
    std::uint32_t to_order = order_;
    std::uint32_t to_row   = row_ + 1U;
    const bool    moved = (flow_ != flow_none);
    switch (flow_) {
      case flow_stop :
        return false;
      case flow_jump :
        to_order = jump_;
        to_row   = 0U;
        break;
      case flow_break :
        to_order = order_ + 1U;
        to_row   = break_row_;
        break;
      case flow_both :
        to_order = jump_;
        to_row   = break_row_;
        break;
      default :
        if (to_row >= rows) {
          to_order = order_ + 1U;
          to_row   = 0U;
        }
        break;
    }
    flow_ = flow_none;
    bool lap = false;
    if (to_order >= song_.length) {
      to_order = 0U;
      lap      = true;
    }
    // a jump back to a position that has played: the song starts over
    if ((moved || (to_order != order_)) && ((played_[to_order / 8U] & (1U << (to_order % 8U))) != 0U)) {
      lap = true;
    }
    if (lap) {
      if ((loops_ != 0U) && (--loops_ == 0U)) {
        return false;
      }
      for (std::size_t i = 0U; i < 16U; ++i) {
        played_[i] = 0U;
      }
    }
    played_[to_order / 8U] = static_cast<std::uint8_t>(played_[to_order / 8U] | (1U << (to_order % 8U)));
    order_ = to_order;
    row_   = to_row;
    return true;
  }

  // start a note on a channel
  void start(channel& c, std::uint32_t period)
  {
    const sample s = sample_of(song_, c.number);
    c.period      = static_cast<std::uint16_t>(period);
    c.vibrato_pos = 0U;
    c.pos         = 0U;
    c.fraction    = 0U;
    c.data        = (s.length != 0U) ? s.data : nullptr;
    c.loop_start  = s.loop_start;
    c.loop_length = s.loop_length;
    c.end         = (s.loop_length != 0U) ? (s.loop_start + s.loop_length) : s.length;
    if (c.effect == 0x9U) {
      // sample offset; beyond the end: the loop, or nothing
      if (c.param != 0U) {
        c.offset = c.param;
      }
      c.pos = static_cast<std::uint32_t>(c.offset) * 256U;
      if (c.pos >= c.end) {
        c.pos  = c.loop_start;
        c.data = (c.loop_length != 0U) ? c.data : nullptr;
      }
    }
    pitch(c, c.period);
  }

  // the phase step of a channel for a period
  void pitch(channel& c, std::uint32_t period) const
  {
    c.step = (period != 0U) ? (scale_ / period) : 0U;
  }

  // the first tick of a row: notes, samples and the effects of that tick
  void first_tick(channel& c, const std::uint8_t* cell)
  {
    const std::uint32_t number = (cell[0] & 0xF0U) | (cell[2] >> 4U);
    const std::uint32_t period = ((cell[0] & 0x0FU) << 8U) | cell[1];
    c.effect = cell[2] & 0x0FU;
    c.param  = cell[3];
    const std::uint32_t x = c.param >> 4U;
    const std::uint32_t y = c.param & 0x0FU;

    if ((number != 0U) && (number <= 31U)) {
      // a sample number sets the volume and the finetune of the sample, for this note and the following
      const sample s = sample_of(song_, number);
      c.number   = static_cast<std::uint8_t>(number);
      c.volume   = s.volume;
      c.finetune = s.finetune;
    }
    c.delayed = 0U;
    if (period != 0U) {
      const std::uint32_t tuned = detail::tune(period, c.finetune);
      if ((c.effect == 0x3U) || (c.effect == 0x5U)) {
        c.target = static_cast<std::uint16_t>(tuned);     // tone portamento: slide there, no new note
      }
      else if ((c.effect == 0xEU) && (x == 0xDU) && (y != 0U)) {
        c.delayed = static_cast<std::uint16_t>(tuned);    // note delay
      }
      else {
        start(c, tuned);
      }
    }
    pitch(c, c.period);     // the arpeggio and the vibrato of the row before end

    switch (c.effect) {
      case 0x3U :
        c.porta = (c.param != 0U) ? c.param : c.porta;
        break;
      case 0x4U :
        c.vibrato = static_cast<std::uint8_t>(((x != 0U) ? (x << 4U) : (c.vibrato & 0xF0U)) |
                                              ((y != 0U) ? y : (c.vibrato & 0x0FU)));
        break;
      case 0xBU :
        jump_ = c.param;
        flow_ = ((flow_ == flow_break) || (flow_ == flow_both)) ? flow_both : flow_jump;
        break;
      case 0xCU :
        c.volume = static_cast<std::uint8_t>((c.param > 64U) ? 64U : c.param);
        break;
      case 0xDU :
        break_row_ = (x * 10U) + y;
        break_row_ = (break_row_ >= rows) ? 0U : break_row_;
        flow_ = ((flow_ == flow_jump) || (flow_ == flow_both)) ? flow_both : flow_break;
        break;
      case 0xEU :
        if (x == 0x1U) {
          slide(c, -static_cast<std::int32_t>(y));
        }
        else if (x == 0x2U) {
          slide(c, static_cast<std::int32_t>(y));
        }
        else if (x == 0xAU) {
          c.volume = static_cast<std::uint8_t>(((c.volume + y) > 64U) ? 64U : (c.volume + y));
        }
        else if (x == 0xBU) {
          c.volume = static_cast<std::uint8_t>((c.volume > y) ? (c.volume - y) : 0U);
        }
        else if ((x == 0xCU) && (y == 0U)) {
          c.volume = 0U;
        }
        break;
      case 0xFU :
        if (c.param == 0U) {
          flow_ = flow_stop;
        }
        else if (c.param < 32U) {
          speed_ = c.param;
        }
        else {
          tempo_ = c.param;
        }
        break;
      default :
        break;
    }
  }

  // the ticks after the first of a row: the effects that move
  void later_tick(channel& c)
  {
    const std::uint32_t x = c.param >> 4U;
    const std::uint32_t y = c.param & 0x0FU;
    switch (c.effect) {
      case 0x0U :
        if (c.param != 0U) {
          // arpeggio: the note, x and y semitones above it, one per tick
          const std::uint32_t up = ((tick_ % 3U) == 1U) ? x : (((tick_ % 3U) == 2U) ? y : 0U);
          pitch(c, (up == 0U) ? c.period : detail::period_of(detail::note_in(c.period, c.finetune) + up, c.finetune));
        }
        break;
      case 0x1U :
        slide(c, -static_cast<std::int32_t>(c.param));
        break;
      case 0x2U :
        slide(c, static_cast<std::int32_t>(c.param));
        break;
      case 0x3U :
        portamento(c);
        break;
      case 0x4U :
        vibrato(c);
        break;
      case 0x5U :
        portamento(c);
        volume_slide(c, x, y);
        break;
      case 0x6U :
        vibrato(c);
        volume_slide(c, x, y);
        break;
      case 0xAU :
        volume_slide(c, x, y);
        break;
      case 0xEU :
        if ((x == 0xCU) && (tick_ == y)) {
          c.volume = 0U;                    // note cut
        }
        else if ((x == 0xDU) && (tick_ == y) && (c.delayed != 0U)) {
          start(c, c.delayed);              // note delay
          c.delayed = 0U;
        }
        break;
      default :
        break;
    }
  }

  // portamento up (negative) or down, within the periods of ProTracker
  void slide(channel& c, std::int32_t by)
  {
    if (c.period == 0U) {
      return;
    }
    std::int32_t p = static_cast<std::int32_t>(c.period) + by;
    p = (p < static_cast<std::int32_t>(period_min)) ? static_cast<std::int32_t>(period_min)
      : ((p > static_cast<std::int32_t>(period_max)) ? static_cast<std::int32_t>(period_max) : p);
    c.period = static_cast<std::uint16_t>(p);
    pitch(c, c.period);
  }

  // tone portamento: the period slides to the target
  void portamento(channel& c)
  {
    if ((c.target == 0U) || (c.period == 0U)) {
      return;
    }
    if (c.period < c.target) {
      c.period = static_cast<std::uint16_t>(((c.target - c.period) > c.porta) ? (c.period + c.porta) : c.target);
    }
    else if (c.period > c.target) {
      c.period = static_cast<std::uint16_t>(((c.period - c.target) > c.porta) ? (c.period - c.porta) : c.target);
    }
    pitch(c, c.period);
  }

  // vibrato: a sine on the period, the note itself stays
  void vibrato(channel& c)
  {
    const std::int32_t sine  = vibrato_sine(c.vibrato_pos);
    const std::int32_t depth = static_cast<std::int32_t>(c.vibrato & 0x0FU);
    const std::int32_t p     = static_cast<std::int32_t>(c.period) + ((sine * depth) / 16448);    // +-2 * depth at the peak
    pitch(c, (p < 1) ? 1U : static_cast<std::uint32_t>(p));
    c.vibrato_pos = static_cast<std::uint8_t>((c.vibrato_pos + (c.vibrato >> 4U)) & 63U);
  }

  // the sine of the vibrato: 64 steps per period
  static std::int32_t vibrato_sine(std::uint32_t pos)
  {
    return sid::detail::sine((pos & 63U) << 26U);
  }

  // volume slide: up by x, else down by y
  static void volume_slide(channel& c, std::uint32_t x, std::uint32_t y)
  {
    if (x != 0U) {
      c.volume = static_cast<std::uint8_t>(((c.volume + x) > 64U) ? 64U : (c.volume + x));
    }
    else {
      c.volume = static_cast<std::uint8_t>((c.volume > y) ? (c.volume - y) : 0U);
    }
  }

  const info      song_;         // a copy: four words
  std::uint32_t   rate_;          // [Hz]
  std::uint32_t   scale_;         // clock * 65536 / rate: step per frame = scale / period
  std::uint32_t   loops_;         // rounds left, 0 = for ever
  std::int32_t    separation_;    // 0 .. 255
  std::uint32_t   left_;          // frames left of the tick (or of the fade)
  std::uint32_t   fraction_;      // carried to the next tick [1 / (2 * tempo) frame]
  std::uint32_t   fade_;          // [frames]
  std::uint32_t   order_;         // position that plays
  std::uint32_t   row_;
  std::uint32_t   tick_;          // tick of the row that comes next
  std::uint32_t   speed_;         // ticks per row
  std::uint32_t   tempo_;         // [BPM]: a tick is 2.5 / tempo s
  std::uint32_t   jump_;          // Bxx
  std::uint32_t   break_row_;     // Dxx
  state_type      state_;
  flow_type       flow_;
  channel         voice_[channels];
  std::uint8_t    played_[16];    // positions played in this round, one bit each
};

} // namespace mod


/**
 * Play a MOD file on an open output; returns when it is queued
 * \param out The output, open with the sample rate to play at
 * \param file The MOD file
 * \param size Size of the file [bytes]
 * \param loops Times the song plays (0 = for ever: never returns)
 * \return False if the output is not open, the file is no MOD file this player takes, or the device failed
 */
inline bool play_mod(output& out, const std::uint8_t* file, std::size_t size, std::uint32_t loops = 1U)
{
  mod::info tune;
  if ((out.rate() == 0U) || !mod::parse(file, size, tune)) {
    return false;
  }
  mod::player music(out.rate(), tune, loops);
  return out.play(music);
}

} // namespace sid

#endif  // _SID_EXT_MOD_H_
