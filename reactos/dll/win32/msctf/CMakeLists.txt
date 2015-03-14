
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(msctf.dll msctf.spec)

list(APPEND SOURCE
    categorymgr.c
    compartmentmgr.c
    context.c
    displayattributemgr.c
    documentmgr.c
    inputprocessor.c
    langbarmgr.c
    msctf.c
    range.c
    threadmgr.c
    msctf_internal.h
    ${CMAKE_CURRENT_BINARY_DIR}/msctf_stubs.c)

add_library(msctf SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msctf.def)

set_module_type(msctf win32dll)
target_link_libraries(msctf uuid wine)
add_importlibs(msctf ole32 oleaut32 user32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_pch(msctf msctf_internal.h SOURCE)
add_cd_file(TARGET msctf DESTINATION reactos/system32 FOR all)
