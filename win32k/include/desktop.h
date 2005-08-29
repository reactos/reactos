#ifndef _WIN32K_DESKTOP_H
#define _WIN32K_DESKTOP_H

#include "msgqueue.h"
#include "window.h"

typedef struct _DESKTOP_OBJECT
{
    PVOID DesktopHeap; /* points to kmode memory! */

    CSHORT Type;
    CSHORT Size;
    /* entry in WinSta's list of desktops */
    LIST_ENTRY ListEntry;
    KSPIN_LOCK Lock;
    
//FIXME: add desktop heap ptr/struct
    
    UNICODE_STRING Name;
    /* Pointer to the associated window station. */
    struct _WINSTATION_OBJECT *WinSta;
    /* Pointer to the active queue. */
    PUSER_MESSAGE_QUEUE ActiveQueue;
    /* Rectangle of the work area */
    RECT WorkArea;
    /* Handle of the desktop window. */
    HWND DesktopWindow;
    HWND PrevActiveWindow;
    /* Thread blocking input */
    PVOID BlockInputThread;

    LIST_ENTRY ShellHookWindows;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;

extern PDESKTOP_OBJECT InputDesktop;
extern HDESK InputDesktopHandle;
extern PWNDCLASS_OBJECT DesktopWindowClass;
extern HDC ScreenDeviceContext;
extern BOOL g_PaintDesktopVersion;

typedef struct _SHELL_HOOK_WINDOW
{
  LIST_ENTRY ListEntry;
  HWND hWnd;
} SHELL_HOOK_WINDOW, *PSHELL_HOOK_WINDOW;


#endif /* _WIN32K_DESKTOP_H */

/* EOF */
