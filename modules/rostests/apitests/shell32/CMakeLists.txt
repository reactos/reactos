
add_definitions(
    -D_ATL_NO_EXCEPTIONS)

set_cpp(WITH_RUNTIME)

spec2def(shell32_apitest.exe shell32_apitest.spec)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    AddCommas.cpp
    CFSFolder.cpp
    CMyComputer.cpp
    CShellDesktop.cpp
    CShellLink.cpp
    CUserNotification.cpp
    Control_RunDLLW.cpp
    IShellFolderViewCB.cpp
    OpenAs_RunDLL.cpp
    PathResolve.cpp
    SHCreateFileExtractIconW.cpp
    SHParseDisplayName.cpp
    ShellExecCmdLine.cpp
    ShellExecuteEx.cpp
    ShellState.cpp
    menu.cpp
    shelltest.cpp
    shelltest.h)

add_executable(shell32_apitest
    ${SOURCE}
    testlist.c
    resource.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_apitest.def)

target_link_libraries(shell32_apitest wine uuid ${PSEH_LIB})
set_module_type(shell32_apitest win32cui)
add_importlibs(shell32_apitest user32 gdi32 shell32 ole32 oleaut32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_pch(shell32_apitest shelltest.h SOURCE)
add_rostests_file(TARGET shell32_apitest)
