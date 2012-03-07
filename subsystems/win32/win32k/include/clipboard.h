#pragma once

typedef struct _CLIP
{
    UINT   fmt;
    HANDLE hData;
    BOOL   fGlobalHandle;
} CLIP, *PCLIP;

UINT APIENTRY
UserEnumClipboardFormats(UINT uFormat);

VOID FASTCALL
UserClipboardFreeWindow(PWND pWindow);

BOOL NTAPI
UserOpenClipboard(HWND hWnd);

BOOL NTAPI
UserCloseClipboard(VOID);

BOOL NTAPI
UserEmptyClipboard(VOID);

VOID NTAPI
UserEmptyClipboardData(struct _WINSTATION_OBJECT *pWinSta);

HANDLE NTAPI
UserSetClipboardData(UINT fmt, HANDLE hData, PSETCLIPBDATA scd);
