# sid tests: one program with all test cases, run by ctest
#   sid_test                  all cases
#   sid_test --filter synth   the cases whose name starts with "synth"

add_executable(sid_test
  main.cpp
  test_tone.cpp
  test_synth.cpp
  test_port.cpp
  test_mod.cpp
  test_meter.cpp
  test_mixer.cpp
  test_rtttl.cpp
  test_sfx.cpp
)
target_link_libraries(sid_test PRIVATE sid::sid)
sid_warnings(sid_test)

# the port tests write their files into the working directory
add_test(NAME sid_test COMMAND sid_test WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
