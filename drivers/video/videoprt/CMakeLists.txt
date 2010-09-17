
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_definitions(-D_VIDEOPORT_)

spec2def(videoprt ${CMAKE_CURRENT_SOURCE_DIR}/videoprt.spec ${CMAKE_CURRENT_BINARY_DIR}/videoprt.def)

list(APPEND SOURCE
    agp.c
    ddc.c
    dispatch.c
    dma.c
    event.c
    funclist.c
    int10.c
    interrupt.c
    resource.c
    services.c
    spinlock.c
    stubs.c
    timer.c
    videoprt.c
    videoprt.rc)

add_library(videoprt SHARED ${CMAKE_CURRENT_BINARY_DIR}/videoprt_videoprt.h.gch ${SOURCE})

set_target_properties(videoprt PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(videoprt
    ${CMAKE_CURRENT_BINARY_DIR}/videoprt.def
    -lntoskrnl
    -lhal)

add_pch(videoprt ${CMAKE_CURRENT_SOURCE_DIR}/videoprt.h ${SOURCE})
add_dependencies(videoprt videoprt_def psdk bugcodes buildno_header)
