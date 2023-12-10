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
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

#include "resource.h"

#define TB_IMAGE_WIDTH  16
#define TB_IMAGE_HEIGHT 16

extern HINSTANCE g_hInstance;
extern HWND g_hDispWnd;
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

#define WC_SHIMGVW L"ShImgVw:CPreviewWnd"

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

#define ANIME_TIMER_ID  9999

void Anime_FreeInfo(PANIME pAnime);
BOOL Anime_LoadInfo(PANIME pAnime);
void Anime_SetTimerWnd(PANIME pAnime, HWND hwndTimer);
void Anime_SetFrameIndex(PANIME pAnime, UINT nFrameIndex);
DWORD Anime_GetFrameDelay(PANIME pAnime, UINT nFrameIndex);
void Anime_Start(PANIME pAnime, DWORD dwDelay);
void Anime_Pause(PANIME pAnime);
BOOL Anime_Step(PANIME pAnime, DWORD *pdwDelay);
