#ifndef _WIN32K_WINDOW_H
#define _WIN32K_WINDOW_H

struct _PROPERTY;
struct _WINDOW_OBJECT;
typedef struct _WINDOW_OBJECT *PWINDOW_OBJECT;

#include <include/object.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/winsta.h>
#include <include/dce.h>
#include <include/prop.h>
#include <include/scroll.h>


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
  /* NOTE: Do *NOT* Move this pointer anywhere in this structure! This
           is a pointer to the WINDOW structure that eventually replaces
           the WINDOW_OBJECT structure! USER32 expects this pointer to
           be here until WINDOW_OBJECT has completely been superseded! */
  PWINDOW Wnd;

  /* Pointer to the thread information */
  PW32THREADINFO ti;
  /* Pointer to the desktop */
  PDESKTOP Desktop;
  union
  {
    /* Pointer to a call procedure handle */
    PCALLPROC CallProc;
    /* Extra Wnd proc (windows of system classes) */
    WNDPROC WndProcExtra;
  };
  /* Indicates whether the window is derived from a system class */
  BOOL IsSystem;
  /* Context help id */
  DWORD ContextHelpId;
  /* system menu handle. */
  HMENU SystemMenu;
  /* Entry in the thread's list of windows. */
  LIST_ENTRY ListEntry;
  /* Handle for the window. */
  HWND hSelf;
  /* Window flags. */
  ULONG Flags;
  /* Handle of region of the window to be updated. */
  HANDLE UpdateRegion;
  /* Handle of the window region. */
  HANDLE WindowRegion;
  /* Pointer to the owning thread's message queue. */
  PUSER_MESSAGE_QUEUE MessageQueue;
  struct _WINDOW_OBJECT* FirstChild;
  struct _WINDOW_OBJECT* LastChild;
  struct _WINDOW_OBJECT* NextSibling;
  struct _WINDOW_OBJECT* PrevSibling;
  /* Entry in the list of thread windows. */
  LIST_ENTRY ThreadListEntry;
  /* Handle to the parent window. */
  struct _WINDOW_OBJECT* Parent;
  /* Handle to the owner window. */
  HWND hOwner;
  /* DC Entries (DCE) */
  PDCE Dce;
  /* Property list head.*/
  LIST_ENTRY PropListHead;
  ULONG PropListItems;
  /* Scrollbar info */
  PWINDOW_SCROLLINFO Scroll;
  BOOL Unicode;
  WNDPROC WndProc;
  PETHREAD OwnerThread;
  HWND hWndLastPopup; /* handle to last active popup window (wine doesn't use pointer, for unk. reason)*/
  PINTERNALPOS InternalPos;
  ULONG Status;
  /* counter for tiled child windows */
  ULONG TiledCounter;
  /* WNDOBJ list */
  LIST_ENTRY WndObjListHead;
} WINDOW_OBJECT; /* PWINDOW_OBJECT already declared at top of file */

/* Window flags. */
#define WINDOWOBJECT_NEED_SIZE            (0x00000001)
#define WINDOWOBJECT_NEED_ERASEBKGND      (0x00000002)
#define WINDOWOBJECT_NEED_NCPAINT         (0x00000004)
#define WINDOWOBJECT_NEED_INTERNALPAINT   (0x00000008)
#define WINDOWOBJECT_RESTOREMAX           (0x00000020)

#define WINDOWSTATUS_DESTROYING         (0x1)
#define WINDOWSTATUS_DESTROYED          (0x2)

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define IntIsDesktopWindow(WndObj) \
  (WndObj->Parent == NULL)

#define IntIsBroadcastHwnd(hWnd) \
  (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST)


#define IntWndBelongsToThread(WndObj, W32Thread) \
  (((WndObj->OwnerThread && WndObj->OwnerThread->Tcb.Win32Thread)) && \
   (WndObj->OwnerThread->Tcb.Win32Thread == W32Thread))

#define IntGetWndThreadId(WndObj) \
  WndObj->OwnerThread->Cid.UniqueThread

#define IntGetWndProcessId(WndObj) \
  WndObj->OwnerThread->ThreadsProcess->UniqueProcessId


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
IntIsWindowVisible (PWINDOW_OBJECT Window);

BOOL FASTCALL
IntIsChildWindow (PWINDOW_OBJECT Parent, PWINDOW_OBJECT Child);

VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd);

VOID FASTCALL
IntLinkWindow(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndParent, PWINDOW_OBJECT WndPrevSibling);

PWINDOW_OBJECT FASTCALL
IntGetAncestor(PWINDOW_OBJECT Wnd, UINT Type);

PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd);

PWINDOW_OBJECT FASTCALL
IntGetOwner(PWINDOW_OBJECT Wnd);


INT FASTCALL
IntGetWindowRgn(PWINDOW_OBJECT Window, HRGN hRgn);

INT FASTCALL
IntGetWindowRgnBox(PWINDOW_OBJECT Window, RECT *Rect);

BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT WindowObject, PWINDOWINFO pwi);

VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT WindowObject, UINT *cx, UINT *cy);

BOOL FASTCALL
IntAnyPopup(VOID);

BOOL FASTCALL
IntIsWindowInDestroy(PWINDOW_OBJECT Window);

BOOL FASTCALL
IntShowOwnedPopups( PWINDOW_OBJECT owner, BOOL fShow );

#endif /* _WIN32K_WINDOW_H */

/* EOF */
