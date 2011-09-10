

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dxdiagn.dll dxdiagn.spec)

add_library(dxdiagn SHARED
    container.c
    dxdiag_main.c
    provider.c
    regsvr.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxdiagn.def)

set_module_type(dxdiagn win32dll UNICODE)

target_link_libraries(dxdiagn uuid wine)

add_importlibs(dxdiagn
    user32
    advapi32
    ole32
    oleaut32
    msvcrt
    kernel32
    ntdll)

add_pch(dxdiagn dxdiag_private.h)
add_cd_file(TARGET dxdiagn DESTINATION reactos/system32 FOR all)
