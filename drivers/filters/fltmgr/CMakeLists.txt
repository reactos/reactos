list(APPEND SOURCE
    Context.c
    Dispatch.c
    Filter.c
    Interface.c
    Lib.c
    Messaging.c
    Object.c
    ${CMAKE_CURRENT_BINARY_DIR}/fltmgr.def
    fltmgr.h)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/fltmgr
    includes)

spec2def(fltmgr.sys fltmgr.spec ADD_IMPORTLIB)

add_library(fltmgr SHARED
    ${SOURCE}
    fltmgr.rc)

set_module_type(fltmgr kernelmodedriver)
target_link_libraries(fltmgr ${PSEH_LIB})
add_importlibs(fltmgr ntoskrnl hal)
add_target_compile_definitions(fltmgr NTDDI_VERSION=NTDDI_WS03SP1)
add_pch(fltmgr fltmgr.h SOURCE)
add_cd_file(TARGET fltmgr DESTINATION reactos/system32/drivers NO_CAB FOR all)
