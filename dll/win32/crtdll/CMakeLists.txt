
add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MSVCRT_LIB_
    -D_MSVCRT_
    -DCRTDLL
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -D__MINGW_IMPORT="")

include_directories(${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

spec2def(crtdll.dll crtdll.spec)

list(APPEND SOURCE
    dllmain.c
    crtdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll.def)

add_library(crtdll SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll_precomp.h.gch
    ${SOURCE})

set_module_type(crtdll win32dll)

target_link_libraries(crtdll
    wine
    crt)

add_importlibs(crtdll kernel32 ntdll)
add_pch(crtdll ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cab_target(crtdll 1)
