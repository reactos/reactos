
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_executable(expand expand.c expand.rc)
set_module_type(expand win32cui)
add_importlibs(expand lz32 setupapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET expand DESTINATION reactos/system32 FOR all)
