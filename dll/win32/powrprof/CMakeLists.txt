
add_library(powrprof MODULE
    powrprof.c
    powrprof.rc)
spec2def(powrprof powrprof.dll powrprof.spec ADD_IMPORTLIB)

set_module_type(powrprof win32dll UNICODE)
target_link_libraries(powrprof wine)
add_importlibs(powrprof advapi32 user32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET powrprof DESTINATION reactos/system32 FOR all)
