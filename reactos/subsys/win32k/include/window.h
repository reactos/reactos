#ifndef __WIN32K_WINDOW_H
#define __WIN32K_WINDOW_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/winsta.h>
#include <include/dce.h>

typedef struct _PROPERTY
{
  LIST_ENTRY PropListEntry;
  HANDLE Data;
  ATOM Atom;
} PROPERTY, *PPROPERTY;

VOID
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
  /* Parent window handle. */
  HWND ParentHandle;
  /* Window menu handle. */
  HMENU Menu;
  /* Handle of the module that created the window. */
  HINSTANCE Instance;
  /* Unknown. */
  LPVOID Parameters;
  /* Entry in the thread's list of windows. */
  LIST_ENTRY ListEntry;
  /* Entry in the global list of windows. */
  LIST_ENTRY DesktopListEntry;
  /* Pointer to the extra data associated with the window. */
  PULONG ExtraData;
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
  /* FIXME: Don't know. */
  UINT IDMenu;
  /* Handle of region of the window to be updated. */
  HANDLE UpdateRegion;
  /* Pointer to the message queue associated with the window. */
  PUSER_MESSAGE_QUEUE MessageQueue;
  /* Head of the list of child windows. */
  LIST_ENTRY ChildrenListHead;
  /* Lock for the list of child windows. */
  FAST_MUTEX ChildrenListLock;
  /* Entry in the parent's list of child windows. */
  LIST_ENTRY SiblingListEntry;
  /* Entry in the list of thread windows. */
  LIST_ENTRY ThreadListEntry;
  /* Pointer to the parent window. */
  struct _WINDOW_OBJECT* Parent;
  /* DC Entries (DCE) */
  PDCE Dce;
  /* Property list head.*/
  LIST_ENTRY PropListHead;
  /* Scrollbar info */
  PSCROLLBARINFO pHScroll;
  PSCROLLBARINFO pVScroll;
  PSCROLLBARINFO wExtra;
} WINDOW_OBJECT, *PWINDOW_OBJECT;

/* Window flags. */
#define WINDOWOBJECT_NEED_SIZE            (0x00000001)
#define WINDOWOBJECT_NEED_BEGINPAINT      (0x00000002)
#define WINDOWOBJECT_NEED_ERASEBACKGRD    (0x00000004)
#define WINDOWOBJECT_NEED_NCPAINT         (0x00000008)
#define WINDOWOBJECT_NEED_INTERNALPAINT   (0x00000010)
#define WINDOWOBJECT_RESTOREMAX           (0x00000020)

NTSTATUS
InitWindowImpl(VOID);
NTSTATUS
CleanupWindowImpl(VOID);
VOID
W32kGetClientRect(PWINDOW_OBJECT WindowObject, PRECT Rect);
PWINDOW_OBJECT
W32kGetWindowObject(HWND hWnd);
VOID
W32kReleaseWindowObject(PWINDOW_OBJECT Window);
HWND STDCALL
W32kCreateDesktopWindow(PWINSTATION_OBJECT WindowStation,
			PWNDCLASS_OBJECT DesktopClass,
			ULONG Width, ULONG Height);
BOOL
W32kIsDesktopWindow(HWND hWnd);
HWND
W32kGetActiveWindow(VOID);
BOOL
W32kIsWindowVisible(HWND Wnd);
BOOL
W32kIsChildWindow(HWND Parent, HWND Child);
HWND W32kGetDesktopWindow();

#endif /* __WIN32K_WINDOW_H */

/* EOF */
