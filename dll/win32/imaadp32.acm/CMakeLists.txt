
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(imaadp32.acm imaadp32.acm.spec)

add_library(imaadp32 SHARED imaadp32.c
    ${CMAKE_CURRENT_BINARY_DIR}/imaadp32.def)

set_entrypoint(imaadp32 0)
set_target_properties(imaadp32 PROPERTIES SUFFIX "acm")

target_link_libraries(imaadp32 wine)

add_importlibs(imaadp32 winmm user32 msvcrt kernel32 ntdll)
add_dependencies(imaadp32 psdk buildno_header)
add_cab_target(imaadp32 1)
