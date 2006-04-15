#ifndef _WIN32K_DESKTOP_H
#define _WIN32K_DESKTOP_H

#include "msgqueue.h"
#include "window.h"

typedef struct _DESKTOP_OBJECT
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ListEntry;

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

    HANDLE hDesktopHeap;
    PSECTION_OBJECT DesktopHeapSection;
    PDESKTOP DesktopInfo;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;

extern PDESKTOP_OBJECT InputDesktop;
extern HDESK InputDesktopHandle;
extern PWINDOWCLASS DesktopWindowClass;
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

PWINDOW_OBJECT FASTCALL 
UserGetDesktopWindow(VOID);

HWND FASTCALL
IntGetCurrentThreadDesktopWindow(VOID);

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID);

VOID FASTCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue);

PDESKTOP_OBJECT FASTCALL
IntGetActiveDesktop(VOID);

NTSTATUS FASTCALL
co_IntShowDesktop(PDESKTOP_OBJECT Desktop, ULONG Width, ULONG Height);

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP_OBJECT Desktop);

HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP_OBJECT DesktopObject);

BOOL IntSetThreadDesktop(IN PDESKTOP_OBJECT DesktopObject,
                         IN BOOL FreeOnFailure);

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

VOID co_IntShellHookNotify(WPARAM Message, LPARAM lParam);

#define IntIsActiveDesktop(Desktop) \
  ((Desktop)->WindowStation->ActiveDesktop == (Desktop))

static __inline PVOID
DesktopHeapAlloc(IN PDESKTOP Desktop,
                 IN SIZE_T Bytes)
{
    return RtlAllocateHeap(Desktop->hKernelHeap,
                           HEAP_NO_SERIALIZE,
                           Bytes);
}

static __inline BOOL
DesktopHeapFree(IN PDESKTOP Desktop,
                IN PVOID lpMem)
{
    return RtlFreeHeap(Desktop->hKernelHeap,
                       HEAP_NO_SERIALIZE,
                       lpMem);
}

static __inline PVOID
DesktopHeapReAlloc(IN PDESKTOP Desktop,
                   IN PVOID lpMem,
                   IN SIZE_T Bytes)
{
#if 0
    /* NOTE: ntoskrnl doesn't export RtlReAllocateHeap... */
    return RtlReAllocateHeap(Desktop->hKernelHeap,
                             HEAP_NO_SERIALIZE,
                             lpMem,
                             Bytes);
#else
    SIZE_T PrevSize;
    PVOID pNew;

    PrevSize = RtlSizeHeap(Desktop->hKernelHeap,
                           HEAP_NO_SERIALIZE,
                           lpMem);

    if (PrevSize == Bytes)
        return lpMem;

    pNew = RtlAllocateHeap(Desktop->hKernelHeap,
                           HEAP_NO_SERIALIZE,
                           Bytes);
    if (pNew != NULL)
    {
        if (PrevSize < Bytes)
            Bytes = PrevSize;

        RtlCopyMemory(pNew,
                      lpMem,
                      Bytes);

        RtlFreeHeap(Desktop->hKernelHeap,
                    HEAP_NO_SERIALIZE,
                    lpMem);
    }

    return pNew;
#endif
}

static __inline ULONG_PTR
DesktopHeapGetUserDelta(VOID)
{
    PW32HEAP_USER_MAPPING Mapping;
    HANDLE hDesktopHeap;
    ULONG_PTR Delta = 0;

    ASSERT(PsGetWin32Thread()->Desktop != NULL);
    hDesktopHeap = PsGetWin32Thread()->Desktop->hDesktopHeap;

    Mapping = PsGetWin32Process()->HeapMappings.Next;
    while (Mapping != NULL)
    {
        if (Mapping->UserMapping == (PVOID)hDesktopHeap)
        {
            Delta = (ULONG_PTR)Mapping->KernelMapping - (ULONG_PTR)Mapping->UserMapping;
            break;
        }

        Mapping = Mapping->Next;
    }

    return Delta;
}

static __inline PVOID
DesktopHeapAddressToUser(IN PDESKTOP Desktop,
                         PVOID lpMem)
{
    PW32HEAP_USER_MAPPING Mapping;

    Mapping = PsGetWin32Process()->HeapMappings.Next;
    while (Mapping != NULL)
    {
        if (Mapping->KernelMapping == (PVOID)Desktop->hKernelHeap)
        {
            return (PVOID)(((ULONG_PTR)lpMem - (ULONG_PTR)Desktop->hKernelHeap) +
                           (ULONG_PTR)Mapping->UserMapping);
        }

        Mapping = Mapping->Next;
    }

    return NULL;
}

#endif /* _WIN32K_DESKTOP_H */

/* EOF */
