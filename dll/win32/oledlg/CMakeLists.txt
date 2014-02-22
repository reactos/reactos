
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(oledlg.dll oledlg.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    insobjdlg.c
    oledlg_main.c
    pastespl.c
    oledlg_private.h)

add_library(oledlg SHARED
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oledlg.def)

set_module_type(oledlg win32dll)
target_link_libraries(oledlg wine)
add_importlibs(oledlg ole32 comdlg32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(oledlg oledlg_private.h SOURCE)
add_cd_file(TARGET oledlg DESTINATION reactos/system32 FOR all)
