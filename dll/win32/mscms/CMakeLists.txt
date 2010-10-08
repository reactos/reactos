
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscms ${CMAKE_CURRENT_SOURCE_DIR}/mscms.spec ${CMAKE_CURRENT_BINARY_DIR}/mscms.def)

add_library(mscms SHARED
    handle.c
    icc.c
    mscms_main.c
    profile.c
    stub.c
    transform.c
    version.rc)

set_module_type(mscms win32dll)

target_link_libraries(mscms
    ${CMAKE_CURRENT_BINARY_DIR}/mscms.def
    wine)

add_importlibs(mscms kernel32 ntdll)
add_dependencies(mscms mscms_def)
