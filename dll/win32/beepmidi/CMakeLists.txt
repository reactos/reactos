


add_definitions(-DUNICODE -D_UNICODE)

spec2def(beepmidi ${CMAKE_CURRENT_SOURCE_DIR}/beepmidi.spec ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_library(beepmidi SHARED beepmidi.c)

set_target_properties(beepmidi PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(beepmidi
    ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def
    -lwinmm
    -lkernel32
    -lntdll)

add_dependencies(beepmidi beepmidi_def psdk)
