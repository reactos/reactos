
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(oledlg.dll oledlg.spec)

add_library(oledlg SHARED
    insobjdlg.c
    oledlg_main.c
    pastespl.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oledlg.def)

set_module_type(oledlg win32dll)

target_link_libraries(oledlg wine)

add_importlibs(oledlg ole32 comdlg32 user32 advapi32 kernel32 ntdll)

add_cab_target(oledlg 1)
