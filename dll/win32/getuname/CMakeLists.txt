

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(getuname.dll getuname.spec)

add_library(getuname SHARED
    getuname.c
    getuname.rc
    ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)


set_module_type(getuname win32dll UNICODE)
add_importlibs(getuname user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET getuname DESTINATION reactos/system32 FOR all)
