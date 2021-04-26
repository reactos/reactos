
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    aacs.c
    autorun.c
    cdrom.c
    common.c
    data.c
    guid.c
    init.c
    ioctl.c
    mmc.c
    pnppower.c
    scratch.c
    sense.c
    zpodd.c
    cdrom.h)

add_library(cdrom MODULE ${SOURCE} cdrom.rc)
set_module_type(cdrom kernelmodedriver)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cdrom PRIVATE -Wno-format -Wno-unused-variable -Wno-pointer-sign)
    target_compile_options(cdrom PRIVATE -Wno-unknown-pragmas -Wno-incompatible-pointer-types -Wno-switch)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cdrom PRIVATE -Wno-enum-conversion -Wno-tautological-constant-compare)
endif()

target_compile_definitions(cdrom PRIVATE DEBUG_USE_KDPRINT)

target_link_libraries(cdrom wdf01000 ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(cdrom ntoskrnl hal)
# add_pch(cdrom cdrom.h SOURCE)
add_cd_file(TARGET cdrom DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(cdrom_reg.inf)
add_driver_inf(cdrom cdrom.inf)
