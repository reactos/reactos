#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_DeferWindowPos(void);
extern void func_desktop(void);
extern void func_GetIconInfo(void);
extern void func_GetKeyState(void);
extern void func_GetPeekMessage(void);
extern void func_GetSystemMetrics(void);
extern void func_InitializeLpkHooks(void);
extern void func_RealGetWindowClass(void);
extern void func_ScrollDC(void);
extern void func_ScrollWindowEx(void);
extern void func_SetActiveWindow(void);
extern void func_SetCursorPos(void);
extern void func_SystemParametersInfo(void);
extern void func_TrackMouseEvent(void);
extern void func_WndProc(void);
extern void func_wsprintf(void);

const struct test winetest_testlist[] =
{
    { "desktop", func_desktop },
    { "DeferWindowPos", func_DeferWindowPos },
    { "GetIconInfo", func_GetIconInfo },
    { "GetKeyState", func_GetKeyState },
    { "GetPeekMessage", func_GetPeekMessage },
    { "GetSystemMetrics", func_GetSystemMetrics },
    { "InitializeLpkHooks", func_InitializeLpkHooks },
    { "RealGetWindowClass", func_RealGetWindowClass },
    { "ScrollDC", func_ScrollDC },
    { "ScrollWindowEx", func_ScrollWindowEx },
    { "SetActiveWindow", func_SetActiveWindow },
    { "SetCursorPos", func_SetCursorPos },
    { "SystemParametersInfo", func_SystemParametersInfo },
    { "TrackMouseEvent", func_TrackMouseEvent },
    { "WndProc", func_WndProc },
    { "wsprintf", func_wsprintf },
    { 0, 0 }
};

