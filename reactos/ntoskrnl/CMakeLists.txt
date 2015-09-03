
PROJECT(NTOS)

include(ntos.cmake)

set(NTOSKRNL_SOURCE ${SOURCE})
set(NTOSKRNL_ASM_SOURCE ${ASM_SOURCE})

set(NTKRNLMP_SOURCE ${SOURCE})
set(NTKRNLMP_ASM_SOURCE ${ASM_SOURCE})

spec2def(ntoskrnl.exe ntoskrnl.spec ADD_IMPORTLIB)
add_asm_files(ntoskrnl_asm ${NTOSKRNL_ASM_SOURCE})

add_executable(ntoskrnl
    ${ntoskrnl_asm}
    ${NTOSKRNL_SOURCE}
    guid.c
    ntoskrnl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntoskrnl.def)

if(ARCH STREQUAL "i386")
    set_entrypoint(ntoskrnl KiSystemStartup 4)
else()
    set_entrypoint(ntoskrnl KiSystemStartup)
endif()
set_subsystem(ntoskrnl native)

if(MSVC)
    set_image_base(ntoskrnl 0x00400000)
    add_target_link_flags(ntoskrnl "/SECTION:.rsrc,!DP") #Accessed from bugcheck code
else()
    if(GDB)
        set_image_base(ntoskrnl 0x00800000)
    else()
        set_image_base(ntoskrnl 0x80800000)
    endif()
endif()

target_link_libraries(ntoskrnl cportlib csq ${PSEH_LIB} cmlib rtl ${ROSSYM_LIB} libcntpr wdmguid ioevent)

if(STACK_PROTECTOR)
    target_link_libraries(ntoskrnl gcc_ssp)
elseif(RUNTIME_CHECKS)
    target_link_libraries(ntoskrnl runtmchk)
endif()

add_importlibs(ntoskrnl hal kdcom bootvid)
add_pch(ntoskrnl ${REACTOS_SOURCE_DIR}/ntoskrnl/include/ntoskrnl.h NTOSKRNL_SOURCE)
add_dependencies(ntoskrnl psdk bugcodes asm)
add_cd_file(TARGET ntoskrnl DESTINATION reactos/system32 NO_CAB FOR all)

if(BUILD_MP)
    add_subdirectory(ntkrnlmp)
endif()

add_asm_files(ntdllsys_asm ntdll.S)
add_library(ntdllsys ${ntdllsys_asm})
set_target_properties(ntdllsys PROPERTIES LINKER_LANGUAGE "C")
