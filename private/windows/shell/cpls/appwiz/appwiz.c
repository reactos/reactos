//
//  APPWIZ.C        Application installation wizard CPL
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  ral 8/09/94 - Clean up
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "appwiz.h"
#include <cpl.h>
#ifdef WINNT
//
// (tedm) Header files for Setup
//
#include <setupapi.h>
#include <syssetup.h>
#endif

TCHAR const c_szPIF[] = TEXT(".pif");
TCHAR const c_szLNK[] = TEXT(".lnk");

#ifdef WX86

BOOL
IsWx86Enabled(
    VOID
    )

//
// Consults the registry to determine if Wx86 is enabled in the system
// NOTE: this should be changed post SUR to call kernel32 which maintanes
//       this information, Instead of reading the registry each time.
//

{
    LONG Error;
    HKEY hKey;
    WCHAR ValueBuffer[MAX_PATH];
    DWORD ValueSize;
    DWORD dwType;

    Error = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Control\\Wx86",
                        &hKey
                        );

    if (Error != ERROR_SUCCESS) {
        return FALSE;
        }

    ValueSize = sizeof(ValueBuffer);
    Error = RegQueryValueExW(hKey,
                             L"cmdline",
                             NULL,
                             &dwType,
                             (LPBYTE)ValueBuffer,
                             &ValueSize
                             );
    RegCloseKey(hKey);

    return (Error == ERROR_SUCCESS &&
            dwType == REG_SZ &&
            ValueSize &&
            *ValueBuffer
            );

}
#endif

BOOL APIENTRY LibMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        hInstance = hDll;

        g_cxIcon = GetSystemMetrics(SM_CXICON);
        g_cyIcon = GetSystemMetrics(SM_CYICON);

        DisableThreadLibraryCalls(hDll);

#ifdef WX86
        bWx86Enabled = IsWx86Enabled();
#endif

    }


    return TRUE;
}


LONG CALLBACK CPlApplet(HWND hwnd, UINT Msg, LPARAM lParam1, LPARAM lParam2 )
{
    UINT nStartPage;
//  not currently used by chicago   LPNEWCPLINFO lpNewCPlInfo;
    LPTSTR lpStartPage;

    switch (Msg)
    {
       case CPL_INIT:
           return TRUE;

       case CPL_GETCOUNT:
           return 1;

       case CPL_INQUIRE:
            #define lpCPlInfo ((LPCPLINFO)lParam2)
            lpCPlInfo->idIcon = IDI_CPLICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;
            lpCPlInfo->lData  = 0;
            #undef lpCPlInfo
           break;

       case CPL_DBLCLK:
           InstallCPL(hwnd, 0);
           break;

       case CPL_STARTWPARMS:
           nStartPage = 0;
           lpStartPage = (LPTSTR)lParam2;

           if (*lpStartPage)
           {
              while (*lpStartPage && (*lpStartPage != TEXT(',')))
              {
                  nStartPage *= 10;
                  nStartPage += *lpStartPage++ - TEXT('0');
              }
           }

           //
           // Make sure that the requested starting page is less than the max page
           //        for the selected applet.

           if (nStartPage >= MAX_PAGES) return FALSE;

           InstallCPL(hwnd, nStartPage);

           return TRUE;  // return non-zero to indicate message handled

       default:
           return FALSE;
    }
    return TRUE;
}  // CPlApplet


//
// This is the addpage which gets called back by external components (yay)
//

BOOL CALLBACK _AddExternalPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *)lParam;

    if( hpage && ( ppsh->nPages < MAX_PAGES ) )
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}


//
//  Adds a page to a property sheet.
//

void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn, LPWIZDATA lpwd)
{
    if (ppsh->nPages < MAX_PAGES)
    {
       PROPSHEETPAGE psp;

       psp.dwSize = sizeof(psp);
       psp.dwFlags = PSP_DEFAULT;
       psp.hInstance = hInstance;
       psp.pszTemplate = MAKEINTRESOURCE(id);
       psp.pfnDlgProc = pfn;
       psp.lParam = (LPARAM)lpwd;

       ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);

       if (ppsh->phpage[ppsh->nPages])
           ppsh->nPages++;
    }
}  // AddPage


//
//  Main control panel entry point.  Displays a property sheet with
//

void InstallCPL(HWND hwnd, UINT nStartPage)
{
    WIZDATA wd;
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    int iPrshtResult;

    memset(&wd, 0, sizeof(wd));

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_NAME);
    psh.nPages = 0;
    psh.nStartPage = nStartPage;
    psh.phpage = rPages;

    //
    //       If there's no net install list then we'll decrement the start page
    //       by 1 so that the tab numbers can remain constant.
    //

    if (AppListGetInfName(&wd))
    {
       AddPage(&psh, DLG_APPLIST, AppListDlgProc, &wd);
    }
    else
    {
       if (psh.nStartPage > 0)
       {
           psh.nStartPage--;
       }
    }

    AddPage(&psh, DLG_INSTUNINSTALL, InstallUninstallDlgProc, &wd);

#ifdef WINNT
    //
    // (tedm)
    //
    // Setup on NT is completely different. No calling into 16-bit DLLs, etc.
    // Just ask the system setup module for its optional components page.
    //
    SetupCreateOptionalComponentsPage(_AddExternalPage,(LPARAM)&psh);
#else
    // Have (16-bit) SETUPX add the Optional Components page
    SHAddPages16( NULL, TEXT("SETUPX.DLL,SUOCPage"), _AddExternalPage,
         (LPARAM)&psh );

    // Have (16-bit) SETUPX add the Emergency Boot Disk page
    SHAddPages16( NULL, TEXT("SETUPX.DLL,SUEBDPage"), _AddExternalPage,
         (LPARAM)&psh );
#endif

    iPrshtResult = PropertySheet(&psh);

    switch(iPrshtResult)
    {
       case ID_PSRESTARTWINDOWS:
       case ID_PSREBOOTSYSTEM:
           RestartDialog(hwnd, NULL,
                       iPrshtResult == ID_PSRESTARTWINDOWS ?
                           EW_RESTARTWINDOWS : EW_REBOOTSYSTEM);
           break;
    }
}


//
//  Common code used by the Link and Setup wizards to initialize the
//  property sheet header and wizard data.
//

void InitWizHeaders(LPPROPSHEETHEADER ppd,
                          HPROPSHEETPAGE *rPages,
                          LPWIZDATA lpwd, int iBmpID)
{
    lpwd->hbmpWizard = LoadBitmap(hInstance, MAKEINTRESOURCE(iBmpID));

    ppd->dwSize = sizeof(*ppd);
    ppd->dwFlags = PSH_PROPTITLE | PSH_WIZARD;
    ppd->hwndParent = lpwd->hwnd;
    ppd->hInstance = hInstance;
    ppd->pszCaption = NULL;
    ppd->nPages = 0;
    ppd->nStartPage = 0;
    ppd->phpage = rPages;
}


//
//  Called when a wizard is dismissed.       Cleans up any garbage left behind.
//

void FreeWizData(LPWIZDATA lpwd)
{
    if (lpwd->hbmpWizard)
    {
       DeleteObject(lpwd->hbmpWizard);
       lpwd->hbmpWizard = NULL;
    }
}

typedef struct _WIZPAGE {
    int     id;
    DLGPROC pfn;
} WIZPAGE;


//
//  Common code used by the Link Wizard, Setup Wizard, and App Wizard (PIF
//  wizard).
//

BOOL DoWizard(LPWIZDATA lpwd, int iIDBitmap, const WIZPAGE *wp, int PageCount)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    int i;
    HWND    hwndTemp = lpwd->hwnd;
    BOOL    bResult = FALSE;

    CoInitialize(0);

    InitWizHeaders(&psh, rPages, lpwd, iIDBitmap);

    for (i = 0; i < PageCount; i++)
    {
       AddPage(&psh, wp[i].id, wp[i].pfn, lpwd);
    }

    bResult = PropertySheet(&psh);
    lpwd->hwnd = hwndTemp;
    FreeWizData(lpwd);

    CoUninitialize();

    return bResult;
}


//
//  Link Wizard
//

BOOL LinkWizard(LPWIZDATA lpwd)
{
    static const WIZPAGE wp[] = {
                   {DLG_BROWSE,         BrowseDlgProc},
                   {DLG_PICKFOLDER,     PickFolderDlgProc},
                   {DLG_GETTITLE,       GetTitleDlgProc},
                   {DLG_PICKICON,       PickIconDlgProc},
                   {DLG_WHICHCFG,       PickConfigDlgProc},
                   {DLG_RMOPTIONS,      ConfigOptionsDlgProc} };

    return DoWizard(lpwd, IDB_SHORTCUTBMP, wp, ARRAYSIZE(wp));
}


BOOL SetupWizard(LPWIZDATA lpwd)
{
    static const WIZPAGE wp[] = {
                   {DLG_SETUP, SetupDlgProc},
                   {DLG_SETUPBROWSE, BrowseDlgProc} };
    BOOL fResult;

    lpwd->dwFlags |= WDFLAG_SETUPWIZ;
    fResult = DoWizard(lpwd, IDB_INSTALLBMP, wp, ARRAYSIZE(wp));
    lpwd->dwFlags &= ~WDFLAG_SETUPWIZ;
    return(fResult);
}

STDAPI InstallAppFromFloppyOrCDROM(HWND hwnd)
{
    WIZDATA wd = {0};
    wd.hwnd = hwnd;
    return SetupWizard(&wd) ? S_OK : E_FAIL;
}

//
//
//

BOOL MSDOSPropOnlyWizard(LPWIZDATA lpwd)
{
    static const WIZPAGE wp[] = { {DLG_RMOPTIONS, ConfigOptionsDlgProc} };
    lpwd->dwFlags |= WDFLAG_PIFPROP;

    return DoWizard(lpwd, IDB_DOSCONFIG, wp, ARRAYSIZE(wp));
}


// BUGBUG -- WHAT THE HECK DO I DO ABOUT hAPPINSTANCE HERE?????

//
//  RUNDLL entry point to create a new link.  An empty file has already been
//  created and is the only parameter passed in lpszCmdLine.
//

void WINAPI NewLinkHere_Common(HWND hwnd, HINSTANCE hAppInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    WIZDATA wd;
    TCHAR   szFolder[MAX_PATH];

    memset(&wd, 0, sizeof(wd));

    wd.hwnd = hwnd;
    wd.dwFlags |= WDFLAG_LINKHEREWIZ | WDFLAG_DONTOPENFLDR;
    wd.lpszOriginalName = lpszCmdLine;

    lstrcpyn(szFolder, lpszCmdLine, ARRAYSIZE(szFolder));

    PathRemoveFileSpec(szFolder);

    wd.lpszFolder = szFolder;

    //
    // If the filename passed in is not valid then we'll just silently fail.
    //

    if (PathFileExists(lpszCmdLine))
    {
       if (!LinkWizard(&wd))
       {
           DeleteFile(lpszCmdLine);
       }
    }
    else
    {
       #define lpwd (&wd)
       WIZERROR(TEXT("APPWIZ ERORR:  Bogus file name passed to NewLinkHere"));
       WIZERROR(lpszCmdLine);
       #undef lpwd
    }
}

void WINAPI NewLinkHere(HWND hwndStub, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
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
        NewLinkHere_Common( hwndStub,
                            hAppInstance,
                            lpwszCmdLine,
                            nCmdShow );
        LocalFree(lpwszCmdLine);
    }
#else
    NewLinkHere_Common( hwndStub,
                             hAppInstance,
                             lpszCmdLine,
                             nCmdShow );
#endif
}

void WINAPI NewLinkHereW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
#ifdef UNICODE
    NewLinkHere_Common( hwndStub,
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
        NewLinkHere_Common( hwndStub,
                            hAppInstance,
                            lpszCmdLine,
                            nCmdShow );
        LocalFree(lpszCmdLine);
    }
#endif
}

//
//  Entry point called from PIFMGR.DLL.  We'll only display the MS-DOS options
//  page for the specified hProps.
//
//  NOTE:  In this instance we want to LocalAlloc the wizard data since
//          we're being called from a 16-bit application (with limited stack).
//          Note that LPTR zero-initializes the block.
//

PIFWIZERR WINAPI AppWizard(HWND hwnd, INT hProps, UINT dwAction)
{
    LPWIZDATA lpwd = (LPWIZDATA)LocalAlloc(LPTR, sizeof(WIZDATA));
    PIFWIZERR err = PIFWIZERR_SUCCESS;

    if (!lpwd)
    {
       return(PIFWIZERR_OUTOFMEM);
    }

    lpwd->hProps = hProps;
    lpwd->hwnd = hwnd;

    switch (dwAction)
    {
       case WIZACTION_UICONFIGPROP:
           if (!MSDOSPropOnlyWizard(lpwd))
           {
              err = PIFWIZERR_USERCANCELED;
           }
           break;

       case WIZACTION_SILENTCONFIGPROP:
           err = ConfigRealModeOptions(lpwd, NULL, CRMOACTION_DEFAULT);
           break;

       case WIZACTION_CREATEDEFCLEANCFG:
           err = ConfigRealModeOptions(lpwd, NULL, CRMOACTION_CLEAN);
           break;

       default:
           err = PIFWIZERR_INVALIDPARAM;
    }

    LocalFree((LPVOID)lpwd);
    return(err);
}


//
//  Called directly by Cabinet
//

BOOL ConfigStartMenu(HWND hwnd, BOOL bDelete)
{
    if (bDelete)
    {
       return(RemoveItemsDialog(hwnd));
    }
    else
    {
       WIZDATA wd;

       memset(&wd, 0, sizeof(wd));

       wd.hwnd = hwnd;
       wd.dwFlags |= WDFLAG_DONTOPENFLDR;

       return(LinkWizard(&wd));
    }
}


//
//  This is a generic function that all the app wizard sheets call
//  to do basic initialization.
//

LPWIZDATA InitWizSheet(HWND hDlg, LPARAM lParam, DWORD dwFlags)
{
    LPPROPSHEETPAGE ppd  = (LPPROPSHEETPAGE)lParam;
    LPWIZDATA       lpwd = (LPWIZDATA)ppd->lParam;
    HWND            hBmp = GetDlgItem(hDlg, IDC_WIZBMP);

    lpwd->hwnd = hDlg;

    SetWindowLong(hDlg, DWL_USER, lParam);

    SendMessage(hBmp, STM_SETIMAGE,
              IMAGE_BITMAP, (LPARAM)lpwd->hbmpWizard);

    return(lpwd);
}

void CleanUpWizData(LPWIZDATA lpwd)
{
    //
    // Release any INewShortcutHook.
    //

    if (lpwd->pnshhk)
    {
        lpwd->pnshhk->lpVtbl->Release(lpwd->pnshhk);
        lpwd->pnshhk = NULL;
    }

    return;
}
