
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include)

spec2def(ftfd.dll ftfd.spec)

add_library(ftfd SHARED
    enable.c
    font.c
    glyph.c
    rosglue.c
    sprintf.c
    ${CMAKE_CURRENT_BINARY_DIR}/ftfd.def)

set_entrypoint(ftfd FtfdEnableDriver@12)
set_subsystem(ftfd native)

target_link_libraries(ftfd
    freetype
    libcntpr)

add_importlibs(ftfd win32k)
add_dependencies(ftfd psdk)
add_cab_target(ftfd 1)
add_importlib_target(ftfd.spec)
