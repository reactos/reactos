
spec2def(powrprof.dll powrprof.spec)

set_unicode()

add_library(powrprof SHARED
            powrprof.c
            powrprof.rc
            ${CMAKE_CURRENT_BINARY_DIR}/powrprof.def)

set_module_type(powrprof win32dll)

target_link_libraries(powrprof wine)

add_importlibs(powrprof advapi32 user32 comctl32 kernel32 ntdll)

add_cab_target(powrprof 1)
add_importlib_target(powrprof.spec)
