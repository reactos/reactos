include_directories(..)

add_library(cdrom_new SHARED cdrom.c data.c ioctl.c mmc.c sec.c)

target_link_libraries(cdrom_new libcntpr wdmguid)
if(ARCH MATCHES i386)
    if(MSVC)
        add_target_compile_flags(cdrom_new "/Gz")
    else()
        add_target_compile_flags(cdrom_new "-mrtd -fno-builtin -Wno-unused-variable -Wno-pointer-sign")
    endif()
endif()

set_module_type(cdrom_new kernelmodedriver)
add_importlibs(cdrom_new classpnp ntoskrnl hal)
