/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/cliputils.c
 * PURPOSE:         Clipboard helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

#include "precomp.h"

static int
GetPredefinedClipboardFormatName(HINSTANCE hInstance,
                                 UINT uFormat,
                                 BOOL Unicode,
                                 PVOID lpszFormat,
                                 UINT cch)
{
    static
    struct FORMAT_NAME
    {
        UINT uFormat;
        UINT uResID;
    } uFormatList[] = {
    /* Table sorted in increasing order of CF_xxx values, please keep it this way! */
        {CF_TEXT        , STRING_CF_TEXT        },          // 1
        {CF_BITMAP      , STRING_CF_BITMAP      },          // 2
        {CF_METAFILEPICT, STRING_CF_METAFILEPICT},          // 3
        {CF_SYLK        , STRING_CF_SYLK        },          // 4
        {CF_DIF         , STRING_CF_DIF         },          // 5
        {CF_TIFF        , 0/*STRING_CF_TIFF*/        },          // 6
        {CF_OEMTEXT     , STRING_CF_OEMTEXT     },          // 7
        {CF_DIB         , STRING_CF_DIB         },          // 8
        {CF_PALETTE     , STRING_CF_PALETTE     },          // 9
        {CF_PENDATA     , 0/*STRING_CF_PENDATA*/     },          // 10
        {CF_RIFF        , 0/*STRING_CF_RIFF*/        },          // 11
        {CF_WAVE        , 0/*STRING_CF_WAVE*/        },          // 12
        {CF_UNICODETEXT , STRING_CF_UNICODETEXT },          // 13
        {CF_ENHMETAFILE , STRING_CF_ENHMETAFILE },          // 14
#if(WINVER >= 0x0400)
        {CF_HDROP       , STRING_CF_HDROP       },          // 15
        {CF_LOCALE      , STRING_CF_LOCALE      },          // 16
#endif
#if(WINVER >= 0x0500)
        {CF_DIBV5       , STRING_CF_DIBV5       },          // 17
#endif
    };

    switch (uFormat)
    {
        case CF_TEXT: case CF_BITMAP: case CF_METAFILEPICT:
        case CF_SYLK: case CF_DIF: // case CF_TIFF:
        case CF_OEMTEXT: case CF_DIB: case CF_PALETTE:
        // case CF_PENDATA: // case CF_RIFF: // case CF_WAVE:
        case CF_UNICODETEXT: case CF_ENHMETAFILE:
#if(WINVER >= 0x0400)
        case CF_HDROP: case CF_LOCALE:
#endif
#if(WINVER >= 0x0500)
        case CF_DIBV5:
#endif
        {
            if (Unicode)
                return LoadStringW(hInstance, uFormatList[uFormat-1].uResID, (LPWSTR)lpszFormat, cch);
            else
                return LoadStringA(hInstance, uFormatList[uFormat-1].uResID, (LPSTR)lpszFormat, cch);
        }

        default:
        {
            return 0;
        }
    }
}

void
RetrieveClipboardFormatName(HINSTANCE hInstance,
                            UINT uFormat,
                            BOOL Unicode,
                            PVOID lpszFormat,
                            UINT cch)
{
    if (!GetPredefinedClipboardFormatName(hInstance, uFormat, Unicode, lpszFormat, cch))
    {
        if (Unicode)
        {
            if (!GetClipboardFormatNameW(uFormat, (LPWSTR)lpszFormat, cch))
                LoadStringW(hInstance, STRING_CF_UNKNOWN, (LPWSTR)lpszFormat, cch);
        }
        else
        {
            if (!GetClipboardFormatNameA(uFormat, (LPSTR)lpszFormat, cch))
                LoadStringA(hInstance, STRING_CF_UNKNOWN, (LPSTR)lpszFormat, cch);
        }
    }
}

void DeleteClipboardContent(void)
{
    if (!OpenClipboard(Globals.hMainWnd))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    if (!EmptyClipboard())
    {
        ShowLastWin32Error(Globals.hMainWnd);
    }

    CloseClipboard();
}

UINT GetAutomaticClipboardFormat(void)
{
    static UINT uFormatList[] = {
        CF_UNICODETEXT,
        CF_ENHMETAFILE,
        CF_METAFILEPICT,
        CF_DIBV5,
        CF_DIB,
        CF_BITMAP
    };

    return GetPriorityClipboardFormat(uFormatList, ARRAYSIZE(uFormatList));
}

BOOL IsClipboardFormatSupported(UINT uFormat)
{
    switch (uFormat)
    {
        case CF_UNICODETEXT:
        case CF_BITMAP:
        case CF_ENHMETAFILE:
        case CF_METAFILEPICT:
        case CF_DIB:
        case CF_DIBV5:
        {
            return TRUE;
        }

        default:
        {
            return FALSE;
        }
    }
}
