#pragma once

typedef struct _HOT_KEY_ITEM
{
  LIST_ENTRY ListEntry;
  struct _ETHREAD *Thread;
  HWND hWnd;
  int id;
  UINT fsModifiers;
  UINT vk;
} HOT_KEY_ITEM, *PHOT_KEY_ITEM;

#define IDHOT_REACTOS (-9)

INIT_FUNCTION NTSTATUS NTAPI InitHotkeyImpl(VOID);

BOOL FASTCALL
GetHotKey (UINT fsModifiers,
	   UINT vk,
	   struct _ETHREAD **Thread,
	   HWND *hWnd,
	   int *id);

VOID FASTCALL UnregisterWindowHotKeys(PWND Window);
VOID FASTCALL UnregisterThreadHotKeys(struct _ETHREAD *Thread);
UINT FASTCALL DefWndGetHotKey(HWND hwnd);
INT FASTCALL  DefWndSetHotKey( PWND pWnd, WPARAM wParam);

/* EOF */
