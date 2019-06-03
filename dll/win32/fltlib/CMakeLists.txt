
add_definitions(-D__WINESRC__)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/fltmgr
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    includes)

list(APPEND SOURCE
    fltlib.c
    message.c
    stubs.c)

add_library(fltlib MODULE ${SOURCE} rsrc.rc)
spec2def(fltlib fltlib.dll fltlib.spec ADD_IMPORTLIB)

set_module_type(fltlib win32dll)
target_link_libraries(fltlib wine)
add_target_compile_definitions(fltlib NTDDI_VERSION=NTDDI_WS03SP1)
add_importlibs(fltlib msvcrt kernel32 ntdll)
add_cd_file(TARGET fltlib DESTINATION reactos/system32 FOR all)
