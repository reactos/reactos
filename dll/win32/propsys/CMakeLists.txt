
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS -D__ROS_LONG64__)
add_definitions(-D__WINESRC__ -D_PROPSYS_)
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
target_link_libraries(propsys uuid wine wine_dll_register)
add_importlibs(propsys ole32 oleaut32 msvcrt kernel32 ntdll)
add_pch(propsys precomp.h SOURCE)
add_cd_file(TARGET propsys DESTINATION reactos/system32 FOR all)
