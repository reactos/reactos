
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    wmvcore_main.c
    writer.c)

add_library(wmvcore MODULE ${SOURCE} version.rc)
spec2def(wmvcore wmvcore.dll wmvcore.spec ADD_IMPORTLIB)

set_module_type(wmvcore win32dll)
target_link_libraries(wmvcore wine)
add_importlibs(wmvcore msvcrt kernel32 ntdll)
add_cd_file(TARGET wmvcore DESTINATION reactos/system32 FOR all)
