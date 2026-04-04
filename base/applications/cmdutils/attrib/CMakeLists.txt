
add_executable(attrib attrib.c attrib.rc)
set_module_type(attrib win32cui UNICODE)
target_link_libraries(attrib conutils)
add_importlibs(attrib msvcrt kernel32)
add_cd_file(TARGET attrib DESTINATION reactos/system32 FOR all)
