
spec2def(rasman.dll rasman.spec)

add_library(rasman SHARED rasman.c rasman.rc ${CMAKE_CURRENT_BINARY_DIR}/rasman.def)

set_module_type(rasman win32dll)

target_link_libraries(rasman
    wine
    uuid)

add_importlibs(rasman msvcrt kernel32 ntdll)

add_cab_target(rasman 1)
