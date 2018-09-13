#include "stdafx.h"

#include "pathpg.h"
#include "fldrpg.h"
#include "netfinpg.h"
#include "ftppg.h"

#include "ulistpg.h"
#include "data.h"
#include "secdlg.h"
#include "advpg.h"

#include "usercom.h"

#include "mnddlg.h"

#include "shlguidp.h"

#pragma hdrstop

HINSTANCE g_hInstance = 0;
ULONG g_cLocks = 0;
BOOL g_bMirroredOS = FALSE;
const TCHAR c_szShell32Dll[] = TEXT("shell32.dll");

extern "C" BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
            g_hInstance = hinstDLL;
            g_bMirroredOS = IS_MIRRORING_ENABLED();
            DebugProcessAttach();
            break;

        case DLL_THREAD_DETACH:
            DebugThreadDetach();
            break;

        case DLL_PROCESS_DETACH:
            DebugProcessDetach();
            break;

        default:
            break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

// Stuff for the net places wizard
void APIENTRY AddNetPlaceRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    CONNECTDLGSTRUCT cds = { 0 };
    cds.cbStructure = SIZEOF(cds);
    cds.hwndOwner = hwndStub;

    if ((pszCmdLine[0] == 'm') || (pszCmdLine[0] == 'M'))        
    {
        NetPlacesWizardDoModal(&cds, NETPLACES_WIZARD_MAPDRIVE, FALSE);
    }
    else
        NetPlacesWizardDoModal(&cds, NETPLACES_WIZARD_ADDPLACE, FALSE);
}

#if 0
void SetupDCInfo(NETPLACESWIZARDDATA* pdata)
{    
    DWORD dwError = NO_ERROR;
    pdata->pdcInfo = NULL;
    pdata->fGCAvailable = FALSE;

// We no longer do any DS stuff in the wizard. At least for now. Don't do all this useless work.
    //
    // Get the name of a domain controller, preferably one
    // that has a GC
    //
    dwError = DsGetDcName(NULL,
                          NULL,
                          NULL,
                          NULL,
                          DS_GC_SERVER_REQUIRED,
                          &pdata->pdcInfo);

    
    if (dwError == NO_ERROR)
    {
        // MPRUI_LOG0(ERROR, "GC is available\n");
        pdata->fGCAvailable = TRUE;
    }
    else
    {
        // MPRUI_LOG0(ERROR, "No GC is available\n");

        pdata->fGCAvailable = FALSE;

        //
        // Try to get a DC name again, this time without
        // preferring a GC
        //
        dwError = DsGetDcName(NULL,
                              NULL,
                              NULL,
                              NULL,
                              0,
                              &pdata->pdcInfo);

    }
}

void DestroyDCInfo(NETPLACESWIZARDDATA* pdata)
{
    // Free the memory allocated in DsGetDcName
    if (pdata->pdcInfo)
    {
        NetApiBufferFree(pdata->pdcInfo);
        pdata->pdcInfo = NULL;
    }
}
#endif

STDAPI_(DWORD)
NetPlacesWizardDoModal(
    LPCONNECTDLGSTRUCTW lpConnDlgStruct,
    NETPLACESWIZARDTYPE npwt,
    BOOL                fIsROPath
    )
// This function creates the Shared Folder Wizard.
// Return Value:
//  Returns WN_SUCCESS if the drive connected with no problem or 
//  RETCODE_CANCEL (0xFFFFFFFF) if the user cancels the Wizard or there 
//  is an unexplained/unrecoverable error

{
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof (iccex);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccex);

    LinkWindow_RegisterClass() ;

    if (npwt == NETPLACES_WIZARD_ADDPLACE)
    {    
        PROPSHEETPAGE        psp;
        HPROPSHEETPAGE       rghpage[4];
        PROPSHEETHEADER      psh;
       // Trace(TRACE_LEVEL_FLOW, TEXT("Entering NetPlacesWizardDoModal\n"));

        _ASSERT(lpConnDlgStruct);

        if (!SUCCEEDED(CoInitialize(NULL)))
            return RETCODE_CANCEL;

        //
        // Initialize the variables shared among all of the pages
        //

        // See if we're already running
        TCHAR szCaption[256];
        LoadString(g_hInstance, IDS_ADDNETPLACE_CAPTION, szCaption, ARRAYSIZE(szCaption));
        CEnsureSingleInstance ESI(szCaption);

        NETPLACESWIZARDDATA data;
        data.dwReturn = WN_SUCCESS;

        if (!ESI.ShouldExit())
        {
            data.fShowFoldersPage = FALSE;

            data.idWelcomePage = IDD_WIZ_WELCOME_ADDPLACE;
            data.idFoldersPage = IDD_WIZ_SELECTFOLDER_ADDPLACE;
            data.idPasswordPage = IDD_WIZ_PASSWORD_ADDPLACE;
            data.idCompletionPage = IDD_WIZ_COMPLETION_ADDPLACE;

            // Load the shell's image lists
            // Important: Don't free these! They're shared ALL OVER the place!
            if (!Shell_GetImageLists(&data.himlLarge, &data.himlSmall))
            {
                // If this fails, null out our lists
                data.himlLarge = data.himlSmall = NULL;
            }

            // Build the wizard pages
            CNetPlacesWizardPage1 page1(&data);
            CNetPlacesWizardPage2 page2(&data);
            CNetPlacesWizardPage3 page3(&data);
            CNetPlacesWizardPage4 page4(&data);

            int nPages = 0;
            psp.dwSize = sizeof (PROPSHEETPAGE);
            psp.hInstance = g_hInstance;


            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.pszTemplate = MAKEINTRESOURCE(data.idWelcomePage);
            page1.SetPropSheetPageMembers(&psp);
            rghpage[nPages++] = CreatePropertySheetPage(&psp);

            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.pszTemplate = MAKEINTRESOURCE(data.idFoldersPage);
            page2.SetPropSheetPageMembers(&psp);
            rghpage[nPages++] = CreatePropertySheetPage(&psp);

            if (data.idPasswordPage)
            {
                psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
                psp.pszTemplate = MAKEINTRESOURCE(data.idPasswordPage);
                page4.SetPropSheetPageMembers(&psp);
                rghpage[nPages++] = CreatePropertySheetPage(&psp);
            }

            // In any case, add the final page to our wizard
            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.pszTemplate = MAKEINTRESOURCE(data.idCompletionPage);

            page3.SetPropSheetPageMembers(&psp);
            rghpage[nPages++] = CreatePropertySheetPage(&psp);
  
            psh.dwSize     = sizeof(PROPSHEETHEADER);
            psh.dwFlags    = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE | PSH_NOAPPLYNOW;
            psh.pszCaption = szCaption;
            psh.hwndParent = lpConnDlgStruct->hwndOwner;

            psh.nPages     = nPages;
            psh.nStartPage = 0;
            psh.phpage     = rghpage;

            PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_PSW));
        }

        CoUninitialize();

        return data.dwReturn;
    }
    else
    {
        // Map net drive
        if (!SUCCEEDED(CoInitialize(NULL)))
            return RETCODE_CANCEL;

        // See if we're already running
        TCHAR szCaption[256];
        LoadString(g_hInstance, IDS_WIZARD_CAPTION_MAPDRIVE, szCaption, ARRAYSIZE(szCaption));
        CEnsureSingleInstance ESI(szCaption);

        DWORD dwReturn = RETCODE_CANCEL;
        if (!ESI.ShouldExit())
        {
            CMapNetDrivePage page(lpConnDlgStruct, &dwReturn);

            PROPSHEETPAGE        psp;
            psp.dwSize = sizeof (PROPSHEETPAGE);
            psp.hInstance = g_hInstance;
            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_MND_PAGE);
            page.SetPropSheetPageMembers(&psp);

            HPROPSHEETPAGE       hpage = CreatePropertySheetPage(&psp);

            PROPSHEETHEADER      psh;
            psh.dwSize     = sizeof(PROPSHEETHEADER);
            psh.dwFlags    = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE | PSH_NOAPPLYNOW;
            psh.pszCaption = szCaption;
            psh.hwndParent = lpConnDlgStruct->hwndOwner;

            psh.nPages     = 1;
            psh.nStartPage = 0;
            psh.phpage     = &hpage;

            PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_PSW));
        }

        CoUninitialize();

        return dwReturn;
    }
}

// User Manager stuff
#define MAX_EXTRA_CPL_PAGES    10
HPSXA AddExtraCplPages(ADDPROPSHEETDATA* ppsd)
{
    TraceEnter(TRACE_USR_CORE, "::AddExtraCplPages");

    HPSXA hpsxa = SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_USERSANDPASSWORDS_CPL, 
        MAX_EXTRA_CPL_PAGES, NULL);

    if (hpsxa != NULL)
    {
        UINT nPagesAdded = SHAddFromPropSheetExtArray(hpsxa, AddPropSheetPageCallback, (LPARAM) ppsd);
    }

    TraceLeaveValue(hpsxa);
}

/*********************************************************************************
 UsersRunDll

  pszCmdLine - The name of the "real" user of the machine. This can be different
                than the user context we are running the applet in. We use this
                username to warn when we need to log off for changes to be
                effective.
*********************************************************************************/

void APIENTRY UsersRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    TraceEnter(TRACE_USR_CORE, "::UsersRunDll");

    HRESULT hr = S_OK;
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
    *szDomainUser = 0;

    // Get the "real" user of this machine - this may be passed in the cmdline
    if (0 == *pszCmdLine)
    {
        // user wasn't passed, assume its the currently logged on user
        TCHAR szUser[MAX_USER + 1];
        DWORD cchUser = ARRAYSIZE(szUser);
        TCHAR szDomain[MAX_DOMAIN + 1];
        DWORD cchDomain = ARRAYSIZE(szDomain);

        if (0 != GetCurrentUserAndDomainName(szUser, &cchUser, szDomain, &cchDomain))
        {
            MakeDomainUserString(szDomain, szUser, szDomainUser, ARRAYSIZE(szDomainUser));
        }
    }
    else
    {
        // User name was passed in, just copy it
        MultiByteToWideChar(GetACP(), 0, pszCmdLine, -1, szDomainUser, ARRAYSIZE(szDomainUser));
    }

    // Initialize COM, but continue even if this fails.
    BOOL fComInited = SUCCEEDED(CoInitialize(NULL));

    // See if we're already running
    TCHAR szCaption[256];
    LoadString(g_hInstance, IDS_USR_APPLET_CAPTION, szCaption, ARRAYSIZE(szCaption));
    CEnsureSingleInstance ESI(szCaption);

    if (!ESI.ShouldExit())
    {
        // Create the security check dialog to ensure the logged-on user
        // is a local admin
        CSecurityCheckDlg dlg(szDomainUser);

        if (dlg.DoModal(g_hInstance, MAKEINTRESOURCE(IDD_USR_SECURITYCHECK_DLG), NULL) == IDOK)
        {
            // Create the shared user mgr object
            CUserManagerData data(szDomainUser);

            // Create the property sheet and page template
            // Maximum number of pages
            ADDPROPSHEETDATA ppd;
            ppd.nPages = 0;

            // Settings common to all pages
            PROPSHEETPAGE psp = {0};
            psp.dwSize = sizeof (psp);
            psp.hInstance = g_hInstance;

            // Create the userlist property sheet page and its managing object
            psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERLIST_PAGE);
            CUserlistPropertyPage userListPage(&data);
            userListPage.SetPropSheetPageMembers(&psp);
            ppd.rgPages[ppd.nPages++] = CreatePropertySheetPage(&psp);
    
            psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_ADVANCED_PAGE);
            CAdvancedPropertyPage advancedPage(&data);
            advancedPage.SetPropSheetPageMembers(&psp);
            ppd.rgPages[ppd.nPages++] = CreatePropertySheetPage(&psp);

            HPSXA hpsxa = AddExtraCplPages(&ppd);

            // Create the prop sheet
            PROPSHEETHEADER psh = {0};
            psh.dwSize = sizeof (psh);
            psh.dwFlags = PSH_DEFAULT;
            psh.hwndParent = hwndStub;
            psh.hInstance = g_hInstance;
            psh.pszCaption = szCaption;
            psh.nPages = ppd.nPages;
            psh.phpage = ppd.rgPages;

            // Show the property sheet
            int iRetCode = PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_USR_USERS));
    
            if (hpsxa != NULL)
            {
                SHDestroyPropSheetExtArray(hpsxa);
            }

            if (iRetCode == -1)
            {
                hr = E_FAIL;
            }
            else
            {
                hr = S_OK;
                // Special case when we must restart or reboot
                if (iRetCode == ID_PSREBOOTSYSTEM)
                {
                    RestartDialog(NULL, NULL, EWX_REBOOT);                
                }
                else if (iRetCode == ID_PSRESTARTWINDOWS)
                {
                    RestartDialog(NULL, NULL, EWX_REBOOT);                
                }
                else if (data.LogoffRequired())
                {
                    int iLogoff = DisplayFormatMessage(NULL, IDS_USERSANDPASSWORDS, IDS_LOGOFFREQUIRED, MB_YESNO | MB_ICONQUESTION);

                    if (iLogoff == IDYES)
                    {
                        // Tell explorer to log off the "real" logged on user. We need to do this
                        // since they may be running U&P as a different user.
                        HWND hwnd = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
                        if ( hwnd )
                        {
                            UINT uMsg = RegisterWindowMessage(TEXT("Logoff User"));

                            PostMessage(hwnd, uMsg, 0,0);
                        } 
                    }
                }
            }
        }
        else
        {
            // Security check told us to exit; either another instance of the CPL is starting
            // with admin priviledges or the user cancelled on the sec. check. dlg.
            hr = E_FAIL;
        }
    }

    if (fComInited)
        CoUninitialize();

    TraceLeave();
}

//
//  Set our Alt+Tab icon for the duration of a modal property sheet.
//
int PropertySheetIcon(LPCPROPSHEETHEADER ppsh, LPCTSTR pszIcon)
{
    int     iResult;
    HWND    hwnd, hwndT;
    BOOL    fChangedIcon = FALSE;
    HICON   hicoPrev;

    // This trick doesn't work for modeless property sheets
    _ASSERT(!(ppsh->dwFlags & PSH_MODELESS));

    // Don't do this if the property sheet itself already has an icon
    _ASSERT(ppsh->hIcon == NULL);

    // Walk up the parent/owner chain until we find the master owner.
    //
    // We need to walk the parent chain because sometimes we are given
    // a child window as our lpwd->hwnd.  And we need to walk the owner
    // chain in order to find the owner whose icon will be used for
    // Alt+Tab.
    //
    // GetParent() returns either the parent or owner.  Normally this is
    // annoying, but we luck out and it's exactly what we want.

    hwnd = ppsh->hwndParent;
    while ((hwndT = GetParent(hwnd)) != NULL)
    {
        hwnd = hwndT;
    }

    // If the master owner isn't visible we can futz his icon without
    // screwing up his appearance.
    if (!IsWindowVisible(hwnd))
    {
        HICON hicoNew = LoadIcon(g_hInstance, pszIcon);
        hicoPrev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoNew);
        fChangedIcon = TRUE;
    }

    iResult = (int)PropertySheet(ppsh);

    // Clean up our icon now that we're done
    if (fChangedIcon)
    {
        // Put the old icon back
        HICON hicoNew = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicoPrev);
        if (hicoNew)
            DestroyIcon(hicoNew);
    }

    return iResult;
}

// COM stuff
/***************************************************
 Exported COM function implementation
***************************************************/

STDAPI DllCanUnloadNow()
{
    TraceEnter(TRACE_USR_COM, "::DllCanUnloadNow");
    
    TraceLeaveValue( (g_cLocks == 0) );
}

STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppvObject)
{
    TraceEnter(TRACE_USR_COM, "::DllGetClassObject");

    HRESULT hr = E_FAIL;
    *ppvObject = NULL;

    if (clsid == CLSID_UserPropertyPages)
    {
        CUserPropertyPagesFactory* pFactory = new CUserPropertyPagesFactory();
        
        if (pFactory != NULL)
        {
            hr = pFactory->QueryInterface(iid, ppvObject);
            pFactory->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // We don't support this object!
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    TraceLeaveResult(hr);
}

/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.
*/
HRESULT _CallRegInstall(LPCSTR szSection, BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
#ifdef WINNT                
            STRENTRY seReg[] = {
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(g_hInstance, szSection, &stReg);
#else
            hr = pfnri(g_hInstance, szSection, NULL);
#endif                
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall 
                // (which does a registry restore) on an INF section that was 
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer()
{
    TraceEnter(TRACE_USR_COM, "::DllRegisterServer");

    _CallRegInstall("UnregDll", TRUE);

    HRESULT hres = _CallRegInstall("RegDll", FALSE);
    if ( SUCCEEDED(hres) )
    {
        //
        // if this is a workstation build then lets install the users and 
        // passwords control panel.  
        //

        NT_PRODUCT_TYPE NtProductType;
        RtlGetNtProductType(&NtProductType);            // get the product type
    
        if ( NtProductType == NtProductWinNt )
            hres = _CallRegInstall("RegDllWorkstation", FALSE);
    }

    TraceLeaveResult(hres);
}

STDAPI DllUnregisterServer()
{
    TraceEnter(TRACE_USR_COM, "::DllUnregisterServer");

    TraceLeaveResult(_CallRegInstall("UnregDll", TRUE));
}
