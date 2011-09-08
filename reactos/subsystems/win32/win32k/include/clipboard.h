#pragma once

#include "window.h"
#include <include/win32.h>

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

struct _WINSTATION_OBJECT;

VOID NTAPI
UserEmptyClipboardData(struct _WINSTATION_OBJECT *pWinSta);
