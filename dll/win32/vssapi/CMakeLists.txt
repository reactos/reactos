
spec2def(vssapi.dll vssapi.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/vssapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/vssapi.def)

if(MSVC AND ARCH STREQUAL "i386")
    add_asm_files(vssapi_asm msvc.s)
endif()

add_library(vssapi MODULE ${SOURCE} ${vssapi_asm})
set_module_type(vssapi win32dll)
target_link_libraries(vssapi wine)
add_importlibs(vssapi msvcrt kernel32 ntdll)
add_cd_file(TARGET vssapi DESTINATION reactos/system32 FOR all)
set_wine_module(vssapi)
