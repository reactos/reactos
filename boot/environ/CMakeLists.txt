
include_directories(BEFORE include)
include_directories(BEFORE include/efi)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/cmlib)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs)
include_directories(${REACTOS_SOURCE_DIR}/drivers/filesystems)

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
     lib/misc/resource.c
     lib/misc/font.c
     lib/misc/rtlcompat.c
     lib/rtl/libsupp.c
     lib/firmware/fwutil.c
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
         lib/arch/i386/transfer.s
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
add_dependencies(bootlib bugcodes asm bootmsg xdk)

list(APPEND BOOTMGR_BASE_SOURCE
     app/bootmgr/efiemu.c
     app/bootmgr/bootmgr.c
     )

add_executable(bootmgfw ${BOOTMGR_BASE_SOURCE} app/bootmgr/bootmgr.rc)
set_target_properties(bootmgfw PROPERTIES SUFFIX ".efi")

if(MSVC)
    if(ARCH STREQUAL "arm")
        target_link_options(bootmgfw PRIVATE /ignore:4078 /ignore:4254 /DRIVER)
    else()
        target_link_options(bootmgfw PRIVATE /ignore:4078 /ignore:4254 /DRIVER /DYNAMICBASE:NO /NXCOMPAT:NO /FIXED)
    endif()
else()
    target_link_options(bootmgfw PRIVATE "-Wl,--strip-all,--exclude-all-symbols")
endif()

set_image_base(bootmgfw 0x10000)

if(MSVC)
    set_subsystem(bootmgfw EFI_APPLICATION)
else()
    set_subsystem(bootmgfw 10)
endif()

set_entrypoint(bootmgfw EfiEntry)

target_link_libraries(bootmgfw bootlib cportlib cmlib rtl libcntpr)

# dynamic analysis switches
if(STACK_PROTECTOR)
    target_sources(bootmgfw PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

if(RUNTIME_CHECKS)
    target_link_libraries(bootmgfw runtmchk)
endif()

add_dependencies(bootmgfw asm bugcodes)

list(APPEND ROSLOAD_BASE_SOURCE
     app/rosload/rosload.c
     app/rosload/roslogo.c
     )

if(ARCH STREQUAL "i386")
    list(APPEND ROSLOAD_ASM_SOURCE
         app/rosload/i386/osxfera.s
         )
endif()

add_asm_files(rosload_asm ${ROSLOAD_ASM_SOURCE})
add_executable(rosload ${ROSLOAD_BASE_SOURCE} ${rosload_asm})
set_target_properties(rosload PROPERTIES SUFFIX ".efi")

if(MSVC)
    target_link_options(rosload PRIVATE /ignore:4078 /ignore:4254 /DRIVER)
else()
    target_link_options(rosload PRIVATE "-Wl,--strip-all,--exclude-all-symbols,--dynamicbase,--pic-executable")
endif()

set_image_base(rosload 0x10000)

if(MSVC)
    set_subsystem(rosload BOOT_APPLICATION)
else()
    set_subsystem(rosload 16)
endif()

if(ARCH STREQUAL "i386")
    set_entrypoint(rosload OslMain 4)
else()
    set_entrypoint(rosload OslMain)
endif()

target_link_libraries(rosload bootlib cportlib cmlib rtl libcntpr)

# dynamic analysis switches
if(STACK_PROTECTOR)
    target_sources(rosload PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

if(RUNTIME_CHECKS)
    target_link_libraries(rosload runtmchk)
endif()

add_dependencies(rosload asm bugcodes)

add_cd_file(TARGET rosload DESTINATION reactos/system32/boot NO_CAB FOR all)
