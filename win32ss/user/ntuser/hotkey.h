#pragma once

typedef struct _HOT_KEY
{
    PTHREADINFO pti;
    PWND pWnd;
    UINT fsModifiers;
    UINT vk;
    INT id;
    struct _HOT_KEY *pNext;
} HOT_KEY, *PHOT_KEY;

/* Special Hot Keys */
#define IDHK_F12       -5
#define IDHK_SHIFTF12  -6
#define IDHK_WINKEY    -7
#define IDHK_WNDKEY    -8 /* WM_SETHOTKEY */

/* Window Snap Hot Keys */
#define IDHK_SNAP_LEFT   -10
#define IDHK_SNAP_RIGHT  -11
#define IDHK_SNAP_UP     -12
#define IDHK_SNAP_DOWN   -13

VOID FASTCALL UnregisterWindowHotKeys(PWND Window);
VOID FASTCALL UnregisterThreadHotKeys(PTHREADINFO pti);
BOOL NTAPI co_UserProcessHotKeys(WORD wVk, BOOL bIsDown);
UINT FASTCALL DefWndGetHotKey(PWND pWnd);
INT FASTCALL DefWndSetHotKey(PWND pWnd, WPARAM wParam);
VOID FASTCALL StartDebugHotKeys(VOID);
BOOL FASTCALL UserRegisterHotKey(PWND pWnd,int id,UINT fsModifiers,UINT vk);
BOOL FASTCALL UserUnregisterHotKey(PWND pWnd, int id);

/* EOF */
