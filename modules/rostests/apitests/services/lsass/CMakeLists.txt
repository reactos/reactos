add_subdirectory(dll)

add_executable(lsass_apitest
               ../framework/remotetests.c
               ../framework/remotesvc.c
               shared/dbgutil.c
               tests.c)
set_module_type(lsass_apitest win32cui)
add_importlibs(lsass_apitest msvcrt advapi32 kernel32 ntdll)
add_rostests_file(TARGET lsass_apitest)
