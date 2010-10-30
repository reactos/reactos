
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dxdiagn ${CMAKE_CURRENT_SOURCE_DIR}/dxdiagn.spec ${CMAKE_CURRENT_BINARY_DIR}/dxdiagn.def)

add_library(dxdiagn SHARED
    container.c
    dxdiag_main.c
    provider.c
    regsvr.c)

set_module_type(dxdiagn win32dll)

target_link_libraries(dxdiagn
    ${CMAKE_CURRENT_BINARY_DIR}/dxdiagn.def
    uuid
    wine)

add_importlibs(dxdiagn
    user32
    advapi32
    ole32
    oleaut32
    kernel32
    ntdll)

add_dependencies(dxdiagn dxdiagn_def)
add_cab_target(dxdiagn 1)
