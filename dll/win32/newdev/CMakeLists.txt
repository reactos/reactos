


add_definitions(-DUNICODE -D_UNICODE)

spec2def(newdev ${CMAKE_CURRENT_SOURCE_DIR}/newdev.spec ${CMAKE_CURRENT_BINARY_DIR}/newdev.def)

add_library(newdev SHARED
    newdev.c
    stubs.c
    wizard.c
    newdev.rc)

set_target_properties(newdev PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(newdev
    ${CMAKE_CURRENT_BINARY_DIR}/newdev.def
    wine
    mingw_common
    -lgdi32
    -lcomctl32
    -lsetupapi
    -ladvapi32
    -luser32
    -lshell32)

add_dependencies(newdev newdev_def psdk)
