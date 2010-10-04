
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_executable(expand expand.c expand.rc)

set_module_type(expand win32cui)

add_importlibs(expand lz32 setupapi user32 msvcrt)
