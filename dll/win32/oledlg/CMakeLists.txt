
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    insobjdlg.c
    oledlg_main.c
    pastespl.c
    precomp.h)

add_library(oledlg MODULE
    ${SOURCE}
    rsrc.rc)
spec2def(oledlg oledlg.dll oledlg.spec ADD_IMPORTLIB)

set_module_type(oledlg win32dll)
target_link_libraries(oledlg wine)
add_importlibs(oledlg ole32 comdlg32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(oledlg precomp.h SOURCE)
add_cd_file(TARGET oledlg DESTINATION reactos/system32 FOR all)
