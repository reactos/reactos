
add_definitions(
    -D__WINESRC__
    -DHAVE_ZLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

spec2def(cabinet.dll cabinet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cabinet_main.c
    fci.c
    fdi.c)

list(APPEND PCH_SKIP_SOURCE
    stubs.c)

add_library(cabinet MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    cabinet.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def)

set_module_type(cabinet win32dll)
target_link_libraries(cabinet wine zlib)
add_importlibs(cabinet msvcrt kernel32 ntdll)
add_pch(cabinet precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET cabinet DESTINATION reactos/system32 FOR all)
