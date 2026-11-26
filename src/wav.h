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
// \brief sid - WAV file reader for a file in memory (flash or RAM)
//        parse() finds the format and the samples of a RIFF / WAVE file: uncompressed PCM, 8 bit (unsigned) or
//        16 bit (signed, little endian), mono or stereo, any sample rate. Everything else is refused. The
//        samples are not copied: info.data points into the file, at any alignment.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_WAV_H_
#define _SID_WAV_H_

#include <cstdint>
#include <cstddef>


namespace sid {
namespace wav {

// format and samples of a WAV file
typedef struct tag_info {
  std::uint32_t         rate;       // sample rate [Hz]
  std::uint8_t          channels;   // 1 = mono, 2 = stereo
  std::uint8_t          bits;       // 8 (unsigned) or 16 (signed, little endian)
  const std::uint8_t*   data;       // the samples, interleaved
  std::uint32_t         frames;     // number of frames (one sample per channel)
} info;


namespace detail {

inline std::uint32_t le16(const std::uint8_t* p)
{ return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U); }

inline std::uint32_t le32(const std::uint8_t* p)
{ return le16(p) | (le16(p + 2) << 16U); }

inline bool tag(const std::uint8_t* p, const char* id)
{ return (p[0] == static_cast<std::uint8_t>(id[0])) && (p[1] == static_cast<std::uint8_t>(id[1])) &&
         (p[2] == static_cast<std::uint8_t>(id[2])) && (p[3] == static_cast<std::uint8_t>(id[3])); }

} // namespace detail


/**
 * Read the header of a WAV file
 * \param file The file
 * \param size Size of the file [bytes]
 * \param result Format and samples, valid if true is returned
 * \return False if this is no WAV file or a format that is not supported
 */
inline bool parse(const std::uint8_t* file, std::size_t size, info& result)
{
  if ((file == nullptr) || (size < 12U) || !detail::tag(file, "RIFF") || !detail::tag(file + 8, "WAVE")) {
    return false;
  }
  bool        format = false;
  std::size_t pos = 12U;
  // the chunks: 4 bytes id, 4 bytes size, the content, padded to an even size
  while ((size - pos) >= 8U) {
    const std::uint8_t* const chunk = file + pos;
    const std::uint32_t length = detail::le32(chunk + 4);
    const std::size_t   room   = size - pos - 8U;
    if (detail::tag(chunk, "fmt ")) {
      if ((length < 16U) || (room < 16U)) {
        return false;
      }
      const std::uint32_t coding   = detail::le16(chunk + 8);
      const std::uint32_t channels = detail::le16(chunk + 10);
      const std::uint32_t bits     = detail::le16(chunk + 22);
      // 1 = PCM; 0xFFFE = extensible: the coding is the first word of the sub format at offset 24 of the content
      const bool pcm = (coding == 1U) ||
                       ((coding == 0xFFFEU) && (length >= 40U) && (room >= 40U) && (detail::le16(chunk + 32) == 1U));
      result.rate = detail::le32(chunk + 12);
      if (!pcm || ((channels != 1U) && (channels != 2U)) || ((bits != 8U) && (bits != 16U)) || (result.rate == 0U) ||
          (detail::le16(chunk + 20) != (channels * (bits / 8U)))) {
        return false;
      }
      result.channels = static_cast<std::uint8_t>(channels);
      result.bits     = static_cast<std::uint8_t>(bits);
      format = true;
    }
    else if (detail::tag(chunk, "data")) {
      if (!format) {
        return false;     // the format comes first
      }
      // a file that was cut off plays as far as it goes
      const std::size_t bytes = (length < room) ? length : room;
      result.data   = chunk + 8;
      result.frames = static_cast<std::uint32_t>(bytes / (static_cast<std::size_t>(result.channels) * (result.bits / 8U)));
      return true;
    }
    if (length > room) {
      return false;
    }
    pos += 8U + length + (length & 1U);
    if (pos > size) {
      return false;
    }
  }
  return false;   // no samples
}

} // namespace wav
} // namespace sid

#endif  // _SID_WAV_H_
