
add_executable(autochk WIN32 autochk.c autochk.rc)

set_module_type(autochk nativecui)

target_link_libraries(autochk mingw_common nt)
add_importlibs(autochk kernel32 ntdll)
add_cd_file(TARGET autochk DESTINATION reactos/system32 FOR all)
