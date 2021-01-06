
add_custom_target(spec2def_test
    py -3 ${CMAKE_CURRENT_SOURCE_DIR}/test.py $<TARGET_PROPERTY:native-spec2def,IMPORTED_LOCATION>)
