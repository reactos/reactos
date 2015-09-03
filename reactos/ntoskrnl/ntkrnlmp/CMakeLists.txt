
include(../ntos.cmake)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl)
spec2def(ntkrnlmp.exe ../ntoskrnl.spec)

if(MSVC_IDE AND (CMAKE_VERSION MATCHES "ReactOS"))
    set_source_files_properties(${NTKRNLMP_ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "CONFIG_SMP")
endif()

add_asm_files(ntkrnlmp_asm ${NTKRNLMP_ASM_SOURCE})

add_executable(ntkrnlmp
    ${ntkrnlmp_asm}
    ${NTKRNLMP_SOURCE}
    ${REACTOS_SOURCE_DIR}/ntoskrnl/guid.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ntoskrnl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntkrnlmp.def)

add_target_compile_definitions(ntkrnlmp CONFIG_SMP)

if(ARCH STREQUAL "i386")
    set_entrypoint(ntkrnlmp KiSystemStartup 4)
else()
    set_entrypoint(ntkrnlmp KiSystemStartup)
endif()
set_subsystem(ntkrnlmp native)

if(MSVC)
    set_image_base(ntkrnlmp 0x00400000)
    add_target_link_flags(ntoskrnl "/SECTION:.rsrc,!DP") #Accessed from bugcheck code
else()
    set_image_base(ntkrnlmp 0x80800000)
endif()

if(STACK_PROTECTOR)
    target_link_libraries(ntkrnlmp gcc_ssp)
elseif(RUNTIME_CHECKS)
    target_link_libraries(ntkrnlmp runtmchk)
endif()

target_link_libraries(ntkrnlmp cportlib csq ${PSEH_LIB} cmlib rtl ${ROSSYM_LIB} libcntpr wdmguid ioevent)
add_importlibs(ntkrnlmp hal kdcom bootvid)
add_pch(ntkrnlmp ${REACTOS_SOURCE_DIR}/ntoskrnl/include/ntoskrnl.h NTKRNLMP_SOURCE)
add_dependencies(ntkrnlmp psdk bugcodes asm)
add_cd_file(TARGET ntkrnlmp DESTINATION reactos/system32 NO_CAB FOR all)
