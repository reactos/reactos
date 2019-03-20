
include_directories(${REACTOS_SOURCE_DIR}/drivers/storage/class)

list(APPEND SOURCE
    cdrom.c
    data.c
    ioctl.c
    mmc.c
    sec.c
    cdrom.h)

add_library(cdrom_new MODULE ${SOURCE})
target_link_libraries(cdrom_new libcntpr wdmguid)

if(USE_CLANG_CL OR (NOT MSVC))
    add_target_compile_flags(cdrom_new "-Wno-format -Wno-unused-variable -Wno-pointer-sign")
    if(USE_CLANG_CL)
        add_target_compile_flags(cdrom_new "-Wno-enum-conversion -Wno-tautological-constant-compare")
    endif()
endif()

set_module_type(cdrom_new kernelmodedriver)
add_importlibs(cdrom_new classpnp ntoskrnl hal)
add_pch(cdrom_new cdrom.h SOURCE)
