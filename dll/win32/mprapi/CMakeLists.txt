
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    mprapi.c)

add_library(mprapi MODULE ${SOURCE})
spec2def(mprapi mprapi.dll mprapi.spec)

set_module_type(mprapi win32dll)
target_link_libraries(mprapi wine)
add_importlibs(mprapi msvcrt kernel32 ntdll)
add_cd_file(TARGET mprapi DESTINATION reactos/system32 FOR all)
