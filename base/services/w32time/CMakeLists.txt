
spec2def(W32Time.dll W32Time.spec ADD_IMPORTLIB)

add_library(W32Time SHARED
    W32Time.c
    ntpclient.c
    ${CMAKE_CURRENT_BINARY_DIR}/w32time.def)

set_module_type(W32Time win32dll UNICODE)

add_importlib(W32Time ws2_32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET W32Time DESTINATION reactos/system32 FOR all)
