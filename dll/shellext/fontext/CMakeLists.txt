
set_unicode()

spec2def(fontext ${CMAKE_CURRENT_SOURCE_DIR}/fontext.spec ${CMAKE_CURRENT_BINARY_DIR}/fontext.def)

add_library(fontext SHARED
    fontext.c
    regsvr.c
    fontext.rc)

set_module_type(fontext win32dll)

target_link_libraries(fontext ${CMAKE_CURRENT_BINARY_DIR}/fontext.def uuid)

add_importlibs(fontext
    user32
    gdi32
    ole32
    shlwapi
    lz32
    advapi32
    setupapi
    kernel32
    ntdll)

add_dependencies(fontext fontext_def)
