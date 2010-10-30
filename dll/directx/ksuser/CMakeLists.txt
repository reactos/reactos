
spec2def(ksuser.dll ksuser.spec)

add_library(ksuser SHARED ksuser.c ksuser.rc ${CMAKE_CURRENT_BINARY_DIR}/ksuser.def)

set_module_type(ksuser win32dll)

add_importlibs(ksuser advapi32 kernel32 ntdll)
add_dependencies(ksuser psdk bugcodes)
add_cab_target(ksuser 1)
