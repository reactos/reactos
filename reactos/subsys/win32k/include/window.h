#ifndef __WIN32K_WINDOW_H
#define __WIN32K_WINDOW_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>

typedef struct _WINDOW_OBJECT
{
  PWNDCLASS_OBJECT Class;
  DWORD ExStyle;
  UNICODE_STRING WindowName;
  DWORD Style;
  int x;
  int y;
  int Width;
  int Height;
  HWND Parent;
  HMENU Menu;
  HINSTANCE Instance;
  LPVOID Parameters;
  LIST_ENTRY ListEntry;
} WINDOW_OBJECT, *PWINDOW_OBJECT;


NTSTATUS
InitWindowImpl(VOID);

NTSTATUS
CleanupWindowImpl(VOID);

#endif /* __WIN32K_WINDOW_H */

/* EOF */
