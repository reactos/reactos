
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(kernel32_vista.dll kernel32_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    GetFileInformationByHandleEx.c
    GetTickCount64.c
    InitOnceExecuteOnce.c
    sync.c
    vista.c)

add_library(kernel32_vista_static ${SOURCE})
add_dependencies(kernel32_vista_static psdk)
add_library(kernel32_vista MODULE DllMain.c ${CMAKE_CURRENT_BINARY_DIR}/kernel32_vista.def)
set_module_type(kernel32_vista win32dll ENTRYPOINT DllMain 12)
target_link_libraries(kernel32_vista kernel32_vista_static kernel32_shared)
add_importlibs(kernel32_vista kernel32 ntdll_vista ntdll)
add_delay_importlibs(kernel32_vista ntdll_vista)
add_dependencies(kernel32_vista psdk)
add_cd_file(TARGET kernel32_vista DESTINATION reactos/system32 FOR all)
