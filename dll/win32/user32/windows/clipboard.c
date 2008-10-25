/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/clipboard.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Pablo Borobia <pborobia@gmail.com>
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 *
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#define NDEBUG

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define QUERY_SIZE 0

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
OpenClipboard(HWND hWndNewOwner)
{
    BOOL ret = NtUserOpenClipboard(hWndNewOwner, 0);
    return ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
CloseClipboard(VOID)
{
    BOOL ret;
    ret = NtUserCloseClipboard();
    return ret;
}

/*
 * @implemented
 */
INT
STDCALL
CountClipboardFormats(VOID)
{
    INT ret = NtUserCountClipboardFormats();
    return ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
EmptyClipboard(VOID)
{
    return NtUserEmptyClipboard();
}

/*
 * @implemented
 */
UINT
STDCALL
EnumClipboardFormats(UINT format)
{
    UINT ret = NtUserCallOneParam(format, ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS);
    return ret;
}

/*
 * @implemented
 */
HANDLE
STDCALL
GetClipboardData(UINT uFormat)
{
    HGLOBAL hGlobal = NULL;
    PVOID pGlobal = NULL;
    DWORD size = 0;

    /* dealing with bitmap object */
    if (uFormat != CF_BITMAP)
    {
        size = (DWORD)NtUserGetClipboardData(uFormat, NULL);

        if (size)
        {
            hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, size);
            pGlobal = GlobalLock(hGlobal);

            size = (DWORD)NtUserGetClipboardData(uFormat, pGlobal);

            GlobalUnlock(hGlobal);
        }
    }
    else
    {
        hGlobal = NtUserGetClipboardData(CF_BITMAP, NULL);
    }

    return hGlobal;
}

/*
 * @implemented
 */
INT
STDCALL
GetClipboardFormatNameA(UINT format,
                        LPSTR lpszFormatName,
                        int cchMaxCount)
{
    LPWSTR lpBuffer;
    UNICODE_STRING FormatName;
    INT Length;

    lpBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, cchMaxCount * sizeof(WCHAR));
    if (!lpBuffer)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    FormatName.Length = 0;
    FormatName.MaximumLength = cchMaxCount * sizeof(WCHAR);
    FormatName.Buffer = lpBuffer;

    /* we need a UNICODE string */
    Length = NtUserGetClipboardFormatName(format, &FormatName, cchMaxCount);

    if (Length != 0)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, lpBuffer, Length, lpszFormatName, cchMaxCount, NULL, NULL))
        {
            /* clear result string */
            Length = 0;
        }
        lpszFormatName[Length] = '\0';
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, lpBuffer);
    return Length;
}

/*
 * @implemented
 */
INT
STDCALL
GetClipboardFormatNameW(UINT format,
                        LPWSTR lpszFormatName,
                        INT cchMaxCount)
{
    UNICODE_STRING FormatName;
    ULONG Ret;

    FormatName.Length = 0;
    FormatName.MaximumLength = cchMaxCount * sizeof(WCHAR);
    FormatName.Buffer = (PWSTR)lpszFormatName;
    Ret = NtUserGetClipboardFormatName(format, &FormatName, cchMaxCount);
    return Ret;

}

/*
 * @implemented
 */
HWND
STDCALL
GetClipboardOwner(VOID)
{
    return NtUserGetClipboardOwner();
}

/*
 * @implemented
 */
DWORD
STDCALL
GetClipboardSequenceNumber(VOID)
{
    return NtUserGetClipboardSequenceNumber();
}

/*
 * @implemented
 */
HWND
STDCALL
GetClipboardViewer(VOID)
{
    return NtUserGetClipboardViewer();
}

/*
 * @implemented
 */
HWND
STDCALL
GetOpenClipboardWindow(VOID)
{
    return NtUserGetOpenClipboardWindow();
}

/*
 * @implemented
 */
INT
STDCALL
GetPriorityClipboardFormat(UINT *paFormatPriorityList, INT cFormats)
{
    INT ret = NtUserGetPriorityClipboardFormat(paFormatPriorityList, cFormats);
    return ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
IsClipboardFormatAvailable(UINT format)
{
    BOOL ret = NtUserIsClipboardFormatAvailable(format);
    return ret;
}

/*
 * @implemented
 */

UINT
STDCALL
RegisterClipboardFormatA(LPCSTR lpszFormat)
{
    UINT ret = 0;
    UNICODE_STRING usFormat = {0};

    if (lpszFormat == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* check for "" */
    if (*lpszFormat == 0) //NULL
    {
        SetLastError(ERROR_INVALID_NAME);
        return 0;
    }

    ret = RtlCreateUnicodeStringFromAsciiz(&usFormat, lpszFormat);
    if (ret)
    {
        ret = NtUserRegisterWindowMessage(&usFormat); //(LPCWSTR)
        RtlFreeUnicodeString(&usFormat);
    }

    return ret;
}

/*
 * @implemented
 */
UINT
STDCALL
RegisterClipboardFormatW(LPCWSTR lpszFormat)
{
    UINT ret = 0;
    UNICODE_STRING usFormat = {0};

    if (lpszFormat == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* check for "" */
    if (*lpszFormat == 0) //NULL
    {
        SetLastError(ERROR_INVALID_NAME);
        return 0;
    }

    RtlInitUnicodeString(&usFormat, lpszFormat);
    ret = NtUserRegisterWindowMessage(&usFormat);

    return ret;
}

HGLOBAL
renderLocale(DWORD Locale)
{
    DWORD* pLocale;
    HGLOBAL hGlobal;

    hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(DWORD));

    if(!hGlobal)
    {
        return hGlobal;
    }

    pLocale = (DWORD*)GlobalLock(hGlobal);

    *pLocale = Locale;

    GlobalUnlock(hGlobal);

    return hGlobal;
}

/*
 * @implemented
 */
HANDLE
STDCALL
SetClipboardData(UINT uFormat, HANDLE hMem)
{
    DWORD size;
    LPVOID pMem;
    HANDLE ret = NULL;

    if (hMem == NULL)
    {
        return NtUserSetClipboardData(uFormat, 0, 0);
    }

    if (uFormat == CF_BITMAP)
    {
        /* GlobalLock should return 0 for GDI handles
        pMem = GlobalLock(hMem);
        if (pMem)
        {
            // not a  GDI handle
            GlobalUnlock(hMem);
            return ret;
        }
        else
        {
            */
            /* check if this GDI handle is a HBITMAP */
            /* GetObject for HBITMAP not implemented in ReactOS */
            //if (GetObject(hMem, 0, NULL) == sifeof(BITMAP))
            //{
                return NtUserSetClipboardData(CF_BITMAP, hMem, 0);
            //}
        /*}*/
    }

    size = GlobalSize(hMem);
    pMem = GlobalLock(hMem);

    if ((pMem) && (size))
    {
        size = GlobalSize(hMem);
        ret = NtUserSetClipboardData(uFormat, pMem, size);
        //should i unlock hMem?
        GlobalUnlock(hMem);
    }
    else
    {
        ERR("SetClipboardData failed\n");
    }

    return ret;

}

/*
 * @implemented
 */
HWND
STDCALL
SetClipboardViewer(HWND hWndNewViewer)
{
    return NtUserSetClipboardViewer(hWndNewViewer);
}

/*
 * @implemented
 */
BOOL
STDCALL
ChangeClipboardChain(HWND hWndRemove,
                     HWND hWndNewNext)
{
    return NtUserChangeClipboardChain(hWndRemove, hWndNewNext);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AddClipboardFormatListener(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}
/*
 * @unimplemented
 */
BOOL
STDCALL
RemoveClipboardFormatListener(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetUpdatedClipboardFormats(PUINT lpuiFormats,
                           UINT cFormats,
                           PUINT pcFormatsOut)
{
    UNIMPLEMENTED;
    return FALSE;
}
