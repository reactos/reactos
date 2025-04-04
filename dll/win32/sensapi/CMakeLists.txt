
spec2def(sensapi.dll sensapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    sensapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/sensapi.def)

add_library(sensapi MODULE ${SOURCE})
set_module_type(sensapi win32dll)
target_link_libraries(sensapi wine)
add_importlibs(sensapi msvcrt kernel32 ntdll)
add_cd_file(TARGET sensapi DESTINATION reactos/system32 FOR all)
set_wine_module(sensapi)
