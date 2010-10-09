
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msctf ${CMAKE_CURRENT_SOURCE_DIR}/msctf.spec ${CMAKE_CURRENT_BINARY_DIR}/msctf.def)

add_library(msctf SHARED
    categorymgr.c
    compartmentmgr.c
    context.c
    displayattributemgr.c
    documentmgr.c
    inputprocessor.c
    langbarmgr.c
    msctf.c
    range.c
    regsvr.c
    threadmgr.c
    version.rc)

set_module_type(msctf win32dll)

target_link_libraries(msctf
    ${CMAKE_CURRENT_BINARY_DIR}/msctf.def
    uuid
    wine)

add_importlibs(msctf ole32 oleaut32 user32 advapi32 kernel32 ntdll)
add_dependencies(msctf msctf_def)
