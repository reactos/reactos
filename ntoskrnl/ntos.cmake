
include_directories(
    ${REACTOS_SOURCE_DIR}
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/arbiter
    ${REACTOS_SOURCE_DIR}/sdk/lib/cmlib
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include/internal
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

add_definitions(
    -D_NTOSKRNL_
    -D_NTSYSTEM_
    -DNTDDI_VERSION=0x05020400)

if(NOT DEFINED NEWCC)
    set(NEWCC FALSE)
endif()

if(NEWCC)
    add_definitions(-DNEWCC)
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/cachesub.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/copysup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/fssup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/lazyrite.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/logsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/mdlsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/pinsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/fault.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/swapout.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/data.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/reqtools.c)
else()
    include(cc/cc.cmake)
endif()

include(config/config.cmake)
include(dbgk/dbgk.cmake)
include(ex/ex.cmake)
include(fsrtl/fsrtl.cmake)
include(fstub/fstub.cmake)
include(inbv/inbv.cmake)
include(io/io.cmake)
include(kd64/kd64.cmake)
include(ke/ke.cmake)
include(lpc/lpc.cmake)
include(mm/mm.cmake)
include(ob/ob.cmake)
include(po/po.cmake)
include(ps/ps.cmake)
include(rtl/rtl.cmake)
include(se/se.cmake)
include(vf/vf.cmake)
include(wmi/wmi.cmake)

list(APPEND SOURCE
    ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/io.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/sptab.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/vdm/vdmmain.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/vdm/vdmexec.c)
elseif(ARCH STREQUAL "amd64")
elseif(ARCH STREQUAL "arm")
endif()

if(NOT _WINKD_)
    include(kd/kd.cmake)
    if(KDBG)
        add_definitions(-DKDBG)
        include(kdbg/kdbg.cmake)
    endif()
else()
    add_definitions(-D_WINKD_)
endif()
