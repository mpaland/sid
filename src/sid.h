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
// \brief sid - Sound Interface Driver library: all of it in one include
//        Sound for small systems in the style of the home computers of the 80s: tones, melodies, recorded
//        clips, a synthesizer and a sequencer. Header-only C++11, integer only, no heap.
//          source.h      sid::source (something that makes sound), sid::device (the output of the platform, the
//                        only thing to port), sid::output (plays sources on a device)
//          tone.h        sid::tone, sid::melody, sid::beep(), sid::play(notes)
//          wav.h         sid::wav::parse(): a WAV file in memory
//          player.h      sid::pcm, sid::play_wav()
//          synth.h       sid::synth, sid::patch, the keys sid::key::c1 .. b7
//          sequencer.h   sid::sequencer, sid::song, sid::play(song)
//        The devices in port/ are not included here: take the one of your platform.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_H_
#define _SID_H_

#include "source.h"
#include "tone.h"
#include "wav.h"
#include "player.h"
#include "synth.h"
#include "sequencer.h"

#endif  // _SID_H_
