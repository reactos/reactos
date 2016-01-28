
add_executable(spoolsv spoolsv.c spoolsv.rc)
target_link_libraries(spoolsv wine)
set_module_type(spoolsv win32cui UNICODE)
add_importlibs(spoolsv advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET spoolsv DESTINATION reactos/system32 FOR all)
