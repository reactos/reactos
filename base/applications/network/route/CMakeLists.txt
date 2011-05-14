
add_definitions(-D_DLL -D__USE_CRTIMP)

set_unicode()

add_executable(route route.c route.rc)

set_module_type(route win32cui)
add_importlibs(route ws2_32 iphlpapi msvcrt kernel32)
add_cd_file(TARGET route DESTINATION reactos/system32 FOR all)
