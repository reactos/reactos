
spec2def(netid ${CMAKE_CURRENT_SOURCE_DIR}/netid.spec ${CMAKE_CURRENT_BINARY_DIR}/netid.def)

add_library(netid SHARED netid.c netid.rc)

set_module_type(netid win32dll)

target_link_libraries(netid
    ${CMAKE_CURRENT_BINARY_DIR}/netid.def)

add_importlibs(netid
    user32
    gdi32
    comctl32
    advapi32
    netapi32
    kernel32
    ntdll)

add_dependencies(netid netid_def)
