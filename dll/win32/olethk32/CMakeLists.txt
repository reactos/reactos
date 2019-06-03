
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    main.c
    version.rc)

add_library(olethk32 MODULE ${SOURCE})
spec2def(olethk32 olethk32.dll olethk32.spec)

set_module_type(olethk32 win32dll)
target_link_libraries(olethk32 wine)
add_importlibs(olethk32 msvcrt kernel32 ntdll)
add_cd_file(TARGET olethk32 DESTINATION reactos/system32 FOR all)
