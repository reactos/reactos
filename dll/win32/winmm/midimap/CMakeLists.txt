
add_definitions(-DUNICODE -D_UNICODE)

spec2def(midimap.dll midimap.spec)

add_library(midimap SHARED midimap.c midimap.rc
    ${CMAKE_CURRENT_BINARY_DIR}/midimap.def)

set_entrypoint(midimap 0)

target_link_libraries(midimap wine)

add_importlibs(midimap kernel32 uuid ntdll advapi32 user32 winmm)
add_dependencies(midimap psdk)
add_cab_target(midimap 1)