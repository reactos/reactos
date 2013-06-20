/*
 * PROJECT:     ReactOS API Test GUI
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        
 * PURPOSE:     miscallanous functions
 * COPYRIGHT:   Copyright 2005 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 - 2008 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include <precomp.h>

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCEW((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = (WCHAR*) LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}

INT
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPTSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(TCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPTSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use Format to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPTSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

BOOL
StatusBarLoadAndFormatString(IN HWND hStatusBar,
                             IN INT PartId,
                             IN HINSTANCE hInstance,
                             IN UINT uID,
                             ...)
{
    BOOL Ret = FALSE;
    LPWSTR lpFormat, lpStr;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, uID);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (VOID*)&lpStr,
                             0,
                             &lArgs);
        va_end(lArgs);

        if (lpStr != NULL)
        {
            Ret = (BOOL)SendMessageW(hStatusBar,
                                     SB_SETTEXT,
                                     (WPARAM)PartId,
                                     (LPARAM)lpStr);
            LocalFree((HLOCAL)lpStr);
        }

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

BOOL
StatusBarLoadString(IN HWND hStatusBar,
                    IN INT PartId,
                    IN HINSTANCE hInstance,
                    IN UINT uID)
{
    BOOL Ret = FALSE;
    LPWSTR lpStr;

    if (AllocAndLoadString(&lpStr,
                           hInstance,
                           uID) > 0)
    {
        Ret = (BOOL)SendMessageW(hStatusBar,
                                 SB_SETTEXT,
                                 (WPARAM)PartId,
                                 (LPARAM)lpStr);
        LocalFree((HLOCAL)lpStr);
    }

    return Ret;
}


INT
GetTextFromEdit(OUT LPWSTR lpString,
                IN HWND hDlg,
                IN UINT Res)
{
    INT len = GetWindowTextLengthW(GetDlgItem(hDlg, Res));
    if(len > 0)
    {
        GetDlgItemTextW(hDlg,
                        Res,
                        lpString,
                        len + 1);
    }
    else
        lpString = NULL;

    return len;
}

VOID DisplayError(INT err)
{
    LPWSTR lpMsgBuf = NULL;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   err,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (VOID*)&lpMsgBuf,
                   0,
                   NULL );

    MessageBoxW(NULL, lpMsgBuf, L"Error!", MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
}

VOID DisplayMessage(LPWSTR lpMsg)
{
    MessageBoxW(NULL, lpMsg, L"Note!", MB_ICONEXCLAMATION|MB_OK);
}



HIMAGELIST
InitImageList(UINT StartResource,
              UINT EndResource,
              UINT Width,
              UINT Height)
{
    HICON hIcon;
    HIMAGELIST hImageList;
    UINT i;
    INT Ret;

    /* Create the toolbar icon image list */
    hImageList = ImageList_Create(Width,
                                  Height,
                                  ILC_MASK | ILC_COLOR32,
                                  EndResource - StartResource,
                                  0);
    if (hImageList == NULL)
        return NULL;

    /* Add all icons to the image list */
    for (i = StartResource; i <= EndResource; i++)
    {
        hIcon = (HICON)LoadImageW(hInstance,
                                  MAKEINTRESOURCEW(i),
                                  IMAGE_ICON,
                                  Width,
                                  Height,
                                  LR_DEFAULTCOLOR);
        if (hIcon == NULL)
            goto fail;

        Ret = ImageList_AddIcon(hImageList,
                                hIcon);
        if (Ret == -1)
            goto fail;

        DestroyIcon(hIcon);
    }

    return hImageList;

fail:
    ImageList_Destroy(hImageList);
    return NULL;
}

DWORD
AnsiToUnicode(LPCSTR lpSrcStr,
              LPWSTR *lpDstStr)
{
    INT length;
    INT ret = 0;

    length = strlen(lpSrcStr) + 1;

    *lpDstStr = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, length * sizeof(WCHAR));
    if (*lpDstStr)
    {
        ret = MultiByteToWideChar(CP_ACP,
                                  0,
                                  lpSrcStr,
                                  -1,
                                  *lpDstStr,
                                  length);
    }

    return ret;
}

DWORD
UnicodeToAnsi(LPCWSTR lpSrcStr,
              LPSTR *lpDstStr)
{
    INT length;
    INT ret = 0;

    length = wcslen(lpSrcStr) + 1;

    *lpDstStr = (LPSTR)HeapAlloc(GetProcessHeap(), 0, length);
    if (*lpDstStr)
    {
        ret = WideCharToMultiByte(CP_ACP,
                                  0,
                                  lpSrcStr,
                                  -1,
                                  *lpDstStr,
                                  length,
                                  NULL,
                                  NULL);
    }

    return ret;
}
