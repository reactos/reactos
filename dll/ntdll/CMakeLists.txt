
add_subdirectory(nt_0600)

spec2def(ntdll.dll def/ntdll.spec ADD_IMPORTLIB)

# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libntdll PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

add_definitions(
    -D__NTDLL__
    -D_NTOSKRNL_
    -DCRTDLL)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

add_library(rtl_um OBJECT
    rtl/libsupp.c
    rtl/uilist.c
    rtl/version.c)
target_link_libraries(rtl_um apisets ${PSEH_LIB})
add_dependencies(rtl_um psdk)

# On NT6+ this is used by advapi32
add_library(etwtrace etw/trace.c)
target_link_libraries(etwtrace ${PSEH_LIB})
add_dependencies(etwtrace psdk)

list(APPEND SOURCE
    dbg/dbgui.c
    ldr/ldrapi.c
    ldr/ldrinit.c
    ldr/ldrpe.c
    ldr/ldrutils.c
    ldr/verifier.c)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE dispatch/i386/dispatch.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE dispatch/amd64/dispatch.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE dispatch/arm/stubs_asm.s)
else()
    list(APPEND SOURCE dispatch/dispatch.c)
endif()

add_asm_files(ntdll_asm ${ASM_SOURCE})

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_stubs.c)

add_library(ntdll MODULE
    ${SOURCE}
    ${ntdll_asm}
    ${PCH_SKIP_SOURCE}
    def/ntdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll.def)

set_module_type(ntdll win32dll ENTRYPOINT 0)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(ntdll console)
################# END  HACK #################

target_link_libraries(ntdll etwtrace csrlib rtl rtl_um rtl_vista ntdllsys libcntpr uuid ${PSEH_LIB})
if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    target_link_libraries(ntdll cryptlib)
endif()

if (STACK_PROTECTOR)
    target_sources(ntdll PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

add_pch(ntdll include/ntdll.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntdll ntstatus asm)
add_cd_file(TARGET ntdll DESTINATION reactos/system32 NO_CAB FOR all)

# Newer VS versions link to _ftol2 / _ftoul2_legacy
if(MSVC AND ARCH STREQUAL "i386")
    target_link_libraries(libntdll ftol2)
endif()
