#ifndef _WIN32K_HOTKEY_H
#define _WIN32K_HOTKEY_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/winsta.h>
#include <include/window.h>

typedef struct _HOT_KEY_ITEM
{
  LIST_ENTRY ListEntry;
  struct _ETHREAD *Thread;
  HWND hWnd;
  int id;
  UINT fsModifiers;
  UINT vk;
} HOT_KEY_ITEM, *PHOT_KEY_ITEM;

NTSTATUS FASTCALL
InitHotKeys(PWINSTATION_OBJECT WinStaObject);

NTSTATUS FASTCALL
CleanupHotKeys(PWINSTATION_OBJECT WinStaObject);

BOOL
GetHotKey (PWINSTATION_OBJECT WinStaObject,
       UINT fsModifiers,
	   UINT vk,
	   struct _ETHREAD **Thread,
	   HWND *hWnd,
	   int *id);

VOID
UnregisterWindowHotKeys(PWINDOW_OBJECT Window);

VOID
UnregisterThreadHotKeys(struct _ETHREAD *Thread);

#define IntLockHotKeys(WinStaObject) \
  ExAcquireFastMutex(&WinStaObject->HotKeyListLock)

#define IntUnLockHotKeys(WinStaObject) \
  ExReleaseFastMutex(&WinStaObject->HotKeyListLock)

#endif /* _WIN32K_HOTKEY_H */

/* EOF */
