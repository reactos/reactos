
list(APPEND SOURCE
    AttachThreadInput.c
    ../include/msgtrace.c
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
    GetPeekMessage.c
    GetSystemMetrics.c
    GetUserObjectInformation.c
    GetWindowPlacement.c
    InitializeLpkHooks.c
    LoadImage.c
    LookupIconIdFromDirectoryEx.c
    NextDlgItem.c
    RealGetWindowClass.c
    RegisterClassEx.c
    RegisterHotKey.c
    ScrollDC.c
    ScrollWindowEx.c
    SendMessageTimeout.c
    SetActiveWindow.c
    SetCursorPos.c
    SetParent.c
    SetProp.c
    SetScrollInfo.c
    SetScrollRange.c
    SystemParametersInfo.c
    TrackMouseEvent.c
    WndProc.c
    precomp.h)

add_executable(user32_apitest
    ${SOURCE}
    testlist.c
    wsprintf.c
    user32_apitest.rc)

target_link_libraries(user32_apitest wine ${PSEH_LIB})
set_module_type(user32_apitest win32cui)
add_importlibs(user32_apitest advapi32 gdi32 user32 msvcrt kernel32 ntdll)
add_pch(user32_apitest precomp.h SOURCE)
add_rostests_file(TARGET user32_apitest)

add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/verclasstest1.manifest")
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/verclasstest2.manifest")
