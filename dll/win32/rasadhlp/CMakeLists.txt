
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/winsock)

spec2def(rasadhlp.dll rasadhlp.spec)

add_library(rasadhlp SHARED
    autodial.c
    init.c
    winsock.c
    ${CMAKE_CURRENT_BINARY_DIR}/rasadhlp.def)

set_module_type(rasadhlp win32dll)

add_importlibs(rasadhlp ws2_32 msvcrt kernel32 ntdll)

add_cab_target(rasadhlp 1)
