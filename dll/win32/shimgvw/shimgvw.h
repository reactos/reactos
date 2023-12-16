/*
 * PROJECT:     ReactOS Picture and Fax Viewer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Image file browsing and manipulation
 * COPYRIGHT:   Copyright Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2018-2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define INITGUID

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <objbase.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <debug.h>

#include "resource.h"

extern HINSTANCE g_hInstance;
extern GpImage *g_pImage;

typedef struct
{
    BOOL Maximized;
    INT X;
    INT Y;
    INT Width;
    INT Height;
} SHIMGVW_SETTINGS;

typedef struct tagSHIMGVW_FILENODE
{
    WCHAR FileName[MAX_PATH];
    struct tagSHIMGVW_FILENODE *Prev;
    struct tagSHIMGVW_FILENODE *Next;
} SHIMGVW_FILENODE;

#define WC_PREVIEW L"ShImgVw:CPreviewWnd"
#define WC_ZOOM L"ShImgVw:CZoomWnd"

/* Animation */
typedef struct tagANIME
{
    UINT           m_nFrameIndex;
    UINT           m_nFrameCount;
    UINT           m_nLoopIndex;
    UINT           m_nLoopCount;
    PropertyItem  *m_pDelayItem;
    HWND           m_hwndTimer;
} ANIME, *PANIME;

void Anime_FreeInfo(PANIME pAnime);
BOOL Anime_LoadInfo(PANIME pAnime);
void Anime_SetTimerWnd(PANIME pAnime, HWND hwndTimer);
void Anime_SetFrameIndex(PANIME pAnime, UINT nFrameIndex);
void Anime_Start(PANIME pAnime, DWORD dwDelay);
void Anime_Pause(PANIME pAnime);
BOOL Anime_OnTimer(PANIME pAnime, WPARAM wParam);

static inline LPVOID QuickAlloc(SIZE_T cbSize, BOOL bZero)
{
    return HeapAlloc(GetProcessHeap(), (bZero ? HEAP_ZERO_MEMORY : 0), cbSize);
}

static inline VOID QuickFree(LPVOID ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
