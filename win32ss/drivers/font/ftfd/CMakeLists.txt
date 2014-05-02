
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include)

spec2def(ftfd.dll ftfd.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    enable.c
    font.c
    glyph.c
    rosglue.c
    ftfd.h)

add_library(ftfd SHARED
    ${SOURCE}
    sprintf.c
    freetype.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ftfd.def)

set_entrypoint(ftfd FtfdEnableDriver 12)
set_subsystem(ftfd native)
set_image_base(ftfd 0x00010000)

target_link_libraries(ftfd
    freetype
    libcntpr)

add_pch(ftfd ftfd.h SOURCE)
add_importlibs(ftfd win32k)
add_dependencies(ftfd psdk)
add_cd_file(TARGET ftfd DESTINATION reactos/system32 FOR all)

