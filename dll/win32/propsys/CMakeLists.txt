
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(propsys.dll propsys.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    propstore.c
    propsys_main.c
    propvar.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/propsys_stubs.c)

add_library(propsys MODULE
    ${SOURCE}
    propsys.rc
    ${CMAKE_CURRENT_BINARY_DIR}/propsys.def)

set_module_type(propsys win32dll)
target_link_libraries(propsys uuid wine)
add_importlibs(propsys ole32 oleaut32 msvcrt kernel32 ntdll)
add_pch(propsys precomp.h SOURCE)
add_cd_file(TARGET propsys DESTINATION reactos/system32 FOR all)
