
spec2def(w32time.dll w32time.spec ADD_IMPORTLIB)

add_library(w32time MODULE
    w32time.c
    ntpclient.c
    register.c
    w32time.rc
    ${CMAKE_CURRENT_BINARY_DIR}/w32time.def)

set_module_type(w32time win32dll UNICODE)

add_importlibs(w32time ws2_32 advapi32 msvcrt user32 kernel32 ntdll)
add_cd_file(TARGET w32time DESTINATION reactos/system32 FOR all)
