
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    traffic_main.c)

add_library(traffic MODULE ${SOURCE})
spec2def(traffic traffic.dll traffic.spec)

set_module_type(traffic win32dll)
target_link_libraries(traffic wine)
add_importlibs(traffic msvcrt kernel32 ntdll)
add_cd_file(TARGET traffic DESTINATION reactos/system32 FOR all)
