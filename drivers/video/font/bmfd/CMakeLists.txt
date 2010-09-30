
add_library(bmfd SHARED
    enable.c
    font.c
    glyph.c)

set_target_properties(bmfd PROPERTIES LINK_FLAGS "-Wl,-entry,_BmfdEnableDriver@12 -Wl,--subsystem,native -Wl,--image-base,0x00010000")

target_link_libraries(bmfd libcntpr)

add_importlibs(bmfd win32k)
add_dependencies(bmfd psdk bugcodes)
