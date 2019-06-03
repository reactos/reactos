
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    propstore.c
    propsys_main.c
    propvar.c
    precomp.h)

add_library(propsys MODULE
    ${SOURCE}
    propsys.rc)
spec2def(propsys propsys.dll propsys.spec ADD_IMPORTLIB)

set_module_type(propsys win32dll)
target_link_libraries(propsys uuid wine)
add_importlibs(propsys ole32 oleaut32 msvcrt kernel32 ntdll)
add_pch(propsys precomp.h SOURCE)
add_cd_file(TARGET propsys DESTINATION reactos/system32 FOR all)
