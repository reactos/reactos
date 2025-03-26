##
## PROJECT:     FreeLoader
## LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
## PURPOSE:     Build definitions for PC-AT and "compatibles" (NEC PC-98, XBOX)
## COPYRIGHT:   Copyright 2003 Brian Palmer <brianp@sginet.com>
##              Copyright 2011-2014 Amine Khaldi <amine.khaldi@reactos.org>
##              Copyright 2011-2014 Timo Kreuzer <timo.kreuzer@reactos.org>
##              Copyright 2014 Hervé Poussineau <hpoussin@reactos.org>
##              Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
##

if(ARCH STREQUAL "i386")
    CreateBootSectorTarget(frldr16
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/realmode/i386.S
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        F800)
elseif(ARCH STREQUAL "amd64")
    CreateBootSectorTarget(frldr16
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/realmode/amd64.S
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        F800)
endif()


spec2def(freeldr.sys freeldr.spec ADD_IMPORTLIB)

list(APPEND PCATLDR_ARC_SOURCE
    ${FREELDR_ARC_SOURCE}
    arch/drivers/hwide.c)

list(APPEND PCATLDR_BOOTMGR_SOURCE
    ${FREELDR_BOOTMGR_SOURCE}
    linuxboot.c)

list(APPEND PCATLDR_BASE_ASM_SOURCE)

if(ARCH STREQUAL "i386")
    list(APPEND PCATLDR_BASE_ASM_SOURCE
        arch/i386/multiboot.S)

    list(APPEND PCATLDR_COMMON_ASM_SOURCE
        arch/i386/drvmap.S
        arch/i386/entry.S
        arch/i386/int386.S
        arch/i386/irqsup.S
        arch/i386/pnpbios.S
        # arch/i386/i386trap.S
        arch/i386/linux.S)

    list(APPEND PCATLDR_ARC_SOURCE
        # disk/scsiport.c
        lib/fs/pxe.c
        arch/i386/drivemap.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwdisk.c
        arch/i386/hwpci.c
        # arch/i386/i386bug.c
        arch/i386/i386idt.c)

    if(SARCH STREQUAL "pc98" OR SARCH STREQUAL "xbox")
        # These machine types require built-in bitmap font
        list(APPEND PCATLDR_ARC_SOURCE
            arch/vgafont.c)
    endif()

    if(SARCH STREQUAL "xbox")
        list(APPEND PCATLDR_ARC_SOURCE
            # FIXME: Abstract things better so we don't need to include /pc/* here
            arch/i386/pc/machpc.c       # machxbox.c depends on it
            arch/i386/pc/pcbeep.c       # machxbox.c depends on it
            arch/i386/pc/pcdisk.c       # hwdisk.c depends on it
            arch/i386/pc/pchw.c         # Many files depends on it
            arch/i386/pc/pcmem.c        # hwacpi.c/xboxmem.c depends on it
            arch/i386/pc/pcvesa.c       # machpc.c depends on it
            arch/i386/xbox/machxbox.c
            arch/i386/xbox/xboxcons.c
            arch/i386/xbox/xboxdisk.c
            arch/i386/xbox/xboxi2c.c
            arch/i386/xbox/xboxmem.c
            arch/i386/xbox/xboxrtc.c
            arch/i386/xbox/xboxvideo.c)
        if(NOT MSVC)
            # Prevent a warning when doing a memcmp with address 0
            set_source_files_properties(arch/i386/xbox/xboxmem.c PROPERTIES COMPILE_OPTIONS "-Wno-nonnull")
        endif()

    elseif(SARCH STREQUAL "pc98")
        list(APPEND PCATLDR_ARC_SOURCE
            arch/i386/pc/pcmem.c
            arch/i386/pc98/machpc98.c
            arch/i386/pc98/pc98beep.c
            arch/i386/pc98/pc98cons.c
            arch/i386/pc98/pc98disk.c
            arch/i386/pc98/pc98hw.c
            arch/i386/pc98/pc98mem.c
            arch/i386/pc98/pc98rtc.c
            arch/i386/pc98/pc98video.c)
    else()
        list(APPEND PCATLDR_ARC_SOURCE
            arch/i386/pc/machpc.c
            arch/i386/pc/pcbeep.c
            arch/i386/pc/pccons.c
            arch/i386/pc/pcdisk.c
            arch/i386/pc/pchw.c
            arch/i386/pc/pcmem.c
            arch/i386/pc/pcrtc.c
            arch/i386/pc/pcvesa.c
            arch/i386/pc/pcvideo.c)
    endif()

elseif(ARCH STREQUAL "amd64")
    list(APPEND PCATLDR_BASE_ASM_SOURCE
        arch/i386/multiboot.S)

    list(APPEND PCATLDR_COMMON_ASM_SOURCE
        arch/amd64/entry.S
        arch/amd64/int386.S
        arch/amd64/pnpbios.S
        arch/amd64/linux.S)

    list(APPEND PCATLDR_ARC_SOURCE
        lib/fs/pxe.c
        arch/i386/drivemap.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwdisk.c
        arch/i386/hwpci.c
        # arch/i386/i386bug.c
        arch/i386/pc/machpc.c
        arch/i386/pc/pcbeep.c
        arch/i386/pc/pccons.c
        arch/i386/pc/pcdisk.c
        arch/i386/pc/pchw.c
        arch/i386/pc/pcmem.c
        arch/i386/pc/pcrtc.c
        arch/i386/pc/pcvesa.c
        arch/i386/pc/pcvideo.c)

elseif(ARCH STREQUAL "arm")
    list(APPEND PCATLDR_COMMON_ASM_SOURCE
        arch/arm/boot.S)

    list(APPEND PCATLDR_ARC_SOURCE
        arch/arm/entry.c
        arch/arm/macharm.c)
else()
    #TBD
endif()

add_asm_files(freeldr_common_asm ${FREELDR_COMMON_ASM_SOURCE} ${PCATLDR_COMMON_ASM_SOURCE})

add_library(freeldr_common
    ${freeldr_common_asm}
    ${PCATLDR_ARC_SOURCE}
    ${FREELDR_BOOTLIB_SOURCE}
    ${PCATLDR_BOOTMGR_SOURCE}
)

if(MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # We need to reduce the binary size
    target_compile_options(freeldr_common PRIVATE "/Os")
endif()
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Prevent using SSE (no support in freeldr)
    target_compile_options(freeldr_common PUBLIC -mno-sse)
endif()

set(PCH_SOURCE
    ${PCATLDR_ARC_SOURCE}
    ${FREELDR_BOOTLIB_SOURCE}
    ${PCATLDR_BOOTMGR_SOURCE}
)

add_pch(freeldr_common include/freeldr.h PCH_SOURCE)
add_dependencies(freeldr_common bugcodes asm xdk)

## GCC builds need this extra thing for some reason...
if(ARCH STREQUAL "i386" AND NOT MSVC)
    target_link_libraries(freeldr_common mini_hal)
endif()

add_asm_files(freeldr_base_asm ${PCATLDR_BASE_ASM_SOURCE})

list(APPEND PCATLDR_BASE_SOURCE
    ${freeldr_base_asm}
    ${FREELDR_BASE_SOURCE})

add_executable(freeldr_pe ${PCATLDR_BASE_SOURCE})

set_target_properties(freeldr_pe
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

if(MSVC)
    if(ARCH STREQUAL "arm")
        target_link_options(freeldr_pe PRIVATE /ignore:4078 /ignore:4254 /DRIVER)
    else()
        target_link_options(freeldr_pe PRIVATE /ignore:4078 /ignore:4254 /DYNAMICBASE:NO /FIXED /FILEALIGN:512 /ALIGN:512)
        add_linker_script(freeldr_pe freeldr_i386.msvc.lds)
    endif()
    # We don't need hotpatching
    remove_target_compile_option(freeldr_pe "/hotpatch")
    remove_target_compile_option(freeldr_common "/hotpatch")
else()
    target_link_options(freeldr_pe PRIVATE -Wl,--exclude-all-symbols,--file-alignment,0x200,--section-alignment,0x200)
    add_linker_script(freeldr_pe freeldr_gcc.lds)
    # Strip everything, including rossym data
    add_custom_command(TARGET freeldr_pe
                    POST_BUILD
                    COMMAND ${CMAKE_STRIP} --remove-section=.rossym $<TARGET_FILE:freeldr_pe>
                    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:freeldr_pe>)
endif()

set_image_base(freeldr_pe 0x10000)
set_subsystem(freeldr_pe native)
set_entrypoint(freeldr_pe RealEntryPoint)

if(ARCH STREQUAL "i386")
    target_link_libraries(freeldr_pe mini_hal)
endif()

target_link_libraries(freeldr_pe freeldr_common cportlib libcntpr blrtl)

# dynamic analysis switches
if(STACK_PROTECTOR)
    target_sources(freeldr_pe PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

if(RUNTIME_CHECKS)
    target_link_libraries(freeldr_pe runtmchk)
    target_link_options(freeldr_pe PRIVATE "/MERGE:.rtc=.text")
endif()

add_dependencies(freeldr_pe asm)

if(SARCH STREQUAL "pc98")
    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/PC98)
    add_custom_target(pc98bootfdd
        COMMAND native-fatten ${REACTOS_BINARY_DIR}/PC98/ReactOS-98.IMG -format 2880 ROS98BOOT -boot ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/pc98/fat12fdd.bin -add ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys FREELDR.SYS -add ${CMAKE_SOURCE_DIR}/boot/bootdata/floppy_pc98.ini FREELDR.INI
        DEPENDS native-fatten fat12pc98 freeldr
        VERBATIM)
endif()

if(NOT ARCH STREQUAL "arm")
    concatenate_files(
        ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:freeldr_pe>)
    add_custom_target(freeldr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys)
else()
    add_custom_target(freeldr ALL DEPENDS freeldr_pe)
endif()

add_cd_file(TARGET freeldr FILE ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FOR bootcd livecd hybridcd regtest)
