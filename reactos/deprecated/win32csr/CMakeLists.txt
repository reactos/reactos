
include_directories(
    BEFORE .
    ${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers
    ${REACTOS_SOURCE_DIR}/dll/cpl/console
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(win32csr.dll win32csr.spec)

list(APPEND SOURCE
    alias.c
    coninput.c
    conoutput.c
    console.c
    desktopbg.c
    dllmain.c
    exitros.c
    file.c
    guiconsole.c
    handle.c
    harderror.c
    lineinput.c
    tuiconsole.c
    win32csr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/win32csr.def)

add_library(win32csr SHARED ${SOURCE})
target_link_libraries(win32csr win32ksys ${PSEH_LIB})
set_module_type(win32csr win32dll)
add_importlibs(win32csr psapi msvcrt kernel32 ntdll csrsrv)
add_delay_importlibs(win32csr user32 gdi32 advapi32)
add_pch(win32csr w32csr.h)
add_dependencies(win32csr bugcodes)
add_cd_file(TARGET win32csr DESTINATION reactos/system32 FOR all)
