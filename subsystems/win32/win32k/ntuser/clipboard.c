/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Clipboard routines
 * FILE:             subsys/win32k/ntuser/clipboard.c
 * PROGRAMER:        Filip Navara <xnavara@volny.cz>
 *                   Pablo Borobia <pborobia@gmail.com>
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define DATA_DELAYED_RENDER  0
#define DATA_SYNTHESIZED_RENDER -1

PTHREADINFO      ClipboardThread;
PTHREADINFO      ClipboardOwnerThread;
PWINDOW_OBJECT  ClipboardWindow;
PWINDOW_OBJECT  ClipboardViewerWindow;
PWINDOW_OBJECT  ClipboardOwnerWindow;
BOOL            sendDrawClipboardMsg;
BOOL            recentlySetClipboard;
BOOL            delayedRender;
UINT            lastEnumClipboardFormats;
DWORD           ClipboardSequenceNumber = 0;

PCLIPBOARDCHAINELEMENT WindowsChain = NULL;
PCLIPBOARDELEMENT      ClipboardData = NULL;

PCHAR synthesizedData;
DWORD synthesizedDataSize;


/*==============================================================*/

/* return the pointer to the prev window of the finded window,
   if NULL does not exists in the chain */
PCLIPBOARDCHAINELEMENT FASTCALL
IntIsWindowInChain(PWINDOW_OBJECT window)
{
    PCLIPBOARDCHAINELEMENT wce = WindowsChain;

    while (wce)
    {
        if (wce->window == window)
        {
            break;
        }
        wce = wce->next;
    }

    return wce;
}

VOID FASTCALL printChain()
{
    /*test*/
    PCLIPBOARDCHAINELEMENT wce2 = WindowsChain;
    while (wce2)
    {
        DPRINT1("chain: %p\n", wce2->window->hSelf);
        wce2 = wce2->next;
    }
}

/* the new window always have to be the first in the chain */
PCLIPBOARDCHAINELEMENT FASTCALL
IntAddWindowToChain(PWINDOW_OBJECT window)
{
    PCLIPBOARDCHAINELEMENT wce = NULL;

    if (!IntIsWindowInChain(window))
    {
        wce = WindowsChain;

        wce = ExAllocatePool(PagedPool, sizeof(CLIPBOARDCHAINELEMENT));
        if (wce == NULL)
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
            goto exit_addChain;
        }

        wce->window = window;
        wce->next = WindowsChain;

        WindowsChain = wce;

        //printChain();
    }
exit_addChain:

    /* return the next window to beremoved later */
    return wce;
}

PCLIPBOARDCHAINELEMENT FASTCALL
IntRemoveWindowFromChain(PWINDOW_OBJECT window)
{
    PCLIPBOARDCHAINELEMENT wce = WindowsChain;
	PCLIPBOARDCHAINELEMENT *link = &WindowsChain;

    if (IntIsWindowInChain(window))
    {
        while (wce != NULL)
        {
            if (wce->window == window)
            {
                *link = wce->next;
                break;
            }

            link = &wce->next;
            wce = wce->next;
        }

        //printChain();

        return wce;
    }
    else
    {
        return NULL;
    }
}


/*==============================================================*/
/* if format exists, returns a non zero value (pointing to format object) */
PCLIPBOARDELEMENT FASTCALL
intIsFormatAvailable(format)
{
    PCLIPBOARDELEMENT ret = NULL;
    PCLIPBOARDELEMENT ce = ClipboardData;

    while(ce)
    {
	    if (ce->format == format)
	    {
	        ret = ce;
	        break;
	    }
	    ce = ce->next;
    }
    return ret;
}

/* counts how many distinct format were are in the clipboard */
DWORD FASTCALL
IntCountClipboardFormats()
{
    DWORD ret = 0;
    PCLIPBOARDELEMENT ce = ClipboardData;

    while(ce)
    {
        ret++;
	    ce = ce->next;
    }
    return ret;
}

/* adds a new format and data to the clipboard */
PCLIPBOARDELEMENT FASTCALL
intAddFormatedData(UINT format, HANDLE hData, DWORD size)
{
    PCLIPBOARDELEMENT ce = NULL;

    ce = ExAllocatePool(PagedPool, sizeof(CLIPBOARDELEMENT));
    if (ce == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        ce->format = format;
        ce->size = size;
        ce->hData = hData;
        ce->next = ClipboardData;

        ClipboardData = ce;

        IntIncrementSequenceNumber();
    }

    return ce;
}

/* removes a format and its data from the clipboard */
BOOL FASTCALL
intRemoveFormatedData(UINT format)
{
    BOOL ret = FALSE;
    PCLIPBOARDELEMENT ce = ClipboardData;
    PCLIPBOARDELEMENT *link = &ClipboardData;

    if (intIsFormatAvailable(format))
    {
        while (ce != NULL)
        {
            if (ce->format == format)
            {
                *link = ce->next;
                break;
            }

            link = &ce->next;
            ce = ce->next;
        }

        if (ce->hData)
        {
            ExFreePool(ce->hData);
        }
        ExFreePool(ce);
        ret = TRUE;
    }

    return ret;
}

VOID FASTCALL
IntEmptyClipboardData()
{
    PCLIPBOARDELEMENT ce = ClipboardData;
    PCLIPBOARDELEMENT tmp;

    while(ce)
    {
        tmp = ce->next;
        ExFreePool(ce->hData);
	    ExFreePool(ce);
	    ce = tmp;
    }

    ClipboardData = NULL;
}

/*==============================================================*/

HANDLE FASTCALL
renderBITMAPfromDIB(LPBYTE hDIB)
{
    HDC hdc;
    HBITMAP hbitmap;
    unsigned int offset;
    BITMAPINFOHEADER *ih;

    //hdc = UserGetDCEx(NULL, NULL, DCX_USESTYLE);
    hdc = UserGetDCEx(ClipboardWindow, NULL, DCX_USESTYLE);

    ih = (BITMAPINFOHEADER *)hDIB;

    offset = sizeof(BITMAPINFOHEADER) + ((ih->biBitCount <= 8) ? (sizeof(RGBQUAD) * (1 << ih->biBitCount)) : 0);

    hbitmap = NtGdiCreateDIBitmapInternal(hdc,
                                          ih->biWidth,
                                          ih->biHeight,
                                          CBM_INIT,
                                          (LPBYTE)ih+offset,
                                          (LPBITMAPINFO)ih,
                                          DIB_RGB_COLORS,
                                          ih->biBitCount,
                                          ih->biSizeImage,
                                          0,
                                          0);
    //UserReleaseDC(NULL, hdc, FALSE);
    UserReleaseDC(ClipboardWindow, hdc, FALSE);

    return hbitmap;
}

BOOL FASTCALL
canSinthesize(UINT format)
{
    BOOL ret = FALSE;

    switch(format)
    {
        case CF_BITMAP:
        case CF_METAFILEPICT:
            ret = TRUE;
    }

    return ret;
}

/* returns the size of the sinthesized data */
DWORD FASTCALL
synthesizeData(UINT format)
{
    DWORD ret = 0;

    synthesizedData = NULL;
    synthesizedDataSize = 0;

    if (!canSinthesize(format))
    {
        return 0;
    }

    switch (format)
    {
        case CF_BITMAP:
        {
            break;
        }

        case CF_METAFILEPICT:
        {
            break;
        }
    }

    ret = 1;

    return ret;
}

VOID FASTCALL
freeSynthesizedData()
{
    ExFreePool(synthesizedData);
}

/*==============================================================*/

BOOL FASTCALL
intIsClipboardOpenByMe()
{
    /* check if we open the clipboard */
    if (ClipboardThread && ClipboardThread == PsGetCurrentThreadWin32Thread())
    {
        /* yes, we got a thread and its the same that opens the clipboard */
        return TRUE;

    }
    /* will fail if not thread (closed) or not open by me*/
    return FALSE;
}

/* IntClipboardFreeWindow it's called when a window was destroyed */
VOID FASTCALL
IntClipboardFreeWindow(PWINDOW_OBJECT window)
{
    /* called from co_UserFreeWindow in window.c */
    /* check if clipboard is not locked by this window, if yes, unlock it */
    if (ClipboardThread == PsGetCurrentThreadWin32Thread())
    {
        /* the window that opens the clipboard was destroyed */
        ClipboardThread = NULL;
        ClipboardWindow = NULL;
        //TODO: free clipboard
    }
    if (window == ClipboardOwnerWindow)
    {
        /* the owner window was destroyed */
        ClipboardOwnerWindow = NULL;
        ClipboardOwnerThread = NULL;
    }
    /* remove window from window chain */
    if (IntIsWindowInChain(window))
    {
		PCLIPBOARDCHAINELEMENT w = IntRemoveWindowFromChain(window);
		if (w)
		{
            ExFreePool(w);
		}
    }
}

BOOL STDCALL
NtUserOpenClipboard(HWND hWnd, DWORD Unknown1)
{

    PWINDOW_OBJECT Window;
    BOOL ret = FALSE;

    UserEnterExclusive();

    sendDrawClipboardMsg = FALSE;
    recentlySetClipboard = FALSE;

    if (ClipboardThread)
    {
        /* clipboard is already open */
        if (ClipboardThread == PsGetCurrentThreadWin32Thread())
        {
            if  (ClipboardOwnerWindow)
            {
                if (ClipboardOwnerWindow->hSelf == hWnd)
                {
                    ret = TRUE;
                }
            }
            else
            {
                 if (hWnd == NULL)
                 {
                    ret = TRUE;
                 }
            }
        }
    }
    else
    {

        if (hWnd != NULL)
        {
            Window = UserGetWindowObject(hWnd);

            if (Window != NULL)
            {
                ClipboardWindow =  Window;
                ClipboardThread = PsGetCurrentThreadWin32Thread();
                ret = TRUE;
            }
            else
            {
                ClipboardWindow = NULL;
                ClipboardThread = NULL;
                ClipboardOwnerWindow = NULL;
                ClipboardOwnerThread = NULL;
            }
        }
        else
        {
            ClipboardWindow =  NULL;
            ClipboardThread = PsGetCurrentThreadWin32Thread();
            ret = TRUE;
        }
    }

    UserLeave();

    return ret;
}

BOOL STDCALL
NtUserCloseClipboard(VOID)
{
    BOOL ret = FALSE;

    UserEnterExclusive();

    if (intIsClipboardOpenByMe())
    {
        ClipboardWindow = NULL;
        ClipboardThread = NULL;
        ret = TRUE;
    }
    else
    {
        SetLastWin32Error(ERROR_CLIPBOARD_NOT_OPEN);
    }

    recentlySetClipboard = FALSE;

    UserLeave();

    if (sendDrawClipboardMsg && WindowsChain)
    {
        /* only send message to the first window in the chain, then they'll do the chain */
        /* commented because it makes a crash in co_MsqSendMessage
        ASSERT(WindowsChain->window);
        ASSERT(WindowsChain->window->hSelf);
        DPRINT1("Clipboard: sending WM_DRAWCLIPBOARD to %p\n", WindowsChain->window->hSelf);
        co_IntSendMessage(WindowsChain->window->hSelf, WM_DRAWCLIPBOARD, 0, 0);
        */
    }

    return ret;
}

HWND STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
    HWND ret = NULL;

    UserEnterShared();

    if (ClipboardWindow)
    {
        ret = ClipboardWindow->hSelf;
    }

    UserLeave();

    return ret;
}

BOOL STDCALL
NtUserChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext)
{
    BOOL ret = FALSE;
    PCLIPBOARDCHAINELEMENT w = NULL;
    PWINDOW_OBJECT removeWindow;
    UserEnterExclusive();

    removeWindow = UserGetWindowObject(hWndRemove);

    if (removeWindow)
    {
        if ((ret = !!IntIsWindowInChain(removeWindow)))
        {
            w = IntRemoveWindowFromChain(removeWindow);
            if (w)
            {
                ExFreePool(w);
            }
        }
    }

    if (ret && WindowsChain)
    {
        // only send message to the first window in the chain,
        // then they do the chain

        /* WindowsChain->window may be NULL */
        LPARAM lparam = WindowsChain->window == NULL ? 0 : (LPARAM)WindowsChain->window->hSelf;
        DPRINT1("Message: WM_CHANGECBCHAIN to %p", WindowsChain->window->hSelf);
        co_IntSendMessage(WindowsChain->window->hSelf, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, lparam);
    }

    UserLeave();

    return ret;
}

DWORD STDCALL
NtUserCountClipboardFormats(VOID)
{
    DWORD ret = 0;

    if (ClipboardData)
    {
       ret = IntCountClipboardFormats();
    }

    return ret;
}

DWORD STDCALL
NtUserEmptyClipboard(VOID)
{
    BOOL ret = FALSE;

    UserEnterExclusive();

    if (intIsClipboardOpenByMe())
    {
        if (ClipboardData)
        {
            IntEmptyClipboardData();
        }

        ClipboardOwnerWindow = ClipboardWindow;
        ClipboardOwnerThread = ClipboardThread;

        IntIncrementSequenceNumber();

        ret = TRUE;
    }
    else
    {
        SetLastWin32Error(ERROR_CLIPBOARD_NOT_OPEN);
    }

    if (ret && ClipboardOwnerWindow)
    {
        DPRINT("Clipboard: WM_DESTROYCLIPBOARD to %p", ClipboardOwnerWindow->hSelf);
        co_IntSendMessage( ClipboardOwnerWindow->hSelf, WM_DESTROYCLIPBOARD, 0, 0);
    }

    UserLeave();

    return ret;
}

HANDLE STDCALL
NtUserGetClipboardData(UINT uFormat, PVOID pBuffer)
{
    HANDLE ret = NULL;

    UserEnterShared();

    if (intIsClipboardOpenByMe())
    {
        /* when Unknown1 is zero, we returns to user32 the data size */
        if (!pBuffer)
        {
            PCLIPBOARDELEMENT data = intIsFormatAvailable(uFormat);

            if (data)
            {
                /* format exists in clipboard */
                if (data->size == DATA_DELAYED_RENDER)
                {
                    /* tell owner what data needs to be rendered */
                    if (ClipboardOwnerWindow)
                    {
                        ASSERT(ClipboardOwnerWindow->hSelf);
                        co_IntSendMessage(ClipboardOwnerWindow->hSelf, WM_RENDERFORMAT, (WPARAM)uFormat, 0);
                        data = intIsFormatAvailable(uFormat);
                        ASSERT(data->size);
                        ret = (HANDLE)(ULONG_PTR)data->size;
                    }
                }
                else
                {
                    if (data->size == DATA_SYNTHESIZED_RENDER)
                    {
                        data->size = synthesizeData(uFormat);
                    }

                }
                ret = (HANDLE)(ULONG_PTR)data->size;
            }
            else
            {
                /* there is no data in this format */
                //ret = (HANDLE)FALSE;
            }
        }
        else
        {
            PCLIPBOARDELEMENT data = intIsFormatAvailable(uFormat);

            if (data)
            {
                if (data->size == DATA_DELAYED_RENDER)
                {
                    // we rendered it in 1st call of getclipboard data
                }
                else
                {
                    if (data->size == DATA_SYNTHESIZED_RENDER)
                    {
                        if (uFormat == CF_BITMAP)
                        {
                            /* BITMAP & METAFILEs returns a GDI handle */
                            PCLIPBOARDELEMENT data = intIsFormatAvailable(CF_DIB);
                            if (data)
                            {
                                ret = renderBITMAPfromDIB(data->hData);
                            }
                        }
                        else
                        {
                            ret = (HANDLE)pBuffer;

                            _SEH_TRY
                            {
                                ProbeForWrite(pBuffer, synthesizedDataSize, 1);
                                memcpy(pBuffer, (PCHAR)synthesizedData, synthesizedDataSize);
                            }
                            _SEH_HANDLE
                            {
                                ret = NULL;
                            }
                            _SEH_END

                            freeSynthesizedData();
                        }
                    }
                    else
                    {
                        ret = (HANDLE)pBuffer;

                        _SEH_TRY
                        {
                            ProbeForWrite(pBuffer, data->size, 1);
                            memcpy(pBuffer, (PCHAR)data->hData, data->size);
                        }
                        _SEH_HANDLE
                        {
                            ret = NULL;
                        }
                        _SEH_END
                    }
                }

            }

        }
    }
    else
    {
        SetLastWin32Error(ERROR_CLIPBOARD_NOT_OPEN);
    }

    UserLeave();

    return ret;
}

INT STDCALL
NtUserGetClipboardFormatName(UINT format, PUNICODE_STRING FormatName,
                             INT cchMaxCount)
{
    UNICODE_STRING sFormatName;
    INT ret = 0;

    /* if the format is built-in we fail */
    if (format < 0xc000)
    {
        /* registetrated formats are >= 0xc000 */
        return 0;
    }

    if((cchMaxCount < 1) || !FormatName)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    _SEH_TRY
    {
        ProbeForWriteUnicodeString(FormatName);
        sFormatName = *(volatile UNICODE_STRING *)FormatName;
        ProbeForWrite(sFormatName.Buffer, sFormatName.MaximumLength, 1);

        ret = IntGetAtomName((RTL_ATOM)format, sFormatName.Buffer, cchMaxCount * sizeof(WCHAR));

        if (ret >= 0)
        {
            ret = ret / sizeof(WCHAR);
            sFormatName.Length = ret;
        }
        else
        {
            ret = 0;
        }
    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
    }
    _SEH_END;

    return ret;
}

HWND STDCALL
NtUserGetClipboardOwner(VOID)
{
    HWND ret = NULL;

    UserEnterShared();

    if (ClipboardOwnerWindow)
    {
        ret = ClipboardOwnerWindow->hSelf;
    }

    UserLeave();

    return ret;
}

HWND STDCALL
NtUserGetClipboardViewer(VOID)
{
    HWND ret = NULL;

    UserEnterShared();

    if (WindowsChain)
    {
        ret = WindowsChain->window->hSelf;
    }

    UserLeave();

    return ret;
}

INT STDCALL
NtUserGetPriorityClipboardFormat(UINT *paFormatPriorityList, INT cFormats)
{
    INT i;
    UINT *priorityList;
    INT ret = 0;

    UserEnterExclusive();

    _SEH_TRY
    {
        if (IntCountClipboardFormats() == 0)
        {
            ret = 0;
        }
        else
        {
            ProbeForRead(paFormatPriorityList, cFormats, sizeof(UINT));

            priorityList = paFormatPriorityList;

            ret = -1;

            for (i = 0; i < cFormats; i++)
            {
                if (intIsFormatAvailable(priorityList[i]))
                {
                    ret = priorityList[i];
                    break;
                }
            }

        }
    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
    }
    _SEH_END;

    UserLeave();

    return ret;

}

BOOL STDCALL
NtUserIsClipboardFormatAvailable(UINT format)
{
    BOOL ret = FALSE;

    UserEnterShared();

    ret = (intIsFormatAvailable(format) != NULL);

    UserLeave();

    return ret;
}



HANDLE STDCALL
NtUserSetClipboardData(UINT uFormat, HANDLE hMem, DWORD size)
{
    HANDLE hCBData = NULL;
    UNICODE_STRING unicodeString;
    OEM_STRING oemString;
    ANSI_STRING ansiString;

    UserEnterExclusive();

    /* to place data here the we need to be the owner */
    if (ClipboardOwnerThread == PsGetCurrentThreadWin32Thread())
    {
        PCLIPBOARDELEMENT data = intIsFormatAvailable(uFormat);
        if (data)
        {

            if (data->size == DATA_DELAYED_RENDER)
            {
                intRemoveFormatedData(uFormat);
            }
            else
            {
                // we already have this format on clipboard
                goto exit_setCB;
            }
        }

        if (hMem)
        {
            _SEH_TRY
            {
                ProbeForRead(hMem, size, 1);
            }
            _SEH_HANDLE
            {
                SetLastNtError(_SEH_GetExceptionCode());
                _SEH_YIELD(goto exit_setCB);
            }
            _SEH_END;

            if (intIsClipboardOpenByMe())
            {
                delayedRender = FALSE;
            }

            if (!canSinthesize(uFormat))
            {
                hCBData = ExAllocatePool(PagedPool, size);
                memcpy(hCBData, hMem, size);
                intAddFormatedData(uFormat, hCBData, size);
                DPRINT1("Data stored\n");
            }

            sendDrawClipboardMsg = TRUE;
            recentlySetClipboard = TRUE;
            lastEnumClipboardFormats = uFormat;

            /* conversions */
            switch (uFormat)
            {
                case CF_TEXT:
                    {
                        //TODO : sinthesize CF_UNICODETEXT & CF_OEMTEXT
                        // CF_TEXT -> CF_UNICODETEXT
                        ansiString.Buffer = hCBData;
                        ansiString.Length = size;
                        RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
                        intAddFormatedData(CF_UNICODETEXT, unicodeString.Buffer, unicodeString.Length * sizeof(WCHAR));
                        // CF_TEXT -> CF_OEMTEXT
                        RtlUnicodeStringToOemString(&oemString, &unicodeString, TRUE);
                        intAddFormatedData(CF_OEMTEXT, oemString.Buffer, oemString.Length);
                        //HKCU\Control Panel\International\Locale
                        //intAddFormatedData(CF_LOCALE, oemString.Buffer, oemString.Length);
                        break;
                    }
                case CF_UNICODETEXT:
                {
                    //TODO : sinthesize CF_TEXT & CF_OEMTEXT
                    //CF_UNICODETEXT -> CF_TEXT
                    unicodeString.Buffer = hCBData;
                    unicodeString.Length = size;
                    RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, TRUE);
                    intAddFormatedData(CF_TEXT, ansiString.Buffer, ansiString.Length);
                    //CF_UNICODETEXT -> CF_OEMTEXT
                    RtlUnicodeStringToOemString(&oemString, &unicodeString, TRUE);
                    intAddFormatedData(CF_OEMTEXT, oemString.Buffer, oemString.Length);
                    break;
                }
                case CF_OEMTEXT:
                {
                    //TODO : sinthesize CF_TEXT & CF_UNICODETEXT
                    //CF_OEMTEXT -> CF_UNICODETEXT
                    oemString.Buffer = hCBData;
                    oemString.Length = size;
                    RtlOemStringToUnicodeString(&unicodeString, &oemString, TRUE);
                    intAddFormatedData(CF_UNICODETEXT, unicodeString.Buffer, unicodeString.Length * sizeof(WCHAR));
                    //CF_OEMTEXT -> CF_TEXT
                    RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, TRUE);
                    intAddFormatedData(CF_TEXT, ansiString.Buffer, ansiString.Length);
                    break;
                }
                case CF_BITMAP:
                {
                    // we need to render the DIB or DIBV5 format as soon as possible
                    // because pallette information may change

                    HDC hdc;
                    INT ret;
                    BITMAP bm;
                    BITMAPINFO bi;
                    BITMAPOBJ *BitmapObj;

                    hdc = UserGetDCEx(NULL, NULL, DCX_USESTYLE);


                    BitmapObj = BITMAPOBJ_LockBitmap(hMem);
                    BITMAP_GetObject(BitmapObj, sizeof(BITMAP), (PVOID)&bm);
                    if(BitmapObj)
                    {
                        BITMAPOBJ_UnlockBitmap(BitmapObj);
                    }

                    bi.bmiHeader.biSize	= sizeof(BITMAPINFOHEADER);
                    bi.bmiHeader.biWidth = bm.bmWidth;
                    bi.bmiHeader.biHeight = bm.bmHeight;
                    bi.bmiHeader.biPlanes = 1;
                    bi.bmiHeader.biBitCount	= bm.bmPlanes * bm.bmBitsPixel;
                    bi.bmiHeader.biCompression = BI_RGB;
                    bi.bmiHeader.biSizeImage = 0;
                    bi.bmiHeader.biXPelsPerMeter = 0;
                    bi.bmiHeader.biYPelsPerMeter = 0;
                    bi.bmiHeader.biClrUsed = 0;

                    ret = NtGdiGetDIBitsInternal(hdc, hMem, 0, bm.bmHeight,  NULL, &bi, DIB_RGB_COLORS, 0, 0);

                    size = bi.bmiHeader.biSizeImage + sizeof(BITMAPINFOHEADER);

                    hCBData = ExAllocatePool(PagedPool, size);
                    memcpy(hCBData, &bi, sizeof(BITMAPINFOHEADER));

                    ret = NtGdiGetDIBitsInternal(hdc, hMem, 0, bm.bmHeight, (LPBYTE)hCBData + sizeof(BITMAPINFOHEADER), &bi, DIB_RGB_COLORS, 0, 0);

                    UserReleaseDC(NULL, hdc, FALSE);

                    intAddFormatedData(CF_DIB, hCBData, size);
                    intAddFormatedData(CF_BITMAP, 0, DATA_SYNTHESIZED_RENDER);
                    // intAddFormatedData(CF_DIBV5, hCBData, size);

                    break;
                }
                case CF_DIB:
                {
                    intAddFormatedData(CF_BITMAP, 0, DATA_SYNTHESIZED_RENDER);
                    // intAddFormatedData(CF_DIBV5, hCBData, size);
                    /* investigate */
                    // intAddFormatedData(CF_PALETTE, hCBData, size);
                    break;
                }
                case CF_DIBV5:
                    // intAddFormatedData(CF_BITMAP, hCBData, size);
                    // intAddFormatedData(CF_PALETTE, hCBData, size);
                    // intAddFormatedData(CF_DIB, hCBData, size);
                    break;
                case CF_ENHMETAFILE:
                    // intAddFormatedData(CF_METAFILEPICT, hCBData, size);
                    break;
                case CF_METAFILEPICT:
                    // intAddFormatedData(CF_ENHMETAFILE, hCBData, size);
                    break;
            }

        }
        else
        {
            // the window provides data in the specified format
            delayedRender = TRUE;
            sendDrawClipboardMsg = TRUE;
            intAddFormatedData(uFormat, NULL, 0);
            DPRINT1("SetClipboardData delayed format: %d\n", uFormat);
        }


    }

exit_setCB:

    UserLeave();

    return hMem;
}

HWND STDCALL
NtUserSetClipboardViewer(HWND hWndNewViewer)
{
    HWND ret = NULL;
    PCLIPBOARDCHAINELEMENT newWC = NULL;
    PWINDOW_OBJECT window;

    UserEnterExclusive();

    window = UserGetWindowObject(hWndNewViewer);

    if (window)
    {
        if ((newWC = IntAddWindowToChain(window)))
        {
            if (newWC)
            {
                // newWC->next may be NULL if we are the first window in the chain
                if (newWC->next)
                {
                    // return the next HWND available window in the chain
                    ret = newWC->next->window->hSelf;
                }
            }
        }
    }

    UserLeave();

    return ret;
}

UINT STDCALL
IntEnumClipboardFormats(UINT uFormat)
{
    UINT ret = 0;

    if (intIsClipboardOpenByMe())
    {
            if (uFormat == 0)
            {
                if (recentlySetClipboard)
                {
                    ret = lastEnumClipboardFormats;
                }
                else
                {
                    /* return the first available format */
                    if (ClipboardData)
                    {
                        ret = ClipboardData->format;
                    }
                }
            }
            else
            {
                if (recentlySetClipboard)
                {
                    ret = 0;
                }
                else
                {
                    /* querying nextt available format */
                    PCLIPBOARDELEMENT data = intIsFormatAvailable(uFormat);

                    if (data)
                    {
                        if (data->next)
                        {
                            ret = data->next->format;
                        }
                        else
                        {
                            /* reached the end */
                            ret = 0;
                        }
                    }
                }

            }
    }
    else
    {
        SetLastWin32Error(ERROR_CLIPBOARD_NOT_OPEN);
    }

    return ret;
}

// This number is incremented whenever the contents of the clipboard change
// or the clipboard is emptied.
// If clipboard rendering is delayed,
// the sequence number is not incremented until the changes are rendered.
VOID FASTCALL
IntIncrementSequenceNumber(VOID)
{
    PTHREADINFO pti;
    PWINSTATION_OBJECT WinStaObj;

    pti = PsGetCurrentThreadWin32Thread();
    WinStaObj = pti->Desktop->WindowStation;

    WinStaObj->Clipboard->ClipboardSequenceNumber++;
}

DWORD STDCALL
NtUserGetClipboardSequenceNumber(VOID)
{
    //windowstation sequence number
    //if no WINSTA_ACCESSCLIPBOARD access to the window station,
    //the function returns zero.
    DWORD sn;

    HWINSTA WinSta;
    PWINSTATION_OBJECT WinStaObj;
    NTSTATUS Status;

    WinSta = UserGetProcessWindowStation();

    Status = IntValidateWindowStationHandle(WinSta, UserMode, WINSTA_ACCESSCLIPBOARD, &WinStaObj);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No WINSTA_ACCESSCLIPBOARD access\n");
        SetLastNtError(Status);
        return 0;
    }

    sn = WinStaObj->ClipboardSequenceNumber;

    ObDereferenceObject(WinStaObj);

    //local copy
    //sn = ClipboardSequenceNumber;

    return sn;
}


/**************** VISTA FUNCTIONS******************/

BOOL STDCALL NtUserAddClipboardFormatListener(
    HWND hwnd
)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL STDCALL NtUserRemoveClipboardFormatListener(
    HWND hwnd
)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL STDCALL NtUserGetUpdatedClipboardFormats(
    PUINT lpuiFormats,
    UINT cFormats,
    PUINT pcFormatsOut
)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
