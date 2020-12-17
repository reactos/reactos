
include_directories(..)

if(MSVC)
    # error C4101: 'ntNameBuffer' : unreferenced local variable in Release Configuration due to logging
    replace_compile_flags("/we4101" " ")
endif()

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
#disable pch to prevent redefinition of _WIN32_WINNT, DiskReadDriveCapacity and DiskGetDetectInfo in Release-Config
#add_pch(disk_new disk.h SOURCE)
add_cd_file(TARGET disk_new DESTINATION reactos/system32/drivers NO_CAB FOR all)
