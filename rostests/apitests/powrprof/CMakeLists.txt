
add_executable(powrprof_apitest power.c testlist.c)
set_module_type(powrprof_apitest win32cui)
add_importlibs(powrprof_apitest advapi32 powrprof msvcrt kernel32 ntdll)
add_cd_file(TARGET powrprof_apitest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    allow_warnings(powrprof_apitest)
endif()
