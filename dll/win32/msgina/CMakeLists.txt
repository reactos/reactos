
set_cpp(WITH_RUNTIME)

add_definitions(
    -D_ATL_NO_EXCEPTIONS)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

spec2def(msgina.dll msgina.spec)

list(APPEND C_SOURCE
    gui.c
    lsa.c
    msgina.c
    shutdown.c
    stubs.c
    tui.c
    msgina.h)

list(APPEND CPP_SOURCE
    dimmedwindow.cpp)

add_library(msgina MODULE
    ${C_SOURCE}
    ${CPP_SOURCE}
    msgina.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msgina_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

set_module_type(msgina win32dll UNICODE)
target_link_libraries(msgina wine uuid ${PSEH_LIB})
add_delay_importlibs(msgina secur32)
add_importlibs(msgina advapi32 user32 gdi32 powrprof userenv msvcrt kernel32 ntdll)
add_pch(msgina msgina.h CPP_SOURCE)
add_cd_file(TARGET msgina DESTINATION reactos/system32 FOR all)
