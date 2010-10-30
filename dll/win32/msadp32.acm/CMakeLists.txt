
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msadp32.acm msadp32.acm.spec)

add_library(msadp32.acm SHARED msadp32.c
    ${CMAKE_CURRENT_BINARY_DIR}/msadp32.def)

set_entrypoint(msadp32.acm 0)
set_target_properties(msadp32.acm PROPERTIES SUFFIX "")

target_link_libraries(msadp32.acm wine)

add_importlibs(msadp32.acm winmm user32 msvcrt kernel32 ntdll)
add_dependencies(msadp32.acm psdk buildno_header)
add_cab_target(msadp32.acm 1)
