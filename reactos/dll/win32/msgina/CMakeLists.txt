
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(msgina.dll msgina.spec)

list(APPEND SOURCE
    gui.c
    msgina.c
    stubs.c
    tui.c
    msgina.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msgina_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

add_library(msgina SHARED ${SOURCE})
set_module_type(msgina win32dll)
target_link_libraries(msgina wine)
add_importlibs(msgina advapi32 user32 gdi32 userenv msvcrt kernel32 ntdll)
add_pch(msgina msgina.h)
add_cd_file(TARGET msgina DESTINATION reactos/system32 FOR all)
