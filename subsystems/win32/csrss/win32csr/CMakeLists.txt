
include_directories(BEFORE .)
include_directories(${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)
include_directories(${REACTOS_SOURCE_DIR}/dll/cpl/console)

spec2def(win32csr ${CMAKE_CURRENT_SOURCE_DIR}/win32csr.spec ${CMAKE_CURRENT_BINARY_DIR}/win32csr.def)

set_rc_compiler()

list(APPEND SOURCE
    alias.c
    appswitch.c
    coninput.c
    conoutput.c
    console.c
    desktopbg.c
    dllmain.c
    exitros.c
    guiconsole.c
    handle.c
    harderror.c
    lineinput.c
    tuiconsole.c
    win32csr.rc)

add_library(win32csr SHARED ${CMAKE_CURRENT_BINARY_DIR}/win32csr_w32csr.h.gch ${SOURCE})

target_link_libraries(win32csr
    ${CMAKE_CURRENT_BINARY_DIR}/win32csr.def
    win32ksys
    pseh)

set_module_type(win32csr win32dll)

add_importlibs(win32csr ntdll user32 gdi32 advapi32 psapi)

add_pch(win32csr ${CMAKE_CURRENT_SOURCE_DIR}/w32csr.h ${SOURCE})

add_dependencies(win32csr win32csr_def psdk bugcodes)
