
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

spec2def(msvfw32 ${CMAKE_CURRENT_SOURCE_DIR}/msvfw32.spec ${CMAKE_CURRENT_BINARY_DIR}/msvfw32.def)

add_library(msvfw32 SHARED
    mciwnd.c
    msvideo_main.c
    drawdib.c
    rsrc.rc)

set_target_properties(msvfw32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(msvfw32
    ${CMAKE_CURRENT_BINARY_DIR}/msvfw32.def
    wine
    -lwinmm
    -lversion
    -lcomctl32
    -luser32
    -lgdi32
    -ladvapi32
    mingw_common)

add_dependencies(msvfw32 msvfw32_def psdk)
