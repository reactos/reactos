#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_InitializeLpkHooks(void);
extern void func_RealGetWindowClass(void);
extern void func_ScrollDC(void);
extern void func_ScrollWindowEx(void);
extern void func_GetSystemMetrics(void);
extern void func_GetIconInfo(void);
extern void func_GetPeekMessage(void);
extern void func_DeferWindowPos(void);
extern void func_GetKeyState(void);
extern void func_SetCursorPos(void);
extern void func_SetActiveWindow(void);
extern void func_SystemParametersInfo(void);
extern void func_TrackMouseEvent(void);
extern void func_WndProc(void);

const struct test winetest_testlist[] =
{
    { "InitializeLpkHooks", func_InitializeLpkHooks },
    { "RealGetWindowClass", func_RealGetWindowClass },
    { "ScrollDC", func_ScrollDC },
    { "ScrollWindowEx", func_ScrollWindowEx },
    { "GetSystemMetrics", func_GetSystemMetrics },
    { "GetIconInfo", func_GetIconInfo },
    { "GetPeekMessage", func_GetPeekMessage },
    { "DeferWindowPos", func_DeferWindowPos },
    { "GetKeyState", func_GetKeyState },
    { "SetCursorPos", func_SetCursorPos },
    { "SetActiveWindow", func_SetActiveWindow },
    { "SystemParametersInfo", func_SystemParametersInfo },
    { "TrackMouseEvent", func_TrackMouseEvent },
    { "WndProc", func_WndProc },
    { 0, 0 }
};

