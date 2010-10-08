
set_unicode()

add_executable(spoolsv spoolsv.c spoolsv.rc)

target_link_libraries(spoolsv wine)

set_module_type(spoolsv win32cui)
add_importlibs(spoolsv advapi32 msvcrt kernel32 ntdll)
