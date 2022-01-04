
spec2def(msgina.dll msgina.spec)

list(APPEND SOURCE
    gui.c
    lsa.c
    msgina.c
    shutdown.c
    stubs.c
    tui.c)

list(APPEND PCH_SKIP_SOURCE
    dimmedwindow.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/msgina_stubs.c)

add_library(msgina MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    msgina.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

set_module_type(msgina win32dll UNICODE)
target_link_libraries(msgina wine uuid ${PSEH_LIB} cpprt atl_classes)
add_delay_importlibs(msgina secur32)
add_importlibs(msgina advapi32 user32 gdi32 powrprof userenv msvcrt kernel32 ntdll)
add_pch(msgina msgina.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET msgina DESTINATION reactos/system32 FOR all)
