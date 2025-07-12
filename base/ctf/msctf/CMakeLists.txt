
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(msctf.dll msctf.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    precomp.h
    categorymgr.cpp
    compartmentmgr.cpp
    context.cpp
    displayattributemgr.cpp
    documentmgr.cpp
    inputprocessor.cpp
    langbarmgr.cpp
    mlng.cpp
    msctf.cpp
    range.cpp
    threadmgr.cpp
    utils.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/msctf_stubs.c)

add_library(msctf MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msctf.def)

if(MSVC)
    # Disable C5286: implicit conversion from enum type 'x' to enum type 'y';
    target_compile_options(msctf PRIVATE /wd5286)
endif()

set_module_type(msctf win32dll UNICODE)
target_link_libraries(msctf uuid wine cicero)
add_importlibs(msctf user32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_delay_importlibs(msctf shell32 shlwapi ole32 oleaut32 imm32 gdi32)
add_pch(msctf precomp.h SOURCE)
add_cd_file(TARGET msctf DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(msctf) # CORE-5743: No CONST_VTABLE
