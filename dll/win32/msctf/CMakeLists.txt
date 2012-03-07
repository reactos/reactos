
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

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
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msctf_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msctf.def)

add_library(msctf SHARED ${SOURCE})

set_module_type(msctf win32dll)
target_link_libraries(msctf uuid wine)
add_importlibs(msctf ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msctf DESTINATION reactos/system32 FOR all)
