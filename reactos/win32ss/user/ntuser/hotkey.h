#pragma once

typedef struct _HOT_KEY
{
    struct _ETHREAD *pThread;
    HWND hWnd;
    UINT fsModifiers;
    UINT vk;
    INT id;
    struct _HOT_KEY *pNext;
} HOT_KEY, *PHOT_KEY;

/* Special Hot Keys */
#define IDHK_F12       -5
#define IDHK_SHIFTF12  -6
#define IDHK_WINKEY    -7
#define IDHK_REACTOS   -8

VOID FASTCALL UnregisterWindowHotKeys(PWND Window);
VOID FASTCALL UnregisterThreadHotKeys(struct _ETHREAD *pThread);
BOOL NTAPI co_UserProcessHotKeys(WORD wVk, BOOL bIsDown);
UINT FASTCALL DefWndGetHotKey(HWND hwnd);
INT FASTCALL DefWndSetHotKey(PWND pWnd, WPARAM wParam);

/* EOF */
