
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(fontsub MODULE
    main.c)
spec2def(fontsub fontsub.dll fontsub.spec)

set_module_type(fontsub win32dll)
target_link_libraries(fontsub wine)
add_importlibs(fontsub msvcrt kernel32 ntdll)
add_cd_file(TARGET fontsub DESTINATION reactos/system32 FOR all)
