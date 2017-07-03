
if(SEPARATE_DBG)
    # FIXME: http://sourceware.org/bugzilla/show_bug.cgi?id=11822
    set(CMAKE_LDR_PE_HELPER_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    set(CMAKE_LDR_PE_HELPER_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C Libraries")
endif()

if(MSVC)
    # We don't need it here
    replace_compile_flags("/hotpatch" " ")
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


list(APPEND FREELDR_BOOTLIB_COMMON_SOURCE
    lib/debug.c
    lib/peloader.c

    lib/comm/rs232.c
    ## add KD support
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
    lib/mm/heap.c
    )

list(APPEND FREELDR_NTLDR_COMMON_SOURCE
    ntldr/conversion.c
    ntldr/registry.c
    ntldr/winldr.c
    ntldr/wlmemory.c
    ntldr/wlregistry.c
    )

list(APPEND FREELDR_ARC_COMMON_SOURCE
    arcname.c
    machine.c
    arch/archwsup.c
    cache/blocklist.c
    cache/cache.c
    disk/disk.c
    disk/partition.c
    disk/ramdisk.c
    #disk/scsiport.c
    )

list(APPEND FREELDR_COMMON_SOURCE
    include/freeldr.h
    cmdline.c
    custom.c
    linuxboot.c
    miscboot.c
    options.c
    oslist.c
    version.c
    ui/directui.c
    ui/gui.c
    ui/minitui.c
    ui/noui.c
    ui/tui.c
    ui/tuimenu.c
    ui/ui.c
    video/fade.c
    video/palette.c
    video/video.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND FREELDR_COMMON_ASM_SOURCE
        arch/i386/entry.S
        arch/i386/int386.S
        arch/i386/pnpbios.S
        arch/i386/i386trap.S
        arch/i386/linux.S
        arch/i386/mb.S
        )

    list(APPEND FREELDR_NTLDR_COMMON_SOURCE
        ntldr/arch/i386/winldr.c
        ntldr/headless.c
        )

    ## list(APPEND FREELDR_COMMON_SOURCE
    list(APPEND FREELDR_ARC_COMMON_SOURCE
        lib/fs/pxe.c

        arch/i386/halstub.c
        arch/i386/ntoskrnl.c

        arch/i386/archmach.c
        arch/i386/drivemap.c
        arch/i386/hardware.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwdisk.c
        arch/i386/hwpci.c
        arch/i386/i386bug.c
        arch/i386/i386idt.c
        arch/i386/i386rtl.c
        arch/i386/i386vid.c
        arch/i386/machpc.c
        arch/i386/pccons.c
        arch/i386/pcdisk.c
        arch/i386/pcmem.c
        arch/i386/pcrtc.c
        arch/i386/pcvideo.c
        arch/i386/machxbox.c
        arch/i386/xboxcons.c
        arch/i386/xboxdisk.c
        arch/i386/xboxfont.c
        arch/i386/xboxi2c.c
        arch/i386/xboxmem.c
        arch/i386/xboxrtc.c
        arch/i386/xboxvideo.c
        disk/scsiport.c)
    if(NOT MSVC)
        list(APPEND FREELDR_COMMON_ASM_SOURCE arch/i386/drvmap.S)
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND FREELDR_COMMON_ASM_SOURCE
        arch/amd64/entry.S
        arch/amd64/int386.S
        arch/amd64/pnpbios.S)

    list(APPEND FREELDR_NTLDR_COMMON_SOURCE
        ntldr/arch/amd64/winldr.c
        )

    ## list(APPEND FREELDR_COMMON_SOURCE
    list(APPEND FREELDR_ARC_COMMON_SOURCE
        lib/fs/pxe.c

        arch/i386/ntoskrnl.c

        arch/i386/drivemap.c
        arch/i386/hardware.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwdisk.c
        arch/i386/hwpci.c
        arch/i386/i386bug.c
        arch/i386/i386rtl.c
        arch/i386/i386vid.c
        arch/i386/machpc.c
        arch/i386/pccons.c
        arch/i386/pcdisk.c
        arch/i386/pcmem.c
        arch/i386/pcrtc.c
        arch/i386/pcvideo.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND FREELDR_COMMON_ASM_SOURCE
        arch/arm/boot.S)

    list(APPEND FREELDR_NTLDR_COMMON_SOURCE
        ntldr/arch/arm/winldr.c
        )

    ## list(APPEND FREELDR_COMMON_SOURCE
    list(APPEND FREELDR_ARC_COMMON_SOURCE
        arch/arm/entry.c
        arch/arm/macharm.c)
else()
#TBD
endif()

add_asm_files(freeldr_common_asm ${FREELDR_COMMON_ASM_SOURCE})
add_library(freeldr_common ${FREELDR_BOOTLIB_COMMON_SOURCE} ${FREELDR_ARC_COMMON_SOURCE}
            ${FREELDR_NTLDR_COMMON_SOURCE}
            ${FREELDR_COMMON_SOURCE} ${freeldr_common_asm})
add_pch(freeldr_common include/freeldr.h FREELDR_COMMON_SOURCE)
add_dependencies(freeldr_common bugcodes asm xdk)

if(ARCH STREQUAL "i386" AND NOT MSVC)
    list(APPEND FREELDR_BASE_SOURCE arch/i386/multiboot.S)
    target_link_libraries(freeldr_common mini_hal)
endif()

list(APPEND FREELDR_BASE_SOURCE
    bootmgr.c # This file is compiled with custom definitions
    freeldr.c
    ntldr/setupldr.c ## Strangely enough this file is needed in GCC builds
                     ## even if ${FREELDR_NTLDR_COMMON_SOURCE} is not added,
                     ## otherwise we get linking errors with Rtl**Bitmap** APIs.
                     ## Do not happen on MSVC builds however...
    lib/inffile/inffile.c
    lib/rtl/libsupp.c)

if(ARCH STREQUAL "i386")
    # Must be included together with disk/scsiport.c
    list(APPEND FREELDR_BASE_SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/freeldr_pe.def)
endif()

add_executable(freeldr_pe ${FREELDR_BASE_SOURCE})
add_executable(freeldr_pe_dbg EXCLUDE_FROM_ALL ${FREELDR_BASE_SOURCE})

if(NOT MSVC AND SEPARATE_DBG)
    set_target_properties(freeldr_pe PROPERTIES LINKER_LANGUAGE LDR_PE_HELPER)
    set_target_properties(freeldr_pe_dbg PROPERTIES LINKER_LANGUAGE LDR_PE_HELPER)
endif()

if(MSVC)
    if(ARCH STREQUAL "arm")
        add_target_link_flags(freeldr_pe "/ignore:4078 /ignore:4254 /DRIVER")
        add_target_link_flags(freeldr_pe_dbg "/ignore:4078 /ignore:4254 /DRIVER")
    else()
        add_target_link_flags(freeldr_pe "/ignore:4078 /ignore:4254 /DRIVER /FIXED /ALIGN:0x400 /SECTION:.text,ERW /SECTION:.data,RW /MERGE:.text16=.text /MERGE:.data=.text /MERGE:.rdata=.text /MERGE:.bss=.text")
        add_target_link_flags(freeldr_pe_dbg "/ignore:4078 /ignore:4254 /DRIVER /FIXED /ALIGN:0x400 /SECTION:.text,ERW /SECTION:.data,RW /MERGE:.text16=.text /MERGE:.data=.text /MERGE:.rdata=.text /MERGE:.bss=.text")
    endif()
else()
    add_target_link_flags(freeldr_pe "-Wl,--strip-all,--exclude-all-symbols,--file-alignment,0x1000,-T,${CMAKE_CURRENT_SOURCE_DIR}/freeldr_i386.lds")
    add_target_link_flags(freeldr_pe_dbg "-Wl,--exclude-all-symbols,--file-alignment,0x1000,-T,${CMAKE_CURRENT_SOURCE_DIR}/freeldr_i386.lds")
endif()

set_image_base(freeldr_pe 0x10000)
set_subsystem(freeldr_pe native)
set_entrypoint(freeldr_pe RealEntryPoint)

set_image_base(freeldr_pe_dbg 0x10000)
set_subsystem(freeldr_pe_dbg native)
set_entrypoint(freeldr_pe_dbg RealEntryPoint)

if(ARCH STREQUAL "i386")
    target_link_libraries(freeldr_pe mini_hal)
    target_link_libraries(freeldr_pe_dbg mini_hal)
endif()

target_link_libraries(freeldr_pe freeldr_common cportlib cmlib rtl libcntpr)
target_link_libraries(freeldr_pe_dbg freeldr_common cportlib cmlib rtl libcntpr)

if(STACK_PROTECTOR)
    target_link_libraries(freeldr_pe gcc_ssp)
    target_link_libraries(freeldr_pe_dbg gcc_ssp)
elseif(RUNTIME_CHECKS)
    target_link_libraries(freeldr_pe runtmchk)
    target_link_libraries(freeldr_pe_dbg runtmchk)
    add_target_link_flags(freeldr_pe "/MERGE:.rtc=.text")
endif()

add_dependencies(freeldr_pe asm)
add_dependencies(freeldr_pe_dbg asm)

# Retrieve the full path to the generated file of the 'freeldr_pe' target
get_target_property(_freeldr_pe_output_file freeldr_pe LOCATION)

if(NOT ARCH STREQUAL "arm")
    concatenate_files(
        ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        ${_freeldr_pe_output_file})
    add_custom_target(freeldr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys)
else()
    add_custom_target(freeldr ALL DEPENDS ${_freeldr_pe_output_file})
endif()

add_cd_file(TARGET freeldr FILE ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys DESTINATION loader NO_CAB FOR bootcd regtest)

if(NOT ARCH STREQUAL "arm")
    concatenate_files(
        ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        ${_freeldr_pe_output_file})
    add_custom_target(setupldr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys)
else()
    add_custom_target(setupldr ALL DEPENDS ${_freeldr_pe_output_file})
endif()

add_cd_file(TARGET setupldr FILE ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys DESTINATION loader NO_CAB FOR bootcd regtest)
