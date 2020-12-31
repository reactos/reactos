
PROJECT(NTOS)

include(ntos.cmake)

set(NTOSKRNL_SOURCE ${SOURCE})
set(NTOSKRNL_ASM_SOURCE ${ASM_SOURCE})

set(NTKRNLMP_SOURCE ${SOURCE})
set(NTKRNLMP_ASM_SOURCE ${ASM_SOURCE})

spec2def(ntoskrnl.exe ntoskrnl.spec ADD_IMPORTLIB)

# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libntoskrnl PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

add_asm_files(ntoskrnl_asm ${NTOSKRNL_ASM_SOURCE})

if (GCC AND CLANG)
    # Clang optimises strcmp calls to memcmp.
    target_sources(libntoskrnl PRIVATE $<TARGET_OBJECTS:memcmp>)
endif()

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_executable(ntoskrnl
    ${ntoskrnl_asm}
    ${NTOSKRNL_SOURCE}
    ${PCH_SKIP_SOURCE}
    ntoskrnl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntoskrnl.def)
set_property(TARGET ntoskrnl PROPERTY ENABLE_EXPORTS TRUE)
set_target_properties(ntoskrnl PROPERTIES DEFINE_SYMBOL "")

source_group(TREE ${REACTOS_SOURCE_DIR}/ntoskrnl PREFIX "Source Files" FILES ${NTOSKRNL_SOURCE})

if(ARCH STREQUAL "i386")
    set_entrypoint(ntoskrnl KiSystemStartup 4)
else()
    set_entrypoint(ntoskrnl KiSystemStartup)
endif()
set_subsystem(ntoskrnl native)

if(MSVC)
    set_image_base(ntoskrnl 0x00400000)
    add_target_link_flags(ntoskrnl "/SECTION:.rsrc,!DP") #Accessed from bugcheck code
    add_target_link_flags(ntoskrnl "/SECTION:INIT,ERWD")
else()
    if(GDB AND NOT CLANG)
        # Completely disable optimizations when debugging the kernel
        # Not for CLang, which fails as following in _SEH2_EXCEPT macro:
        # error: inline assembly requires more registers than available
        target_compile_options(ntoskrnl PRIVATE -O0)
        set_image_base(ntoskrnl 0x00800000)
    else()
        set_image_base(ntoskrnl 0x80800000)
    endif()
    add_linker_script(ntoskrnl ${REACTOS_SOURCE_DIR}/sdk/cmake/init-section.lds)
endif()

target_link_libraries(ntoskrnl cportlib csq ${PSEH_LIB} arbiter cmlib ntlsalib rtl ${ROSSYM_LIB} libcntpr wdmguid ioevent)

if(STACK_PROTECTOR)
    target_sources(ntoskrnl PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
elseif(RUNTIME_CHECKS)
    target_link_libraries(ntoskrnl runtmchk)
endif()

add_importlibs(ntoskrnl hal kdcom bootvid)
add_pch(ntoskrnl ${REACTOS_SOURCE_DIR}/ntoskrnl/include/ntoskrnl.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntoskrnl psdk bugcodes asm)
add_cd_file(TARGET ntoskrnl DESTINATION reactos/system32 NO_CAB FOR all)

if(BUILD_MP)
    add_subdirectory(ntkrnlmp)
endif()

add_asm_files(ntdllsys_asm ntdll.S)
add_library(ntdllsys ${ntdllsys_asm})
set_target_properties(ntdllsys PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(ntdllsys asm)
