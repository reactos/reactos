#ifndef __WIN32K_WINDOW_H
#define __WIN32K_WINDOW_H

struct _PROPERTY;
struct _WINDOW_OBJECT;
typedef struct _WINDOW_OBJECT *PWINDOW_OBJECT;

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/winsta.h>
#include <include/dce.h>
#include <include/prop.h>

typedef struct _PROPERTY
{
  LIST_ENTRY PropListEntry;
  HANDLE Data;
  ATOM Atom;
} PROPERTY, *PPROPERTY;

VOID FASTCALL
WinPosSetupInternalPos(VOID);

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
  /* Initial window position. */
  INT x;
  INT y;
  INT Width;
  INT Height;
  /* Context help id */
  DWORD ContextHelpId;
  /* system menu handle. */
  HMENU SystemMenu;
  /* Handle of the module that created the window. */
  HINSTANCE Instance;
  /* Unknown. */
  LPVOID Parameters;
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
  /* Pointer to the owning thread's message queue. */
  PUSER_MESSAGE_QUEUE MessageQueue;
  struct _WINDOW_OBJECT* FirstChild;
  struct _WINDOW_OBJECT* LastChild;
  /* Lock for the list of child windows. */
  FAST_MUTEX ChildrenListLock;
  struct _WINDOW_OBJECT* NextSibling;
  struct _WINDOW_OBJECT* PrevSibling;
  /* Entry in the list of thread windows. */
  LIST_ENTRY ThreadListEntry;
  /* Pointer to the parent window. */
  struct _WINDOW_OBJECT* Parent;
  /* Pointer to the owner window. */
  struct _WINDOW_OBJECT* Owner;
  /* DC Entries (DCE) */
  PDCE Dce;
  /* Property list head.*/
  LIST_ENTRY PropListHead;
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
} WINDOW_OBJECT; /* PWINDOW_OBJECT already declared at top of file */

/* Window flags. */
#define WINDOWOBJECT_NEED_SIZE            (0x00000001)
/* Not used anymore: define WINDOWOBJECT_NEED_BEGINPAINT      (0x00000002) */
#define WINDOWOBJECT_NEED_ERASEBACKGRD    (0x00000004)
#define WINDOWOBJECT_NEED_NCPAINT         (0x00000008)
#define WINDOWOBJECT_NEED_INTERNALPAINT   (0x00000010)
#define WINDOWOBJECT_RESTOREMAX           (0x00000020)

inline BOOL IntIsDesktopWindow(PWINDOW_OBJECT WindowObject);

inline BOOL IntIsBroadcastHwnd( HWND hwnd );

BOOLEAN FASTCALL
IntWndBelongsToThread(PWINDOW_OBJECT Window, PW32THREAD ThreadData);

NTSTATUS FASTCALL
InitWindowImpl (VOID);

NTSTATUS FASTCALL
CleanupWindowImpl (VOID);

VOID FASTCALL
IntGetClientRect (PWINDOW_OBJECT WindowObject, PRECT Rect);

PWINDOW_OBJECT FASTCALL
IntGetWindowObject (HWND hWnd);

VOID FASTCALL
IntReleaseWindowObject (PWINDOW_OBJECT Window);

HWND STDCALL
IntCreateDesktopWindow (PWINSTATION_OBJECT WindowStation,
			PWNDCLASS_OBJECT DesktopClass,
			ULONG Width, ULONG Height);

HWND FASTCALL
IntGetActiveWindow (VOID);

BOOL FASTCALL
IntIsWindowVisible (HWND Wnd);

BOOL FASTCALL
IntIsChildWindow (HWND Parent, HWND Child);

HWND FASTCALL
IntGetDesktopWindow (VOID);

HWND FASTCALL
IntGetFocusWindow (VOID);

HWND FASTCALL
IntSetFocusWindow (HWND hWnd);

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data);

PPROPERTY FASTCALL
IntGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom);

DWORD FASTCALL
IntGetWindowThreadProcessId(PWINDOW_OBJECT Wnd, PDWORD pid);

VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd);

VOID FASTCALL
IntLinkWindow(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndParent, PWINDOW_OBJECT WndPrevSibling);

ULONG
UserHasDlgFrameStyle(ULONG Style, ULONG ExStyle);

ULONG
UserHasThickFrameStyle(ULONG Style, ULONG ExStyle);

PWINDOW_OBJECT FASTCALL
IntGetAncestor(PWINDOW_OBJECT Wnd, UINT Type);

PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd);

typedef enum _WINLOCK_TYPE
{
  None,
  Any,
  Shared,
  Exclusive
} WINLOCK_TYPE; 

#define ASSERT_WINLOCK(a) assert(IntVerifyWinLock(a))

inline VOID IntAcquireWinLockShared();
inline VOID IntAcquireWinLockExclusive();
inline VOID IntReleaseWinLock();
BOOL FASTCALL IntVerifyWinLock(WINLOCK_TYPE Type);
WINLOCK_TYPE FASTCALL IntSuspendWinLock();
VOID FASTCALL IntRestoreWinLock(WINLOCK_TYPE Type);
inline BOOL IntInitializeWinLock();
inline VOID IntDeleteWinLock();

#endif /* __WIN32K_WINDOW_H */

/* EOF */
