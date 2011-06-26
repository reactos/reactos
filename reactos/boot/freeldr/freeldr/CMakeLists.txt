
if(ARCH MATCHES i386 OR ARCH MATCHES amd64)
    CreateBootSectorTarget2(frldr16
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/realmode/i386.S
        ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
        F800)
endif()

include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
include_directories(${REACTOS_SOURCE_DIR}/lib/cmlib)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/elf)

add_definitions(-D_NTHAL_ -D_BLDR_ -D_NTSYSTEM_)

if(ARCH MATCHES arm)
    if(SARCH MATCHES omap-zoom2)
        add_definitions(-D_ZOOM2_)
    endif()
endif()

list(APPEND FREELDR_COMMON_SOURCE
    cmdline.c
    debug.c
    linuxboot.c
    machine.c
    options.c
    oslist.c
    version.c
    arcemul/mm.c
    arcemul/time.c
    cache/blocklist.c
    cache/cache.c
    comm/rs232.c
    disk/disk.c
    disk/partition.c
    disk/ramdisk.c
    #disk/scsiport.c
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
)

if(ARCH MATCHES i386)
    list(APPEND FREELDR_COMMON_SOURCE
        arch/i386/archmach.c
        arch/i386/custom.c
        arch/i386/drivemap.c
        arch/i386/entry.S
        arch/i386/halstub.c
        arch/i386/hardware.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwpci.c
        arch/i386/i386bug.c
        arch/i386/i386disk.c
        arch/i386/i386idt.c
        arch/i386/i386pnp.cmake.S
        arch/i386/i386rtl.c
        arch/i386/i386trap.S
        arch/i386/i386vid.c
        arch/i386/linux.cmake.S
        arch/i386/loader.c
        arch/i386/machpc.c
        arch/i386/mb.S
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
        windows/i386/wlmemory.c
        windows/headless.c
        disk/scsiport.c)
    if(NOT MSVC)
        list(APPEND FREELDR_COMMON_SOURCE
            arch/i386/drvmap.S
            arch/i386/multiboot.S)
    endif()
elseif(ARCH MATCHES amd64)
    list(APPEND FREELDR_COMMON_SOURCE
        arch/amd64/entry.S
        arch/amd64/loader.c
        arch/i386/hardware.c
        arch/i386/hwacpi.c
        arch/i386/hwapm.c
        arch/i386/hwpci.c
        arch/i386/i386rtl.c
        arch/i386/i386disk.c
        arch/i386/i386vid.c
        arch/i386/machpc.c
        arch/i386/ntoskrnl.c
        arch/i386/pccons.c
        arch/i386/pcdisk.c
        arch/i386/pcmem.c
        arch/i386/pcrtc.c
        arch/i386/pcvideo.c
        windows/amd64/ntsetup.c
        windows/amd64/wlmemory.c)
else()
#TBD
endif()

add_library(freeldr_common ${FREELDR_COMMON_SOURCE})
add_dependencies(freeldr_common bugcodes)


list(APPEND FREELDR_BASE_SOURCE
    bootmgr.c # This file is compiled with custom definitions
    freeldr.c
    rtl/bget.c
    rtl/libsupp.c
)

add_executable(freeldr_pe ${FREELDR_BASE_SOURCE})

if(NOT MSVC)
    set_target_properties(freeldr_pe PROPERTIES LINK_FLAGS "-Wl,--strip-all -Wl,--exclude-all-symbols -Wl,--file-alignment,0x1000 -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/freeldr_i386.lnk")
else()
    set_target_properties(freeldr_pe PROPERTIES LINK_FLAGS "/ignore:4078 /ignore:4254 /DRIVER /FIXED /ALIGN:0x400 /SECTION:.text,ERW /SECTION:.data,RW /MERGE:.text16=.text /MERGE:.data=.text /MERGE:.rdata=.text /MERGE:.bss=.text /SUBSYSTEM:BOOT_APPLICATION")
endif()

set_image_base(freeldr_pe 0x10000)
set_subsystem(freeldr_pe native)
set_entrypoint(freeldr_pe RealEntryPoint)

if(ARCH MATCHES i386)
    target_link_libraries(freeldr_pe mini_hal)
endif()

target_link_libraries(freeldr_pe
    freeldr_common
    cportlib
    rossym
    cmlib
    rtl
    libcntpr)

add_pch(freeldr_pe include/freeldr.h)
add_dependencies(freeldr_pe asm)

concatenate_files(
    ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
    ${CMAKE_CURRENT_BINARY_DIR}/freeldr_pe.exe
    ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys)

add_custom_target(freeldr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys)

add_cd_file(TARGET freeldr FILE ${CMAKE_CURRENT_BINARY_DIR}/freeldr.sys DESTINATION loader NO_CAB FOR all)

list(APPEND SETUPLDR_SOURCE
    inffile/inffile.c
    reactos/setupldr.c)

if(ARCH MATCHES i386 OR ARCH MATCHES amd64)
    list(APPEND SETUPLDR_SOURCE windows/setupldr2.c)
endif()

if(NOT MSVC)
    set_source_files_properties(${SETUPLDR_SOURCE} PROPERTIES COMPILE_FLAGS "-ffreestanding -fno-builtin -fno-inline -fno-zero-initialized-in-bss")
endif()

add_executable(setupldr_pe ${FREELDR_BASE_SOURCE} ${SETUPLDR_SOURCE})

if(NOT MSVC)
    set_target_properties(setupldr_pe PROPERTIES LINK_FLAGS "-Wl,--strip-all -Wl,--exclude-all-symbols -Wl,--file-alignment,0x1000 -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/freeldr_i386.lnk" COMPILE_DEFINITIONS "FREELDR_REACTOS_SETUP")
else()
    set_target_properties(setupldr_pe PROPERTIES LINK_FLAGS "/ignore:4078 /ignore:4254 /DRIVER /FIXED /ALIGN:0x400 /SECTION:.text,ERW /SECTION:.data,RW /MERGE:.text16=.text /MERGE:.data=.text /MERGE:.rdata=.text /MERGE:.bss=.text /SUBSYSTEM:BOOT_APPLICATION" COMPILE_DEFINITIONS "FREELDR_REACTOS_SETUP")
endif()

set_image_base(setupldr_pe 0x10000)
set_subsystem(setupldr_pe native)
set_entrypoint(setupldr_pe RealEntryPoint)

if(ARCH MATCHES i386)
    target_link_libraries(setupldr_pe mini_hal)
endif()

target_link_libraries(setupldr_pe
    freeldr_common
    cportlib
    rossym
    cmlib
    rtl
    libcntpr)

add_dependencies(setupldr_pe asm)

concatenate_files(
    ${CMAKE_CURRENT_BINARY_DIR}/frldr16.bin
    ${CMAKE_CURRENT_BINARY_DIR}/setupldr_pe.exe
    ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys)

add_custom_target(setupldr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys)

add_cd_file(TARGET setupldr FILE ${CMAKE_CURRENT_BINARY_DIR}/setupldr.sys DESTINATION loader NO_CAB FOR all)

