
set_unicode()

add_executable(dwnl dwnl.c)

set_module_type(dwnl win32cui)

target_link_libraries(dwnl uuid)

add_importlibs(dwnl urlmon wininet msvcrt kernel32)

add_livecd_target(dwnl reactos/system32)