
add_executable(subst subst.c subst.rc)
set_module_type(subst win32cui UNICODE)
target_link_libraries(subst conutils)
add_importlibs(subst msvcrt kernel32)
add_cd_file(TARGET subst DESTINATION reactos/system32 FOR all)
