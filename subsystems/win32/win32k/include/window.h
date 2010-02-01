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

extern ATOM AtomMessage;
extern ATOM AtomWndObj; /* WNDOBJ list */

BOOL FASTCALL UserUpdateUiState(PWND Wnd, WPARAM wParam);

typedef struct _WINDOW_OBJECT
{
  THRDESKHEAD head;
  PWND Wnd;

  /* Pointer to the thread information */
  PTHREADINFO pti; // Use Wnd->head.pti
  /* system menu handle. */
  HMENU SystemMenu;
  /* Handle for the window. */
  HWND hSelf; // Use Wnd->head.h
  /* Window flags. */
  ULONG state;
  /* Handle of region of the window to be updated. */
  HANDLE hrgnUpdate;
  /* Handle of the window region. */
  HANDLE hrgnClip;
  struct _WINDOW_OBJECT* spwndChild;
  struct _WINDOW_OBJECT* spwndNext;
  struct _WINDOW_OBJECT* spwndPrev;
  /* Handle to the parent window. */
  struct _WINDOW_OBJECT* spwndParent;
  /* Handle to the owner window. */
  HWND hOwner; // Use spwndOwner


  /* Scrollbar info */
  PSBINFOEX pSBInfo; // convert to PSBINFO
  /* Entry in the list of thread windows. */
  LIST_ENTRY ThreadListEntry;
} WINDOW_OBJECT; /* PWINDOW_OBJECT already declared at top of file */

/* Window flags. */
#define WINDOWOBJECT_NEED_SIZE            WNDS_SENDSIZEMOVEMSGS
#define WINDOWOBJECT_NEED_ERASEBKGND      WNDS_ERASEBACKGROUND
#define WINDOWOBJECT_NEED_NCPAINT         WNDS_SENDNCPAINT
#define WINDOWOBJECT_RESTOREMAX           (0x00000020) // Set/Clr WS_MAXIMIZE && Clr/Set WS_EX2_VERTICALLYMAXIMIZEDLEFT/RIGHT

#define WINDOWSTATUS_DESTROYING         WNDS2_INDESTROY
#define WINDOWSTATUS_DESTROYED          WNDS_DESTROYED

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define IntIsDesktopWindow(WndObj) \
  (WndObj->spwndParent == NULL)

#define IntIsBroadcastHwnd(hWnd) \
  (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST)


#define IntWndBelongsToThread(WndObj, W32Thread) \
  (((WndObj->pti->pEThread && WndObj->pti->pEThread->Tcb.Win32Thread)) && \
   (WndObj->pti->pEThread->Tcb.Win32Thread == W32Thread))
//  ((WndObj->head.pti) && (WndObj->head.pti == W32Thread))

#define IntGetWndThreadId(WndObj) \
  WndObj->pti->pEThread->Cid.UniqueThread
//  WndObj->head.pti->pEThread->Cid.UniqueThread

#define IntGetWndProcessId(WndObj) \
  WndObj->pti->pEThread->ThreadsProcess->UniqueProcessId
//  WndObj->head.pti->pEThread->ThreadsProcess->UniqueProcessId

BOOL FASTCALL
IntIsWindow(HWND hWnd);

HWND* FASTCALL
IntWinListChildren(PWINDOW_OBJECT Window);

NTSTATUS FASTCALL
InitWindowImpl (VOID);

NTSTATUS FASTCALL
CleanupWindowImpl (VOID);

VOID FASTCALL
IntGetClientRect (PWINDOW_OBJECT WindowObject, RECTL *Rect);

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
IntGetWindowRgnBox(PWINDOW_OBJECT Window, RECTL *Rect);

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

LRESULT FASTCALL
IntDefWindowProc( PWINDOW_OBJECT Window, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL Ansi);

VOID FASTCALL IntNotifyWinEvent(DWORD, PWND, LONG, LONG);

PWND APIENTRY co_IntCreateWindowEx(DWORD,PUNICODE_STRING,PUNICODE_STRING,DWORD,LONG,LONG,LONG,LONG,HWND,HMENU,HINSTANCE,LPVOID,DWORD,BOOL);
WNDPROC FASTCALL IntGetWindowProc(PWND,BOOL);

#endif /* _WIN32K_WINDOW_H */

/* EOF */
