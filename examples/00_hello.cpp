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
// \brief sid example 00: the quick start of the readme
//        The code of the readme as a program, so it is built and run with every commit: a beep, a melody, a note
//        on the synthesizer and a song of two bars, written to hello.wav.
//
///////////////////////////////////////////////////////////////////////////////

#include "sid/src/sid.h"
#include "sid/src/port/wav_file.h"


// a bass: a sawtooth through the resonant low pass, which closes while the note sounds
static sid::patch make_bass()
{
  sid::patch p = { };
  p.wave       = sid::osc_saw;
  p.level      = 150U;
  p.attack     = 2U;        // [ms]
  p.decay      = 150U;
  p.sustain    = 200U;      // of 255
  p.release    = 200U;
  p.filter     = sid::filter_low;
  p.resonance  = 170U;
  p.cutoff     = 2200U;     // [Hz] when the note starts
  p.cutoff_end = 350U;      // where it moves to
  p.cutoff_ms  = 400U;      // in this time
  return p;
}

// a song: the instruments, one pattern of four steps, and the order - the bar in A, then four semitones lower in F
static const sid::patch patches[] = { make_bass() };
static const sid::step  line[4]   = { { sid::key::a2, 0U }, { sid::key::hold, 0U }, { sid::key::a3, 0U }, { sid::key::off, 0U } };
static const sid::step* const patterns[] = { line };
static const sid::slot  order[]   = { { 0U, 0 }, { 0U, -4 } };
static const sid::song  tune      = { patches, patterns, order, 2U, 1U, 4U, 250U };     // 2 rows, 1 track, 4 steps of 250 ms


int main()
{
  sid::port::wav_file file("hello.wav");
  sid::output out(file);
  if (!out.open(22050U)) {
    return 1;
  }

  // a beep and a melody
  bool ok = sid::beep(out, 880U, 150U);
  static const sid::note notes[] = { { sid::c5, 200U }, { sid::e5, 200U }, { sid::g5, 400U } };
  ok = ok && sid::play(out, notes, 3U, sid::wave_triangle);

  // a note on the synthesizer
  sid::synth synth(out.rate());
  synth.note_on(0U, patches[0], sid::key::a2);
  ok = ok && out.play(synth, 800U);             // the synthesizer never ends: it is played for a time
  synth.note_off(0U);
  ok = ok && out.play(synth, 300U);

  // the song
  ok = ok && sid::play(out, tune);

  out.close();
  return (ok && file.close()) ? 0 : 1;
}
