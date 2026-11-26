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
// \brief sid example 10: the sequencer
//        A song for the sequencer is written like the tunes of the 80s, in patterns: a pattern is one bar for one
//        voice, an array of steps { key, patch }. A step starts a note, ends the running one (key::off) or lets
//        it go on (key::hold). The order of the song has one row per bar and names, for every track, the pattern
//        it plays and a transposition. All of it is constant data.
//        Here: drums on track 0, a bass on track 1 that is written once and transposed through four chords.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/sequencer.h"

#include "common.h"


// the instruments of the song
enum { KICK = 0, SNARE, HAT, BASS };

static const sid::patch patches[] = {
  // wave            level attack decay sustain release pulse pwm  lfo: rate delay vibrato  sweep glide  arpeggio     ms  mod            pitch  filter           reson. cutoff end  ms
  { sid::osc_triangle, 200U,   0U, 140U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,  -300,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // kick
  { sid::osc_noise,    110U,   0U, 110U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,   -60,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // snare
  { sid::osc_noise,     50U,   0U,  35U,    0U,    20U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // hat
  { sid::osc_saw,      120U,   2U, 160U,  110U,    60U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_low,  170U, 2200U, 350U, 140U }    // bass
};

#define REST  { sid::key::hold, 0U }
#define OFF   { sid::key::off,  0U }

// 16 steps = one bar in sixteenth notes; the key of a drum is the pitch of the kick and the colour of the noise
static const sid::step drums[16] = {
  { sid::key::g3, KICK },  REST, { sid::key::e7, HAT }, REST,
  { sid::key::c5, SNARE }, REST, { sid::key::e7, HAT }, REST,
  { sid::key::g3, KICK },  REST, { sid::key::g3, KICK }, REST,
  { sid::key::c5, SNARE }, REST, { sid::key::e7, HAT }, REST
};
static const sid::step drum_fill[16] = {
  { sid::key::g3, KICK },  REST, { sid::key::e7, HAT }, REST,
  { sid::key::c5, SNARE }, REST, { sid::key::e7, HAT }, REST,
  { sid::key::g3, KICK },  REST, { sid::key::g3, KICK }, REST,
  { sid::key::c5, SNARE }, { sid::key::c5, SNARE }, { sid::key::g4, SNARE }, { sid::key::g4, SNARE }
};

// the bass, written for A: root and octave, the fifth before the last step
static const sid::step bass_line[16] = {
  { sid::key::a2, BASS }, REST, { sid::key::a3, BASS }, { sid::key::a2, BASS },
  REST, { sid::key::a3, BASS }, { sid::key::a2, BASS }, { sid::key::a2, BASS },
  REST, { sid::key::a3, BASS }, { sid::key::a2, BASS }, REST,
  { sid::key::a3, BASS }, REST, { sid::key::e3, BASS }, { sid::key::a3, BASS }
};
static const sid::step bass_end[16] = {
  { sid::key::a2, BASS }, REST, REST, REST, REST, REST, REST, REST, OFF, REST, REST, REST, REST, REST, REST, REST
};
static const sid::step drum_end[16] = {
  { sid::key::g3, KICK }, REST, REST, REST, REST, REST, REST, REST, REST, REST, REST, REST, REST, REST, REST, REST
};

#undef REST
#undef OFF

enum { P_DRUMS = 0, P_FILL, P_BASS, P_BASS_END, P_DRUM_END };
static const sid::step* const patterns[] = { drums, drum_fill, bass_line, bass_end, drum_end };

// the song: one row per bar, { pattern, transposition } for track 0 (drums) and track 1 (bass)
static const sid::slot order[] = {
  { P_DRUMS,    0 }, { sid::no_pattern, 0 },      // the drums alone
  { P_FILL,     0 }, { sid::no_pattern, 0 },
  { P_DRUMS,    0 }, { P_BASS,      0 },          // A
  { P_DRUMS,    0 }, { P_BASS,     -4 },          // F
  { P_DRUMS,    0 }, { P_BASS,      3 },          // C
  { P_FILL,     0 }, { P_BASS,     -2 },          // G
  { P_DRUM_END, 0 }, { P_BASS_END,  0 }           // A
};

static const sid::song song = {
  patches, patterns, order,
  7U,       // rows
  2U,       // tracks
  16U,      // steps per pattern
  120U      // [ms] per step: sixteenth notes at 125 beats per minute
};


static bool play(sid::output& out)
{
  bool ok = out.open(example::rate);

  // the short way: the song from its first to its last row
  ok = ok && sid::play(out, song);

  // the sequencer is a source like any other; with a second argument the song is played that many times
  sid::sequencer twice(out.rate(), song, 2U);
  ok = ok && out.play(twice);

  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  return example::run(argc, argv, "10_drum_pattern", &play);
}
