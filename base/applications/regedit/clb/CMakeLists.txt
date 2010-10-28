
spec2def(clb ${CMAKE_CURRENT_SOURCE_DIR}/clb.spec ${CMAKE_CURRENT_BINARY_DIR}/clb.def)

set_unicode()
set_rc_compiler()

add_library(clb SHARED
    clb.c
    clb.rc)

set_module_type(clb win32dll)

target_link_libraries(clb ${CMAKE_CURRENT_BINARY_DIR}/clb.def)
add_dependencies(clb clb_def)
add_importlibs(clb user32 gdi32 comctl32 kernel32 ntdll)
    