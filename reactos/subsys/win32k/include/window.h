#ifndef __WIN32K_WINDOW_H
#define __WIN32K_WINDOW_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>

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
  struct _WINDOW_OBJECT* Parent;
  /* Window menu handle. */
  HMENU Menu;
  /* Handle of the module that created the window. */
  HINSTANCE Instance;
  /* Unknown. */
  LPVOID Parameters;
  /* Entry in the thread's list of windows. */
  LIST_ENTRY ListEntry;
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
  UINT IDMenu;
} WINDOW_OBJECT, *PWINDOW_OBJECT;

#define WINDOWOBJECT_NEED_SIZE            (0x00000001)

NTSTATUS
InitWindowImpl(VOID);

NTSTATUS
CleanupWindowImpl(VOID);

#endif /* __WIN32K_WINDOW_H */

/* EOF */
