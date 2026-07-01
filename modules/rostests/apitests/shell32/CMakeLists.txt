
spec2def(shell32_apitest.exe shell32_apitest.spec)

list(APPEND SOURCE
    AddCommas.cpp
    CFSFolder.cpp
    CheckEscapes.cpp
    CIDLData.cpp
    CMyComputer.cpp
    CommandLineToArgvW.cpp
    CShellDesktop.cpp
    CShellDispatch.cpp
    CShellLink.cpp
    #CUserNotification.cpp # Test is broken on Win 2003
    Control_RunDLLW.cpp
    DragDrop.cpp
    ExtractIconEx.cpp
    FindExecutable.cpp
    GetDisplayNameOf.cpp
    GUIDFromString.cpp
    LockServer.cpp
    Int64ToString.cpp
    IShellFolderViewCB.cpp
    ItemIDList.cpp
    #OpenAs_RunDLL.cpp # Test hangs on Win 2003
    PathIsEqualOrSubFolder.cpp
    PathIsSlow.cpp
    PathIsTemporary.cpp
    PathMakeUniqueName.cpp
    PathProcessCommand.cpp
    PathResolve.cpp
    RealShellExecuteEx.cpp
    SHAppBarMessage.cpp
    SHChangeNotify.cpp
    SHCreateDataObject.cpp
    SHCreateFileDataObject.cpp
    SHCreateFileExtractIconW.cpp
    SHEvaluateSystemCommandTemplate.cpp
    SHGetComputerDisplayNameW.cpp
    SHGetUnreadMailCountW.cpp
    SHIsBadInterfacePtr.cpp
    SHParseDisplayName.cpp
    SHRestricted.cpp
    SHShouldShowWizards.cpp
    SHEnumerateUnreadMailAccountsW.cpp
    She.cpp
    #ShellExec_RunDLL.cpp # Broke on Windows
    #ShellExecCmdLine.cpp # Broke on Windows
    #ShellExecuteEx.cpp # Broke on Windows
    #ShellExecuteW.cpp # Broke on Windows
    ShellHook.cpp
    ShellInfo.cpp
    ShellState.cpp
    SHGetAttributesFromDataObject.cpp
    SHGetUserDisplayName.cpp
    SHLimitInputEdit.cpp
    SHSetUnreadMailCountW.cpp
    StrRStr.cpp
    menu.cpp
    shelltest.cpp)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(shell32_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    resource.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_apitest.def)

set_target_properties(shell32_apitest
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

target_link_libraries(shell32_apitest wine uuid ${PSEH_LIB} cpprt atl_classes)
set_module_type(shell32_apitest win32cui)
target_compile_definitions(shell32_apitest PRIVATE UNICODE _UNICODE)
add_importlibs(shell32_apitest user32 gdi32 shell32 shlwapi ole32 oleaut32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_pch(shell32_apitest shelltest.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET shell32_apitest)

# shell32_apitest_sub.exe
add_executable(shell32_apitest_sub shell32_apitest_sub.cpp)
target_link_libraries(shell32_apitest_sub cpprt atl_classes)
set_module_type(shell32_apitest_sub win32gui UNICODE)
add_importlibs(shell32_apitest_sub msvcrt kernel32 user32 shell32 shlwapi ole32)
add_rostests_file(TARGET shell32_apitest_sub SUBDIR testdata)
add_dependencies(shell32_apitest shell32_apitest_sub)
