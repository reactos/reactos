
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscms.dll mscms.spec)

add_library(mscms SHARED
    handle.c
    icc.c
    mscms_main.c
    profile.c
    stub.c
    transform.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mscms.def)

set_module_type(mscms win32dll)

target_link_libraries(mscms wine)

add_importlibs(mscms msvcrt kernel32 ntdll)

add_cab_target(mscms 1)
