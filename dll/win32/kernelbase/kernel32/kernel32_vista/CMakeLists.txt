
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys ..)

list(APPEND SOURCE
    firmware.c
    EnumSystemLocalesEx.c
    GetLocaleInfoEx.c
    GetFileInformationByHandleEx.c
    GetFinalPathNameByHandle.c
    GetNamedPipeClientProcessId.c
    GetTickCount64.c
    GetUserDefaultLocaleName.c
    InitOnce.c
    IsValidLocaleName.c
    LCIDToLocaleName.c
    LocaleNameToLCID.c
    SetThreadDescription.c
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
