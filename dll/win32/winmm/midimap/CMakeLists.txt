
add_definitions(-DUNICODE -D_UNICODE)

spec2def(midimap ${CMAKE_CURRENT_SOURCE_DIR}/midimap.spec ${CMAKE_CURRENT_BINARY_DIR}/midimap.def)

add_library(midimap SHARED midimap.c midimap.rc)

set_entrypoint(midimap 0)

target_link_libraries(midimap
    ${CMAKE_CURRENT_BINARY_DIR}/midimap.def
    wine)

add_importlibs(midimap kernel32 uuid ntdll advapi32 user32 winmm)
add_dependencies(midimap midimap_def psdk)
add_cab_target(midimap 1)