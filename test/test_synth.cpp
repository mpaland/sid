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
// \brief sid tests: the synthesizer and the sequencer
//        The sound is measured, not compared with stored samples: pitch by the distance of the zero crossings,
//        loudness by the peak, colour by the power at single frequencies. Waveforms (single and combined),
//        envelope, LFO, arpeggio, glide and sweep, ring modulation and sync, the filter and its sums, the mix;
//        the sequencer by where its notes start and end and by the length of the song; the song of the examples
//        as a whole.
//
///////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <vector>

#include "sid/src/synth.h"
#include "sid/src/sequencer.h"

#include "test.h"
#include "memory_device.h"
#include "../examples/data/invention13.h"


namespace {

const std::uint32_t RATE = 22050U;

typedef std::vector<std::int16_t> sound;

// the next frames of a synth (left channel only), rendered in chunks of an odd size
sound take(sid::synth& s, std::uint32_t ms, std::size_t chunk = 37U)
{
  sound all;
  std::vector<std::int16_t> part(chunk * 2U);
  std::size_t frames = static_cast<std::size_t>((static_cast<std::uint64_t>(s.rate()) * ms) / 1000U);
  while (frames != 0U) {
    const std::size_t want = (frames < chunk) ? frames : chunk;
    const std::size_t n = s.render(&part[0], want);
    for (std::size_t i = 0U; i < n; ++i) {
      all.push_back(part[i * 2U]);
      if (part[i * 2U] != part[i * 2U + 1U]) {
        all.back() = 0x5555;     // both channels carry the same: a difference shows as a wrong sound
      }
    }
    frames -= want;
    if (n != want) {
      break;
    }
  }
  return all;
}

// all frames of a source that ends (left channel)
sound all_of(sid::source& src, std::size_t chunk = 37U, std::size_t limit = 4000000U)
{
  sound all;
  std::vector<std::int16_t> part(chunk * 2U);
  while (all.size() < limit) {
    const std::size_t n = src.render(&part[0], chunk);
    for (std::size_t i = 0U; i < n; ++i) {
      all.push_back(part[i * 2U]);
    }
    if (n < chunk) {
      break;
    }
  }
  return all;
}

// part of a sound
sound cut(const sound& s, std::uint32_t from_ms, std::uint32_t to_ms, std::uint32_t rate = RATE)
{
  const std::size_t a = static_cast<std::size_t>((static_cast<std::uint64_t>(rate) * from_ms) / 1000U);
  const std::size_t b = static_cast<std::size_t>((static_cast<std::uint64_t>(rate) * to_ms) / 1000U);
  return sound(s.begin() + static_cast<std::ptrdiff_t>((a < s.size()) ? a : s.size()),
               s.begin() + static_cast<std::ptrdiff_t>((b < s.size()) ? b : s.size()));
}

int peak(const sound& s)
{
  int p = 0;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    const int a = (s[i] < 0) ? -s[i] : s[i];
    p = (a > p) ? a : p;
  }
  return p;
}

// frequency [Hz] from the first to the last rising zero crossing, the crossings placed between the frames
double frequency(const sound& s, std::uint32_t rate = RATE)
{
  double first = 0.0;
  double last  = 0.0;
  std::size_t count = 0U;
  for (std::size_t i = 1U; i < s.size(); ++i) {
    if ((s[i - 1U] < 0) && (s[i] >= 0)) {
      const double at = static_cast<double>(i - 1U)
                      + (static_cast<double>(-s[i - 1U]) / static_cast<double>(s[i] - s[i - 1U]));
      if (count == 0U) {
        first = at;
      }
      last = at;
      ++count;
    }
  }
  return (count < 2U) ? 0.0 : ((static_cast<double>(count - 1U) * static_cast<double>(rate)) / (last - first));
}

// power of a sound at one frequency (mean square of that component)
double power(const sound& s, double hz, std::uint32_t rate = RATE)
{
  double re = 0.0;
  double im = 0.0;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    const double angle = 6.283185307179586 * hz * static_cast<double>(i) / static_cast<double>(rate);
    re += static_cast<double>(s[i]) * std::cos(angle);
    im += static_cast<double>(s[i]) * std::sin(angle);
  }
  const double n = static_cast<double>(s.empty() ? 1U : s.size());
  return ((re * re) + (im * im)) / (n * n);
}

// part of the frames in the upper half of the swing
double duty(const sound& s)
{
  int top    = 0;
  int bottom = 0;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    top    = (s[i] > top) ? s[i] : top;
    bottom = (s[i] < bottom) ? s[i] : bottom;
  }
  std::size_t high = 0U;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    high += (s[i] > ((top + bottom) / 2)) ? 1U : 0U;
  }
  return s.empty() ? 0.0 : (static_cast<double>(high) / static_cast<double>(s.size()));
}

bool near(double value, double expected, double tolerance)
{
  return std::fabs(value - expected) <= tolerance;
}

// a plain patch: full level at once, held, a short release
sid::patch plain(sid::osc_type wave)
{
  sid::patch p = { };
  p.wave    = static_cast<std::uint8_t>(wave);
  p.level   = 255U;
  p.sustain = 255U;
  p.release = 10U;
  p.pulse   = 128U;
  return p;
}

// the sound of one held note
sound note(const sid::patch& p, std::uint32_t key, std::uint32_t ms, std::uint32_t rate = RATE)
{
  sid::synth s(rate);
  s.note_on(0U, p, key);
  return take(s, ms);
}

} // namespace


TEST_CASE(sid_synth_helpers)
{
  // the band limited step: nothing away from the jump, half the jump at it, smooth to both sides
  const std::uint32_t step = 0x04000000UL;      // 64 frames per period
  CHECK_EQ(sid::detail::blep(0x40000000UL, step), 0);
  CHECK_EQ(sid::detail::blep(step, step), 0);
  CHECK_EQ(sid::detail::blep(0U - step, step), 0);
  CHECK_EQ(sid::detail::blep(0U, step), -32768);
  CHECK(near(sid::detail::blep(0xFFFFFFFFUL, step), 32768.0, 16.0));
  CHECK(near(sid::detail::blep(step / 2U, step), -8192.0, 16.0));
  CHECK(near(sid::detail::blep(0U - (step / 2U), step), 8192.0, 16.0));
  CHECK_EQ(sid::detail::blep(0U, 100U), 0);     // no pitch: no step

  // the exponential move: 63 / 64 of the way in the time, it arrives, it does not overshoot
  CHECK_EQ(sid::detail::approach_rate(0U), 65536);
  CHECK_EQ(sid::detail::approach_rate(1U), 65536);
  const std::uint32_t times[] = { 50U, 400U, 3000U };
  for (std::size_t t = 0U; t < 3U; ++t) {
    const std::int32_t rate = sid::detail::approach_rate(times[t]);
    std::int32_t down = 1L << 24U;
    std::int32_t up   = 0;
    for (std::uint32_t ms = 0U; ms < times[t]; ++ms) {
      down = sid::detail::approach(down, 0, rate);
      up   = sid::detail::approach(up, 1L << 24U, rate);
    }
    CHECK_MSG((down > ((1L << 24U) / 90)) && (down < ((1L << 24U) / 50)), "about 1 / 64 is left");
    CHECK(near(static_cast<double>(up + down), 16777216.0, 4096.0));
  }
  std::int32_t value = 5000;
  for (std::uint32_t i = 0U; i < 6000U; ++i) {
    const std::int32_t next = sid::detail::approach(value, 0, 1);
    CHECK(next <= value);
    value = next;
  }
  CHECK_EQ(value, 0);
  CHECK_EQ(sid::detail::approach(-7, 3, 65536), -6);     // a short distance: one unit per step
  CHECK_EQ(sid::detail::approach(3, 3, 65536), 3);
}


TEST_CASE(sid_synth_pitch)
{
  // equal temperament, a4 = 440 Hz, for every waveform with a pitch and at more than one sample rate
  const sid::osc_type waves[] = { sid::osc_saw, sid::osc_pulse, sid::osc_triangle };
  for (std::size_t w = 0U; w < 3U; ++w) {
    const sid::patch p = plain(waves[w]);
    CHECK(near(frequency(note(p, sid::key::a4, 1000U)), 440.0, 0.3));
    CHECK(near(frequency(note(p, sid::key::a5, 1000U)), 880.0, 0.6));
    CHECK(near(frequency(note(p, sid::key::c4, 1000U)), 261.63, 0.3));
    CHECK(near(frequency(note(p, sid::key::e2, 1000U)), 82.41, 0.2));
    CHECK(near(frequency(note(p, sid::key::a4, 1000U, 48000U), 48000U), 440.0, 0.3));
    CHECK(near(frequency(note(p, sid::key::a4, 1000U, 8000U), 8000U), 440.0, 0.3));
    CHECK(near(static_cast<double>(peak(note(p, sid::key::a4, 200U))), 32767.0, 1500.0));      // a sawtooth loses its tip to the smooth edge
  }

  // the synth never ends and is silent without a note; a voice or a key that does not exist does no harm
  sid::synth s(RATE);
  CHECK_EQ(s.rate(), RATE);
  CHECK(s.silent());
  std::int16_t frames[20] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  CHECK_EQ(s.render(frames, 10U), 10U);
  CHECK_EQ(peak(sound(frames, frames + 20)), 0);
  const sid::patch p = plain(sid::osc_triangle);
  s.note_on(SID_VOICES, p, sid::key::a4);
  s.note_off(SID_VOICES);
  CHECK(s.silent());
  s.note_on(0U, p, 250U);
  CHECK(!s.silent());
  CHECK(frequency(take(s, 200U)) > 10000.0);      // the highest key (b9 = 15804 Hz), aliased or not: no silence
  CHECK_EQ(sid::synth(0U).rate(), 1000U);         // no sample rate: no division by zero
}


TEST_CASE(sid_synth_envelope)
{
  sid::patch p = plain(sid::osc_pulse);
  p.attack  = 100U;
  p.decay   = 200U;
  p.sustain = 128U;
  p.release = 300U;
  sid::synth s(RATE);
  s.note_on(0U, p, sid::key::a4);
  const sound held = take(s, 1500U);
  CHECK(peak(cut(held, 0U, 10U)) < 4000);                                    // rises from silence
  CHECK(near(static_cast<double>(peak(cut(held, 45U, 55U))), 16384.0, 2500.0));     // linear: half way
  CHECK(peak(cut(held, 95U, 110U)) > 32000);                                 // full after the attack
  const double level = 32767.0 * 128.0 / 255.0;
  CHECK(peak(cut(held, 140U, 150U)) > 22000);                                // falls, not at once
  CHECK(near(static_cast<double>(peak(cut(held, 300U, 310U))), level + (16300.0 / 64.0), 250.0));    // 63 / 64 of the way
  CHECK(near(static_cast<double>(peak(cut(held, 1400U, 1500U))), level, 100.0));   // stays at the sustain level
  CHECK(!s.silent());

  s.note_off(0U);
  const sound released = take(s, 1500U);
  CHECK(peak(cut(released, 0U, 10U)) > 15000);
  CHECK(near(static_cast<double>(peak(cut(released, 300U, 310U))), level / 64.0, 120.0));
  CHECK(s.silent());
  CHECK_EQ(peak(cut(released, 1000U, 1500U)), 0);
  s.note_off(0U);       // no note: nothing
  CHECK(s.silent());

  // no times: full level within the first ms, gone within a few ms after the note
  sid::patch quick = plain(sid::osc_pulse);
  quick.release = 0U;
  sid::synth q(RATE);
  q.note_on(0U, quick, sid::key::a4);
  CHECK(peak(take(q, 5U)) > 32000);
  q.note_off(0U);
  (void)take(q, 5U);
  CHECK(q.silent());

  // sustain 0: the note ends by itself
  sid::patch pluck = plain(sid::osc_triangle);
  pluck.decay   = 50U;
  pluck.sustain = 0U;
  sid::synth d(RATE);
  d.note_on(0U, pluck, sid::key::a4);
  const sound plucked = take(d, 400U);
  CHECK(peak(cut(plucked, 0U, 10U)) > 25000);
  CHECK(peak(cut(plucked, 50U, 60U)) < 1200);
  CHECK(d.silent());
  CHECK_EQ(peak(cut(plucked, 300U, 400U)), 0);

  // the level of the patch scales the sound; level 0 is silence
  sid::patch soft = plain(sid::osc_pulse);
  soft.level = 128U;
  CHECK(near(static_cast<double>(peak(note(soft, sid::key::a4, 200U))), level, 400.0));
  soft.level = 0U;
  CHECK_EQ(peak(note(soft, sid::key::a4, 200U)), 0);

  // a new note on a sounding voice takes the sound over: no jump in a triangle (2620 per frame at a4)
  sid::patch smooth = plain(sid::osc_triangle);
  smooth.attack = 20U;
  sid::synth r(RATE);
  r.note_on(0U, smooth, sid::key::a4);
  sound twice = take(r, 100U);
  r.note_on(0U, smooth, sid::key::a4);
  const sound again = take(r, 100U);
  twice.insert(twice.end(), again.begin(), again.end());
  int jump = 0;
  for (std::size_t i = 1U; i < twice.size(); ++i) {
    const int d1 = twice[i] - twice[i - 1U];
    jump = ((d1 < 0 ? -d1 : d1) > jump) ? (d1 < 0 ? -d1 : d1) : jump;
  }
  CHECK(jump < 2800);
  CHECK(peak(cut(twice, 100U, 105U)) > 30000);      // and no new attack from silence
}


TEST_CASE(sid_synth_pulse_and_noise)
{
  // pulse width
  sid::patch p = plain(sid::osc_pulse);
  p.level = 100U;
  CHECK(near(duty(note(p, sid::key::a3, 1000U)), 0.5, 0.01));
  p.pulse = 64U;
  CHECK(near(duty(note(p, sid::key::a3, 1000U)), 0.25, 0.01));
  p.pulse = 224U;
  CHECK(near(duty(note(p, sid::key::a3, 1000U)), 0.875, 0.01));
  p.pulse = 0U;
  CHECK(near(duty(note(p, sid::key::a3, 1000U)), 1.0 / 32.0, 0.01));      // never closed
  // a pulse swings around its mean: no offset at any width, a narrow one reaches higher (1 / 4: 3 / 2 up, 1 / 2 down)
  p.pulse = 64U;
  p.level = 128U;
  const sound narrow = note(p, sid::key::a4, 1000U);
  double offset = 0.0;
  int lowest = 0;
  for (std::size_t i = 0U; i < narrow.size(); ++i) {
    offset += static_cast<double>(narrow[i]);
    lowest = (narrow[i] < lowest) ? narrow[i] : lowest;
  }
  CHECK(std::fabs(offset / static_cast<double>(narrow.size())) < 60.0);
  CHECK(near(static_cast<double>(peak(narrow)), 1.5 * 16448.0, 300.0));
  CHECK(near(static_cast<double>(lowest), -0.5 * 16448.0, 300.0));
  p.level = 100U;

  // pulse width modulation: the LFO (1 Hz) starts in its middle and rises
  p.pulse    = 128U;
  p.pwm      = 64U;
  p.lfo_rate = 10U;
  const sound moving = note(p, sid::key::a4, 1000U);      // windows of 25 ms: 11 periods
  CHECK(near(duty(cut(moving, 0U, 25U)), 0.5, 0.03));
  CHECK(near(duty(cut(moving, 238U, 263U)), 0.75, 0.03));
  CHECK(near(duty(cut(moving, 488U, 513U)), 0.5, 0.03));
  CHECK(near(duty(cut(moving, 738U, 763U)), 0.25, 0.03));
  CHECK(near(frequency(moving), 440.0, 0.3));       // the width moves, the pitch does not

  // noise: no pitch, around zero, loud; the key is its colour (16 new values per period)
  const sid::patch n = plain(sid::osc_noise);
  const sound low  = note(n, sid::key::c3, 1000U);
  const sound high = note(n, sid::key::c7, 1000U);
  std::size_t changes_low  = 0U;
  std::size_t changes_high = 0U;
  double mean = 0.0;
  for (std::size_t i = 1U; i < low.size(); ++i) {
    changes_low  += (low[i] != low[i - 1U]) ? 1U : 0U;
    changes_high += (high[i] != high[i - 1U]) ? 1U : 0U;
    mean += static_cast<double>(high[i]);
  }
  CHECK(near(static_cast<double>(changes_low), 130.81 * 16.0, 30.0));
  CHECK(changes_high > 21900U);
  CHECK(std::fabs(mean / static_cast<double>(high.size())) < 500.0);
  CHECK(peak(high) > 32000);
  CHECK(power(high, 1000.0) < (power(note(plain(sid::osc_triangle), sid::key::c6, 1000U), 1046.5) / 100.0));

  // the same notes give the same noise; another voice has its own
  CHECK(note(n, sid::key::c5, 100U) == note(n, sid::key::c5, 100U));
  sid::synth s(RATE);
  s.note_on(1U, n, sid::key::c5);
  CHECK(take(s, 100U) != note(n, sid::key::c5, 100U));
}


TEST_CASE(sid_synth_pitch_moves)
{
  // arpeggio: the note, 4 and 7 semitones above it, 100 ms each, round and round
  sid::patch p = plain(sid::osc_triangle);
  p.arp[0] = 4U;
  p.arp[1] = 7U;
  p.arp_ms = 100U;
  const sound chord = note(p, sid::key::a4, 450U);
  CHECK(near(frequency(cut(chord, 5U, 95U)), 440.0, 1.0));
  CHECK(near(frequency(cut(chord, 105U, 195U)), 554.37, 1.2));
  CHECK(near(frequency(cut(chord, 205U, 295U)), 659.26, 1.5));
  CHECK(near(frequency(cut(chord, 305U, 395U)), 440.0, 1.0));

  // vibrato: 2 Hz, 400 cents to both sides, the LFO starts in its middle and rises
  sid::patch v = plain(sid::osc_triangle);
  v.lfo_rate = 20U;
  v.vibrato  = 400U;
  const sound wobble = note(v, sid::key::a4, 500U);
  CHECK(near(frequency(cut(wobble, 0U, 12U)), 440.0, 12.0));
  CHECK(near(frequency(cut(wobble, 115U, 135U)), 554.37, 10.0));      // 4 semitones up at the peak
  CHECK(near(frequency(cut(wobble, 365U, 385U)), 349.23, 8.0));       // 4 down
  // delayed: a straight note for 300 ms
  v.lfo_delay = 30U;
  const sound late = note(v, sid::key::a4, 500U);
  CHECK(near(frequency(cut(late, 0U, 295U)), 440.0, 0.5));
  CHECK(near(frequency(cut(late, 365U, 385U)), 349.23, 8.0));
  // a depth above two octaves is two octaves
  v.lfo_delay = 0U;
  v.vibrato   = 60000U;
  const double top = frequency(cut(note(v, sid::key::a4, 200U), 120U, 130U));
  CHECK((top > 1550.0) && (top < 1770.0));

  // sweep: an octave per second down, and up
  sid::patch d = plain(sid::osc_triangle);
  d.sweep = -12;
  const sound falling = note(d, sid::key::a4, 1100U);
  CHECK(near(frequency(cut(falling, 480U, 520U)), 311.13, 3.0));
  CHECK(near(frequency(cut(falling, 980U, 1020U)), 220.0, 2.0));
  d.sweep = 12;
  CHECK(near(frequency(cut(note(d, sid::key::a4, 1100U), 980U, 1020U)), 880.0, 8.0));
  // a fast sweep ends at the lowest pitch and stays quiet there (no wrap to the top)
  d.sweep = -2000;
  const sound dropped = note(d, sid::key::a4, 400U);
  CHECK(frequency(cut(dropped, 200U, 400U)) < 20.0);
  d.sweep = -32768;
  CHECK(frequency(cut(note(d, sid::key::a4, 2500U), 500U, 2500U)) < 20.0);      // and stays there

  // glide: the first note of a voice jumps, the next one slides for 200 ms
  sid::patch g = plain(sid::osc_triangle);
  g.glide = 200U;
  sid::synth s(RATE);
  s.note_on(0U, g, sid::key::a4);
  CHECK(near(frequency(take(s, 300U)), 440.0, 0.5));
  s.note_on(0U, g, sid::key::a5);
  const sound slide = take(s, 400U);
  CHECK(near(frequency(cut(slide, 90U, 110U)), 622.25, 12.0));        // half way: 6 semitones
  CHECK(near(frequency(cut(slide, 210U, 400U)), 880.0, 0.6));
}


TEST_CASE(sid_synth_modulation)
{
  // ring modulation: a4 times a triangle a fifth above gives sum and difference, not the note
  sid::patch ring = plain(sid::osc_triangle);
  ring.mod       = sid::mod_ring;
  ring.mod_pitch = 7;
  const sound bell = note(ring, sid::key::a4, 1000U);
  const double sum_and_difference = power(bell, 659.26 - 440.0) + power(bell, 659.26 + 440.0);
  CHECK(sum_and_difference > (power(bell, 440.0) * 100.0));
  CHECK(sum_and_difference > 1.0e7);
  CHECK(peak(bell) <= 32767);

  // sync: the oscillator runs a fifth higher but restarts with the note, so the sound repeats with the note:
  // at 44 kHz every 100 frames
  sid::patch sync = plain(sid::osc_saw);
  sync.mod       = sid::mod_sync;
  sync.mod_pitch = 7;
  const sound hard = note(sync, sid::key::a4, 500U, 44000U);
  const sound free = note(plain(sid::osc_saw), sid::key::e5, 500U, 44000U);
  double repeat_hard = 0.0;
  double repeat_free = 0.0;
  double size_hard   = 0.0;
  for (std::size_t i = 2000U; i < 20000U; ++i) {
    repeat_hard += std::fabs(static_cast<double>(hard[i]) - static_cast<double>(hard[i + 100U]));
    repeat_free += std::fabs(static_cast<double>(free[i]) - static_cast<double>(free[i + 100U]));
    size_hard   += std::fabs(static_cast<double>(hard[i]));
  }
  CHECK(size_hard > (18000.0 * 5000.0));
  CHECK(repeat_hard < (size_hard / 100.0));
  CHECK(repeat_free > (size_hard / 4.0));
  // and it is not the plain note: the fifth sounds in it
  const sound plain_note = note(plain(sid::osc_saw), sid::key::a4, 500U, 44000U);
  CHECK(power(hard, 1320.0, 44000U) > (power(plain_note, 1320.0, 44000U) * 2.0));

  // sync with vibrato: the colour moves slowly, the note stays (the sound still nearly repeats with it)
  sync.lfo_rate = 20U;
  sync.vibrato  = 700U;
  const sound sweep = note(sync, sid::key::a4, 500U, 44000U);
  double repeat_sweep = 0.0;
  double far_apart    = 0.0;
  for (std::size_t i = 2000U; i < 15000U; ++i) {
    repeat_sweep += std::fabs(static_cast<double>(sweep[i]) - static_cast<double>(sweep[i + 100U]));
    far_apart    += std::fabs(static_cast<double>(sweep[i]) - static_cast<double>(sweep[i + 5500U]));     // 1 / 4 LFO period
  }
  CHECK(repeat_sweep < (far_apart / 4.0));
  CHECK(far_apart > (13000.0 * 3000.0));
}


TEST_CASE(sid_synth_filter)
{
  // a low sawtooth (110 Hz) with all its harmonics
  sid::patch open = plain(sid::osc_saw);
  open.level = 64U;      // room for the resonance
  const sound raw = note(open, sid::key::a2, 1000U);
  const double f1  = power(raw, 110.0);
  const double f10 = power(raw, 1100.0);
  const double f20 = power(raw, 2200.0);
  const double f40 = power(raw, 4400.0);
  CHECK(f1 > (f20 * 100.0));

  // low pass at 300 Hz: the note stays, the 20th harmonic goes (12 dB per octave: 1 / 54 of the amplitude)
  sid::patch low = open;
  low.filter = sid::filter_low;
  low.cutoff = 300U;
  const sound dull = cut(note(low, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(near(power(dull, 110.0) / f1, 1.0, 0.25));
  CHECK(power(dull, 2200.0) < (f20 / 1000.0));

  // high pass at 2 kHz: the note goes (by more than 30 dB; the simple structure of the filter does not go much
  // deeper), the high harmonics stay
  sid::patch high = open;
  high.filter = sid::filter_high;
  high.cutoff = 2000U;
  const sound thin = cut(note(high, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(power(thin, 110.0) < (f1 / 1000.0));
  CHECK(near(power(thin, 4400.0) / f40, 1.0, 0.3));

  // band pass at 1100 Hz: the 10th harmonic stays, both ends go
  sid::patch band = open;
  band.filter = sid::filter_band;
  band.cutoff = 1100U;
  const sound nasal = cut(note(band, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(power(nasal, 110.0) < (f1 / 20.0));
  CHECK(power(nasal, 4400.0) < (f40 / 4.0));
  CHECK(power(nasal, 1100.0) > (f10 / 4.0));

  // resonance: the harmonic at the cutoff stands out, more with more resonance, and the filter stays stable
  low.cutoff    = 1100U;
  const double flat = power(cut(note(low, sid::key::a2, 1100U), 100U, 1100U), 1100.0);
  low.resonance = 128U;
  const double some = power(cut(note(low, sid::key::a2, 1100U), 100U, 1100U), 1100.0);
  low.resonance = 255U;
  const sound ringing = cut(note(low, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(near(flat / f10, 0.5, 0.15));           // -3 dB at the cutoff without resonance
  CHECK(some > (flat * 2.5));
  CHECK(power(ringing, 1100.0) > (some * 6.0));
  CHECK(near(power(ringing, 110.0) / f1, 1.0, 0.25));

  // the cutoff sweeps from its start to its end value in the time of the patch
  sid::patch wow = open;
  wow.filter     = sid::filter_low;
  wow.cutoff     = 4000U;
  wow.cutoff_end = 200U;
  wow.cutoff_ms  = 200U;
  const sound closing = note(wow, sid::key::a2, 1000U);
  const double bright = power(cut(closing, 0U, 20U), 2200.0);
  const double dark   = power(cut(closing, 500U, 600U), 2200.0);
  CHECK(bright > (f20 / 4.0));
  CHECK(dark < (bright / 300.0));
  // without a time the cutoff stays where it starts
  wow.cutoff_ms = 0U;
  CHECK(power(cut(note(wow, sid::key::a2, 600U), 500U, 600U), 2200.0) > (f20 / 2.0));

  // a cutoff above a third of the sample rate is that third, a cutoff of 0 is 20 Hz: the filter stays sane
  sid::patch wide = open;
  wide.filter    = sid::filter_low;
  wide.cutoff    = 60000U;
  wide.resonance = 255U;
  const sound all = cut(note(wide, sid::key::a2, 600U), 100U, 600U);
  CHECK(near(power(all, 110.0) / f1, 1.0, 0.1));
  CHECK(near(power(all, 2200.0) / f20, 1.0, 0.35));
  wide.cutoff = 0U;
  CHECK(power(cut(note(wide, sid::key::a2, 600U), 100U, 600U), 110.0) < (f1 / 100.0));

  // after the note the filter settles at silence
  sid::patch last = open;
  last.filter    = sid::filter_low;
  last.cutoff    = 350U;
  last.resonance = 170U;
  last.release   = 20U;
  sid::synth e(RATE);
  e.note_on(0U, last, sid::key::a2);
  (void)take(e, 300U);
  e.note_off(0U);
  const sound after = take(e, 1000U);
  CHECK(e.silent());
  CHECK_EQ(peak(cut(after, 500U, 1000U)), 0);

  // one filter for all voices: a voice that does not ask for it is not filtered, and the sums add up
  sid::patch flute = plain(sid::osc_triangle);
  flute.level = 100U;
  low.level     = 100U;
  low.cutoff    = 250U;
  low.resonance = 0U;
  sid::synth s(RATE);
  s.note_on(0U, low, sid::key::a2);
  s.note_on(1U, flute, sid::key::a6);
  const sound both = cut(take(s, 1100U), 100U, 1100U);
  const sound alone = note(flute, sid::key::a6, 1000U);
  CHECK(near(power(both, 1760.0) / power(alone, 1760.0), 1.0, 0.05));
  CHECK(power(both, 2200.0) < (f20 / 1000.0));
  CHECK(power(both, 110.0) > (f1 / 10.0));
}


TEST_CASE(sid_synth_band_limit_and_mix)
{
  // a high sawtooth (b7, 3951 Hz) at 22 kHz: its 5th harmonic folds back to 2295 Hz, the 6th to 1656 Hz. With
  // plain edges those lie 14 and 16 dB below the note; the smoothed edges put them far lower
  const sound high = note(plain(sid::osc_saw), sid::key::b7, 1000U);
  const double tone = power(high, 3951.07);
  CHECK(tone > 1.0e7);
  CHECK(power(high, 22050.0 - (5.0 * 3951.07)) < (tone / 1000.0));
  CHECK(power(high, (6.0 * 3951.07) - 22050.0) < (tone / 1000.0));
  // the same for both edges of a pulse (3rd harmonic of a 4186 Hz square would fold to 9492 Hz ... the 5th to 1120)
  const sound square = note(plain(sid::osc_pulse), 108U, 1000U);      // c8
  CHECK(power(square, 22050.0 - (5.0 * 4186.01)) < (power(square, 4186.01) / 1000.0));
  // low notes keep their harmonics: the 10th of a sawtooth is 1 / 10 of the note in amplitude
  const sound low = note(plain(sid::osc_saw), sid::key::a2, 1000U);
  CHECK(near(power(low, 1100.0) / power(low, 110.0), 0.01, 0.002));

  // the mix clips, it does not wrap: four full squares in step are one full square
  const sid::patch p = plain(sid::osc_pulse);
  sid::synth s(RATE);
  for (std::size_t i = 0U; i < SID_VOICES; ++i) {
    s.note_on(i, p, sid::key::a3);
  }
  const sound loud = take(s, 200U);
  const sound one  = note(p, sid::key::a3, 200U);
  REQUIRE(loud.size() == one.size());
  bool same_side = true;
  for (std::size_t i = 100U; i < loud.size(); ++i) {
    if ((one[i] > 9000) && (loud[i] != 32767)) {
      same_side = false;
    }
    if ((one[i] < -9000) && (loud[i] != -32767)) {
      same_side = false;
    }
  }
  CHECK(same_side);

  // voices add up: two notes of level 100 are both in the sound
  sid::patch soft = plain(sid::osc_triangle);
  soft.level = 100U;
  sid::synth two(RATE);
  two.note_on(0U, soft, sid::key::a4);
  two.note_on(3U, soft, sid::key::e5);
  const sound pair = take(two, 1000U);
  CHECK(near(power(pair, 440.0) / power(note(soft, sid::key::a4, 1000U), 440.0), 1.0, 0.05));
  CHECK(near(power(pair, 659.26) / power(note(soft, sid::key::e5, 1000U), 659.26), 1.0, 0.05));
  two.all_off();
  (void)take(two, 200U);
  CHECK(two.silent());

  // the size of the chunks the sound is asked for in does not change it
  sid::patch rich = plain(sid::osc_pulse);
  rich.attack = 30U;  rich.decay = 100U;  rich.sustain = 150U;  rich.pwm = 60U;  rich.lfo_rate = 50U;
  rich.vibrato = 30U;  rich.arp[0] = 3U;  rich.arp[1] = 7U;  rich.arp_ms = 20U;
  rich.filter = sid::filter_low;  rich.resonance = 200U;  rich.cutoff = 3000U;  rich.cutoff_end = 400U;  rich.cutoff_ms = 150U;
  const sid::patch hiss = plain(sid::osc_noise);       // the synth keeps a pointer to the patch: no temporary
  sound parts[3];
  const std::size_t chunks[3] = { 1U, 37U, 4096U };
  for (std::size_t c = 0U; c < 3U; ++c) {
    sid::synth r(RATE);
    r.note_on(0U, rich, sid::key::a3);
    r.note_on(1U, hiss, sid::key::c5);
    parts[c] = take(r, 300U, chunks[c]);
  }
  CHECK(parts[0] == parts[1]);
  CHECK(parts[0] == parts[2]);
  CHECK(peak(parts[0]) > 10000);
}


TEST_CASE(sid_synth_combined_waveforms)
{
  // the waveforms and the filter modes are single bits, so they combine
  CHECK_EQ(sid::osc_triangle | sid::osc_saw | sid::osc_pulse | sid::osc_noise, 15);
  CHECK_EQ(sid::filter_low | sid::filter_band | sid::filter_high, 7);
  CHECK_EQ(static_cast<int>(sid::filter_off), 0);

  // no waveform: silence
  sid::patch none = plain(sid::osc_triangle);
  none.wave = 0U;
  CHECK_EQ(peak(note(none, sid::key::a3, 100U)), 0);

  // triangle and square: the rising half of the triangle, then nothing. The pitch stays, no offset is left
  // (the mean is followed, so the first 200 ms are left out), and it has the even harmonics neither of the two has
  sid::patch p = plain(sid::osc_triangle);
  p.level = 128U;
  p.wave  = static_cast<std::uint8_t>(sid::osc_triangle | sid::osc_pulse);
  const sound both = cut(note(p, sid::key::a3, 1200U), 200U, 1200U);
  CHECK(near(frequency(both), 220.0, 0.3));
  double mean = 0.0;
  std::size_t flat = 0U;
  for (std::size_t i = 1U; i < both.size(); ++i) {
    mean += static_cast<double>(both[i]);
    const int step = both[i] - both[i - 1U];
    flat += ((step < 100) && (step > -100)) ? 1U : 0U;
  }
  CHECK(std::fabs(mean / static_cast<double>(both.size())) < 150.0);
  CHECK(near(static_cast<double>(flat) / static_cast<double>(both.size()), 0.49, 0.04));     // the silent half
  CHECK(peak(both) < 32767);
  sid::patch single = plain(sid::osc_triangle);
  single.level = 128U;
  const double even_triangle = power(cut(note(single, sid::key::a3, 1200U), 200U, 1200U), 440.0);
  single.wave = static_cast<std::uint8_t>(sid::osc_pulse);
  const double even_square = power(cut(note(single, sid::key::a3, 1200U), 200U, 1200U), 440.0);
  CHECK(power(both, 440.0) > 1.0e5);
  CHECK(power(both, 440.0) > (even_triangle * 100.0));
  CHECK(power(both, 440.0) > (even_square * 100.0));

  // the pulse width shapes it: a narrow pulse lets only the start of the triangle through
  p.pulse = 32U;
  const sound narrow = cut(note(p, sid::key::a3, 1200U), 200U, 1200U);
  int top = -32768;
  int bottom = 32767;
  int top_both = -32768;
  int bottom_both = 32767;
  for (std::size_t i = 0U; i < narrow.size(); ++i) {
    top         = (narrow[i] > top) ? narrow[i] : top;
    bottom      = (narrow[i] < bottom) ? narrow[i] : bottom;
    top_both    = (both[i] > top_both) ? both[i] : top_both;
    bottom_both = (both[i] < bottom_both) ? both[i] : bottom_both;
  }
  CHECK(near(static_cast<double>(top - bottom) / static_cast<double>(top_both - bottom_both), 0.25, 0.04));

  // sawtooth and triangle: their AND is smaller than either
  sid::patch st = plain(sid::osc_triangle);
  st.level = 128U;
  st.wave  = static_cast<std::uint8_t>(sid::osc_saw | sid::osc_triangle);
  const sound thin = cut(note(st, sid::key::a3, 1200U), 200U, 1200U);
  int top_thin = -32768;
  int bottom_thin = 32767;
  for (std::size_t i = 0U; i < thin.size(); ++i) {
    top_thin    = (thin[i] > top_thin) ? thin[i] : top_thin;
    bottom_thin = (thin[i] < bottom_thin) ? thin[i] : bottom_thin;
  }
  CHECK(near(static_cast<double>(top_thin - bottom_thin) / 32767.0, 0.667, 0.05));      // up to 2 / 3 of the full swing
  CHECK(power(thin, 220.0) > 1.0e5);

  // noise and square: the note gates the noise, so the note is in it
  sid::patch gated = plain(sid::osc_noise);
  gated.level = 128U;
  const double plain_noise = power(cut(note(gated, sid::key::a3, 1200U), 200U, 1200U), 220.0);
  gated.wave = static_cast<std::uint8_t>(sid::osc_noise | sid::osc_pulse);
  const sound bursts = cut(note(gated, sid::key::a3, 1200U), 200U, 1200U);
  CHECK(power(bursts, 220.0) > 1.0e5);
  CHECK(power(bursts, 220.0) > (plain_noise * 20.0));
  CHECK(note(gated, sid::key::a3, 100U) == note(gated, sid::key::a3, 100U));
  // and it is still noise in the bursts, not a square: no period is like the one before (22 kHz: 100 frames)
  const sound random = cut(note(gated, sid::key::a3, 1200U, 22000U), 200U, 1200U, 22000U);
  double differs = 0.0;
  double size    = 0.0;
  for (std::size_t i = 0U; (i + 100U) < random.size(); ++i) {
    differs += std::fabs(static_cast<double>(random[i]) - static_cast<double>(random[i + 100U]));
    size    += std::fabs(static_cast<double>(random[i]));
  }
  CHECK(differs > (size / 5.0));

  // a single waveform after a combined one on the same voice has no offset from it
  sid::synth s(RATE);
  s.note_on(0U, p, sid::key::a3);
  (void)take(s, 300U);
  const sid::patch alone = plain(sid::osc_triangle);    // the synth keeps a pointer to the patch: no temporary
  s.note_on(0U, alone, sid::key::a3);
  const sound after = cut(take(s, 500U), 100U, 500U);
  double mean_after = 0.0;
  for (std::size_t i = 0U; i < after.size(); ++i) {
    mean_after += static_cast<double>(after[i]);
  }
  CHECK(std::fabs(mean_after / static_cast<double>(after.size())) < 150.0);
}


TEST_CASE(sid_synth_filter_sums)
{
  // a low sawtooth (110 Hz) with all its harmonics
  sid::patch open = plain(sid::osc_saw);
  open.level = 64U;
  const sound raw = note(open, sid::key::a2, 1000U);
  const double f1  = power(raw, 110.0);
  const double f10 = power(raw, 1100.0);
  const double f40 = power(raw, 4400.0);

  // low and high pass together: a notch at the cutoff, the rest stays
  sid::patch notch = open;
  notch.filter = static_cast<std::uint8_t>(sid::filter_low | sid::filter_high);
  notch.cutoff = 1100U;
  const sound gap = cut(note(notch, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(power(gap, 1100.0) < (f10 / 1000.0));       // 30 dB down
  CHECK(near(power(gap, 110.0) / f1, 1.0, 0.25));
  CHECK(near(power(gap, 4400.0) / f40, 1.0, 0.3));
  // with resonance the notch is narrow: the harmonics beside it stay
  notch.resonance = 200U;
  const sound slit = cut(note(notch, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(power(slit, 1100.0) < (f10 / 1000.0));
  CHECK(near(power(slit, 660.0) / power(raw, 660.0), 0.9, 0.25));
  CHECK(near(power(slit, 1540.0) / power(raw, 1540.0), 0.8, 0.25));

  // all three: nothing is missing (the band fills the notch; not flat, the three do not add up in phase)
  sid::patch all = open;
  all.filter = static_cast<std::uint8_t>(sid::filter_low | sid::filter_band | sid::filter_high);
  all.cutoff = 1100U;
  const sound whole = cut(note(all, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(near(power(whole, 110.0) / f1, 1.0, 0.3));
  CHECK(near(power(whole, 1100.0) / f10, 0.5, 0.25));
  CHECK((power(whole, 4400.0) > (f40 / 2.0)) && (power(whole, 4400.0) < (f40 * 2.0)));

  // low and band: the top goes; band and high: the bottom goes
  sid::patch lower = open;
  lower.filter = static_cast<std::uint8_t>(sid::filter_low | sid::filter_band);
  lower.cutoff = 1100U;
  const sound dull = cut(note(lower, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(power(dull, 4400.0) < (f40 / 5.0));
  CHECK(power(dull, 110.0) > (f1 / 2.0));
  sid::patch upper = open;
  upper.filter = static_cast<std::uint8_t>(sid::filter_band | sid::filter_high);
  upper.cutoff = 1100U;
  const sound thin = cut(note(upper, sid::key::a2, 1100U), 100U, 1100U);
  CHECK(power(thin, 110.0) < (f1 / 20.0));
  CHECK(power(thin, 4400.0) > (f40 / 2.0));
}


TEST_CASE(sid_synth_voice_modulation)
{
  // the modulator: a note of level 0 on the voice before. It is not heard but its voice runs
  sid::patch quiet = plain(sid::osc_triangle);
  quiet.level = 0U;
  sid::patch ring = plain(sid::osc_triangle);
  ring.mod = sid::mod_ring_voice;
  {
    sid::synth s(RATE);
    s.note_on(0U, quiet, sid::key::e5);
    CHECK_EQ(peak(take(s, 200U)), 0);
    CHECK(!s.silent());
  }

  // ring modulation by the voice before: a4 times a triangle a fifth above gives sum and difference, not the note
  sid::synth s(RATE);
  s.note_on(0U, quiet, sid::key::e5);
  s.note_on(1U, ring, sid::key::a4);
  const sound bell = take(s, 1000U);
  const double sum_and_difference = power(bell, 659.26 - 440.0) + power(bell, 659.26 + 440.0);
  CHECK(sum_and_difference > (power(bell, 440.0) * 100.0));
  CHECK(sum_and_difference > 1.0e7);
  CHECK(peak(bell) <= 32767);

  // the waveform of the modulator is the colour: a square brings its strong third harmonic (3 x 659 - 440 Hz)
  sid::patch quiet_square = plain(sid::osc_pulse);
  quiet_square.level = 0U;
  sid::synth h(RATE);
  h.note_on(0U, quiet_square, sid::key::e5);
  h.note_on(1U, ring, sid::key::a4);
  const sound hard = take(h, 1000U);
  CHECK(power(hard, (3.0 * 659.26) - 440.0) > (power(bell, (3.0 * 659.26) - 440.0) * 4.0));
  CHECK(power(hard, 659.26 - 440.0) > 1.0e7);

  // voice 0 takes the last voice
  sid::synth w(RATE);
  w.note_on(SID_VOICES - 1U, quiet, sid::key::e5);
  w.note_on(0U, ring, sid::key::a4);
  const sound around = take(w, 1000U);
  CHECK((power(around, 659.26 - 440.0) + power(around, 659.26 + 440.0)) > 1.0e7);
  CHECK((power(around, 659.26 - 440.0) + power(around, 659.26 + 440.0)) > (power(around, 440.0) * 100.0));

  // without a note on the voice before there is nothing to multiply with: silence, also after that note has ended
  sid::synth lone(RATE);
  lone.note_on(1U, ring, sid::key::a4);
  CHECK_EQ(peak(take(lone, 200U)), 0);
  s.note_off(0U);
  (void)take(s, 100U);
  CHECK_EQ(peak(take(s, 200U)), 0);
  CHECK(!s.silent());       // the note of voice 1 is still held

  // sync by the voice before: the oscillator runs a fifth higher but restarts with the note of that voice, so
  // the sound repeats with it: at 44 kHz every 100 frames. For voice 1 behind voice 0, and voice 0 behind the last
  sid::patch quiet_saw = plain(sid::osc_saw);
  quiet_saw.level = 0U;
  sid::patch sync = plain(sid::osc_saw);
  sync.mod = sid::mod_sync_voice;
  const std::size_t pairs[2][2] = { { 0U, 1U }, { SID_VOICES - 1U, 0U } };
  for (std::size_t n = 0U; n < 2U; ++n) {
    sid::synth y(44000U);
    y.note_on(pairs[n][0], quiet_saw, sid::key::a4);
    y.note_on(pairs[n][1], sync, sid::key::e5);
    const sound locked = take(y, 500U);
    double repeat = 0.0;
    double size   = 0.0;
    for (std::size_t i = 2000U; i < 20000U; ++i) {
      repeat += std::fabs(static_cast<double>(locked[i]) - static_cast<double>(locked[i + 100U]));
      size   += std::fabs(static_cast<double>(locked[i]));
    }
    CHECK(size > (18000.0 * 5000.0));
    CHECK(repeat < (size / 100.0));

    // when the note of the voice before has ended the oscillator runs free again
    y.note_off(pairs[n][0]);
    (void)take(y, 100U);
    CHECK(near(frequency(take(y, 400U), 44000U), 659.26, 0.6));
  }
}


namespace {

// patches of the test songs: a full square that starts and ends at once, a soft one, a triangle
sid::patch beep_patch()
{
  sid::patch p = plain(sid::osc_pulse);
  p.release = 0U;
  return p;
}

const sid::patch song_patches[3] = { beep_patch(), [] { sid::patch p = beep_patch(); p.level = 64U; return p; }(),
                                     [] { sid::patch p = plain(sid::osc_triangle); p.level = 100U; p.release = 0U; return p; }() };

#define S_HOLD  { sid::key::hold, 0U }
#define S_OFF   { sid::key::off,  0U }

const sid::step line_a[4]  = { { sid::key::a4, 0U }, S_HOLD, S_OFF, { sid::key::a5, 1U } };
const sid::step line_b[4]  = { { sid::key::e5, 2U }, S_HOLD, S_HOLD, S_HOLD };
const sid::step line_c[4]  = { { sid::key::a4, 2U }, S_HOLD, S_HOLD, S_HOLD };
const sid::step silence[4] = { S_HOLD, S_HOLD, S_HOLD, S_HOLD };
const sid::step* const song_patterns[] = { line_a, line_b, line_c, silence };

} // namespace


TEST_CASE(sid_sequencer_steps)
{
  // one track, one row of 4 steps of 100 ms: a4, go on, off, a5 (soft)
  static const sid::slot order[] = { { 0U, 0 } };
  static const sid::song tune = { song_patches, song_patterns, order, 1U, 1U, 4U, 100U };
  sid::sequencer seq(RATE, tune);
  const sound s = all_of(seq);

  CHECK(peak(cut(s, 0U, 3U)) > 30000);                     // the note starts with the step
  CHECK(near(frequency(cut(s, 5U, 195U)), 440.0, 0.5));
  CHECK(peak(cut(s, 190U, 200U)) > 30000);                 // and goes on through the second step
  CHECK_EQ(peak(cut(s, 205U, 300U)), 0);                   // off
  CHECK(peak(cut(s, 300U, 301U)) > 0);                     // the next note, of the second patch
  CHECK(near(static_cast<double>(peak(cut(s, 310U, 400U))), 32767.0 * 64.0 / 255.0, 200.0));
  CHECK(near(frequency(cut(s, 305U, 395U)), 880.0, 2.0));
  // the note that is held at the end of the song ends with it; the song is as long as its steps plus the fade
  CHECK(s.size() >= 8820U);
  CHECK(s.size() <= (8820U + 441U));
  CHECK_EQ(peak(cut(s, 405U, 500U)), 0);

  // played twice: twice the steps
  sid::sequencer two(RATE, tune, 2U);
  const sound twice = all_of(two);
  CHECK(twice.size() >= 17640U);
  CHECK(twice.size() <= (17640U + 441U));
  CHECK(near(frequency(cut(twice, 405U, 595U)), 440.0, 0.5));

  // for ever: no end
  sid::sequencer endless(RATE, tune, 0U);
  CHECK(all_of(endless, 37U, 200000U).size() >= 200000U);

  // the length is exact also when a step is no whole number of frames: 16 steps of 125 ms at 11025 Hz
  static const sid::slot quiet[] = { { 3U, 0 }, { 3U, 0 }, { 3U, 0 }, { 3U, 0 } };
  static const sid::song rests = { song_patches, song_patterns, quiet, 4U, 1U, 4U, 125U };
  sid::sequencer q(11025U, rests);
  CHECK_EQ(all_of(q).size(), 22050U);

  // a song without rows, steps or time ends at once
  static const sid::song none = { song_patches, song_patterns, order, 0U, 1U, 4U, 100U };
  static const sid::song flat = { song_patches, song_patterns, order, 1U, 1U, 0U, 100U };
  static const sid::song fast = { song_patches, song_patterns, order, 1U, 1U, 4U, 0U };
  sid::sequencer n(RATE, none);
  sid::sequencer f(RATE, flat);
  sid::sequencer z(RATE, fast);
  CHECK_EQ(all_of(n).size(), 0U);
  CHECK_EQ(all_of(f).size(), 0U);
  CHECK_EQ(all_of(z).size(), 0U);
}


TEST_CASE(sid_sequencer_position)
{
  // two rows of 4 steps of 100 ms: the sequencer tells the row and the step that play
  static const sid::slot order[] = { { 0U, 0 }, { 1U, 0 } };
  static const sid::song tune = { song_patches, song_patterns, order, 2U, 1U, 4U, 100U };
  const std::size_t step_frames = RATE / 10U;
  std::vector<std::int16_t> buffer(step_frames * 2U);

  sid::sequencer seq(RATE, tune);
  CHECK_EQ(seq.row(), 0U);
  CHECK_EQ(seq.step(), 0U);
  CHECK(!seq.ended());
  for (std::size_t n = 0U; n < 8U; ++n) {
    // the first frame of a step starts it, the last one still belongs to it
    CHECK_EQ(seq.render(&buffer[0], 1U), 1U);
    CHECK_EQ(seq.row(), n / 4U);
    CHECK_EQ(seq.step(), n % 4U);
    CHECK_EQ(seq.ended(), n == 7U);         // no step follows the last one
    CHECK_EQ(seq.render(&buffer[0], step_frames - 1U), step_frames - 1U);
    CHECK_EQ(seq.row(), n / 4U);
    CHECK_EQ(seq.step(), n % 4U);
  }

  // a song that plays for ever goes round and never ends
  sid::sequencer endless(RATE, tune, 0U);
  for (std::size_t n = 0U; n < 8U; ++n) {
    CHECK_EQ(endless.render(&buffer[0], step_frames), step_frames);
  }
  CHECK_EQ(endless.row(), 1U);
  CHECK_EQ(endless.step(), 3U);
  CHECK_EQ(endless.render(&buffer[0], 1U), 1U);
  CHECK_EQ(endless.row(), 0U);
  CHECK_EQ(endless.step(), 0U);
  CHECK(!endless.ended());
}


TEST_CASE(sid_sequencer_tracks)
{
  // two tracks at once, the second transposed; a row without a pattern lets the note go on
  static const sid::slot order[] = {
    { 2U, 0 },               { 1U, 0 },
    { 2U, 12 },              { 1U, -12 },
    { sid::no_pattern, 0 },  { 3U, 0 }
  };
  static const sid::song tune = { song_patches, song_patterns, order, 3U, 2U, 4U, 100U };
  sid::sequencer seq(RATE, tune);
  const sound s = all_of(seq);
  REQUIRE(s.size() >= 26460U);

  const sound first = cut(s, 10U, 390U);
  CHECK(power(first, 440.0) > 1.0e6);
  CHECK(power(first, 659.26) > 1.0e6);
  CHECK(power(first, 880.0) < (power(first, 440.0) / 100.0));
  const sound second = cut(s, 410U, 790U);
  CHECK(power(second, 880.0) > 1.0e6);            // a4 + 12
  CHECK(power(second, 329.63) > 1.0e6);           // e5 - 12
  CHECK(power(second, 440.0) < (power(second, 880.0) / 100.0));
  const sound third = cut(s, 810U, 1190U);
  CHECK(power(third, 880.0) > 1.0e6);             // both go on
  CHECK(power(third, 329.63) > 1.0e6);

  // more tracks than voices: the tracks above are mute; a transposition below the lowest key is the lowest key
  static const sid::slot wide[] = { { 3U, 0 }, { 3U, 0 }, { 3U, 0 }, { 3U, 0 }, { 0U, 0 }, { 1U, -128 } };
  static const sid::song many = { song_patches, song_patterns, wide, 1U, 6U, 4U, 100U };
  sid::sequencer m(RATE, many);
  const sound mute = all_of(m);
  CHECK_EQ(mute.size(), 8820U);
  CHECK_EQ(peak(mute), 0);
  static const sid::slot deep[] = { { 1U, -128 } };
  static const sid::song low = { song_patches, song_patterns, deep, 1U, 1U, 4U, 100U };
  sid::sequencer l(RATE, low);
  const sound rumble = all_of(l);
  CHECK(peak(rumble) > 5000);
  CHECK(frequency(rumble) < 20.0);

  // the voices above the tracks belong to the app
  static const sid::slot one[] = { { 3U, 0 } };
  static const sid::song rest = { song_patches, song_patterns, one, 1U, 1U, 4U, 100U };
  sid::sequencer own(RATE, rest);
  own.voices().note_on(3U, song_patches[2], sid::key::a4);
  CHECK(near(frequency(cut(all_of(own), 5U, 395U)), 440.0, 0.5));
}


TEST_CASE(sid_sequencer_end)
{
  // the last notes fade before the song ends, but not longer than SID_SONG_TAIL_MS
  static sid::patch slow = plain(sid::osc_triangle);
  slow.release = 500U;
  static const sid::step held[4] = { { sid::key::a4, 0U }, S_HOLD, S_HOLD, S_HOLD };
  static const sid::step* const patterns[] = { held };
  static const sid::slot order[] = { { 0U, 0 } };
  static const sid::song tune = { &slow, patterns, order, 1U, 1U, 4U, 100U };
  sid::sequencer seq(RATE, tune);
  const sound s = all_of(seq);
  CHECK(peak(cut(s, 395U, 400U)) > 30000);
  CHECK(peak(cut(s, 700U, 750U)) < 3000);
  CHECK(peak(cut(s, 700U, 750U)) > 100);                  // released at the end of the last step, fading
  CHECK(s.size() > static_cast<std::size_t>(22050U * 0.9));
  CHECK(s.size() < static_cast<std::size_t>(22050U * 1.5));
  CHECK(peak(cut(s, static_cast<std::uint32_t>((s.size() * 1000U) / RATE) - 10U, 100000U)) < 60);

  slow.release = 60000U;
  sid::sequencer capped(RATE, tune);
  const sound c = all_of(capped);
  CHECK_EQ(c.size(), 8820U + ((SID_SONG_TAIL_MS / 10U) * 220U));
  slow.release = 500U;

  // through the output to a device: the short form
  memory_device dev;
  sid::output out(dev);
  REQUIRE(out.open(RATE));
  REQUIRE(sid::play(out, tune));
  CHECK_EQ(dev.frames(), s.size());
  CHECK_EQ(dev.rate(), RATE);
  bool same = (dev.samples().size() == (s.size() * 2U));
  for (std::size_t i = 0U; same && (i < s.size()); ++i) {
    same = (dev.samples()[i * 2U] == s[i]) && (dev.samples()[i * 2U + 1U] == s[i]);
  }
  CHECK(same);
  out.close();
}

#undef S_HOLD
#undef S_OFF


TEST_CASE(sid_example_song)
{
  // the song of the examples (Bach, Invention 13): it ends, it is as long as its rows, every row sounds and nearly nothing clips
  sid::sequencer seq(RATE, bach::song);
  const sound s = all_of(seq);
  const std::size_t steps = static_cast<std::size_t>(bach::song.rows) * bach::song.steps;
  const std::size_t frames = (steps * bach::song.step_ms * RATE) / 1000U;
  CHECK(s.size() >= frames);
  CHECK(s.size() < (frames + (RATE / 2U)));
  CHECK_EQ(sizeof(bach::order) / sizeof(bach::order[0]), static_cast<std::size_t>(bach::song.rows) * bach::song.tracks);

  const std::uint32_t row_ms = static_cast<std::uint32_t>(bach::song.steps) * bach::song.step_ms;
  for (std::uint32_t row = 0U; row < bach::song.rows; ++row) {
    CHECK(peak(cut(s, row * row_ms, (row + 1U) * row_ms)) > 8000);
  }
  std::size_t clipped = 0U;
  for (std::size_t i = 0U; i < s.size(); ++i) {
    clipped += ((s[i] >= 32767) || (s[i] <= -32767)) ? 1U : 0U;
  }
  CHECK_MSG(clipped < (s.size() / 2000U), "the levels of the patches fit: less than 0.05 % of the frames clip");
  CHECK(peak(s) > 20000);

  // every pattern has the steps of the song, every step names a patch of the song
  const std::size_t patches = sizeof(bach::patches) / sizeof(bach::patches[0]);
  const std::size_t patterns = sizeof(bach::patterns) / sizeof(bach::patterns[0]);
  for (std::size_t i = 0U; i < (static_cast<std::size_t>(bach::song.rows) * bach::song.tracks); ++i) {
    CHECK((bach::order[i].pattern == sid::no_pattern) || (bach::order[i].pattern < patterns));
  }
  for (std::size_t p = 0U; p < patterns; ++p) {
    for (std::size_t i = 0U; i < bach::song.steps; ++i) {
      CHECK(bach::patterns[p][i].patch < patches);
    }
  }
}
