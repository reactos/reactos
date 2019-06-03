
add_definitions(
    -D__WINESRC__
    -DHAVE_ZLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

list(APPEND SOURCE
    cabinet_main.c
    fci.c
    fdi.c
    precomp.h)

add_library(cabinet MODULE
    ${SOURCE}
    stubs.c
    cabinet.rc)
spec2def(cabinet cabinet.dll cabinet.spec ADD_IMPORTLIB)

set_module_type(cabinet win32dll)
target_link_libraries(cabinet wine zlib)
add_importlibs(cabinet msvcrt kernel32 ntdll)
add_pch(cabinet precomp.h SOURCE)
add_cd_file(TARGET cabinet DESTINATION reactos/system32 FOR all)
