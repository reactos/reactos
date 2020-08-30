
remove_definitions(-D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    data.c
    disk.c
    diskwmi.c
    geometry.c
    pnp.c
    disk.h)

add_library(disk MODULE ${SOURCE} disk.rc)

target_compile_definitions(disk PUBLIC
    DEBUG_USE_KDPRINT
    _WIN32_WINNT=0x602
    NTDDI_VERSION=0x06020000) # NTDDI_WIN8

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(disk PRIVATE -Wno-format -Wno-pointer-sign)
endif()

if(GCC)
    target_compile_options(disk PRIVATE -Wno-unused-but-set-variable -Wno-pointer-to-int-cast -Wno-switch)
endif()

set_module_type(disk kernelmodedriver)
target_link_libraries(disk ntoskrnl_vista libcntpr wdmguid)
add_importlibs(disk classpnp ntoskrnl hal)
add_cd_file(TARGET disk DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(disk_reg.inf)
add_driver_inf(disk disk.inf)
