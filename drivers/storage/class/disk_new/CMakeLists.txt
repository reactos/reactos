remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(..)

add_library(disk SHARED data.c disk.c diskwmi.c enum.c geometry.c part.c pnp.c disk.rc)

set_module_type(disk kernelmodedriver)
add_importlibs(disk
    class2
    scsiport
    ntoskrnl
    hal)

add_cd_file(TARGET disk DESTINATION reactos/system32/drivers NO_CAB FOR all)
