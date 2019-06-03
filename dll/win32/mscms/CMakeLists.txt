
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    handle.c
    icc.c
    mscms_main.c
    profile.c
    stub.c
    transform.c)

add_library(mscms MODULE ${SOURCE} version.rc)
spec2def(mscms mscms.dll mscms.spec)

set_module_type(mscms win32dll)
target_link_libraries(mscms wine)
add_importlibs(mscms advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mscms DESTINATION reactos/system32 FOR all)
