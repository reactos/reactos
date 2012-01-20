
spec2def(powrprof.dll powrprof.spec)


add_library(powrprof SHARED
    powrprof.c
    powrprof.rc
    ${CMAKE_CURRENT_BINARY_DIR}/powrprof.def)

set_module_type(powrprof win32dll UNICODE)

target_link_libraries(powrprof wine)

add_importlibs(powrprof advapi32 user32 comctl32 msvcrt kernel32 ntdll)
add_importlib_target(powrprof.spec)
add_cd_file(TARGET powrprof DESTINATION reactos/system32 FOR all)
