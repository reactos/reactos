
set_unicode()

spec2def(powrprof ${CMAKE_CURRENT_SOURCE_DIR}/powrprof.spec ${CMAKE_CURRENT_BINARY_DIR}/powrprof.def)

add_library(powrprof SHARED powrprof.c powrprof.rc)

set_module_type(powrprof win32dll)

target_link_libraries(powrprof
    ${CMAKE_CURRENT_BINARY_DIR}/powrprof.def
    wine)

add_importlibs(powrprof advapi32 user32 comctl32 kernel32 ntdll)
add_dependencies(powrprof powrprof_def)
add_livecd_target(powrprof reactos/system32)
