/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/clipboard.c
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

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
OpenClipboard(HWND hWndNewOwner)
{
    return NtUserOpenClipboard(hWndNewOwner, 0);
}

/*
 * @implemented
 */
UINT
WINAPI
EnumClipboardFormats(UINT format)
{
    return NtUserxEnumClipboardFormats(format);
}

/*
 * @implemented
 */
INT
WINAPI
GetClipboardFormatNameA(UINT format,
                        LPSTR lpszFormatName,
                        int cchMaxCount)
{
    LPWSTR lpBuffer;
    INT Length;

    lpBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, cchMaxCount * sizeof(WCHAR));
    if (!lpBuffer)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    /* we need a UNICODE string */
    Length = NtUserGetClipboardFormatName(format, lpBuffer, cchMaxCount);

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
WINAPI
GetClipboardFormatNameW(UINT uFormat,
                        LPWSTR lpszFormatName,
                        INT cchMaxCount)
{
    return NtUserGetClipboardFormatName(uFormat, lpszFormatName, cchMaxCount);
}

/*
 * @implemented
 */
UINT
WINAPI
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
WINAPI
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

static PVOID WINAPI
IntSynthesizeMultiByte(PVOID pwStr, DWORD cbStr, BOOL bOem)
{
    HANDLE hGlobal;
    PVOID pGlobal;
    INT cbGlobal;

    cbGlobal = WideCharToMultiByte(bOem ? CP_OEMCP : CP_ACP,
                                0, pwStr, cbStr / sizeof(WCHAR),
                                NULL, 0, NULL, NULL);
    hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cbGlobal);
    if (!hGlobal)
        return NULL;

    pGlobal = GlobalLock(hGlobal);
    WideCharToMultiByte(bOem ? CP_OEMCP : CP_ACP,
                        0, pwStr, cbStr / sizeof(WCHAR),
                        pGlobal, cbGlobal, NULL, NULL);
    return pGlobal;
}

static PVOID WINAPI
IntSynthesizeWideChar(PVOID pwStr, DWORD cbStr, BOOL bOem)
{
    HANDLE hGlobal;
    PVOID pGlobal;
    INT cbGlobal;

    cbGlobal = MultiByteToWideChar(bOem ? CP_OEMCP : CP_ACP,
                                   0, pwStr, cbStr, NULL, 0) * sizeof(WCHAR);
    hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cbGlobal);
    if (!hGlobal)
        return NULL;

    pGlobal = GlobalLock(hGlobal);
    MultiByteToWideChar(bOem ? CP_OEMCP : CP_ACP,
                        0, pwStr, cbStr, pGlobal, cbGlobal);
    return pGlobal;
}

/*
 * @implemented
 */
HANDLE
WINAPI
GetClipboardData(UINT uFormat)
{
    HANDLE hData = NULL;
    PVOID pData = NULL;
    DWORD cbData = 0;
    GETCLIPBDATA gcd;

    hData = NtUserGetClipboardData(uFormat, &gcd);
    if (!hData)
        return NULL;

    if (gcd.fGlobalHandle)
    {
        HANDLE hGlobal;

        NtUserCreateLocalMemHandle(hData, NULL, 0, &cbData);
        hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cbData);
        pData = GlobalLock(hGlobal);
        NtUserCreateLocalMemHandle(hData, pData, cbData, NULL);
        hData = hGlobal;
    }

    if (gcd.uFmtRet != uFormat)
    {
        SETCLIPBDATA scd = {FALSE, FALSE};
        HANDLE hNewData = NULL;
        PVOID pNewData = NULL;

        /* Synthesize requested format */
        switch (uFormat)
        {
            case CF_TEXT:
                if (gcd.uFmtRet == CF_UNICODETEXT)
                    pNewData = IntSynthesizeMultiByte(pData, cbData, uFormat == CF_OEMTEXT);
                else // CF_OEMTEXT
                    OemToCharBuffA(pData, pData, cbData);
                break;
            case CF_OEMTEXT:
                if (gcd.uFmtRet == CF_UNICODETEXT)
                    pNewData = IntSynthesizeMultiByte(pData, cbData, uFormat == CF_OEMTEXT);
                else
                    CharToOemBuffA(pData, pData, cbData);
                break;
            case CF_UNICODETEXT:
                pNewData = IntSynthesizeWideChar(pData, cbData, gcd.uFmtRet == CF_OEMTEXT);
                break;
            default:
                FIXME("Format: %u != %u\n", uFormat, gcd.uFmtRet);
        }

        /* Is it a global handle? */
        if (pNewData)
            hNewData = GlobalHandle(pNewData);

        if (hNewData)
        {
            /* Free old data */
            if (pData)
            {
                GlobalUnlock(hData);
                GlobalFree(hData);
            }
            hData = hNewData;
            pData = pNewData;
        }

        /* Save synthesized format in clipboard */
        if (pData)
        {
            HANDLE hMem;

            scd.fGlobalHandle = TRUE;
            hMem = NtUserConvertMemHandle(pData, GlobalSize(hData));
            NtUserSetClipboardData(uFormat, hMem, &scd);
        }
        else if (hData)
            NtUserSetClipboardData(uFormat, hData, &scd);
    }

    /* Unlock global handle */
    if (pData)
        GlobalUnlock(hData);

    return hData;
}

/*
 * @implemented
 */
HANDLE
WINAPI
SetClipboardData(UINT uFormat, HANDLE hMem)
{
    DWORD dwSize;
    HANDLE hGlobal;
    LPVOID pMem;
    HANDLE hRet = NULL;
    SETCLIPBDATA scd = {FALSE, FALSE};

    /* Check if this is a delayed rendering */
    if (hMem == NULL)
        return NtUserSetClipboardData(uFormat, NULL, &scd);

    if (hMem <= (HANDLE)4)
        SetLastError(ERROR_INVALID_PARAMETER);
    /* Bitmaps and palette does not use global handles */
    else if (uFormat == CF_BITMAP || uFormat == CF_DSPBITMAP || uFormat == CF_PALETTE)
        hRet = NtUserSetClipboardData(uFormat, hMem, &scd);
    /* Meta files are probably checked for validity */
    else if (uFormat == CF_DSPMETAFILEPICT || uFormat == CF_METAFILEPICT ||
             uFormat == CF_DSPENHMETAFILE || uFormat == CF_ENHMETAFILE)
    {
        UNIMPLEMENTED;
        hRet = NULL; // not supported yet
    }
    else
    {
        /* Some formats accept only global handles, other accept global handles or integer values */
        pMem = GlobalLock(hMem);
        dwSize = GlobalSize(hMem);

        if (pMem || uFormat == CF_DIB || uFormat == CF_DIBV5 ||
            uFormat == CF_DSPTEXT || uFormat == CF_LOCALE ||
            uFormat == CF_OEMTEXT || uFormat == CF_TEXT ||
            uFormat == CF_UNICODETEXT)
        {
            if (pMem)
            {
                /* This is a local memory. Make global memory object */
                hGlobal = NtUserConvertMemHandle(pMem, dwSize);

                /* Unlock memory */
                GlobalUnlock(hMem);
                /* FIXME: free hMem when CloseClipboard is called */

                if (hGlobal)
                {
                    /* Save data */
                    scd.fGlobalHandle = TRUE;
                    hRet = NtUserSetClipboardData(uFormat, hGlobal, &scd);
                }

                /* On success NtUserSetClipboardData returns pMem
                   however caller expects us to return hMem */
                if (hRet == hGlobal)
                    hRet = hMem;
            }
            else
                SetLastError(ERROR_INVALID_HANDLE);
        }
        else
        {
            /* Save a number */
            hRet = NtUserSetClipboardData(uFormat, hMem, &scd);
        }
    }

    if (!hRet)
        ERR("SetClipboardData(%u, %p) failed\n", uFormat, hMem);

    return hRet;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AddClipboardFormatListener(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}
/*
 * @unimplemented
 */
BOOL
WINAPI
RemoveClipboardFormatListener(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetUpdatedClipboardFormats(PUINT lpuiFormats,
                           UINT cFormats,
                           PUINT pcFormatsOut)
{
    UNIMPLEMENTED;
    return FALSE;
}
