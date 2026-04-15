/*
 * PROJECT:     ReactOS User API Server DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Defining FakeDrag* functions and removing shell32 import
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <consrv.h>
#include <shellapi.h>
#include <shlobj.h>
#include "fakedrag.h"

#define NDEBUG
#include <debug.h>

UINT FakeDragQueryFileA(HDROP hDrop, UINT lFile, LPSTR lpszFile, UINT lLength)
{
    LPSTR lpDrop;
    UINT i = 0;
    DROPFILES *lpDropFileStruct = GlobalLock(hDrop);

    if (!lpDropFileStruct)
        goto end;

    lpDrop = (LPSTR)lpDropFileStruct + lpDropFileStruct->pFiles;

    if (lpDropFileStruct->fWide)
    {
        LPWSTR lpszFileW = NULL;
        if (lpszFile && lFile != 0xFFFFFFFF)
        {
            lpszFileW = HeapAlloc(GetProcessHeap(), 0, lLength*sizeof(WCHAR));
            if (!lpszFileW)
                goto end;
        }

        i = FakeDragQueryFileW(hDrop, lFile, lpszFileW, lLength);

        if (lpszFileW)
        {
            WideCharToMultiByte(CP_ACP, 0, lpszFileW, -1, lpszFile, lLength, 0, NULL);
            HeapFree(GetProcessHeap(), 0, lpszFileW);
        }
        goto end;
    }

    while (i++ < lFile)
    {
        while (*lpDrop++); /* skip filename */

        if (!*lpDrop)
        {
            i = (lFile == 0xFFFFFFFF) ? i : 0;
            goto end;
        }
    }

    i = lstrlenA(lpDrop);
    if (!lpszFile)
        goto end;
    lstrcpynA(lpszFile, lpDrop, lLength);
end:
    GlobalUnlock(hDrop);
    return i;
}

UINT FakeDragQueryFileW(HDROP hDrop, UINT lFile, LPWSTR lpszwFile, UINT lLength)
{
    LPWSTR lpwDrop;
    UINT i = 0;
    DROPFILES *lpDropFileStruct = GlobalLock(hDrop);

    if (!lpDropFileStruct)
        goto end;

    lpwDrop = (LPWSTR)((LPSTR)lpDropFileStruct + lpDropFileStruct->pFiles);

    if (!lpDropFileStruct->fWide)
    {
        LPSTR lpszFileA = NULL;

        if (lpszwFile && lFile != 0xFFFFFFFF)
        {
            lpszFileA = HeapAlloc(GetProcessHeap(), 0, lLength);
            if (!lpszFileA)
                goto end;
        }

        i = FakeDragQueryFileA(hDrop, lFile, lpszFileA, lLength);

        if (lpszFileA)
        {
            MultiByteToWideChar(CP_ACP, 0, lpszFileA, -1, lpszwFile, lLength);
            HeapFree(GetProcessHeap(), 0, lpszFileA);
        }
        goto end;
    }

    i = 0;
    while (i++ < lFile)
    {
        while (*lpwDrop++); /* skip filename */

        if (!*lpwDrop)
        {
            i = (lFile == 0xFFFFFFFF) ? i : 0;
            goto end;
        }
    }

    i = lstrlenW(lpwDrop);
    if (!lpszwFile)
        goto end;
    lstrcpynW(lpszwFile, lpwDrop, lLength);

end:
    GlobalUnlock(hDrop);
    return i;
}

VOID FakeDragAcceptFiles(HWND hWnd, BOOL fAccept)
{
    if (!IsWindow(hWnd))
        return;

    LONG_PTR exstyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (fAccept)
        exstyle |= WS_EX_ACCEPTFILES;
    else
        exstyle &= ~WS_EX_ACCEPTFILES;

    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exstyle);
}
