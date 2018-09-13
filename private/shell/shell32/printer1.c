#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"

typedef struct tagPRINTERS_RUNDLL_INFO
{
    UINT  uAction;
    LPTSTR lpBuf1;
    LPTSTR lpBuf2;
} PRINTERS_RUNDLL_INFO, * LPPRI;

// forward prototypes
void Printer_OpenMe(HWND hwnd, LPCTSTR pName, LPCTSTR pServer, BOOL fModal);
VOID Printers_ProcessCommand(HWND hwndStub, LPPRI lpPRI, BOOL fModal);


TCHAR const c_szPrintersGetCommand_RunDLL[] = TEXT("SHELL32,PrintersGetCommand_RunDLL");

SHSTDAPI_(BOOL) SHInvokePrinterCommand(
    IN HWND    hwnd, 
    IN UINT    uAction, 
    IN LPCTSTR lpBuf1, 
    IN LPCTSTR lpBuf2, 
    IN BOOL    fModal)
{
#ifndef WINNT
    UINT  cchBuf1, cchBuf2 = 0;
#endif
    PRINTERS_RUNDLL_INFO PRI;
    BOOL  fRet = FALSE;

    PRI.uAction = uAction;
    PRI.lpBuf1 = (LPTSTR)lpBuf1;
    PRI.lpBuf2 = (LPTSTR)lpBuf2;

#ifdef WINNT
    //
    // Use same processes on NT.
    //
#else

    if (!fModal)     // force modal
    {
        LPTSTR lpCmdLine;

        cchBuf1 = lstrlen(lpBuf1);
        if (lpBuf2)
        {
            cchBuf2 = lstrlen(lpBuf2);
        }
        else
        {
            lpBuf2 = TEXT("");
        }

        lpCmdLine = (LPTSTR)Alloc(SIZEOF(c_szPrintersGetCommand_RunDLL)
                            + (cchBuf1 + cchBuf2 + 1 + 1 + 30) * SIZEOF(TCHAR));
            // Size of "Shell32,PrintersGetComm... ###,###,###,{lpbuf1} {lpbuf2}
            // 1 = space
            // 1 = NUL terminator
            // 30 = max length of digits in uAction,cchBuf1,cchBuf2

        if (!lpCmdLine)
            return FALSE;

        wsprintf(lpCmdLine, TEXT("%s %d,%d,%d,%s %s"), c_szPrintersGetCommand_RunDLL,
                  uAction, cchBuf1, cchBuf2, lpBuf1, lpBuf2);

        // Why do we want this as another process? Isn't another thread enough?
        fRet = SHRunDLLProcess(hwnd, lpCmdLine, SW_SHOWNORMAL, IDS_PRINTERS, FALSE);

        Free(lpCmdLine);
        return TRUE;
    }

#endif

    Printers_ProcessCommand(hwnd, &PRI, fModal);
    return TRUE;
}


#ifdef UNICODE

SHSTDAPI_(BOOL)
SHInvokePrinterCommandA(
    IN HWND    hwnd, 
    IN UINT    uAction, 
    IN LPCSTR  lpBuf1,      OPTIONAL
    IN LPCSTR  lpBuf2,      OPTIONAL
    IN BOOL    fModal)
{
    WCHAR szBuf1[MAX_PATH];    
    WCHAR szBuf2[MAX_PATH];    

    if (lpBuf1)
    {
        MultiByteToWideChar(CP_ACP, 0, lpBuf1, -1, szBuf1, SIZECHARS(szBuf1));
        lpBuf1 = (LPCSTR)szBuf1;
    }

    if (lpBuf2)
    {
        MultiByteToWideChar(CP_ACP, 0, lpBuf2, -1, szBuf2, SIZECHARS(szBuf2));
        lpBuf2 = (LPCSTR)szBuf2;
    }

    return SHInvokePrinterCommand(hwnd, uAction, (LPCWSTR)lpBuf1, (LPCWSTR)lpBuf2, fModal);
}

#else

SHSTDAPI_(BOOL)
SHInvokePrinterCommandW(
    IN HWND    hwnd, 
    IN UINT    uAction, 
    IN LPCWSTR lpBuf1,      OPTIONAL
    IN LPCWSTR lpBuf2,      OPTIONAL
    IN BOOL    fModal)
{
    CHAR szBuf1[MAX_PATH];    
    CHAR szBuf2[MAX_PATH];    

    if (lpBuf1)
    {
        WideCharToMultiByte(CP_ACP, 0, lpBuf1, -1, szBuf1, SIZECHARS(szBuf1), NULL, NULL);
        lpBuf1 = (LPCWSTR)szBuf1;
    }

    if (lpBuf2)
    {
        WideCharToMultiByte(CP_ACP, 0, lpBuf2, -1, szBuf2, SIZECHARS(szBuf2), NULL, NULL);
        lpBuf2 = (LPCWSTR)szBuf2;
    }

    return SHInvokePrinterCommand(hwnd, uAction, (LPCSTR)lpBuf1, (LPCSTR)lpBuf2, fModal);
}

#endif  // UNICODE


// needs to be referenced by printobj.c for now.  When NotifyOnPrinterChange
// is up and running, we will no longer need this to notify open queue views,
// as those windows can listen to the notifications directly themselves.
HDSA hdsaPrintDef = NULL;

extern IDataObjectVtbl c_CPrintersIDLDataVtbl;

VOID WINAPI PrintersGetCommand_RunDLL_Common(HWND hwndStub, HINSTANCE hAppInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    PRINTERS_RUNDLL_INFO    PRI;
    UINT cchBuf1;
    UINT cchBuf2;
    LPTSTR lpComma;
    LPTSTR lpCommaNext;
    lpComma = StrChr(lpszCmdLine,TEXT(','));
    if (lpComma == NULL)
    {
        goto BadCmdLine;
    }
    *lpComma = TEXT('\0');        // Terminate it here
    PRI.uAction = StrToLong(lpszCmdLine);

    lpCommaNext = StrChr(lpComma+1,TEXT(','));
    if (lpCommaNext == NULL)
    {
        goto BadCmdLine;
    }
    *lpCommaNext = TEXT('\0');        // Terminate it here
    cchBuf1 = StrToLong(lpComma+1);
    lpComma = lpCommaNext;

    lpCommaNext = StrChr(lpComma+1,TEXT(','));
    if (lpCommaNext == NULL)
    {
        goto BadCmdLine;
    }
    *lpCommaNext = TEXT('\0');        // Terminate it here
    cchBuf2 = StrToLong(lpComma+1);
    lpComma = lpCommaNext;

    PRI.lpBuf1 = lpComma+1;     // Just past the comma
    *(PRI.lpBuf1+cchBuf1) = '\0';

    if (cchBuf2 == 0)
    {
        PRI.lpBuf2 = NULL;
    }
    else
    {
        PRI.lpBuf2 = PRI.lpBuf1+cchBuf1+1;
    }

#ifdef WINNT
    //
    // Make this modal.
    //
    Printers_ProcessCommand(hwndStub, &PRI, TRUE);
#else
    Printers_ProcessCommand(hwndStub, &PRI, FALSE);
#endif
    return;

BadCmdLine:
    DebugMsg(DM_ERROR, TEXT("pgc_rd: bad command line: %s"), lpszCmdLine);
    return;
}

VOID WINAPI PrintersGetCommand_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
#ifdef UNICODE
    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR  lpwszCmdLine;

    lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*SIZEOF(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);
        PrintersGetCommand_RunDLL_Common( hwndStub,
                                          hAppInstance,
                                          lpwszCmdLine,
                                          nCmdShow );
        LocalFree(lpwszCmdLine);
    }
#else
    PrintersGetCommand_RunDLL_Common( hwndStub,
                                      hAppInstance,
                                      lpszCmdLine,
                                      nCmdShow );
#endif
}

VOID WINAPI PrintersGetCommand_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
#ifdef UNICODE
    PrintersGetCommand_RunDLL_Common( hwndStub,
                                      hAppInstance,
                                      lpwszCmdLine,
                                      nCmdShow );
#else
    UINT iLen = WideCharToMultiByte(CP_ACP, 0,
                                    lpwszCmdLine, -1,
                                    NULL, 0, NULL, NULL)+1;
    LPSTR  lpszCmdLine;

    lpszCmdLine = (LPSTR)LocalAlloc(LPTR,iLen);
    if (lpszCmdLine)
    {
        WideCharToMultiByte(CP_ACP, 0,
                            lpwszCmdLine, -1,
                            lpszCmdLine, iLen,
                            NULL, NULL);
        PrintersGetCommand_RunDLL_Common( hwndStub,
                                          hAppInstance,
                                          lpszCmdLine,
                                          nCmdShow );
        LocalFree(lpszCmdLine);
    }
#endif
}

/********************************************************************

    lpPRI structure description based on uAction.

    uAction             lpBuf1   lpBuf2

    OPEN,               printer, server
    PROPERTIES,         printer, SheetName
    NETINSTALL,         printer,
    NETINSTALLLINK,     printer, target directory to create link
    OPENNETPRN,         printer,
    TESTPAGE            printer

********************************************************************/

VOID Printers_ProcessCommand(HWND hwndStub, LPPRI lpPRI, BOOL fModal)
{
    switch (lpPRI->uAction)
    {
    case PRINTACTION_OPEN:
        if (!lstrcmpi(lpPRI->lpBuf1, c_szNewObject))
        {
#ifdef WINNT
            Printers_PrinterSetup(hwndStub, MSP_NEWPRINTER_MODELESS,
                                  lpPRI->lpBuf1, lpPRI->lpBuf2);
#else
            LPITEMIDLIST pidl;
            pidl = Printers_PrinterSetup(hwndStub, MSP_NEWPRINTER,
                                         lpPRI->lpBuf1, lpPRI->lpBuf2);
            if (pidl)
                ILFree(pidl);
#endif
        }
        else
        {
            Printer_OpenMe(hwndStub, lpPRI->lpBuf1, lpPRI->lpBuf2, fModal);
        }
        break;

#ifdef WINNT

    //
    // BUGBUG
    //
    // Using hdsa and Printer_OneWindowAction guards against duplicate
    // windows within a single processes.  If you launch 2 explorers,
    // then you can get duplicates.
    //

    case PRINTACTION_SERVERPROPERTIES:
    {
        static HDSA hdsaServerProp = NULL;
        LPCTSTR pszServer = (LPTSTR)(lpPRI->lpBuf1);

        // we should never get called with c_szNewObject
        ASSERT(lstrcmpi(lpPRI->lpBuf1, c_szNewObject));
        Printer_OneWindowAction(hwndStub, pszServer, &hdsaServerProp,
                                vServerPropPages, 0, fModal);
        break;
    }
    case PRINTACTION_DOCUMENTDEFAULTS:
    {
        static HDSA hdsaDocDef = NULL;

        // we should never get called with c_szNewObject
        ASSERT(lstrcmpi(lpPRI->lpBuf1, c_szNewObject));
        Printer_OneWindowAction(hwndStub, lpPRI->lpBuf1, &hdsaDocDef,
                                vDocumentDefaults, (LPARAM)(lpPRI->lpBuf2),
                                fModal);
        break;
    }
#endif

    case PRINTACTION_PROPERTIES:
    {
        static HDSA hdsaProp = NULL;

        // we should never get called with c_szNewObject
        ASSERT(lstrcmpi(lpPRI->lpBuf1, c_szNewObject));

#ifdef WINNT // PRINTQ
        Printer_OneWindowAction(hwndStub, lpPRI->lpBuf1, &hdsaProp,
                                vPrinterPropPages, (LPARAM)(lpPRI->lpBuf2),
                                fModal);
#else
        Printer_OneWindowAction(hwndStub, lpPRI->lpBuf1, &hdsaProp,
                                Printer_Properties, (LPARAM)(lpPRI->lpBuf2),
                                fModal);
#endif
        break;
    }

    case PRINTACTION_NETINSTALLLINK:
    case PRINTACTION_NETINSTALL:
    {
        LPITEMIDLIST pidl = Printers_PrinterSetup(hwndStub, MSP_NETPRINTER, lpPRI->lpBuf1, NULL);
        if (pidl)
        {
            if (lpPRI->uAction == PRINTACTION_NETINSTALLLINK)
            {
                // BUGBUG: Do we want Out Of Memory errors on failure?

                LPITEMIDLIST pidlParent = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
                if (pidlParent)
                {
                    LPDATAOBJECT pDataObj;
                    LPCITEMIDLIST pidlChild = ILFindLastID(pidl);
                    HRESULT hres = CIDLData_CreateFromIDArray(pidlParent, 1, &pidlChild, &pDataObj);
                    if (SUCCEEDED(hres))
                    {
                        SHCreateLinks(hwndStub, lpPRI->lpBuf2, pDataObj, SHCL_USETEMPLATE, NULL);
                        pDataObj->lpVtbl->Release(pDataObj);
                    }
                    ILFree(pidlParent);
                }
            }
            ILFree(pidl);
        }

        break;
    }

    case PRINTACTION_OPENNETPRN:
    {
        LPITEMIDLIST pidl;

        pidl = Printers_GetInstalledNetPrinter(lpPRI->lpBuf1);

        // Can't find it? Install one.
        if (!pidl)
        {
            int i = ShellMessageBox(HINST_THISDLL,
                        hwndStub,
                        MAKEINTRESOURCE(IDS_INSTALLNETPRINTER),
                        MAKEINTRESOURCE(IDS_PRINTERS),
                        MB_YESNO|MB_ICONINFORMATION,
                        lpPRI->lpBuf1);
            if (i == IDYES)
            {
                pidl = Printers_PrinterSetup(hwndStub, MSP_NETPRINTER, lpPRI->lpBuf1, NULL);
            }
        }

        if (pidl)
        {
            LPIDPRINTER pidp = (LPIDPRINTER)ILFindLastID(pidl);

            Printer_OpenMe(hwndStub, pidp->cName, NULL, fModal);

            ILFree(pidl);
        }

        break;
    } // case PRINTACTION_OPENNETPRN

    case PRINTACTION_TESTPAGE:
        Printers_PrinterSetup(hwndStub, MSP_TESTPAGEPARTIALPROMPT,
                        lpPRI->lpBuf1, NULL);
        break;

    default:
        DebugMsg(TF_WARNING, TEXT("PrintersGetCommand_RunDLL() received unrecognized uAction %d"), lpPRI->uAction);
        break;
    }
}

void Printer_OpenMe(HWND hwnd, LPCTSTR pName, LPCTSTR pServer, BOOL fModal)
{
    BOOL fOpened = FALSE;
    HKEY hkeyPrn;
    TCHAR buf[50+MAXNAMELEN];

    hwnd = NULL;    // BUGBUG::: set hwnd NULL to make it work correctly

    wsprintf(buf, TEXT("Printers\\%s"), pName);
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, buf, &hkeyPrn))
    {
        SHELLEXECUTEINFO sei =
        {
            SIZEOF(SHELLEXECUTEINFO),
            SEE_MASK_CLASSKEY | SEE_MASK_FLAG_NO_UI, // fMask
            hwnd,                       // hwnd
            NULL,                       // lpVerb
            pName,                      // lpFile
            NULL,                       // lpParameters
            NULL,                       // lpDirectory
            SW_SHOWNORMAL,              // nShow
            NULL,                       // hInstApp
            NULL,                       // lpIDList
            NULL,                       // lpClass
            hkeyPrn,                    // hkeyClass
            0,                          // dwHotKey
            NULL                        // hIcon
        };

        fOpened = ShellExecuteEx(&sei);

        RegCloseKey(hkeyPrn);
    }

    if (!fOpened)
    {
#ifdef WINNT // PRINTQ
        Printer_OneWindowAction(hwnd, pName, &hdsaPrintDef,
                                vQueueCreate, (LPARAM)fModal, FALSE);
#else
        Printer_OneWindowAction(hwnd, pName, &hdsaPrintDef,
                                Printer_ViewQueue, 0, FALSE);
#endif
    }
}

#ifdef DEBUG
//
// Type checking
//
const RUNDLLPROCA lpfnRunDLL = PrintersGetCommand_RunDLL;
const RUNDLLPROCW lpfnRunDLLW = PrintersGetCommand_RunDLLW;
#endif

// Printers_GetInstalledNetPrinter returns the absolute pidl to the first
// printer found connected to lpNetPath.  If no printers are found with this
// connection, NULL is returned.
LPITEMIDLIST Printers_GetInstalledNetPrinter(LPCTSTR lpNetPath)
{
    LPITEMIDLIST pidl = NULL;

    // this function is called from several places in the shell for point&print,
    // so make sure we have winspool loaded
#ifdef WINNT

    LPCTSTR pszPrinter = lpNetPath;
    HANDLE hPrinter = Printer_OpenPrinter( lpNetPath );
    LPPRINTER_INFO_2 pInfo2 = NULL;

    LPPRINTER_INFO_4 pPrinter;
    DWORD dwNumPrinters;

    //
    // Try and resolve to the real printer name rather than the
    // share name, since the printer info from EnumPrinters info4
    // are printer names, and not share names.
    //
    if (hPrinter)
    {
        pInfo2 = (LPPRINTER_INFO_2)Printer_GetPrinterInfo(hPrinter, 2);

        if (pInfo2)
        {
            pszPrinter = pInfo2->pPrinterName;
        }

        Printer_ClosePrinter(hPrinter);
    }

    //
    // Retrieve all the printers on the local machine.  It's
    // cheaper to use level 4 since we won't hit the net.
    //
    dwNumPrinters = Printers_EnumPrinters(NULL,
                                          PRINTER_ENUM_LOCAL |
                                              PRINTER_ENUM_FAVORITE,
                                          4, &pPrinter);
#else
    LPPRINTER_INFO_5 pPrinter;
    DWORD dwNumPrinters = Printers_EnumPrinters(NULL, PRINTER_ENUM_LOCAL,
                                                5, &pPrinter);
#endif
    if (dwNumPrinters)
    {
        DWORD i;
        for (i = 0; i < dwNumPrinters ; i++)
        {
#ifdef WINNT
            if (!lstrcmpi(pszPrinter, pPrinter[i].pPrinterName))
#else
            if (!lstrcmpi(lpNetPath, pPrinter[i].pPortName))
#endif
            {
                DebugMsg(DM_TRACE, TEXT("Found installed net printer: %s"), pPrinter[i].pPrinterName);

                pidl = Printers_GetPidl(NULL, pPrinter[i].pPrinterName);
                break;
            }
        }
        LocalFree((HLOCAL)pPrinter);
    }

#ifdef WINNT
    if (pInfo2)
    {
        LocalFree(pInfo2);
    }
#endif

    return pidl;
}


//
// Arguments:
//  pidl -- (absolute) pidl to the object of interest
//
// Return '"""<Printer Name>""" """<Driver Name>""" """<Path>"""' if success,
//        NULL if failure
//
// We need """ because shlexec strips the outer quotes and converts "" to "
//
UINT Printer_GetPrinterInfoFromPidl(LPCITEMIDLIST pidl, LPTSTR *plpParms)
{
    LPCIDPRINTER pidp = (LPCIDPRINTER)ILFindLastID(pidl);
    LPTSTR lpBuffer = NULL;
    HANDLE hPrinter;
    UINT uErr = ERROR_NOT_ENOUGH_MEMORY;

    hPrinter = Printer_OpenPrinter(pidp->cName);
    if (hPrinter)
    {
        PRINTER_INFO_5 *pPrinter;
        pPrinter = Printer_GetPrinterInfo(hPrinter, 5);
        if (pPrinter)
        {
            DRIVER_INFO_2 *pPrinterDriver;
            pPrinterDriver = Printer_GetPrinterDriver(hPrinter, 2);
            if (pPrinterDriver)
            {
                LPTSTR lpDriverName = PathFindFileName(pPrinterDriver->pDriverPath);

                lpBuffer = (void*)LocalAlloc(LPTR, (2+lstrlen(pidp->cName)+1+
                                 2+lstrlen(lpDriverName)+1+
                                 2+lstrlen(pPrinter->pPortName)+1) * SIZEOF(TCHAR));
                if (lpBuffer)
                {
                    //wsprintf(lpBuffer,"\"\"\"%s\"\"\" \"\"\"%s\"\"\" \"\"\"%s\"\"\"",
                    wsprintf(lpBuffer,TEXT("\"%s\" \"%s\" \"%s\""),
                             pidp->cName, lpDriverName, pPrinter->pPortName);
                    uErr = ERROR_SUCCESS;
                }

                LocalFree((HLOCAL)pPrinterDriver);
            }
            LocalFree((HLOCAL)pPrinter);
        }
        Printer_ClosePrinter(hPrinter);
    }
    else
    {
        // HACK: special case this error return in calling function,
        // as we need a special error message
        uErr = ERROR_SUCCESS;
    }

    *plpParms = lpBuffer;

    return(uErr);
}


//
// Arguments:
//  hwndParent -- Specifies the parent window.
//  szFilePath -- The file to printed.
//
void Printer_PrintFile(HWND hWnd, LPCTSTR szFilePath, LPCITEMIDLIST pidl)
{
    LPTSTR  lpFName;                    // name of file we're printing
    TCHAR   szFPath[MAX_PATH];          // path to file we're printing
    LPCTSTR lpOperation;                // ShellExecuteEx operation
    LPTSTR  lpParms;                    // parameters for above operation
    UINT    uErr;                       // return value from ShellExecuteEx
    BOOL    fTryOnce = TRUE;            // we retry once
    LPTSTR  lpRelease = NULL;
    SHELLEXECUTEINFO ExecInfo;

    lpFName = PathFindFileName(szFilePath);

    lstrcpy(szFPath, szFilePath);
    PathRemoveFileSpec(szFPath);

    lpOperation = c_szPrintTo;
    uErr = Printer_GetPrinterInfoFromPidl(pidl, &lpParms);
    if (uErr != ERROR_SUCCESS)
    {
        goto DisplayError;
    }
    if (!lpParms)
    {
        // If you rename a printer and then try to use a link to that
        // printer, we hit this case. Also, if you get a link to a printer
        // on another computer, we'll likely hit this case.
        ShellMessageBox(HINST_THISDLL, hWnd,
#ifdef WINNT
            MAKEINTRESOURCE(IDS_CANTPRINT),
#else
            MAKEINTRESOURCE(IDS_PRINTERNAME_CHANGED),
#endif
            MAKEINTRESOURCE(IDS_PRINTINGERROR),
            MB_OK|MB_ICONINFORMATION);
        return;
    }
    lpRelease = lpParms;

    // print the file

    // REVIEW: It would probably be better to get the context menu of
    // the file and check for the verb PrintTo and then the verb Print,
    // avoiding ShellExecuteEx completely. Less work being done that way.

TryAgain:

    DebugMsg(TF_PRINTER, TEXT("Printer_PrintFile: verb=[%s] file=[%s|%s] parms=[%s]"),lpOperation, szFPath, lpFName, lpParms);
    FillExecInfo(ExecInfo, hWnd, lpOperation, lpFName, lpParms, szFPath, SW_SHOWNORMAL);
    ExecInfo.fMask |= SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
    if (!ShellExecuteEx(&ExecInfo))
    {
        uErr = GetLastError();

        switch (uErr)
        {
        case ERROR_NO_ASSOCIATION:
            if (fTryOnce)
            {
                LPCIDPRINTER pidp = (LPCIDPRINTER)ILFindLastID(pidl);
                BOOL fTryAgain;

                fTryOnce = FALSE;

                // we failed a "PrintTo", set up for a "Print"

                fTryAgain = IsDefaultPrinter(pidp->cName, 0);
                if (!fTryAgain)
                {
                    // this isn't the default printer, ask first
                    fTryAgain =
                        IDYES == ShellMessageBox(
                            HINST_THISDLL, GetTopLevelAncestor(hWnd),
                            MAKEINTRESOURCE(IDS_CHANGEDEFAULTPRINTER),
                            MAKEINTRESOURCE(IDS_PRINTINGERROR),
                            MB_YESNO|MB_ICONINFORMATION) ;
                    if (fTryAgain)
                    {
                        Printer_SetAsDefault(pidp->cName);
                    }
                }

                if (fTryAgain)
                {
                    lpOperation = c_szPrint;
                    lpParms = (LPTSTR)c_szNULL;
                    goto TryAgain;
                }
                break;  // we're done -- no message, the user cancelled
            }

            // fall through

        default:
            {
                TCHAR szTemp[MAX_PATH];
DisplayError:

                if ((UINT_PTR)FindExecutable(lpFName, szFPath, szTemp) <= 32)
                {
                    lstrcpy(szTemp, szFilePath);
                }

                ExecInfo.fMask &= ~SEE_MASK_FLAG_NO_UI;
                _ShellExecuteError(&ExecInfo, szTemp, uErr);

                break;
            }
        }
    }

    if (lpRelease)
        LocalFree((HLOCAL)lpRelease);
}


BOOL Printer_ModifyPrinter(LPCTSTR lpszPrinterName, DWORD dwCommand)
{
    HANDLE hPrinter = Printer_OpenPrinterAdmin(lpszPrinterName);

    BOOL fRet = FALSE;
    if (hPrinter)
    {
        fRet = SetPrinter(hPrinter, 0, NULL, dwCommand);

        if (fRet)
        {
            PrintDef_RefreshQueue(lpszPrinterName);
        }
        Printer_ClosePrinter(hPrinter);
    }

    return fRet;
}


void Printer_CheckMenuItem(HMENU hmModify, UINT fState, UINT uChecked, UINT uUnchecked)
{
    MENUITEMINFO mii;

    if (fState&MF_CHECKED)
    {
        UINT uTemp = uChecked;
        uChecked = uUnchecked;
        uUnchecked = uTemp;
    }

    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_ID|MIIM_STATE;
    mii.cch = 0;        // just in case...
    if (GetMenuItemInfo(hmModify, uChecked, FALSE, &mii))
    {
        mii.wID = uUnchecked;
        mii.fState = fState;
        SetMenuItemInfo(hmModify, uChecked, FALSE, &mii);
    }
}

void Printer_EnableMenuItems(HMENU hmModify, BOOL bEnable)
{
    int i;

    for (i=GetMenuItemCount(hmModify)-1; i>=0; --i)
    {
        EnableMenuItem(hmModify, i,
            bEnable ? MF_BYPOSITION|MF_ENABLED : MF_BYPOSITION|MF_GRAYED);
    }
}

BOOL IsAvoidAutoDefaultPrinter(LPCTSTR pszPrinter);

// this will find the first printer (if any) and set  it as the default
// and inform the user
void Printers_ChooseNewDefault(HWND hwnd)
{
#ifdef WINNT
    PRINTER_INFO_4 *pPrinters = NULL;
#else
    PRINTER_INFO_1 *pPrinters = NULL;
#endif
    DWORD iPrinter, dwNumPrinters = Printers_EnumPrinters(NULL,
#ifdef WINNT
                                          PRINTER_ENUM_LOCAL | PRINTER_ENUM_FAVORITE,
                                          4,
#else
                                          PRINTER_ENUM_LOCAL,
                                          1,
#endif
                                          &pPrinters);
    if (dwNumPrinters)
    {
        if (pPrinters)
        {
            for (iPrinter = 0 ; iPrinter < dwNumPrinters ; iPrinter++)
            {
#ifdef WINNT
                if (!IsAvoidAutoDefaultPrinter(pPrinters[iPrinter].pPrinterName))
#else
                if (!IsAvoidAutoDefaultPrinter(pPrinters[iPrinter].pName))
#endif
                    break;
            }
            if (iPrinter == dwNumPrinters)
            {
                dwNumPrinters = 0;
            }
            else
            {
#ifdef WINNT
                Printer_SetAsDefault(pPrinters[iPrinter].pPrinterName);
#else
                Printer_SetAsDefault(pPrinters[iPrinter].pName);
#endif
            }
        }
        else
        {
            dwNumPrinters = 0;
        }
    }

    // Inform user
    if (dwNumPrinters)
    {
        ShellMessageBox(HINST_THISDLL,
                        hwnd,
                        MAKEINTRESOURCE(IDS_DELNEWDEFAULT),
                        MAKEINTRESOURCE(IDS_PRINTERS),
                        MB_OK,
#ifdef WINNT
                        pPrinters[iPrinter].pPrinterName);
#else
                        pPrinters[iPrinter].pName);
#endif
    }
    else
    {
#ifdef WINNT
        Printer_SetAsDefault(NULL); // clear the default printer
#endif
        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_DELNODEFAULT),
                    MAKEINTRESOURCE(IDS_PRINTERS),  MB_OK);
    }

    if (pPrinters)
        LocalFree((HLOCAL)pPrinters);
}

BOOL Printer_SetAsDefault(LPCTSTR lpszPrinterName)
{
#ifdef WINNT // PRINTQ

    //
    // HACK for SUR.
    //

    TCHAR szDefaultPrinterString[MAX_PATH * 2];
    TCHAR szBuffer[MAX_PATH * 2];

    if( lpszPrinterName ){

        //
        // Not the default, set it.
        //
        if( !GetProfileString( TEXT( "Devices" ),
                               lpszPrinterName,
                               TEXT( "" ),
                               szBuffer,
                               ARRAYSIZE( szBuffer ))){
            return FALSE;
        }

        lstrcpy( szDefaultPrinterString, lpszPrinterName );
        lstrcat( szDefaultPrinterString, TEXT( "," ));
        lstrcat( szDefaultPrinterString, szBuffer );

        //
        // Use the new string for Windows.Device.
        //
        lpszPrinterName = szDefaultPrinterString;
    }

    if( !WriteProfileString( TEXT( "Windows" ),
                             TEXT( "Device" ),
                             lpszPrinterName )){
        return FALSE;
    }

    //
    // Tell the world and make everyone flash.
    //
    SendNotifyMessage( HWND_BROADCAST,
                       WM_WININICHANGE,
                       0,
                       (LPARAM)TEXT( "Windows" ));

   return TRUE;

#else

    BOOL bRet = FALSE;
    HANDLE hPrinter;

    if (lpszPrinterName)
    {
        hPrinter = Printer_OpenPrinter(lpszPrinterName);
        if (hPrinter)
        {
            PRINTER_INFO_5 * pPI5;

            pPI5 = Printer_GetPrinterInfo(hPrinter, 5);
            if (pPI5)
            {
                pPI5->Attributes |= PRINTER_ATTRIBUTE_DEFAULT;

                bRet = SetPrinter(hPrinter, 5, (LPBYTE)pPI5, 0);

                LocalFree((HLOCAL)pPI5);
            }

            Printer_ClosePrinter(hPrinter);
        }
    }
    return(bRet);
#endif
}


LPVOID Printer_EnumProps(HANDLE hPrinter, DWORD dwLevel, DWORD *lpdwNum,
    ENUMPROP lpfnEnum, LPVOID lpData)
{
    DWORD dwSize, dwNeeded;
    LPBYTE pEnum;

    dwSize = 0;
    SetLastError(0);
    lpfnEnum(lpData, hPrinter, dwLevel, NULL, 0, &dwSize, lpdwNum);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        pEnum = NULL;
        goto Error1;
    }

    ASSERT(dwSize < 0x100000L);

    pEnum = (void*)LocalAlloc(LPTR, dwSize);
TryAgain:
    if (!pEnum)
    {
        goto Error1;
    }

    SetLastError(0);
    if (!lpfnEnum(lpData, hPrinter, dwLevel, pEnum, dwSize, &dwNeeded, lpdwNum))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            LPBYTE pTmp;
            dwSize = dwNeeded;
            pTmp = (void*)LocalReAlloc((HLOCAL)pEnum, dwSize,
                    LMEM_MOVEABLE|LMEM_ZEROINIT);
            if (pTmp)
            {
                pEnum = pTmp;
                goto TryAgain;
            }
        }

        LocalFree((HLOCAL)pEnum);
        pEnum = NULL;
    }

Error1:
    return pEnum;
}


BOOL Printers_EnumPrintersCB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return EnumPrinters(PtrToUlong(lpData), (LPTSTR)hPrinter, dwLevel,
                             pEnum, dwSize, lpdwNeeded, lpdwNum);
}

DWORD Printers_EnumPrinters(LPCTSTR pszServer, DWORD dwType, DWORD dwLevel, void **ppPrinters)
{
    DWORD dwNum = 0L;

    //
    // If the server is szNULL, pass in NULL, since EnumPrinters expects
    // this for the local server.
    //
    if (pszServer && !pszServer[0])
    {
        pszServer = NULL;
    }

    *ppPrinters = Printer_EnumProps((HANDLE)pszServer, dwLevel, &dwNum, Printers_EnumPrintersCB, ULongToPtr(dwType));
    if (*ppPrinters==NULL)
    {
        dwNum = 0;
    }
    return dwNum;
}

#ifdef PRN_FOLDERDATA

BOOL Printers_FolderEnumPrintersCB(LPVOID lpData, HANDLE hFolder, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return bFolderEnumPrinters(hFolder, (PFOLDER_PRINTER_DATA)pEnum,
                                   dwSize, lpdwNeeded, lpdwNum);
}

DWORD Printers_FolderEnumPrinters(HANDLE hFolder, LPVOID *ppPrinters)
{
    DWORD dwNum = 0L;

    *ppPrinters = Printer_EnumProps(hFolder, 0, &dwNum,
                                    Printers_FolderEnumPrintersCB,
                                    NULL);
    if (*ppPrinters==NULL)
    {
        dwNum=0;
    }
    return dwNum;
}

BOOL Printers_FolderGetPrinterCB(LPVOID lpData, HANDLE hFolder, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return bFolderGetPrinter(hFolder, (LPCTSTR)lpData, (PFOLDER_PRINTER_DATA)pEnum, dwSize, lpdwNeeded);
}


LPVOID Printer_FolderGetPrinter(HANDLE hFolder, LPCTSTR pszPrinter)
{
    return Printer_EnumProps(hFolder, 0, NULL, Printers_FolderGetPrinterCB, (LPVOID)pszPrinter);
}

#endif

BOOL Printers_GetPrinterDriverCB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return GetPrinterDriver(hPrinter, NULL, dwLevel, pEnum, dwSize, lpdwNeeded);
}


LPVOID Printer_GetPrinterDriver(HANDLE hPrinter, DWORD dwLevel)
{
    return Printer_EnumProps(hPrinter, dwLevel, NULL, Printers_GetPrinterDriverCB, NULL);
}

// cbModule = sizeof(*pszModule)  and  cbModule ~== MAX_PATH+slop
// returns NULL and sets *pid to the icon id in HINST_THISDLL   or
// returns pszModule and sets *pid to the icon id for module pszModule

LPTSTR Printer_FindIcon(CPrinterFolder *psf, LPCTSTR pszPrinterName, LPTSTR pszModule, LONG cbModule, LPINT piIcon)
{
    LPTSTR pszRet = NULL;
    TCHAR szKeyName[256];

    // registry override of the icon

    wnsprintf(szKeyName, ARRAYSIZE(szKeyName), TEXT("Printers\\%s\\DefaultIcon"), pszPrinterName);
    if (SHGetValue(HKEY_CLASSES_ROOT, szKeyName, NULL, NULL, pszModule, &cbModule) && cbModule)
    {
        *piIcon = PathParseIconLocation(pszModule);
        pszRet = pszModule;
    }
    else
    {
        PVOID pData = NULL;
        DWORD dwAttributes = 0;
        LPTSTR pszPort = NULL;
        BOOL fDef;

#ifdef PRN_FOLDERDATA
        //
        // Try retrieving the information from hFolder if it's remote
        // to avoid hitting the net.
        //
        if (psf && psf->pszServer && psf->hFolder &&
            (NULL != (pData = Printer_FolderGetPrinter(psf->hFolder, pszPrinterName))))
        {
            dwAttributes = ((PFOLDER_PRINTER_DATA)pData)->Attributes;
        }
        else
#endif

#ifdef WINNT
        if (Printer_CheckNetworkPrinterByName(pszPrinterName, NULL))
        {
            // avoid hitting the network.
            dwAttributes = PRINTER_ATTRIBUTE_NETWORK;
        }
        else
#endif
        {
            pData = Printer_GetPrinterInfoStr(pszPrinterName, 5);
            if (pData)
            {
                dwAttributes = ((LPPRINTER_INFO_5)pData)->Attributes;
                pszPort = ((LPPRINTER_INFO_5)pData)->pPortName;
            }
        }

        fDef = IsDefaultPrinter(pszPrinterName, dwAttributes);

        if (dwAttributes & PRINTER_ATTRIBUTE_NETWORK)
            *piIcon = fDef ? IDI_DEF_PRINTER_NET : IDI_PRINTER_NET;
        else if (pszPort && !lstrcmp(pszPort, c_szFileColon))
            *piIcon = fDef ? IDI_DEF_PRINTER_FILE : IDI_PRINTER_FILE;
        else
            *piIcon = fDef ? IDI_DEF_PRINTER : IDI_PRINTER;

        if (pData)
            LocalFree((HLOCAL)pData);
    }
    return pszRet;
}

void Printer_LoadIcons(LPCTSTR pszPrinterName, HICON *phLargeIcon, HICON *phSmallIcon)
{
    TCHAR szBuf[MAX_PATH+20];
    LPTSTR pszModule;
    INT iIcon;
    HINSTANCE hinst = NULL;

    // Guarantee that we don't do work for nothing...
    ASSERT(phLargeIcon || phSmallIcon);

    pszModule = Printer_FindIcon(NULL, pszPrinterName, szBuf, ARRAYSIZE(szBuf), &iIcon);

    // cool! we're using someone elses dll... we gotta load it though
    if (pszModule)
    {
        hinst = LoadLibrary(pszModule);
        if (!hinst)
        {
            DebugMsg(TF_ERROR, TEXT("Printer_FindIcon reported an unloadable module!!!"));
        }
    }

    // Ensure we have a valid instance handle
    if (!hinst)
    {
        iIcon = IDI_PRINTER;
        hinst = HINST_THISDLL;
    }

    // Get the large icon.        
    if (phLargeIcon)
        *phLargeIcon = LoadImage(hinst, MAKEINTRESOURCE(iIcon),
                                IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    // Get the small icon.
    if (phSmallIcon)
        *phSmallIcon = LoadImage(hinst, MAKEINTRESOURCE(iIcon),
                                IMAGE_ICON, g_cxSmIcon, g_cxSmIcon, 0);

    // Release the dll if one was loaded.
    if (hinst && hinst != HINST_THISDLL)
        FreeLibrary(hinst);
}
