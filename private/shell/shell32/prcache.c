//---------------------------------------------------------------------------
// prcache.c
//
// implements an Open Printer cache and Printer_ functions which use it.
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"

#include "copy.h"

#ifdef WINNT
HANDLE Printer_OpenPrinterAdmin(LPCTSTR lpszPrinterName)
{
    HANDLE hPrinter = NULL;

    PRINTER_DEFAULTS PrinterDefaults;
    PrinterDefaults.pDatatype = NULL;
    PrinterDefaults.pDevMode  = NULL;
    PrinterDefaults.DesiredAccess  = PRINTER_ALL_ACCESS;

    // PRINTER_READ ? READ_CONTROL

    if (!OpenPrinter((LPTSTR)lpszPrinterName, &hPrinter, &PrinterDefaults))
    {
        hPrinter = NULL; // OpenPrinter may trash hPrinter
    }

    return(hPrinter);
}
#endif

HANDLE Printer_OpenPrinter(LPCTSTR lpszPrinterName)
{
    HANDLE hPrinter = NULL;

    if (!OpenPrinter((LPTSTR)lpszPrinterName, &hPrinter, NULL))
    {
        hPrinter = NULL; // OpenPrinter may trash hPrinter
    }

    return(hPrinter);
}

VOID Printer_ClosePrinter(HANDLE hPrinter)
{
    ClosePrinter(hPrinter);
}

BOOL Printers_DeletePrinter(HWND hWnd, LPCTSTR pszFullPrinter, DWORD dwAttributes, LPCTSTR pszServer)
{
    DWORD dwCommand = MSP_REMOVEPRINTER;

    if (SHIsRestricted(hWnd, REST_NOPRINTERDELETE))
        return FALSE;

#ifdef WINNT
    if (pszServer && pszServer[0])
    {
        //
        // It's a printer on the remote server.  (Skip \\ prefix on server.)
        //
        if (ShellMessageBox(HINST_THISDLL, hWnd,
            MAKEINTRESOURCE(IDS_SUREDELETEREMOTE),
            MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION,
            pszFullPrinter, SkipServerSlashes(pszServer)) != IDYES)
        {
            return FALSE;
        }
    }
    else if (dwAttributes & PRINTER_ATTRIBUTE_NETWORK)
    {
        TCHAR szScratch[MAXNAMELENBUFFER];
        LPTSTR pszPrinter, pszServer;

        if (!(dwAttributes & PRINTER_ATTRIBUTE_LOCAL))
        {
            //
            // If it's not local, then it must be a remote connection.  Note
            // that we can't just check for PRINTER_ATTRIBUTE_NETWORK because
            // NT's spooler has 'masq' printers that are local printers
            // that masquarade as network printers.  Even though they
            // are created by connecting to a printer, the have both LOCAL
            // and NETWORK bits set.
            //
            dwCommand = MSP_REMOVENETPRINTER;
        }

        Printer_SplitFullName(szScratch, pszFullPrinter, &pszServer, &pszPrinter);

        if (pszServer && *pszServer)
        {
            //
            // It's a printer connection.
            //
            if (ShellMessageBox(HINST_THISDLL, hWnd,
                MAKEINTRESOURCE(IDS_SUREDELETECONNECTION),
                MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION,
                pszPrinter, SkipServerSlashes(pszServer)) != IDYES)
            {
                return FALSE;
            }
        }
        else
        {
            //
            // It's a printer connection with a printer name that 
            // does not have a server name prefix i.e. \\server\printer.  This
            // is true for the http connected printer, which have printer names
            // of the form http://server/printer on NT these printers are 
            // 'masq' printers.  A 'masq' printer is a printer which 
            // is a local printer acting as network connection.
            //
            if (ShellMessageBox(HINST_THISDLL, hWnd,
                MAKEINTRESOURCE(IDS_SUREDELETECONNECTIONNOSERVERNAME),
                MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION,
                pszPrinter) != IDYES)
            {
                return FALSE;
            }
        }
    }
    else

    //
    // Neither a remote printer nor a local connection.  The final
    // upcoming else clause is a local printer.
    //

#endif

    if (ShellMessageBox(HINST_THISDLL, hWnd, MAKEINTRESOURCE(IDS_SUREDELETE),
        MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION, pszFullPrinter)
        != IDYES)
    {
        return FALSE;
    }

    if (CallPrinterCopyHooks(hWnd, PO_DELETE, 0, pszFullPrinter, 0, NULL, 0)
        != IDYES)
    {
        return FALSE;
    }

    //
    // Cast away const.  Safe since Printers_PrinterSetup only modifies
    // pszPrinter if dwCommand is MSP_NEWDRIVER.
    //
    return BOOLFROMPTR(Printers_PrinterSetup(hWnd, dwCommand,
        (LPTSTR)pszFullPrinter, pszServer));
}

BOOL Printer_GPI2CB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pBuf, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return GetPrinter(hPrinter, dwLevel, pBuf, dwSize, lpdwNeeded);
}

#ifdef WINNT
//
// Old NT printers don't support the level 5.  So we try for the 2 after 5.
// Win96 WILL PROBABLY WANT TO DO THIS TOO!
//
LPPRINTER_INFO_5 Printer_MakePrinterInfo5( HANDLE hPrinter )
{
    LPPRINTER_INFO_5 pPI5;
    DWORD cbPI5;
    DWORD cbName;
    LPPRINTER_INFO_2 pPI2 = Printer_EnumProps(hPrinter, 2, NULL, Printer_GPI2CB, (LPVOID)0);
    if (!pPI2)
        return NULL;

    cbName = (lstrlen(pPI2->pPrinterName)+1) * SIZEOF(TCHAR);

    cbPI5 = SIZEOF(PRINTER_INFO_5) + cbName;

    //
    // Port name may not be supported (e.g., downlevel machines).
    //
    if (pPI2->pPortName)
    {
        cbPI5 += (lstrlen(pPI2->pPortName)+1) * SIZEOF(TCHAR);
    }

    pPI5 = (LPPRINTER_INFO_5)LocalAlloc(LPTR, cbPI5);
    if (pPI5)
    {
        ASSERT(pPI5->pPrinterName==NULL);   // These should be null for the
        ASSERT(pPI5->pPortName==NULL);      // no names case

        if (pPI2->pPrinterName)
        {
            pPI5->pPrinterName = (LPTSTR)(pPI5+1);
            lstrcpy(pPI5->pPrinterName, pPI2->pPrinterName);
        }
        if (pPI2->pPortName)
        {
            pPI5->pPortName    = (LPTSTR)((LPBYTE)(pPI5+1) + cbName);
            lstrcpy(pPI5->pPortName, pPI2->pPortName);
        }
        pPI5->Attributes = pPI2->Attributes;
        pPI5->DeviceNotSelectedTimeout = 0;
        pPI5->TransmissionRetryTimeout = 0;
    }
    LocalFree(pPI2);

    return(pPI5);
}
#endif

LPVOID Printer_GetPrinterInfo(HANDLE hPrinter, DWORD dwLevel)
{
    LPVOID pPrinter = Printer_EnumProps(hPrinter, dwLevel, NULL, Printer_GPI2CB, (LPVOID)0);
#ifdef WINNT
    //
    // Old NT printers don't support the level 5.  So we try for the 2 after 5.
    // Win96 WILL PROBABLY WANT TO DO THIS TOO!
    //
    if (!pPrinter && dwLevel == 5)
        return(Printer_MakePrinterInfo5(hPrinter));
#endif
    return pPrinter;

}

LPVOID Printer_GetPrinterInfoStr(LPCTSTR lpszPrinterName, DWORD dwLevel)
{
    LPPRINTER_INFO_2 pPI2 = NULL;
    HANDLE hPrinter = Printer_OpenPrinter(lpszPrinterName);
    if (hPrinter)
    {
        pPI2 = Printer_GetPrinterInfo(hPrinter, dwLevel);
        Printer_ClosePrinter(hPrinter);
    }
    return pPI2;
}

// Generate a SHCNE_RENAMEITEM event for this name change.
void Printer_SHChangeNotifyRename(LPTSTR pOldName, LPTSTR pNewName)
{
    LPITEMIDLIST pidlParent = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
    if (pidlParent)
    {
        IDPRINTER idpOld;
        LPITEMIDLIST pidlOld;

        Printers_FillPidl(&idpOld, pOldName);
        *(USHORT *)((LPBYTE)(&idpOld) + idpOld.cb) = 0;

        pidlOld = ILCombine(pidlParent, (LPCITEMIDLIST)&idpOld);
        if (pidlOld)
        {
            IDPRINTER idpNew;
            LPITEMIDLIST pidlNew;

            Printers_FillPidl(&idpNew, pNewName);

            pidlNew = ILCombine(pidlParent, (LPCITEMIDLIST)&idpNew);
            if (pidlNew)
            {
                SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_IDLIST, pidlOld, pidlNew);
                ILFree(pidlNew);
            }
            ILFree(pidlOld);
        }
        ILFree(pidlParent);
    }
}
