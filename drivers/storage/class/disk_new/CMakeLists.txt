include_directories(..)

add_library(disk_new SHARED data.c disk.c diskwmi.c enum.c geometry.c part.c pnp.c disk.rc)

target_link_libraries(disk_new libcntpr wdmguid)
if(ARCH MATCHES i386)
    if(MSVC)
        add_target_compile_flags(disk_new "/Gz")
    else()
        add_target_compile_flags(disk_new "-mrtd -fno-builtin -Wno-unused-variable -Wno-pointer-sign")
    endif()
endif()

set_module_type(disk_new kernelmodedriver)
add_importlibs(disk_new classpnp ntoskrnl hal)
add_cd_file(TARGET disk_new DESTINATION reactos/system32/drivers NO_CAB FOR all)
