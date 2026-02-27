
add_executable(conime WIN32 conime.c conime.rc)
set_module_type(conime win32gui UNICODE)
target_link_libraries(conime wine ${PSEH_LIB})
add_importlibs(conime imm32 advapi32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET conime DESTINATION reactos/system32 FOR all)
