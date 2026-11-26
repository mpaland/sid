#!/bin/sh
# \author (c) Marco Paland (info@paland.design)
#             2026, PALANDesign, LLC
#
# \license The MIT License (MIT)
#          This file is part of the sid library (https://github.com/mpaland/sid), see LICENSE.
#
# \brief Compile the library for a bare Cortex-M and print its sizes: the embedded check of CI
#        test/embedded.cpp is compiled with all warnings as errors, without exceptions and run time type
#        information, for a Cortex-M0 and a Cortex-M4, once per part of the library. The table shows the code
#        (text + data of the object file) per part and the RAM of the objects (sizeof).
#        Usage (from the root of the repository): sh tools/embedded_size.sh [prefix of the compiler]
#          sh tools/embedded_size.sh                       arm-none-eabi-g++ from the PATH
#          sh tools/embedded_size.sh /opt/gcc/bin/arm-none-eabi-

PREFIX=${1:-arm-none-eabi-}
OUT=${TMPDIR:-/tmp}/sid-embedded
mkdir -p "$OUT" || exit 1

FLAGS="-std=c++11 -mthumb -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -Wall -Wextra -Wshadow -Wpedantic -Werror"

echo "code [bytes]             Cortex-M0  Cortex-M4"
for part in 1 2 3 4 0 5 6 7 8 9; do
  case $part in
    1) name="tones and melodies    " ;;
    2) name="WAV clips             " ;;
    3) name="synthesizer           " ;;
    4) name="synthesizer, sequencer" ;;
    0) name="all of it             " ;;
    5) name="ext: MOD player       " ;;
    6) name="ext: mixer, two tones " ;;
    7) name="ext: RTTTL player     " ;;
    8) name="ext: two sound effects" ;;
    9) name="ext: level meter      " ;;
  esac
  line="$name"
  for cpu in cortex-m0 cortex-m4; do
    obj="$OUT/embedded-$cpu-$part.o"
    "${PREFIX}g++" $FLAGS -mcpu=$cpu -DSID_EMBEDDED_PART=$part -c test/embedded.cpp -o "$obj" || exit 1
    size=$("${PREFIX}size" "$obj" | awk 'NR == 2 { print $1 + $2 }')
    line="$line  $(printf '%9s' "$size")"
  done
  echo "$line"
done

echo
echo "RAM of the objects [bytes] (Cortex-M4)"
obj="$OUT/embedded-sizes.o"
"${PREFIX}g++" $FLAGS -mcpu=cortex-m4 -DSID_EMBEDDED_SIZES -c test/embedded.cpp -o "$obj" || exit 1
"${PREFIX}nm" -S "$obj" | grep size_of_ | sort -k 4 | while read -r address size type name; do
  printf '  %-13s %5d\n' "${name#size_of_}" "0x$size"
done
