
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include)

add_library(ftfd SHARED
    enable.c
    font.c
    glyph.c
    rosglue.c
    sprintf.c)

set_entrypoint(ftfd FtfdEnableDriver@12)
set_subsystem(ftfd native)

target_link_libraries(ftfd
    ${CMAKE_CURRENT_SOURCE_DIR}/freetype.def
    freetype
    libcntpr)

add_importlibs(ftfd win32k)
add_dependencies(ftfd psdk)
add_cab_target(ftfd 1)
add_importlib_target(ftfd.spec)