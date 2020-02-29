
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_KERNEL32_)
spec2def(kernel32_vista_addon.dll kernel32_vista_addon.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    GetFileInformationByHandleEx.c
    GetTickCount64.c
    InitOnceExecuteOnce.c
    sync.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32_vista_addon.def)

add_library(kernel32_vista_static ${SOURCE})
add_library(kernel32_vista_addon MODULE DllMain.c)
set_module_type(kernel32_vista_addon win32dll ENTRYPOINT DllMain 12)
target_link_libraries(kernel32_vista_addon kernel32_vista_static)
add_importlibs(kernel32_vista_addon kernel32 ntdll)
add_delay_importlibs(kernel32_vista_addon ntdll_vista)
add_dependencies(kernel32_vista_addon psdk)
