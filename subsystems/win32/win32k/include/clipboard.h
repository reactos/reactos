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

VOID NTAPI
UserEmptyClipboardData(struct _WINSTATION_OBJECT *pWinSta);
