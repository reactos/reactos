
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
add_definitions(-D_VIDEOPORT_)
spec2def(videoprt.sys videoprt.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    agp.c
    child.c
    ddc.c
    dispatch.c
    dma.c
    event.c
    funclist.c
    int10.c
    interrupt.c
    registry.c
    resource.c
    services.c
    spinlock.c
    stubs.c
    timer.c
    videoprt.c
    videoprt.h)

add_library(videoprt SHARED
    ${SOURCE}
    guid.c
    videoprt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/videoprt.def)

set_module_type(videoprt kernelmodedriver)
add_importlibs(videoprt ntoskrnl hal)
add_pch(videoprt videoprt.h SOURCE)
add_cd_file(TARGET videoprt DESTINATION reactos/system32/drivers FOR all)
