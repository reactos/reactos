include_directories(..)

list(APPEND SOURCE
    cdrom.c
    data.c
    ioctl.c
    mmc.c
    sec.c)

add_library(cdrom_new SHARED ${SOURCE})

target_link_libraries(cdrom_new libcntpr wdmguid)

if(ARCH MATCHES i386)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    if(MSVC)
        #add_target_compile_flags(cdrom_new "/Gz")
        set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/Gz")
    else()
        #add_target_compile_flags(cdrom_new "-mrtd -fno-builtin -Wno-unused-variable -Wno-pointer-sign")
        set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin -Wno-unused-variable -Wno-pointer-sign")
    endif()
endif()

set_module_type(cdrom_new kernelmodedriver)
add_importlibs(cdrom_new classpnp ntoskrnl hal)
