
spec2def(rasman ${CMAKE_CURRENT_SOURCE_DIR}/rasman.spec ${CMAKE_CURRENT_BINARY_DIR}/rasman.def)

add_library(rasman SHARED rasman.c rasman.rc)

set_module_type(rasman win32dll)

target_link_libraries(rasman
    ${CMAKE_CURRENT_BINARY_DIR}/rasman.def
    wine
    uuid)

add_importlibs(rasman kernel32 ntdll)
add_dependencies(rasman rasman_def)
add_livecd_target(rasman reactos/system32)
