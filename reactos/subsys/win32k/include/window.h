#ifndef _WIN32K_WINDOW_H
#define _WIN32K_WINDOW_H

struct _PROPERTY;
struct _WINDOW_OBJECT;
typedef struct _WINDOW_OBJECT *PWINDOW_OBJECT;

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/object.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/winsta.h>
#include <include/dce.h>
#include <include/prop.h>


VOID FASTCALL
WinPosSetupInternalPos(VOID);

typedef struct _INTERNALPOS
{
  RECT NormalRect;
  POINT IconPos;
  POINT MaxPos;
} INTERNALPOS, *PINTERNALPOS;

typedef struct _WINDOW_OBJECT
{
  /* Pointer to the window class. */
  PWNDCLASS_OBJECT Class;
  /* Extended style. */
  DWORD ExStyle;
  /* Window name. */
  UNICODE_STRING WindowName;
  /* Style. */
  DWORD Style;
  /* Context help id */
  DWORD ContextHelpId;
  /* system menu handle. */
  HMENU SystemMenu;
  /* Handle of the module that created the window. */
  HINSTANCE Instance;
  /* Entry in the thread's list of windows. */
  LIST_ENTRY ListEntry;
  /* Pointer to the extra data associated with the window. */
  PCHAR ExtraData;
  /* Size of the extra data associated with the window. */
  ULONG ExtraDataSize;
  /* Position of the window. */
  RECT WindowRect;
  /* Position of the window's client area. */
  RECT ClientRect;
  /* Handle for the window. */
  HANDLE Self;
  /* Window flags. */
  ULONG Flags;
  /* Window menu handle or window id */
  UINT IDMenu;
  /* Handle of region of the window to be updated. */
  HANDLE UpdateRegion;
  HANDLE NCUpdateRegion;
  /* Lock to be held when manipulating (NC)UpdateRegion */
  FAST_MUTEX UpdateLock;
  /* Pointer to the owning thread's message queue. */
  PUSER_MESSAGE_QUEUE MessageQueue;
  /* Lock for the list of child windows. */
  FAST_MUTEX RelativesLock;
  struct _WINDOW_OBJECT* FirstChild;
  struct _WINDOW_OBJECT* LastChild;
  struct _WINDOW_OBJECT* NextSibling;
  struct _WINDOW_OBJECT* PrevSibling;
  /* Entry in the list of thread windows. */
  LIST_ENTRY ThreadListEntry;
  /* Pointer to the parent window. */
  HANDLE Parent;
  /* Pointer to the owner window. */
  HANDLE Owner;
  /* DC Entries (DCE) */
  PDCE Dce;
  /* Property list head.*/
  LIST_ENTRY PropListHead;
  FAST_MUTEX PropListLock;
  ULONG PropListItems;
  /* Scrollbar info */
  PSCROLLBARINFO pHScroll;
  PSCROLLBARINFO pVScroll;
  PSCROLLBARINFO wExtra;
  LONG UserData;
  BOOL Unicode;
  WNDPROC WndProcA;
  WNDPROC WndProcW;
  PETHREAD OwnerThread;
  HWND hWndLastPopup; /* handle to last active popup window (wine doesn't use pointer, for unk. reason)*/
  PINTERNALPOS InternalPos;
} WINDOW_OBJECT; /* PWINDOW_OBJECT already declared at top of file */

/* Window flags. */
#define WINDOWOBJECT_NEED_SIZE            (0x00000001)
#define WINDOWOBJECT_NEED_ERASEBKGND      (0x00000002)
#define WINDOWOBJECT_NEED_NCPAINT         (0x00000004)
#define WINDOWOBJECT_NEED_INTERNALPAINT   (0x00000008)
#define WINDOWOBJECT_RESTOREMAX           (0x00000020)

#define IntIsDesktopWindow(WndObj) \
  (WndObj->Parent == NULL)

#define IntIsBroadcastHwnd(hWnd) \
  (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST)

#define IntGetWindowObject(hWnd) \
  IntGetProcessWindowObject(PsGetWin32Process(), hWnd)

#define IntReferenceWindowObject(WndObj) \
  ObmReferenceObjectByPointer(WndObj, otWindow)

#define IntReleaseWindowObject(WndObj) \
  ObmDereferenceObject(WndObj)

#define IntWndBelongsToThread(WndObj, W32Thread) \
  (((WndObj->OwnerThread && WndObj->OwnerThread->Win32Thread)) && \
   (WndObj->OwnerThread->Win32Thread == W32Thread))

#define IntGetWndThreadId(WndObj) \
  WndObj->OwnerThread->Cid.UniqueThread

#define IntGetWndProcessId(WndObj) \
  WndObj->OwnerThread->ThreadsProcess->UniqueProcessId

#define IntLockRelatives(WndObj) \
  ExAcquireFastMutexUnsafe(&WndObj->RelativesLock)

#define IntUnLockRelatives(WndObj) \
  ExReleaseFastMutexUnsafe(&WndObj->RelativesLock)


PWINDOW_OBJECT FASTCALL
IntGetProcessWindowObject(PW32PROCESS ProcessData, HWND hWnd);

BOOL FASTCALL
IntIsWindow(HWND hWnd);

HWND* FASTCALL
IntWinListChildren(PWINDOW_OBJECT Window);

NTSTATUS FASTCALL
InitWindowImpl (VOID);

NTSTATUS FASTCALL
CleanupWindowImpl (VOID);

VOID FASTCALL
IntGetClientRect (PWINDOW_OBJECT WindowObject, PRECT Rect);

HWND FASTCALL
IntGetActiveWindow (VOID);

BOOL FASTCALL
IntIsWindowVisible (HWND hWnd);

BOOL FASTCALL
IntIsChildWindow (HWND Parent, HWND Child);

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data);

PPROPERTY FASTCALL
IntGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom);

VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd);

VOID FASTCALL
IntLinkWindow(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndParent, PWINDOW_OBJECT WndPrevSibling);

PWINDOW_OBJECT FASTCALL
IntGetAncestor(PWINDOW_OBJECT Wnd, UINT Type);

PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd);

PWINDOW_OBJECT FASTCALL
IntGetParentObject(PWINDOW_OBJECT Wnd);

DWORD IntRemoveWndProcHandle(WNDPROC Handle);
DWORD IntRemoveProcessWndProcHandles(HANDLE ProcessID);
DWORD IntAddWndProcHandle(WNDPROC WindowProc, BOOL IsUnicode);

#endif /* _WIN32K_WINDOW_H */

/* EOF */
