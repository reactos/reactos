
include_directories(BEFORE include)
include_directories(BEFORE include/efi)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
include_directories(${REACTOS_SOURCE_DIR}/lib/cmlib)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs)

add_definitions(-D_NTHAL_ -D_BLDR_ -D_NTSYSTEM_)

list(APPEND BOOTLIB_SOURCE
     app/bootmgr/bootmgr.h
     lib/bootlib.c
     lib/misc/debug.c
     lib/misc/bcd.c
     lib/misc/bcdopt.c
     lib/misc/bootreg.c
     lib/misc/util.c
     lib/misc/image.c
     lib/firmware/efi/firmware.c
     lib/mm/mm.c
     lib/mm/pagealloc.c
     lib/mm/heapalloc.c
     lib/mm/blkalloc.c
     lib/mm/descriptor.c
     lib/platform/time.c
     lib/io/blkcache.c
     lib/io/etfs.c
     lib/io/io.c
     lib/io/device.c
     lib/io/file.c
     lib/io/fat.c
     lib/io/display/display.c
     lib/io/display/emscons.c
     lib/io/display/guicons.c
     lib/io/display/textcons.c
     lib/io/display/efi/textcons.c
     lib/io/display/efi/guicons.c
     lib/io/display/efi/gop.c
     lib/io/display/efi/uga.c)

if(ARCH STREQUAL "i386")
    list(APPEND BOOTLIB_ASM_SOURCE
         #lib/arch/i386/foo.asm
         )
    list(APPEND BOOTLIB_SOURCE
         lib/arch/i386/arch.c
         lib/mm/i386/mmx86.c
         )
elseif(ARCH STREQUAL "amd64")
    list(APPEND BOOTLIB_ASM_SOURCE
         #lib/arch/amd64/foo.asm
         )
    list(APPEND BOOTLIB_SOURCE
         #lib/arch/amd64/foo.c
         lib/arch/stub/arch.c
         lib/mm/stub/mm.c
         )
else()
    list(APPEND BOOTLIB_SOURCE
         lib/arch/stub/arch.c
         lib/mm/stub/mm.c
         )
endif()

add_asm_files(bootlib_asm ${BOOTLIB_ASM_SOURCE})
add_library(bootlib ${BOOTLIB_SOURCE} ${bootlib_asm})
add_pch(bootlib app/bootmgr/bootmgr.h BOOTLIB_SOURCE)
add_dependencies(bootlib bugcodes xdk)

list(APPEND BOOTMGR_BASE_SOURCE
     app/bootmgr/efiemu.c
     app/bootmgr/bootmgr.c
     app/bootmgr/rtlcompat.c
     )

add_executable(bootmgfw ${BOOTMGR_BASE_SOURCE})
set_target_properties(bootmgfw PROPERTIES SUFFIX ".efi")

if(MSVC)
    add_target_link_flags(bootmgfw "/ignore:4078 /ignore:4254 /DRIVER /FIXED")
else()
    add_target_link_flags(bootmgfw "-Wl,--strip-all,--exclude-all-symbols")
endif()

set_image_base(bootmgfw 0x10000)

if(MSVC)
    set_subsystem(bootmgfw EFI_APPLICATION)
else()
    set_subsystem(bootmgfw 10)
endif()

set_entrypoint(bootmgfw EfiEntry)

target_link_libraries(bootmgfw bootlib cportlib cmlib rtl libcntpr)

if(STACK_PROTECTOR)
    target_link_libraries(bootmgfw gcc_ssp)
elseif(RUNTIME_CHECKS)
    target_link_libraries(bootmgfw runtmchk)
endif()

add_dependencies(bootmgfw asm bugcodes)

add_cd_file(TARGET bootmgfw FILE ${_bootmgfw_output_file} DESTINATION efi/boot NO_CAB FOR bootcd regtest NAME_ON_CD boot${EFI_PLATFORM_ID}.efi)

