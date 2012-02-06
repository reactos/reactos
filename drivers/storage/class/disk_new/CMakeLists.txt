include_directories(..)

add_library(disk SHARED data.c disk.c diskwmi.c enum.c geometry.c part.c pnp.c disk.rc)

if(ARCH MATCHES i386)
    add_target_compile_flags(disk "-mrtd -fno-builtin -Wno-unused-variable -Wno-pointer-sign")
endif()

set_module_type(disk kernelmodedriver)
add_importlibs(disk
    classpnp
    scsiport
    ntoskrnl
    hal)

add_cd_file(TARGET disk DESTINATION reactos/system32/drivers NO_CAB FOR all)
