
spec2def(netid.dll netid.spec)

add_library(netid SHARED netid.c netid.rc ${CMAKE_CURRENT_BINARY_DIR}/netid.def)

set_module_type(netid win32dll)

add_importlibs(netid user32 gdi32 comctl32 advapi32 netapi32 kernel32 ntdll)

add_cab_target(netid 1)
