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
#include "priv.h"
#include "appwiz.h"
#include <cpl.h>
#include "util.h"
#include "resource.h"


#ifdef WINNT

//
// (tedm) Header files for Setup
//
#include <setupapi.h>
#include <syssetup.h>
#endif

#define NUM_APPLETS     2  //AppWiz applet and the Folder Options applet.

typedef struct tagApplets 
{     
    int icon;         // icon resource identifier 
    int namestring;   // name-string resource identifier 
    int descstring;   // description-string resource identifier 
} APPLETS;

//AppWIZ needs to be first for Win95 compat
const APPLETS AppWizApplets[NUM_APPLETS] = 
{
    IDI_CPLICON, IDS_NAME, IDS_INFO,
    IDI_FOLDEROPT, IDS_FOLDEROPT_NAME, IDS_FOLDEROPT_INFO
};

#define APPWIZARD_APPLET   0  //Index of AppWizard applet in the above array.
#define FOLDEROPT_APPLET   1  //Index of FolderOptions applet in the above array.
    
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

const LPCTSTR g_szStartPages[] = { TEXT("remove"), TEXT("install"), TEXT("configurewindows") };
    
int ParseStartParams(LPCTSTR pcszStart)
{
    int iStartPage = 0;
    if (IsInRange(*pcszStart, TEXT('0'), TEXT('9')))
        return StrToInt(pcszStart);

    if (g_bRunOnNT5)
    {
        int i;
        for (i = 0; i < ARRAYSIZE(g_szStartPages); i++)
        {
            if (!StrCmpI(g_szStartPages[i], pcszStart))
            {
                iStartPage = i;
                break;
            }
        }
    }

    return iStartPage;
}

LONG CALLBACK CPlApplet(HWND hwnd, UINT Msg, LPARAM lParam1, LPARAM lParam2 )
{
    UINT nStartPage;
    int  iAppletIndex = (int)lParam1;
//  not currently used by chicago   LPNEWCPLINFO lpNewCPlInfo;
    LPTSTR lpStartPage;

    switch (Msg)
    {
    case CPL_INIT:
        return TRUE;

    case CPL_GETCOUNT:
#ifdef DOWNLEVEL
        return 1;
#else
        return NUM_APPLETS; //AppWiz and Folder Options.
#endif //DOWNLEVEL

    case CPL_INQUIRE:
#define lpCPlInfo ((LPCPLINFO)lParam2)
        lpCPlInfo->idIcon = AppWizApplets[iAppletIndex].icon;
        lpCPlInfo->idName = AppWizApplets[iAppletIndex].namestring;
        lpCPlInfo->idInfo = AppWizApplets[iAppletIndex].descstring;
        lpCPlInfo->lData  = 0;
#undef lpCPlInfo
        break;

    case CPL_DBLCLK:
        if(iAppletIndex == APPWIZARD_APPLET)
        {
#ifndef DOWNLEVEL
            OpenAppMgr(hwnd, 0);
#else
            InstallCPL(hwnd, 0);
#endif
        }
        else
        {
            // We need to bring up the FolderOptions dialog in the context of some explorer window.
            // So, we post a message here; When processing this message, we create a thread in explorer 
            // process which puts up this dialog.
            PostMessage(GetShellWindow(), CWM_SHOWFOLDEROPT, (WPARAM)0, (LPARAM)0);
        }
        break;

    case CPL_STARTWPARMS:
        if(iAppletIndex == APPWIZARD_APPLET)
        {
            lpStartPage = (LPTSTR)lParam2;

            if (*lpStartPage)
                nStartPage = ParseStartParams(lpStartPage);

            //
            // Make sure that the requested starting page is less than the max page
            //        for the selected applet.

            if (nStartPage >= MAX_PAGES) return FALSE;

#ifndef DOWNLEVEL
            OpenAppMgr(hwnd, nStartPage);
#else
            InstallCPL(hwnd, nStartPage);
#endif

            return TRUE;  // return non-zero to indicate message handled
        }
        else if (iAppletIndex == FOLDEROPT_APPLET)
        {
            // We need to bring up the FolderOptions dialog in the context of some explorer window.
            // So, we post a message here; When processing this message, we create a thread in explorer 
            // process which puts up this dialog.
            PostMessage(GetShellWindow(), CWM_SHOWFOLDEROPT, (WPARAM)0, (LPARAM)0);

            return TRUE; // return non-zero to indicate message handled.
        }
        else
            return FALSE; //We don't need this processing for the AppWizard applet.

    default:
        return FALSE;
    }
    return TRUE;
}  // CPlApplet


#ifdef DOWNLEVEL

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

#endif // DOWNLEVEL


//
//  Adds a page to a property sheet.
//

void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn, LPWIZDATA lpwd, DWORD dwPageFlags)
{
    if (ppsh->nPages < MAX_PAGES)
    {
       PROPSHEETPAGE psp;

       psp.dwSize = sizeof(psp);
       psp.dwFlags = dwPageFlags;
       psp.hInstance = g_hinst;
       psp.pszTemplate = MAKEINTRESOURCE(id);
       psp.pfnDlgProc = pfn;
       psp.lParam = (LPARAM)lpwd;

       ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);

       if (ppsh->phpage[ppsh->nPages])
           ppsh->nPages++;
    }
}  // AddPage


#ifdef DOWNLEVEL

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

    //PROPSHEETHEADER_V1_SIZE: needs to run on downlevel platform (NT4, Win95)

    psh.dwSize = PROPSHEETHEADER_V1_SIZE;
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.hInstance = g_hinst;
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
       AddPage(&psh, DLG_APPLIST, AppListDlgProc, &wd, PSP_DEFAULT);
    }
    else
    {
       if (psh.nStartPage > 0)
       {
           psh.nStartPage--;
       }
    }

    AddPage(&psh, DLG_INSTUNINSTALL, InstallUninstallDlgProc, &wd, PSP_DEFAULT);

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

    iPrshtResult = (int)PropertySheet(&psh);

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

#endif // DOWNLEVEL


// This function disables auto-run during the add from floppy or CD wizard.
// It is a subclass of the property sheet window.

LRESULT CALLBACK WizParentWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                                        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static UINT msgQueryCancelAutoPlay = 0;

    if (!msgQueryCancelAutoPlay)
        msgQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

    if (uMsg == msgQueryCancelAutoPlay)
    {
        return TRUE; // Yes, cancel auto-play when wizard is running
    }
    else
    {
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);        
    }

}

// The following callback is specified when a wizard wants to disable auto-run.
// The callback subclasses the wizard's window, to catch the QueryCancelAutoRun
// message sent by the shell when it wants to auto-run a CD.

int CALLBACK DisableAutoRunCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        SetWindowSubclass(hwnd, WizParentWindowProc, 0, 0);
    }

    return 0;
}

//
//  Common code used by the Link and Setup wizards to initialize the
//  property sheet header and wizard data.
//

void InitWizHeaders(LPPROPSHEETHEADER ppd,
                          HPROPSHEETPAGE *rPages,
                          LPWIZDATA lpwd, int iBmpID, DWORD dwFlags)
{
    lpwd->hbmpWizard = LoadBitmap(g_hinst, MAKEINTRESOURCE(iBmpID));

    //PROPSHEETHEADER_V1_SIZE: needs to run on downlevel platform (NT4, Win95)

    ppd->dwSize = PROPSHEETHEADER_V1_SIZE;
    ppd->dwFlags = dwFlags;
    ppd->hwndParent = lpwd->hwnd;
    ppd->hInstance = g_hinst;
    ppd->pszCaption = NULL;
    ppd->nPages = 0;
    ppd->nStartPage = 0;
    ppd->phpage = rPages;

    if (lpwd->dwFlags & WDFLAG_NOAUTORUN)
    {
        ppd->dwFlags |= PSH_USECALLBACK;
        ppd->pfnCallback = DisableAutoRunCallback;
    }
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

BOOL DoWizard(LPWIZDATA lpwd, int iIDBitmap, const WIZPAGE *wp, int PageCount, 
              DWORD dwWizardFlags, DWORD dwPageFlags)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    int i;
    HWND    hwnd, hwndT;
    BOOL    bResult = FALSE;
    BOOL    bChangedIcon = FALSE;
    HICON   hicoPrev;

    CoInitialize(0);

    InitWizHeaders(&psh, rPages, lpwd, iIDBitmap, dwWizardFlags);

    for (i = 0; i < PageCount; i++)
    {
       AddPage(&psh, wp[i].id, wp[i].pfn, lpwd, dwPageFlags);
    }

    // Walk up the parent/owner chain until we find the master owner.
    //
    // We need to walk the parent chain because sometimes we are given
    // a child window as our lpwd->hwnd.  And we need to walk the owner
    // chain in order to find the owner whose icon will be used for
    // Alt+Tab.
    //
    // GetParent() returns either the parent or owner.  Normally this is
    // annoying, but we luck out and it's exactly what we want.

    hwnd = lpwd->hwnd;
    while ((hwndT = GetParent(hwnd)) != NULL)
    {
        hwnd = hwndT;
    }

    // If the master owner isn't visible we can futz his icon without
    // screwing up his appearance.
    if (!IsWindowVisible(hwnd))
    {
        HICON hicoNew = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_CPLICON));
        hicoPrev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoNew);
        bChangedIcon = TRUE;
    }

    bResult = (BOOL)PropertySheet(&psh);
    FreeWizData(lpwd);

    // Clean up our icon now that we're done
    if (bChangedIcon)
    {
        // Put the old icon back
        HICON hicoNew = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoPrev);
        if (hicoNew)
            DestroyIcon(hicoNew);
    }

    CoUninitialize();

    return bResult;
}


//
//  Link Wizard
//

BOOL LinkWizard(LPWIZDATA lpwd)
{
    BOOL fSuccess;

    static const WIZPAGE wp[] = {
                   {DLG_BROWSE,         BrowseDlgProc},
                   {DLG_PICKFOLDER,     PickFolderDlgProc},
                   {DLG_GETTITLE,       GetTitleDlgProc},
                   {DLG_PICKICON,       PickIconDlgProc},
                   {DLG_WHICHCFG,       PickConfigDlgProc},
                   {DLG_RMOPTIONS,      ConfigOptionsDlgProc} };

    // Don't set lpwd->hwnd to NULL!
    // We must create the link wizard with a parent or you end up with one
    // thread displaying two independent top-level windows and things get
    // really weird really fast.

    fSuccess = DoWizard(lpwd, IDB_SHORTCUTBMP, wp, ARRAYSIZE(wp), 
                        PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_WIZARD_LITE,
                        PSP_DEFAULT | PSP_HIDEHEADER);
    
    return fSuccess;
}


BOOL SetupWizard(LPWIZDATA lpwd)
{
    static const WIZPAGE wp[] = {
                   {DLG_SETUP, SetupDlgProc},
                   {DLG_SETUPBROWSE, SetupBrowseDlgProc},
#ifndef DOWNLEVEL
#ifdef WINNT
                   {DLG_CHGUSRFINISH_PREV, ChgusrFinishPrevDlgProc},
                   {DLG_CHGUSRFINISH, ChgusrFinishDlgProc}
#endif // WINNT
#endif // DOWNLEVEL
    };
                   
    BOOL fResult;

    lpwd->dwFlags |= WDFLAG_SETUPWIZ;

    if (g_bRunOnNT5)
    {
        fResult = DoWizard(lpwd, IDB_INSTALLBMP, wp, ARRAYSIZE(wp), 
                           PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_WIZARD_LITE,
                           PSP_DEFAULT | PSP_HIDEHEADER);
    }
    else
    {
        fResult = DoWizard(lpwd, IDB_LEGACYINSTALLBMP, wp, ARRAYSIZE(wp), 
                           PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_WIZARD,
                           PSP_DEFAULT);
    }

    lpwd->dwFlags &= ~WDFLAG_SETUPWIZ;
    return(fResult);
}

STDAPI InstallAppFromFloppyOrCDROM(HWND hwnd)
{
    WIZDATA wd = {0};

    #ifdef WINNT
    if ( (IsTerminalServicesRunning() && !IsUserAnAdmin())) 
    {
       ShellMessageBox(g_hinst, hwnd, MAKEINTRESOURCE(IDS_RESTRICTION),
           MAKEINTRESOURCE(IDS_NAME), MB_OK | MB_ICONEXCLAMATION);
        return S_FALSE;
    }
    #endif
    
    wd.hwnd = hwnd;
    wd.dwFlags |= WDFLAG_NOAUTORUN;
    return SetupWizard(&wd) ? S_OK : E_FAIL;
}

//
//
//

BOOL MSDOSPropOnlyWizard(LPWIZDATA lpwd)
{
    static const WIZPAGE wp[] = { {DLG_RMOPTIONS, ConfigOptionsDlgProc} };
    lpwd->dwFlags |= WDFLAG_PIFPROP;

    return DoWizard(lpwd, IDB_DOSCONFIG, wp, ARRAYSIZE(wp), PSH_PROPTITLE | PSH_WIZARD,
        PSP_DEFAULT);
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

PIFWIZERR WINAPI AppWizard(HWND hwnd, HANDLE hProps, UINT dwAction)
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

    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

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
