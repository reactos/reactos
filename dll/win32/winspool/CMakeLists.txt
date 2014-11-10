
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(winspool.drv winspool.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    info.c
    stubs.c
    precomp.h)

add_library(winspool.drv SHARED
    ${SOURCE}
    winspool.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool.def)

set_target_properties(winspool.drv PROPERTIES SUFFIX "")
set_module_type(winspool.drv win32dll UNICODE)
target_link_libraries(winspool.drv wine)
add_importlibs(winspool.drv advapi32 shlwapi msvcrt kernel32 ntdll)
add_pch(winspool.drv precomp.h SOURCE)
add_cd_file(TARGET winspool.drv DESTINATION reactos/system32 FOR all)
