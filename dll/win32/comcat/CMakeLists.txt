
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(comcat.dll comcat.spec)

list(APPEND SOURCE
    comcat_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/comcat.def)

add_library(comcat MODULE ${SOURCE} version.rc)
set_module_type(comcat win32dll)
add_importlibs(comcat ole32 msvcrt kernel32)
add_cd_file(TARGET comcat DESTINATION reactos/system32 FOR all)
