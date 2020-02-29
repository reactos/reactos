
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_ADVAPI32_)
spec2def(advapi32_vista_addon.dll advapi32_vista_addon.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    RegDeleteTree.c
    RegSetKeyValue.c
    RegLoadMUIString.c
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32_vista_addon.def)

add_library(advapi32_vista_static ${SOURCE})
add_library(advapi32_vista_addon MODULE DllMain.c)
set_module_type(advapi32_vista_addon win32dll ENTRYPOINT DllMain 12)
target_link_libraries(advapi32_vista_addon advapi32_vista_static wine)
add_importlibs(advapi32_vista_addon advapi32 kernel32 ntdll)
add_dependencies(advapi32_vista_addon psdk)
