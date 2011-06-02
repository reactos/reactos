
include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

if(ARCH MATCHES arm)
    if(SARCH MATCHES omap-zoom2)
        add_definitions(-D_ZOOM2_)
    endif()
endif()

if(ARCH MATCHES i386)
    if(MSVC)
        list(APPEND FREELDR_BASE64K_SOURCE
            arch/i386/realmode.S)
    else()
        list(APPEND FREELDR_STARTUP_SOURCE
            arch/i386/fathelp.S
            arch/i386/arch.S)
    endif()
elseif(ARCH MATCHES amd64)
    list(APPEND FREELDR_STARTUP_SOURCE
        arch/i386/fathelp.S
        arch/amd64/arch.S)
endif()

if(ARCH MATCHES i386)
    if(NOT MSVC)
        list(APPEND FREELDR_BASE64K_SOURCE
            arch/i386/boot.S
            arch/i386/drvmap.S
            arch/i386/i386cpu.S
            arch/i386/i386idt.S
            arch/i386/i386pnp.S
            arch/i386/i386pxe.S
            arch/i386/i386trap.S
            arch/i386/int386.S
            arch/i386/linux.S
            arch/i386/mb.S
            arch/i386/i386bug.c)
    endif()
elseif(ARCH MATCHES amd64)
    list(APPEND FREELDR_BASE64K_SOURCE
        arch/i386/drvmap.S
        arch/i386/i386cpu.S
        arch/i386/i386idt.S
        arch/i386/i386trap.S
        arch/amd64/mb.S)
endif()

set_source_files_properties(${FREELDR_BASE64K_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_NTHAL_")

include_directories(${REACTOS_SOURCE_DIR}/lib/cmlib)

list(APPEND FREELDR_BASE_SOURCE
    arcemul/mm.c
    arcemul/time.c
    cache/blocklist.c
    cache/cache.c
    comm/rs232.c
    disk/disk.c
    disk/partition.c
    disk/ramdisk.c
    disk/scsiport.c
    fs/ext2.c
    fs/fat.c
    fs/fs.c
    fs/iso.c
    fs/ntfs.c
    fs/pxe.c
    inifile/ini_init.c
    inifile/inifile.c
    inifile/parse.c
    mm/meminit.c
    mm/mm.c
    reactos/registry.c
    reactos/arcname.c
    reactos/archwsup.c
    reactos/binhive.c
    reactos/reactos.c
    reactos/imageldr.c
    rtl/bget.c
    rtl/libsupp.c
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
    windows/conversion.c
    windows/peloader.c
    windows/winldr.c
    windows/wlmemory.c
    windows/wlregistry.c
    freeldr.c
    debug.c
    version.c
    cmdline.c
    machine.c
    options.c
    linuxboot.c
    oslist.c)

if(ARCH MATCHES i386)
    list(APPEND FREELDR_BASE_SOURCE
        windows/headless.c
        disk/scsiport.c)
endif()

set_source_files_properties(${FREELDR_BASE_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_NTHAL_;_BLDR_;_NTSYSTEM_")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/elf)

if(ARCH MATCHES i386)
    list(APPEND FREELDR_ARCH_SOURCE
        arch/i386/archmach.c
        arch/i386/custom.c
        arch/i386/drivemap.c
        arch/i386/halstub.c
        arch/i386/hardware.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwpci.c
        arch/i386/i386disk.c
        arch/i386/i386rtl.c
        arch/i386/i386vid.c
        arch/i386/loader.c
        arch/i386/machpc.c
        arch/i386/miscboot.c
        arch/i386/ntoskrnl.c
        arch/i386/pccons.c
        arch/i386/pcdisk.c
        arch/i386/pcmem.c
        arch/i386/pcrtc.c
        arch/i386/pcvideo.c
        arch/i386/machxbox.c
        arch/i386/xboxcons.c
        arch/i386/xboxdisk.c
        arch/i386/xboxfont.c
        arch/i386/xboxhw.c
        arch/i386/xboxi2c.c
        arch/i386/xboxmem.c
        arch/i386/xboxrtc.c
        arch/i386/xboxvideo.c
        windows/i386/ntsetup.c
        windows/i386/wlmemory.c)
else()
#TBD
endif()

set_source_files_properties(${FREELDR_ARCH_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_NTHAL_;_BLDR_;_NTSYSTEM_")
add_library(freeldr_arch ${FREELDR_ARCH_SOURCE})
add_dependencies(freeldr_arch bugcodes)

list(APPEND FREELDR_SOURCE
    bootmgr.c
    ${FREELDR_STARTUP_SOURCE}
    ${FREELDR_BASE64K_SOURCE}
    ${FREELDR_BASE_SOURCE}
    )

add_library(freeldr SHARED ${FREELDR_SOURCE})

if(NOT MSVC)
    set_target_properties(freeldr PROPERTIES LINK_FLAGS "-Wl,--strip-all -Wl,--exclude-all-symbols -Wl,--file-alignment,0x1000 -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/freeldr_i386.lnk" SUFFIX ".sys")
set_image_base(freeldr 0x8000)
else()
    set_target_properties(freeldr PROPERTIES LINK_FLAGS "/ignore:4078 /ignore:4254 /DRIVER /FIXED /ALIGN:0x400 /SECTION:.text,ERW /SECTION:.data,RW /MERGE:.text16=.text /MERGE:.data=.text /MERGE:.rdata=.text /MERGE:.bss=.text /SUBSYSTEM:BOOT_APPLICATION" SUFFIX ".sys")
set_image_base(freeldr 0x10000)
endif()

set_subsystem(freeldr native)
set_entrypoint(freeldr mainCRTStartup)

if(ARCH MATCHES i386)
    target_link_libraries(freeldr mini_hal)
endif()

target_link_libraries(freeldr
    freeldr_arch
    cportlib
    rossym
    cmlib
    rtl
    libcntpr)

add_pch(freeldr include/freeldr.h)
add_dependencies(freeldr asm)

add_cd_file(TARGET freeldr DESTINATION loader NO_CAB FOR all)

list(APPEND SETUPLDR_MAIN_SOURCE
    bootmgr.c
    inffile/inffile.c
    reactos/setupldr.c)

if(ARCH MATCHES i386 OR ARCH MATCHES amd64)
    list(APPEND SETUPLDR_MAIN_SOURCE windows/setupldr2.c)
endif()

if(NOT MSVC)
    set_source_files_properties(${SETUPLDR_MAIN_SOURCE} PROPERTIES COMPILE_FLAGS "-ffreestanding -fno-builtin -fno-inline -fno-zero-initialized-in-bss")
endif()

list(APPEND SETUPLDR_SOURCE
    ${FREELDR_STARTUP_SOURCE}
    ${FREELDR_BASE64K_SOURCE}
    ${FREELDR_BASE_SOURCE}
    ${SETUPLDR_MAIN_SOURCE})

add_library(setupldr SHARED ${SETUPLDR_SOURCE})

if(NOT MSVC)
    set_target_properties(setupldr PROPERTIES LINK_FLAGS "-Wl,--strip-all -Wl,--exclude-all-symbols -Wl,--file-alignment,0x1000 -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/freeldr_i386.lnk" SUFFIX ".sys" COMPILE_DEFINITIONS "FREELDR_REACTOS_SETUP")
set_image_base(setupldr 0x8000)
else()
    set_target_properties(setupldr PROPERTIES LINK_FLAGS "/DRIVER /SECTION:.text,ERWP,ALIGN=0x1000" SUFFIX ".sys" COMPILE_DEFINITIONS "FREELDR_REACTOS_SETUP")
endif()

set_subsystem(setupldr native)
set_entrypoint(setupldr mainCRTStartup)

if(ARCH MATCHES i386)
    target_link_libraries(setupldr mini_hal)
endif()

target_link_libraries(setupldr
    freeldr_arch
    cportlib
    rossym
    cmlib
    rtl
    libcntpr)

add_dependencies(setupldr asm)

add_cd_file(TARGET setupldr DESTINATION loader NO_CAB FOR all)

