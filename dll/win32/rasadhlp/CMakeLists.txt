
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/winsock)

list(APPEND SOURCE
    autodial.c
    init.c
    winsock.c
    precomp.h)

add_library(rasadhlp MODULE ${SOURCE})
spec2def(rasadhlp rasadhlp.dll rasadhlp.spec)

set_module_type(rasadhlp win32dll)
add_importlibs(rasadhlp ws2_32 msvcrt kernel32 ntdll)
add_pch(rasadhlp precomp.h SOURCE)
add_cd_file(TARGET rasadhlp DESTINATION reactos/system32 FOR all)
