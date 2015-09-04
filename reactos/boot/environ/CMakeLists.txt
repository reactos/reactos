
include_directories(BEFORE include)
include_directories(BEFORE include/efi)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
include_directories(${REACTOS_SOURCE_DIR}/lib/cmlib)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs)

add_definitions(-D_NTHAL_ -D_BLDR_ -D_NTSYSTEM_)

list(APPEND BOOTMGR_COMMON_SOURCE
     app/bootmgr/bootmgr.h
     lib/bootlib.c
     lib/misc/util.c)

if(ARCH STREQUAL "i386")
    list(APPEND BOOTMGR_COMMON_ASM_SOURCE
         #lib/arch/i386/foo.asm
         )
    list(APPEND BOOTMGR_COMMON_SOURCE
         #lib/arch/i386/foo.c
         )
elseif(ARCH STREQUAL "amd64")
    list(APPEND BOOTMGR_COMMON_ASM_SOURCE
         #lib/arch/amd64/foo.asm
         )
    list(APPEND BOOTMGR_COMMON_SOURCE
         #lib/arch/amd64/foo.c
         )
else()
#TBD
endif()

add_asm_files(bootmgr_common_asm ${BOOTMGR_COMMON_ASM_SOURCE})
add_library(bootmgr_common ${BOOTMGR_COMMON_SOURCE} ${bootmgr_common_asm})
add_pch(bootmgr_common app/bootmgr/bootmgr.h BOOTMGR_COMMON_SOURCE)
#add_dependencies(bootmgr_common bugcodes)

list(APPEND BOOTMGR_BASE_SOURCE
     app/bootmgr/efiemu.c
     app/bootmgr/bootmgr.c
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

target_link_libraries(bootmgfw bootmgr_common cportlib cmlib rtl libcntpr)

add_dependencies(bootmgfw asm bugcodes)

add_cd_file(TARGET bootmgfw FILE ${_bootmgfw_output_file} DESTINATION loader NO_CAB FOR bootcd regtest)

