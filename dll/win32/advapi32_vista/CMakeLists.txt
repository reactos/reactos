
add_definitions(-D_ADVAPI32_)
spec2def(advapi32_vista.dll advapi32_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    RegDeleteTree.c
    RegSetKeyValue.c
    RegLoadMUIString.c
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32_vista.def)

add_library(advapi32_vista MODULE ${SOURCE})
set_module_type(advapi32_vista win32dll ENTRYPOINT DllMain 12 TARGET_VERSION vista)
target_link_libraries(advapi32_vista wine)
add_importlibs(advapi32_vista advapi32 kernel32 ntdll)
add_dependencies(advapi32_vista psdk)
add_cd_file(TARGET advapi32_vista DESTINATION reactos/system32 FOR all)
