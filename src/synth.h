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
// \brief sid - Sound Interface Driver library: the synthesizer
//        The sound of the home computers of the 80s: SID_VOICES voices (4), each one oscillator (sawtooth, pulse
//        with a variable width, triangle or noise, or several of them combined) with an envelope (attack, decay,
//        sustain, release), an LFO for vibrato and pulse width modulation, arpeggio, glide and pitch sweep, and
//        ring modulation or hard sync, by a second oscillator of the voice or by the voice before it. All voices
//        that ask for it go through one resonant filter (low, band, high pass or a sum of them) whose cutoff
//        sweeps with every note.
//        The sound of a note is a patch: a constant struct, so the instruments of a tune live in the flash.
//          note_on(voice, patch, key)  starts a note, key = 12 per octave, key::a4 = 69 = 440 Hz
//          note_off(voice)             the note goes into its release
//        The synth is a source that never ends: render() always fills all frames (silence when nothing plays).
//        A tune that ends is a sid::sequencer (sequencer.h), which owns a synth and plays a song on it.
//        Sawtooth and pulse are band limited (the edges are smoothed over two samples), so high notes stay clean
//        at the low sample rates of small outputs. The mix is clipped, not wrapped: the levels of the patches of
//        voices that sound together should add up to about 255. A pulse swings around its mean (no offset), so a
//        narrow one reaches higher than its level. Combined waveforms have plain edges.
//        Integer only. Envelope, LFO and pitch are computed once per ms, the oscillators and the filter per frame.
//        One task: note_on / note_off and render() are called from the task that plays.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_SYNTH_H_
#define _SID_SYNTH_H_

#include "tone.h"

// number of voices
#ifndef SID_VOICES
#define SID_VOICES              4U
#endif


namespace sid {

// waveform of the oscillator of a voice. Several can be combined with |: the voice then gives the AND of their
// values, as the SID chip did (triangle | pulse and sawtooth | pulse are the classic ones)
typedef enum tag_osc_type {
  osc_triangle = 1,   // triangle: soft, flutes and bass drums
  osc_saw      = 2,   // sawtooth: bright, brass and bass
  osc_pulse    = 4,   // pulse: hollow (width 128) to nasal (narrow), alive with pulse width modulation
  osc_noise    = 8    // noise, the key sets its colour: drums and effects
} osc_type;

// modulation of the oscillator of a voice, by its own second oscillator or by the voice before it (voice 0: the
// last voice). The voice before must play a note for that; one that shall not be heard plays a patch of level 0
typedef enum tag_mod_type {
  mod_none = 0,
  mod_ring,           // the voice is multiplied by a triangle of the second oscillator: bells, metal
  mod_sync,           // the second oscillator runs at the note and restarts the first, which runs higher: screaming leads
  mod_ring_voice,     // the voice is multiplied by the oscillator of the voice before: its waveform is the color
  mod_sync_voice      // the oscillator of the voice before restarts the one of this voice
} mod_type;

// way of a voice through the filter. Several can be combined with |: the sum of them, low | high is a notch
typedef enum tag_filter_type {
  filter_off  = 0,    // not filtered
  filter_low  = 1,    // low pass
  filter_band = 2,    // band pass
  filter_high = 4     // high pass
} filter_type;

// the sound of a note. All times are approximate, the decaying parts are exponential (98 % after the time)
typedef struct tag_patch {
  std::uint8_t    wave;         // osc_type, or several combined with |
  std::uint8_t    level;        // loudness: 0 .. 255 (full scale)
  std::uint16_t   attack;       // time to rise to full level [ms]
  std::uint16_t   decay;        // time to fall to the sustain level [ms]
  std::uint8_t    sustain;      // level while the note is held: 0 .. 255; 0: the note ends by itself (drums, plucks)
  std::uint16_t   release;      // time to fall to silence after note_off [ms]
  std::uint8_t    pulse;        // osc_pulse: width 8 .. 248 of 256 (also with pwm; beyond it is limited), 128 = square
  std::uint8_t    pwm;          // osc_pulse: depth of the width modulation by the LFO, 0 .. 127 of 256
  std::uint8_t    lfo_rate;     // rate of the LFO [0.1 Hz]
  std::uint8_t    lfo_delay;    // time until the vibrato sets in [10 ms]
  std::uint16_t   vibrato;      // depth of the pitch modulation by the LFO [cents], up to 2400
  std::int16_t    sweep;        // pitch slide while the note sounds [semitones / s], negative = down (drums)
  std::uint16_t   glide;        // time to slide from the last note of the voice to this one [ms], 0 = jump
  std::uint8_t    arp[2];       // arpeggio: semitones above the note of the second and third step
  std::uint8_t    arp_ms;       // duration of an arpeggio step [ms], 0 = no arpeggio
  std::uint8_t    mod;          // mod_type
  std::int8_t     mod_pitch;    // mod_ring: pitch of the second oscillator, mod_sync: of the first; [semitones] from the note
  std::uint8_t    filter;       // filter_type, or several combined with |
  std::uint8_t    resonance;    // emphasis at the cutoff: 0 (none) .. 255 (rings)
  std::uint16_t   cutoff;       // cutoff when the note starts [Hz]
  std::uint16_t   cutoff_end;   // cutoff the filter moves to [Hz]
  std::uint16_t   cutoff_ms;    // time of that move [ms], 0 = the cutoff stays
} patch;

// keys: 12 per octave, a4 = 69 = 440 Hz ('s' = sharp); 131 is the highest
namespace key {
const std::uint8_t c1 = 24U, cs1 = 25U, d1 = 26U, ds1 = 27U, e1 = 28U, f1 = 29U, fs1 = 30U, g1 = 31U, gs1 = 32U, a1 = 33U, as1 = 34U, b1 = 35U;
const std::uint8_t c2 = 36U, cs2 = 37U, d2 = 38U, ds2 = 39U, e2 = 40U, f2 = 41U, fs2 = 42U, g2 = 43U, gs2 = 44U, a2 = 45U, as2 = 46U, b2 = 47U;
const std::uint8_t c3 = 48U, cs3 = 49U, d3 = 50U, ds3 = 51U, e3 = 52U, f3 = 53U, fs3 = 54U, g3 = 55U, gs3 = 56U, a3 = 57U, as3 = 58U, b3 = 59U;
const std::uint8_t c4 = 60U, cs4 = 61U, d4 = 62U, ds4 = 63U, e4 = 64U, f4 = 65U, fs4 = 66U, g4 = 67U, gs4 = 68U, a4 = 69U, as4 = 70U, b4 = 71U;
const std::uint8_t c5 = 72U, cs5 = 73U, d5 = 74U, ds5 = 75U, e5 = 76U, f5 = 77U, fs5 = 78U, g5 = 79U, gs5 = 80U, a5 = 81U, as5 = 82U, b5 = 83U;
const std::uint8_t c6 = 84U, cs6 = 85U, d6 = 86U, ds6 = 87U, e6 = 88U, f6 = 89U, fs6 = 90U, g6 = 91U, gs6 = 92U, a6 = 93U, as6 = 94U, b6 = 95U;
const std::uint8_t c7 = 96U, cs7 = 97U, d7 = 98U, ds7 = 99U, e7 = 100U, f7 = 101U, fs7 = 102U, g7 = 103U, gs7 = 104U, a7 = 105U, as7 = 106U, b7 = 107U;
} // namespace key


namespace detail {

// triangle: one period per 2^32 of phase, from -32768 up to 32767 in the first half, down in the second
inline std::int32_t triangle(std::uint32_t phase)
{
  const std::int32_t ramp = static_cast<std::int32_t>((phase >> 15U) & 0xFFFFU);     // 0 .. 65535 per half
  return ((phase & 0x80000000UL) != 0U) ? (32767 - ramp) : (ramp - 32768);
}


/**
 * Band limited step: what a jump from -32768 to 32767 at phase 0 lacks in the sample before and the one after it,
 * so the edge is smooth over two samples and its harmonics above half the sample rate do not fold back
 * \param phase Phase of the oscillator
 * \param step Phase per frame
 * \return -32768 .. 32768, 0 away from the jump
 */
inline std::int32_t blep(std::uint32_t phase, std::uint32_t step)
{
  const std::uint32_t unit = step >> 12U;     // phase of 1 / 4096 frame
  if (unit == 0U) {
    return 0;
  }
  if (phase < step) {
    // the frame behind the jump
    std::int32_t t = static_cast<std::int32_t>(phase / unit);
    t = (t > 4096) ? 4096 : t;
    return ((2 * t) - ((t * t) / 4096) - 4096) * 8;
  }

  const std::uint32_t ahead = 0U - phase;
  if (ahead < step) {
    // the frame before the jump
    std::int32_t t = static_cast<std::int32_t>(ahead / unit);
    t = (t > 4096) ? 4096 : t;
    return (((4096 - t) * (4096 - t)) / 4096) * 8;
  }

  return 0;
}


/**
 * One step of an exponential move to a target
 * \param value Current value, up to 2^24
 * \param target Value to reach
 * \param rate Part of the distance per step: 0 .. 65536 (all of it)
 * \return The next value; it always moves, so the target is reached
 */
inline std::int32_t approach(std::int32_t value, std::int32_t target, std::int32_t rate)
{
  const std::int32_t distance = value - target;
  std::int32_t move = ((distance / 1024) * rate) / 64;
  if (move == 0) {
    move = (distance > 0) ? 1 : ((distance < 0) ? -1 : 0);
  }
  return value - move;
}


// rate for approach() which covers 63 / 64 of the distance in 'ms' steps of 1 ms
inline std::int32_t approach_rate(std::uint32_t ms)
{
  const std::uint32_t rate = (ms != 0U) ? (272560U / ms) : 65536U;     // 65536 * ln(64) / ms
  return static_cast<std::int32_t>((rate > 65536U) ? 65536U : rate);
}

} // namespace detail


// the synthesizer
class synth : public source
{
private:
  // part of the envelope a voice is in
  typedef enum tag_stage_type {
    stage_idle = 0,
    stage_attack,
    stage_decay,        // falls to the sustain level and stays there
    stage_release
  } stage_type;

  typedef struct tag_voice {
    const patch*    sound;          // patch of the note, nullptr: no note yet
    std::uint32_t   phase;          // oscillator: 2^32 = one period
    std::uint32_t   step;           // phase per frame
    std::uint32_t   phase2;         // second oscillator
    std::uint32_t   step2;
    std::uint32_t   pulse;          // phase at which the pulse falls
    std::uint32_t   random;         // noise generator
    std::int32_t    noise;          // current noise sample
    std::int32_t    offset;         // 256 times the mean of a combined waveform
    std::int32_t    out;            // sample of the oscillator in this frame, for the voice behind
    bool            wrapped;        // the oscillator has started a period in this frame
    std::int32_t    amp;            // gain of the frame: 2^27 = 1
    std::int32_t    amp_step;       // gain per frame, to the gain of the next ms
    std::int32_t    envelope;       // 0 .. envelope_full
    std::int32_t    attack_step;    // envelope per ms
    std::int32_t    decay_rate;     // approach rates
    std::int32_t    release_rate;
    std::int32_t    pitch;          // [1 / 256 semitone], on its way to the target
    std::int32_t    target;         // pitch of the note
    std::int32_t    slide;          // pitch sweep so far [1 / 65536 semitone]
    std::uint32_t   glide;          // [ms] left to the target
    std::uint32_t   age;            // [ms] since note_on
    std::uint32_t   lfo;            // phase of the LFO
    stage_type      stage;
    bool            active;         // the voice sounds
  } voice;

  voice           voice_[SID_VOICES];
  std::uint32_t   rate_;            // [Hz]
  std::uint32_t   tick_;            // frames per ms
  std::uint32_t   countdown_;       // frames to the next ms
  std::uint32_t   hz_scale_;        // 2^40 / rate: [1 / 256 Hz] to phase per frame (>> 16)
  std::uint32_t   angle_scale_;     // 2^30 / rate: [Hz] to the angle of the filter factor
  std::uint8_t    filter_;          // filter_type bits, filter_off until a note uses the filter
  std::int32_t    damping_;         // 1 / 32768
  std::int32_t    factor_;          // 1 / 32768
  std::int32_t    low_;             // state of the filter [1 / 64 sample]
  std::int32_t    band_;
  std::int32_t    cutoff_;          // [1 / 256 Hz]
  std::int32_t    cutoff_end_;
  std::int32_t    cutoff_rate_;

  static const std::uint32_t key_max       = 131U;
  static const std::int32_t  envelope_full = 1L << 24U;
  static const std::int32_t  envelope_end  = 1L << 14U;     // -60 dB: a falling note ends here
  static const std::int32_t  slide_max     = 127L * 65536L; // [1 / 65536 semitone]

public:
  /**
   * \param sample_rate Sample rate of the output [Hz]
   */
  explicit synth(std::uint32_t sample_rate)
    : rate_((sample_rate < 1000U) ? 1000U : sample_rate)
    , tick_(rate_ / 1000U)
    , countdown_(0U)
    , hz_scale_(static_cast<std::uint32_t>((static_cast<std::uint64_t>(1U) << 40U) / rate_))
    , angle_scale_((1UL << 30U) / rate_)
    , filter_(filter_off)
    , damping_(0)
    , factor_(0)
    , low_(0)
    , band_(0)
    , cutoff_(0)
    , cutoff_end_(0)
    , cutoff_rate_(0)
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      voice empty = { };
      voice_[i] = empty;
      voice_[i].random = 0x2545F491UL + static_cast<std::uint32_t>(i);
    }
  }

  
  /**
   * Start a note on a voice. A note that still sounds on the voice is taken over without a click
   * \param index Voice: 0 .. SID_VOICES - 1
   * \param sound The patch; it must stay valid while the note sounds
   * \param number Key: 12 per octave, key::a4 = 69 = 440 Hz
   */
  void note_on(std::size_t index, const patch& sound, std::uint32_t number)
  {
    if (index >= SID_VOICES) {
      return;
    }
    voice& v = voice_[index];
    const std::int32_t pitch = static_cast<std::int32_t>((number > key_max) ? static_cast<std::uint32_t>(key_max) : number) * 256;   // a value, so key_max needs no definition
    if ((sound.glide != 0U) && (v.sound != nullptr)) {
      v.glide = sound.glide;      // slide from the pitch the voice has
    }
    else {
      v.pitch = pitch;
      v.glide = 0U;
    }
    v.target       = pitch;
    v.sound        = &sound;
    v.stage        = stage_attack;
    v.active       = true;
    v.age          = 0U;
    v.lfo          = 0U;
    v.slide        = 0;
    v.attack_step  = envelope_full / static_cast<std::int32_t>((sound.attack != 0U) ? sound.attack : 1U);
    v.decay_rate   = detail::approach_rate(sound.decay);
    v.release_rate = detail::approach_rate(sound.release);
    if (sound.filter != filter_off) {
      // the filter is one for all voices: the last note that uses it sets it
      filter_      = sound.filter;
      damping_     = 46341 - (static_cast<std::int32_t>(sound.resonance) * 170);     // 1.41 (no emphasis) .. 0.09
      cutoff_      = static_cast<std::int32_t>(sound.cutoff) * 256;
      cutoff_end_  = (sound.cutoff_ms != 0U) ? (static_cast<std::int32_t>(sound.cutoff_end) * 256) : cutoff_;
      cutoff_rate_ = detail::approach_rate(sound.cutoff_ms);
    }
  }

  
  /**
   * End the note of a voice: it goes into its release
   * \param index Voice
   */
  void note_off(std::size_t index)
  {
    if (index < SID_VOICES) {
      voice_[index].stage = stage_release;      // a voice without a note stays silent
    }
  }

  
  // end the notes of all voices
  void all_off()
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      note_off(i);
    }
  }

  
  // true when no voice sounds
  bool silent() const
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      if (voice_[i].active) {
        return false;
      }
    }
    return true;
  }

  
  // sample rate the synth is made for [Hz]
  std::uint32_t rate() const
  { return rate_; }

  
  /**
   * Produce the next frames; the synth never ends
   * \return frames
   */
  virtual std::size_t render(std::int16_t* stereo, std::size_t frames)
  {
    for (std::size_t n = 0U; n < frames; ++n) {
      if (countdown_ == 0U) {
        control();
        countdown_ = tick_;
      }
      --countdown_;

      std::int32_t direct   = 0;
      std::int32_t filtered = 0;
      for (std::size_t i = 0U; i < SID_VOICES; ++i) {
        voice& v = voice_[i];
        if (v.active) {
          v.amp += v.amp_step;
          const std::int32_t sample = (oscillator(v, voice_[(i + SID_VOICES - 1U) % SID_VOICES]) * (v.amp / 8192)) / 16384;
          if (v.sound->filter != filter_off) {
            filtered += sample;
          }
          else {
            direct += sample;
          }
        }
      }
      if (filter_ != filter_off) {
        direct += filter(filtered);
      }
      direct = (direct > 32767) ? 32767 : ((direct < -32767) ? -32767 : direct);
      stereo[n * 2U]      = static_cast<std::int16_t>(direct);
      stereo[n * 2U + 1U] = static_cast<std::int16_t>(direct);
    }
    return frames;
  }


private:
  // a * b, a in 1 / 32768
  static std::int32_t scale(std::int32_t a, std::int32_t b)
  {
    return static_cast<std::int32_t>((static_cast<std::int64_t>(a) * b) / 32768);
  }


  // phase per frame of a pitch [1 / 256 semitone]
  std::uint32_t step_of(std::int32_t pitch) const
  {
    // frequencies of the keys 120 .. 132 [1 / 256 Hz]; the octaves below are halves
    static const std::int32_t top[13] = {
      2143237L, 2270680L, 2405702L, 2548752L, 2700309L, 2860878L, 3030994L,
      3211227L, 3402176L, 3604480L, 3818814L, 4045892L, 4286473L
    };
    const std::int32_t  highest  = (static_cast<std::int32_t>(key_max) * 256) + 255;
    const std::uint32_t p        = static_cast<std::uint32_t>((pitch < 0) ? 0 : ((pitch > highest) ? highest : pitch));
    const std::uint32_t number   = p >> 8U;
    const std::int32_t  fraction = static_cast<std::int32_t>(p & 0xFFU);
    const std::uint32_t semitone = number % 12U;
    const std::uint32_t hz = static_cast<std::uint32_t>(top[semitone] + (((top[semitone + 1U] - top[semitone]) * fraction) / 256))
                             >> (10U - (number / 12U));
    const std::uint64_t s  = (static_cast<std::uint64_t>(hz) * hz_scale_) >> 16U;
    return (s > 0x7FFFFFFFULL) ? 0x7FFFFFFFUL : static_cast<std::uint32_t>(s);     // up to half the sample rate
  }


  /**
   * The next sample of the oscillator of a voice
   * \param v The voice
   * \param ahead The voice before it (in this frame already, the last voice one frame late)
   * \return -32768 .. 32767, a narrow pulse and a combined waveform up to twice that
   */
  static std::int32_t oscillator(voice& v, const voice& ahead)
  {
    const patch& sound = *v.sound;
    const std::uint32_t before = v.phase;
    v.phase += v.step;
    v.wrapped = (v.phase < before);
    if ((sound.mod == mod_ring) || (sound.mod == mod_sync)) {
      const std::uint32_t before2 = v.phase2;
      v.phase2 += v.step2;
      if ((sound.mod == mod_sync) && (v.phase2 < before2) && (v.step2 != 0U)) {
        // the second oscillator has started a period within this frame: the first restarts with it
        v.phase   = static_cast<std::uint32_t>((static_cast<std::uint64_t>(v.phase2) * v.step) / v.step2);
        v.wrapped = true;
      }
    }
    else if ((sound.mod == mod_sync_voice) && ahead.wrapped && (ahead.step != 0U)) {
      // the same with the oscillator of the voice before
      v.phase   = static_cast<std::uint32_t>((static_cast<std::uint64_t>(ahead.phase) * v.step) / ahead.step);
      v.wrapped = true;
    }

    if ((sound.wave & osc_noise) != 0U) {
      // a new random value 16 times per period: the key is the colour of the noise
      if (((v.phase ^ before) & 0xF0000000UL) != 0U) {
        v.random ^= v.random << 13U;
        v.random ^= v.random >> 17U;
        v.random ^= v.random << 5U;
        v.noise   = static_cast<std::int32_t>(v.random >> 16U) - 32768;
      }
    }

    std::int32_t sample;
    switch (sound.wave) {
      case osc_pulse :
        // around its mean, so a narrow pulse is no offset: at a width of 1 / 4 it swings from -1 / 2 to 3 / 2
        sample = ((v.phase < v.pulse) ? 32767 : -32767) - (static_cast<std::int32_t>(v.pulse >> 16U) - 32768)
               + detail::blep(v.phase, v.step) - detail::blep(v.phase - v.pulse, v.step);
        break;
      case osc_triangle :
        sample = detail::triangle(v.phase);
        break;
      case osc_noise :
        sample = v.noise;
        break;
      case osc_saw :
        sample = (static_cast<std::int32_t>(v.phase >> 16U) - 32768) - detail::blep(v.phase, v.step);
        break;
      case 0U :
        sample = 0;
        break;
      default : {
        // several waveforms: the AND of their plain values, 0 .. 65535 each. Its mean depends on the waveforms
        // and the pulse width, so it is followed (256 frames) and taken off
        std::uint32_t bits = 0xFFFFU;
        if ((sound.wave & osc_triangle) != 0U) {
          bits &= static_cast<std::uint32_t>(detail::triangle(v.phase) + 32768);
        }
        if ((sound.wave & osc_saw) != 0U) {
          bits &= v.phase >> 16U;
        }
        if ((sound.wave & osc_pulse) != 0U) {
          bits &= (v.phase < v.pulse) ? 0xFFFFU : 0U;
        }
        if ((sound.wave & osc_noise) != 0U) {
          bits &= static_cast<std::uint32_t>(v.noise + 32768);
        }
        const std::int32_t value = static_cast<std::int32_t>(bits) - 32768;
        v.offset += value - (v.offset / 256);
        sample    = value - (v.offset / 256);
        break;
      }
    }
    // ring modulation; half the sample keeps the product in 32 bit
    if (sound.mod == mod_ring) {
      sample = ((sample / 2) * detail::triangle(v.phase2)) / 16384;
    }
    else if (sound.mod == mod_ring_voice) {
      const std::int32_t other = (ahead.out > 32767) ? 32767 : ((ahead.out < -32767) ? -32767 : ahead.out);
      sample = ((sample / 2) * other) / 16384;
    }
    v.out = sample;
    return sample;
  }


  // the filter (state variable, two rounds per frame so the cutoff can go up to a third of the sample rate).
  // It works in 1 / 64 of a sample, so it settles at silence and not a few steps beside it
  std::int32_t filter(std::int32_t in)
  {
    const std::int32_t limit = 1L << 28U;
    std::int32_t high = 0;
    std::int32_t band_before = 0;     // the band at the start of the two rounds, added
    in *= 64;
    for (std::uint32_t round = 0U; round < 2U; ++round) {
      band_before += band_;
      high  = in - low_ - scale(damping_, band_);
      band_ += scale(factor_, high);
      low_  += scale(factor_, band_);
      band_ = (band_ > limit) ? limit : ((band_ < -limit) ? -limit : band_);
      low_  = (low_  > limit) ? limit : ((low_  < -limit) ? -limit : low_);
    }
    std::int32_t out = 0;
    if ((filter_ & (filter_low | filter_high)) == (filter_low | filter_high)) {
      // low and high together are the input without the band: a notch. It is exact with the band a round has
      // started with (the sum of the two outputs has a newer one, which fills the notch), as the mean of both rounds
      out += in - scale(damping_, band_before / 2);
    }
    else if ((filter_ & filter_low) != 0U) {
      out += low_;
    }
    else if ((filter_ & filter_high) != 0U) {
      out += high;
    }
    if ((filter_ & filter_band) != 0U) {
      out += band_;
    }
    return out / 64;
  }


  // call once per ms: envelopes, LFOs, pitches, the cutoff
  void control()
  {
    for (std::size_t i = 0U; i < SID_VOICES; ++i) {
      voice& v = voice_[i];
      if (!v.active) {
        continue;
      }
      const patch& sound = *v.sound;

      // envelope
      const std::int32_t sustain = static_cast<std::int32_t>(sound.sustain) * 65793L;     // 255 = full
      if (v.stage == stage_attack) {
        v.envelope += v.attack_step;
        if (v.envelope >= envelope_full) {
          v.envelope = envelope_full;
          v.stage    = stage_decay;
        }
      }
      else if (v.stage == stage_decay) {
        v.envelope = detail::approach(v.envelope, sustain, v.decay_rate);
        if ((sustain == 0) && (v.envelope < envelope_end)) {
          v.stage = stage_idle;
        }
      }
      else if (v.stage == stage_release) {
        v.envelope = detail::approach(v.envelope, 0, v.release_rate);
        if (v.envelope < envelope_end) {
          v.stage = stage_idle;
        }
      }
      if (v.stage == stage_idle) {
        v.envelope = 0;
      }

      // the gain moves to that of the envelope within this ms
      const std::int32_t gain = (((v.envelope / 512) * static_cast<std::int32_t>(sound.level)) / 255) * 4096;
      v.amp_step = (gain - v.amp) / static_cast<std::int32_t>(tick_);
      if (v.amp_step == 0) {
        v.amp = gain;
        if (v.stage == stage_idle) {
          v.active  = false;    // faded out; its oscillator stands still and modulates nothing
          v.wrapped = false;
          v.out     = 0;
          continue;
        }
      }

      // LFO: a triangle that starts in its middle, rising
      const std::int32_t lfo = detail::triangle(v.lfo + 0x40000000UL);
      v.lfo += static_cast<std::uint32_t>(sound.lfo_rate) * 429497UL;     // 2^32 / 10000 per 0.1 Hz and ms

      // pitch: glide to the note, sweep, arpeggio, vibrato
      if (v.glide != 0U) {
        v.pitch += (v.target - v.pitch) / static_cast<std::int32_t>(v.glide);
        --v.glide;
      }
      else {
        v.pitch = v.target;
      }
      v.slide += (static_cast<std::int32_t>(sound.sweep) * 8192) / 125;     // 65536 / 1000 per semitone / s
      v.slide = (v.slide > slide_max) ? slide_max : ((v.slide < -slide_max) ? -slide_max : v.slide);
      std::int32_t pitch = v.pitch + (v.slide / 256);
      if (sound.arp_ms != 0U) {
        const std::uint32_t arp_step = (v.age / sound.arp_ms) % 3U;
        if (arp_step != 0U) {
          pitch += static_cast<std::int32_t>(sound.arp[arp_step - 1U]) * 256;
        }
      }
      std::int32_t vibrato = 0;
      if (v.age >= (static_cast<std::uint32_t>(sound.lfo_delay) * 10U)) {
        const std::int32_t depth = static_cast<std::int32_t>((sound.vibrato > 2400U) ? 2400U : sound.vibrato);
        vibrato = (lfo * depth) / 12800;      // cents * 256 / 100, the LFO is +-32768
      }
      const std::int32_t offset = static_cast<std::int32_t>(sound.mod_pitch) * 256;
      if (sound.mod == mod_sync) {
        v.step  = step_of(pitch + offset + vibrato);     // the vibrato moves the colour, the note stays
        v.step2 = step_of(pitch);
      }
      else {
        v.step  = step_of(pitch + vibrato);
        v.step2 = step_of(pitch + offset);
      }

      // pulse width
      std::int32_t width = (static_cast<std::int32_t>(sound.pulse) * 256) + ((lfo * static_cast<std::int32_t>(sound.pwm)) / 128);
      width   = (width < 0x0800) ? 0x0800 : ((width > 0xF800) ? 0xF800 : width);      // 1 / 32 .. 31 / 32
      v.pulse = static_cast<std::uint32_t>(width) << 16U;

      ++v.age;
    }

    // the filter: the cutoff moves to its end value, at most a third of the sample rate
    if (filter_ != filter_off) {
      cutoff_ = detail::approach(cutoff_, cutoff_end_, cutoff_rate_);
      std::uint32_t hz = static_cast<std::uint32_t>(cutoff_) >> 8U;
      hz = (hz < 20U) ? 20U : ((hz > (rate_ / 3U)) ? (rate_ / 3U) : hz);
      // 2 * sin(pi * hz / (2 * rate)): the angle is hz / (4 * rate) of a period
      factor_ = 2 * static_cast<std::int32_t>(detail::sine(hz * angle_scale_));
    }
  }
};

} // namespace sid

#endif  // _SID_SYNTH_H_
