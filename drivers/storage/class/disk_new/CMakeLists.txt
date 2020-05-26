
include_directories(..)

list(APPEND SOURCE
    data.c
    disk.c
    diskwmi.c
    enum.c
    geometry.c
    part.c
    pnp.c
    disk.h)

add_library(disk_new MODULE ${SOURCE} disk.rc)
target_link_libraries(disk_new libcntpr wdmguid)

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(disk_new PRIVATE -Wno-format -Wno-pointer-sign)
endif()

set_module_type(disk_new kernelmodedriver)
add_importlibs(disk_new classpnp ntoskrnl hal)
add_pch(disk_new disk.h SOURCE)
add_cd_file(TARGET disk_new DESTINATION reactos/system32/drivers NO_CAB FOR all)
#add_driver_inf(disk_new diskdev.inf)
