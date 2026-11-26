# sid examples: one small program per topic
# Each writes <name>.wav into the folder it is started in; with --play it plays live (Windows), with --stdout it
# writes raw samples for a pipe into a player (Linux, macOS) - but 00_hello, the plain program of the readme.
# ctest runs them all (they write into wav/ below the build folder of the examples), so the examples are tested and
# their sound can be listened to.

set(SID_EXAMPLES
  00_hello
  01_beep
  02_melody
  03_wav_clip
  04_custom_source
  05_patches
  06_filter
  07_ring_and_sync
  08_combined_waveforms
  09_arpeggio_chords
  10_drum_pattern
  11_sound_effects
  12_song
  13_loop_and_notes
  14_mod_player
  15_rtttl
  16_sfx
  17_mixer
  18_meter
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wav")

foreach(example ${SID_EXAMPLES})
  add_executable(sid_example_${example} ${example}.cpp)
  target_link_libraries(sid_example_${example} PRIVATE sid::sid)
  target_include_directories(sid_example_${example} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  sid_warnings(sid_example_${example})
  add_test(NAME example_${example} COMMAND sid_example_${example} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wav")
endforeach()
