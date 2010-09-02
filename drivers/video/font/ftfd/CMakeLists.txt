
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

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
