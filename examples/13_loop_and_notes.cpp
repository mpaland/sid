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
// \brief sid example 13: music that loops, with notes of the app on top
//        What a game or a device with a user interface does: background music that goes on for ever, and sounds
//        of the app over it. A sequencer made with 0 loops never ends; the output plays it in slices of time, and
//        between two slices the app starts notes on a voice the song leaves free (sequencer::voices() is the
//        synthesizer of the song). On a microcontroller the slices are the loop of the audio task.
//        Here: a two bar loop on the voices 0 .. 2, and every now and then an effect on voice 3.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/sequencer.h"

#include "common.h"


enum { KICK = 0, SNARE, BASS, CHORD_MINOR, CHORD_MAJOR, COIN, LASER };

static const sid::patch patches[] = {
  // wave            level attack decay sustain release pulse pwm  lfo: rate delay vibrato  sweep glide  arpeggio     ms  mod            pitch  filter           reson. cutoff end  ms
  { sid::osc_triangle, 150U,   0U, 140U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,  -300,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // kick
  { sid::osc_noise,     80U,   0U, 110U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,   -60,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // snare
  { sid::osc_saw,       90U,   2U, 160U,  110U,    60U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_low,  170U, 2200U, 350U, 140U },   // bass
  { sid::osc_pulse,     40U,   2U, 260U,    0U,   100U,   64U, 48U,       8U,   0U,     0U,     0,   0U, { 3U, 7U }, 20U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // minor chord
  { sid::osc_pulse,     40U,   2U, 260U,    0U,   100U,   64U, 48U,       8U,   0U,     0U,     0,   0U, { 4U, 7U }, 20U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // major chord
  { sid::osc_pulse,     70U,   0U, 400U,    0U,   100U,  128U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // coin
  { sid::osc_pulse,     70U,   0U, 250U,    0U,    50U,   64U,  0U,       0U,   0U,     0U,  -190,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U }    // laser
};

#define REST  { sid::key::hold, 0U }

static const sid::step drums[16] = {
  { sid::key::g3, KICK }, REST, REST, REST, { sid::key::c5, SNARE }, REST, REST, { sid::key::g3, KICK },
  { sid::key::g3, KICK }, REST, REST, REST, { sid::key::c5, SNARE }, REST, REST, REST
};
static const sid::step bass_line[16] = {
  { sid::key::a2, BASS }, REST, { sid::key::a3, BASS }, { sid::key::a2, BASS }, REST, { sid::key::a3, BASS }, { sid::key::a2, BASS }, REST,
  { sid::key::a2, BASS }, REST, { sid::key::a3, BASS }, { sid::key::a2, BASS }, REST, { sid::key::e3, BASS }, { sid::key::a3, BASS }, REST
};
static const sid::step chords_minor[16] = {
  { sid::key::a4, CHORD_MINOR }, REST, REST, { sid::key::a4, CHORD_MINOR }, REST, REST, { sid::key::a4, CHORD_MINOR }, REST,
  REST, REST, { sid::key::a4, CHORD_MINOR }, REST, { sid::key::a4, CHORD_MINOR }, REST, REST, REST
};
static const sid::step chords_major[16] = {
  { sid::key::a4, CHORD_MAJOR }, REST, REST, { sid::key::a4, CHORD_MAJOR }, REST, REST, { sid::key::a4, CHORD_MAJOR }, REST,
  REST, REST, { sid::key::a4, CHORD_MAJOR }, REST, { sid::key::a4, CHORD_MAJOR }, REST, REST, REST
};

#undef REST

static const sid::step* const patterns[] = { drums, bass_line, chords_minor, chords_major };

// two bars: A minor and F (the bass and the chords of the second bar are those of the first, 4 semitones down)
static const sid::slot order[] = {
  { 0U, 0 }, { 1U,  0 }, { 2U,  0 },
  { 0U, 0 }, { 1U, -4 }, { 3U, -4 }
};

static const sid::song loop = { patches, patterns, order, 2U, 3U, 16U, 125U };


static bool play(sid::output& out)
{
  if (!out.open(example::rate)) {
    return false;
  }
  sid::sequencer music(out.rate(), loop, 0U);     // 0 loops: for ever
  bool ok = true;

  // 16 s in slices of 250 ms; a real app would look at its buttons and events between the slices
  for (std::uint32_t slice = 0U; ok && (slice < 64U); ++slice) {
    if ((slice % 16U) == 6U) {
      music.voices().note_on(3U, patches[COIN], sid::key::b5);      // voice 3 is free: the song has three tracks
    }
    if ((slice % 16U) == 7U) {
      music.voices().note_on(3U, patches[COIN], sid::key::e6);
    }
    if ((slice % 16U) == 13U) {
      music.voices().note_on(3U, patches[LASER], sid::key::c7);
    }
    ok = ok && out.play(music, 250U);
  }

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "13_loop_and_notes", &play);
}
