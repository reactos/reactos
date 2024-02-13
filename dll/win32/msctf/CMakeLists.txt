
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msctf.dll msctf.spec ADD_IMPORTLIB)

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
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/msctf_stubs.c)

list(APPEND PCH_SKIP_SOURCE
    utils.cpp)

add_library(msctf MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msctf.def)

set_module_type(msctf win32dll)
target_link_libraries(msctf uuid wine cicero_a)
target_include_directories(msctf BEFORE PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/lib/cicero)
add_importlibs(msctf user32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_delay_importlibs(msctf ole32 oleaut32)
add_pch(msctf precomp.h SOURCE)
add_cd_file(TARGET msctf DESTINATION reactos/system32 FOR all)
