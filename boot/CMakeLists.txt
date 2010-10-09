
CreateBootSectorTarget(dosmbr ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/dosmbr.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/dosmbr.bin)
CreateBootSectorTarget(ext2 ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/ext2.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/ext2.bin)
CreateBootSectorTarget(fat32 ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/fat32.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat32.bin)
CreateBootSectorTarget(fat ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/fat.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat.bin)
CreateBootSectorTarget(isoboot ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/isoboot.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin)
CreateBootSectorTarget(isobtrt ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/isobtrt.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin)

include_directories(BEFORE freeldr/freeldr/include)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

if(ARCH MATCHES i386)
list(APPEND FREELDR_STARTUP_SOURCE
    freeldr/freeldr/arch/i386/fathelp.S
    freeldr/freeldr/arch/i386/arch.S)
elseif(ARCH MATCHES amd64)
list(APPEND FREELDR_STARTUP_SOURCE
    freeldr/freeldr/arch/amd64/fathelp.S
    freeldr/freeldr/arch/amd64/arch.S)
endif(ARCH MATCHES i386)

if(ARCH MATCHES i386)
list(APPEND FREELDR_BASE64K_SOURCE
    freeldr/freeldr/arch/i386/boot.S
    freeldr/freeldr/arch/i386/drvmap.S
    freeldr/freeldr/arch/i386/i386cpu.S
    freeldr/freeldr/arch/i386/i386idt.S
    freeldr/freeldr/arch/i386/i386pnp.S
    freeldr/freeldr/arch/i386/i386trap.S
    freeldr/freeldr/arch/i386/int386.S
    freeldr/freeldr/arch/i386/linux.S
    freeldr/freeldr/arch/i386/mb.S)
elseif(ARCH MATCHES amd64)
list(APPEND FREELDR_BASE64K_SOURCE
    freeldr/freeldr/arch/amd64/drvmap.S
    freeldr/freeldr/arch/amd64/i386cpu.S
    freeldr/freeldr/arch/amd64/i386idt.S
    freeldr/freeldr/arch/amd64/i386trap.S
    freeldr/freeldr/arch/amd64/mb.S)
endif(ARCH MATCHES i386)

set_source_files_properties(${FREELDR_BASE64K_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_NTHAL_")

include_directories(${REACTOS_SOURCE_DIR}/lib/cmlib)

list(APPEND FREELDR_BASE_SOURCE
    freeldr/freeldr/arcemul/mm.c
    freeldr/freeldr/arcemul/time.c
    freeldr/freeldr/cache/blocklist.c
    freeldr/freeldr/cache/cache.c
    freeldr/freeldr/comm/rs232.c
    freeldr/freeldr/disk/disk.c
    freeldr/freeldr/disk/partition.c
    freeldr/freeldr/disk/ramdisk.c
    freeldr/freeldr/fs/ext2.c
    freeldr/freeldr/fs/fat.c
    freeldr/freeldr/fs/fs.c
    freeldr/freeldr/fs/iso.c
    freeldr/freeldr/fs/ntfs.c
    freeldr/freeldr/inifile/ini_init.c
    freeldr/freeldr/inifile/inifile.c
    freeldr/freeldr/inifile/parse.c
    freeldr/freeldr/mm/meminit.c
    freeldr/freeldr/mm/mm.c
    freeldr/freeldr/reactos/registry.c
    freeldr/freeldr/reactos/arcname.c
    freeldr/freeldr/reactos/archwsup.c
    freeldr/freeldr/reactos/binhive.c
    freeldr/freeldr/reactos/reactos.c
    freeldr/freeldr/reactos/imageldr.c
    freeldr/freeldr/rtl/bget.c
    freeldr/freeldr/rtl/libsupp.c
    freeldr/freeldr/ui/directui.c
    freeldr/freeldr/ui/gui.c
    freeldr/freeldr/ui/minitui.c
    freeldr/freeldr/ui/noui.c
    freeldr/freeldr/ui/tui.c
    freeldr/freeldr/ui/tuimenu.c
    freeldr/freeldr/ui/ui.c
    freeldr/freeldr/video/fade.c
    freeldr/freeldr/video/palette.c
    freeldr/freeldr/video/video.c
    freeldr/freeldr/windows/conversion.c
    freeldr/freeldr/windows/headless.c
    freeldr/freeldr/windows/peloader.c
    freeldr/freeldr/windows/winldr.c
    freeldr/freeldr/windows/wlmemory.c
    freeldr/freeldr/windows/wlregistry.c
    freeldr/freeldr/freeldr.c
    freeldr/freeldr/debug.c
    freeldr/freeldr/version.c
    freeldr/freeldr/cmdline.c
    freeldr/freeldr/machine.c
    freeldr/freeldr/options.c
    freeldr/freeldr/linuxboot.c
    freeldr/freeldr/oslist.c)

if(ARCH MATCHES i386)
list(APPEND FREELDR_BASE_SOURCE freeldr/freeldr/disk/scsiport.c)
endif(ARCH MATCHES i386)

set_source_files_properties(${FREELDR_BASE_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_NTHAL_;_BLDR_;_NTSYSTEM_")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/elf)

if(ARCH MATCHES i386)
list(APPEND FREELDR_ARCH_SOURCE
    freeldr/freeldr/arch/i386/_alloca.S
    freeldr/freeldr/arch/i386/archmach.c
    freeldr/freeldr/arch/i386/custom.c
    freeldr/freeldr/arch/i386/drivemap.c
    freeldr/freeldr/arch/i386/halstub.c
    freeldr/freeldr/arch/i386/hardware.c
    freeldr/freeldr/arch/i386/hwacpi.c
    freeldr/freeldr/arch/i386/hwapm.c
    freeldr/freeldr/arch/i386/hwpci.c
    freeldr/freeldr/arch/i386/i386disk.c
    freeldr/freeldr/arch/i386/i386rtl.c
    freeldr/freeldr/arch/i386/i386vid.c
    freeldr/freeldr/arch/i386/loader.c
    freeldr/freeldr/arch/i386/machpc.c
    freeldr/freeldr/arch/i386/miscboot.c
    freeldr/freeldr/arch/i386/ntoskrnl.c
    freeldr/freeldr/arch/i386/pccons.c
    freeldr/freeldr/arch/i386/pcdisk.c
    freeldr/freeldr/arch/i386/pcmem.c
    freeldr/freeldr/arch/i386/pcrtc.c
    freeldr/freeldr/arch/i386/pcvideo.c
    freeldr/freeldr/arch/i386/machxbox.c
    freeldr/freeldr/arch/i386/xboxcons.c
    freeldr/freeldr/arch/i386/xboxdisk.c
    freeldr/freeldr/arch/i386/xboxfont.c
    freeldr/freeldr/arch/i386/xboxhw.c
    freeldr/freeldr/arch/i386/xboxi2c.c
    freeldr/freeldr/arch/i386/xboxmem.c
    freeldr/freeldr/arch/i386/xboxrtc.c
    freeldr/freeldr/arch/i386/xboxvideo.c
    freeldr/freeldr/windows/i386/ntsetup.c
    freeldr/freeldr/windows/i386/wlmemory.c)
else()
#TBD
endif(ARCH MATCHES i386)

set_source_files_properties(${FREELDR_ARCH_SOURCE} PROPERTIES COMPILE_DEFINITIONS "_NTHAL_;_BLDR_;_NTSYSTEM_")

list(APPEND SETUPLDR_MAIN_SOURCE
    freeldr/freeldr/bootmgr.c
    freeldr/freeldr/inffile/inffile.c
    freeldr/freeldr/reactos/setupldr.c)

if(ARCH MATCHES i386)
list(APPEND SETUPLDR_MAIN_SOURCE freeldr/freeldr/windows/setupldr2.c)
elseif(ARCH MATCHES amd64)
list(APPEND SETUPLDR_MAIN_SOURCE freeldr/freeldr/windows/setupldr2.c)
endif(ARCH MATCHES i386)

set_source_files_properties(${SETUPLDR_MAIN_SOURCE} PROPERTIES COMPILE_FLAGS "-ffreestanding -fno-builtin -fno-inline -fno-zero-initialized-in-bss")



list(APPEND FREELDR_SOURCE
    freeldr/freeldr/bootmgr.c
    ${FREELDR_STARTUP_SOURCE}
    ${FREELDR_BASE64K_SOURCE}
    ${FREELDR_BASE_SOURCE}
    ${FREELDR_ARCH_SOURCE})

add_library(freeldr SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/freeldr_freeldr.h.gch
    ${FREELDR_SOURCE})

set_target_properties(freeldr PROPERTIES LINK_FLAGS "-Wl,--strip-all -Wl,-entry,_mainCRTStartup -Wl,--image-base,0x8000 -Wl,--exclude-all-symbols -Wl,--subsystem,native -Wl,--file-alignment,0x1000 -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds" SUFFIX ".sys")

if(ARCH MATCHES i386)
target_link_libraries(freeldr mini_hal)
endif(ARCH MATCHES i386)

target_link_libraries(freeldr
    cportlib
    rossym
    cmlib
    rtl
    libcntpr)
add_pch(freeldr ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/freeldr/include/freeldr.h ${FREELDR_SOURCE})

list(APPEND SETUPLDR_SOURCE
    ${FREELDR_STARTUP_SOURCE}
    ${FREELDR_BASE64K_SOURCE}
    ${FREELDR_BASE_SOURCE}
    ${FREELDR_ARCH_SOURCE}
    ${SETUPLDR_MAIN_SOURCE})

add_library(setupldr SHARED ${SETUPLDR_SOURCE})

set_target_properties(setupldr PROPERTIES LINK_FLAGS "-Wl,--strip-all -Wl,-entry,_mainCRTStartup -Wl,--image-base,0x8000 -Wl,--exclude-all-symbols -Wl,--subsystem,native -Wl,--file-alignment,0x1000" SUFFIX ".sys" COMPILE_DEFINITIONS "FREELDR_REACTOS_SETUP")

if(ARCH MATCHES i386)
target_link_libraries(setupldr mini_hal)
endif(ARCH MATCHES i386)

target_link_libraries(setupldr
    cportlib
    rossym
    cmlib
    rtl
    libcntpr)
