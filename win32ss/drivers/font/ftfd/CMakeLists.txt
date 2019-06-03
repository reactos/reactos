
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/freetype/include)

list(APPEND SOURCE
    enable.c
    font.c
    glyph.c
    rosglue.c
    ftfd.h)

add_library(ftfd MODULE
    ${SOURCE}
    sprintf.c
    freetype.rc)
spec2def(ftfd ftfd.dll ftfd.spec ADD_IMPORTLIB)

set_module_type(ftfd kerneldll ENTRYPOINT FtfdEnableDriver 12)
target_link_libraries(ftfd freetype libcntpr)
add_pch(ftfd ftfd.h SOURCE)
add_importlibs(ftfd win32k)
add_dependencies(ftfd psdk)
add_cd_file(TARGET ftfd DESTINATION reactos/system32 FOR all)
