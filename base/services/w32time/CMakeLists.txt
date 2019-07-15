
spec2def(w32time.dll w32time.spec ADD_IMPORTLIB)

add_library(w32time SHARED
    w32time.c
    ntpclient.c
    ${CMAKE_CURRENT_BINARY_DIR}/w32time.def)

set_module_type(w32time win32dll UNICODE)

add_importlibs(w32time ws2_32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET w32time DESTINATION reactos/system32 FOR all)
