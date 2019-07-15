
list(APPEND SOURCE
    AttachThreadInput.c
    ../include/msgtrace.c
    CloseWindow.c
    CreateDialog.c
    CreateIconFromResourceEx.c
    CreateWindowEx.c
    DeferWindowPos.c
    DestroyCursorIcon.c
    DrawIconEx.c
    desktop.c
    EmptyClipboard.c
    EnumDisplaySettings.c
    GetDCEx.c
    GetIconInfo.c
    GetKeyState.c
    GetMessageTime.c
    GetPeekMessage.c
    GetSystemMetrics.c
    GetUserObjectInformation.c
    GetWindowPlacement.c
    InitializeLpkHooks.c
    LoadImage.c
    LookupIconIdFromDirectoryEx.c
    MessageStateAnalyzer.c
    NextDlgItem.c
    PrivateExtractIcons.c
    RealGetWindowClass.c
    RedrawWindow.c
    RegisterClassEx.c
    RegisterHotKey.c
    ScrollBarWndExtra.c
    ScrollDC.c
    ScrollWindowEx.c
    SendMessageTimeout.c
    SetActiveWindow.c
    SetCursorPos.c
    SetParent.c
    SetProp.c
    SetScrollInfo.c
    SetScrollRange.c
    SwitchToThisWindow.c
    SystemParametersInfo.c
    TrackMouseEvent.c
    WndProc.c
    wsprintf.c
    precomp.h)

add_executable(user32_apitest ${SOURCE} testlist.c user32_apitest.rc)
target_link_libraries(user32_apitest wine ${PSEH_LIB})
set_module_type(user32_apitest win32cui)
add_importlibs(user32_apitest advapi32 gdi32 user32 msvcrt kernel32 ntdll)
add_pch(user32_apitest precomp.h SOURCE)
add_rostests_file(TARGET user32_apitest)

add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/verclasstest1.manifest")
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/verclasstest2.manifest")
