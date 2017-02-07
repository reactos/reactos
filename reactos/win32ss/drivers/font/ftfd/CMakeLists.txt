
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include)

spec2def(ftfd.dll ftfd.spec ADD_IMPORTLIB)

add_library(ftfd SHARED
    enable.c
    font.c
    glyph.c
    rosglue.c
    sprintf.c
    ${CMAKE_CURRENT_BINARY_DIR}/ftfd.def)

set_entrypoint(ftfd FtfdEnableDriver 12)
set_subsystem(ftfd native)
set_image_base(ftfd 0x00010000)

target_link_libraries(ftfd
    freetype
    libcntpr)

add_pch(ftfd ftfd.h)
add_importlibs(ftfd win32k)
add_dependencies(ftfd psdk)
add_cd_file(TARGET ftfd DESTINATION reactos/system32 FOR all)

