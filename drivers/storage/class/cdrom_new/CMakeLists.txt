
include_directories(${REACTOS_SOURCE_DIR}/drivers/storage/class)

list(APPEND SOURCE
    cdrom.c
    data.c
    ioctl.c
    mmc.c
    sec.c
    cdrom.h)

add_library(cdrom_new SHARED ${SOURCE})

target_link_libraries(cdrom_new libcntpr wdmguid)

if(ARCH STREQUAL "i386")
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    if(NOT MSVC)
        #add_target_compile_flags(cdrom_new "-Wno-format -Wno-unused-variable -Wno-pointer-sign")
        set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-format -Wno-unused-variable -Wno-pointer-sign")
    endif()
endif()

set_module_type(cdrom_new kernelmodedriver)
add_importlibs(cdrom_new classpnp ntoskrnl hal)
add_pch(cdrom_new cdrom.h SOURCE)
