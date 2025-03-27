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
#define COBJMACROS

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
#include <shobjidl.h>

#include <debug.h>

#include "resource.h"

#define WM_UPDATECOMMANDSTATE (WM_APP + 0)

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

void DoShellContextMenuOnFile(HWND hwnd, PCWSTR File, LPARAM lParam);
void EnableCommandIfVerbExists(UINT ImageId, HWND hwnd, UINT CmdId, PCWSTR Verb, PCWSTR File);
void ShellExecuteVerb(HWND hwnd, PCWSTR Verb, PCWSTR File, BOOL Quit);
UINT ErrorBox(HWND hwnd, UINT Error);
void DisplayHelp(HWND hwnd);

static inline LPVOID QuickAlloc(SIZE_T cbSize, BOOL bZero)
{
    return HeapAlloc(GetProcessHeap(), (bZero ? HEAP_ZERO_MEMORY : 0), cbSize);
}

static inline VOID QuickFree(LPVOID ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

static inline WORD Swap16(WORD v)
{
    return MAKEWORD(HIBYTE(v), LOBYTE(v));
}

static inline UINT Swap32(UINT v)
{
    return MAKELONG(Swap16(HIWORD(v)), Swap16(LOWORD(v)));
}

#ifdef _WIN32
#define BigToHost32 Swap32
#endif

static inline ULARGE_INTEGER MakeULargeInteger(UINT64 value)
{
    ULARGE_INTEGER ret;
    ret.QuadPart = value;
    return ret;
}

static inline HRESULT SHIMGVW_HResultFromWin32(DWORD hr)
{
     // HRESULT_FROM_WIN32 will evaluate its parameter twice, this function will not.
    return HRESULT_FROM_WIN32(hr);
}

static inline HRESULT HResultFromGdiplus(Status status)
{
    switch ((UINT)status)
    {
        case Ok: return S_OK;
        case InvalidParameter: return E_INVALIDARG;
        case OutOfMemory: return E_OUTOFMEMORY;
        case NotImplemented: return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
        case Win32Error: return SHIMGVW_HResultFromWin32(GetLastError());
        case FileNotFound: return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        case AccessDenied: return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        case UnknownImageFormat: return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    return E_FAIL;
}
