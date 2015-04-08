/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/misc.c
 * PURPOSE:     miscallanous functions
 * COPYRIGHT:   Copyright 2005 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

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
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

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
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(WCHAR));
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
    LPWSTR lpFormat;
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
                             (LPWSTR)lpTarget,
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

VOID GetError(VOID)
{
    LPWSTR lpMsgBuf = NULL;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (VOID*)&lpMsgBuf,
                   0,
                   NULL );

    MessageBoxW(NULL, lpMsgBuf, L"Error!", MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
}

VOID DisplayString(LPWSTR Msg)
{
    MessageBoxW(NULL, Msg, L"Note!", MB_ICONEXCLAMATION|MB_OK);
}



HIMAGELIST
InitImageList(UINT StartResource,
              UINT EndResource,
              UINT Width,
              UINT Height,
              ULONG type)
{
    HANDLE hImage;
    HIMAGELIST himl;
    UINT i;
    INT ret;

    /* Create the toolbar icon image list */
    himl = ImageList_Create(Width,
                            Height,
                            ILC_MASK | ILC_COLOR32,
                            EndResource - StartResource,
                            0);
    if (himl == NULL)
        return NULL;

    ret = 0;
    for (i = StartResource; i <= EndResource && ret != -1; i++)
    {
        hImage = LoadImageW(hInstance,
                            MAKEINTRESOURCEW(i),
                            type,
                            Width,
                            Height,
                            LR_LOADTRANSPARENT);
        if (hImage == NULL)
        {
            ImageList_Destroy(himl);
            himl = NULL;
            break;
        }

        if (type == IMAGE_BITMAP)
        {
            ret = ImageList_AddMasked(himl,
                                      hImage,
                                      RGB(255, 0, 128));
        }
        else if (type == IMAGE_ICON)
        {
            ret = ImageList_AddIcon(himl,
                                    hImage);
        }

        DeleteObject(hImage);
    }

    if (ret == -1)
    {
        ImageList_Destroy(himl);
        himl = NULL;
    }

    return himl;
}
