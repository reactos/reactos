
set_unicode()

spec2def(joy ${CMAKE_CURRENT_SOURCE_DIR}/joy.spec ${CMAKE_CURRENT_BINARY_DIR}/joy.def)

add_library(joy SHARED joy.c joy.rc)

set_module_type(joy cpl)

target_link_libraries(joy ${CMAKE_CURRENT_BINARY_DIR}/joy.def uuid)

add_importlibs(joy
    user32
    comctl32
    ole32
    shell32)

add_dependencies(joy joy_def)
add_cab_target(joy 1)
