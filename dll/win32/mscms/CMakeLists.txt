
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(mscms.dll mscms.spec)

list(APPEND SOURCE
    handle.c
    icc.c
    mscms_main.c
    profile.c
    stub.c
    transform.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mscms_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mscms.def)

add_library(mscms SHARED ${SOURCE})
set_module_type(mscms win32dll)
target_link_libraries(mscms wine)
add_importlibs(mscms msvcrt advapi32 kernel32 ntdll)
add_cd_file(TARGET mscms DESTINATION reactos/system32 FOR all)
