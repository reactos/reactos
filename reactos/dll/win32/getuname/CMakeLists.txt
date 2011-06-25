
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(getuname.dll getuname.spec ADD_IMPORTLIB)

add_library(getuname SHARED
    getuname.c
    getuname.rc
    ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

add_importlib_target(getuname.spec)
set_module_type(getuname win32dll)
add_importlibs(getuname msvcrt kernel32 ntdll)
add_cd_file(TARGET getuname DESTINATION reactos/system32 FOR all)
