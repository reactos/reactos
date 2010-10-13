
set_unicode()

spec2def(appwiz ${CMAKE_CURRENT_SOURCE_DIR}/appwiz.spec ${CMAKE_CURRENT_BINARY_DIR}/appwiz.def)

add_library(appwiz SHARED
    appwiz.c
    createlink.c
    appwiz.rc)

set_module_type(appwiz cpl)

target_link_libraries(appwiz ${CMAKE_CURRENT_BINARY_DIR}/appwiz.def uuid)

add_importlibs(appwiz
    msvcrt
    user32
    ole32
    advapi32
    comctl32
    shell32
    kernel32)

add_dependencies(appwiz appwiz_def)
