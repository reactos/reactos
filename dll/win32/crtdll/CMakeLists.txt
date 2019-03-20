
spec2def(crtdll.dll crtdll.spec ADD_IMPORTLIB)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MSVCRT_LIB_
    -D_MSVCRT_
    -DCRTDLL
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -D__MINGW_IMPORT="")

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

list(APPEND SOURCE
    dllmain.c
    crtdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll.def)

add_library(crtdll MODULE ${SOURCE})
set_module_type(crtdll win32dll ENTRYPOINT DllMain 12)
target_link_libraries(crtdll crt wine)

if(MSVC)
    add_importlibs(crtdll msvcrt)
    target_link_libraries(crtdll oldnames)
endif()

add_importlibs(crtdll kernel32 ntdll)
add_cd_file(TARGET crtdll DESTINATION reactos/system32 FOR all)
