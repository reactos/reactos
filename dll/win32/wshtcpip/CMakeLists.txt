
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/tdilib)

list(APPEND SOURCE
    iflist.c
    wshtcpip.c
    wshtcpip.rc)

add_library(wshtcpip MODULE ${SOURCE})
spec2def(wshtcpip wshtcpip.dll wshtcpip.spec)

set_module_type(wshtcpip win32dll UNICODE)
target_link_libraries(wshtcpip tdilib)
add_importlibs(wshtcpip ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wshtcpip DESTINATION reactos/system32 FOR all)
