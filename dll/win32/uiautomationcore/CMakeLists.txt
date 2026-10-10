
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(uiautomationcore.dll uiautomationcore.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    reactos.c
    uia_client.c
    uia_com_client.c
    uia_event.c
    uia_ids.c
    uia_main.c
    uia_provider.c
    uia_utils.c
    ${CMAKE_CURRENT_BINARY_DIR}/uiautomationcore_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uiautomationcore.def)

add_idl_headers(uiautomationcore_idlheader uia_classes_client.idl uia_classes_core.idl uia_classes.idl)
add_library(uiautomationcore SHARED ${SOURCE})
set_module_type(uiautomationcore win32dll ENTRYPOINT 0)
add_idl_reg_scripts(uiautomationcore regtypelib uia_classes_client.idl uia_classes_core.idl uia_classes.idl)
target_link_libraries(uiautomationcore wine wine_dll_register uuid)
add_dependencies(uiautomationcore psdk uiautomationcore_idlheader)
add_importlibs(uiautomationcore shlwapi oleaut32 ole32 oleacc combase user32 kernel32_vista ucrtbase kernel32 ntdll)
add_cd_file(TARGET uiautomationcore DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(uiautomationcore)
