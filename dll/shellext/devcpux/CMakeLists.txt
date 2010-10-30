
set_unicode()

spec2def(devcpux ${CMAKE_CURRENT_SOURCE_DIR}/devcpux.spec ${CMAKE_CURRENT_BINARY_DIR}/devcpux.def)

add_library(devcpux SHARED processor.c processor.rc)

set_module_type(devcpux win32dll)

target_link_libraries(devcpux ${CMAKE_CURRENT_BINARY_DIR}/devcpux.def)

add_importlibs(devcpux
    user32
    powrprof
    comctl32
    kernel32
    ntdll)

add_dependencies(devcpux devcpux_def)
add_cab_target(devcpux 1)
