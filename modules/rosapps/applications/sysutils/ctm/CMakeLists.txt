
add_executable(ctm ctm.c ctm.rc)
set_module_type(ctm win32cui UNICODE)
target_link_libraries(ctm epsapi)
add_importlibs(ctm ntdll user32 msvcrt kernel32)
add_cd_file(TARGET ctm DESTINATION reactos/system32 FOR all)
