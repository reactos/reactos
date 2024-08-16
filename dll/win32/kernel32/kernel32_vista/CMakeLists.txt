
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys ..)
spec2def(kernel32_vista.dll kernel32_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    firmware.c
    GetFileInformationByHandleEx.c
    GetTickCount64.c
    InitOnce.c
    sync.c
    vista.c)

# These functions are not exported from kernel32_vista (yet).
# They are only used when NT6 exports in kernel32 are enabled.
if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    list(APPEND SOURCE
        ../client/console/vista.c)
endif()

add_library(kernel32_vista_static ${SOURCE})
target_link_libraries(kernel32_vista_static ${PSEH_LIB})
add_dependencies(kernel32_vista_static psdk)

add_library(kernel32_vista MODULE
    DllMain.c
    kernel32_vista.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32_vista.def)
set_module_type(kernel32_vista win32dll ENTRYPOINT DllMain 12)
target_link_libraries(kernel32_vista kernel32_vista_static kernel32_shared)
add_importlibs(kernel32_vista kernel32 ntdll_vista ntdll)
add_delay_importlibs(kernel32_vista ntdll_vista)
add_dependencies(kernel32_vista psdk)
add_cd_file(TARGET kernel32_vista DESTINATION reactos/system32 FOR all)
