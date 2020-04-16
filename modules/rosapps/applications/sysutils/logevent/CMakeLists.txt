
add_executable(logevent logevent.c logevent.rc)
set_module_type(logevent win32cui)
if(NOT MSVC)
    target_compile_options(logevent PRIVATE "-Wno-unused-but-set-variable")
endif()
add_importlibs(logevent advapi32 msvcrt kernel32)
add_cd_file(TARGET logevent DESTINATION reactos/system32 FOR all)
