/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Clipboard routines
 * FILE:             win32ss/user/ntuser/clipboard.c
 * PROGRAMER:        Filip Navara <xnavara@volny.cz>
 *                   Pablo Borobia <pborobia@gmail.com>
 *                   Rafal Harabien <rafalh@reactos.org>
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserClipbrd);

#define DATA_DELAYED     (HANDLE)0
#define DATA_SYNTH_USER  (HANDLE)1
#define DATA_SYNTH_KRNL  (HANDLE)2
#define IS_DATA_DELAYED(ce)     ((ce)->hData == DATA_DELAYED)
#define IS_DATA_SYNTHESIZED(ce) ((ce)->hData == DATA_SYNTH_USER || (ce)->hData == DATA_SYNTH_KRNL)

static PWINSTATION_OBJECT FASTCALL
IntGetWinStaForCbAccess(VOID)
{
    HWINSTA hWinSta;
    PWINSTATION_OBJECT pWinStaObj;
    NTSTATUS Status;

    hWinSta = UserGetProcessWindowStation();
    Status = IntValidateWindowStationHandle(hWinSta, UserMode, WINSTA_ACCESSCLIPBOARD, &pWinStaObj, 0);
    if (!NT_SUCCESS(Status))
    {
        ERR("Cannot open winsta\n");
        SetLastNtError(Status);
        return NULL;
    }

    return pWinStaObj;
}

/* If format exists, returns a non-null value (pointing to formated object) */
static PCLIP FASTCALL
IntGetFormatElement(PWINSTATION_OBJECT pWinStaObj, UINT fmt)
{
    DWORD i;

    for (i = 0; i < pWinStaObj->cNumClipFormats; ++i)
    {
        if (pWinStaObj->pClipBase[i].fmt == fmt)
            return &pWinStaObj->pClipBase[i];
    }

    return NULL;
}

static BOOL FASTCALL
IntIsFormatAvailable(PWINSTATION_OBJECT pWinStaObj, UINT fmt)
{
    return IntGetFormatElement(pWinStaObj, fmt) != NULL;
}

static VOID FASTCALL
IntFreeElementData(PCLIP pElement)
{
    if (!IS_DATA_DELAYED(pElement) &&
        !IS_DATA_SYNTHESIZED(pElement))
    {
        if (pElement->fGlobalHandle)
            UserDeleteObject(pElement->hData, TYPE_CLIPDATA);
        else if (pElement->fmt == CF_BITMAP          ||
                 pElement->fmt == CF_PALETTE         ||
                 pElement->fmt == CF_DSPBITMAP       ||
                 pElement->fmt == CF_METAFILEPICT    ||
                 pElement->fmt == CF_DSPMETAFILEPICT ||
                 pElement->fmt == CF_DSPENHMETAFILE  ||
                 pElement->fmt == CF_ENHMETAFILE )
        {
            GreSetObjectOwner(pElement->hData, GDI_OBJ_HMGR_POWNED);
            GreDeleteObject(pElement->hData);
        }
    }
}

/* Adds a new format and data to the clipboard */
static PCLIP NTAPI
IntAddFormatedData(PWINSTATION_OBJECT pWinStaObj, UINT fmt, HANDLE hData, BOOLEAN fGlobalHandle, BOOL bEnd)
{
    PCLIP pElement = NULL;

    /* Use existing entry with specified format */
    if (!bEnd)
        pElement = IntGetFormatElement(pWinStaObj, fmt);

    /* Put new entry at the end if nothing was found */
    if (!pElement)
    {
        /* Allocate bigger clipboard if needed. We could use lists but Windows uses array */
        if (pWinStaObj->cNumClipFormats % 4 == 0)
        {
            PCLIP pNewClip;

            /* Allocate new clipboard */
            pNewClip = ExAllocatePoolWithTag(PagedPool,
                                             (pWinStaObj->cNumClipFormats + 4) * sizeof(CLIP),
                                             USERTAG_CLIPBOARD);
            if (!pNewClip)
            {
                EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return NULL;
            }

            /* Copy data */
            memcpy(pNewClip, pWinStaObj->pClipBase, pWinStaObj->cNumClipFormats * sizeof(CLIP));

            /* Free old clipboard */
            if (pWinStaObj->pClipBase)
                ExFreePoolWithTag(pWinStaObj->pClipBase, USERTAG_CLIPBOARD);

            /* Update WinSta */
            pWinStaObj->pClipBase = pNewClip;
        }

        /* New element is at the end */
        pElement = &pWinStaObj->pClipBase[pWinStaObj->cNumClipFormats];
        pElement->fmt = fmt;
        pWinStaObj->cNumClipFormats++;
    }
    else
        IntFreeElementData(pElement);

    pElement->hData = hData;
    pElement->fGlobalHandle = fGlobalHandle;

    return pElement;
}

static BOOL FASTCALL
IntIsClipboardOpenByMe(PWINSTATION_OBJECT pWinSta)
{
    /* Check if the current thread has opened the clipboard */
    return (pWinSta->ptiClipLock &&
            pWinSta->ptiClipLock == PsGetCurrentThreadWin32Thread());
}

static VOID NTAPI
IntSynthesizeDib(
    PWINSTATION_OBJECT pWinStaObj,
    HBITMAP hbm)
{
    HDC hdc;
    ULONG cjInfoSize, cjDataSize;
    PCLIPBOARDDATA pClipboardData;
    HANDLE hMem;
    INT iResult;
    struct
    {
        BITMAPINFOHEADER bmih;
        RGBQUAD rgbColors[256];
    } bmiBuffer;
    PBITMAPINFO pbmi = (PBITMAPINFO)&bmiBuffer;

    /* Get the display DC */
    hdc = UserGetDCEx(NULL, NULL, DCX_USESTYLE);
    if (!hdc)
    {
        return;
    }

    /* Get information about the bitmap format */
    memset(&bmiBuffer, 0, sizeof(bmiBuffer));
    pbmi->bmiHeader.biSize = sizeof(bmiBuffer.bmih);
    iResult = GreGetDIBitsInternal(hdc,
                                   hbm,
                                   0,
                                   0,
                                   NULL,
                                   pbmi,
                                   DIB_RGB_COLORS,
                                   0,
                                   sizeof(bmiBuffer));
    if (iResult == 0)
    {
       goto cleanup;
    }

    /* Get the size for a full BITMAPINFO */
    cjInfoSize = DIB_BitmapInfoSize(pbmi, DIB_RGB_COLORS);

    /* Calculate the size of the clipboard data, which is a packed DIB */
    cjDataSize = cjInfoSize + pbmi->bmiHeader.biSizeImage;

    /* Create the clipboard data */
    pClipboardData = (PCLIPBOARDDATA)UserCreateObject(gHandleTable,
                                                      NULL,
                                                      NULL,
                                                      &hMem,
                                                      TYPE_CLIPDATA,
                                                      sizeof(CLIPBOARDDATA) + cjDataSize);
    if (!pClipboardData)
    {
        goto cleanup;
    }

    /* Set the data size */
    pClipboardData->cbData = cjDataSize;

    /* Copy the BITMAPINFOHEADER */
    memcpy(pClipboardData->Data, pbmi, sizeof(BITMAPINFOHEADER));

    /* Get the bitmap bits and the color table */
    iResult = GreGetDIBitsInternal(hdc,
                                   hbm,
                                   0,
                                   abs(pbmi->bmiHeader.biHeight),
                                   (LPBYTE)pClipboardData->Data + cjInfoSize,
                                   (LPBITMAPINFO)pClipboardData->Data,
                                   DIB_RGB_COLORS,
                                   pbmi->bmiHeader.biSizeImage,
                                   cjInfoSize);

    /* Add the clipboard data */
    IntAddFormatedData(pWinStaObj, CF_DIB, hMem, TRUE, TRUE);

    /* Release the extra reference (UserCreateObject added 2 references) */
    UserDereferenceObject(pClipboardData);

cleanup:
    UserReleaseDC(NULL, hdc, FALSE);
}

static VOID WINAPI
IntSynthesizeBitmap(PWINSTATION_OBJECT pWinStaObj, PCLIP pBmEl)
{
    HDC hdc = NULL;
    PBITMAPINFO pBmi, pConvertedBmi = NULL;
    HBITMAP hBm = NULL;
    PCLIPBOARDDATA pMemObj;
    PCLIP pDibEl;
    ULONG Offset;

    TRACE("IntSynthesizeBitmap(%p, %p)\n", pWinStaObj, pBmEl);

    pDibEl = IntGetFormatElement(pWinStaObj, CF_DIB);
    ASSERT(pDibEl && !IS_DATA_SYNTHESIZED(pDibEl));
    if (!pDibEl->fGlobalHandle)
        return;

    pMemObj = (PCLIPBOARDDATA)UserGetObject(gHandleTable, pDibEl->hData, TYPE_CLIPDATA);
    if (!pMemObj)
        return;

    pBmi = (BITMAPINFO*)pMemObj->Data;

    if (pMemObj->cbData < sizeof(DWORD) && pMemObj->cbData < pBmi->bmiHeader.biSize)
        goto cleanup;

    pConvertedBmi = DIB_ConvertBitmapInfo(pBmi, DIB_RGB_COLORS);
    if (!pConvertedBmi)
        goto cleanup;

    Offset = DIB_BitmapInfoSize(pBmi, DIB_RGB_COLORS);

    hdc = UserGetDCEx(NULL, NULL, DCX_USESTYLE);
    if (!hdc)
        goto cleanup;

    hBm = GreCreateDIBitmapInternal(hdc,
                                    pConvertedBmi->bmiHeader.biWidth,
                                    pConvertedBmi->bmiHeader.biHeight,
                                    CBM_INIT,
                                    pMemObj->Data + Offset,
                                    pConvertedBmi,
                                    DIB_RGB_COLORS,
                                    0,
                                    pMemObj->cbData - Offset,
                                    0);

    if (hBm)
    {
        GreSetObjectOwner(hBm, GDI_OBJ_HMGR_PUBLIC);
        pBmEl->hData = hBm;
    }

cleanup:
    if (hdc)
        UserReleaseDC(NULL, hdc, FALSE);

    if (pConvertedBmi)
        DIB_FreeConvertedBitmapInfo(pConvertedBmi, pBmi, -1);
}

static VOID NTAPI
IntAddSynthesizedFormats(PWINSTATION_OBJECT pWinStaObj)
{
    BOOL bHaveText, bHaveUniText, bHaveOemText, bHaveLocale, bHaveBm, bHaveDib, bHaveMFP, bHaveEMF;

    bHaveText = IntIsFormatAvailable(pWinStaObj, CF_TEXT);
    bHaveOemText = IntIsFormatAvailable(pWinStaObj, CF_OEMTEXT);
    bHaveUniText = IntIsFormatAvailable(pWinStaObj, CF_UNICODETEXT);
    bHaveLocale = IntIsFormatAvailable(pWinStaObj, CF_LOCALE);
    bHaveBm = IntIsFormatAvailable(pWinStaObj, CF_BITMAP);
    bHaveDib = IntIsFormatAvailable(pWinStaObj, CF_DIB);
    bHaveMFP = IntIsFormatAvailable(pWinStaObj, CF_METAFILEPICT);
    bHaveEMF = IntIsFormatAvailable(pWinStaObj, CF_ENHMETAFILE);

    /* Add CF_LOCALE format if we have CF_TEXT, CF_OEMTEXT or CF_UNICODETEXT */
    if (!bHaveLocale && (bHaveText || bHaveOemText || bHaveUniText))
    {
        PCLIPBOARDDATA pMemObj;
        HANDLE hMem;

        pMemObj = (PCLIPBOARDDATA)UserCreateObject(gHandleTable, NULL, NULL, &hMem, TYPE_CLIPDATA,
                                                   sizeof(CLIPBOARDDATA) + sizeof(LCID));
        if (pMemObj)
        {
            pMemObj->cbData = sizeof(LCID);
            *((LCID*)pMemObj->Data) = NtCurrentTeb()->CurrentLocale;
            IntAddFormatedData(pWinStaObj, CF_LOCALE, hMem, TRUE, TRUE);

            /* Release the extra reference (UserCreateObject added 2 references) */
            UserDereferenceObject(pMemObj);
        }
    }

    /* Add CF_TEXT. Note: it is synthesized in user32.dll */
    if (!bHaveText && (bHaveUniText || bHaveOemText))
        IntAddFormatedData(pWinStaObj, CF_TEXT, DATA_SYNTH_USER, FALSE, TRUE);

    /* Add CF_OEMTEXT. Note: it is synthesized in user32.dll */
    if (!bHaveOemText && (bHaveUniText || bHaveText))
        IntAddFormatedData(pWinStaObj, CF_OEMTEXT, DATA_SYNTH_USER, FALSE, TRUE);

    /* Add CF_UNICODETEXT. Note: it is synthesized in user32.dll */
    if (!bHaveUniText && (bHaveText || bHaveOemText))
        IntAddFormatedData(pWinStaObj, CF_UNICODETEXT, DATA_SYNTH_USER, FALSE, TRUE);

    /* Add CF_BITMAP. Note: it is synthesized on demand */
    if (!bHaveBm && bHaveDib)
        IntAddFormatedData(pWinStaObj, CF_BITMAP, DATA_SYNTH_KRNL, FALSE, TRUE);

    /* Add CF_ENHMETAFILE. Note: it is synthesized in gdi32.dll */
    if (bHaveMFP && !bHaveEMF)
        IntAddFormatedData(pWinStaObj, CF_ENHMETAFILE, DATA_SYNTH_USER, FALSE, TRUE);

    /* Add CF_METAFILEPICT. Note: it is synthesized in gdi32.dll */
    if (bHaveEMF && !bHaveMFP)
        IntAddFormatedData(pWinStaObj, CF_METAFILEPICT, DATA_SYNTH_USER, FALSE, TRUE);

    /* Note: We need to render the DIB or DIBV5 format as soon as possible
       because palette information may change */
    if (!bHaveDib && bHaveBm)
        IntSynthesizeDib(pWinStaObj, IntGetFormatElement(pWinStaObj, CF_BITMAP)->hData);
}

VOID NTAPI
UserEmptyClipboardData(PWINSTATION_OBJECT pWinSta)
{
    DWORD i;
    PCLIP pElement;

    for (i = 0; i < pWinSta->cNumClipFormats; ++i)
    {
        pElement = &pWinSta->pClipBase[i];
        IntFreeElementData(pElement);
    }

    if (pWinSta->pClipBase)
        ExFreePoolWithTag(pWinSta->pClipBase, USERTAG_CLIPBOARD);

    pWinSta->pClipBase = NULL;
    pWinSta->cNumClipFormats = 0;
}

/* UserClipboardRelease is called from IntSendDestroyMsg in window.c */
VOID FASTCALL
UserClipboardRelease(PWND pWindow)
{
    PWINSTATION_OBJECT pWinStaObj;

    if (!pWindow)
        return;

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        return;

    co_IntSendMessage(UserHMGetHandle(pWinStaObj->spwndClipOwner), WM_RENDERALLFORMATS, 0, 0);

    /* If the window being destroyed is the current clipboard owner... */
    if (pWindow == pWinStaObj->spwndClipOwner)
    {
        /* ... make it release the clipboard */
        pWinStaObj->spwndClipOwner = NULL;
    }

    if (pWinStaObj->fClipboardChanged)
    {
        /* Add synthesized formats - they are rendered later */
        IntAddSynthesizedFormats(pWinStaObj);

        /* Notify viewer windows in chain */
        pWinStaObj->fClipboardChanged = FALSE;
        if (pWinStaObj->spwndClipViewer)
        {
            TRACE("Clipboard: sending WM_DRAWCLIPBOARD to %p\n", UserHMGetHandle(pWinStaObj->spwndClipViewer));
            // For 32-bit applications this message is sent as a notification
            co_IntSendMessageNoWait(UserHMGetHandle(pWinStaObj->spwndClipViewer), WM_DRAWCLIPBOARD, 0, 0);
        }
    }

    ObDereferenceObject(pWinStaObj);
}

/* UserClipboardFreeWindow is called from co_UserFreeWindow in window.c */
VOID FASTCALL
UserClipboardFreeWindow(PWND pWindow)
{
    PWINSTATION_OBJECT pWinStaObj;

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        return;

    if (pWindow == pWinStaObj->spwndClipOwner)
    {
        /* The owner window was destroyed */
        pWinStaObj->spwndClipOwner = NULL;
    }

    /* Check if clipboard is not locked by this window, if yes, unlock it */
    if (pWindow == pWinStaObj->spwndClipOpen)
    {
        /* The window that opens the clipboard was destroyed */
        pWinStaObj->spwndClipOpen = NULL;
        pWinStaObj->ptiClipLock = NULL;
    }
    /* Remove window from window chain */
    if (pWindow == pWinStaObj->spwndClipViewer)
        pWinStaObj->spwndClipViewer = NULL;

    ObDereferenceObject(pWinStaObj);
}

UINT APIENTRY
UserEnumClipboardFormats(UINT fmt)
{
    UINT Ret = 0;
    PCLIP pElement;
    PWINSTATION_OBJECT pWinStaObj;

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    /* Check if the clipboard has been opened */
    if (!IntIsClipboardOpenByMe(pWinStaObj))
    {
        EngSetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        goto cleanup;
    }

    if (fmt == 0)
    {
        /* Return first format */
        if (pWinStaObj->pClipBase)
            Ret = pWinStaObj->pClipBase[0].fmt;
    }
    else
    {
        /* Return next format */
        pElement = IntGetFormatElement(pWinStaObj, fmt);
        if (pElement != NULL)
        {
            ++pElement;
            if (pElement < &pWinStaObj->pClipBase[pWinStaObj->cNumClipFormats])
            {
                Ret = pElement->fmt;
            }
        }
    }

cleanup:
    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    return Ret;
}

BOOL NTAPI
UserOpenClipboard(HWND hWnd)
{
    PWND pWindow = NULL;
    BOOL bRet = FALSE;
    PWINSTATION_OBJECT pWinStaObj = NULL;

    if (hWnd)
    {
        pWindow = UserGetWindowObject(hWnd);
        if (!pWindow)
            goto cleanup;
    }

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    /* Check if we already opened the clipboard */
    if ((pWindow == pWinStaObj->spwndClipOpen) && IntIsClipboardOpenByMe(pWinStaObj))
    {
        bRet = TRUE;
        goto cleanup;
    }

    /* If the clipboard was already opened by somebody else, bail out */
    if ((pWindow != pWinStaObj->spwndClipOpen) && pWinStaObj->ptiClipLock)
    {
        ERR("Access denied!\n");
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto cleanup;
    }

    /* Open the clipboard */
    pWinStaObj->spwndClipOpen = pWindow;
    pWinStaObj->ptiClipLock = PsGetCurrentThreadWin32Thread();
    bRet = TRUE;

cleanup:
    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    return bRet;
}

BOOL APIENTRY
NtUserOpenClipboard(HWND hWnd, DWORD Unknown1)
{
    BOOL bRet;

    UserEnterExclusive();
    bRet = UserOpenClipboard(hWnd);
    UserLeave();

    return bRet;
}

BOOL NTAPI
UserCloseClipboard(VOID)
{
    BOOL bRet = FALSE;
    PWINSTATION_OBJECT pWinStaObj;

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    /* Check if the clipboard has been opened */
    if (!IntIsClipboardOpenByMe(pWinStaObj))
    {
        EngSetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        goto cleanup;
    }

    /* Clipboard is no longer open */
    pWinStaObj->spwndClipOpen = NULL;
    pWinStaObj->ptiClipLock = NULL;
    bRet = TRUE;

    if (pWinStaObj->fClipboardChanged)
    {
        /* Add synthesized formats - they are rendered later */
        IntAddSynthesizedFormats(pWinStaObj);

        /* Notify viewer windows in chain */
        pWinStaObj->fClipboardChanged = FALSE;
        if (pWinStaObj->spwndClipViewer)
        {
            TRACE("Clipboard: sending WM_DRAWCLIPBOARD to %p\n", UserHMGetHandle(pWinStaObj->spwndClipViewer));
            // For 32-bit applications this message is sent as a notification
            co_IntSendMessageNoWait(UserHMGetHandle(pWinStaObj->spwndClipViewer), WM_DRAWCLIPBOARD, 0, 0);
        }
    }

cleanup:
    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    return bRet;
}

BOOL APIENTRY
NtUserCloseClipboard(VOID)
{
    BOOL bRet;

    UserEnterExclusive();
    bRet = UserCloseClipboard();
    UserLeave();

    return bRet;
}

HWND APIENTRY
NtUserGetOpenClipboardWindow(VOID)
{
    HWND hWnd = NULL;
    PWINSTATION_OBJECT pWinStaObj;

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    if (pWinStaObj->spwndClipOpen)
        hWnd = UserHMGetHandle(pWinStaObj->spwndClipOpen);

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return hWnd;
}

BOOL APIENTRY
NtUserChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext)
{
    BOOL bRet = FALSE;
    PWND pWindowRemove;
    PWINSTATION_OBJECT pWinStaObj;

    TRACE("NtUserChangeClipboardChain(%p, %p)\n", hWndRemove, hWndNewNext);

    UserEnterExclusive();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    pWindowRemove = UserGetWindowObject(hWndRemove);

    if (pWindowRemove && pWinStaObj->spwndClipViewer)
    {
        if (pWindowRemove == pWinStaObj->spwndClipViewer)
            pWinStaObj->spwndClipViewer = UserGetWindowObject(hWndNewNext);

        if (pWinStaObj->spwndClipViewer)
            bRet = (BOOL)co_IntSendMessage(UserHMGetHandle(pWinStaObj->spwndClipViewer), WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndNewNext);
    }

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return bRet;
}

DWORD APIENTRY
NtUserCountClipboardFormats(VOID)
{
    DWORD cFormats = 0;
    PWINSTATION_OBJECT pWinStaObj;

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    cFormats = pWinStaObj->cNumClipFormats;

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return cFormats;
}

BOOL NTAPI
UserEmptyClipboard(VOID)
{
    BOOL bRet = FALSE;
    PWINSTATION_OBJECT pWinStaObj;

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        return FALSE;

    /* Check if the clipboard has been opened */
    if (!IntIsClipboardOpenByMe(pWinStaObj))
    {
        EngSetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        goto cleanup;
    }

    UserEmptyClipboardData(pWinStaObj);

    if (pWinStaObj->spwndClipOwner)
    {
        TRACE("Clipboard: WM_DESTROYCLIPBOARD to %p\n", UserHMGetHandle(pWinStaObj->spwndClipOwner));
        // For 32-bit applications this message is sent as a notification
        co_IntSendMessage(UserHMGetHandle(pWinStaObj->spwndClipOwner), WM_DESTROYCLIPBOARD, 0, 0);
    }

    pWinStaObj->spwndClipOwner = pWinStaObj->spwndClipOpen;

    pWinStaObj->iClipSerialNumber++;
    pWinStaObj->iClipSequenceNumber++;
    pWinStaObj->fClipboardChanged = TRUE;
    pWinStaObj->fInDelayedRendering = FALSE;

    bRet = TRUE;

cleanup:
    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    return bRet;
}

BOOL APIENTRY
NtUserEmptyClipboard(VOID)
{
    BOOL bRet;

    TRACE("NtUserEmptyClipboard()\n");

    UserEnterExclusive();
    bRet = UserEmptyClipboard();
    UserLeave();

    return bRet;
}

INT APIENTRY
NtUserGetClipboardFormatName(UINT fmt, LPWSTR lpszFormatName, INT cchMaxCount)
{
    INT iRet = 0;

    UserEnterShared();

    /* If the format is built-in we fail */
    if (fmt < 0xc000 || fmt > 0xffff)
    {
        /* Registetrated formats are >= 0xc000 */
        goto cleanup;
    }

    if (cchMaxCount < 1 || !lpszFormatName)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    _SEH2_TRY
    {
        ProbeForWrite(lpszFormatName, cchMaxCount * sizeof(WCHAR), 1);

        iRet = IntGetAtomName((RTL_ATOM)fmt,
                              lpszFormatName,
                              cchMaxCount * sizeof(WCHAR));
        iRet /= sizeof(WCHAR);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

cleanup:
    UserLeave();

    return iRet;
}

HWND APIENTRY
NtUserGetClipboardOwner(VOID)
{
    HWND hWnd = NULL;
    PWINSTATION_OBJECT pWinStaObj;

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    if (pWinStaObj->spwndClipOwner)
        hWnd = UserHMGetHandle(pWinStaObj->spwndClipOwner);

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return hWnd;
}

HWND APIENTRY
NtUserGetClipboardViewer(VOID)
{
    HWND hWnd = NULL;
    PWINSTATION_OBJECT pWinStaObj;

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    if (pWinStaObj->spwndClipViewer)
        hWnd = UserHMGetHandle(pWinStaObj->spwndClipViewer);

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return hWnd;
}

INT APIENTRY
NtUserGetPriorityClipboardFormat(UINT *paFormatPriorityList, INT cFormats)
{
    INT i, iRet = 0;
    PWINSTATION_OBJECT pWinStaObj;

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    if (pWinStaObj->pClipBase == NULL)
    {
        iRet = 0;
    }
    else
    {
        _SEH2_TRY
        {
            ProbeForRead(paFormatPriorityList, cFormats * sizeof(UINT), sizeof(UINT));

            iRet = -1;

            for (i = 0; i < cFormats; ++i)
            {
                if (IntIsFormatAvailable(pWinStaObj, paFormatPriorityList[i]))
                {
                    iRet = paFormatPriorityList[i];
                    break;
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return iRet;

}

BOOL APIENTRY
NtUserIsClipboardFormatAvailable(UINT fmt)
{
    BOOL bRet = FALSE;
    PWINSTATION_OBJECT pWinStaObj;

    TRACE("NtUserIsClipboardFormatAvailable(%x)\n", fmt);

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    if (IntIsFormatAvailable(pWinStaObj, fmt))
        bRet = TRUE;

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return bRet;
}

HANDLE APIENTRY
NtUserGetClipboardData(UINT fmt, PGETCLIPBDATA pgcd)
{
    HANDLE hRet = NULL;
    PCLIP pElement;
    PWINSTATION_OBJECT pWinStaObj;
    UINT uSourceFmt = fmt;

    TRACE("NtUserGetClipboardData(%x, %p)\n", fmt, pgcd);

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    /* Check if the clipboard has been opened */
    if (!IntIsClipboardOpenByMe(pWinStaObj))
    {
        EngSetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        goto cleanup;
    }

    pElement = IntGetFormatElement(pWinStaObj, fmt);
    if (!pElement)
        goto cleanup;

    if (IS_DATA_SYNTHESIZED(pElement))
    {
        /* Note: Data is synthesized in usermode */
        /* TODO: Add more formats */
        switch (fmt)
        {
            case CF_UNICODETEXT:
            case CF_TEXT:
            case CF_OEMTEXT:
                uSourceFmt = CF_UNICODETEXT;
                pElement = IntGetFormatElement(pWinStaObj, uSourceFmt);
                if (IS_DATA_SYNTHESIZED(pElement))
                {
                    uSourceFmt = CF_TEXT;
                    pElement = IntGetFormatElement(pWinStaObj, uSourceFmt);
                }
                if (IS_DATA_SYNTHESIZED(pElement))
                {
                    uSourceFmt = CF_OEMTEXT;
                    pElement = IntGetFormatElement(pWinStaObj, uSourceFmt);
                }
                break;

            case CF_BITMAP:
                IntSynthesizeBitmap(pWinStaObj, pElement);
                break;

            case CF_METAFILEPICT:
                uSourceFmt = CF_ENHMETAFILE;
                pElement = IntGetFormatElement(pWinStaObj, uSourceFmt);
                break;

            case CF_ENHMETAFILE:
                uSourceFmt = CF_METAFILEPICT;
                pElement = IntGetFormatElement(pWinStaObj, uSourceFmt);
                break;

            default:
                ASSERT(FALSE);
        }
    }

    if (pElement && IS_DATA_DELAYED(pElement) && pWinStaObj->spwndClipOwner)
    {
        /* Send WM_RENDERFORMAT message */
        pWinStaObj->fInDelayedRendering = TRUE;
        co_IntSendMessage(UserHMGetHandle(pWinStaObj->spwndClipOwner), WM_RENDERFORMAT, (WPARAM)uSourceFmt, 0);
        pWinStaObj->fInDelayedRendering = FALSE;

        /* Data should be in clipboard now */
        pElement = IntGetFormatElement(pWinStaObj, uSourceFmt);
    }

    if (!pElement || IS_DATA_DELAYED(pElement))
        goto cleanup;

    _SEH2_TRY
    {
        ProbeForWrite(pgcd, sizeof(*pgcd), 1);
        pgcd->uFmtRet = pElement->fmt;
        pgcd->fGlobalHandle = pElement->fGlobalHandle;

        /* Text and bitmap needs more data */
        if (fmt == CF_TEXT)
        {
            PCLIP pLocaleEl;

            pLocaleEl = IntGetFormatElement(pWinStaObj, CF_LOCALE);
            if (pLocaleEl && !IS_DATA_DELAYED(pLocaleEl))
                pgcd->hLocale = pLocaleEl->hData;
        }
        else if (fmt == CF_BITMAP)
        {
            PCLIP pPaletteEl;

            pPaletteEl = IntGetFormatElement(pWinStaObj, CF_PALETTE);
            if (pPaletteEl && !IS_DATA_DELAYED(pPaletteEl))
                pgcd->hPalette = pPaletteEl->hData;
        }

        hRet = pElement->hData;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

cleanup:
    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    UserLeave();

    TRACE("NtUserGetClipboardData returns %p\n", hRet);

    return hRet;
}

HANDLE NTAPI
UserSetClipboardData(UINT fmt, HANDLE hData, PSETCLIPBDATA scd)
{
    HANDLE hRet = NULL;
    PWINSTATION_OBJECT pWinStaObj;

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    if (!fmt || !pWinStaObj->ptiClipLock)
    {
        ERR("Access denied!\n");
        EngSetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        goto cleanup;
    }

    if (scd->fIncSerialNumber)
        pWinStaObj->iClipSerialNumber++;

    /* Is it a delayed rendering? */
    if (hData)
    {
        /* Is it a bitmap? */
        if (fmt == CF_BITMAP)
        {
            /* Make bitmap public */
            GreSetObjectOwner(hData, GDI_OBJ_HMGR_PUBLIC);
        }

        /* Save data in the clipboard */
        IntAddFormatedData(pWinStaObj, fmt, hData, scd->fGlobalHandle, FALSE);
        TRACE("hData stored\n");

        /* If the serial number was increased, increase also the sequence number */
        if (scd->fIncSerialNumber)
            pWinStaObj->iClipSequenceNumber++;

        pWinStaObj->fClipboardChanged = TRUE;

        /* Note: Synthesized formats are added in NtUserCloseClipboard */
    }
    else
    {
        /* This is a delayed rendering */
        IntAddFormatedData(pWinStaObj, fmt, DATA_DELAYED, FALSE, FALSE);
        TRACE("SetClipboardData delayed format: %u\n", fmt);
    }

    /* Return hData on success */
    hRet = hData;

cleanup:
    TRACE("NtUserSetClipboardData returns: %p\n", hRet);

    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    return hRet;
}

HANDLE APIENTRY
NtUserSetClipboardData(UINT fmt, HANDLE hData, PSETCLIPBDATA pUnsafeScd)
{
    SETCLIPBDATA scd;
    HANDLE hRet;

    TRACE("NtUserSetClipboardData(%x %p %p)\n", fmt, hData, pUnsafeScd);

    _SEH2_TRY
    {
        ProbeForRead(pUnsafeScd, sizeof(*pUnsafeScd), 1);
        RtlCopyMemory(&scd, pUnsafeScd, sizeof(scd));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return NULL;)
    }
    _SEH2_END

    UserEnterExclusive();

    /* Call internal function */
    hRet = UserSetClipboardData(fmt, hData, &scd);

    UserLeave();

    return hRet;
}

HWND APIENTRY
NtUserSetClipboardViewer(HWND hWndNewViewer)
{
    HWND hWndNext = NULL;
    PWINSTATION_OBJECT pWinStaObj;
    PWND pWindow;

    UserEnterExclusive();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    pWindow = UserGetWindowObject(hWndNewViewer);
    if (!pWindow)
    {
        EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
        goto cleanup;
    }

    /* Return previous viewer. New viever window should
       send messages to rest of the chain */
    if (pWinStaObj->spwndClipViewer)
        hWndNext = UserHMGetHandle(pWinStaObj->spwndClipViewer);

    /* Set new viewer window */
    pWinStaObj->spwndClipViewer = pWindow;

    /* Notify viewer windows in chain */
    pWinStaObj->fClipboardChanged = FALSE;
    if (pWinStaObj->spwndClipViewer)
    {
        TRACE("Clipboard: sending WM_DRAWCLIPBOARD to %p\n", UserHMGetHandle(pWinStaObj->spwndClipViewer));
        // For 32-bit applications this message is sent as a notification
        co_IntSendMessageNoWait(UserHMGetHandle(pWinStaObj->spwndClipViewer), WM_DRAWCLIPBOARD, 0, 0);
    }

cleanup:
    if (pWinStaObj)
        ObDereferenceObject(pWinStaObj);

    UserLeave();

    return hWndNext;
}

// Sequence number is incremented whenever the contents of the clipboard change
// or the clipboard is emptied. If clipboard rendering is delayed,
// the sequence number is not incremented until the changes are rendered.

DWORD APIENTRY
NtUserGetClipboardSequenceNumber(VOID)
{
    DWORD dwRet = 0;
    PWINSTATION_OBJECT pWinStaObj;

    UserEnterShared();

    pWinStaObj = IntGetWinStaForCbAccess();
    if (!pWinStaObj)
        goto cleanup;

    /* Get windowstation sequence number */
    dwRet = (DWORD)pWinStaObj->iClipSequenceNumber;

    ObDereferenceObject(pWinStaObj);

cleanup:
    UserLeave();

    return dwRet;
}

HANDLE APIENTRY
NtUserConvertMemHandle(
   PVOID pData,
   DWORD cbData)
{
    HANDLE hMem = NULL;
    PCLIPBOARDDATA pMemObj;

    UserEnterExclusive();

    /* Create Clipboard data object */
    pMemObj = UserCreateObject(gHandleTable, NULL, NULL, &hMem, TYPE_CLIPDATA, sizeof(CLIPBOARDDATA) + cbData);
    if (!pMemObj)
        goto cleanup;

    pMemObj->cbData = cbData;

    /* Copy data */
    _SEH2_TRY
    {
        ProbeForRead(pData, cbData, 1);
        memcpy(pMemObj->Data, pData, cbData);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        pMemObj = NULL;
    }
    _SEH2_END;

    /* Release the extra reference (UserCreateObject added 2 references) */
    UserDereferenceObject(pMemObj);

    /* If we failed to copy data, remove handle */
    if (!pMemObj)
    {
        UserDeleteObject(hMem, TYPE_CLIPDATA);
        hMem = NULL;
    }

cleanup:
    UserLeave();

    return hMem;
}

NTSTATUS APIENTRY
NtUserCreateLocalMemHandle(
   HANDLE hMem,
   PVOID pData,
   DWORD cbData,
   DWORD *pcbData)
{
    PCLIPBOARDDATA pMemObj;
    NTSTATUS Status = STATUS_SUCCESS;

    UserEnterShared();

    /* Get Clipboard data object */
    pMemObj = (PCLIPBOARDDATA)UserGetObject(gHandleTable, hMem, TYPE_CLIPDATA);
    if (!pMemObj)
    {
        Status = STATUS_INVALID_HANDLE;
        goto cleanup;
    }

    /* Don't overrun */
    if (cbData > pMemObj->cbData)
        cbData = pMemObj->cbData;

    /* Copy data to usermode */
    _SEH2_TRY
    {
        if (pcbData)
        {
            ProbeForWrite(pcbData, sizeof(*pcbData), 1);
            *pcbData = pMemObj->cbData;
        }

        ProbeForWrite(pData, cbData, 1);
        memcpy(pData, pMemObj->Data, cbData);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

cleanup:
    UserLeave();

    return Status;
}

/* EOF */
