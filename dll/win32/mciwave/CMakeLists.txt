
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciwave ${CMAKE_CURRENT_SOURCE_DIR}/mciwave.spec ${CMAKE_CURRENT_BINARY_DIR}/mciwave.def)

add_library(mciwave SHARED mciwave.c)

set_entrypoint(mciwave 0)

target_link_libraries(mciwave
    ${CMAKE_CURRENT_BINARY_DIR}/mciwave.def
    wine)

add_importlibs(mciwave winmm user32 msvcrt kernel32 ntdll)
add_dependencies(mciwave mciwave_def psdk buildno_header)
add_cab_target(mciwave 1)
