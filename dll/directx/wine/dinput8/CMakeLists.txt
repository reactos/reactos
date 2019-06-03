
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    dinput8_main.c)

add_library(dinput8 MODULE ${SOURCE} version.rc)
spec2def(dinput8 dinput8.dll dinput8.spec ADD_IMPORTLIB)

set_module_type(dinput8 win32dll)
target_link_libraries(dinput8 dxguid uuid wine)
add_importlibs(dinput8 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput8 DESTINATION reactos/system32 FOR all)
