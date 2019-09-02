
add_custom_target(spec2def_test
    py -3 ${CMAKE_CURRENT_SOURCE_DIR}/test.py $<TARGET_FILE:native-spec2def>)
