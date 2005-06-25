#ifndef _WIN32K_DESKTOP_H
#define _WIN32K_DESKTOP_H

#include "msgqueue.h"
#include "window.h"

typedef struct _DESKTOP_OBJECT
{
    PVOID DesktopHeap; /* points to kmode memory! */

    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ListEntry;
    KSPIN_LOCK Lock;
    UNICODE_STRING Name;
    /* Pointer to the associated window station. */
    struct _WINSTATION_OBJECT *WindowStation;
    /* Pointer to the active queue. */
    PVOID ActiveMessageQueue;
    /* Rectangle of the work area */
    RECT WorkArea;
    /* Handle of the desktop window. */
    HANDLE DesktopWindow;
    HANDLE PrevActiveWindow;
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

NTSTATUS FASTCALL
InitDesktopImpl(VOID);

NTSTATUS FASTCALL
CleanupDesktopImpl(VOID);
                       
NTSTATUS STDCALL
IntDesktopObjectCreate(PVOID ObjectBody,
		       PVOID Parent,
		       PWSTR RemainingPath,
		       struct _OBJECT_ATTRIBUTES* ObjectAttributes);

VOID STDCALL
IntDesktopObjectDelete(PVOID DeletedObject);

VOID FASTCALL
IntGetDesktopWorkArea(PDESKTOP_OBJECT Desktop, PRECT Rect);

LRESULT CALLBACK
IntDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HDC FASTCALL
IntGetScreenDC(VOID);

HWND FASTCALL
IntGetDesktopWindow (VOID);

HWND FASTCALL
IntGetCurrentThreadDesktopWindow(VOID);

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID);

VOID FASTCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue);

PDESKTOP_OBJECT FASTCALL
IntGetActiveDesktop(VOID);

NTSTATUS FASTCALL
IntShowDesktop(PDESKTOP_OBJECT Desktop, ULONG Width, ULONG Height);

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP_OBJECT Desktop);

HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP_OBJECT DesktopObject);

NTSTATUS FASTCALL
IntValidateDesktopHandle(
   HDESK Desktop,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PDESKTOP_OBJECT *Object);

NTSTATUS FASTCALL
IntParseDesktopPath(PEPROCESS Process,
                    PUNICODE_STRING DesktopPath,
                    HWINSTA *hWinSta,
                    HDESK *hDesktop);

BOOL FASTCALL
IntDesktopUpdatePerUserSettings(BOOL bEnable);

BOOL IntRegisterShellHookWindow(HWND hWnd);
BOOL IntDeRegisterShellHookWindow(HWND hWnd);

VOID IntShellHookNotify(WPARAM Message, LPARAM lParam);

#define IntIsActiveDesktop(Desktop) \
  ((Desktop)->WindowStation->ActiveDesktop == (Desktop))

#endif /* _WIN32K_DESKTOP_H */

/* EOF */
