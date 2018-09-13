#include "shellprv.h"
#pragma  hdrstop

#include "copy.h"
#include "printer.h"

#include <regstr.h>


#ifdef WINNT

extern IDataObjectVtbl c_CPrintersIDLDataVtbl;

VOID
Printer_AddPrinterPropPages(LPCTSTR pszPrinterName, LPPROPSHEETHEADER lpsh)
{
    HKEY hkeyBaseProgID;

    //
    // Add hooked pages if they exist in the registry
    //

    hkeyBaseProgID = NULL;
    RegOpenKey(HKEY_CLASSES_ROOT, c_szPrinters, &hkeyBaseProgID);
    if (hkeyBaseProgID)
    {
        // we need an IDataObject
        LPITEMIDLIST pidlParent = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
        if (pidlParent)
        {
            HRESULT hres;
            IDataObject *lpdtobj;
            IDPRINTER idp;
            LPITEMIDLIST pidp;

            Printers_FillPidl(&idp, pszPrinterName);
            pidp = (LPITEMIDLIST)&idp;
            hres = CIDLData_CreateFromIDArray2(&c_CPrintersIDLDataVtbl, pidlParent, 1, &pidp, &lpdtobj);
            if (hres == NOERROR)
            {
                // add the hooked pages
                HDCA hdca = DCA_Create();
                if (hdca)
                {
                    DCA_AddItemsFromKey(hdca, hkeyBaseProgID, c_szPropSheet);
                    DCA_AppendClassSheetInfo(hdca, hkeyBaseProgID, lpsh, lpdtobj);
                    DCA_Destroy(hdca);
                }
                lpdtobj->lpVtbl->Release(lpdtobj);
            }
            ILFree(pidlParent);
        }
        RegCloseKey(hkeyBaseProgID);
    }
}

#else

// Not in any header files, and GetFileNameFromBrowse doesn't have dwFlags
BOOL _GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cbFilePath,
                                       LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle,
                                       DWORD dwFlags);


#define DATATYPE_NAMELEN 64 // length of DeviceCapability()s datatype name buffer

// these two defines are from SDK\INC16\PRINT.H, which, for some reason,
// isn't includable from here and I don't want to figure out why right now...
#define DC_EMF_COMPLIANT     20
#define DC_DATATYPE_PRODUCED 21

#define SetDlgFocus(hDlg, hwnd) SendMessage(hDlg, WM_NEXTDLGCTL,(WPARAM)(hwnd), 1L)


// these are the Attributes bits we play with
#define PS_ATTRIBUTE_MASK (PRINTER_ATTRIBUTE_DIRECT|PRINTER_ATTRIBUTE_QUEUED|PRINTER_ATTRIBUTE_ENABLE_BIDI)

typedef struct tagPRINTER_INFO_2_COPY {
    TCHAR pPrinterName[MAXNAMELEN];
    TCHAR pComment[256];
    TCHAR pPortName[MAX_PATH];
    TCHAR pDriverName[MAXNAMELEN];
    TCHAR pSepFile[MAX_PATH];
    TCHAR pPrintProcessor[128];
    TCHAR pDatatype[128];
    DWORD Attributes;
    DWORD StartTime;
    DWORD UntilTime;

    // from PRINTER_INFO_5
    DWORD DeviceNotSelectedTimeout;
    DWORD TransmissionRetryTimeout;
} PRINTER_INFO_2_COPY, * LPPRINTER_INFO_2_COPY;

// This information is available to each prop sheet page
typedef struct _PRINTERSTUFF
{
    HANDLE hPrinter;
    PRINTER_INFO_2_COPY sPrinter;
    UINT uFlags;
    UINT nPages;

    // Starts out FALSE.
    // If a page is visited, it's set to TRUE.
    // If apply pressed and it's TRUE, save and set to FALSE.
    // If apply pressed and it's FALSE, then info saved by another page.
    BOOL fSaveEverything;

    // TRUE during PSN_SETACTIVE to avoid turning on the Apply button...
    BOOL fInitializing;

    // A random perf flag for IDDC_PRINTTO -- port stuff is slow
    BOOL fPortModified;

    // MRU of ports connected to
    HANDLE hmru;

    // icon to display
    HICON hIcon;

} PRINTERSTUFF, * LPPRINTERSTUFF;
// uFlags defines:
#define PS_OLDSTYLE              0x0001  // this is an old (3.1) driver
#define DETAIL_PORTS_INITIALIZED 0x0002  // the ports combobox has been set up
#define DETAIL_PORTS_FILLED      0x0004  // the ports combobox has been filled
// hmru registry location:
TCHAR const c_szPrnPortsMRU[] = REGSTR_PATH_EXPLORER TEXT("\\PrnPortsMRU");
TCHAR const c_szLPT[] = TEXT("LPT");

// Each page gets one of these
// psPrinterStuff points to info shared between pages
typedef struct _PRINTERPAGEINFO  // ppi
{
    PROPSHEETPAGE  psp;                 // prop sheet page description
    LPPRINTERSTUFF psPrinterStuff;      // pointer to shared sheet info
} PRINTERPAGEINFO, * LPPRINTERPAGEINFO;

typedef struct _DELPORTDLG // dpd
{
    LPPRINTERPAGEINFO pppi;
    HWND hwndCB;
} DELPORTDLG, * LPDELPORTDLG;


void GeneralDlg_SetFields(HWND hDlg, LPPRINTERPAGEINFO pppi);
void DetailDlg_SetFields(HWND hDlg, LPPRINTERPAGEINFO pppi);
BOOL CALLBACK Printer_AddPortDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Printer_DelPortDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Printer_SpoolSettingsDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
LONG GeneralDlg_SaveEverything(HWND hDlg, LPPRINTERPAGEINFO pppi, int fSave);
UINT _AddPrinterPropPages(LPPRINTERSTUFF psPrinterStuff, HWND hWnd, LPPROPSHEETHEADER lpsh);
BOOL Printer_Load(LPPRINTERSTUFF psPrinterStuff);

// for GeneralDlg_SaveEverything(..., fSave)
#define GDSE_SAVE           0 // save changes, return PSN_ return code
#define GDSE_QUERY_TESTPAGE 1 // if a change affects testpage, return TRUE
// not currently used, but may be useful some day
//#define GDSE_QUERY_SAVE     2 // if a change exists, return TRUE
#define GDSE_NOUI           4 // don't put up UI on errors

// from shprsht.c
BOOL CALLBACK _AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);

//==========================================================================
// Private helper functions
//==========================================================================

void Dlg_CB_GetSelText(HWND hDlg, UINT uID, LPTSTR szText, int cchTextMax)
{
    HWND hCB;
    int iCurSel;
    VDATEINPUTBUF(szText, TCHAR, cchTextMax);

    hCB = GetDlgItem(hDlg, uID);
    iCurSel = ComboBox_GetCurSel(hCB);

    if (iCurSel != CB_ERR)
    {
        ComboBox_GetLBText(hCB, iCurSel, szText);
    }
    else
    {
        ComboBox_GetText(hCB, szText, cchTextMax);
        ComboBox_AddString(hCB, szText);
    }
}


BOOL Printer_SelDlgComboEntry(HWND hWnd, int nID, LPCTSTR lpszEntry, BOOL bAdd)
{
    if (!lpszEntry || !*lpszEntry)
    {
        return FALSE;
    }

    if (nID)
    {
        hWnd = GetDlgItem(hWnd, nID);
    }

    nID = ComboBox_FindStringExact(hWnd, 0, lpszEntry);
    if (nID<0 && bAdd)
    {
        nID = ComboBox_AddString(hWnd, lpszEntry);
    }
    SendMessage(hWnd, CB_SETCURSEL, nID, 0L);

    return(nID >= 0);
}


// Effects: removes every property sheet except the first two (General, Detail)
void _RemovePropSheets(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    int i;
    HWND hdlgParent = GetParent(hDlg);

    // General is page 0, Detail is page 1.  So delete all others.
    for (i = pppi->psPrinterStuff->nPages-1 ; i >= 2 ; i--)
    {
        PropSheet_RemovePage(hdlgParent, i, NULL);
    }

    return;
}

// Effects: rebuilds extension sheets and adds them to property sheet
void _RebuildPropSheets(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    HPROPSHEETPAGE ahpage[MAX_FILE_PROP_PAGES];
    PROPSHEETHEADER psh;
    HWND hwndParent;
    UINT i;

    DebugMsg(TF_PRINTER, TEXT("rebuilding printer property sheets"));

    //
    // Initialize PROPSHEETHEADER
    //
    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hDlg;
    psh.pszCaption = NULL;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = ahpage;

    pppi->psPrinterStuff->nPages = _AddPrinterPropPages(pppi->psPrinterStuff, hDlg, &psh);

    // Destroy General and Detail pages (if they were added)
    if (!SHRestricted(REST_NOPRINTERTABS))
    {
        ASSERT(psh.nPages >= 2);
        for (i=0 ; i<2 ; i++)
            DestroyPropertySheetPage(psh.phpage[i]);
    }

    // Add all other pages
    hwndParent = GetParent(hDlg);
    for (    ; i<psh.nPages ; i++)
    {
        // BUGBUG: if this page doesn't get added for some reason, nobody
        // will call DestroyPropertySheetPage on the hpage!
        //
        PropSheet_AddPage(hwndParent, psh.phpage[i]);
    }

    return;
}

void PrtProp_Apply(HWND hdlg, LPPRINTERPAGEINFO pppi)
{
    if (pppi->psPrinterStuff->fSaveEverything)
    {
        LRESULT lRet = GeneralDlg_SaveEverything(hdlg, pppi, GDSE_SAVE);
        if (lRet == PSNRET_NOERROR)
        {
            pppi->psPrinterStuff->fSaveEverything = FALSE;
        }
        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, lRet);
    }
}


//==========================================================================
// Detail page
//==========================================================================

// If pName is not a prefix in hcbPorts, add "pName (pDescription)", handling
// the case where pDescription is null (3rd party port drivers may screw up)
VOID AddPortToCombobox(HWND hcbPorts, LPTSTR pName, LPTSTR pDescription)
{
    if (CB_ERR == ComboBox_FindString(hcbPorts, 0, pName))
    {
        LPTSTR lpPortName = NULL;
        if (Str_SetPtr(&lpPortName, pName))
        {
            LPTSTR lpFree = NULL;
            LPTSTR lpPort = NULL;

            if (pDescription && *pDescription)
            {
                lpFree =
                lpPort = ShellConstructMessageString(HINST_THISDLL,
                    MAKEINTRESOURCE(IDS_PRTPROP_PORT_FORMAT), pName, pDescription);
            }

            if (!lpPort)
                lpPort = pName;

            ComboBox_SetItemData(hcbPorts,
                ComboBox_AddString(hcbPorts, lpPort),
                lpPortName);

            if (lpFree)
                LocalFree(lpFree);
        }
    }
}

// Get port name (sans description) and return TRUE iff UNC port
BOOL GetPortFromCombobox(HWND hcbPorts, LPTSTR pBuf, int cchBuf)
{
    int iCurSel;
    LPTSTR lpPortName;
    VDATEINPUTBUF(pBuf, TCHAR, cchBuf);

    iCurSel = ComboBox_GetCurSel(hcbPorts);
    if (iCurSel != CB_ERR)
    {
        if (ComboBox_GetLBTextLen(hcbPorts, iCurSel) < cchBuf)
        {
            ComboBox_GetLBText(hcbPorts, iCurSel, pBuf);
            goto try_again;
        }
    }

    if (ComboBox_GetText(hcbPorts, pBuf, cchBuf))
    {
        iCurSel = ComboBox_FindString(hcbPorts, 0, pBuf);
        if (iCurSel != CB_ERR)
        {
try_again:
            lpPortName = (LPTSTR)ComboBox_GetItemData(hcbPorts, iCurSel);
            if (lpPortName)
            {
                lstrcpyn(pBuf, lpPortName, cchBuf);
                return FALSE;
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID NukePortsFromCombobox(HWND hcbPorts)
{
    LPTSTR lpPortName;

    do {
        lpPortName = (LPTSTR)ComboBox_GetItemData(hcbPorts, 0);
        if (lpPortName && lpPortName != (LPTSTR)(LPARAM)CB_ERR)
        {
            Str_SetPtr(&lpPortName, NULL);
        }
    } while (CB_ERR != ComboBox_DeleteString(hcbPorts, 0));

    // probably not necessary
    ComboBox_ResetContent(hcbPorts);
}

VOID EnumNetworkPorts(HWND hcbPorts, DWORD dwScope)
{
    HANDLE        hEnum;
    LPNETRESOURCE lpBuffer;
#ifdef WINNT
    if(NO_ERROR==WNetOpenEnum(dwScope,RESOURCETYPE_PRINT,RESOURCEUSAGE_ATTACHED,NULL,&hEnum))
#else
        if(NO_ERROR==WNetOpenEnum(dwScope,RESOURCETYPE_PRINT, 0,NULL,&hEnum))
#endif
    {
        if(NULL != (lpBuffer=(LPNETRESOURCE)(void*)LocalAlloc(LPTR, 4096)))
        {
            while (TRUE)
            {
                DWORD dwSize=4096;
                DWORD dwEntries=1L;
                DWORD dwResult;

                dwResult=WNetEnumResource(hEnum,&dwEntries,lpBuffer,&dwSize);

                if(NO_ERROR==dwResult && 1==dwEntries)
                {
                    if((RESOURCETYPE_PRINT & lpBuffer->dwType) &&
                        (RESOURCEUSAGE_CONNECTABLE & lpBuffer->dwUsage)
                        && lpBuffer->lpRemoteName
                        && lpBuffer->lpLocalName)
                    {
                        TCHAR szLocalName[20];
                        WORD wStrLength;

                        // Make sure that the local name terminates with
                        // a colon (since this is how it appears from
                        // EnumPorts)

                        lstrcpyn(szLocalName,lpBuffer->lpLocalName,
                            ARRAYSIZE(szLocalName));

                        wStrLength=lstrlen(szLocalName);
                        if(TEXT(':') != szLocalName[wStrLength-1])
                        {
                            if((wStrLength + 2) <= ARRAYSIZE(szLocalName))
                                lstrcat(szLocalName,TEXT(":"));
                            else
                                continue;    // Skip this entry
                        }

                        DebugMsg(DM_TRACE,TEXT("sh PRTPROP - Checking network port [%s]"), szLocalName);
                        AddPortToCombobox(hcbPorts, szLocalName, lpBuffer->lpRemoteName);
                    }
                }
                else
                    break;
            }

            LocalFree((HLOCAL)lpBuffer);
        }

        WNetCloseEnum(hEnum);
    }
}

BOOL Printers_EnumPortsCB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
        LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return(EnumPorts(NULL, dwLevel, pEnum, dwSize, lpdwNeeded, lpdwNum));
}

VOID EnumLocalPorts(HWND hcbPorts)
{
    LPPORT_INFO_2 pPorts;
    DWORD dwNum;

    pPorts = Printer_EnumProps(NULL, 2, &dwNum, Printers_EnumPortsCB, NULL);
    if (pPorts)
    {
        for ( ; dwNum > 0 ; )
        {
            --dwNum;
            DebugMsg(DM_TRACE,TEXT("sh PRTPROP - Checking local port [%s]"), pPorts[dwNum].pPortName);
            AddPortToCombobox(hcbPorts, pPorts[dwNum].pPortName, pPorts[dwNum].pDescription);
        }

        LocalFree((HLOCAL)pPorts);
    }
}


// DetailDlg_FillPorts fills in all the ports and their descriptions into
// the combobox.  We use connected network resources, remembered network
// resources, local ports, and an mru of the last 10 (unc) ports connected to.
void DetailDlg_FillPorts(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    HWND hTemp;

    hTemp = GetDlgItem(hDlg, IDDC_PRINTTO);

#if 0 // there's no easy way to get the listbox any more; it's not a child
      // window of the combobox!  So we can't use tabs...
    if (!(pppi->psPrinterStuff->uFlags & DETAIL_PORTS_INITIALIZED))
    {
        HWND hLB;
        TCHAR szClassName[20];
        int  nTabStop;
        WORD wXBaseUnit;
        RECT rcListBox;

        DebugMsg(DM_TRACE, TEXT("sh PRTPROP - Initializing port combobox"));

        // find listbox of combobox
        hLB = GetWindow(hTemp, GW_CHILD);
        GetClassName(hLB, szClassName, ARRAYSIZE(szClassName));
        while (lstrcmp(szClassName, c_szListViewClass))
        {
            hLB = GetWindow(hLB, GW_HWNDNEXT);
            if (!hLB)
            {
                DebugMsg(DM_WARNING, TEXT("sh PRTPROP - Could not find child window"));
                goto skip_setup;
            }
        }

        // Set up the tabstop so the right hand column is 1/3 of the way
        // across the listbox.
        wXBaseUnit=LOWORD(GetDialogBaseUnits());
        GetWindowRect(hLB,&rcListBox);
        nTabStop=(rcListBox.right-rcListBox.left) * 4 / (3*wXBaseUnit);
        ListBox_SetTabStops(hLB, 1, &nTabStop);

skip_setup:
        pppi->psPrinterStuff->uFlags |= DETAIL_PORTS_INITIALIZED;
    }
#endif

    if (!(pppi->psPrinterStuff->uFlags & DETAIL_PORTS_FILLED))
    {
        TCHAR szOldPort[MAX_PATH];
        int iSel;

        if (ComboBox_GetText(hTemp, szOldPort, ARRAYSIZE(szOldPort)))
        {
            // extract the local name if we can
            // try the exact name for unc paths, otherwise "\\net\hp4" may match to "\\net\hp4si"
            iSel = ComboBox_FindStringExact(hTemp, 0, szOldPort);
            if (iSel == CB_ERR)
            {
                iSel = ComboBox_FindString(hTemp, 0, szOldPort);
            }
            if (iSel != CB_ERR)
            {
                LPTSTR lpPortName = (LPTSTR)ComboBox_GetItemData(hTemp, iSel);
                if (lpPortName)
                {
                    lstrcpy(szOldPort, lpPortName);
                }
            }
        }
        else
        {
            // edit box is empty (!DETAIL_PORTS_INITIALIZED), go with given
            lstrcpy(szOldPort, pppi->psPrinterStuff->sPrinter.pPortName);
        }

        NukePortsFromCombobox(hTemp);

        EnumNetworkPorts(hTemp, RESOURCE_CONNECTED);
        EnumNetworkPorts(hTemp, RESOURCE_REMEMBERED);
        EnumLocalPorts(hTemp);

        // REVIEW: This should probably be a FindStringExact
        if (CB_ERR == ComboBox_FindString(hTemp, 0, pppi->psPrinterStuff->sPrinter.pPortName))
        {
            // Must be a UNC -- make sure it's in the MRU
            if (!IntlStrEqNI(c_szLPT, pppi->psPrinterStuff->sPrinter.pPortName, 3))
                AddMRUString(pppi->psPrinterStuff->hmru, pppi->psPrinterStuff->sPrinter.pPortName);
        }

        // Add MRU ports (unc paths)
        if (pppi->psPrinterStuff->hmru)
        {
            TCHAR szPort[MAX_PATH];
            int i;

            for (i = EnumMRUList(pppi->psPrinterStuff->hmru, -1, NULL, 0) ;
                 i-- > 0 ; )
            {
                if (EnumMRUList(pppi->psPrinterStuff->hmru, i, szPort, ARRAYSIZE(szPort)))
                {
                    int iSel = ComboBox_FindStringExact(hTemp, 0, szPort);

                    DebugMsg(DM_TRACE,TEXT("sh PRTPROP - Checking mru unc port [%s]"), szPort);

                    // Don't add an MRU port if it's already there --
                    // this may happen if someone enters "LPT3:"
                    // before redirecting the LPT3: port
                    if (CB_ERR == iSel)
                    {
                        ComboBox_AddString(hTemp, szPort);
                    }
                    else
                    {
                        // BUGBUG: Not implemented yet
                        //DelMRUString(pppi->psPrinterStuff->hmru, iSel);
                    }
                }
            }
        }

        // select the old port
        iSel = ComboBox_FindStringExact(hTemp, 0, szOldPort);
        if (iSel == CB_ERR)
        {
            iSel = ComboBox_FindString(hTemp, 0, szOldPort);
        }
        if (iSel != CB_ERR)
        {
            ComboBox_SetCurSel(hTemp, iSel);
        }
        else
        {
            ComboBox_SetText(hTemp, szOldPort);
        }

        pppi->psPrinterStuff->uFlags |= DETAIL_PORTS_FILLED;
    }

}


BOOL Printers_EnumPrinterDriversCB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
        LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return(EnumPrinterDrivers(NULL, NULL, dwLevel, pEnum, dwSize, lpdwNeeded, lpdwNum));
}


void DetailDlg_FillDrivers(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    HWND hTemp;
    DWORD dwNum;
    DRIVER_INFO_1 * pDrivers;

    hTemp = GetDlgItem(hDlg, IDDC_DRIVER);
    ComboBox_ResetContent(hTemp);

    pDrivers = Printer_EnumProps(NULL, 1, &dwNum, Printers_EnumPrinterDriversCB, NULL);
    if (pDrivers)
    {
        for ( ; dwNum>0; )
        {
            --dwNum;
            ComboBox_AddString(hTemp, pDrivers[dwNum].pName);
        }

        if (!Printer_SelDlgComboEntry(hTemp, 0, pppi->psPrinterStuff->sPrinter.pDriverName, FALSE))
        {
            DebugMsg(TF_WARNING, TEXT("Printer is using unenumerated driver!  We'll try to force it."));
            ASSERT(FALSE);
            // BUGBUG: We probably want to select "LPT1:"
            Printer_SelDlgComboEntry(hTemp, 0, pppi->psPrinterStuff->sPrinter.pDriverName, TRUE);
        }

        LocalFree((HLOCAL)pDrivers);
    }

}


//---------------------------------------------------------------------
// Function: CreateUniqueName(lpDest,lpBaseName,wInstance)
//
// Action: Create a unique friendly name for this printer. If wInstance
//         is 0, just copy the name over. Otherwise, play some games
//         with truncating the name so it will fit.
//
// Return: TRUE if we created a name, FALSE if something went wrong
//
// NOTE: THIS CODE IS FROM MSPRINT2.DLL! We should match their naming scheme.
//
//---------------------------------------------------------------------
BOOL WINAPI CreateUniqueName(LPTSTR lpDest,
                             LPTSTR lpBaseName,
                             WORD  wInstance)
{
    BOOL bSuccess=FALSE;

    if(wInstance)
    {
        // We want to provide a fully localizable way to create a
        // unique friendly name for each instance. We start with
        // a single string from the resource, and call wsprintf
        // twice, getting something like this:
        //
        // "%%s (Copy %u)"             From resource
        // "%s (Copy 2)"               After first wsprintf
        // "Foobar Laser (Copy 2)"     After second wsprintf
        //
        // We can't make a single wsprintf call, since it has no
        // concept of limiting the string size. We truncate the
        // model name (in a DBCS-aware fashion) to the appropriate
        // size, so the whole string fits in MAXNAMELEN bytes. This
        // may cause some name truncation, but only in cases where
        // the model name is extremely long.

        TCHAR szFormat1[MAXNAMELEN];
        TCHAR szFormat2[MAXNAMELEN];

        if(LoadString(HINST_THISDLL,IDS_PRTPROP_UNIQUE_FORMAT,szFormat1,ARRAYSIZE(szFormat1)))
        {
            WORD wFormatLength;
            TCHAR szBaseName[MAXNAMELEN];

            // wFormatLength is length of format string before inserting
            // the model name. Subtract 2 to remove the "%s", and add
            // 1 to compensate for the terminating NULL, which is
            // counted in the total buffer length, but not the string length
            wFormatLength=wsprintf(szFormat2,szFormat1,wInstance+1)-1;

            lstrcpyn(szBaseName,lpBaseName,ARRAYSIZE(szBaseName)-wFormatLength);

            wsprintf(lpDest,szFormat2,(LPTSTR)szBaseName);

            bSuccess=TRUE;
        }
    }
    else
    {
        lstrcpyn(lpDest,lpBaseName,MAXNAMELEN);
        bSuccess=TRUE;
    }

    return bSuccess;
}


//-----------------------------------------------------------------------
// Function: NewFriendlyName(hDlg,lpBaseName,lpNewName)
//
// Action: Create a new (and unique) friendly name
//
// Return: TRUE if lpFriendlyName recevies new unique name, FALSE if not
//
// NOTE: THIS CODE IS FROM MSPRINT2.DLL! We should match their naming scheme.
//
//-----------------------------------------------------------------------
BOOL NewFriendlyName(LPTSTR lpBaseName,
                                LPTSTR lpNewName)
{
    TCHAR szTestName[MAXNAMELEN];
    WORD wCount;

    // Set upper limit of 1000 tries, just to avoid hanging forever

    for(wCount=0;wCount<1000;wCount++)
    {
        HANDLE hPrinter;

        if(CreateUniqueName(szTestName,lpBaseName,wCount))
        {
            hPrinter = Printer_OpenPrinter(szTestName);
            if(hPrinter)
            {
                // Name is in use--try another one
                Printer_ClosePrinter(hPrinter);
            }
            else
            {
                lstrcpyn(lpNewName,szTestName,MAXNAMELEN);
                return TRUE;
            }
        }
    }

    // Pretty unlikely to ever make it here, but just in case...
    return FALSE;
}

void DetailDlg_HandleDriverChange(HWND hDlg, LPPRINTERPAGEINFO pppi, LPTSTR pOldDriver)
{
    int len;

    // if the current printer name is built from the old driver name,
    // we want to rename this printer so nobody gets confused.
    len = lstrlen(pOldDriver);
    if (2 == CompareString(LOCALE_SYSTEM_DEFAULT, 0,
                pOldDriver, len,
                pppi->psPrinterStuff->sPrinter.pPrinterName, len))
    {
        TCHAR szNewName[MAXNAMELEN];

        if (NewFriendlyName(pppi->psPrinterStuff->sPrinter.pDriverName, szNewName))
        {
            if (NOERROR == Printer_SetNameOf(NULL, hDlg, pppi->psPrinterStuff->sPrinter.pPrinterName, szNewName, NULL))
            {
                lstrcpy(pppi->psPrinterStuff->sPrinter.pPrinterName, szNewName);
                PostMessage(GetParent(hDlg), PSM_SETTITLE, PSH_PROPTITLE,
                    (LPARAM)(pppi->psPrinterStuff->sPrinter.pPrinterName));
            }
        }
    }


    // If the DIRECT attribute is set, then the datatype
    // must be RAW... and we can't change it.
    if (!(pppi->psPrinterStuff->sPrinter.Attributes & PRINTER_ATTRIBUTE_DIRECT))
    {
        LPDRIVER_INFO_3 pDriver;

        // Always use the default datatype when the driver changes. (Kind of like
        // installing a new printer...) The real reason: print subsystem doesn't validate
        // pDatatype so we wind up with EMF in PS printers. And even if the print
        // subsystem said "hey this is invalid, use the default", then when you
        // switched back, the datatype would be RAW (and slow) instead of EMF (and fast).
        pDriver = Printer_GetPrinterDriver(pppi->psPrinterStuff->hPrinter, 3);
        if (pDriver)
        {
            lstrcpy(pppi->psPrinterStuff->sPrinter.pDatatype, pDriver->pDefaultDataType);
            LocalFree((HLOCAL)pDriver);
        }
    }

    // Save these changes so we don't rely on the user
    // pressing OK to get out of here. If there's an error
    // we reload below, so don't put up UI.
    GeneralDlg_SaveEverything(hDlg, pppi, GDSE_SAVE | GDSE_NOUI);

    // Changing the driver and datatype may affect spool settings and other junk.
    Printer_Load(pppi->psPrinterStuff);
    DetailDlg_SetFields(hDlg, pppi);

}

void DetailDlg_DriverChange(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    TCHAR szDriver[MAXNAMELEN];
    int  i;

    if (SHIsRestricted(hDlg, REST_NOPRINTERADD))
    {
        return;
    }

    Dlg_CB_GetSelText(hDlg, IDDC_DRIVER, szDriver, ARRAYSIZE(szDriver));

    if (!lstrcmp(pppi->psPrinterStuff->sPrinter.pDriverName, szDriver))
    {
        return;
    }

    i = ShellMessageBox(HINST_THISDLL, hDlg,
            MAKEINTRESOURCE(IDS_PRTPROP_DRIVER_WARN),
            MAKEINTRESOURCE(IDS_PRINTERS),
            MB_YESNO|MB_ICONEXCLAMATION);

    if (i != IDYES)
        goto error_exit;

    if (PropSheet_Apply(GetParent(hDlg)))
    {
        TCHAR szOldDriver[MAXNAMELEN];
        DECLAREWAITCURSOR;

        DebugMsg(TF_PRINTER, TEXT("saved print properties, changing driver"));

        SetWaitCursor();

        _RemovePropSheets(hDlg, pppi);

        // Don't change the driver until everything else has been saved
        // AND removed.  A driver change could adversely affect
        // the driver's pages.
        lstrcpy(szOldDriver, pppi->psPrinterStuff->sPrinter.pDriverName);
        lstrcpy(pppi->psPrinterStuff->sPrinter.pDriverName, szDriver);
        if (GeneralDlg_SaveEverything(hDlg, pppi, GDSE_SAVE))
        {
            // we couldn't save the driver!  (Bad net?)  go back to old one.
            lstrcpy(pppi->psPrinterStuff->sPrinter.pDriverName, szOldDriver);
            Printer_SelDlgComboEntry(hDlg, IDDC_DRIVER, szOldDriver, FALSE);
        }
        else
        {
            DetailDlg_HandleDriverChange(hDlg, pppi, szOldDriver);
        }

        _RebuildPropSheets(hDlg, pppi);

        ResetWaitCursor();
    }
    else
    {
error_exit:
        // reselect the old driver in the combobox
        Printer_SelDlgComboEntry(hDlg, IDDC_DRIVER,
            pppi->psPrinterStuff->sPrinter.pDriverName, FALSE);
    }
}

void DetailDlg_NewDriver(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    if (SHIsRestricted(hDlg, REST_NOPRINTERADD))
    {
        return;
    }

    if (ShellMessageBox(HINST_THISDLL, hDlg,
            MAKEINTRESOURCE(IDS_PRTPROP_DRIVER_WARN),
            MAKEINTRESOURCE(IDS_PRINTERS),
            MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
    {
        return;
    }

    if (PropSheet_Apply(GetParent(hDlg)))
    {
        TCHAR szDriver[MAXNAMELEN];
        DECLAREWAITCURSOR;

        SetWaitCursor();

        // Remove the pages before calling PrinterSetup.  Even though this
        // creates more work if the driver cannot be installed.  This will
        // give the appearance of a quicker return after the PrinterSetup
        // dialogs go away.
        _RemovePropSheets(hDlg, pppi);

        szDriver[0] = TEXT('\0');
        Printers_PrinterSetup(hDlg, MSP_NEWDRIVER, szDriver, NULL);

        if (szDriver[0])
        {
            TCHAR szOldDriver[MAXNAMELEN];

            lstrcpy(szOldDriver, pppi->psPrinterStuff->sPrinter.pDriverName);
            lstrcpy(pppi->psPrinterStuff->sPrinter.pDriverName, szDriver);
            if (GeneralDlg_SaveEverything(hDlg, pppi, GDSE_SAVE))
            {
                // we couldn't save the driver! go back to old one
                lstrcpy(pppi->psPrinterStuff->sPrinter.pDriverName, szOldDriver);
            }
            else
            {
                DetailDlg_HandleDriverChange(hDlg, pppi, szOldDriver);
            }
        }

        _RebuildPropSheets(hDlg, pppi);

        ResetWaitCursor();
    }
}


// This function puts pppi info into the dialog and
// hides/enables/disables controls as needed
void DetailDlg_SetFields(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    // turn of PSM_CHANGE messages
    pppi->psPrinterStuff->fInitializing = TRUE;

    SetDlgItemText(hDlg, IDC_PRINTER_NAME, pppi->psPrinterStuff->sPrinter.pPrinterName);

    // Port "FILE:" cannot have spool settings
    EnableWindow(GetDlgItem(hDlg, IDDB_SPOOL),
        lstrcmp(pppi->psPrinterStuff->sPrinter.pPortName, c_szFileColon));

    // 3.1 drivers show the Setup... button, 4.0 drivers do not.
    if (!(pppi->psPrinterStuff->uFlags & PS_OLDSTYLE))
        ShowWindow(GetDlgItem(hDlg, IDDB_SETUP), SW_HIDE);
    else
        ShowWindow(GetDlgItem(hDlg, IDDB_SETUP), SW_SHOWNA);

    DetailDlg_FillPorts(hDlg, pppi);
    DetailDlg_FillDrivers(hDlg, pppi);

    SetDlgItemInt(hDlg, IDC_TIMEOUT_NOTSELECTED,
        pppi->psPrinterStuff->sPrinter.DeviceNotSelectedTimeout/1000, FALSE);
    SetDlgItemInt(hDlg, IDC_TIMEOUT_TRANSRETRY,
        pppi->psPrinterStuff->sPrinter.TransmissionRetryTimeout/1000, FALSE);
    if (2 == CompareString(LOCALE_SYSTEM_DEFAULT, 0,
                pppi->psPrinterStuff->sPrinter.pPortName, 3,
                c_szLPT, 3))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_TIMEOUT_NOTSELECTED), TRUE);
        goto enable_common;
    }
    else if (2 == CompareString(LOCALE_SYSTEM_DEFAULT, 0,
                pppi->psPrinterStuff->sPrinter.pPortName, 3,
                TEXT("COM"), 3))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_TIMEOUT_NOTSELECTED), FALSE);
enable_common:
        EnableWindow(GetDlgItem(hDlg, IDC_TIMEOUT_TRANSRETRY), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_TIMEOUT_NOTSELECTED), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_TIMEOUT_TRANSRETRY), FALSE);
    }

    // BUGBUG 15512: disable the port settings button if:
    //   a) port is FILE:
    //   b) port is redirected local port
    //   c) port is not associated with a port monitor
    // (a) is trivial to check, (b) less so, and (c) even less so.
    // Do (a) for now and (b+c) if time.
    if (!lstrcmp(pppi->psPrinterStuff->sPrinter.pPortName, c_szFileColon))
    {
        EnableWindow(GetDlgItem(hDlg, IDDB_PORT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDDB_PORT), TRUE);
    }

    pppi->psPrinterStuff->fInitializing = FALSE;
}

// This function reads information from hDlg into pppi.
// returns TRUE iff an invalid state was detected
BOOL DetailDlg_GetFields(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    TCHAR szPortName[MAX_PATH];
    BOOL b;
    HWND hcbPorts = GetDlgItem(hDlg, IDDC_PRINTTO);

    b = GetPortFromCombobox(hcbPorts, szPortName, ARRAYSIZE(szPortName));
    PathRemoveBlanks(szPortName);
    if (!szPortName[0])
    {
        SetDlgFocus(hDlg, hcbPorts);
        ShellMessageBox(HINST_THISDLL, hDlg,
                MAKEINTRESOURCE(IDS_PRTPROP_PORT_ERROR),
                MAKEINTRESOURCE(IDS_PRINTERS),
                MB_OK|MB_ICONINFORMATION);
        return TRUE;
    }
    if (b)
    {
        if (!IntlStrEqNI(c_szLPT, szPortName, 3))
            AddMRUString(pppi->psPrinterStuff->hmru, szPortName);
    }
    lstrcpy(pppi->psPrinterStuff->sPrinter.pPortName, szPortName);

    // pDriverName is grabbed and verified as soon as it is changed

    pppi->psPrinterStuff->sPrinter.DeviceNotSelectedTimeout = 1000 *
        GetDlgItemInt(hDlg, IDC_TIMEOUT_NOTSELECTED, &b, FALSE);
    pppi->psPrinterStuff->sPrinter.TransmissionRetryTimeout = 1000 *
        GetDlgItemInt(hDlg, IDC_TIMEOUT_TRANSRETRY, &b, FALSE);

    return FALSE;
}


//--------------------------------------------------------------------------
// Dialog procedure of "Detail" page
//

static const DWORD aDetailHelpIds[] = {
    IDC_PRINTER_ICON,           IDH_PRTPROPS_ICON,
    IDC_PRINTER_NAME,           IDH_PRTPROPS_NAME_STATIC,
    IDD_LINE_1,                 NO_HELP,
    IDDC_PRINTTO,               IDH_PRTPROPS_PORT,
    IDDB_ADD_PORT,              IDH_PRTPROPS_NEW_PORT,
    IDDB_DEL_PORT,              IDH_PRTPROPS_DEL_PORT,
    IDDC_DRIVER,                IDH_PRTPROPS_DRIVER,
    IDDB_NEWDRIVER,             IDH_PRTPROPS_NEW_DRIVER,
    IDC_TIMEOUTSETTING,         IDH_COMM_GROUPBOX,
    IDC_TIMEOUTTEXT_1,          IDH_PRTPROPS_TIMEOUT_NOTSELECTED,
    IDC_TIMEOUTTEXT_2,          IDH_PRTPROPS_TIMEOUT_NOTSELECTED,
    IDC_TIMEOUT_NOTSELECTED,    IDH_PRTPROPS_TIMEOUT_NOTSELECTED,
    IDC_TIMEOUTTEXT_3,          IDH_PRTPROPS_TIMEOUT_TRANSRETRY,
    IDC_TIMEOUTTEXT_4,          IDH_PRTPROPS_TIMEOUT_TRANSRETRY,
    IDC_TIMEOUT_TRANSRETRY,     IDH_PRTPROPS_TIMEOUT_TRANSRETRY,
    IDDB_SPOOL,                 IDH_PRTPROPS_SPOOL_SETTINGS,
    IDDB_PORT,                  IDH_PRTPROPS_PORT_SETTINGS,
    IDDB_SETUP,                 IDH_PRTPROPS_SETUP,
    IDDB_CAPTURE_PORT,          IDH_PRTPROPS_MAP_PRN_PORT,
    IDDB_RELEASE_PORT,          IDH_PRTPROPS_UNMAP_PRN_PORT,

    0, 0
};

BOOL Printers_EnumMonitorsCB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
        LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return(EnumMonitors(NULL, dwLevel, pEnum, dwSize, lpdwNeeded, lpdwNum));
}

BOOL CALLBACK Printer_DetailDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LPPRINTERPAGEINFO pppi = (LPPRINTERPAGEINFO)GetWindowPtr(hdlg, DWLP_USER);

    switch (uMessage)
    {
    case WM_INITDIALOG:
    {
        HICON hIcon;

        pppi = (LPPRINTERPAGEINFO)lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, lParam);

        // we have to destroy the icon returned
        if (pppi->psPrinterStuff->hIcon)
            if (NULL != (hIcon = (HICON)SendDlgItemMessage(hdlg, IDC_PRINTER_ICON, STM_SETICON, (WPARAM)pppi->psPrinterStuff->hIcon, 0L)))
                DestroyIcon(hIcon);

        break;
    }

    case WM_DESTROY:
        NukePortsFromCombobox(GetDlgItem(hdlg, IDDC_PRINTTO));
        // since we're sharing our 1 hicon across 2 pages, set this to null
        // so the dialog box procedure doesn't destroy it twice (we destroy it
        // once after we return from the PropertySheet function).
        if (pppi->psPrinterStuff->hIcon)
            SendDlgItemMessage(hdlg, IDC_PRINTER_ICON, STM_SETICON, 0, 0L);
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
            (DWORD) (LPTSTR) aDetailHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD) (LPTSTR) aDetailHelpIds);
        break;

    case WM_COMMAND:
    {
        BOOL fApply = FALSE; // Do we want to turn on the Apply button?

        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {

        case IDDC_DRIVER:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
            {
                DetailDlg_DriverChange(hdlg, pppi);
                // don't need fApply -- _DriverChange saves everything
            }
            break;

        case IDDC_PRINTTO:
            // most of the time when we get a CBN_KILLFOCUS, nothing has
            // happened... we only need to do any work after a CBN_EDITCHANGE
            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_EDITCHANGE)
            {
                pppi->psPrinterStuff->fPortModified = TRUE;
                fApply = TRUE;
            }

            if ((GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)     ||
                ((GET_WM_COMMAND_CMD(wParam, lParam) == CBN_KILLFOCUS) &&
                 pppi->psPrinterStuff->fPortModified                     )  )
            {
                TCHAR szPortName[MAX_PATH];

                pppi->psPrinterStuff->fPortModified = FALSE;
UpdatePrintTo:
                // save old port name
                lstrcpy(szPortName, pppi->psPrinterStuff->sPrinter.pPortName);

                // The port may have changed.  Update timeout settings and the
                // spool settings button.  Save code by calling _SetFields.
                if (!DetailDlg_GetFields(hdlg, pppi))
                    DetailDlg_SetFields(hdlg, pppi);

                fApply = lstrcmp(szPortName, pppi->psPrinterStuff->sPrinter.pPortName);
            }
            break;

        case IDDB_NEWDRIVER:
            DetailDlg_NewDriver(hdlg, pppi);
            // don't need fApply -- _NewDriver saves everything
            break;

        case IDDB_ADD_PORT:
            // This may update the contents/selection of the ports combobox
            if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_ADDPORT),
                        hdlg, Printer_AddPortDlgProc,
                        (LPARAM)GetDlgItem(hdlg, IDDC_PRINTTO)))
            {
                goto UpdatePrintTo; // turns on fApply
            }
            break;

        case IDDB_DEL_PORT:
        {
            DELPORTDLG dpd;

            dpd.pppi = pppi;
            dpd.hwndCB = GetDlgItem(hdlg, IDDC_PRINTTO);

            // This may update the contents/selection of the ports combobox
            if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_DELPORT),
                        hdlg, Printer_DelPortDlgProc,
                        (LPARAM)&dpd))
            {
                goto UpdatePrintTo; // turns on fApply
            }
            break;
        }

        case IDDB_PORT:
          {
            TCHAR szPrintTo[MAX_PATH];

            GetPortFromCombobox(GetDlgItem(hdlg, IDDC_PRINTTO),
                szPrintTo, ARRAYSIZE(szPrintTo));

            ConfigurePort(NULL, hdlg, szPrintTo);
            break;
          }

        case IDDB_CAPTURE_PORT:
            SHNetConnectionDialog(hdlg, NULL, RESOURCETYPE_PRINT);
            pppi->psPrinterStuff->uFlags &= ~DETAIL_PORTS_FILLED;
            DetailDlg_FillPorts(hdlg, pppi);
            break;

        case IDDB_RELEASE_PORT:
            WNetDisconnectDialog(hdlg, RESOURCETYPE_PRINT);
            pppi->psPrinterStuff->uFlags &= ~DETAIL_PORTS_FILLED;
            DetailDlg_FillPorts(hdlg, pppi);
            break;

        case IDDB_SPOOL:
            fApply = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_SPOOLSETTINGS), hdlg,
                           Printer_SpoolSettingsDlgProc, (LPARAM)pppi) > 0;
            break;

        case IDDB_SETUP:
            // In the !PS_OLDSTYLE case we hide the window, but the
            // control's mnemonic key still works. We only want IDDB_SETUP
            // to work in the PS_OLDSTYLE case.
            if (pppi->psPrinterStuff->uFlags & PS_OLDSTYLE)
            {
                if (!PrinterProperties(hdlg, pppi->psPrinterStuff->hPrinter))
                {
                    ShellMessageBox(HINST_THISDLL, hdlg,
                        MAKEINTRESOURCE(IDS_CANTOPENDRIVERPROP),
                        MAKEINTRESOURCE(IDS_PRINTERS), MB_OK|MB_ICONSTOP);
                }
            }
            break;

        case IDC_TIMEOUT_NOTSELECTED:
        case IDC_TIMEOUT_TRANSRETRY:
            fApply = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;
            break;

        default:
            break;
        }

        if (fApply && !pppi->psPrinterStuff->fInitializing)
        {
            PostMessage(GetParent(hdlg), PSM_CHANGED, (WPARAM)hdlg, 0);
        }

        break;
    }

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_KILLACTIVE:
            // User is leaving this page.
            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, DetailDlg_GetFields(hdlg, pppi));
            break;

        case PSN_APPLY:
            // save everything on OK or Apply Now
            PrtProp_Apply(hdlg, pppi);
            break;

        case PSN_SETACTIVE:
            // Update information that may have changed on the General page
            pppi->psPrinterStuff->fSaveEverything = TRUE;
            DetailDlg_SetFields(hdlg, pppi);
            ASSERT(pppi->psPrinterStuff->fPortModified == FALSE);
            pppi->psPrinterStuff->fPortModified = FALSE;
            // We need this because _SetFields trashes DWLP_MSGRESULT...
            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


//==========================================================================
// General page
//==========================================================================

void GeneralDlg_FillSeparator(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    HWND hTemp;
    TCHAR buf[128];
    UINT id;

    hTemp = GetDlgItem(hDlg, IDDC_SEPARATOR);
    ComboBox_ResetContent(hTemp);

    for (id = IDS_PRNSEP_FULL ; id >= IDS_PRNSEP_NONE ; id--)
    {
        LoadString(HINST_THISDLL, id, buf, ARRAYSIZE(buf));
        ComboBox_AddString(hTemp, buf);
    }

    if (pppi->psPrinterStuff->sPrinter.pSepFile[0])
        Printer_SelDlgComboEntry(hTemp, 0, pppi->psPrinterStuff->sPrinter.pSepFile, TRUE);
    else
        Printer_SelDlgComboEntry(hTemp, 0, buf, FALSE);
}

void GeneralDlg_SetFields(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    // turn off PSM_CHANGE messages
    pppi->psPrinterStuff->fInitializing = TRUE;

    SetDlgItemText(hDlg, IDC_PRINTER_NAME, pppi->psPrinterStuff->sPrinter.pPrinterName);
    SetDlgItemText(hDlg, IDGE_COMMENT, pppi->psPrinterStuff->sPrinter.pComment);
    GeneralDlg_FillSeparator(hDlg, pppi);

    pppi->psPrinterStuff->fInitializing = FALSE;
}

// returns TRUE iff invalid information detected
BOOL GeneralDlg_GetFields(HWND hDlg, LPPRINTERPAGEINFO pppi)
{
    GetDlgItemText(hDlg, IDGE_COMMENT, pppi->psPrinterStuff->sPrinter.pComment, ARRAYSIZE(pppi->psPrinterStuff->sPrinter.pComment));

    Dlg_CB_GetSelText(hDlg, IDDC_SEPARATOR,
        pppi->psPrinterStuff->sPrinter.pSepFile,
        ARRAYSIZE(pppi->psPrinterStuff->sPrinter.pSepFile));

    return FALSE;
}

// fSaveMe == GDSE_SAVE => save changes and
//      return PSNRET_NOERROR, PSNRET_INVALID, or PSNRET_INVALID_NOCHANGEPAGE
// fSaveMe == GDSE_QUERY_TESTPAGE => don't save anything and
//      return TRUE iff a "test page state change" has been entered
// fSaveMe == GDSE_QUERY_SAVE => don't save anything and
//      return TRUE iff some change exists
LRESULT GeneralDlg_SaveEverything(HWND hDlg, LPPRINTERPAGEINFO pppi, int fSaveMe)
{
    LPPRINTER_INFO_2 pPrinter;
    LPPRINTER_INFO_5 pPrinter5;
    LONG lRet = PSNRET_NOERROR;
    BOOL fError = FALSE;
    BOOL fTestPage = FALSE;
    HWND hTempEditBox = NULL;     // save some code
    HWND hTempComboBox = NULL;
    WPARAM wPage;
    LPCTSTR lpError;
    BOOL fSave = FALSE;
    BOOL fPortChange = FALSE;
    DWORD dwOldAttributes; // old info, valid iff fPortChange
    TCHAR szOldPortName[MAX_PATH]; // old info, valid iff fPortChange

    ASSERT(((fSaveMe & GDSE_SAVE|GDSE_QUERY_TESTPAGE) == GDSE_SAVE) ||
           ((fSaveMe & GDSE_SAVE|GDSE_QUERY_TESTPAGE) == GDSE_QUERY_TESTPAGE));

    pPrinter = Printer_GetPrinterInfo(pppi->psPrinterStuff->hPrinter, 2);
    if (pPrinter)
    {
        LPPRINTER_INFO_2_COPY pCopy = &(pppi->psPrinterStuff->sPrinter);

        if (lstrcmp(pPrinter->pPortName, pCopy->pPortName)
            && pCopy->pPortName[0])
        {
            TCHAR newPrinterPort[ARRAYSIZE(pCopy->pPrinterName) +
                                ARRAYSIZE(pCopy->pPortName)];
            TCHAR oldPrinterPort[ARRAYSIZE(pCopy->pPrinterName) +
                                ARRAYSIZE(pCopy->pPortName)];
            LPTSTR pNewInfo;
            LPTSTR pOldInfo;
            int i;

            // If there's a port change, call the copyhook right away.
            // If we are going to proceed, we have to re-get the PRINTER_INFO_2,
            // since the copyhook may have modified it.  (Assume that the name
            // and port won't change.)

            // the copyhook receives a doubly-null
            // terminated list of "printername\0port\0\0", so muck with
            // the strings.

            lstrcpy(newPrinterPort,pPrinter->pPrinterName);
            i = lstrlen(newPrinterPort)+1;
            lstrcpy(&newPrinterPort[i],pCopy->pPortName);
            i = i+lstrlen(&newPrinterPort[i])+1;
            newPrinterPort[i] = TEXT('\0');

            lstrcpy(oldPrinterPort,pPrinter->pPrinterName);
            i = lstrlen(oldPrinterPort)+1;
            lstrcpy(&oldPrinterPort[i],pPrinter->pPortName);
            i = i+lstrlen(&oldPrinterPort[i])+1;
            oldPrinterPort[i] = TEXT('\0');

            pNewInfo = newPrinterPort;
            pOldInfo = oldPrinterPort;

            if (fSaveMe == GDSE_SAVE)
            {
                if (CallPrinterCopyHooks(hDlg, PO_PORTCHANGE, 0,
                        pNewInfo, 0, pOldInfo, 0)
                    != IDYES)
                {
                    // user canceled a shared printer port change, bail.
                    return(PSNRET_INVALID_NOCHANGEPAGE);
                }

                // get the PRINTER_INFO_2 again, since the attributes/status may
                // have been changed by the called CopyHook.
                LocalFree((HLOCAL)pPrinter);
                pPrinter = Printer_GetPrinterInfo(pppi->psPrinterStuff->hPrinter, 2);
                if (!pPrinter)
                    goto handle_error;
            }

            // flag port changes so we can update links to printers!
            // keep track of relevant info.
            fPortChange = TRUE;
            dwOldAttributes = pPrinter->Attributes;
            lstrcpy(szOldPortName, pPrinter->pPortName);

            // set the pPortName AFTER we get the new PRINTER_INFO_2. duh.
            pPrinter->pPortName = pCopy->pPortName;
            fSave = TRUE;
            fTestPage = TRUE;
        }

        if (pPrinter->pComment == NULL)
        {
            if (pCopy->pComment[0])
                goto save_comment;
        }
        else if (lstrcmp(pPrinter->pComment, pCopy->pComment))
        {
save_comment:
            pPrinter->pComment = pCopy->pComment;
            fSave = TRUE;
        }

        if (lstrcmp(pPrinter->pDriverName, pCopy->pDriverName)
            && pCopy->pDriverName[0])
        {
            pPrinter->pDriverName = pCopy->pDriverName;
            fSave = TRUE;
            fTestPage = TRUE;
        }

        {
            TCHAR buf[128];
            buf[0]= TEXT('\0');
            LoadString(HINST_THISDLL, IDS_PRNSEP_NONE, buf, ARRAYSIZE(buf));

            if (pPrinter->pSepFile == NULL || *pPrinter->pSepFile == TEXT('\0'))
            {
                if (pCopy->pSepFile[0] && lstrcmp(pCopy->pSepFile, buf))
                    goto save_sepfile;
            }
            else if (lstrcmp(pPrinter->pSepFile, pCopy->pSepFile))
            {
save_sepfile:
                if (!lstrcmp(pCopy->pSepFile, buf))
                {
                    pPrinter->pSepFile = (LPTSTR)c_szNULL;
                }
                else
                {
                    pPrinter->pSepFile = pCopy->pSepFile;
                    fTestPage = TRUE;
                }
                fSave = TRUE;
            }
        }

        if (pPrinter->pDatatype == NULL)
        {
            if (pCopy->pDatatype[0])
                goto save_datatype;
        }
        else if (lstrcmp(pPrinter->pDatatype, pCopy->pDatatype))
        {
save_datatype:
            pPrinter->pDatatype = pCopy->pDatatype;
            fSave = TRUE;
            fTestPage = TRUE;
        }

        if ((pPrinter->Attributes & PS_ATTRIBUTE_MASK) !=
            (pCopy->Attributes & PS_ATTRIBUTE_MASK))
        {
            pPrinter->Attributes = (pPrinter->Attributes & ~PS_ATTRIBUTE_MASK)
                | (pCopy->Attributes & PS_ATTRIBUTE_MASK);
            fSave = TRUE;
            fTestPage = TRUE;
        }

        if (fSave && (fSaveMe == GDSE_SAVE))
        {

            DebugMsg(TF_PRINTER, TEXT("Saving printer property sheet info_2"));
            fError = !SetPrinter(pppi->psPrinterStuff->hPrinter, 2, (LPBYTE)pPrinter, 0);

        } // if (fSave)

        LocalFree((HLOCAL)pPrinter);

        // don't save PRINTER_INFO_5 if PRINTER_INFO_2 failed
        if (fError)
            goto handle_error;

        pPrinter5 = Printer_GetPrinterInfo(pppi->psPrinterStuff->hPrinter, 5);
        if (pPrinter5)
        {
            if (fSaveMe == GDSE_SAVE)
            {
                // update our internal Attributes state, since it may have
                // changed from the above SetPrinter(2) call.
                pppi->psPrinterStuff->sPrinter.Attributes = pPrinter5->Attributes;
            }

            if (pPrinter5->DeviceNotSelectedTimeout != pCopy->DeviceNotSelectedTimeout
             || pPrinter5->TransmissionRetryTimeout != pCopy->TransmissionRetryTimeout)
            {
                pPrinter5->DeviceNotSelectedTimeout = pCopy->DeviceNotSelectedTimeout;
                pPrinter5->TransmissionRetryTimeout = pCopy->TransmissionRetryTimeout;
                fSave = TRUE;
                fTestPage = TRUE;

                if (fSaveMe == GDSE_SAVE)
                {
                    DebugMsg(TF_PRINTER, TEXT("Saving printer property sheet info_5"));
                    fError = !SetPrinter(pppi->psPrinterStuff->hPrinter, 5, (LPBYTE)pPrinter5, 0);
                }
            } // if PRINTER_INFO_5 change

            LocalFree((HLOCAL)pPrinter5);

            if (fError)
            {
                // we're in a mixed state, but there's not much we can do about
                // it now...
                goto handle_error;
            }
        }
        else
        {
            // Assuming that if we got a PRINTER_INFO_2, then we'd be able
            // to get a PRINTER_INFO_5 implies that we'd never get here.
            DebugMsg(TF_PRINTER, TEXT("Couldn't get PRINTER_INFO_5, some info may not be saved (unlikely)"));
        }

        if (fPortChange && (fSaveMe == GDSE_SAVE))
        {
            // if the port change causes an icon change, update the correct icon images
            if (((dwOldAttributes & PRINTER_ATTRIBUTE_NETWORK) !=
                 (pppi->psPrinterStuff->sPrinter.Attributes & PRINTER_ATTRIBUTE_NETWORK)) ||
                (!lstrcmp(c_szFileColon, szOldPortName) ||
                 !lstrcmp(c_szFileColon, pCopy->pPortName)))
            {
                TCHAR szModule[MAX_PATH];
                LPTSTR pszModule;
                int iIcon1, iIcon2;
                int iImage;

                GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));
                pszModule = PathFindFileName(szModule);

                if (dwOldAttributes & PRINTER_ATTRIBUTE_NETWORK)
                {
                    DebugMsg(DM_TRACE, TEXT("sh PRTPROP - sending icon change for network printer %s"), pppi->psPrinterStuff->sPrinter.pPrinterName);
                    iIcon1 = IDI_PRINTER_NET;
                    iIcon2 = IDI_DEF_PRINTER_NET;
                }
                else if (!lstrcmp(c_szFileColon, szOldPortName))
                {
                    DebugMsg(DM_TRACE, TEXT("sh PRTPROP - sending icon change for file printer %s"), pppi->psPrinterStuff->sPrinter.pPrinterName);
                    iIcon1 = IDI_PRINTER_FILE;
                    iIcon2 = IDI_DEF_PRINTER_FILE;
                }
                else
                {
                    DebugMsg(DM_TRACE, TEXT("sh PRTPROP - sending icon change for local printer %s"), pppi->psPrinterStuff->sPrinter.pPrinterName);
                    iIcon1 = IDI_PRINTER;
                    iIcon2 = IDI_DEF_PRINTER;
                }

                iImage = LookupIconIndex(pszModule, EIRESID(iIcon1), 0);
                if (iImage != -1)
                {
                    SHUpdateImage( pszModule, EIRESID(iIcon1), 0, iImage );
//                    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)iImage, NULL);
                }
                iImage = LookupIconIndex(pszModule, EIRESID(iIcon2), 0);
                if (iImage != -1)
                {
                    SHUpdateImage( pszModule, EIRESID(iIcon2), 0, iImage );
//                    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)iImage, NULL);
                }
            }
        }

    } // if (pPrinter)
    else
    {
        DWORD dwLastError;

handle_error:

        if (fSaveMe & GDSE_NOUI)
        {
            return(PSNRET_INVALID);
        }

        lRet = PSNRET_INVALID;
        dwLastError = GetLastError();

        switch (dwLastError)
        {
        case ERROR_UNKNOWN_PORT:
            // The user tried to change to an invalid port
            hTempComboBox = GetDlgItem(hDlg, IDDC_PRINTTO);
            wPage = 1;
            lpError = MAKEINTRESOURCE(IDS_PRTPROP_PORT_ERROR);
            break;

        case ERROR_INVALID_SEPARATOR_FILE:
            // The user selected an invalid separator page
            hTempComboBox = GetDlgItem(hDlg, IDDC_SEPARATOR);
            wPage = 1;
            lpError = MAKEINTRESOURCE(IDS_PRTPROP_SEP_ERROR);
            break;

        default:
        {
            TCHAR szTemplate[128];
            TCHAR buf[MAX_PATH];

            // Default message if FormatMessage doesn't recognize dwLastError
            LoadString(HINST_THISDLL, IDS_PRTPROP_UNKNOWNERROR, szTemplate, ARRAYSIZE(szTemplate));
            wsprintf(buf, szTemplate, dwLastError);

            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0L,
                    buf, ARRAYSIZE(buf), NULL);
            ShellMessageBox(HINST_THISDLL, hDlg,
                MAKEINTRESOURCE(IDS_PRTPROP_UNKNOWN_ERROR),
                MAKEINTRESOURCE(IDS_PRINTERS),
                MB_OK|MB_ICONEXCLAMATION, buf);

            return(PSNRET_INVALID);
            break;
        }
        } // switch (GetLastError())

        SendMessage(GetParent(hDlg), PSM_SETCURSEL, wPage, 0L);
        if (hTempEditBox)
        {
            SetDlgFocus(hDlg, hTempEditBox);
            Edit_SetSel(hTempEditBox, 0, -1);
        }
        if (hTempComboBox)
        {
            SetDlgFocus(hDlg, hTempComboBox);
            ComboBox_SetEditSel(hTempComboBox, 0, -1);
        }
        ShellMessageBox(HINST_THISDLL, hDlg, lpError,
            MAKEINTRESOURCE(IDS_PRINTERS),
            MB_OK|MB_ICONEXCLAMATION);

        return(PSNRET_INVALID_NOCHANGEPAGE);
    }

    switch (fSaveMe)
    {
    case GDSE_SAVE:             return(lRet);
    case GDSE_QUERY_TESTPAGE:   return(fTestPage);
//  not currently used, but it may be useful some day
//  case GDSE_QUERY_SAVE:       return(fSave);
    default:                    ASSERT(FALSE);
    }

    return PSNRET_INVALID;	// Should never get here but this exist for the compiler's sakes.
}

//--------------------------------------------------------------------------
// Dialog procedure of "General" page
//

static const DWORD aGeneralHelpIds[] = {
    IDD_LINE_1,                 NO_HELP,
    IDD_LINE_2,                 NO_HELP,
    IDD_LINE_3,                 NO_HELP,
    IDC_PRINTER_ICON,           IDH_PRTPROPS_ICON,
    IDC_PRINTER_NAME,           IDH_PRTPROPS_NAME_STATIC,
    IDGS_TYPE_TXT,              IDH_PRTPROPS_TYPE_LOCATION,
    IDGS_TYPE,                  IDH_PRTPROPS_TYPE_LOCATION,
    IDGS_LOCATION_TXT,          IDH_PRTPROPS_TYPE_LOCATION,
    IDGS_LOCATION,              IDH_PRTPROPS_TYPE_LOCATION,
    IDGE_COMMENT_TXT,           IDH_PRTPROPS_COMMENT,
    IDGE_COMMENT,               IDH_PRTPROPS_COMMENT,
    IDDB_TESTPAGE,              IDH_PRTPROPS_TEST_PAGE,
    IDDC_SEPARATOR_TXT,         IDH_PRTPROPS_SEPARATOR,
    IDDC_SEPARATOR,             IDH_PRTPROPS_SEPARATOR,
    IDDB_CHANGESEPARATOR,       IDH_PRTPROPS_SEPARATOR_BROWSE,

    0, 0
};

BOOL CALLBACK Printer_GeneralDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LPPRINTERPAGEINFO pppi = (LPPRINTERPAGEINFO)GetWindowPtr(hdlg, DWLP_USER);

    switch (uMessage)
    {
    case WM_INITDIALOG:
    {
        HICON hIcon;

        pppi = (LPPRINTERPAGEINFO)lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, lParam);

        SendDlgItemMessage(hdlg, IDDC_SEPARATOR, CB_LIMITTEXT,
                SIZEOF(pppi->psPrinterStuff->sPrinter.pSepFile)-1, 0L);
        if (pppi->psPrinterStuff->sPrinter.Attributes & PRINTER_ATTRIBUTE_NETWORK)
        {
            EnableWindow(GetDlgItem(hdlg, IDDC_SEPARATOR_TXT), FALSE);
            EnableWindow(GetDlgItem(hdlg, IDDC_SEPARATOR), FALSE);
            EnableWindow(GetDlgItem(hdlg, IDDB_CHANGESEPARATOR), FALSE);
        }

        if (pppi->psPrinterStuff->hIcon)
            if (NULL != (hIcon = (HICON)SendDlgItemMessage(hdlg, IDC_PRINTER_ICON, STM_SETICON, (WPARAM)pppi->psPrinterStuff->hIcon, 0L)))
                DestroyIcon(hIcon);
        break;
    }
    case WM_CLOSE:
        // MLE controls generate a WM_CLOSE message when the user hits
        // the escape key, so post one up to our parent to let him
        // process it...
        PostMessage(GetParent(hdlg), WM_CLOSE, wParam, lParam);
        break;

    case WM_DESTROY:
        if (pppi->psPrinterStuff->hIcon)
            SendDlgItemMessage(hdlg, IDC_PRINTER_ICON, STM_SETICON, 0, 0L);
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
            (DWORD) (LPTSTR) aGeneralHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD) (LPTSTR) aGeneralHelpIds);
        break;

    case WM_COMMAND:
    {
        BOOL fApply = FALSE; // turn on Apply button?

        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDDB_TESTPAGE:
                if (GeneralDlg_SaveEverything(hdlg, pppi, GDSE_QUERY_TESTPAGE))
                {
                    // State may have changed, so TestPage may not work
                    if (IDYES != ShellMessageBox(HINST_THISDLL, hdlg,
                            MAKEINTRESOURCE(IDS_PRTPROP_TESTPAGE_WARN),
                            MAKEINTRESOURCE(IDS_PRINTERS),
                            MB_YESNO|MB_ICONEXCLAMATION))
                    {
                        break;
                    }
                }
                SHInvokePrinterCommand(hdlg, PRINTACTION_TESTPAGE, pppi->psPrinterStuff->sPrinter.pPrinterName, NULL, FALSE);
                break;

            case IDDB_CHANGESEPARATOR:
            {
                TCHAR buf[MAX_PATH];

                buf[0] = TEXT('\0');
                if (_GetFileNameFromBrowse(hdlg, buf, ARRAYSIZE(buf), NULL,
                        MAKEINTRESOURCE(IDS_CLP),
                        MAKEINTRESOURCE(IDS_SEPARATORFILTER),
                        MAKEINTRESOURCE(IDS_BROWSE),
                        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST))

                {
                    HWND hcbSeparator = GetDlgItem(hdlg, IDDC_SEPARATOR);

                    ComboBox_SetCurSel(hcbSeparator,
                            ComboBox_AddString(hcbSeparator, buf));
                    fApply = TRUE;
                }
                break;
            }

            case IDGE_COMMENT:
                fApply = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;
                break;

            case IDDC_SEPARATOR:
                fApply = GET_WM_COMMAND_CMD(wParam, lParam) == CBN_EDITCHANGE ||
                         GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE;
                break;
        }

        if (fApply && !pppi->psPrinterStuff->fInitializing)
        {
            PostMessage(GetParent(hdlg), PSM_CHANGED, (WPARAM)hdlg, 0);
        }

        break;
    }

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_KILLACTIVE:
            // validate entries when leaving this page
            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, GeneralDlg_GetFields(hdlg, pppi));
            break;

        case PSN_APPLY:
            // save everything on OK or Apply Now
            PrtProp_Apply(hdlg, pppi);
            break;

        case PSN_SETACTIVE:
            pppi->psPrinterStuff->fSaveEverything = TRUE;
            GeneralDlg_SetFields(hdlg, pppi);
            // We need this because _SetFields trashes DWLP_MSGRESULT...
            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


//==========================================================================
// Spool Settings dialog
//==========================================================================

#define IDDC_DATATYPE           cmb1
#define IDGRB_SPOOL             rad1
#define IDGRB_DIRECT            rad2
#define IDGRB_DESPOOL_LATER     rad3
#define IDGRB_DESPOOL_NOW       rad4

BOOL Printers_EnumPrintProcessorDatatypesCB(LPVOID lpData, HANDLE hPrinter,
        DWORD dwLevel, LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return(EnumPrintProcessorDatatypes(NULL, lpData, dwLevel, pEnum,
        dwSize, lpdwNeeded, lpdwNum));
}

// GDI hardcodes these strings
TCHAR const c_szEMF[] = TEXT("EMF");
TCHAR const c_szRAW[] = TEXT("RAW");

void DetailDlg_FillDatatypes(HWND hDlg, LPPRINTERSTUFF psPrinterStuff)
{
    HWND hcbDatatypes;
    DWORD dwNum;
    DATATYPES_INFO_1 * pDataTypes;
    int iSel;
    int numDatatypes;

    hcbDatatypes = GetDlgItem(hDlg, IDDC_DATATYPE);
    ComboBox_ResetContent(hcbDatatypes);

    // Get datatypes supported by print processor; set the ItemData to FALSE
    pDataTypes = Printer_EnumProps(NULL, 1, &dwNum,
            Printers_EnumPrintProcessorDatatypesCB,
            (LPVOID)psPrinterStuff->sPrinter.pPrintProcessor);
    if (pDataTypes)
    {
        for ( ; dwNum>0; )
        {
            --dwNum;
            ComboBox_SetItemData(hcbDatatypes,
                ComboBox_AddString(hcbDatatypes, pDataTypes[dwNum].pName),
                FALSE);

            DebugMsg(DM_TRACE, TEXT("sh FILLDATATYPE - adding %s"), pDataTypes[dwNum].pName);
        }

        LocalFree((HLOCAL)pDataTypes);
    }

    // make sure driver supports EMF. If not, make sure not in datatype list.
    iSel = ComboBox_FindString(hcbDatatypes, 0, c_szEMF);
    if (iSel != -1)
    {
        // BUGBUG: old MicroGrafx drivers have a bug in which they always
        //         dereference pDevMode. Is it too much effort to get a devmode?
        //         Or should we just blow off this old driver?
        BOOL fEMF = _DeviceCapabilities(psPrinterStuff->sPrinter.pPrinterName,
            psPrinterStuff->sPrinter.pPortName, DC_EMF_COMPLIANT, NULL, NULL) > 0;
        if (!fEMF)
        {
            // remove EMF if driver doesn't support it
            ComboBox_DeleteString(hcbDatatypes, iSel);
            DebugMsg(DM_TRACE, TEXT("sh FILLDATATYPE - deleted EMF"));
        }
        else
        {
            // mark EMF as supported so it doesn't get removed later
            ComboBox_SetItemData(hcbDatatypes, iSel, TRUE);
            DebugMsg(DM_TRACE, TEXT("sh FILLDATATYPE - support EMF"));
        }
    }

    numDatatypes = _DeviceCapabilities(psPrinterStuff->sPrinter.pPrinterName,
        psPrinterStuff->sPrinter.pPortName, DC_DATATYPE_PRODUCED, NULL, NULL);
    if (numDatatypes > 0)
    {
        LPTSTR pDatatypesGenerated = (void*)LocalAlloc(LPTR, numDatatypes * DATATYPE_NAMELEN);
        if (pDatatypesGenerated)
        {
            if (-1 != _DeviceCapabilities(psPrinterStuff->sPrinter.pPrinterName,
                psPrinterStuff->sPrinter.pPortName, DC_DATATYPE_PRODUCED,
                pDatatypesGenerated, NULL))
            {
                LPTSTR pDatatype = pDatatypesGenerated;

                while (numDatatypes--)
                {
                    iSel = ComboBox_FindString(hcbDatatypes, 0, pDatatype);
                    if (iSel != -1)
                    {
                        // mark pDatatype as supported
                        ComboBox_SetItemData(hcbDatatypes, iSel, TRUE);
                        DebugMsg(DM_TRACE, TEXT("sh FILLDATATYPE - support %s"), pDatatype);
                    }
                    pDatatype += DATATYPE_NAMELEN;
                }

                // remove all unsupported datatypes
                for (numDatatypes = ComboBox_GetCount(hcbDatatypes) ;
                     numDatatypes > 0 ;
                     numDatatypes -- )
                {
                    if (!ComboBox_GetItemData(hcbDatatypes, numDatatypes))
                    {
                        ComboBox_DeleteString(hcbDatatypes, numDatatypes);
                        DebugMsg(DM_TRACE, TEXT("sh FILLDATATYPE - deleted %s"), pDatatype);
                    }
                }
            }
            LocalFree((HLOCAL)pDatatypesGenerated);
        }
    }

    // select the datatype
    if (!Printer_SelDlgComboEntry(hcbDatatypes, 0, psPrinterStuff->sPrinter.pDatatype, FALSE))
    {
        LPDRIVER_INFO_3 pDriver;

        // it's not in the list, try the default datatype
        pDriver = Printer_GetPrinterDriver(psPrinterStuff->hPrinter, 3);
        if (pDriver)
        {
            Printer_SelDlgComboEntry(hcbDatatypes, 0, pDriver->pDefaultDataType, TRUE);
            LocalFree((HLOCAL)pDriver);
        }
    }
}


//--------------------------------------------------------------------------
// Dialog procedure of "Add Port" dialog
//

const static DWORD aAddPortHelpIDs[] = {  // Context Help IDs
     IDD_ADDPORT_NETWORK,   IDH_ADDPORT_NETWORK,
     IDD_ADDPORT_PORTMON,   IDH_ADDPORT_PORTMON,
     IDD_ADDPORT_NETPATH,   IDH_ADDPORT_NETPATH,
     IDD_ADDPORT_BROWSE,    IDH_ADDPORT_BROWSE,
     IDD_ADDPORT_LB,        IDH_ADDPORT_LB,

     0, 0
};

BOOL CALLBACK Printer_AddPortDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndLB;

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            LPMONITOR_INFO_1 pMonitors;
            DWORD dwNum;

            // store dest HWND away
            SetWindowLongPtr(hdlg, DWLP_USER, lParam);

            // select network radio button
            CheckRadioButton(hdlg, IDD_ADDPORT_NETWORK, IDD_ADDPORT_PORTMON,
                IDD_ADDPORT_NETWORK);

            // fill portmon listbox with port monitors
            hwndLB = GetDlgItem(hdlg, IDD_ADDPORT_LB);
            pMonitors = Printer_EnumProps(NULL, 1, &dwNum, Printers_EnumMonitorsCB, NULL);
            if (pMonitors)
            {
                int i;

                for (i = 0 ; i < (int)dwNum ; i++)
                {
                    ListBox_AddString(hwndLB, pMonitors[i].pName);
                }
                ListBox_SetCurSel(hwndLB, 0);

                LocalFree((HLOCAL)pMonitors);
            }

            SHAutoComplete(GetDlgItem(hdlg, IDD_ADDPORT_NETPATH), 0);

            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD)(LPTSTR) aAddPortHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD)(LPVOID)aAddPortHelpIDs);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDD_ADDPORT_NETWORK:
                    CheckRadioButton(hdlg, IDD_ADDPORT_NETWORK, IDD_ADDPORT_PORTMON,
                        IDD_ADDPORT_NETWORK);
                    EnableWindow(GetDlgItem(hdlg, IDD_ADDPORT_NETPATH), TRUE);
                    EnableWindow(GetDlgItem(hdlg, IDD_ADDPORT_BROWSE), TRUE);
                    EnableWindow(GetDlgItem(hdlg, IDD_ADDPORT_LB), FALSE);
                    break;

                case IDD_ADDPORT_PORTMON:
                    CheckRadioButton(hdlg, IDD_ADDPORT_NETWORK, IDD_ADDPORT_PORTMON,
                        IDD_ADDPORT_PORTMON);
                    EnableWindow(GetDlgItem(hdlg, IDD_ADDPORT_NETPATH), FALSE);
                    EnableWindow(GetDlgItem(hdlg, IDD_ADDPORT_BROWSE), FALSE);
                    EnableWindow(GetDlgItem(hdlg, IDD_ADDPORT_LB), TRUE);
                    break;

                case IDD_ADDPORT_BROWSE:
                {
                    LPITEMIDLIST pidlNetPrinter;
                    BROWSEINFO   bi;
                    TCHAR         szBuf[MAX_PATH];

                    LoadString(HINST_THISDLL,IDS_BROWSE,szBuf,ARRAYSIZE(szBuf));

                    ZeroMemory(&bi,SIZEOF(BROWSEINFO));
                    bi.hwndOwner=hdlg;
                    bi.pidlRoot=(LPITEMIDLIST)MAKEINTRESOURCE(CSIDL_NETWORK);
                    bi.lpszTitle=szBuf;
                    bi.ulFlags=BIF_BROWSEFORPRINTER;

                    pidlNetPrinter = SHBrowseForFolder(&bi);
                    if (pidlNetPrinter)
                    {
                        if (SHGetPathFromIDList(pidlNetPrinter,szBuf))
                        {
                            Edit_SetText(GetDlgItem(hdlg, IDD_ADDPORT_NETPATH), szBuf);
                        }
                        ILFree(pidlNetPrinter);
                    }
                    break;
                }

                case IDOK:
                {
                    TCHAR szBuf[MAX_PATH];
                    HWND hwndCB;
                    int i;

                    if (IsDlgButtonChecked(hdlg, IDD_ADDPORT_NETWORK))
                    {
                        HANDLE hPrinter;

                        Edit_GetText(GetDlgItem(hdlg, IDD_ADDPORT_NETPATH),
                                szBuf, ARRAYSIZE(szBuf));

                        DebugMsg(DM_TRACE, TEXT("sh ADDPORT -- adding UNC port '%s'"), szBuf);

                        // validate port
                        hPrinter = Printer_OpenPrinter(szBuf);
                        if (hPrinter)
                        {
                            Printer_ClosePrinter(hPrinter);
                        }
                        else
                        {
                            ShellMessageBox(HINST_THISDLL, hdlg,
                                MAKEINTRESOURCE(IDS_PRTPROP_PORT_ERROR),
                                MAKEINTRESOURCE(IDS_PRINTERS),
                                MB_OK|MB_ICONSTOP);

                            // stay in dialog -- break out of IDOK processing
                            break;
                        }

                        // Add port to port list combobox
                        hwndCB = (HWND)GetWindowPtr(hdlg, DWLP_USER);
                        AddPortToCombobox(hwndCB, szBuf, NULL);
                        ComboBox_SetCurSel(hwndCB,
                            ComboBox_FindString(hwndCB, 0, szBuf));
                    }
                    else
                    {
                        hwndLB = GetDlgItem(hdlg, IDD_ADDPORT_LB);
                        i = ListBox_GetCurSel(hwndLB);
                        if (i >= 0)
                        {
                            LPPORT_INFO_2 pPorts;
                            DWORD dwNumPorts;
                            BOOL fAddPort;

                            ListBox_GetText(hwndLB, i, szBuf);

                            DebugMsg(DM_TRACE, TEXT("sh ADDPORT - Adding a '%s' port"), szBuf);

                            // We want to add a port AND select the newly added port
                            // into the listbox, but there's no easy way to get this
                            // info...  So we need to re-enumerate the ports and
                            // grab the new one...

                            pPorts = Printer_EnumProps(NULL, 2, &dwNumPorts, Printers_EnumPortsCB, NULL);

                            fAddPort = AddPort(NULL, hdlg, szBuf);
                            if (fAddPort && pPorts)
                            {
                                LPPORT_INFO_2 pNewPorts;
                                DWORD dwNumNewPorts;

                                pNewPorts = Printer_EnumProps(NULL, 2, &dwNumNewPorts, Printers_EnumPortsCB, NULL);
                                if (pNewPorts)
                                {
                                    DWORD i;

                                    // I'm assuming no other thread called DeletePort
                                    // during this operation.  I'm worrying about the
                                    // case where dwNumPorts == dwNumNewPorts just in
                                    // case there's some way for AddPort to return
                                    // success without actually adding a port.
                                    ASSERT(dwNumNewPorts >= dwNumPorts);

                                    // I'm also assuming that the ports (exluding the
                                    // newly added one) will be enumerated in
                                    // the same order
                                    for (i = 0 ; i < dwNumPorts ; i++)
                                    {
                                        if (lstrcmp(pPorts[i].pPortName, pNewPorts[i].pPortName))
                                        {
                                            // we found the new port, select it
                                            goto AddPort;
                                        }
                                    }
                                    if (i == dwNumNewPorts - 1)
                                    {
                                        // the new port was added to the end of the list
AddPort:
                                        hwndCB = (HWND)GetWindowPtr(hdlg, DWLP_USER);
                                        AddPortToCombobox(
                                            hwndCB,
                                            pNewPorts[i].pPortName,
                                            pNewPorts[i].pDescription);
                                        ComboBox_SetCurSel(hwndCB,
                                            ComboBox_FindString(hwndCB, 0,
                                                pNewPorts[i].pPortName));
                                    }

                                    LocalFree((HLOCAL)pNewPorts);
                                } // if (pNewPorts)
                            } // if (AddPort() && pPorts)

                            if (pPorts)
                                LocalFree((HLOCAL)pPorts);

                            if (!fAddPort)
                            {
                                // user cancelled AddPort, so stay in this dlg.
                                break;
                            }
                        }
                        else
                        {
                            DebugMsg(DM_TRACE, TEXT("sh ADDPORT - No portmon is selected!"));
                            ASSERT(0);
                        }
                    }

                    EndDialog (hdlg, TRUE);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog (hdlg, FALSE);
                    return FALSE;

                default:
                    return FALSE;
            }
            return TRUE;

        default:
            return FALSE;
    }
    return TRUE;
}


//--------------------------------------------------------------------------
// Dialog procedure of "Del Port" dialog
//

const static DWORD aDelPortHelpIDs[] = {  // Context Help IDs
     IDD_DELPORT_LB,        IDH_DELPORT_LB,
     IDD_DELPORT_TEXT_1,    IDH_DELPORT_LB,

     0, 0
};

BOOL CALLBACK Printer_DelPortDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LPDELPORTDLG pdpd = (LPDELPORTDLG)GetWindowPtr(hdlg, DWLP_USER);

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            HWND hwndLB;
            int i;

            // store dest HWND away
            pdpd = (LPDELPORTDLG)lParam;
            SetWindowLongPtr(hdlg, DWLP_USER, lParam);

            // move all entries in hwndCB into this listbox
            hwndLB = GetDlgItem(hdlg, IDD_DELPORT_LB);
            for (i = ComboBox_GetCount(pdpd->hwndCB)-1 ; i >= 0 ; i--)
            {
                TCHAR szBuf[MAX_PATH];
                ComboBox_GetLBText(pdpd->hwndCB, i, szBuf);
                ListBox_AddString(hwndLB, szBuf);
            }
            ListBox_SetCurSel(hwndLB, 0);

            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD)(LPTSTR) aDelPortHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD)(LPVOID)aDelPortHelpIDs);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                {
                    HWND hwndLB;
                    TCHAR szBuf[MAX_PATH];
                    int iSel;

                    // get port to remove
                    hwndLB = GetDlgItem(hdlg, IDD_DELPORT_LB);
                    iSel = ListBox_GetCurSel(hwndLB);
                    if (LB_ERR != iSel &&
                        LB_ERR != ListBox_GetText(hwndLB, iSel, szBuf))
                    {
                        LPTSTR pPortName;

                        // find port NAME from combobox
                        iSel = ComboBox_FindStringExact(pdpd->hwndCB, 0, szBuf);
                        ASSERT(iSel != CB_ERR);
                        pPortName = (LPTSTR)ComboBox_GetItemData(pdpd->hwndCB, iSel);

                        // if this port is currently selected, don't delete it
                        if (iSel == ComboBox_GetCurSel(pdpd->hwndCB))
                        {
                            goto PortIsBusy;
                        }

                        if (pPortName)
                        {
                            // remove port from subsystem
                            if (!DeletePort(NULL, hdlg, pPortName))
                            {
                                switch (GetLastError())
                                {
                                case ERROR_BUSY:
PortIsBusy:
                                    ShellMessageBox(HINST_THISDLL, hdlg,
                                        MAKEINTRESOURCE(IDS_PRTPROP_ADDPORT_CANTDEL_BUSY),
                                        MAKEINTRESOURCE(IDS_PRINTERS),
                                        MB_OK|MB_ICONINFORMATION);
                                    break;
                                default:
                                    ShellMessageBox(HINST_THISDLL, hdlg,
                                        MAKEINTRESOURCE(IDS_PRTPROP_ADDPORT_CANTDEL_LOCAL),
                                        MAKEINTRESOURCE(IDS_PRINTERS),
                                        MB_OK|MB_ICONINFORMATION);
                                    break;
                                }
                                // stay in dlg
                                break;
                            }
                        }
                        else
                        {
                            // remove port from MRU
                            DelMRUString(pdpd->pppi->psPrinterStuff->hmru,
                                FindMRUString(pdpd->pppi->psPrinterStuff->hmru,
                                        szBuf, NULL));
                        }

                        // remove port from combobox
                        ComboBox_DeleteString(pdpd->hwndCB, iSel);
                        Str_SetPtr(&pPortName, NULL);

                        // REVIEW: Do we need to select another port if
                        // this one becomes empty?
                    }
                    else
                    {
                        ASSERT(FALSE);
                    }

                    EndDialog (hdlg, TRUE);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog (hdlg, FALSE);
                    return FALSE;

                default:
                    return FALSE;
            }
            return TRUE;

        default:
            return FALSE;
    }
    return TRUE;
}


//--------------------------------------------------------------------------
// Dialog procedure of "Spool Settings" dialog
//

const static DWORD aSpoolSettingsHelpIDs[] = {  // Context Help IDs
     rad1,              IDH_SPOOLSETTINGS_SPOOL,
     rad2,              IDH_SPOOLSETTINGS_NOSPOOL,
     rad3,              IDH_SPOOLSETTINGS_PRINT_FASTER,
     rad4,              IDH_SPOOLSETTINGS_LESS_SPACE,
     cmb1,              IDH_SPOOLSETTINGS_DATA_FORMAT,
     IDD_SPOOL_TXT,     IDH_SPOOLSETTINGS_DATA_FORMAT,
     IDD_RESTORE,       IDH_SPOOLSETTINGS_RESTORE,
     IDD_ENABLE_BIDI,   IDH_SPOOLSETTINGS_ENABLE_BIDI,
     IDD_DISABLE_BIDI,  IDH_SPOOLSETTINGS_DISABLE_BIDI,

     0, 0
};

BOOL CALLBACK Printer_SpoolSettingsDlgProc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LPPRINTERPAGEINFO pppi = (LPPRINTERPAGEINFO)GetWindowPtr(hdlg, DWLP_USER);

    switch (uMessage) {

        case WM_INITDIALOG: {
            pppi = (LPPRINTERPAGEINFO)lParam;
            SetWindowLongPtr(hdlg, DWLP_USER, lParam);

            DetailDlg_FillDatatypes(hdlg, pppi->psPrinterStuff);

            if (pppi->psPrinterStuff->sPrinter.Attributes &
                PRINTER_ATTRIBUTE_QUEUED)
            {
                CheckRadioButton(hdlg, IDGRB_DESPOOL_LATER,
                            IDGRB_DESPOOL_NOW, IDGRB_DESPOOL_LATER);
            }
            else
            {
                CheckRadioButton(hdlg, IDGRB_DESPOOL_LATER,
                            IDGRB_DESPOOL_NOW, IDGRB_DESPOOL_NOW);
            }

            if (pppi->psPrinterStuff->sPrinter.Attributes & PRINTER_ATTRIBUTE_DIRECT)
            {
                CheckRadioButton(hdlg, IDGRB_SPOOL, IDGRB_DIRECT, IDGRB_DIRECT);
                EnableWindow(GetDlgItem(hdlg, IDDC_DATATYPE), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_LATER), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_NOW), FALSE);
            }
            else
            {
                CheckRadioButton(hdlg, IDGRB_SPOOL, IDGRB_DIRECT, IDGRB_SPOOL);
            }

            if (pppi->psPrinterStuff->sPrinter.Attributes & PRINTER_ATTRIBUTE_SHARED)
            {
                EnableWindow(GetDlgItem(hdlg, IDGRB_DIRECT), FALSE);
            }

            if (pppi->psPrinterStuff->sPrinter.Attributes &
                PRINTER_ATTRIBUTE_NETWORK)
            {
DisableBIDI:
                EnableWindow(GetDlgItem(hdlg, IDD_ENABLE_BIDI), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDD_DISABLE_BIDI), FALSE);
            }
            else
            {
                LPDRIVER_INFO_3 pDriver;
                BOOL fNoBIDI;
                pDriver = Printer_GetPrinterDriver(pppi->psPrinterStuff->hPrinter, 3);
                fNoBIDI = !pDriver || pDriver->pMonitorName == NULL || *(pDriver->pMonitorName) == TEXT('\0');
                if (pDriver)
                    LocalFree(pDriver);
                if (fNoBIDI)
                    goto DisableBIDI;

                if (pppi->psPrinterStuff->sPrinter.Attributes &
                    PRINTER_ATTRIBUTE_ENABLE_BIDI)
                {
                    CheckRadioButton(hdlg, IDD_ENABLE_BIDI,
                                    IDD_DISABLE_BIDI, IDD_ENABLE_BIDI);
                }
                else
                {
                    CheckRadioButton(hdlg, IDD_ENABLE_BIDI,
                                    IDD_DISABLE_BIDI, IDD_DISABLE_BIDI);
                }
            }


            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD)(LPTSTR) aSpoolSettingsHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD)(LPVOID)aSpoolSettingsHelpIDs);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {

                case IDGRB_SPOOL:
                    EnableWindow(GetDlgItem(hdlg, IDDC_DATATYPE), TRUE);
                    EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_LATER), TRUE);
                    EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_NOW), TRUE);
                    break;

                case IDGRB_DIRECT:
                {
                    HWND hTemp = GetDlgItem(hdlg, IDDC_DATATYPE);

                    // DIRECT only supports RAW printing
                    Printer_SelDlgComboEntry(hTemp, 0, c_szRAW, FALSE);

                    EnableWindow(hTemp, FALSE);
                    EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_LATER), FALSE);
                    EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_NOW), FALSE);
                    break;
                }

#if 0 // just in case we decide we want this code sometime...
                case IDDC_DATATYPE:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        TCHAR szDataType[DATATYPE_NAMELEN];
                        LPDRIVER_INFO_3 pDriver;

                        // if the user selects a datatype other than RAW or
                        // their DEFAULT datatype, double check the change.
                        Dlg_CB_GetSelText(hdlg, IDDC_DATATYPE, szDataType, ARRAYSIZE(szDataType));
                        pDriver = Printer_GetPrinterDriver(pppi->psPrinterStuff->hPrinter, 3);
                        if (!lstrcmp(szDataType, c_szRAW) &&
                            (pDriver && !lstrcmp(szDataType, pDriver->pDefaultDataType)))
                        {
                            if (IDYES != ShellMessageBox(HINST_THISDLL, hdlg,
                                    MAKEINTRESOURCE(IDS_PRTPROP_DRIVER_WARN),
                                    MAKEINTRESOURCE(IDS_PRINTERS),
                                    MB_YESNO|MB_ICONEXCLAMATION))
                            {
                                if (IsDlgButtonChecked(hdlg, IDGRB_SPOOL))
                                {
                                    Printer_SelDlgComboEntry(hdlg, IDDC_DATATYPE,
                                        pppi->psPrinterStuff->sPrinter.pDatatype, TRUE);
                                }
                                else
                                {
                                    // if you're printing direct, we must force raw
                                    Printer_SelDlgComboEntry(hdlg, IDDC_DATATYPE,
                                        c_szRAW, TRUE);
                                }
                            }
                        }
                        if (pDriver)
                            LocalFree((HLOCAL)pDriver);
                    }
                    break;
#endif

                case IDD_RESTORE:
                {
                    HWND hTemp = GetDlgItem(hdlg, IDDC_DATATYPE);
                    LPDRIVER_INFO_3 pDriver;

                    CheckRadioButton(hdlg, IDGRB_SPOOL, IDGRB_DIRECT, IDGRB_SPOOL);
                    EnableWindow(hTemp, TRUE);
                    EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_LATER), TRUE);
                    EnableWindow(GetDlgItem(hdlg, IDGRB_DESPOOL_NOW), TRUE);
                    CheckRadioButton(hdlg, IDGRB_DESPOOL_LATER, IDGRB_DESPOOL_NOW, IDGRB_DESPOOL_NOW);

                    pDriver = Printer_GetPrinterDriver(pppi->psPrinterStuff->hPrinter, 3);
                    if (pDriver)
                    {
                        Printer_SelDlgComboEntry(hTemp, 0, pDriver->pDefaultDataType, TRUE);
                        LocalFree((HLOCAL)pDriver);
                    }

                    // BUGBUG: we have no default BIDI support

                    break;
                }

                case IDOK:
                {
                    DWORD dwAttributes;
                    TCHAR  szDatatype[128];

                    // store sPrinter info
                    dwAttributes = pppi->psPrinterStuff->sPrinter.Attributes;
                    lstrcpy(szDatatype, pppi->psPrinterStuff->sPrinter.pDatatype);

                    // update sPrinter
                    if (IsDlgButtonChecked(hdlg, IDGRB_DIRECT))
                    {
                        pppi->psPrinterStuff->sPrinter.Attributes |= PRINTER_ATTRIBUTE_DIRECT;
                        pppi->psPrinterStuff->sPrinter.Attributes &= ~PRINTER_ATTRIBUTE_QUEUED;
                    }
                    else
                    {
                        pppi->psPrinterStuff->sPrinter.Attributes &= ~PRINTER_ATTRIBUTE_DIRECT;
                        pppi->psPrinterStuff->sPrinter.Attributes &= ~PRINTER_ATTRIBUTE_QUEUED;
                        if (IsDlgButtonChecked(hdlg, IDGRB_DESPOOL_LATER))
                            pppi->psPrinterStuff->sPrinter.Attributes |= PRINTER_ATTRIBUTE_QUEUED;
                    }
                    Dlg_CB_GetSelText(hdlg, IDDC_DATATYPE,
                                      pppi->psPrinterStuff->sPrinter.pDatatype,
                                      ARRAYSIZE(pppi->psPrinterStuff->sPrinter.pDatatype));
                    if (IsDlgButtonChecked(hdlg, IDD_ENABLE_BIDI))
                    {
                        pppi->psPrinterStuff->sPrinter.Attributes |= PRINTER_ATTRIBUTE_ENABLE_BIDI;
                    }
                    else
                    {
                        pppi->psPrinterStuff->sPrinter.Attributes &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;
                    }

                    // return TRUE iff something changed
                    EndDialog (hdlg, dwAttributes != pppi->psPrinterStuff->sPrinter.Attributes ||
                                     lstrcmp(szDatatype, pppi->psPrinterStuff->sPrinter.pDatatype));
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog (hdlg, FALSE);
                    return FALSE;

                default:
                    return FALSE;
            }
            return TRUE;

        default:
            return FALSE;
    }
    return TRUE;
}


//--------------------------------------------------------------------------
// Add a "Setup" page for Win 3.1 driver
//
void _AddDlgPage(UINT idResource,
                             DLGPROC lpfnDlgProc,
                             PRINTERSTUFF *psPrinterStuff,
                             LPPROPSHEETHEADER lpsh)
{
    PRINTERPAGEINFO ppi;
    HPROPSHEETPAGE hpage;

    ZeroMemory(&ppi, SIZEOF(PRINTERPAGEINFO));

    // Fill common part
    ppi.psp.dwSize = SIZEOF(ppi);        // extra data
    ppi.psp.dwFlags = PSP_DEFAULT;
    ppi.psp.hInstance = HINST_THISDLL;
    //ppi.psp.lParam    = 0;
    ppi.psp.pszTemplate = MAKEINTRESOURCE(idResource);
    ppi.psp.pfnDlgProc = lpfnDlgProc;

    // Fill extra parameter
    ppi.psPrinterStuff = psPrinterStuff;

    hpage = CreatePropertySheetPage(&ppi.psp);
    if (hpage) {
        _AddPropSheetPage(hpage, (LPARAM)lpsh);
    }
}


extern IDataObjectVtbl c_CPrintersIDLDataVtbl;

UINT _AddPrinterPropPages(LPPRINTERSTUFF psPrinterStuff, HWND hWnd, LPPROPSHEETHEADER lpsh)
{
    UINT cpageGeneric;
    HKEY hkeyBaseProgID;

    // First add our pages
    if (!SHRestricted(REST_NOPRINTERTABS))
    {
        _AddDlgPage(DLG_PRN_GENERAL, Printer_GeneralDlgProc, psPrinterStuff, lpsh);
        _AddDlgPage(DLG_PRN_DETAIL, Printer_DetailDlgProc, psPrinterStuff, lpsh);
    }

    // Then, add hooked pages if they exist in the registry
    hkeyBaseProgID = NULL;
    RegOpenKey(HKEY_CLASSES_ROOT, c_szPrinters, &hkeyBaseProgID);
    if (hkeyBaseProgID)
    {
        // we need an IDataObject
        LPITEMIDLIST pidlParent = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
        if (pidlParent)
        {
            HRESULT hres;
            IDataObject *lpdtobj;
            IDPRINTER idp;
            LPITEMIDLIST pidp;

            Printers_FillPidl(&idp, psPrinterStuff->sPrinter.pPrinterName);
            pidp = (LPITEMIDLIST)&idp;
            hres = CIDLData_CreateFromIDArray2(&c_CPrintersIDLDataVtbl, pidlParent, 1,
                        &pidp, &lpdtobj);
            if (hres == NOERROR)
            {
                // add the hooked pages
                HDCA hdca = DCA_Create();
                if (hdca)
                {
                    DCA_AddItemsFromKey(hdca, hkeyBaseProgID, c_szPropSheet);
                    DCA_AppendClassSheetInfo(hdca, hkeyBaseProgID, lpsh, lpdtobj);
                    DCA_Destroy(hdca);
                }
                lpdtobj->lpVtbl->Release(lpdtobj);
            }
            ILFree(pidlParent);
        }

        RegCloseKey(hkeyBaseProgID);
    }

    // Then, let the driver add pages
    cpageGeneric = lpsh->nPages;
    EnumPrinterPropertySheets(psPrinterStuff->hPrinter, hWnd, _AddPropSheetPage, (LPARAM)lpsh);

    // Check if the driver has append any pages.
    if (cpageGeneric == lpsh->nPages)
        psPrinterStuff->uFlags |= PS_OLDSTYLE;
    else
        psPrinterStuff->uFlags &= ~PS_OLDSTYLE; // to catch 3.1 -> 4.0 transition

    return lpsh->nPages;
}

BOOL Printer_Load(LPPRINTERSTUFF psPrinterStuff)
{
    LPPRINTER_INFO_2 pPrinter;
    LPPRINTER_INFO_5 pPrinter5;

    pPrinter = Printer_GetPrinterInfo(psPrinterStuff->hPrinter, 2);
    if (!pPrinter)
        return FALSE;

    // make a copy for us to party on
    //
    // be a bit paranoid and make sure we have all the strings.  some of them
    // can be null some of the time.  we expect a few to never be null.
    ASSERT(pPrinter->pPrinterName);
    if (pPrinter->pPrinterName)
        lstrcpyn(psPrinterStuff->sPrinter.pPrinterName, pPrinter->pPrinterName, MAXNAMELEN);
    if (pPrinter->pComment)
        lstrcpyn(psPrinterStuff->sPrinter.pComment, pPrinter->pComment, 256);
    ASSERT(pPrinter->pPortName);
    if (pPrinter->pPortName)
        lstrcpyn(psPrinterStuff->sPrinter.pPortName, pPrinter->pPortName, MAX_PATH);
    ASSERT(pPrinter->pDriverName);
    if (pPrinter->pDriverName)
        lstrcpyn(psPrinterStuff->sPrinter.pDriverName, pPrinter->pDriverName, MAXNAMELEN);
    if (pPrinter->pSepFile)
        lstrcpyn(psPrinterStuff->sPrinter.pSepFile, pPrinter->pSepFile, MAX_PATH);
    if (pPrinter->pPrintProcessor)
        lstrcpyn(psPrinterStuff->sPrinter.pPrintProcessor, pPrinter->pPrintProcessor, 128);
    else
        DebugMsg(DM_WARNING, TEXT("pPrintProcessor is NULL."));
    if (pPrinter->pDatatype)
        lstrcpyn(psPrinterStuff->sPrinter.pDatatype, pPrinter->pDatatype, 128);
    else
        DebugMsg(TF_ERROR, TEXT("bug 19753 is back: pDatatype is NULL."));
    psPrinterStuff->sPrinter.Attributes = pPrinter->Attributes;
    psPrinterStuff->sPrinter.StartTime = pPrinter->StartTime;
    psPrinterStuff->sPrinter.UntilTime = pPrinter->UntilTime;

    LocalFree((HLOCAL)pPrinter);

    pPrinter5 = Printer_GetPrinterInfo(psPrinterStuff->hPrinter, 5);
    if (!pPrinter5)
        return FALSE;
    psPrinterStuff->sPrinter.DeviceNotSelectedTimeout = pPrinter5->DeviceNotSelectedTimeout;
    psPrinterStuff->sPrinter.TransmissionRetryTimeout = pPrinter5->TransmissionRetryTimeout;
    LocalFree((HLOCAL)pPrinter5);

    return TRUE;
}

//
// This function opens the property sheet of specified printer
//
// Arguments:
//  hWnd       -- Specifies the parent window (optional)
//  pszPrinter -- Specifies the printer (e.g., "HP LaserJet IIISi")
//  lParam -- pszArgs -- may specify a sheet name to open to
//
void Printer_Properties(HWND hWnd, LPCTSTR pszPrinterName, int nCmdShow, LPARAM lParam)
{
    PRINTERSTUFF sPrinterStuff;
    HPROPSHEETPAGE ahpage[MAX_FILE_PROP_PAGES]; // must use MAX_FILE_PROP_PAGES
    PROPSHEETHEADER psh;                        // since we call _AppendClassSheetInfo
    BOOL fError = TRUE;
    MRUINFO mi;

    // set the stub window icon and title
    if (hWnd)
        SendMessage(hWnd, STUBM_SETICONTITLE, (WPARAM)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_PRINTER), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE), (LPARAM)pszPrinterName);

    ZeroMemory(&sPrinterStuff, SIZEOF(sPrinterStuff));

    sPrinterStuff.hPrinter = Printer_OpenPrinter(pszPrinterName);
    if (!sPrinterStuff.hPrinter)
        goto Error0;

    if (!Printer_Load(&sPrinterStuff))
        goto Error1;

    sPrinterStuff.fSaveEverything = FALSE;

    mi.cbSize      = SIZEOF(MRUINFO);
    mi.uMax        = 7;
    mi.fFlags      = MRU_CACHEWRITE;
    mi.hKey        = HKEY_CURRENT_USER;
    mi.lpszSubKey  = c_szPrnPortsMRU;
    mi.lpfnCompare = NULL;
    sPrinterStuff.hmru = CreateMRUList(&mi);

    Printer_LoadIcons(pszPrinterName, &(sPrinterStuff.hIcon), NULL);

    //
    // Initialize PROPSHEETHEADER
    //

    ZeroMemory(&psh, SIZEOF(psh));
    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hWnd;
    psh.pszCaption = pszPrinterName;
    psh.phpage = ahpage;
    if (lParam)
    {
        psh.dwFlags |= PSH_USEPSTARTPAGE;
        psh.pStartPage = (LPCTSTR)lParam;
    }

    //
    // Add property sheet pages
    //
    sPrinterStuff.nPages = _AddPrinterPropPages(&sPrinterStuff, hWnd, &psh);
    if (sPrinterStuff.nPages)
    {
        // Successfuly done. Open the property sheet.
        fError = PropertySheet(&psh) < 0;
    }

    if (sPrinterStuff.hIcon)
        DestroyIcon(sPrinterStuff.hIcon);

    if (sPrinterStuff.hmru)
        FreeMRUList(sPrinterStuff.hmru);
Error1:
    Printer_ClosePrinter(sPrinterStuff.hPrinter);
Error0:
    if (fError)
    {
        ShellMessageBox(HINST_THISDLL, hWnd,
            MAKEINTRESOURCE(IDS_PRTPROP_CANNOT_OPEN),
            MAKEINTRESOURCE(IDS_PRINTERS), MB_OK|MB_ICONEXCLAMATION);
    }
}

#endif // WINNT

// returns 0 if this is a legal name
// returns the IDS_ string id of the error string for an illegal name
int Printer_IllegalName(LPTSTR lpFriendlyName)
{
    int fIllegal = 0;

    if (*lpFriendlyName == TEXT('\0'))
    {
        fIllegal = IDS_PRTPROP_RENAME_NULL;
    }
    else if (lstrlen(lpFriendlyName) >= MAXLOCALNAMELEN)
    {
        fIllegal = IDS_PRTPROP_RENAME_TOO_LONG;
    }
    else
    {
#if (defined(DBCS) || (defined(FE_SB) && !defined(UNICODE)))
        while (*lpFriendlyName)
        {
            if (IsDBCSLeadByte(*lpFriendlyName) && *(lpFriendlyName+1))
            {
                lpFriendlyName ++;
            }
#ifdef WINNT
            else if ( *lpFriendlyName == TEXT('!')  ||
                      *lpFriendlyName == TEXT('\\') ||
                      *lpFriendlyName == TEXT(',')    )
#else
            else if ( *lpFriendlyName == TEXT('=')  ||
                      *lpFriendlyName == TEXT('\\') ||
                      *lpFriendlyName == TEXT(';')  ||
                      *lpFriendlyName == TEXT(',')    )
#endif
                break;

            lpFriendlyName++;
        }
#else
        while (*lpFriendlyName         &&
#ifdef WINNT
               *lpFriendlyName != TEXT('!')  &&
               *lpFriendlyName != TEXT('\\') &&
               *lpFriendlyName != TEXT(',')    )
#else
               *lpFriendlyName != TEXT('=')  &&
               *lpFriendlyName != TEXT('\\') &&
               *lpFriendlyName != TEXT(';')  &&
               *lpFriendlyName != TEXT(',')    )
#endif
        {
            lpFriendlyName++ ;
        }
#endif
        if (*lpFriendlyName)
        {
            fIllegal = IDS_PRTPROP_RENAME_BADCHARS;
        }
    }

    return fIllegal;
}
