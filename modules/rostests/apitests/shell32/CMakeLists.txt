
spec2def(shell32_apitest.exe shell32_apitest.spec)

list(APPEND SOURCE
    AddCommas.cpp
    CFSFolder.cpp
    CheckEscapes.cpp
    CIDLData.cpp
    CMyComputer.cpp
    CShellDesktop.cpp
    CShellLink.cpp
    CUserNotification.cpp
    Control_RunDLLW.cpp
    DragDrop.cpp
    ExtractIconEx.cpp
    FindExecutable.cpp
    IShellFolderViewCB.cpp
    OpenAs_RunDLL.cpp
    PathResolve.cpp
    SHAppBarMessage.cpp
    SHChangeNotify.cpp
    SHCreateDataObject.cpp
    SHCreateFileDataObject.cpp
    SHCreateFileExtractIconW.cpp
    SHParseDisplayName.cpp
    ShellExecCmdLine.cpp
    ShellExecuteEx.cpp
    ShellExecuteW.cpp
    ShellHook.cpp
    ShellState.cpp
    SHGetAttributesFromDataObject.cpp
    SHLimitInputEdit.cpp
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
add_importlibs(shell32_apitest user32 gdi32 shell32 ole32 oleaut32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_pch(shell32_apitest shelltest.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET shell32_apitest)

# shell32_apitest_sub.exe
add_executable(shell32_apitest_sub shell32_apitest_sub.cpp)
target_link_libraries(shell32_apitest_sub cpprt atl_classes)
set_module_type(shell32_apitest_sub win32gui UNICODE)
add_importlibs(shell32_apitest_sub msvcrt kernel32 user32 shell32 shlwapi ole32)
add_rostests_file(TARGET shell32_apitest_sub SUBDIR testdata)
