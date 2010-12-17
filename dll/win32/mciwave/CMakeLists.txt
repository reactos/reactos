
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciwave.dll mciwave.spec)

list(APPEND SOURCE
    mciwave.c
    ${CMAKE_CURRENT_BINARY_DIR}/mciwave.def)

add_library(mciwave SHARED ${SOURCE})

set_entrypoint(mciwave 0)
target_link_libraries(mciwave wine)
add_importlibs(mciwave user32 winmm msvcrt kernel32 ntdll)
add_dependencies(mciwave psdk buildno_header)
add_cab_target(mciwave 1)
