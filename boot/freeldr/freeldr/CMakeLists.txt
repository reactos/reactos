
if(MSVC)
    # Explicitly use string pooling
    add_compile_options("/GF")
endif()

spec2def(freeldr_pe.exe freeldr.spec)

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

include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/cmlib)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/elf)

add_definitions(-D_NTHAL_ -D_BLDR_ -D_NTSYSTEM_)

list(APPEND FREELDR_BOOTLIB_SOURCE
    lib/arcsupp.c
    lib/debug.c
    lib/peloader.c
    lib/cache/blocklist.c
    lib/cache/cache.c
    lib/comm/rs232.c
    ## add KD support
    lib/fs/btrfs.c
    lib/fs/ext2.c
    lib/fs/fat.c
    lib/fs/fs.c
    lib/fs/iso.c
    lib/fs/ntfs.c
    lib/inifile/ini_init.c
    lib/inifile/inifile.c
    lib/inifile/parse.c
    lib/mm/meminit.c
    lib/mm/mm.c
    lib/mm/heap.c)

list(APPEND FREELDR_NTLDR_SOURCE
    ntldr/conversion.c
    ntldr/registry.c
    ntldr/winldr.c
    ntldr/wlmemory.c
    ntldr/wlregistry.c)

list(APPEND FREELDR_ARC_SOURCE
    arcname.c
    arch/drivers/hwide.c
    arch/arcemul.c
    arch/archwsup.c
    disk/disk.c
    disk/partition.c
    disk/ramdisk.c)
    #disk/scsiport.c

list(APPEND FREELDR_BOOTMGR_SOURCE
    include/freeldr.h
    cmdline.c
    custom.c
    linuxboot.c
    miscboot.c
    options.c
    oslist.c
    ui/directui.c
    ui/gui.c
    ui/minitui.c
    ui/noui.c
    ui/tui.c
    ui/tuimenu.c
    ui/ui.c
    ui/video.c)

list(APPEND FREELDR_BASE_ASM_SOURCE)

if(ARCH STREQUAL "i386")
    list(APPEND FREELDR_BASE_ASM_SOURCE
        arch/i386/multiboot.S)

    list(APPEND FREELDR_COMMON_ASM_SOURCE
        arch/i386/drvmap.S
        arch/i386/entry.S
        arch/i386/int386.S
        arch/i386/pnpbios.S
        arch/i386/i386trap.S
        arch/i386/linux.S)

    list(APPEND FREELDR_NTLDR_SOURCE
        ntldr/arch/i386/winldr.c
        ntldr/headless.c)

    list(APPEND FREELDR_ARC_SOURCE
        lib/fs/pxe.c
        arch/i386/halstub.c
        arch/i386/ntoskrnl.c
        arch/i386/drivemap.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwdisk.c
        arch/i386/hwpci.c
        arch/i386/i386bug.c
        arch/i386/i386idt.c
        disk/scsiport.c)

    if(SARCH STREQUAL "pc98" OR SARCH STREQUAL "xbox")
        # These machine types require built-in bitmap font
        list(APPEND FREELDR_ARC_SOURCE
            arch/vgafont.c)
    endif()

    if(SARCH STREQUAL "xbox")
        list(APPEND FREELDR_ARC_SOURCE
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
    elseif(SARCH STREQUAL "pc98")
        list(APPEND FREELDR_ARC_SOURCE
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
        list(APPEND FREELDR_ARC_SOURCE
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
    list(APPEND FREELDR_COMMON_ASM_SOURCE
        arch/amd64/entry.S
        arch/amd64/int386.S
        arch/amd64/pnpbios.S
        arch/amd64/linux.S)

    list(APPEND FREELDR_NTLDR_SOURCE
        ntldr/arch/amd64/winldr.c)

    list(APPEND FREELDR_ARC_SOURCE
        lib/fs/pxe.c
        arch/i386/ntoskrnl.c
        arch/i386/drivemap.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwdisk.c
        arch/i386/hwpci.c
        arch/i386/i386bug.c
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
    list(APPEND FREELDR_COMMON_ASM_SOURCE
        arch/arm/boot.S)

    list(APPEND FREELDR_NTLDR_SOURCE
        ntldr/arch/arm/winldr.c)

    list(APPEND FREELDR_ARC_SOURCE
        arch/arm/entry.c
        arch/arm/macharm.c)
else()
#TBD
endif()

add_asm_files(freeldr_common_asm ${FREELDR_COMMON_ASM_SOURCE})

add_library(freeldr_common
    ${freeldr_common_asm}
    ${FREELDR_BOOTLIB_SOURCE}
    ${FREELDR_ARC_SOURCE}
    ${FREELDR_NTLDR_SOURCE}
    ${FREELDR_BOOTMGR_SOURCE})

if(USE_CLANG_CL)
    # We need to reduce the binary size
    target_compile_options(freeldr_common PRIVATE "/Os")
endif()
if(NOT MSVC AND ARCH STREQUAL "i386" AND SARCH STREQUAL "xbox")
  # Prevent a warning when doing a memcmp with address 0
  set_source_files_properties(arch/i386/xbox/xboxmem.c PROPERTIES COMPILE_FLAGS "-Wno-nonnull")
endif()
if(ARCH STREQUAL "amd64" AND NOT MSVC)
    # Prevent using SSE (not support in freeldr)
    target_compile_options(freeldr_common PUBLIC -mno-sse)
endif()

set(PCH_SOURCE
    ${FREELDR_BOOTLIB_SOURCE}
    ${FREELDR_ARC_SOURCE}
    ${FREELDR_NTLDR_SOURCE}
    ${FREELDR_BOOTMGR_SOURCE})

add_pch(freeldr_common include/freeldr.h PCH_SOURCE)
add_dependencies(freeldr_common bugcodes asm xdk)

## GCC builds need this extra thing for some reason...
if(ARCH STREQUAL "i386" AND NOT MSVC)
    target_link_libraries(freeldr_common mini_hal)
endif()

add_asm_files(freeldr_base_asm ${FREELDR_BASE_ASM_SOURCE})

list(APPEND FREELDR_BASE_SOURCE
    ${freeldr_base_asm}
    bootmgr.c # This file is compiled with custom definitions
    freeldr.c
    ntldr/setupldr.c ## Strangely enough this file is needed in GCC builds
                     ## even if ${FREELDR_NTLDR_SOURCE} is not added,
                     ## otherwise we get linking errors with Rtl**Bitmap** APIs.
                     ## Do not happen on MSVC builds however...
    ntldr/inffile.c
    ntldr/ntldropts.c
    lib/rtl/libsupp.c)

if(ARCH STREQUAL "i386")
    # Must be included together with disk/scsiport.c
    list(APPEND FREELDR_BASE_SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/freeldr_pe.def)
endif()

add_executable(freeldr_pe ${FREELDR_BASE_SOURCE})

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

target_link_libraries(freeldr_pe freeldr_common cportlib cmlib rtl libcntpr)

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

# Rename freeldr on livecd to setupldr.sys because isoboot.bin looks for setupldr.sys
add_cd_file(TARGET freeldr FILE ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys DESTINATION loader NO_CAB FOR bootcd regtest)
add_cd_file(TARGET freeldr FILE ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FOR livecd hybridcd NAME_ON_CD setupldr.sys)

if(NOT ARCH STREQUAL "arm")
    concatenate_files(
        ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:freeldr_pe>)
    add_custom_target(setupldr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys)
else()
    add_custom_target(setupldr ALL DEPENDS freeldr_pe)
endif()

add_cd_file(TARGET setupldr FILE ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys DESTINATION loader NO_CAB FOR bootcd regtest)
