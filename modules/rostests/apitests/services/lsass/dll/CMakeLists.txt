
add_library(lsass_apitest.dll MODULE
            lsafntable.c
            lsamem.c
            lsatest.c
            ../../framework/clientdll.c
            ../shared/dbgutil.c)
set_module_type(lsass_apitest.dll win32dll UNICODE)
target_link_libraries(lsass_apitest.dll ${PSEH_LIB})
add_importlibs(lsass_apitest.dll msvcrt advapi32 kernel32 ntdll)
set_target_properties(lsass_apitest.dll PROPERTIES SUFFIX "")
add_rostests_file(TARGET lsass_apitest.dll)
