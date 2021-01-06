
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
add_definitions(-D_VIDEOPORT_)
spec2def(videoprt.sys videoprt.spec ADD_IMPORTLIB)

# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libvideoprt PRIVATE $<TARGET_OBJECTS:gcc_ssp_videoprt>)
endif()

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
    videoprt.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(videoprt MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    videoprt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/videoprt.def)

set_module_type(videoprt kernelmodedriver)
target_link_libraries(videoprt ${PSEH_LIB})
add_importlibs(videoprt ntoskrnl hal)
add_pch(videoprt videoprt.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET videoprt DESTINATION reactos/system32/drivers FOR all)
