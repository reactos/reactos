/*
 * internal executive prototypes
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

#define NTOS_MODE_KERNEL
#include <ntos.h>

/* one systemcursor for custom cursors */
#define SYSCURSORCOUNT (14 + 1)

typedef struct _CURSORCLIP_INFO
{
  BOOL IsClipped;
  UINT Left;
  UINT Top;
  UINT Right;
  UINT Bottom;
} CURSORCLIP_INFO, *PCURSORCLIP_INFO;

typedef struct _SYSCURSOR
{
  HANDLE hCursor;
  LONG cx, cy;
  LONG hx, hy;
  PVOID AndImage;
  PVOID XorImage;
} SYSCURSOR, *PSYSCURSOR;

typedef struct _SYSTEM_CURSORINFO
{
  BOOL Enabled;
  BOOL SwapButtons;
  UINT CurrentCursor;
  LONG x, y;
  BOOL SafetySwitch, SafetySwitch2;
  FAST_MUTEX CursorMutex;
  CURSORCLIP_INFO CursorClipInfo;
  SYSCURSOR SystemCursors[SYSCURSORCOUNT];
  UINT DblClickSpeed;
  UINT DblClickWidth;
  UINT DblClickHeight;
  DWORD LastBtnDown;
  LONG LastBtnDownX;
  LONG LastBtnDownY;
} SYSTEM_CURSORINFO, *PSYSTEM_CURSORINFO;

typedef struct _WINSTATION_OBJECT
{   
  CSHORT Type;
  CSHORT Size;

  KSPIN_LOCK Lock;
  UNICODE_STRING Name;
  LIST_ENTRY DesktopListHead;
  PRTL_ATOM_TABLE AtomTable;
  PVOID HandleTable;
  HANDLE SystemMenuTemplate;
  SYSTEM_CURSORINFO SystemCursor;
  struct _DESKTOP_OBJECT* ActiveDesktop;
  /* FIXME: Clipboard */
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

typedef struct _DESKTOP_OBJECT
{   
  CSHORT Type;
  CSHORT Size;
  LIST_ENTRY ListEntry;
  KSPIN_LOCK Lock;
  UNICODE_STRING Name;
  /* Pointer to the associated window station. */
  struct _WINSTATION_OBJECT *WindowStation;
  /* Pointer to the active queue. */
  PVOID ActiveMessageQueue;
  /* Handle of the desktop window. */
  HANDLE DesktopWindow;
  HANDLE PrevActiveWindow;
  struct _WINDOW_OBJECT* CaptureWindow;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;


typedef VOID (*PLOOKASIDE_MINMAX_ROUTINE)(
  POOL_TYPE PoolType,
  ULONG Size,
  PUSHORT MinimumDepth,
  PUSHORT MaximumDepth);

/* GLOBAL VARIABLES *********************************************************/

TIME_ZONE_INFORMATION SystemTimeZoneInfo;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID
ExpWin32kInit(VOID);

VOID 
ExInit (VOID);
VOID 
ExInitTimeZoneInfo (VOID);
VOID 
ExInitializeWorkerThreads(VOID);
VOID
ExpInitLookasideLists(VOID);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */
