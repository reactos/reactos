
spec2def(compstui.dll compstui.spec)

list(APPEND SOURCE
    compstui_main.c
    compstui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/compstui.def)

add_library(compstui MODULE ${SOURCE})
set_module_type(compstui win32dll ENTRYPOINT 0)
target_link_libraries(compstui wine)
add_importlibs(compstui comctl32 user32 ucrtbase kernel32 ntdll)
add_cd_file(TARGET compstui DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(compstui) # CORE-5743: No ARRAY_SIZE macro
