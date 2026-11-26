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
// \brief sid examples: a song for the synthesizer and the sequencer (sid/src/sequencer.h)
//        J. S. Bach, Invention 13 in A minor (BWV 784), the way the home computers of the 80s played Bach:
//        the right hand as a pulse lead with a quiet echo, the left hand as a sawtooth bass through the
//        resonant low pass, and drums. 26 rows of one bar (16 steps of 110 ms), 46 s.
//        The piece is in the public domain; the notes are those of the Mutopia Project edition
//        (Mutopia-2008/06/15-59, typeset by Allen Garvin after the Bach-Gesellschaft edition, placed in the
//        public domain by the typesetter).
//        Made by make_invention13.py, which holds the notes and the arrangement - change the song there.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_EXAMPLE_INVENTION13_H_
#define _SID_EXAMPLE_INVENTION13_H_

#include "sid/src/sequencer.h"


namespace bach {

// the instruments
const sid::patch patches[] = {
  // wave            level attack decay sustain release pulse pwm  lfo: rate delay vibrato  sweep glide  arpeggio     ms  mod            pitch  filter           reson. cutoff end  ms
  { sid::osc_pulse,    66U,    2U, 180U,  150U,   220U,   80U, 30U,      45U,  30U,    14U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // lead
  { sid::osc_saw,      90U,    1U, 140U,  120U,   200U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_low,  150U, 2400U, 600U, 120U },   // bass
  { sid::osc_pulse,    20U,    4U, 180U,  130U,   220U,  128U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // echo
  { sid::osc_triangle, 110U,   0U, 140U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,  -280,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // kick
  { sid::osc_noise,    56U,    0U, 120U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,   -50,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // snare
  { sid::osc_noise,    26U,    0U,  35U,    0U,    20U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U }    // hat
};

// the patterns: { key, patch } per step; key 0 = the note goes on (sid::key::hold), 1 = it ends (sid::key::off)
const sid::step pattern_0[16] = { {   0U, 0U }, {  64U, 0U }, {  69U, 0U }, {  72U, 0U }, {  71U, 0U }, {  64U, 0U }, {  71U, 0U }, {  74U, 0U }, {  72U, 0U }, {   0U, 0U }, {  76U, 0U }, {   0U, 0U }, {  68U, 0U }, {   0U, 0U }, {  76U, 0U }, {   0U, 0U } };
const sid::step pattern_1[16] = { {  45U, 1U }, {   0U, 1U }, {  57U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {  56U, 1U }, {   0U, 1U }, {  57U, 1U }, {  52U, 1U }, {  57U, 1U }, {  60U, 1U }, {  59U, 1U }, {  52U, 1U }, {  59U, 1U }, {  62U, 1U } };
const sid::step pattern_2[16] = { {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  64U, 2U }, {  69U, 2U }, {  72U, 2U }, {  71U, 2U }, {  64U, 2U }, {  71U, 2U }, {  74U, 2U }, {  72U, 2U }, {   0U, 2U }, {  76U, 2U }, {   0U, 2U }, {  68U, 2U } };
const sid::step pattern_3[16] = { {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U } };
const sid::step pattern_4[16] = { {  69U, 0U }, {  64U, 0U }, {  69U, 0U }, {  72U, 0U }, {  71U, 0U }, {  64U, 0U }, {  71U, 0U }, {  74U, 0U }, {  72U, 0U }, {   0U, 0U }, {  69U, 0U }, {   0U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_5[16] = { {  60U, 1U }, {   0U, 1U }, {  57U, 1U }, {   0U, 1U }, {  56U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  57U, 1U }, {  52U, 1U }, {  57U, 1U }, {  60U, 1U }, {  59U, 1U }, {  52U, 1U }, {  59U, 1U }, {  62U, 1U } };
const sid::step pattern_6[16] = { {   0U, 2U }, {  76U, 2U }, {   0U, 2U }, {  69U, 2U }, {  64U, 2U }, {  69U, 2U }, {  72U, 2U }, {  71U, 2U }, {  64U, 2U }, {  71U, 2U }, {  74U, 2U }, {  72U, 2U }, {   0U, 2U }, {  69U, 2U }, {   0U, 2U }, {   1U, 2U } };
const sid::step pattern_7[16] = { {   0U, 0U }, {  76U, 0U }, {  72U, 0U }, {  76U, 0U }, {  69U, 0U }, {  72U, 0U }, {  64U, 0U }, {  67U, 0U }, {  65U, 0U }, {   0U, 0U }, {  69U, 0U }, {   0U, 0U }, {  74U, 0U }, {   0U, 0U }, {  77U, 0U }, {   0U, 0U } };
const sid::step pattern_8[16] = { {  60U, 1U }, {   0U, 1U }, {  57U, 1U }, {   0U, 1U }, {  60U, 1U }, {   0U, 1U }, {  57U, 1U }, {   0U, 1U }, {  62U, 1U }, {  57U, 1U }, {  53U, 1U }, {  57U, 1U }, {  50U, 1U }, {  53U, 1U }, {  45U, 1U }, {  48U, 1U } };
const sid::step pattern_9[16] = { {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  76U, 2U }, {  72U, 2U }, {  76U, 2U }, {  69U, 2U }, {  72U, 2U }, {  64U, 2U }, {  67U, 2U }, {  65U, 2U }, {   0U, 2U }, {  69U, 2U }, {   0U, 2U }, {  74U, 2U } };
const sid::step pattern_10[16] = { {  55U, 3U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, {  72U, 4U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, {  55U, 3U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, {  72U, 4U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U } };
const sid::step pattern_11[16] = { {   0U, 0U }, {  74U, 0U }, {  71U, 0U }, {  74U, 0U }, {  67U, 0U }, {  71U, 0U }, {  62U, 0U }, {  65U, 0U }, {  64U, 0U }, {   0U, 0U }, {  67U, 0U }, {   0U, 0U }, {  72U, 0U }, {   0U, 0U }, {  76U, 0U }, {   0U, 0U } };
const sid::step pattern_12[16] = { {  47U, 1U }, {   0U, 1U }, {  50U, 1U }, {   0U, 1U }, {  55U, 1U }, {   0U, 1U }, {  59U, 1U }, {   0U, 1U }, {   0U, 1U }, {  55U, 1U }, {  52U, 1U }, {  55U, 1U }, {  48U, 1U }, {  52U, 1U }, {  43U, 1U }, {  47U, 1U } };
const sid::step pattern_13[16] = { {   0U, 2U }, {  77U, 2U }, {   0U, 2U }, {   0U, 2U }, {  74U, 2U }, {  71U, 2U }, {  74U, 2U }, {  67U, 2U }, {  71U, 2U }, {  62U, 2U }, {  65U, 2U }, {  64U, 2U }, {   0U, 2U }, {  67U, 2U }, {   0U, 2U }, {  72U, 2U } };
const sid::step pattern_14[16] = { {   0U, 0U }, {  72U, 0U }, {  69U, 0U }, {  72U, 0U }, {  65U, 0U }, {   0U, 0U }, {  74U, 0U }, {   0U, 0U }, {   0U, 0U }, {  71U, 0U }, {  67U, 0U }, {  71U, 0U }, {  64U, 0U }, {   0U, 0U }, {  72U, 0U }, {   0U, 0U } };
const sid::step pattern_15[16] = { {  45U, 1U }, {   0U, 1U }, {  48U, 1U }, {   0U, 1U }, {  50U, 1U }, {  53U, 1U }, {  47U, 1U }, {  50U, 1U }, {  43U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {  48U, 1U }, {  52U, 1U }, {  45U, 1U }, {  48U, 1U } };
const sid::step pattern_16[16] = { {   0U, 2U }, {  76U, 2U }, {   0U, 2U }, {   0U, 2U }, {  72U, 2U }, {  69U, 2U }, {  72U, 2U }, {  65U, 2U }, {   0U, 2U }, {  74U, 2U }, {   0U, 2U }, {   0U, 2U }, {  71U, 2U }, {  67U, 2U }, {  71U, 2U }, {  64U, 2U } };
const sid::step pattern_17[16] = { {   0U, 0U }, {  69U, 0U }, {  65U, 0U }, {  69U, 0U }, {  62U, 0U }, {   0U, 0U }, {  71U, 0U }, {   0U, 0U }, {  72U, 0U }, {   0U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_18[16] = { {  41U, 1U }, {   0U, 1U }, {  38U, 1U }, {   0U, 1U }, {  43U, 1U }, {  55U, 1U }, {  53U, 1U }, {  55U, 1U }, {  48U, 1U }, {  55U, 1U }, {  60U, 1U }, {  64U, 1U }, {  62U, 1U }, {  55U, 1U }, {  62U, 1U }, {  65U, 1U } };
const sid::step pattern_19[16] = { {   0U, 2U }, {  72U, 2U }, {   0U, 2U }, {   0U, 2U }, {  69U, 2U }, {  65U, 2U }, {  69U, 2U }, {  62U, 2U }, {   0U, 2U }, {  71U, 2U }, {   0U, 2U }, {  72U, 2U }, {   0U, 2U }, {   1U, 2U }, {   0U, 2U }, {   0U, 2U } };
const sid::step pattern_20[16] = { {  55U, 3U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, {  72U, 4U }, {   0U, 0U }, { 100U, 5U }, {   0U, 0U }, {  55U, 3U }, {   0U, 0U }, {  55U, 3U }, {   0U, 0U }, {  72U, 4U }, {  72U, 4U }, {  72U, 4U }, {  72U, 4U } };
const sid::step pattern_21[16] = { {   0U, 0U }, {  67U, 0U }, {  72U, 0U }, {  76U, 0U }, {  74U, 0U }, {  67U, 0U }, {  74U, 0U }, {  77U, 0U }, {  76U, 0U }, {   0U, 0U }, {  79U, 0U }, {   0U, 0U }, {  71U, 0U }, {   0U, 0U }, {  79U, 0U }, {   0U, 0U } };
const sid::step pattern_22[16] = { {  64U, 1U }, {   0U, 1U }, {  60U, 1U }, {   0U, 1U }, {  59U, 1U }, {   0U, 1U }, {  55U, 1U }, {   0U, 1U }, {  60U, 1U }, {  55U, 1U }, {  60U, 1U }, {  64U, 1U }, {  62U, 1U }, {  55U, 1U }, {  62U, 1U }, {  65U, 1U } };
const sid::step pattern_23[16] = { {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  67U, 2U }, {  72U, 2U }, {  76U, 2U }, {  74U, 2U }, {  67U, 2U }, {  74U, 2U }, {  77U, 2U }, {  76U, 2U }, {   0U, 2U }, {  79U, 2U }, {   0U, 2U }, {  71U, 2U } };
const sid::step pattern_24[16] = { {  72U, 0U }, {  67U, 0U }, {  72U, 0U }, {  76U, 0U }, {  74U, 0U }, {  67U, 0U }, {  74U, 0U }, {  77U, 0U }, {  76U, 0U }, {   0U, 0U }, {  72U, 0U }, {   0U, 0U }, {  79U, 0U }, {   0U, 0U }, {  76U, 0U }, {   0U, 0U } };
const sid::step pattern_25[16] = { {  64U, 1U }, {   0U, 1U }, {  60U, 1U }, {   0U, 1U }, {   1U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {  67U, 1U }, {  64U, 1U }, {  67U, 1U }, {  60U, 1U }, {  64U, 1U }, {  55U, 1U }, {  59U, 1U } };
const sid::step pattern_26[16] = { {   0U, 2U }, {  79U, 2U }, {   0U, 2U }, {  72U, 2U }, {  67U, 2U }, {  72U, 2U }, {  76U, 2U }, {  74U, 2U }, {  67U, 2U }, {  74U, 2U }, {  77U, 2U }, {  76U, 2U }, {   0U, 2U }, {  72U, 2U }, {   0U, 2U }, {  79U, 2U } };
const sid::step pattern_27[16] = { {  84U, 0U }, {  81U, 0U }, {  76U, 0U }, {  81U, 0U }, {  72U, 0U }, {  76U, 0U }, {  69U, 0U }, {  72U, 0U }, {  74U, 0U }, {   0U, 0U }, {  78U, 0U }, {   0U, 0U }, {  81U, 0U }, {   0U, 0U }, {  84U, 0U }, {   0U, 0U } };
const sid::step pattern_28[16] = { {  57U, 1U }, {   0U, 1U }, {  60U, 1U }, {   0U, 1U }, {  64U, 1U }, {   0U, 1U }, {  67U, 1U }, {   0U, 1U }, {  66U, 1U }, {  69U, 1U }, {  62U, 1U }, {  66U, 1U }, {  57U, 1U }, {  62U, 1U }, {  54U, 1U }, {  57U, 1U } };
const sid::step pattern_29[16] = { {   0U, 2U }, {  76U, 2U }, {   0U, 2U }, {  84U, 2U }, {  81U, 2U }, {  76U, 2U }, {  81U, 2U }, {  72U, 2U }, {  76U, 2U }, {  69U, 2U }, {  72U, 2U }, {  74U, 2U }, {   0U, 2U }, {  78U, 2U }, {   0U, 2U }, {  81U, 2U } };
const sid::step pattern_30[16] = { {  83U, 0U }, {  79U, 0U }, {  74U, 0U }, {  79U, 0U }, {  71U, 0U }, {  74U, 0U }, {  67U, 0U }, {  71U, 0U }, {  72U, 0U }, {   0U, 0U }, {  76U, 0U }, {   0U, 0U }, {  79U, 0U }, {   0U, 0U }, {  83U, 0U }, {   0U, 0U } };
const sid::step pattern_31[16] = { {  55U, 1U }, {   0U, 1U }, {  59U, 1U }, {   0U, 1U }, {  62U, 1U }, {   0U, 1U }, {  66U, 1U }, {   0U, 1U }, {  64U, 1U }, {  67U, 1U }, {  60U, 1U }, {  64U, 1U }, {  55U, 1U }, {  60U, 1U }, {  52U, 1U }, {  55U, 1U } };
const sid::step pattern_32[16] = { {   0U, 2U }, {  84U, 2U }, {   0U, 2U }, {  83U, 2U }, {  79U, 2U }, {  74U, 2U }, {  79U, 2U }, {  71U, 2U }, {  74U, 2U }, {  67U, 2U }, {  71U, 2U }, {  72U, 2U }, {   0U, 2U }, {  76U, 2U }, {   0U, 2U }, {  79U, 2U } };
const sid::step pattern_33[16] = { {  81U, 0U }, {  78U, 0U }, {  75U, 0U }, {  78U, 0U }, {  71U, 0U }, {  75U, 0U }, {  66U, 0U }, {  69U, 0U }, {  67U, 0U }, {   0U, 0U }, {  79U, 0U }, {   0U, 0U }, {   0U, 0U }, {  76U, 0U }, {  72U, 0U }, {  76U, 0U } };
const sid::step pattern_34[16] = { {  54U, 1U }, {   0U, 1U }, {  57U, 1U }, {   0U, 1U }, {  59U, 1U }, {   0U, 1U }, {  63U, 1U }, {   0U, 1U }, {   1U, 1U }, {  64U, 1U }, {  60U, 1U }, {  64U, 1U }, {  57U, 1U }, {  60U, 1U }, {  64U, 1U }, {  67U, 1U } };
const sid::step pattern_35[16] = { {   0U, 2U }, {  83U, 2U }, {   0U, 2U }, {  81U, 2U }, {  78U, 2U }, {  75U, 2U }, {  78U, 2U }, {  71U, 2U }, {  75U, 2U }, {  66U, 2U }, {  69U, 2U }, {  67U, 2U }, {   0U, 2U }, {  79U, 2U }, {   0U, 2U }, {   0U, 2U } };
const sid::step pattern_36[16] = { {  69U, 0U }, {   0U, 0U }, {  78U, 0U }, {   0U, 0U }, {   0U, 0U }, {  74U, 0U }, {  71U, 0U }, {  74U, 0U }, {  67U, 0U }, {   0U, 0U }, {  76U, 0U }, {   0U, 0U }, {   0U, 0U }, {  72U, 0U }, {  69U, 0U }, {  72U, 0U } };
const sid::step pattern_37[16] = { {  66U, 1U }, {  62U, 1U }, {  59U, 1U }, {  62U, 1U }, {  55U, 1U }, {  59U, 1U }, {  62U, 1U }, {  66U, 1U }, {  64U, 1U }, {  60U, 1U }, {  57U, 1U }, {  60U, 1U }, {  54U, 1U }, {  57U, 1U }, {  60U, 1U }, {   0U, 1U } };
const sid::step pattern_38[16] = { {  76U, 2U }, {  72U, 2U }, {  76U, 2U }, {  69U, 2U }, {   0U, 2U }, {  78U, 2U }, {   0U, 2U }, {   0U, 2U }, {  74U, 2U }, {  71U, 2U }, {  74U, 2U }, {  67U, 2U }, {   0U, 2U }, {  76U, 2U }, {   0U, 2U }, {   0U, 2U } };
const sid::step pattern_39[16] = { {  66U, 0U }, {  79U, 0U }, {  78U, 0U }, {  76U, 0U }, {  75U, 0U }, {  78U, 0U }, {  71U, 0U }, {  75U, 0U }, {  76U, 0U }, {   0U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_40[16] = { {   0U, 1U }, {  59U, 1U }, {  60U, 1U }, {  57U, 1U }, {  59U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {  52U, 1U }, {  64U, 1U }, {  59U, 1U }, {  55U, 1U }, {  52U, 1U }, {  47U, 1U }, {  43U, 1U }, {  47U, 1U } };
const sid::step pattern_41[16] = { {  72U, 2U }, {  69U, 2U }, {  72U, 2U }, {  66U, 2U }, {  79U, 2U }, {  78U, 2U }, {  76U, 2U }, {  75U, 2U }, {  78U, 2U }, {  71U, 2U }, {  75U, 2U }, {  76U, 2U }, {   0U, 2U }, {   1U, 2U }, {   0U, 2U }, {   0U, 2U } };
const sid::step pattern_42[16] = { {   0U, 0U }, {  79U, 0U }, {  82U, 0U }, {  79U, 0U }, {  76U, 0U }, {  79U, 0U }, {  73U, 0U }, {  76U, 0U }, {  79U, 0U }, {  76U, 0U }, {  73U, 0U }, {  76U, 0U }, {  69U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_43[16] = { {  40U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  55U, 1U }, {   0U, 1U }, {  58U, 1U }, {   0U, 1U }, {  49U, 1U }, {   0U, 1U }, {   1U, 1U }, {   0U, 1U }, {   0U, 1U }, {  67U, 1U }, {  65U, 1U }, {  64U, 1U } };
const sid::step pattern_44[16] = { {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  79U, 2U }, {  82U, 2U }, {  79U, 2U }, {  76U, 2U }, {  79U, 2U }, {  73U, 2U }, {  76U, 2U }, {  79U, 2U }, {  76U, 2U }, {  73U, 2U }, {  76U, 2U }, {  69U, 2U } };
const sid::step pattern_45[16] = { {   0U, 0U }, {  77U, 0U }, {  81U, 0U }, {  77U, 0U }, {  74U, 0U }, {  77U, 0U }, {  71U, 0U }, {  74U, 0U }, {  77U, 0U }, {  74U, 0U }, {  71U, 0U }, {  74U, 0U }, {  67U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_46[16] = { {  62U, 1U }, {   0U, 1U }, {  50U, 1U }, {   0U, 1U }, {  53U, 1U }, {   0U, 1U }, {  56U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {   1U, 1U }, {   0U, 1U }, {   0U, 1U }, {  65U, 1U }, {  64U, 1U }, {  62U, 1U } };
const sid::step pattern_47[16] = { {   1U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  77U, 2U }, {  81U, 2U }, {  77U, 2U }, {  74U, 2U }, {  77U, 2U }, {  71U, 2U }, {  74U, 2U }, {  77U, 2U }, {  74U, 2U }, {  71U, 2U }, {  74U, 2U }, {  67U, 2U } };
const sid::step pattern_48[16] = { {   0U, 0U }, {  76U, 0U }, {  79U, 0U }, {  76U, 0U }, {  72U, 0U }, {  76U, 0U }, {  69U, 0U }, {  72U, 0U }, {  75U, 0U }, {  72U, 0U }, {  69U, 0U }, {  72U, 0U }, {  66U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_49[16] = { {  60U, 1U }, {   0U, 1U }, {  48U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  54U, 1U }, {   0U, 1U }, {  45U, 1U }, {   0U, 1U }, {   1U, 1U }, {   0U, 1U }, {   0U, 1U }, {  64U, 1U }, {  63U, 1U }, {  61U, 1U } };
const sid::step pattern_50[16] = { {   1U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  76U, 2U }, {  79U, 2U }, {  76U, 2U }, {  72U, 2U }, {  76U, 2U }, {  69U, 2U }, {  72U, 2U }, {  75U, 2U }, {  72U, 2U }, {  69U, 2U }, {  72U, 2U }, {  66U, 2U } };
const sid::step pattern_51[16] = { {   0U, 0U }, {  74U, 0U }, {  77U, 0U }, {  74U, 0U }, {  71U, 0U }, {  74U, 0U }, {  68U, 0U }, {  71U, 0U }, {  74U, 0U }, {  71U, 0U }, {  68U, 0U }, {  71U, 0U }, {  64U, 0U }, {   1U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_52[16] = { {  59U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {  50U, 1U }, {   0U, 1U }, {  53U, 1U }, {   0U, 1U }, {  44U, 1U }, {   0U, 1U }, {   1U, 1U }, {   0U, 1U }, {   0U, 1U }, {  62U, 1U }, {  60U, 1U }, {  59U, 1U } };
const sid::step pattern_53[16] = { {   1U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  74U, 2U }, {  77U, 2U }, {  74U, 2U }, {  71U, 2U }, {  74U, 2U }, {  68U, 2U }, {  71U, 2U }, {  74U, 2U }, {  71U, 2U }, {  68U, 2U }, {  71U, 2U }, {  64U, 2U } };
const sid::step pattern_54[16] = { {   0U, 0U }, {  64U, 0U }, {  69U, 0U }, {  72U, 0U }, {  71U, 0U }, {  64U, 0U }, {  71U, 0U }, {  74U, 0U }, {  72U, 0U }, {   0U, 0U }, {  69U, 0U }, {   0U, 0U }, {  68U, 0U }, {   0U, 0U }, {  64U, 0U }, {   0U, 0U } };
const sid::step pattern_55[16] = { {   1U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {  64U, 2U }, {  69U, 2U }, {  72U, 2U }, {  71U, 2U }, {  64U, 2U }, {  71U, 2U }, {  74U, 2U }, {  72U, 2U }, {   0U, 2U }, {  69U, 2U }, {   0U, 2U }, {  68U, 2U } };
const sid::step pattern_56[16] = { {  69U, 0U }, {  72U, 0U }, {  76U, 0U }, {  72U, 0U }, {  69U, 0U }, {  72U, 0U }, {  66U, 0U }, {  69U, 0U }, {  72U, 0U }, {  69U, 0U }, {  66U, 0U }, {  69U, 0U }, {  63U, 0U }, {  72U, 0U }, {  71U, 0U }, {  69U, 0U } };
const sid::step pattern_57[16] = { {  60U, 1U }, {  64U, 1U }, {  69U, 1U }, {  64U, 1U }, {  60U, 1U }, {  64U, 1U }, {  57U, 1U }, {  60U, 1U }, {  54U, 1U }, {  57U, 1U }, {  60U, 1U }, {  57U, 1U }, {  54U, 1U }, {  57U, 1U }, {  51U, 1U }, {  54U, 1U } };
const sid::step pattern_58[16] = { {   0U, 2U }, {  64U, 2U }, {   0U, 2U }, {  69U, 2U }, {  72U, 2U }, {  76U, 2U }, {  72U, 2U }, {  69U, 2U }, {  72U, 2U }, {  66U, 2U }, {  69U, 2U }, {  72U, 2U }, {  69U, 2U }, {  66U, 2U }, {  69U, 2U }, {  63U, 2U } };
const sid::step pattern_59[16] = { {  68U, 0U }, {  71U, 0U }, {  74U, 0U }, {  71U, 0U }, {  68U, 0U }, {  71U, 0U }, {  62U, 0U }, {  65U, 0U }, {  68U, 0U }, {  65U, 0U }, {  62U, 0U }, {  65U, 0U }, {  59U, 0U }, {  65U, 0U }, {  64U, 0U }, {  62U, 0U } };
const sid::step pattern_60[16] = { {  52U, 1U }, {   0U, 1U }, {  56U, 1U }, {   0U, 1U }, {  59U, 1U }, {   0U, 1U }, {  56U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {  44U, 1U }, {   0U, 1U }, {  40U, 1U }, {   0U, 1U } };
const sid::step pattern_61[16] = { {  72U, 2U }, {  71U, 2U }, {  69U, 2U }, {  68U, 2U }, {  71U, 2U }, {  74U, 2U }, {  71U, 2U }, {  68U, 2U }, {  71U, 2U }, {  62U, 2U }, {  65U, 2U }, {  68U, 2U }, {  65U, 2U }, {  62U, 2U }, {  65U, 2U }, {  59U, 2U } };
const sid::step pattern_62[16] = { {  60U, 0U }, {  64U, 0U }, {  69U, 0U }, {  64U, 0U }, {  60U, 0U }, {  64U, 0U }, {  57U, 0U }, {  60U, 0U }, {  63U, 0U }, {  60U, 0U }, {  57U, 0U }, {  60U, 0U }, {  54U, 0U }, {  60U, 0U }, {  59U, 0U }, {  57U, 0U } };
const sid::step pattern_63[16] = { {  45U, 1U }, {   0U, 1U }, {  48U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  48U, 1U }, {   0U, 1U }, {  45U, 1U }, {   0U, 1U }, {  48U, 1U }, {   0U, 1U }, {  39U, 1U }, {   0U, 1U }, {   1U, 1U }, {   0U, 1U } };
const sid::step pattern_64[16] = { {  65U, 2U }, {  64U, 2U }, {  62U, 2U }, {  60U, 2U }, {  64U, 2U }, {  69U, 2U }, {  64U, 2U }, {  60U, 2U }, {  64U, 2U }, {  57U, 2U }, {  60U, 2U }, {  63U, 2U }, {  60U, 2U }, {  57U, 2U }, {  60U, 2U }, {  54U, 2U } };
const sid::step pattern_65[16] = { {  56U, 0U }, {   0U, 0U }, {  71U, 0U }, {   0U, 0U }, {  68U, 0U }, {   0U, 0U }, {  64U, 0U }, {   0U, 0U }, {   1U, 0U }, {  64U, 0U }, {  69U, 0U }, {  72U, 0U }, {  71U, 0U }, {  64U, 0U }, {  71U, 0U }, {  74U, 0U } };
const sid::step pattern_66[16] = { {   0U, 1U }, {  59U, 1U }, {  56U, 1U }, {  52U, 1U }, {  50U, 1U }, {  59U, 1U }, {  56U, 1U }, {  50U, 1U }, {  48U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  44U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U } };
const sid::step pattern_67[16] = { {  60U, 2U }, {  59U, 2U }, {  57U, 2U }, {  56U, 2U }, {   0U, 2U }, {  71U, 2U }, {   0U, 2U }, {  68U, 2U }, {   0U, 2U }, {  64U, 2U }, {   0U, 2U }, {   1U, 2U }, {  64U, 2U }, {  69U, 2U }, {  72U, 2U }, {  71U, 2U } };
const sid::step pattern_68[16] = { {  72U, 0U }, {  69U, 0U }, {  72U, 0U }, {  76U, 0U }, {  74U, 0U }, {  71U, 0U }, {  74U, 0U }, {  77U, 0U }, {  76U, 0U }, {  72U, 0U }, {  76U, 0U }, {  79U, 0U }, {  77U, 0U }, {  76U, 0U }, {  74U, 0U }, {  72U, 0U } };
const sid::step pattern_69[16] = { {  45U, 1U }, {   0U, 1U }, {  54U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {  56U, 1U }, {   0U, 1U }, {  48U, 1U }, {   0U, 1U }, {  57U, 1U }, {   0U, 1U }, {  50U, 1U }, {   0U, 1U }, {  58U, 1U }, {   0U, 1U } };
const sid::step pattern_70[16] = { {  64U, 2U }, {  71U, 2U }, {  74U, 2U }, {  72U, 2U }, {  69U, 2U }, {  72U, 2U }, {  76U, 2U }, {  74U, 2U }, {  71U, 2U }, {  74U, 2U }, {  77U, 2U }, {  76U, 2U }, {  72U, 2U }, {  76U, 2U }, {  79U, 2U }, {  77U, 2U } };
const sid::step pattern_71[16] = { {  71U, 0U }, {  72U, 0U }, {  74U, 0U }, {  76U, 0U }, {  77U, 0U }, {  74U, 0U }, {  80U, 0U }, {  74U, 0U }, {  83U, 0U }, {  74U, 0U }, {  72U, 0U }, {  81U, 0U }, {  77U, 0U }, {  74U, 0U }, {  71U, 0U }, {  74U, 0U } };
const sid::step pattern_72[16] = { {  56U, 1U }, {   0U, 1U }, {  53U, 1U }, {   0U, 1U }, {  50U, 1U }, {   0U, 1U }, {  47U, 1U }, {   0U, 1U }, {  44U, 1U }, {   0U, 1U }, {  45U, 1U }, {   0U, 1U }, {  38U, 1U }, {   0U, 1U }, {  40U, 1U }, {   0U, 1U } };
const sid::step pattern_73[16] = { {  76U, 2U }, {  74U, 2U }, {  72U, 2U }, {  71U, 2U }, {  72U, 2U }, {  74U, 2U }, {  76U, 2U }, {  77U, 2U }, {  74U, 2U }, {  80U, 2U }, {  74U, 2U }, {  83U, 2U }, {  74U, 2U }, {  72U, 2U }, {  81U, 2U }, {  77U, 2U } };
const sid::step pattern_74[16] = { {  68U, 0U }, {  71U, 0U }, {  72U, 0U }, {  69U, 0U }, {  64U, 0U }, {  69U, 0U }, {  71U, 0U }, {  68U, 0U }, {  69U, 0U }, {  64U, 0U }, {  60U, 0U }, {  64U, 0U }, {  57U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_75[16] = { {  41U, 1U }, {   0U, 1U }, {  39U, 1U }, {   0U, 1U }, {  40U, 1U }, {   0U, 1U }, {  52U, 1U }, {   0U, 1U }, {  45U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U } };
const sid::step pattern_76[16] = { {  74U, 2U }, {  71U, 2U }, {  74U, 2U }, {  68U, 2U }, {  71U, 2U }, {  72U, 2U }, {  69U, 2U }, {  64U, 2U }, {  69U, 2U }, {  71U, 2U }, {  68U, 2U }, {  69U, 2U }, {  64U, 2U }, {  60U, 2U }, {  64U, 2U }, {  57U, 2U } };
const sid::step pattern_77[16] = { {  55U, 3U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_78[16] = { {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U }, {   0U, 0U } };
const sid::step pattern_79[16] = { {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U }, {   0U, 1U } };
const sid::step pattern_80[16] = { {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U }, {   0U, 2U } };

const sid::step* const patterns[] = {
  pattern_0, pattern_1, pattern_2, pattern_3, pattern_4, pattern_5, pattern_6, pattern_7,
  pattern_8, pattern_9, pattern_10, pattern_11, pattern_12, pattern_13, pattern_14, pattern_15,
  pattern_16, pattern_17, pattern_18, pattern_19, pattern_20, pattern_21, pattern_22, pattern_23,
  pattern_24, pattern_25, pattern_26, pattern_27, pattern_28, pattern_29, pattern_30, pattern_31,
  pattern_32, pattern_33, pattern_34, pattern_35, pattern_36, pattern_37, pattern_38, pattern_39,
  pattern_40, pattern_41, pattern_42, pattern_43, pattern_44, pattern_45, pattern_46, pattern_47,
  pattern_48, pattern_49, pattern_50, pattern_51, pattern_52, pattern_53, pattern_54, pattern_55,
  pattern_56, pattern_57, pattern_58, pattern_59, pattern_60, pattern_61, pattern_62, pattern_63,
  pattern_64, pattern_65, pattern_66, pattern_67, pattern_68, pattern_69, pattern_70, pattern_71,
  pattern_72, pattern_73, pattern_74, pattern_75, pattern_76, pattern_77, pattern_78, pattern_79,
  pattern_80
};

// the song, one row per bar: lead, bass, echo, drums
const sid::slot order[] = {
  { 0U,             0 }, { 1U,             0 }, { 2U,             0 }, { 3U,             0 },     // bar 1
  { 4U,             0 }, { 5U,             0 }, { 6U,             0 }, { 3U,             0 },     // bar 2
  { 7U,             0 }, { 8U,             0 }, { 9U,             0 }, { 10U,            0 },     // bar 3
  { 11U,            0 }, { 12U,            0 }, { 13U,            0 }, { 10U,            0 },     // bar 4
  { 14U,            0 }, { 15U,            0 }, { 16U,            0 }, { 10U,            0 },     // bar 5
  { 17U,            0 }, { 18U,            0 }, { 19U,            0 }, { 20U,            0 },     // bar 6
  { 21U,            0 }, { 22U,            0 }, { 23U,            0 }, { 10U,            0 },     // bar 7
  { 24U,            0 }, { 25U,            0 }, { 26U,            0 }, { 10U,            0 },     // bar 8
  { 27U,            0 }, { 28U,            0 }, { 29U,            0 }, { 10U,            0 },     // bar 9
  { 30U,            0 }, { 31U,            0 }, { 32U,            0 }, { 10U,            0 },     // bar 10
  { 33U,            0 }, { 34U,            0 }, { 35U,            0 }, { 10U,            0 },     // bar 11
  { 36U,            0 }, { 37U,            0 }, { 38U,            0 }, { 10U,            0 },     // bar 12
  { 39U,            0 }, { 40U,            0 }, { 41U,            0 }, { 20U,            0 },     // bar 13
  { 42U,            0 }, { 43U,            0 }, { 44U,            0 }, { 10U,            0 },     // bar 14
  { 45U,            0 }, { 46U,            0 }, { 47U,            0 }, { 10U,            0 },     // bar 15
  { 48U,            0 }, { 49U,            0 }, { 50U,            0 }, { 10U,            0 },     // bar 16
  { 51U,            0 }, { 52U,            0 }, { 53U,            0 }, { 20U,            0 },     // bar 17
  { 54U,            0 }, { 5U,             0 }, { 55U,            0 }, { 10U,            0 },     // bar 18
  { 56U,            0 }, { 57U,            0 }, { 58U,            0 }, { 10U,            0 },     // bar 19
  { 59U,            0 }, { 60U,            0 }, { 61U,            0 }, { 10U,            0 },     // bar 20
  { 62U,            0 }, { 63U,            0 }, { 64U,            0 }, { 10U,            0 },     // bar 21
  { 65U,            0 }, { 66U,            0 }, { 67U,            0 }, { 10U,            0 },     // bar 22
  { 68U,            0 }, { 69U,            0 }, { 70U,            0 }, { 10U,            0 },     // bar 23
  { 71U,            0 }, { 72U,            0 }, { 73U,            0 }, { 20U,            0 },     // bar 24
  { 74U,            0 }, { 75U,            0 }, { 76U,            0 }, { 77U,            0 },     // bar 25
  { 78U,            0 }, { 79U,            0 }, { 80U,            0 }, { sid::no_pattern, 0 }      // bar 26
};

const sid::song song = { patches, patterns, order, 26U, 4U, 16U, 110U };

} // namespace bach

#endif  // _SID_EXAMPLE_INVENTION13_H_
