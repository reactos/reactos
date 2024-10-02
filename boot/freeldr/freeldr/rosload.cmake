##
## PROJECT:     FreeLoader
## LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
## PURPOSE:     Build definitions for rosload 2nd stage loader
## COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
##

spec2def(rosload.exe rosload.spec)

list(APPEND ROSLOAD_SOURCE
    include/freeldr.h
    bootmgr.c
    custom.c
    linuxboot.c
    miscboot.c
    options.c
    oslist.c
    lib/rtl/libsupp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmboot.c
    ntldr/conversion.c
    ntldr/inffile.c
    ntldr/registry.c
    ntldr/setupldr.c
    ntldr/winldr.c
    ntldr/wlmemory.c
    ntldr/wlregistry.c
)

if(ARCH STREQUAL "i386")

    list(APPEND ROSLOAD_SOURCE
        arch/i386/halstub.c
        arch/i386/ntoskrnl.c
        disk/scsiport.c
        ntldr/arch/i386/winldr.c
        ntldr/headless.c)

    if(SARCH STREQUAL "pc98" OR SARCH STREQUAL "xbox")
        # These machine types require built-in bitmap font
        list(APPEND ROSLOAD_SOURCE
            arch/vgafont.c)
    endif()

    list(APPEND ROSLOAD_ASM_SOURCE
        arch/i386/drvmap.S
        arch/i386/linux.S)

elseif(ARCH STREQUAL "amd64")

    list(APPEND ROSLOAD_SOURCE
        ntldr/arch/amd64/winldr.c)

    list(APPEND ROSLOAD_ASM_SOURCE
        arch/amd64/misc.S
        arch/amd64/linux.S
    )

elseif(ARCH STREQUAL "arm")

    list(APPEND ROSLOAD_SOURCE
        ntldr/arch/arm/winldr.c)

    list(APPEND ROSLOAD_ASM_SOURCE
        arch/arm/boot.S)

else()
    #TBD
endif()

add_asm_files(rosload_asm ${ROSLOAD_ASM_SOURCE})

add_executable(rosload
    ${ROSLOAD_SOURCE}
    ${rosload_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/rosload.def
)

set_target_properties(rosload
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

set_image_base(rosload 0x10000) # 0x200000
set_subsystem(rosload native)
set_entrypoint(rosload RunLoader)

if(ARCH STREQUAL "i386")
    target_link_libraries(rosload mini_hal)
endif()

target_link_libraries(rosload blcmlib blrtl libcntpr)
add_importlibs(rosload freeldr)

# dynamic analysis switches
if(STACK_PROTECTOR)
    target_sources(rosload PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

if(RUNTIME_CHECKS)
    target_link_libraries(rosload runtmchk)
endif()

add_dependencies(rosload bugcodes asm xdk)

add_cd_file(TARGET rosload DESTINATION loader NO_CAB FOR bootcd regtest livecd hybridcd)
