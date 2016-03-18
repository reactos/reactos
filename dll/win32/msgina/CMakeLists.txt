
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msgina.dll msgina.spec)

list(APPEND SOURCE
    gui.c
    lsa.c
    msgina.c
    shutdown.c
    stubs.c
    tui.c
    msgina.h)

add_library(msgina SHARED
    ${SOURCE}
    msgina.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msgina_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

set_module_type(msgina win32dll)
target_link_libraries(msgina wine)
add_importlibs(msgina advapi32 user32 gdi32 powrprof userenv msvcrt kernel32 ntdll)
add_delay_importlibs(msgina secur32)
add_pch(msgina msgina.h SOURCE)
add_cd_file(TARGET msgina DESTINATION reactos/system32 FOR all)
