


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-DUNICODE -D_UNICODE)

spec2def(t2embed ${CMAKE_CURRENT_SOURCE_DIR}/t2embed.spec ${CMAKE_CURRENT_BINARY_DIR}/t2embed.def)

add_library(t2embed SHARED t2embed.c t2embed.rc)

set_target_properties(t2embed PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(t2embed
    ${CMAKE_CURRENT_BINARY_DIR}/t2embed.def
    wine
    -lgdi32
    -luser32
    -ladvapi32
    -lversion
    mingw_common)

add_dependencies(t2embed t2embed_def psdk)
