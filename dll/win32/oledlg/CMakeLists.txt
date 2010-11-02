
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(oledlg SHARED
    insobjdlg.c
    oledlg_main.c
    pastespl.c
    rsrc.rc
    oledlg.def)

if(NOT MSVC)
    set_source_files_properties(oledlg.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(oledlg win32dll)

target_link_libraries(oledlg wine)

add_importlibs(oledlg ole32 comdlg32 user32 advapi32 kernel32 ntdll)

add_cab_target(oledlg 1)
