
spec2def(ntdll.dll def/ntdll.spec)

add_definitions(
    -D__NTDLL__
    -D_NTOSKRNL_
    -DCRTDLL)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    csr/api.c
    csr/capture.c
    csr/connect.c
    dbg/dbgui.c
	ldr/actctx.c
	ldr/ldrapi.c
	ldr/ldrinit.c
	ldr/ldrpe.c
	ldr/ldrutils.c
    ldr/startup.c
    ldr/utils.c
    rtl/libsupp.c
    rtl/version.c
    def/ntdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll.def)
    
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
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_ntdll.h.gch
    ${SOURCE})

set_entrypoint(ntdll 0)

if(MSVC)
target_link_libraries(ntdll rtl)
else()
target_link_libraries(ntdll -Wl,--whole-archive rtl -Wl,--no-whole-archive)
endif()

target_link_libraries(ntdll
                      ntdllsys
                      libcntpr
                      ${PSEH_LIB})

set_image_base(ntdll ${baseaddress_ntdll})
add_pch(ntdll ${CMAKE_CURRENT_SOURCE_DIR}/include/ntdll.h ${SOURCE})
add_dependencies(ntdll ntstatus asm)

add_cd_file(TARGET ntdll DESTINATION reactos/system32 NO_CAB FOR all)
add_importlib_target(def/ntdll.spec)
