#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"

typedef struct tagPREVPRINTER
{
    TCHAR szPrinterName[MAXNAMELENBUFFER];
    HWND hwndStub;
} PREVPRINTER, *LPPREVPRINTER;


int FindPrinter(HDSA hPrevPrinters, LPCTSTR lpszPrinterName)
{
    int i = -1;

    if (hPrevPrinters)
    {
        for (i=DSA_GetItemCount(hPrevPrinters)-1; i>=0; --i)
        {
            LPPREVPRINTER pPrevPrinter = DSA_GetItemPtr(hPrevPrinters, i);
            if (lstrcmpi(pPrevPrinter->szPrinterName, lpszPrinterName) == 0)
            {
                break;
            }
        }
    }

    return(i);
}


//
// if uAction IS NOT MSP_NEWDRIVER then:
//    installs a printer (uAction).  If successful, notifies the shell and
//    returns a pidl to the printer.  ILFree() is callers responsibility.
// otherwise, if uAction IS MSP_NEWDRIVER then:
//    installs a printer driver (uAction).  If successful, fills the new
//    driver's name into lpBuffer (ASSUMED >= MAXNAMELEN).
//    Always returns NULL.
// if uAction is MSP_TESTPAGEPARTIALPROMPT then:
//    executes the test page code
//    Always returns NULL.
//

HWND hwndPrinterSetup = NULL; // active printer setup window, if any
LPITEMIDLIST Printers_PrinterSetup(HWND hwndStub, UINT uAction, LPTSTR lpBuffer, LPCTSTR pszServer)
{
#ifndef WINNT // PRINTQ
    HINSTANCE hmMSPrint;
    PRINTERSETUPPROC32 pfnPrinterSetup;
#endif
    LPITEMIDLIST pidl = NULL;

    //
    // HACK! This hack is related to BUG #272207
    // This function is called from Printers_DeletePrinter for
    // printer deletion and this case we should not check
    // for REST_NOPRINTERADD restriction.
    //
    // -LazarI
    //
    if (MSP_NEWPRINTER == uAction ||
        MSP_NETPRINTER == uAction ||
        MSP_NEWPRINTER_MODELESS == uAction)
    {
            if (SHIsRestricted(hwndStub, REST_NOPRINTERADD))
            {
                return NULL;
            }
    }

#ifndef WINNT
    //
    // Windows 95 permits a user to printer a test page from the help trouble 
    // shooter, with out any contextual information.  i.e. a printer has 
    // not been selected.  In this case they will prompt the user for a 
    // printer from a list of printers.  In Windows NT we do not support
    // printing a test page from the help trouble shooter.  
    //
    // In Windows NT we prevent multiple add printer wizards from within 
    // the bPrinterSetup API located in printui.dll, where as Windows 95
    // prevents multiple add printer wizards here using a global.
    // we only want one PrinterSetup window up at a time
    //
    if (uAction != MSP_TESTPAGEPARTIALPROMPT)
    {
        BOOL fRet = FALSE;
        HWND hwnd;

        ENTERCRITICAL;
        if (hwndPrinterSetup != NULL &&
            SetForegroundWindow(
                hwnd = GetLastActivePopup(hwndPrinterSetup) ))
        {
            fRet = TRUE;
        }
        else
        {
            hwndPrinterSetup = hwndStub;
        }
        LEAVECRITICAL;

        if (fRet)
        {
            // In the case of deleting a printer, it is very unclear
            // why the "install wizard" popped up into the user's face.
            // Put up a generic "you must complete this operation" message.
            ShellMessageBox(HINST_THISDLL, hwnd,
                MAKEINTRESOURCE(IDS_MUSTCOMPLETE), MAKEINTRESOURCE(IDS_PRINTERS), MB_OK);

            return NULL;
        }
    }
#endif

#if WINNT // PRINTQ
    if (1)
    {
        DWORD cchBufLen = 0;
#else
    hmMSPrint = LoadLibrary(TEXT("MSPRINT2.DLL"));

    if (!ISVALIDHINSTANCE(hmMSPrint))
    {
        DebugMsg(DM_WARNING,TEXT("sh WN - Printers_PrinterSetup could not load MSPRINT2.DLL"));
        ASSERT(FALSE);
        return NULL;
    }

    // BUGBUG: pop up a message box on error?

    pfnPrinterSetup = (PRINTERSETUPPROC32)GetProcAddress(hmMSPrint, "PrinterSetup32");
    if (pfnPrinterSetup)
    {
        WORD cchBufLen = 0;
#endif // PRINTQ
        LPIDPRINTER pidp;

        if (lpBuffer)
            cchBufLen = lstrlen(lpBuffer) + 1;
        if (cchBufLen < ARRAYSIZE(pidp->cName))
            cchBufLen = ARRAYSIZE(pidp->cName);

        pidp = (void*)LocalAlloc(LPTR, SIZEOF(IDPRINTER) - SIZEOF(pidp->cName) + cchBufLen * SIZEOF(TCHAR));
        if (pidp)
        {
            pidp->cName[0] = TEXT('\0');
            if (lpBuffer)
                lstrcpy(pidp->cName, lpBuffer);

            //
            // We don't have to worry about PrinterSetup failing due to the
            // output buffer being too small.  It's the right size
            // (32 bytes for Win9x, MAXNAMELENBUFFER for WinNT).
            //
            // On Win9x, this is ANSI, on WinNT, setup expects UNICODE.
            //
            // Also, there are no alignment problems since pidp is
            // allocated by us.
            //

#ifdef WINNT // PRINTQ

            if (bPrinterSetup(hwndStub,LOWORD(uAction),cchBufLen, pidp->cName,&cchBufLen, pszServer))
            {
#else
            if (pfnPrinterSetup(hwndStub,LOWORD(uAction),cchBufLen,
                (LPBYTE)pidp->cName,&cchBufLen))
            {
#endif
                if (uAction == MSP_NEWDRIVER)
                {
                    ASSERT(lstrlen(pidp->cName) < MAXNAMELEN);
                    lstrcpy(lpBuffer, (LPTSTR)(pidp->cName));
                }
                else if (uAction == MSP_TESTPAGEPARTIALPROMPT)
                {
                    // nothing to do for this case
                }
                else if (uAction == MSP_REMOVEPRINTER)
                {
                    // a bit ugly, but we need to pass back success for this case
                    pidl = (LPITEMIDLIST)TRUE;
                }
#ifdef WINNT
                else if (uAction == MSP_NEWPRINTER_MODELESS)
                {
                    // a bit ugly, but we need to pass back success for this case
                    pidl = (LPITEMIDLIST)TRUE;
                }
#endif
                else
                {
                    LPITEMIDLIST pidlParent = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
                    if (pidlParent)
                    {
                        pidp->cb = (USHORT)(FIELD_OFFSET(IDPRINTER,cName) + (lstrlen(pidp->cName) + 1) * SIZEOF(TCHAR));
                        *(USHORT *)((LPBYTE)(pidp) + pidp->cb) = 0;

                        pidl = ILCombine(pidlParent, (LPCITEMIDLIST)pidp);
                        ILFree(pidlParent);
                    }
                }
            }
            else
            {
                DebugMsg(DM_TRACE,TEXT("sh TR - PrinterSetup32() failed (%x)"), GetLastError());
            }

            LocalFree((HLOCAL)pidp);
        }
    }
    else
    {
        DebugMsg(DM_ERROR,TEXT("sh ER - GetProcAddress(MSPRINT32.DLL,PrinterSetup32) failed"));
    }

    hwndPrinterSetup = NULL;
#ifndef WINNT // PRINTQ
    FreeLibrary(hmMSPrint);
#endif
    return(pidl);
}

//
// Printer_OneWindowAction calls pfn iff it's not in use for printer lpName.
// If it's already in use for printer lpName, bring focus to that window.
//
void Printer_OneWindowAction(HWND hwndStub, LPCTSTR lpName, HDSA *lphdsa, LPFNPRINTACTION pfn, LPARAM lParam, BOOL fModal)
{
    int i;
    PREVPRINTER sThisPrinter;
    LPPREVPRINTER pPrevPrinter;

    EnterCriticalSection(&g_csPrinters);

    // Initialize the DSA if we need to
    if (!*lphdsa)
    {
        *lphdsa = DSA_Create(SIZEOF(sThisPrinter), 4);
        if (!*lphdsa)
        {
            goto error_exit;
        }
    }

    // Bring window up if lpName is in use
    i = FindPrinter(*lphdsa, lpName);
    if (i >= 0)
    {
        HWND hwnd;

        if (fModal) {
            ShellMessageBox(HINST_THISDLL,
                hwndStub,
                MAKEINTRESOURCE(IDS_CANTOPENMODALPROP),
                MAKEINTRESOURCE(IDS_PRINTERS),
                MB_OK|MB_ICONERROR);
        }

        pPrevPrinter = DSA_GetItemPtr(*lphdsa, i);

        if (pPrevPrinter->hwndStub == NULL ||
            (hwnd = GetLastActivePopup(pPrevPrinter->hwndStub)) == NULL ||
            !ShowWindow(hwnd, SW_SHOW) ||
            !SetForegroundWindow(hwnd) )
        {
            // The window must have crashed before.  Remove it from the
            // hdsa and add it again.
            // WARNING: This window could be stuck waiting for a
            // PRINTER_INFO_2 before putting up the window. In this case, the
            // ShowWindow will fail and we'll wind up with two windows up.
            DSA_DeleteItem(*lphdsa, i);
        }
        else
        {
            // We brought the existing window to the front.  We're done.

            goto exit;
        }
    }

    // We're bringing up the window
    lstrcpy(sThisPrinter.szPrinterName, lpName);
    sThisPrinter.hwndStub = hwndStub;
    if (DSA_AppendItem(*lphdsa, &sThisPrinter) < 0)
    {
        goto error_exit;
    }

    // Call the printer function.  Make sure winspool is loaded
    LeaveCriticalSection(&g_csPrinters);
    pfn(hwndStub, lpName, SW_SHOWNORMAL, lParam);
    EnterCriticalSection(&g_csPrinters);

    // The window is gone
    // (Use sThisPrinter.szPrinterName since it may have been renamed)
    i = FindPrinter(*lphdsa, sThisPrinter.szPrinterName);
    if (i >= 0)
    {
        DSA_DeleteItem(*lphdsa, i);

        // Don't use up memory we don't need
        if (DSA_GetItemCount(*lphdsa) == 0)
        {
            DSA_Destroy(*lphdsa);
            *lphdsa = NULL;
        }
    }

    goto exit;

error_exit:
    // BUGBUG: do we want to put up an Out Of Memory error?
    DebugMsg(DM_WARNING, TEXT("sh WN - Printer_OneWindowAction() - Out Of Memory"));

exit:
    LeaveCriticalSection(&g_csPrinters);
    return;
}


extern HDSA hdsaPrintDef;

void PrintDef_UpdateHwnd(LPCTSTR lpszPrinterName, HWND hWnd)
{
    int i;
    LPPREVPRINTER pThisPrinter;

    EnterCriticalSection(&g_csPrinters);

    if (!hdsaPrintDef)
    {
        goto Error0;
    }

    i = FindPrinter(hdsaPrintDef, lpszPrinterName);
    if (i < 0)
    {
        goto Error0;
    }

    pThisPrinter = DSA_GetItemPtr(hdsaPrintDef, i);
    pThisPrinter->hwndStub = hWnd;

Error0:

    LeaveCriticalSection(&g_csPrinters);

}

void PrintDef_UpdateName(LPCTSTR lpszPrinterName, LPCTSTR lpszNewName)
{
    int i;
    LPPREVPRINTER pThisPrinter;

    EnterCriticalSection(&g_csPrinters);

    if (!hdsaPrintDef)
    {
        goto Error0;
    }

    i = FindPrinter(hdsaPrintDef, lpszPrinterName);
    if (i < 0)
    {
        goto Error0;
    }

    pThisPrinter = DSA_GetItemPtr(hdsaPrintDef, i);
    lstrcpy(pThisPrinter->szPrinterName, lpszNewName);

Error0:

    LeaveCriticalSection(&g_csPrinters);

}


void PrintDef_RefreshQueue(LPCTSTR lpszPrinterName)
{
    int i;
    LPPREVPRINTER pThisPrinter;

    EnterCriticalSection(&g_csPrinters);

    if (!hdsaPrintDef)
    {
        goto Error0;
    }

    if (lpszPrinterName)
    {
        i = FindPrinter(hdsaPrintDef, lpszPrinterName);
        if (i < 0)
        {
            goto Error0;
        }

        pThisPrinter = DSA_GetItemPtr(hdsaPrintDef, i);
        SendMessage(pThisPrinter->hwndStub, WM_COMMAND, ID_VIEW_REFRESH, 0L);
    }
    else
    {
        for (i=DSA_GetItemCount(hdsaPrintDef)-1; i>=0; --i)
        {
            pThisPrinter = DSA_GetItemPtr(hdsaPrintDef, i);
            SendMessage(pThisPrinter->hwndStub, WM_COMMAND, ID_VIEW_REFRESH, 0L);
        }
    }
Error0:
    LeaveCriticalSection(&g_csPrinters);
}


//---------------------------------------------------------------------------
//
// IDropTarget stuff
//

STDMETHODIMP CPrintObjs_DragEnter(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdt);

    // let the base-class process it now to save away pdwEffect
    CIDLDropTarget_DragEnter(pdt, pDataObj, grfKeyState, pt, pdwEffect);

    // We allow files to be dropped for printing
    // if it is from the bitbucket only DROEFFECT_MOVE will be set in *pdwEffect
    // so this will keep us from printing wastbasket items.

    if (this->dwData & DTID_HDROP)
        *pdwEffect &= DROPEFFECT_COPY;
    else
        *pdwEffect = DROPEFFECT_NONE;   // Default action is nothing

    this->dwEffectLastReturned = *pdwEffect;

    return S_OK;
}

void Printer_PrintHDROPFiles(HWND hwnd, HDROP hdrop, LPCITEMIDLIST pidlPrinter)
{
    DRAGINFO di;

    di.uSize = SIZEOF(di);
    if (DragQueryInfo(hdrop, &di))
    {
        LPTSTR lpFileName = di.lpFileList;
        int i = IDYES;

        // Printing more than one file at a time can easily fail.
        // Ask the user to confirm this operation.
        if (*lpFileName && *(lpFileName + lstrlen(lpFileName) + 1))
        {
            i = ShellMessageBox(HINST_THISDLL,
                NULL,
                MAKEINTRESOURCE(IDS_MULTIPLEPRINTFILE),
                MAKEINTRESOURCE(IDS_PRINTERS),
                MB_YESNO|MB_ICONINFORMATION);
        }

        if (i == IDYES)
        {
            // BUGBUG: It would be really nice to have a progress bar when
            // printing multiple files.  And there should definitely be a way
            // to cancel this operation. Oh well, we warned them...

            while (*lpFileName != TEXT('\0'))
            {
                Printer_PrintFile(hwnd, lpFileName, pidlPrinter);

                lpFileName += lstrlen(lpFileName) + 1;
            }
        }

        SHFree(di.lpFileList);
    }
}

void FreePrinterThreadParam(PRNTHREADPARAM *pthp)
{
    if (pthp->pDataObj)
        pthp->pDataObj->lpVtbl->Release(pthp->pDataObj);

    if (pthp->pstmDataObj)
        pthp->pstmDataObj->lpVtbl->Release(pthp->pstmDataObj);

    ILFree(pthp->pidl);
    LocalFree((HLOCAL)pthp);
}
//
// This is the entry of "drop thread"
//
DWORD CALLBACK CPrintObj_DropThreadProc(void *pv)
{
    PRNTHREADPARAM *pthp = (PRNTHREADPARAM *)pv;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    CoGetInterfaceAndReleaseStream(pthp->pstmDataObj, &IID_IDataObject, (void **)&pthp->pDataObj);
    pthp->pstmDataObj = NULL;

    if (pthp->pDataObj && SUCCEEDED(pthp->pDataObj->lpVtbl->GetData(pthp->pDataObj, &fmte, &medium)))
    {
        Printer_PrintHDROPFiles(pthp->hwnd, medium.hGlobal, pthp->pidl);
        ReleaseStgMedium(&medium);
    }

    FreePrinterThreadParam(pthp);
    return 0;
}

HRESULT PrintObj_DropPrint(IDataObject *pDataObj, HWND hwnd, DWORD dwEffect, LPCITEMIDLIST pidl, LPTHREAD_START_ROUTINE pfn)
{
    HRESULT hres;

    PRNTHREADPARAM *pthp = (PRNTHREADPARAM *)LocalAlloc(LPTR, SIZEOF(*pthp));
    if (pthp)
    {
        CoMarshalInterThreadInterfaceInStream(&IID_IDataObject, (IUnknown *)pDataObj, &pthp->pstmDataObj);

        pthp->hwnd = hwnd;
        pthp->dwEffect = dwEffect;
        pthp->pidl = ILClone(pidl);

        if (hwnd)
            ShellFolderView_GetAnchorPoint(hwnd, FALSE, &pthp->ptDrop);

        if (SHCreateThread(pfn, pthp, CTF_COINIT, NULL))
        {
            hres = S_OK;
        }
        else
        {
            FreePrinterThreadParam(pthp);
            hres = E_OUTOFMEMORY;
        }
    }
    return hres;
}

STDMETHODIMP CPrintObjs_DropCallback(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect, LPTHREAD_START_ROUTINE pfn)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdt);
    HRESULT hres;

    *pdwEffect = this->dwEffectLastReturned;

    if (*pdwEffect)
        hres = CIDLDropTarget_DragDropMenu(this, DROPEFFECT_COPY, pDataObj,
                                           pt, pdwEffect, NULL, NULL, MENU_PRINTOBJ_DD, grfKeyState);
    else
        hres = S_FALSE;

    if (*pdwEffect)
        hres = PrintObj_DropPrint(pDataObj, this->hwndOwner, *pdwEffect, this->pidl, pfn);

    CIDLDropTarget_DragLeave(pdt);
    return hres;
}

STDMETHODIMP CPrintObjs_Drop(IDropTarget *pdt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return CPrintObjs_DropCallback(pdt, pDataObj, grfKeyState, pt, pdwEffect, CPrintObj_DropThreadProc);
}


const IDropTargetVtbl c_CPrintObjsDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CPrintObjs_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CPrintObjs_Drop,
};
