
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    main.c)

add_library(mssip32 MODULE ${SOURCE})
spec2def(mssip32 mssip32.dll mssip32.spec)

set_module_type(mssip32 win32dll)
target_link_libraries(mssip32 wine)
add_importlibs(mssip32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mssip32 DESTINATION reactos/system32 FOR all)
