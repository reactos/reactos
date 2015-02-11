
list(APPEND SOURCE
    AttachThreadInput.c
    helper.c
    CreateIconFromResourceEx.c
    CreateWindowEx.c
    DeferWindowPos.c
    DestroyCursorIcon.c
    DrawIconEx.c
    desktop.c
    EnumDisplaySettings.c
    GetIconInfo.c
    GetKeyState.c
    GetPeekMessage.c
    GetSystemMetrics.c
    GetUserObjectInformation.c
    InitializeLpkHooks.c
    LoadImage.c
    LookupIconIdFromDirectoryEx.c
    RealGetWindowClass.c
    ScrollDC.c
    ScrollWindowEx.c
    SendMessageTimeout.c
    SetActiveWindow.c
    SetCursorPos.c
    SystemParametersInfo.c
    TrackMouseEvent.c
    WndProc.c
    wsprintf.c
    testlist.c
    user32_apitest.rc)

add_executable(user32_apitest ${SOURCE})
target_link_libraries(user32_apitest wine ${PSEH_LIB})
set_module_type(user32_apitest win32cui)
add_importlibs(user32_apitest gdi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET user32_apitest DESTINATION reactos/bin FOR all)
