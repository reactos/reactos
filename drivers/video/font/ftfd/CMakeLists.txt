


include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include)

add_library(ftfd SHARED
    enable.c
    font.c
    glyph.c
    rosglue.c
    sprintf.c)

set_target_properties(ftfd PROPERTIES LINK_FLAGS "-Wl,-entry,_FtfdEnableDriver@12 -Wl,--subsystem,native")

target_link_libraries(ftfd
    ${CMAKE_CURRENT_SOURCE_DIR}/freetype.def
    -lwin32k
    freetype
    libcntpr)

add_dependencies(ftfd psdk)
