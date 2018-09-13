//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************

//
//      OPENCPL.C - The is the heart of starting the "Internet Control Panel".
//  was DIALDLG.C

//      HISTORY:
//
//      4/5/95  jeremys         Created.
//
//      6/22/96 t-gpease        Moved everything that wasn't need (or global) 
//                              to start the control panel into separate, 
//                              managable pieces. This was part of the massive 
//                              (now defuncted) "dialdlg.c" that had just 
//                              about every property sheet written into it. 
//                              This completes the tearing apart of this 
//                              ("dialdlg.c") file. Whew!
//
//
// [arthurbi]
// WARNING - DO NOT ADD "static" variables to Dlg Procs!!!!
//      This code is designed to put up the same dlg in multi
//      threaded environments.  Using globals that maintain
//      state is dangerous.
//
//      State needs to be maintained in the hWin otherwise
//      it will generate bugs.
//

//
// [arthurbi]
// "REMOVED_ID", or DEAD Id: are based on PM decisions to no longer
//      have URLs Underlined.  Left around because PMs may always
//      change their mind.  In the meantime registry is always
//      there for people to change things.
//

#include "inetcplp.h"
#include <inetcpl.h>   // public header for INETCPL

#include <mluisupp.h>

extern HMODULE hOLE32;
typedef HRESULT (* PCOINIT) (LPVOID);
extern PCOINIT pCoInitialize;
extern BOOL _StartOLE32(); // initializes OLE

#define MAX_NUM_OPTION_PAGES    24

#ifndef UNIX
#define NUM_OPTION_PAGES        6
#else

#ifdef UNIX_FEATURE_ALIAS
#define NUM_OPTION_PAGES        8
#else
#define NUM_OPTION_PAGES        7
#endif /* UNIX_FEATURE_ALIAS */

#endif

RESTRICT_FLAGS g_restrict = {0};

//
// Warning!  LaunchConnectionDialog() below is sensitive to the order of
// this list - it assumes connection tab is 4th in list.  If you change it,
// fix the function.
//
#ifdef UNIX
extern INT_PTR CALLBACK DialupDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
#endif
const struct {
    DWORD dwPageFlag;
    int idRes;
    DLGPROC pfnDlgProc;
    BOOL *pfDisabled;
} c_PropInfo[] = {
    { INET_PAGE_GENERAL,    IDD_GENERAL,       General_DlgProc,     &g_restrict.fGeneralTab     },
    { INET_PAGE_SECURITY,   IDD_SECURITY,      SecurityDlgProc,     &g_restrict.fSecurityTab    },
    { INET_PAGE_CONTENT,    IDD_CONTENT,       ContentDlgProc,      &g_restrict.fContentTab     },
#ifndef UNIX
    { INET_PAGE_CONNECTION, IDD_CONNECTION,    ConnectionDlgProc,   &g_restrict.fConnectionsTab },
#else
    { INET_PAGE_CONNECTION, IDD_CONNECTION,    DialupDlgProc,       &g_restrict.fConnectionsTab },
#endif
    { INET_PAGE_PROGRAMS,   IDD_PROGRAMS,      ProgramsDlgProc,     &g_restrict.fProgramsTab    },
    { INET_PAGE_ADVANCED,   IDD_ADVANCED,      AdvancedDlgProc,     &g_restrict.fAdvancedTab    },
#ifdef UNIX
    { INET_PAGE_ASSOC,      IDD_ASSOCIATIONS,  AssocDlgProc,        NULL                        },
#ifdef UNIX_FEATURE_ALIAS
    { INET_PAGE_ALIAS,      IDD_ALIASDLG,      AliasDlgProc,        NULL                        },
#endif /* UNIX_FEATURE_ALIAS */
#endif
};

#define IsPropPageEnabled(iPage)  (c_PropInfo[iPage].pfDisabled==NULL || *c_PropInfo[iPage].pfDisabled==FALSE)

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#define ARRAYSIZE(arr) (sizeof(arr)/sizeof(arr[0]))


TCHAR g_szCurrentURL[INTERNET_MAX_URL_LENGTH] = {0};


WNDPROC pfnStaticWndProc = NULL;
HWND g_hwndUpdate = NULL;
BOOL g_fChangedMime = FALSE;     // needed to indicate to MSHTML to
// refresh the current page with a 
// new MIME and/or code page.
BOOL g_fSecurityChanged = FALSE;    // needed to indicate if a Active Security
// has changed.

HWND g_hwndPropSheet = NULL;
PFNPROPSHEETCALLBACK g_PropSheetCallback2 = NULL;

int CALLBACK PropSheetCallback(
    HWND hwndDlg,
    UINT uMsg,
    LPARAM lParam
)
{
    if (uMsg==PSCB_INITIALIZED)
    {
        ASSERT(hwndDlg);
        g_hwndPropSheet = hwndDlg;
    }
#ifdef REPLACE_PROPSHEET_TEMPLATE
    return g_PropSheetCallback2 ? g_PropSheetCallback2(hwndDlg, uMsg, lParam)
                                 : PropSheetProc(hwndDlg, uMsg, lParam);
#else
    return g_PropSheetCallback2 ? g_PropSheetCallback2(hwndDlg, uMsg, lParam)
                                 : 0;
#endif
}


// begin ui restrictions stuff 
// key below used to be \software\microsoft\intenet explorer\restrictui
//
const TCHAR c_szKeyRestrict[]       = REGSTR_PATH_INETCPL_RESTRICTIONS;

const TCHAR c_szGeneralTab[]        = REGSTR_VAL_INETCPL_GENERALTAB;
const TCHAR c_szSecurityTab[]       = REGSTR_VAL_INETCPL_SECURITYTAB;
const TCHAR c_szContentTab[]        = REGSTR_VAL_INETCPL_CONTENTTAB;
const TCHAR c_szConnectionsTab[]    = REGSTR_VAL_INETCPL_CONNECTIONSTAB;
const TCHAR c_szProgramsTab[]       = REGSTR_VAL_INETCPL_PROGRAMSTAB;
const TCHAR c_szAdvancedTab[]       = REGSTR_VAL_INETCPL_ADVANCEDTAB;
const TCHAR c_szColors[]            = TEXT("Colors");        
const TCHAR c_szLinks[]             = TEXT("Links");         
const TCHAR c_szFonts[]             = TEXT("Fonts");         
const TCHAR c_szInternational[]     = TEXT("Languages");                   // used to be International
const TCHAR c_szDialing[]           = TEXT("Connection Settings");         // used to be Dialing
const TCHAR c_szProxy[]             = TEXT("Proxy");         
const TCHAR c_szPlaces[]            = TEXT("HomePage");                    // used to be Places
const TCHAR c_szHistory[]           = TEXT("History");       
const TCHAR c_szMailNews[]          = TEXT("Messaging");                   // used to be MailNews
const TCHAR c_szResetWebSettings[]  = TEXT("ResetWebSettings");
const TCHAR c_szDefault[]           = TEXT("Check_If_Default");            // used to be Default
const TCHAR c_szRatings[]           = TEXT("Ratings");       
const TCHAR c_szCertif[]            = TEXT("Certificates");                // used to be Certif
const TCHAR c_szCertifPers[]        = TEXT("CertifPers");    
const TCHAR c_szCertifSite[]        = TEXT("CertifSite");    
const TCHAR c_szCertifPub[]         = TEXT("CertifPub");     
const TCHAR c_szCache[]             = TEXT("Cache");         
const TCHAR c_szAutoConfig[]        = TEXT("AutoConfig");
const TCHAR c_szAccessibility[]     = TEXT("Accessibility");
const TCHAR c_szSecChangeSettings[] = TEXT("SecChangeSettings");
const TCHAR c_szSecAddSites[]       = TEXT("SecAddSites");
const TCHAR c_szProfiles[]          = TEXT("Profiles");
const TCHAR c_szFormSuggest[]       = TEXT("FormSuggest");
const TCHAR c_szFormPasswords[]     = TEXT("FormSuggest Passwords");
#ifdef WALLET
const TCHAR c_szWallet[]            = TEXT("Wallet");
#endif
const TCHAR c_szConnectionWizard[]  = TEXT("Connwiz Admin Lock");
const TCHAR c_szCalContact[]        = TEXT("CalendarContact");             // used to be CalContact
const TCHAR c_szAdvanced[]          = TEXT("Advanced");
const TCHAR c_szSettings[]          = TEXT("Settings");

#if 0       // obsolete keys
const TCHAR c_szMultimedia[]     = TEXT("Multimedia");
const TCHAR c_szToolbar[]        = TEXT("Toolbar");       
const TCHAR c_szActiveX[]        = TEXT("ActiveX");       
const TCHAR c_szActiveDownload[] = TEXT("ActiveDownload");
const TCHAR c_szActiveControls[] = TEXT("ActiveControls");
const TCHAR c_szActiveScript[]   = TEXT("ActiveScript");  
const TCHAR c_szActiveJava[]     = TEXT("ActiveJava");    
const TCHAR c_szActiveSafety[]   = TEXT("ActiveSafety");  
const TCHAR c_szWarnings[]       = TEXT("Warnings");      
const TCHAR c_szOther[]          = TEXT("Other");         
const TCHAR c_szCrypto[]         = TEXT("Crypto");        
const TCHAR c_szFileTypes[]      = TEXT("FileTypes");     
#endif

// end ui restrictions stuff 

#ifdef REPLACE_PROPSHEET_TEMPLATE
// Used for modifying propsheet template
const TCHAR c_szComctl[] = TEXT("comctl32.dll");
#define DLG_PROPSHEET 1006 // Bad hack...assumes comctl's res id

typedef struct 
{
    int inumLang;
    WORD wLang;
} ENUMLANGDATA;

//
// EnumResLangProc
//
// purpose: a callback function for EnumResourceLanguages().
//          look into the type passed in and if it is RT_DIALOG
//          copy the lang of the first resource to our buffer 
//          this also counts # of lang if more than one of them 
//          are passed in
//
// IN:      lparam: ENUMLANGDATA - defined at the top of this file
//
BOOL CALLBACK EnumResLangProc(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIdLang, LPARAM lparam)
{
    ENUMLANGDATA *pel = (ENUMLANGDATA *)lparam;

    ASSERT(pel);

    if (lpszType == RT_DIALOG)
    {
        if (pel->inumLang == 0)
            pel->wLang = wIdLang;

        pel->inumLang++;
    }
    return TRUE;   // continue until we get all langs...
}

//
// GetDialogLang
//
// purpose: fill out the ENUMLANGDATA (see top of this file) with the
//          # of available langs in the module passed in, and the langid
//          of what system enumerates first. i.e, the langid eq. to what
//          the module localized if the module is localized in single 
//          language
//
// IN:      hinstCpl - this is supposed to be a instance handle of inetcpl.
//          pel - a pointer to the buffer we fill out
//
// RESULT:  TRUE  - everything cool, continue with adjusting property sheet
//          FALSE - somethings wrong, abort adjusting property sheet.
//
BOOL GetDialogLang(HMODULE hinstCpl, ENUMLANGDATA *pel)
{
    ASSERT(pel);
    OSVERSIONINFOA osvi;

    // Determine which version of NT or Windows we're running on
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA(&osvi);

    // Get the possible languages the template localized in.
    if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
        EnumResourceLanguages(hinstCpl, RT_DIALOG, MAKEINTRESOURCE(IDD_GENERAL), EnumResLangProc, (LPARAM)pel);
    else    
        EnumResourceLanguagesA(hinstCpl, (LPSTR)RT_DIALOG, MAKEINTRESOURCEA(IDD_GENERAL), (ENUMRESLANGPROCA)EnumResLangProc, (LPARAM)pel);

    return TRUE;
}
//
// PropSheetProc
//
// purpose: the callback function to modify resource template
//          in order to make DLG_PROPSHEET's lang mathed with ours.
//          there could be a general way but for now this is
//          an ugly hack from inetcpl.
//
//
int CALLBACK PropSheetProc (HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    LPVOID pTemplate = (LPVOID)lParam;
    LPVOID pTmpInLang = NULL;
    ENUMLANGDATA el = {0,0};
    HINSTANCE hComctl;
    HRSRC hrsrc;
    HGLOBAL hgmem;
    DWORD cbNewTmp;

    // Comm ctrl gives us a chance to recreate resource by this msg.
    if (uMsg==PSCB_PRECREATE && pTemplate)
    {
        // enumrate any possible language used in this cpl for dialogs
        if (!GetDialogLang(ghInstance, &el))
            return 0; // failed to get resouce name

        if (el.inumLang > 1)
        {
            // we've got multi-language templates
            // let comctl load the one that matches our thread langid.
            return 0;
        }
        if (GetSystemDefaultLangID() != el.wLang)
        {
            // Get comctl32's module handle
            hComctl = GetModuleHandle(c_szComctl);
            if (hComctl)
            {
                // this is a horrible hack because we assume DLG_PROPSHEET
                hrsrc = FindResourceExA(hComctl, (LPSTR)RT_DIALOG, MAKEINTRESOURCEA(DLG_PROPSHEET), el.wLang);
                if (hrsrc)
                {
                    if (hgmem = LoadResource(hComctl, hrsrc))
                    {
                        pTmpInLang = LockResource(hgmem);
                    } 
                    if (pTmpInLang)
                    {
                        cbNewTmp = SizeofResource(hComctl, hrsrc);
                        hmemcpy(pTemplate, pTmpInLang, cbNewTmp);
                    }    
                    if (hgmem && pTmpInLang)
                    {
                        UnlockResource(hgmem);
                        return 1; // everything went ok.
                    }
                }
            }
        }
    }
    return 0;
}
#endif // REPLACE_PROPSHEET_TEMPLATE

void ResWinHelp( HWND hwnd, int ids, int id2, DWORD_PTR dwp)
{
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];
    SHWinHelpOnDemandWrap((HWND)hwnd, LoadSz(ids,szSmallBuf,sizeof(szSmallBuf)),
            id2, (DWORD_PTR)dwp);
}

/*******************************************************************

NAME:       _AddPropSheetPage

SYNOPSIS:   Adds a property sheet page to a property sheet
header's array of property sheet pages.

********************************************************************/
BOOL CALLBACK _AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    BOOL bResult;
    LPPROPSHEETHEADER ppsh = (LPPROPSHEETHEADER)lParam;

    bResult = (ppsh->nPages < MAX_NUM_OPTION_PAGES);

    if (bResult)
        ppsh->phpage[ppsh->nPages++] = hpage;

    return(bResult);
}

// hunts down all windows and notifies them that they should update themselves
// a-msadek; Use SendMessage instead of PostMessage to work around a syncronization problem causing the system to
// hang when changing security settings
void UpdateAllWindows()
{
    SendMessage(g_hwndUpdate, INM_UPDATE, 0, 0);
    // PostMessage(g_hwndUpdate, INM_UPDATE, 0, 0);
}



// walk the window list and post a message to it telling the windows to
// update (borrowed from MSHTML and slightly modified)
void WINAPI MSHTMLNotifyAllRefresh()
{
    TCHAR szClassName[32];
    TCHAR *Hidden_achClassName;
    HWND hwnd = GetTopWindow(GetDesktopWindow());

    //
    // BUGBUG: These should be gotten from some place that is public
    //         to both MSHTML and INETCPL.
    //
    Hidden_achClassName = TEXT("Internet Explorer_Hidden");
#define WM_DO_UPDATEALL     WM_USER + 338

    while (hwnd) {
        GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName));

        // is this our hidden window?
        if (!StrCmpI(szClassName, Hidden_achClassName))
        {
            // yes...  post it a message..
            PostMessage(hwnd, WM_DO_UPDATEALL, 0, g_fSecurityChanged );
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    //
    // Notify top level windows that registry changed.
    // Defview monitors this message to update the listview
    // font underline settings on the fly.
    //
    SendBroadcastMessage(WM_WININICHANGE, SPI_GETICONTITLELOGFONT, (LPARAM)REGSTR_PATH_IEXPLORER);

    g_fSecurityChanged = FALSE;
}

// this window's sole purpose is to collect all the posted messages
// saying that we should update everything and collapse it down so that
// we only do it once
LRESULT WINAPI UpdateWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    switch(uMsg) {
        case WM_DESTROY:
            g_hwndUpdate = NULL;
            // update everything as needed.. remove all messages
            if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
                MSHTMLNotifyAllRefresh();
                SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pfnStaticWndProc);
            }
            // since EXPLORER.EXE can hang on to us for a long while
            // we should bail as many of these as we possibly can.
            if (g_hinstWinInet)
            {
                FreeLibrary(g_hinstWinInet);
                g_hinstWinInet = NULL;
            }

            if (g_hinstRatings)
            {
                FreeLibrary(g_hinstRatings);
                g_hinstRatings = NULL;
            }

            if (g_hinstUrlMon)
            {
                FreeLibrary(g_hinstUrlMon);
                g_hinstUrlMon = NULL;
            }

            if (g_hinstMSHTML)
            {
                FreeLibrary(g_hinstMSHTML);
                g_hinstMSHTML = NULL;
            }
            if (hOLE32)
            {
                FreeLibrary(hOLE32);
                hOLE32 = NULL;
            }
            if (g_hinstShdocvw)
            {
                FreeLibrary(g_hinstShdocvw);
                g_hinstShdocvw = NULL;
            }
            if (g_hinstCrypt32)
            {
                FreeLibrary(g_hinstCrypt32);
                g_hinstCrypt32 = NULL;
            }
                        
                        
            break;

        case INM_UPDATE:
            // remove any other posted messages of this type
            while( PeekMessage(&msg, hwnd, INM_UPDATE, INM_UPDATE, PM_REMOVE))
            {
            }
            MSHTMLNotifyAllRefresh();
            break;

        default:
            return CallWindowProc(pfnStaticWndProc, hwnd, uMsg, wParam, lParam);
    }
    return 0L;
}

// this creates our private message window that we use to collapse notifies
// of updates
void CreatePrivateWindow(HWND hDlg)
{
    // Don't allow creation of more than one Private Window per Instance
    if(g_hwndUpdate) { 
        return;
    }

    g_hwndUpdate = CreateWindow(TEXT("static"), NULL, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, ghInstance, NULL);
    if (g_hwndUpdate) {
        // subclass it
        pfnStaticWndProc = (WNDPROC) GetWindowLongPtr(g_hwndUpdate, GWLP_WNDPROC);
        SetWindowLongPtr(g_hwndUpdate, GWLP_WNDPROC, (LONG_PTR)UpdateWndProc);
    }
}


BOOL IsRestricted( HKEY hkey, const TCHAR * pszValue )
{
    LONG  lResult;
    DWORD lSize;
    DWORD  lValue;

    lValue = 0; // clear it
    lSize = sizeof(lValue);
    lResult = RegQueryValueEx( hkey, pszValue, NULL, NULL, (LPBYTE)&lValue, &lSize );
    if( ERROR_SUCCESS != lResult )
        return FALSE;

    return (0 != lValue);
}
void GetRestrictFlags( RESTRICT_FLAGS *pRestrict )
{
    LONG lResult;
    HKEY hkey;

    // BUGBUG: key is wrong!
    lResult = RegOpenKeyEx( HKEY_CURRENT_USER, c_szKeyRestrict, (DWORD)0, KEY_READ, &hkey );
    if( ERROR_SUCCESS != lResult )
        return;

#if 0
    pRestrict->fMultimedia    = IsRestricted( hkey, c_szMultimedia     );
    pRestrict->fToolbar       = IsRestricted( hkey, c_szToolbar        );
    pRestrict->fFileTypes     = IsRestricted( hkey, c_szFileTypes      );
    pRestrict->fActiveX       = IsRestricted( hkey, c_szActiveX        );
    pRestrict->fActiveDownload= IsRestricted( hkey, c_szActiveDownload );
    pRestrict->fActiveControls= IsRestricted( hkey, c_szActiveControls );
    pRestrict->fActiveScript  = IsRestricted( hkey, c_szActiveScript   );
    pRestrict->fActiveJava    = IsRestricted( hkey, c_szActiveJava     );
    pRestrict->fActiveSafety  = IsRestricted( hkey, c_szActiveSafety   );
    pRestrict->fWarnings      = IsRestricted( hkey, c_szWarnings       );
    pRestrict->fOther         = IsRestricted( hkey, c_szOther          );
    pRestrict->fCrypto        = IsRestricted( hkey, c_szCrypto         );
#endif
    
    pRestrict->fGeneralTab    = IsRestricted( hkey, c_szGeneralTab     );
    pRestrict->fSecurityTab   = IsRestricted( hkey, c_szSecurityTab    );
    pRestrict->fContentTab    = IsRestricted( hkey, c_szContentTab     );
    pRestrict->fConnectionsTab= IsRestricted( hkey, c_szConnectionsTab );
    pRestrict->fProgramsTab   = IsRestricted( hkey, c_szProgramsTab    );
    pRestrict->fAdvancedTab   = IsRestricted( hkey, c_szAdvancedTab    );
    pRestrict->fColors        = IsRestricted( hkey, c_szColors         );
    pRestrict->fLinks         = IsRestricted( hkey, c_szLinks          );
    pRestrict->fFonts         = IsRestricted( hkey, c_szFonts          );
    pRestrict->fInternational = IsRestricted( hkey, c_szInternational  );
    pRestrict->fDialing       = IsRestricted( hkey, c_szDialing        );
    pRestrict->fProxy         = IsRestricted( hkey, c_szProxy          );
    pRestrict->fPlaces        = IsRestricted( hkey, c_szPlaces         );
    pRestrict->fHistory       = IsRestricted( hkey, c_szHistory        );
    pRestrict->fMailNews      = IsRestricted( hkey, c_szMailNews       );
    pRestrict->fRatings       = IsRestricted( hkey, c_szRatings        );
    pRestrict->fCertif        = IsRestricted( hkey, c_szCertif         );
    pRestrict->fCertifPers    = IsRestricted( hkey, c_szCertifPers     );
    pRestrict->fCertifSite    = IsRestricted( hkey, c_szCertifSite     );
    pRestrict->fCertifPub     = IsRestricted( hkey, c_szCertifPub      );
    pRestrict->fCache         = IsRestricted( hkey, c_szCache          );
    pRestrict->fAutoConfig    = IsRestricted( hkey, c_szAutoConfig     );
    pRestrict->fAccessibility = IsRestricted( hkey, c_szAccessibility  );
    pRestrict->fSecChangeSettings = IsRestricted( hkey, c_szSecChangeSettings );
    pRestrict->fSecAddSites   = IsRestricted( hkey, c_szSecAddSites    );
    pRestrict->fProfiles      = IsRestricted( hkey, c_szProfiles       );
    pRestrict->fFormSuggest   = IsRestricted( hkey, c_szFormSuggest    );
    pRestrict->fFormPasswords = IsRestricted( hkey, c_szFormPasswords  );
#ifdef WALLET
    pRestrict->fWallet        = IsRestricted( hkey, c_szWallet         );
#endif
    pRestrict->fConnectionWizard = IsRestricted( hkey, c_szConnectionWizard );
    pRestrict->fCalContact    = IsRestricted( hkey, c_szCalContact     );
    pRestrict->fAdvanced      = IsRestricted( hkey, c_szAdvanced       );
    pRestrict->fCacheReadOnly = IsRestricted( hkey, c_szSettings       );
    pRestrict->fResetWebSettings = IsRestricted( hkey, c_szResetWebSettings );
    pRestrict->fDefault       = IsRestricted( hkey, c_szDefault        );
    
    RegCloseKey( hkey );
}

// this sets up any globals that need to be init'ed before the prop sheets come up
void PreInitPropSheets(HWND hDlg)
{
    INITCOMMONCONTROLSEX icex;

    //
    // initialize the ieak restrictions data
    //
    GetRestrictFlags(&g_restrict);

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_USEREX_CLASSES|ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);
    CreatePrivateWindow(hDlg);
}


/*******************************************************************

NAME:       AddInternetPropertySheetsEx

SYNOPSIS:   Adds the Internet property sheets through a provided
callback function.  Allows caller to specify common
parent reference count pointer and common callback
function for those property sheets.

********************************************************************/
STDAPI AddInternetPropertySheetsEx(LPFNADDPROPSHEETPAGE pfnAddPage,
                                   LPARAM lparam, PUINT pucRefCount,
                                   LPFNPSPCALLBACK pfnCallback,
                                   LPIEPROPPAGEINFO piepi)
{
    HRESULT hr = S_OK;
    PROPSHEETPAGE psPage = { 0 };
    int nPageIndex;
    LPPROPSHEETHEADER ppsh = (LPPROPSHEETHEADER)lparam;

    PreInitPropSheets(NULL);

    // if REPLACE_PROPSHEET_TEMPLATE is defined, then this
    // callback will call another proc
    // this is to modify a resource template so we can use 
    // the resource in matched language for property sheet
    g_PropSheetCallback2 = NULL;
    if (ppsh)
    {
        if (!(ppsh->dwFlags & PSH_USECALLBACK))
        {
            ppsh->dwFlags |= PSH_USECALLBACK;
        }
        else
        {
            g_PropSheetCallback2 = ppsh->pfnCallback;
        }
        ppsh->pfnCallback = PropSheetCallback;
    }

    if (piepi->pszCurrentURL)
    {
#ifdef UNICODE
        SHAnsiToUnicode(piepi->pszCurrentURL, g_szCurrentURL, ARRAYSIZE(g_szCurrentURL));
#else
        StrCpyN(g_szCurrentURL, piepi->pszCurrentURL, ARRAYSIZE(g_szCurrentURL));
#endif
    }
    else
        g_szCurrentURL[0] = 0;

    // fill out common data property sheet page struct
    psPage.dwSize = sizeof(psPage);
    psPage.hInstance = MLGetHinst();
    psPage.dwFlags = PSP_DEFAULT;
    if (pucRefCount)
    {
        psPage.pcRefParent = pucRefCount;
        psPage.dwFlags |= PSP_USEREFPARENT;
    }
    if (pfnCallback)
    {
        psPage.pfnCallback = pfnCallback;
        psPage.dwFlags |= PSP_USECALLBACK;
    }

    // if old IE30 users specify the old security page, then we'll show the new security page and the content page
    if (piepi->dwFlags & INET_PAGE_SECURITY_OLD)
        piepi->dwFlags |= INET_PAGE_SECURITY | INET_PAGE_CONTENT;

    // create a property sheet page for each page
    for (nPageIndex = 0; nPageIndex < ARRAYSIZE(c_PropInfo); nPageIndex++)
    {
        if (c_PropInfo[nPageIndex].dwPageFlag & piepi->dwFlags &&
            IsPropPageEnabled(nPageIndex))
        {
            HPROPSHEETPAGE hpage;

            psPage.pfnDlgProc  = c_PropInfo[nPageIndex].pfnDlgProc;
            psPage.pszTemplate = MAKEINTRESOURCE(c_PropInfo[nPageIndex].idRes);

            // set a pointer to the PAGEINFO struct as the private data for this page
            psPage.lParam = (LPARAM)nPageIndex;

            hpage = CreatePropertySheetPage(&psPage);

            if (hpage)
            {
                if (pfnAddPage(hpage, lparam))
                    hr = S_OK;
                else
                {
                    DestroyPropertySheetPage(hpage);
                    hr = E_FAIL;
                }
            }
            else
                hr = E_OUTOFMEMORY;

            if (hr != S_OK)
                break;
        }
    }

    // this must be done after the pre-init prop sheet which has first
    // crack at initializing g_restrict
    if (piepi->cbSize == sizeof(IEPROPPAGEINFO))
    {
        //
        // blast in the new settings
        //
        if (piepi->dwRestrictMask & (R_MULTIMEDIA | R_WARNINGS | R_CRYPTOGRAPHY | R_ADVANCED))
            g_restrict.fAdvanced = (piepi->dwRestrictFlags & (R_MULTIMEDIA | R_WARNINGS | R_CRYPTOGRAPHY | R_ADVANCED));

        if (piepi->dwRestrictMask & R_DIALING)
            g_restrict.fDialing    = (piepi->dwRestrictFlags & R_DIALING);

        if (piepi->dwRestrictMask & R_PROXYSERVER)        
            g_restrict.fProxy      = (piepi->dwRestrictFlags & R_PROXYSERVER);

        if (piepi->dwRestrictMask & R_CUSTOMIZE)       
            g_restrict.fPlaces     = (piepi->dwRestrictFlags & R_CUSTOMIZE);

        if (piepi->dwRestrictMask & R_HISTORY)
            g_restrict.fHistory    = (piepi->dwRestrictFlags & R_HISTORY);

        if (piepi->dwRestrictMask & R_MAILANDNEWS)
            g_restrict.fMailNews   = (piepi->dwRestrictFlags & R_MAILANDNEWS);

        if (piepi->dwRestrictMask & R_CHECKBROWSER )
            g_restrict.fDefault = (piepi->dwRestrictFlags & R_CHECKBROWSER);

        if (piepi->dwRestrictMask & R_COLORS)
            g_restrict.fColors  = (piepi->dwRestrictFlags & R_COLORS);

        if (piepi->dwRestrictMask & R_LINKS)
            g_restrict.fLinks    = (piepi->dwRestrictFlags & R_LINKS);

        if (piepi->dwRestrictMask & R_FONTS)        
            g_restrict.fFonts      = (piepi->dwRestrictFlags & R_FONTS);

        if (piepi->dwRestrictMask & R_RATINGS)       
            g_restrict.fRatings     = (piepi->dwRestrictFlags & R_RATINGS);

        if (piepi->dwRestrictMask & R_CERTIFICATES) {
            g_restrict.fCertif    = (piepi->dwRestrictFlags & R_CERTIFICATES);
            g_restrict.fCertifPers = g_restrict.fCertif;
            g_restrict.fCertifSite = g_restrict.fCertif;
            g_restrict.fCertifPub  = g_restrict.fCertif;
        }
        
        if (piepi->dwRestrictMask & (R_ACTIVECONTENT | R_SECURITY_CHANGE_SETTINGS))
            g_restrict.fSecChangeSettings   = (piepi->dwRestrictFlags & (R_ACTIVECONTENT | R_SECURITY_CHANGE_SETTINGS ));

        if (piepi->dwRestrictMask & (R_ACTIVECONTENT | R_SECURITY_CHANGE_SITES))
            g_restrict.fSecAddSites = (piepi->dwRestrictFlags & (R_ACTIVECONTENT | R_SECURITY_CHANGE_SITES));

        if (piepi->dwRestrictMask & R_CACHE)
            g_restrict.fCache  = (piepi->dwRestrictFlags & R_CACHE);

        if (piepi->dwRestrictMask & R_LANGUAGES )
            g_restrict.fInternational  = (piepi->dwRestrictFlags & R_LANGUAGES );
        
        if (piepi->dwRestrictMask & R_ACCESSIBILITY )
            g_restrict.fAccessibility = (piepi->dwRestrictFlags & R_ACCESSIBILITY);
        
        if (piepi->dwRestrictMask & R_PROFILES)
        {
            g_restrict.fFormSuggest=    // piggyback on "profile assistant" restriction
            g_restrict.fProfiles   = (piepi->dwRestrictFlags & R_PROFILES);
        }
        
#ifdef WALLET
        if (piepi->dwRestrictMask & R_WALLET)
            g_restrict.fWallet  = (piepi->dwRestrictFlags & R_WALLET);
#endif
        
        if (piepi->dwRestrictMask & R_CONNECTION_WIZARD)
            g_restrict.fConnectionWizard  = (piepi->dwRestrictFlags & R_CONNECTION_WIZARD);
        
        if (piepi->dwRestrictMask & R_AUTOCONFIG)
            g_restrict.fAutoConfig  = (piepi->dwRestrictFlags & R_AUTOCONFIG);
        
        if (piepi->dwRestrictMask & R_CAL_CONTACT)
            g_restrict.fCalContact  = (piepi->dwRestrictFlags & R_CAL_CONTACT);
        
        if (piepi->dwRestrictMask & R_ADVANCED)
            g_restrict.fAdvanced  = (piepi->dwRestrictFlags & R_ADVANCED);

        }

    return(hr);
}

STDAPI AddInternetPropertySheets(LPFNADDPROPSHEETPAGE pfnAddPage,
                                 LPARAM lparam, PUINT pucRefCount,
                                 LPFNPSPCALLBACK pfnCallback)
{
    IEPROPPAGEINFO iepi;

    iepi.cbSize = sizeof(iepi);
    iepi.pszCurrentURL = NULL;

    // if not loaded, try to load
    if (!g_hinstMSHTML)
        g_hinstMSHTML = LoadLibrary(c_tszMSHTMLDLL);

    // if MSHTML found, then do the standard INETCPL
    if (g_hinstMSHTML)
    {
        iepi.dwFlags = (DWORD)-1;
        iepi.dwRestrictFlags = (DWORD)0;
        iepi.dwRestrictMask  = (DWORD)0;
    }
    else
    {
        // adjust these flags for the "special" inetcpl for the office guys.
        iepi.dwFlags = (DWORD) 
                       INET_PAGE_CONNECTION | INET_PAGE_PROGRAMS
                       | INET_PAGE_SECURITY | INET_PAGE_ADVANCED;
        iepi.dwRestrictFlags = (DWORD)
                               R_HISTORY | R_OTHER | R_CHECKBROWSER;
        iepi.dwRestrictMask  = (DWORD)
                               R_HISTORY | R_OTHER | R_CHECKBROWSER;
    }

    return AddInternetPropertySheetsEx(pfnAddPage, lparam, pucRefCount, pfnCallback, &iepi);
}

void DestroyPropertySheets(LPPROPSHEETHEADER ppsHeader)
{
    UINT nFreeIndex;

    for (nFreeIndex = 0; nFreeIndex < ppsHeader->nPages; nFreeIndex++)
        DestroyPropertySheetPage(ppsHeader->phpage[nFreeIndex]);

}




/*******************************************************************

NAME:       LaunchInternetControlPanel

SYNOPSIS:   Runs the Internet control panel.


WARNING:  This needs to be as bare bones as possible.
the pages are also added in from the internet (iShellFolder)
property page extension which means it doesn't come into this
code.  if you do any initialization/destruction here,
it won't get called on all cases.

********************************************************************/
STDAPI_(BOOL) LaunchInternetControlPanelAtPage(HWND hDlg, UINT nStartPage)
{
    HPROPSHEETPAGE hOptPage[ MAX_NUM_OPTION_PAGES ];  // array to hold handles to pages
    PROPSHEETHEADER     psHeader;
    BOOL fRet;
    HPSXA hpsxa;
    HRESULT hrOle = SHCoInitialize();
    // OLE Needs to be initialized for AutoComplete and the FTP URL association.

    if (nStartPage != DEFAULT_CPL_PAGE &&
       (nStartPage < 0 || nStartPage >= ARRAYSIZE(c_PropInfo)))
    {
        nStartPage = DEFAULT_CPL_PAGE;
    }
    
    //
    // initialize the ieak restrictions data
    //
    GetRestrictFlags(&g_restrict);

    //
    // The caller will pass DEFAULT_CPL_PAGE for nStartPage when it doesn't care
    // which tab should be displayed first.  In this case, we just
    // display the first tab that's not disabled via IEAK
    //
    if (DEFAULT_CPL_PAGE == nStartPage)
    {
        int iPage;

        for (iPage=0; iPage<ARRAYSIZE(c_PropInfo); iPage++)
        {
            if (IsPropPageEnabled(iPage))
                break;
        }

        //
        // If the ieak has disabled ALL the pages, then we don't
        // can't display the inetcpl.  Show a message.
        //
        if (iPage == ARRAYSIZE(c_PropInfo))
        {
            MLShellMessageBox(
                hDlg, 
                MAKEINTRESOURCEW(IDS_RESTRICTED_MESSAGE), 
                MAKEINTRESOURCEW(IDS_RESTRICTED_TITLE), 
                MB_OK);
            return FALSE;
        }
        else
        {
            nStartPage = 0;
        }

    }

    //
    // This means the caller has requested that a specific start
    // page for the propsheet
    //
    else
    {
        int iPage;

        //
        // If the caller has requested a specific page, and that 
        // page is disabled due to IEAK restrictions, then don't 
        // display the inetcpl at all.  Show a messagebox.
        //
        if (!IsPropPageEnabled(nStartPage))
        {
            MLShellMessageBox(
                hDlg, 
                MAKEINTRESOURCE(IDS_RESTRICTED_MESSAGE), 
                MAKEINTRESOURCE(IDS_RESTRICTED_TITLE),
                MB_OK);
            return FALSE;

        }

        //
        // Due to IEAK restrictions, there may be one or more pages
        // before nStartPage that are actually missing.  We need to
        // decrement nStartPage to take that into account.
        //
        for (iPage=nStartPage-1; iPage>=0; iPage--)
            if (!IsPropPageEnabled(iPage))
                nStartPage--;

    }


    memset(&psHeader,0,sizeof(psHeader));

    psHeader.dwSize = sizeof(psHeader);
    psHeader.dwFlags = PSH_PROPTITLE;
    psHeader.hwndParent = hDlg;
    psHeader.hInstance = MLGetHinst();
    psHeader.nPages = 0;
    psHeader.nStartPage = nStartPage;
    psHeader.phpage = hOptPage;
    psHeader.pszCaption = MAKEINTRESOURCE(IDS_INTERNET_LOC);

    if (AddInternetPropertySheets(&_AddPropSheetPage, (LPARAM)&psHeader, NULL,
                                  NULL) == S_OK)
    {
        // add any extra pages from hooks in the registry
        if( ( hpsxa = SHCreatePropSheetExtArray( HKEY_LOCAL_MACHINE, 
            REGSTR_PATH_INETCPL_PS_EXTENTIONS, MAX_NUM_OPTION_PAGES - NUM_OPTION_PAGES ) ) != NULL )
        {
            SHAddFromPropSheetExtArray( hpsxa, _AddPropSheetPage, (LPARAM)&psHeader );
        }

        // bring the sucker up
        PropertySheet( &psHeader );

        // free the hooks if we loaded them 
        if ( hpsxa )
            SHDestroyPropSheetExtArray( hpsxa );

        fRet = TRUE;
    }
    else
    {
        DestroyPropertySheets(&psHeader);

        MsgBox(NULL, IDS_ERROutOfMemory, MB_ICONEXCLAMATION, MB_OK);
        fRet = FALSE;
    }

    SHCoUninitialize(hrOle);

    return fRet;
}

STDAPI_(BOOL) LaunchInternetControlPanel(HWND hDlg)
{
    return LaunchInternetControlPanelAtPage(hDlg, DEFAULT_CPL_PAGE);
}

STDAPI_(BOOL) LaunchConnectionDialog(HWND hDlg)
{
    ASSERT(INET_PAGE_CONNECTION == c_PropInfo[3].dwPageFlag);

    return LaunchInternetControlPanelAtPage(hDlg, 3);
}


BOOL IsCompatModeProcess()
{
    if (GetModuleHandle(TEXT("IE4.EXE")) || GetModuleHandle(TEXT("IESQRL.EXE")))
    {
        // If we are running in compat mode, exit because we don't want users
        // mucking with control panel from here.
        WinExec("control.exe", SW_NORMAL);
        
        return TRUE;
    }

    return FALSE;
}


