#ifndef __WIN32K_HOTKEY_H
#define __WIN32K_HOTKEY_H

#include <windows.h>
#include <ddk/ntddk.h>

NTSTATUS FASTCALL
InitHotKeyImpl (VOID);

NTSTATUS FASTCALL
CleanupHotKeyImpl (VOID);

BOOL
GetHotKey (UINT fsModifiers,
	   UINT vk,
	   struct _ETHREAD **Thread,
	   HWND *hWnd,
	   int *id);

VOID
UnregisterWindowHotKeys(HWND hWnd);

VOID
UnregisterThreadHotKeys(struct _ETHREAD *Thread);

#endif /* __WIN32K_HOTKEY_H */

/* EOF */
