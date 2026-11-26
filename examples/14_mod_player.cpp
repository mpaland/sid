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
// \brief sid example 14: a MOD player (ext/mod.h)
//          sid_example_14_mod_player song.mod            the file into 14_mod_player.wav
//          sid_example_14_mod_player song.mod --play     live (Windows)
//          sid_example_14_mod_player                     a short tune made here, as a MOD file in memory
//        It shows the title and the samples of the file and where the song is while it plays. The output plays
//        the song in slices of 100 ms, the way an app keeps control while music plays.
//        The file is read into memory here; on a microcontroller it is a constant array in the flash
//        (tools/wav2h.py makes such a header of any file).
//
///////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <vector>

#include "sid/src/ext/mod.h"

#include "common.h"


// the MOD file that plays
static std::vector<std::uint8_t> file;


// a small tune of its own as a MOD file: four bars in A minor, bass, lead, arpeggio chords and drums. The samples
// are computed: a sawtooth, a square, a narrow pulse, a kick drum and a hi-hat
static std::vector<std::uint8_t> demo()
{
  enum { BASS = 1, LEAD, CHORD, KICK, HAT };
  static const std::uint16_t period[36] = {     // C-1 .. B-3
    856U, 808U, 762U, 720U, 678U, 640U, 604U, 570U, 538U, 508U, 480U, 453U,
    428U, 404U, 381U, 360U, 340U, 320U, 302U, 285U, 269U, 254U, 240U, 226U,
    214U, 202U, 190U, 180U, 170U, 160U, 151U, 143U, 135U, 127U, 120U, 113U
  };
  std::vector<std::uint8_t> mod(1084U + 1024U, 0U);
  const char title[] = "sid demo";
  for (std::size_t i = 0U; i < (sizeof(title) - 1U); ++i) {
    mod[i] = static_cast<std::uint8_t>(title[i]);
  }
  mod[950] = 2U;              // the pattern twice
  mod[951] = 127U;
  mod[1080] = 'M';  mod[1081] = '.';  mod[1082] = 'K';  mod[1083] = '.';

  // the samples, one after the other behind the pattern
  std::vector<std::int8_t> samples[5];
  for (std::size_t i = 0U; i < 64U; ++i) {
    samples[0].push_back(static_cast<std::int8_t>(static_cast<int>(i * 4U) - 128));            // sawtooth
  }
  for (std::size_t i = 0U; i < 32U; ++i) {
    samples[1].push_back(static_cast<std::int8_t>((i < 16U) ? 90 : -90));                       // square
    samples[2].push_back(static_cast<std::int8_t>((i < 8U) ? 90 : -30));                        // pulse 1 / 4
  }
  std::uint32_t phase = 0U;
  std::uint32_t random = 12345U;
  for (std::size_t i = 0U; i < 3000U; ++i) {
    // kick: a sine falling from 180 to 50 Hz (at the rate of C-2, 8287 Hz), fading
    phase += static_cast<std::uint32_t>((50U + ((130U * (3000U - i)) / 3000U)) * 518250U);     // 2^32 / 8287
    const std::int32_t fade = static_cast<std::int32_t>(3000U - i);
    samples[3].push_back(static_cast<std::int8_t>((sid::detail::sine(phase) / 256 * fade) / 3000));
  }
  for (std::size_t i = 0U; i < 1200U; ++i) {
    random = (random * 1103515245U) + 12345U;
    const std::int32_t noise = static_cast<std::int32_t>((random >> 16U) & 0xFFU) - 128;
    samples[4].push_back(static_cast<std::int8_t>((noise * static_cast<std::int32_t>(1200U - i)) / 1200));
  }
  const std::uint8_t volume[5] = { 48U, 34U, 22U, 64U, 30U };
  for (std::size_t s = 0U; s < 5U; ++s) {
    std::uint8_t* const h = &mod[20U + (s * 30U)];
    const std::size_t words = samples[s].size() / 2U;
    h[22] = static_cast<std::uint8_t>(words >> 8U);
    h[23] = static_cast<std::uint8_t>(words);
    h[25] = volume[s];
    h[29] = 1U;
    if (s < 3U) {
      h[28] = static_cast<std::uint8_t>(words >> 8U);     // the waveforms loop over all of it
      h[29] = static_cast<std::uint8_t>(words);
    }
    for (std::size_t i = 0U; i < samples[s].size(); ++i) {
      mod.push_back(static_cast<std::uint8_t>(samples[s][i]));
    }
  }

  // one cell of the pattern; note: index of the period, -1 = no note
  struct cell_writer {
    std::vector<std::uint8_t>& m;
    void operator()(std::size_t row, std::size_t channel, std::uint32_t number, int note, std::uint32_t effect = 0U,
                    std::uint32_t param = 0U) const
    {
      const std::uint32_t per = (note < 0) ? 0U : period[note];
      std::uint8_t* const c = &m[1084U + (row * 16U) + (channel * 4U)];
      c[0] = static_cast<std::uint8_t>((number & 0xF0U) | (per >> 8U));
      c[1] = static_cast<std::uint8_t>(per);
      c[2] = static_cast<std::uint8_t>(((number & 0x0FU) << 4U) | effect);
      c[3] = static_cast<std::uint8_t>(param);
    }
  };
  const cell_writer cell = { mod };

  // the bars: Am, F, C, G - root of the bass (C-1 + n), arpeggio of the chord, four notes of the lead
  static const int          root[4]   = { 9, 5, 0, 7 };
  static const std::uint8_t chord[4]  = { 0x37U, 0x47U, 0x47U, 0x47U };
  static const int          lead[4][4] = { { 28, 24, 21, 24 }, { 33, 29, 24, 29 }, { 31, 28, 24, 28 }, { 31, 26, 23, 26 } };
  for (std::size_t bar = 0U; bar < 4U; ++bar) {
    const std::size_t at = bar * 16U;
    static const std::size_t bass_rows[6] = { 0U, 3U, 6U, 8U, 11U, 14U };
    for (std::size_t i = 0U; i < 6U; ++i) {
      cell(at + bass_rows[i], 0U, BASS, root[bar] + (((i % 2U) == 1U) ? 12 : 0));
    }
    for (std::size_t i = 0U; i < 4U; ++i) {
      cell(at + (i * 4U), 1U, LEAD, lead[bar][i]);
      cell(at + (i * 4U) + 2U, 1U, 0U, -1, 0x4U, 0x46U);       // vibrato on the held note
      cell(at + (i * 4U) + 3U, 1U, 0U, -1, 0x6U, 0x02U);       // and fading
    }
    cell(at, 2U, CHORD, root[bar] + 12, 0x0U, chord[bar]);
    cell(at + 8U, 2U, CHORD, root[bar] + 12, 0x0U, chord[bar]);
    for (std::size_t r = 1U; r < 16U; ++r) {
      if (r != 8U) {
        cell(at + r, 2U, 0U, -1, 0x0U, chord[bar]);             // the arpeggio goes on
      }
    }
    cell(at, 3U, KICK, 12);
    cell(at + 8U, 3U, KICK, 12);
    cell(at + 10U, 3U, KICK, 12, 0xCU, 40U);
    for (std::size_t r = 2U; r < 16U; r += 4U) {
      cell(at + r, 3U, HAT, 24);
    }
    cell(at + 4U, 3U, HAT, 22, 0xCU, 64U);
    cell(at + 12U, 3U, HAT, 22, 0xCU, 64U);
  }
  return mod;
}


// a whole file into memory
static bool load(const char* path, std::vector<std::uint8_t>& data)
{
  std::FILE* f = nullptr;
#if defined(_MSC_VER)
  if (fopen_s(&f, path, "rb") != 0) {
    f = nullptr;
  }
#else
  f = std::fopen(path, "rb");
#endif
  if (f == nullptr) {
    return false;
  }
  unsigned char part[4096];
  std::size_t n;
  while ((n = std::fread(part, 1U, sizeof(part), f)) != 0U) {
    data.insert(data.end(), part, part + n);
  }
  const bool ok = (std::ferror(f) == 0);
  (void)std::fclose(f);
  return ok;
}


static bool play(sid::output& out)
{
  sid::mod::info song;
  if (file.empty() || !sid::mod::parse(&file[0], file.size(), song)) {
    std::fprintf(stderr, "14_mod_player: no MOD file of 4 channels and 31 samples\n");
    return false;
  }

  char title[21];
  sid::mod::title(song, title);
  std::fprintf(stderr, "\"%s\": %u positions, %u patterns\n", title, static_cast<unsigned>(song.length),
               static_cast<unsigned>(song.patterns));
  for (std::uint32_t n = 1U; n <= 31U; ++n) {
    const sid::mod::sample s = sid::mod::sample_of(song, n);
    if (s.length == 0U) {
      continue;
    }
    char name[23];
    for (std::size_t i = 0U; i < 22U; ++i) {
      const char c = static_cast<char>(file[20U + ((n - 1U) * 30U) + i]);
      name[i] = ((c == '\0') || ((c >= ' ') && (c <= '~'))) ? c : '.';
    }
    name[22] = '\0';
    std::fprintf(stderr, "  %2u  %-22s %6u bytes  volume %2u%s\n", static_cast<unsigned>(n), name,
                 static_cast<unsigned>(s.length), static_cast<unsigned>(s.volume), (s.loop_length != 0U) ? "  loop" : "");
  }

  if (!out.open(example::rate)) {
    return false;
  }
  sid::mod::player music(out.rate(), song);
  bool ok = true;
  std::uint32_t seconds = 0U;
  for (std::uint32_t slices = 0U; ok && !music.ended(); ++slices) {
    ok = out.play(music, 100U);
    if ((slices % 10U) == 0U) {
      std::fprintf(stderr, "\r  %2u:%02u  position %3u of %u  pattern %3u  row %2u ", static_cast<unsigned>(seconds / 60U),
                   static_cast<unsigned>(seconds % 60U), static_cast<unsigned>(music.order() + 1U),
                   static_cast<unsigned>(song.length), static_cast<unsigned>(file[952U + music.order()]),
                   static_cast<unsigned>(music.row()));
      ++seconds;
    }
  }
  std::fprintf(stderr, "\n");
  out.close();
  return ok;
}


int main(int argc, char** argv)
{
  for (int i = 1; i < argc; ++i) {
    if ((argv[i][0] != '-') && file.empty()) {
      if (!load(argv[i], file)) {
        std::fprintf(stderr, "14_mod_player: cannot read %s\n", argv[i]);
        return 1;
      }
    }
  }
  if (file.empty()) {
    file = demo();
  }
  return example::run(argc, argv, "14_mod_player", &play);
}
