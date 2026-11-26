#!/usr/bin/env python3
# \author (c) Marco Paland (info@paland.design)
#             2026, PALANDesign, LLC
#
# \license The MIT License (MIT)
#          This file is part of the sid library (https://github.com/mpaland/sid), see LICENSE.
#
# \brief Makes invention13.h, the song of the examples, for the synthesizer of the sid library:
#        J. S. Bach, Invention 13 in A minor (BWV 784), arranged the way the home computers of the 80s played
#        Bach: the right hand as a pulse lead with a quiet echo three steps behind it, the left hand as a
#        sawtooth bass through the resonant low pass, and drums.
#        The piece is in the public domain. The notes below are those of the Mutopia Project edition
#        (www.mutopiaproject.org, Mutopia-2008/06/15-59, typeset by Allen Garvin after the Bach-Gesellschaft
#        edition and placed in the public domain by the typesetter), in LilyPond notation: note names in Dutch
#        (fis = F sharp, bes = B flat), every note within a fourth of the one before it, ' and , move it by an
#        octave, a number is the note value (16 = one step of the song), ~ ties two notes, | ends a bar.
#        Usage: python make_invention13.py   (writes invention13.h)

import os
import re

STEP_MS       = 110     # one sixteenth note
STEPS_PER_BAR = 16
ECHO_STEPS    = 3       # the echo follows the lead by a dotted eighth

RIGHT_HAND = (4, """
   r16  e a c  b e, b' d  c8 e gis, e' |
   a,16 e a c  b e, b' d  c8 a r4 |
   r16  e'16 c e  a, c e, g  f8 a d f ~ |
   f16 d b d  g, b d, f  e8 g c e ~ |
   e16 c a c  f,8 d' ~  d16 b g b  e,8 c' ~ |
   c16 a f a  d,8 b' c r r4 |
   r16  g c e  d g, d' f  e8 g b, g' |
   c,16 g c e  d g, d' f  e8 c g' e |
   c'16 a e a  c, e a, c  d8 fis a c |
   b16 g d g  b, d g, b  c8 e g b |
   a16 fis dis fis  b, dis fis, a  g8 g' ~  g16 e c e |
   a,8 fis' ~  fis16 d b d  g,8 e' ~  e16 c a c |
   fis,16 g' fis e  dis fis b, dis e8 r r4 |
   r16  g bes g  e g cis, e  g e cis e a, r r8 |
   r16  f' a f  d f b, d  f d b d g, r r8 |
   r16  e' g e  c e a, c  dis c a c fis, r r8 |
   r16  d' f d  b d gis, b  d b gis b e, r r8 |
   r16  e a c  b e, b' d  c8 a gis e |
   a16 c e c  a c fis, a  c a fis a  dis, c' b a |
   gis b d b  gis b d, f  gis f d f  b, f' e d |
   c e a e  c e a, c  dis c a c  fis, c' b a |
   gis8 b' gis e r16  e a c  b e, b' d  |
   c a c e  d b d f  e c e g  f e d c |
   b c d e  f d gis d  b' d, c a'  f d b d |
   gis, b c a  e a b gis  a e c e a,4
""")

LEFT_HAND = (3, """
   a8 a'4 gis8  a16 e a c  b e, b' d |
   c8 a gis e  a16 e a c  b e, b' d |
   c8 a c a  d16 a f a  d, f a, c |
   b8 d g b ~  b16 g e g  c, e g, b |
   a8 c  d16 f b, d  g,8 b  c16 e a, c |
   f,8 d  g16 g' f g  c, g' c e  d g, d' f |
   e8 c b g  c16 g c e  d g, d' f |
   e8 c r4 r16  g' e g  c, e g, b |
   a8 c e g  fis16 a d, fis  a, d fis, a |
   g8 b d fis  e16 g c, e  g, c e, g |
   fis8 a b dis r16  e c e  a, c e g |
   fis d b d  g, b d fis  e c a c  fis, a c8 ~ |
   c16 b c a  b8 b,  e16 e' b g  e b g b |
   e,8 e' g bes cis,8 r r16  g'' f e |
   d8 d, f aes b, r r16  f'' e d |
   c8 c, e fis a, r r16  e'' dis cis |
   b8 b, d f gis, r r16  d'' c b |
   c8 a gis e  a16 e a c  b e, b' d |
   c e a e  c e a, c  fis, a c a  fis a dis, fis |
   e8 gis b gis  e b gis e |
   a c e c  a c dis, r |
   r16  b'' gis e  d b' gis d  c8 e gis, e' |
   a, fis' b, gis'  c, a' d, bes' |
   gis f d b  gis a d, e |
   f dis e e' a,2
""")

MIT = '''///////////////////////////////////////////////////////////////////////////////
// \\author (c) Marco Paland (info@paland.design)
//             2026, PALANDesign, LLC
//
// \\license The MIT License (MIT)
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
'''

HOLD = 0    # sid::key::hold
OFF  = 1    # sid::key::off

NAMES     = 'cdefgab'
SEMITONES = (0, 2, 4, 5, 7, 9, 11)


def keys_of(hand):
    """The steps of one hand: the key of a note where it starts, OFF where a rest starts, HOLD elsewhere."""
    octave, text = hand
    index = 0             # the note before: c
    steps = []
    duration = 16
    tied = False
    sounding = False
    for token in text.split():
        if token == '|':
            assert len(steps) % STEPS_PER_BAR == 0, 'bar %d is not full' % (len(steps) // STEPS_PER_BAR + 1)
            continue
        if token == '~':
            tied = True
            continue
        m = re.match(r"^(r|[a-g])(is|es)?([',]*)(\d+)?$", token)
        assert m, token
        if m.group(4):
            duration = int(m.group(4))
        length = STEPS_PER_BAR // duration
        if m.group(1) == 'r':
            steps += [OFF if sounding else HOLD] + [HOLD] * (length - 1)
            sounding = False
            continue
        now = NAMES.index(m.group(1))
        if now - index > 3:
            octave -= 1
        elif now - index < -3:
            octave += 1
        octave += m.group(3).count("'") - m.group(3).count(',')
        index = now
        key = 12 * (octave + 1) + SEMITONES[now] + {'is': 1, 'es': -1, None: 0}[m.group(2)]
        steps += [HOLD if tied else key] + [HOLD] * (length - 1)
        tied = False
        sounding = True
    return steps


def key_name(key):
    if key == HOLD:
        return 'H'
    if key == OFF:
        return 'O'
    return '%3d' % key


def main():
    lead = keys_of(RIGHT_HAND)
    bass = keys_of(LEFT_HAND)
    bars = len(lead) // STEPS_PER_BAR
    assert len(lead) == len(bass) == bars * STEPS_PER_BAR == 25 * STEPS_PER_BAR, (len(lead), len(bass))

    # a last bar in which the closing notes ring on, and the echo: the lead again, some steps later
    lead += [HOLD] * STEPS_PER_BAR
    bass += [HOLD] * STEPS_PER_BAR
    rows = bars + 1
    echo = ([HOLD] * ECHO_STEPS + lead)[:len(lead)]

    # drums: kick, snare and hat on one voice
    K, S, T = (55, 3), (72, 4), (100, 5)       # (key, patch)
    N = (HOLD, 0)
    drums = {
        'intro': [N, N, N, N, T, N, N, N, N, N, N, N, T, N, T, N],
        'beat':  [K, N, T, N, S, N, T, N, K, N, T, N, S, N, T, N],
        'fill':  [K, N, T, N, S, N, T, N, K, N, K, N, S, S, S, S],
        'end':   [K, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N],
    }
    fills = (6, 13, 17, 24)     # the bars before a new part
    drum_of = []
    for bar in range(1, rows + 1):
        drum_of.append('intro' if bar <= 2 else 'end' if bar == bars else None if bar > bars else 'fill' if bar in fills else 'beat')

    # patterns: one per bar and voice, equal ones only once
    patterns = []
    def pattern(steps):
        if steps not in patterns:
            patterns.append(steps)
        return patterns.index(steps)

    order = []
    for row in range(rows):
        cut = slice(row * STEPS_PER_BAR, (row + 1) * STEPS_PER_BAR)
        slots = [pattern([(k, 0) for k in lead[cut]]),
                 pattern([(k, 1) for k in bass[cut]]),
                 pattern([(k, 2) for k in echo[cut]]),
                 pattern(drums[drum_of[row]]) if drum_of[row] else None]
        order.append(slots)
    assert len(patterns) < 255

    out = []
    w = out.append
    for line in MIT.split('\n')[:-1]:
        w(line)
    w('// \\brief sid examples: a song for the synthesizer and the sequencer (sid/src/sequencer.h)')
    w('//        J. S. Bach, Invention 13 in A minor (BWV 784), the way the home computers of the 80s played Bach:')
    w('//        the right hand as a pulse lead with a quiet echo, the left hand as a sawtooth bass through the')
    w('//        resonant low pass, and drums. %d rows of one bar (%d steps of %d ms), %d s.'
      % (rows, STEPS_PER_BAR, STEP_MS, round(rows * STEPS_PER_BAR * STEP_MS / 1000.0)))
    w('//        The piece is in the public domain; the notes are those of the Mutopia Project edition')
    w('//        (Mutopia-2008/06/15-59, typeset by Allen Garvin after the Bach-Gesellschaft edition, placed in the')
    w('//        public domain by the typesetter).')
    w('//        Made by make_invention13.py, which holds the notes and the arrangement - change the song there.')
    w('//')
    w('///////////////////////////////////////////////////////////////////////////////')
    w('')
    w('#ifndef _SID_EXAMPLE_INVENTION13_H_')
    w('#define _SID_EXAMPLE_INVENTION13_H_')
    w('')
    w('#include "sid/src/sequencer.h"')
    w('')
    w('')
    w('namespace bach {')
    w('')
    w('// the instruments')
    w('const sid::patch patches[] = {')
    w('  // wave            level attack decay sustain release pulse pwm  lfo: rate delay vibrato  sweep glide  arpeggio     ms  mod            pitch  filter           reson. cutoff end  ms')
    w('  { sid::osc_pulse,    66U,    2U, 180U,  150U,   220U,   80U, 30U,      45U,  30U,    14U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // lead')
    w('  { sid::osc_saw,      90U,    1U, 140U,  120U,   200U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_low,  150U, 2400U, 600U, 120U },   // bass')
    w('  { sid::osc_pulse,    20U,    4U, 180U,  130U,   220U,  128U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // echo')
    w('  { sid::osc_triangle, 110U,   0U, 140U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,  -280,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // kick')
    w('  { sid::osc_noise,    56U,    0U, 120U,    0U,    40U,    0U,  0U,       0U,   0U,     0U,   -50,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U },   // snare')
    w('  { sid::osc_noise,    26U,    0U,  35U,    0U,    20U,    0U,  0U,       0U,   0U,     0U,     0,   0U, { 0U, 0U },  0U, sid::mod_none,     0, sid::filter_off,    0U,    0U,   0U,   0U }    // hat')
    w('};')
    w('')
    w('// the patterns: { key, patch } per step; key 0 = the note goes on (sid::key::hold), 1 = it ends (sid::key::off)')
    for i, steps in enumerate(patterns):
        w('const sid::step pattern_%d[%d] = { %s };' % (i, STEPS_PER_BAR, ', '.join('{ %3dU, %dU }' % s for s in steps)))
    w('')
    w('const sid::step* const patterns[] = {')
    for i in range(0, len(patterns), 8):
        w('  ' + ', '.join('pattern_%d' % n for n in range(i, min(i + 8, len(patterns)))) + (',' if i + 8 < len(patterns) else ''))
    w('};')
    w('')
    w('// the song, one row per bar: lead, bass, echo, drums')
    w('const sid::slot order[] = {')
    for row, slots in enumerate(order):
        cells = ['{ %-15s 0 }' % (('%dU,' % s) if s is not None else 'sid::no_pattern,') for s in slots]
        w('  ' + ', '.join(cells) + (',' if row + 1 < rows else ' ') + '     // bar %d' % (row + 1))
    w('};')
    w('')
    w('const sid::song song = { patches, patterns, order, %dU, 4U, %dU, %dU };' % (rows, STEPS_PER_BAR, STEP_MS))
    w('')
    w('} // namespace bach')
    w('')
    w('#endif  // _SID_EXAMPLE_INVENTION13_H_')

    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'invention13.h')
    with open(path, 'w', encoding='utf-8', newline='') as f:
        f.write('\n'.join(out) + '\n')
    print('invention13.h: %d rows, %d patterns, %d s' % (rows, len(patterns), round(rows * STEPS_PER_BAR * STEP_MS / 1000.0)))
    print('lead bar 1:', ' '.join(key_name(k) for k in lead[:16]))
    print('bass bar 1:', ' '.join(key_name(k) for k in bass[:16]))
    print('lead range', min(k for k in lead if k > 1), max(lead), 'bass range', min(k for k in bass if k > 1), max(bass))
    print('last notes: lead', [k for k in lead if k > 1][-1], 'bass', [k for k in bass if k > 1][-1])


if __name__ == '__main__':
    main()
