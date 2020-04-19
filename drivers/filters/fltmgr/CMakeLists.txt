
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/fltmgr
    includes)

spec2def(fltmgr.sys fltmgr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    Context.c
    Dispatch.c
    Filter.c
    Interface.c
    Lib.c
    Messaging.c
    Misc.c
    Object.c
    Registry.c
    Volume.c
    fltmgr.h)

add_library(fltmgr MODULE
    ${SOURCE}
    fltmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fltmgr.def)

set_module_type(fltmgr kernelmodedriver)
target_link_libraries(fltmgr ${PSEH_LIB})
add_importlibs(fltmgr ntoskrnl hal)
target_compile_definitions(fltmgr PRIVATE NTDDI_VERSION=NTDDI_WS03SP1)
add_pch(fltmgr fltmgr.h SOURCE)
add_cd_file(TARGET fltmgr DESTINATION reactos/system32/drivers NO_CAB FOR all)
