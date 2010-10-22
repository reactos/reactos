


add_definitions(-D__NTDLL__)
add_definitions(-D_NTOSKRNL_)
add_definitions(-DCRTDLL)

include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    csr/api.c
    csr/capture.c
    csr/connect.c
    dbg/dbgui.c
    ldr/startup.c
    ldr/utils.c
    ldr/actctx.c
    rtl/libsupp.c
    rtl/version.c
    def/ntdll.rc)

if(ARCH MATCHES i386)
list(APPEND SOURCE dispatch/i386/dispatch.S)
elseif(ARCH MATCHES amd64)
list(APPEND SOURCE dispatch/amd64/stubs.c)
elseif(ARCH MATCHES arm)
list(APPEND SOURCE dispatch/arm/stubs_asm.s)
else()
list(APPEND SOURCE dispatch/dispatch.c)
endif(ARCH MATCHES i386)

add_library(ntdll SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_ntdll.h.gch)

set_target_properties(ntdll PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(ntdll ${CMAKE_CURRENT_SOURCE_DIR}/def/ntdll.def
                      -Wl,--whole-archive rtl -Wl,--no-whole-archive
                      ntdllsys
                      libcntpr
                      pseh)
                      
add_pch(ntdll ${CMAKE_CURRENT_SOURCE_DIR}/include/ntdll.h ${SOURCE})
add_dependencies(ntdll ntstatus version)

add_minicd_target(ntdll reactos/system32 ntdll.dll)
add_livecd_target(ntdll reactos/system32)