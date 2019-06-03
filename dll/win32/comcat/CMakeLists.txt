
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    comcat_main.c)

add_library(comcat MODULE ${SOURCE} version.rc)
spec2def(comcat comcat.dll comcat.spec)

set_module_type(comcat win32dll)
add_importlibs(comcat ole32 msvcrt kernel32)
add_cd_file(TARGET comcat DESTINATION reactos/system32 FOR all)
