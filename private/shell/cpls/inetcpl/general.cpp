///////////////////////////////////////////////////////////////////////
//           Microsoft Windows                   //
//         Copyright(c) Microsoft Corp., 1995            //
///////////////////////////////////////////////////////////////////////
//
// GENERAL.C - "General" property page for InetCpl
//

// HISTORY:
//
// 6/22/96  t-gpease    moved code from dialdlg.c - no changes
//

#include "inetcplp.h"

#include <urlhist.h>
#include <initguid.h>
#include <shlguid.h>
#include <cleanoc.h>

#include <mluisupp.h>

//#include <shdocvw.h>
SHDOCAPI_(BOOL) ParseURLFromOutsideSourceA (LPCSTR psz, LPSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL);
SHDOCAPI_(BOOL) ParseURLFromOutsideSourceW (LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL);
#ifdef UNICODE
#define ParseURLFromOutsideSource ParseURLFromOutsideSourceW 
#else
#define ParseURLFromOutsideSource ParseURLFromOutsideSourceA 
#endif

// 
// See inetcplp.h for documentation on this flag
//
BOOL g_fReloadHomePage = FALSE;

// 
// Private Functions and Structures
//
// from cachecpl.c
#define CONTENT 0
BOOL InvokeCachevu(HWND hDlg);
INT_PTR CALLBACK EmptyCacheDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

INT_PTR CALLBACK ColorsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK AccessibilityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

/////// General Tab Info Structure ///////

typedef struct _GeneralTabInfo {
    HWND  hDlg;
    HWND  hwndUrl;
    TCHAR szCurrentURL[INTERNET_MAX_URL_LENGTH];   // current url in browser
    TCHAR szStartPageURL[INTERNET_MAX_URL_LENGTH]; // current url for start page
    
    BOOL    fInternalChange;
    BOOL    fChanged;
} GeneralTabInfo, *LPGENERALTABINFO, GENERALTABINFO;

void SetandSelectText(LPGENERALTABINFO pgti, HWND hwnd, LPTSTR psz);
BOOL GetHistoryFolderPath(LPTSTR pszPath);
void EmptyHistory();
void HistorySave(LPGENERALTABINFO pgti);
static DWORD GetDaysToKeep(VOID);
VOID SetDaysToKeep(DWORD dwDays);
void GetDefaultStartPage(LPGENERALTABINFO pgti);
HRESULT _GetStdLocation(LPTSTR pszPath, DWORD cbPathSize, UINT id);
HRESULT _SetStdLocation(LPTSTR szPath, UINT id);

// from shdocvw
#define IDS_DEF_HOME    998  //// WARNING!!! DO NOT CHANGE THESE VALUES
#define IDS_DEF_SEARCH  999 //// WARNING!!!  INETCPL RELIES ON THEM

#define IDS_SEARCHPAGE                  IDS_DEF_SEARCH
#define IDS_STARTPAGE                   IDS_DEF_HOME

#if defined(ux10) && defined(UNIX)
//Work around for mmap limitation in hp-ux10
#define MAX_HISTORY_DAYS        30
#else
#define MAX_HISTORY_DAYS        999
#endif

#define DEFAULT_DAYS_TO_KEEP    14
#define SAFERELEASE(p)      if(p) {(p)->Release(); (p) = NULL;}

TCHAR szDefURLValueNames[] = TEXT("Default_Page_URL");

//
// Functions
//
BOOL General_InitDialog(HWND hDlg)
{
    DWORD cb = sizeof(DWORD);
    LPGENERALTABINFO pgti;
#ifdef UNIX
    BOOL  bCacheIsReadOnly = FALSE;
#endif /* UNIX */

    // allocate memory for a structure which will hold all the info
    // gathered from this page
    //
    pgti = (LPGENERALTABINFO)LocalAlloc(LPTR, sizeof(GENERALTABINFO));
    if (!pgti)
    {
        EndDialog(hDlg, 0);
        return FALSE;
    }
    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pgti);

    // cross-lang platform support
    SHSetDefaultDialogFont(hDlg, IDC_START_ADDRESS);
    SHAutoComplete(GetDlgItem(hDlg, IDC_START_ADDRESS), SHACF_DEFAULT);                

    pgti->hDlg = hDlg;
    // enable the "Use Current" button if we have a current url
    StrCpyN(pgti->szCurrentURL, g_szCurrentURL, ARRAYSIZE(pgti->szCurrentURL));
    EnableWindow(GetDlgItem(hDlg, IDC_USECURRENT), pgti->szCurrentURL[0]);

    // get the url edit control and set the text limit
    pgti->hwndUrl = GetDlgItem(hDlg, IDC_START_ADDRESS);
    SendMessage(pgti->hwndUrl, EM_LIMITTEXT, ARRAYSIZE(pgti->szStartPageURL)-1, 0);

    GetDefaultStartPage(pgti);
    _GetStdLocation(pgti->szStartPageURL, ARRAYSIZE(pgti->szStartPageURL), IDS_STARTPAGE);
    SetandSelectText(pgti, pgti->hwndUrl, (LPTSTR)pgti->szStartPageURL);
    // set restrictions on history controls
    SendDlgItemMessage(pgti->hDlg, IDC_HISTORY_SPIN,
                       UDM_SETRANGE, 0, MAKELPARAM(MAX_HISTORY_DAYS, 0));

    SendDlgItemMessage(pgti->hDlg, IDC_HISTORY_SPIN,
                       UDM_SETPOS, 0, MAKELPARAM((WORD) GetDaysToKeep(), 0));

    Edit_LimitText(GetDlgItem(hDlg,IDC_HISTORY_DAYS),3);    // limit edit ctrl to 3 chars

    // only when invoked from View|Options
    if (g_szCurrentURL[0])
    {
        TCHAR szTitle[128];
        MLLoadString(IDS_INTERNETOPTIONS, szTitle, ARRAYSIZE(szTitle));
        SendMessage(GetParent(hDlg), WM_SETTEXT, 0, (LPARAM)szTitle);
    }

    // disable stuff based on restrictions
    if (g_restrict.fPlaces)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_START_ADDRESS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_USEDEFAULT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_USEBLANK), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_USECURRENT), FALSE);
    }
    
    if (g_restrict.fCacheReadOnly)
    {
       EnableWindow(GetDlgItem(hDlg, IDC_CACHE_DELETE_FILES), FALSE);
       EnableWindow(GetDlgItem(hDlg, IDC_CACHE_SETTINGS), FALSE);
    }
    
#ifdef UNIX
    bCacheIsReadOnly = IsCacheReadOnly();

    if (bCacheIsReadOnly)
    {
       EnableWindow(GetDlgItem(hDlg, IDC_CACHE_DELETE_FILES), FALSE);
       EnableWindow(GetDlgItem(hDlg, IDC_CACHE_SETTINGS), FALSE);
    }

    if (g_restrict.fCache || bCacheIsReadOnly)
    {
       TCHAR szText[1024];
 
       MLLoadString(IDS_READONLY_CACHE_TEXT,  szText, sizeof(szText));
 
       SetWindowText(GetDlgItem(hDlg, IDC_READONLY_CACHE_WARNING), szText);
       ShowWindow( GetDlgItem(hDlg, IDC_READONLY_CACHE_WARNING), SW_SHOW );
 
       ShowWindow( GetDlgItem(hDlg, IDC_TEMP_INTERNET_TEXT), SW_HIDE);
    }
#endif /* !UNIX */
    if (g_restrict.fHistory)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_HISTORY_DAYS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_HISTORY_SPIN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_HISTORY_CLEAR), FALSE);
    }
    return TRUE;
}

BOOL General_OnCommand(LPGENERALTABINFO pgti, UINT id, UINT nCmd)
{
    switch (id)
    { 

        case IDC_START_ADDRESS:
            switch (nCmd)
            {
                case EN_CHANGE:
                    if (!pgti->fInternalChange)
                    {
                        PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
                        pgti->fChanged = TRUE;
                    }
                    break;
            }
            break;

        case IDC_USECURRENT:
            if (nCmd == BN_CLICKED)
            {
                StrCpyN(pgti->szStartPageURL, pgti->szCurrentURL, ARRAYSIZE(pgti->szStartPageURL));
                SetandSelectText(pgti, pgti->hwndUrl,  pgti->szStartPageURL);
                PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
                pgti->fChanged = TRUE;
            }
            break;

        case IDC_USEDEFAULT:
            if (nCmd == BN_CLICKED)
            {
                GetDefaultStartPage(pgti);
                SetandSelectText(pgti, pgti->hwndUrl,  pgti->szStartPageURL);
                PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
                pgti->fChanged = TRUE;
            }
            break;

        case IDC_USEBLANK:
            if (nCmd == BN_CLICKED)
            {
                StrCpyN(pgti->szStartPageURL, TEXT("about:blank"), ARRAYSIZE(pgti->szStartPageURL));
                SetandSelectText(pgti, pgti->hwndUrl,  pgti->szStartPageURL);
                PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
                pgti->fChanged = TRUE;
            }
            break;

        case IDC_HISTORY_SPIN:
        case IDC_HISTORY_DAYS:
            if (pgti && (nCmd == EN_CHANGE))
            {
                PropSheet_Changed(GetParent(pgti->hDlg),pgti->hDlg);
                pgti->fChanged = TRUE;
            }
            break;

        case IDC_HISTORY_VIEW:
        {
            TCHAR szPath[MAX_PATH];

            if (!GetHistoryFolderPath(szPath))
            {
                GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
                PathAppend(szPath, TEXT("history"));
            }

            SHELLEXECUTEINFO shei= { 0 };

            shei.cbSize     = sizeof(shei);
            shei.lpFile     = szPath;
            shei.lpClass    = TEXT("Folder");
            shei.fMask      = SEE_MASK_CLASSNAME;
            shei.nShow      = SW_SHOWNORMAL;
            ShellExecuteEx(&shei);

            break;
        }

        case IDC_HISTORY_CLEAR:
            if (MsgBox(pgti->hDlg, IDS_ClearHistory, MB_ICONQUESTION,
                       MB_OKCANCEL | MB_DEFBUTTON2 )
                == IDOK)
            {

                HCURSOR hOldCursor = NULL;
                HCURSOR hNewCursor = NULL;

                // IEUNIX-Removing redundant use of MAKEINTRESOURCE
#ifndef UNIX
                hNewCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
#else
                hNewCursor = LoadCursor(NULL, IDC_WAIT);
#endif

                if (hNewCursor) 
                    hOldCursor = SetCursor(hNewCursor);

                EmptyHistory();

                if(hOldCursor)
                    SetCursor(hOldCursor);

            }
            break;

        case IDC_CACHE_SETTINGS:
            DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_TEMP_FILES),
                      pgti->hDlg, TemporaryDlgProc);

            break; // IDC_ADVANCED_CACHE_FILES_BUTTON

        case IDC_CACHE_DELETE_FILES:
        {
            INT_PTR iRet = DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_CACHE_EMPTY),
                             pgti->hDlg, EmptyCacheDlgProc);

            if ((iRet == 1) || (iRet == 3))
            {
                HCURSOR hOldCursor      = NULL;
                HCURSOR hAdvancedCursor = NULL;
                INTERNET_CACHE_CONFIG_INFOA icci;
                icci.dwContainer = CONTENT;

                GetUrlCacheConfigInfoA(&icci, NULL, CACHE_CONFIG_DISK_CACHE_PATHS_FC);

#ifndef UNIX
                hAdvancedCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
#else
                //IEUNIX-Removing redundant use of MAKEINTRESOURCE
                hAdvancedCursor = LoadCursor(NULL, IDC_WAIT);
#endif

                if (hAdvancedCursor)
                    hOldCursor = SetCursor(hAdvancedCursor);

                switch (iRet)   {
                    case 1:
                        FreeUrlCacheSpaceA(icci.CachePath, 100, STICKY_CACHE_ENTRY);
                        TraceMsg(TF_GENERAL, "Call FreeUrlCacheSpace with 0x%x",STICKY_CACHE_ENTRY);
                        break;
                    case 3:
                        FreeUrlCacheSpaceA(icci.CachePath, 100, 0 /*remove all*/);
                        TraceMsg(TF_GENERAL, "Call FreeUrlCacheSpace with 0");
                        break;
                    default:
                        break;
                }

                // Remove expired controls from Downloaded Program Files ( OCCache )
                // We'll do this silently, which leaves uncertain stuff behing, cuz
                // this is preferrable to raising a variable number of confirmation dialogs.
                RemoveExpiredControls( REC_SILENT, 0);
                TraceMsg(TF_GENERAL, "Call RemoveExpiredControls (silent)");

                if (hOldCursor)
                    SetCursor(hOldCursor);

            } 
            break;
        }

        case IDC_LANGUAGES:
            if (nCmd == BN_CLICKED)
            {
                KickLanguageDialog(pgti->hDlg);
            }
            break;

        case IDC_FONTS:
            if (nCmd == BN_CLICKED)
                OpenFontsDialogEx( pgti->hDlg, NULL );
            break;

        case IDC_COLORS:
            if (nCmd == BN_CLICKED)
                DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_COLORS), pgti->hDlg, ColorsDlgProc);
            break;

        case IDC_ACCESSIBILITY:
            if (nCmd == BN_CLICKED)
                DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_ACCESSIBILITY), pgti->hDlg, AccessibilityDlgProc);
            break;            
        
    }
    return TRUE;
}


void General_Apply(HWND hDlg)
{
    LPGENERALTABINFO pgti = (LPGENERALTABINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    if (pgti->fChanged)
    {
        INT_PTR iDays = SendDlgItemMessage(pgti->hDlg, IDC_HISTORY_SPIN, UDM_GETPOS, 0, 0 );
        TCHAR szStartPageURL[MAX_URL_STRING];
        
        SendMessage(pgti->hwndUrl, WM_GETTEXT, (WPARAM)ARRAYSIZE(szStartPageURL), (LPARAM)(szStartPageURL));
        
        if (szStartPageURL[0])
        {
            StrCpyN(pgti->szStartPageURL, szStartPageURL, ARRAYSIZE(pgti->szStartPageURL));
            PathRemoveBlanks(pgti->szStartPageURL);
            _SetStdLocation(pgti->szStartPageURL, IDS_STARTPAGE);
        }
        else
        {
            SendMessage(pgti->hwndUrl, WM_SETTEXT, (WPARAM)ARRAYSIZE(pgti->szStartPageURL), (LPARAM)(pgti->szStartPageURL));
        }

        // make sure that the edit box is not beyond the maximum allowed value
        if (iDays>=0xFFFF)
            iDays = MAX_HISTORY_DAYS;
        SetDaysToKeep((DWORD)iDays);

        UpdateAllWindows();
        // reset this flag, now that we've applied the changes
        pgti->fChanged = FALSE;
    }
}

void ReloadHomePageIfNeeded(LPGENERALTABINFO pgti)
{
    ASSERT(pgti);
    if (!pgti)
        return;

    if (g_fReloadHomePage)
    {
        //
        // If needed, reload the homepage url from the registry
        //
        _GetStdLocation(pgti->szStartPageURL, ARRAYSIZE(pgti->szStartPageURL), IDS_STARTPAGE);
        SetandSelectText(pgti, pgti->hwndUrl, (LPTSTR)pgti->szStartPageURL);

        g_fReloadHomePage = FALSE;
    }
}

INT_PTR CALLBACK General_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    // get our tab info structure
    LPGENERALTABINFO pgti;

    if (uMsg == WM_INITDIALOG)
        return General_InitDialog(hDlg);

    else
        pgti = (LPGENERALTABINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pgti)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    ReloadHomePageIfNeeded(pgti);
                    return TRUE;

                case PSN_KILLACTIVE:
#if defined(ux10) && defined(UNIX)
//Work around for mmap limitation in hp-ux10
                    INT_PTR iDays = SendDlgItemMessage(pgti->hDlg, IDC_HISTORY_SPIN, UDM_GETPOS, 0, 0 );
                    if (iDays > MAX_HISTORY_DAYS)
                    {
                      MessageBox(pgti->hDlg, TEXT("Days to keep pages in history cannot be greater 30."), NULL, MB_OK);
                      Edit_SetText(GetDlgItem(hDlg,IDC_HISTORY_DAYS), TEXT("30"));
                      SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                      return TRUE;
                    }
                    else
                    {
                      SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                      return TRUE;
                    }
#endif
                case PSN_QUERYCANCEL:
                case PSN_RESET:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return TRUE;

                case PSN_APPLY:
                    ReloadHomePageIfNeeded(pgti);
                    General_Apply(hDlg);
                    break;
            }
            break;                  
        }

        case WM_COMMAND:
            General_OnCommand(pgti, LOWORD(wParam), HIWORD(wParam));
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            // destroying this deliberately flushes its update (see WM_DESTROY in the UpdateWndProc);
            SHRemoveDefaultDialogFont(hDlg);

#ifndef UNIX
            // Should only be destroyed in process detach
            if (g_hwndUpdate)
                DestroyWindow(g_hwndUpdate);
#endif

            if (pgti)
                LocalFree(pgti);

            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);  // make sure we don't re-enter
            break;

    }
    return FALSE;
}


////////////////////////////////////////////////////////
//
// helper functions
//
////////////////////////////////////////////////////////

VOID SetDaysToKeep(DWORD dwDays)
{
    HKEY hk;
    DWORD dwDisp;

    DWORD Error = RegCreateKeyEx(
                                 HKEY_CURRENT_USER,
                                 REGSTR_PATH_URLHISTORY,
                                 0, NULL, 0,
                                 KEY_WRITE,
                                 NULL,
                                 &hk,
                                 &dwDisp);

    if(ERROR_SUCCESS != Error)
    {
        ASSERT(FALSE);
        return;
    }

    Error = RegSetValueEx(
                          hk,
                          REGSTR_VAL_DAYSTOKEEP,
                          0,
                          REG_DWORD,
                          (LPBYTE) &dwDays,
                          sizeof(dwDays));

    ASSERT(ERROR_SUCCESS == Error);

    RegCloseKey(hk);

    return;
}

static DWORD GetDaysToKeep(VOID)
{
    HKEY hk;
    DWORD cbDays = sizeof(DWORD);
    DWORD dwDays  = DEFAULT_DAYS_TO_KEEP;


    DWORD Error = RegOpenKeyEx(
                               HKEY_CURRENT_USER,
                               REGSTR_PATH_URLHISTORY,
                               0,
                               KEY_READ,
                               &hk);


    if(Error)
    {
        Error = RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             REGSTR_PATH_URLHISTORY,
                             0,
                             KEY_READ,
                             &hk);
    }


    if(!Error)
    {

        Error = RegQueryValueEx(
                                hk,
                                REGSTR_VAL_DAYSTOKEEP,
                                0,
                                NULL,
                                (LPBYTE) &dwDays,
                                &cbDays);


        RegCloseKey(hk);
    }

    return dwDays;
}

typedef HRESULT (* PCOINIT) (LPVOID);
typedef VOID (* PCOUNINIT) (VOID);
typedef VOID (* PCOMEMFREE) (LPVOID);
typedef HRESULT (* PCOCREINST) (REFCLSID, LPUNKNOWN, DWORD,     REFIID, LPVOID * );

HMODULE hOLE32 = NULL;
PCOINIT pCoInitialize = NULL;
PCOUNINIT pCoUninitialize = NULL;
PCOMEMFREE pCoTaskMemFree = NULL;
PCOCREINST pCoCreateInstance = NULL;

BOOL _StartOLE32()
{
    if (!hOLE32)
        hOLE32 = LoadLibrary(TEXT("OLE32.DLL"));

    if(!hOLE32)
        return FALSE;

    pCoInitialize = (PCOINIT) GetProcAddress(hOLE32, "CoInitialize");
    pCoUninitialize = (PCOUNINIT) GetProcAddress(hOLE32, "CoUninitialize");
    pCoTaskMemFree = (PCOMEMFREE) GetProcAddress(hOLE32, "CoTaskMemFree");
    pCoCreateInstance = (PCOCREINST) GetProcAddress(hOLE32, "CoCreateInstance");


    if(!pCoInitialize || !pCoUninitialize || !pCoTaskMemFree || !pCoCreateInstance)
        return FALSE;

    return TRUE;
}


void EmptyHistory()
{
    HRESULT hr = S_OK;
    IUrlHistoryStg2 *piuhs = NULL;

#ifdef UNIX
    LONG  lResult;
    HKEY  hkSubKey;
    DWORD dwIndex;
    TCHAR szSubKeyName[MAX_PATH + 1];
    DWORD cchSubKeyName = ARRAYSIZE(szSubKeyName);
    TCHAR szClass[MAX_PATH];
    DWORD cbClass = ARRAYSIZE(szClass);

    /* v-sriran: 12/18/97
     * In shdocvw/aclmru.cpp, we keep m_hKey as a handle to the key TypedURLs.
     * After deleting history, if somebody types something in the address bar,
     * we create the key again. So, here we are just deleting the contents of
     * the key TypedURLs and not the key itself.
     */
    /* Open the subkey so we can enumerate any children */
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER,
                           TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"),
                           0,
                           KEY_ALL_ACCESS,
                           &hkSubKey);
    if (ERROR_SUCCESS == lResult)
    {
       /* I can't just call RegEnumKey with an ever-increasing index, because */
       /* I'm deleting the subkeys as I go, which alters the indices of the   */
       /* remaining subkeys in an implementation-dependent way.  In order to  */
       /* be safe, I have to count backwards while deleting the subkeys.      */

       /* Find out how many subkeys there are */
       lResult = RegQueryInfoKey(hkSubKey,
                                 szClass,
                                 &cbClass,
                                 NULL,
                                 &dwIndex, /* The # of subkeys -- all we need */
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
         
       if (ERROR_SUCCESS == lResult) {
          /* dwIndex is now the count of subkeys, but it needs to be  */
          /* zero-based for RegEnumKey, so I'll pre-decrement, rather */
          /* than post-decrement. */
          while (ERROR_SUCCESS == RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
          {
                RegDeleteKey(hkSubKey, szSubKeyName);
          }
       }
  
       RegCloseKey(hkSubKey);
    }
#else
    // Warning : if you ever have subkeys - this will fail on NT
    RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"));
#endif

    // Warning : if you ever have subkeys - this will fail on NT
    RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"));

    // this broadcast will nuke the address bars
    SendBroadcastMessage(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"));
    SendBroadcastMessage(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"));

    //
    // As requested (bug 60089) we remove these reg values when history is
    // cleared.  This will reset the encoding menu UI to the defaults.
    //
    HKEY hkeyInternational = NULL;

    if (ERROR_SUCCESS == 
            RegOpenKeyEx(
                HKEY_CURRENT_USER,
                REGSTR_PATH_INTERNATIONAL,
                0,
                KEY_WRITE,
                &hkeyInternational))
    {

        ASSERT(hkeyInternational);

        RegDeleteValue(hkeyInternational, TEXT("CpCache"));
        RegDeleteValue(hkeyInternational, TEXT("CNum_CpCache"));
        
        RegCloseKey(hkeyInternational);

    }

    //  we will enumerate and kill each entry.  <gryn>
    //  this way we only kill peruser

    if(!hOLE32)
    {
        if(!_StartOLE32())
        {
            ASSERT(FALSE);
            return ;
        }
    }

    hr = pCoInitialize(NULL);

    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return;

    hr = pCoCreateInstance(
                           CLSID_CUrlHistory,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IUrlHistoryStg2,
                           (LPVOID *) &piuhs);

    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        goto quit;

    piuhs->ClearHistory();

quit:

    SAFERELEASE(piuhs);

    pCoUninitialize();

    return;

}

#define HISTORY 2

BOOL GetHistoryFolderPath(LPTSTR pszPath)
{
    INTERNET_CACHE_CONFIG_INFOA cci;
    cci.dwContainer = HISTORY;

    if (GetUrlCacheConfigInfoA(&cci, NULL, CACHE_CONFIG_DISK_CACHE_PATHS_FC))
    {
#ifdef UNICODE
        SHAnsiToUnicode(cci.CachePath, pszPath, MAX_PATH);
#else
        StrCpyN(pszPath, cci.CachePath, MAX_PATH);
#endif
        return TRUE;
    }
    return FALSE;
}

void SetandSelectText(LPGENERALTABINFO pgti, HWND hwnd, LPTSTR psz)
{
    pgti->fInternalChange = TRUE;
    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)psz);
    Edit_SetSel(hwnd, 0, 0);    // makesure everything is scrolled over first
    Edit_SetSel(hwnd, 0, -1);    // select everything
    pgti->fInternalChange = FALSE;
}

void GetDefaultStartPage(LPGENERALTABINFO pgti)
{
#ifdef UNICODE
    CHAR szPath[MAX_PATH];
    CHAR szValue[MAX_PATH];
    CHAR szURL[INTERNET_MAX_URL_LENGTH];

    SHUnicodeToAnsi(REGSTR_PATH_MAIN,szPath,ARRAYSIZE(szPath));
    SHUnicodeToAnsi(szDefURLValueNames,szValue,ARRAYSIZE(szValue));
    URLSubRegQueryA(szPath,
                    szValue,
                    TRUE,
                    szURL,
                    ARRAYSIZE(pgti->szStartPageURL),
                    URLSUB_ALL);
    SHAnsiToUnicode(szURL,pgti->szStartPageURL,ARRAYSIZE(pgti->szStartPageURL)); 
#else
    URLSubRegQueryA(REGSTR_PATH_MAIN,
                    szDefURLValueNames,
                    TRUE,
                    pgti->szStartPageURL,
                    ARRAYSIZE(pgti->szStartPageURL),
                    URLSUB_ALL);
#endif
}

HRESULT _GetStdLocation(LPTSTR pszPath, DWORD cbPathSize, UINT id)
{
    HRESULT hres = E_FAIL;
    LPCTSTR pszName;

    switch(id) {
        case IDS_STARTPAGE:
            pszName = REGSTR_VAL_STARTPAGE;
            break;

        case IDS_SEARCHPAGE:
            pszName = REGSTR_VAL_SEARCHPAGE;
            break;
#if 0
        case IDM_GOLOCALPAGE:
            pszName = REGSTR_VAL_LOCALPAGE;
            break;
#endif
        default:
            return E_INVALIDARG;
    }

#ifdef UNICODE
    CHAR szPath[MAX_PATH];
    CHAR szValue[MAX_PATH];
    CHAR szURL[INTERNET_MAX_URL_LENGTH];

    SHUnicodeToAnsi(REGSTR_PATH_MAIN,szPath,ARRAYSIZE(szPath));
    SHUnicodeToAnsi(pszName,szValue,ARRAYSIZE(szValue));
    if (SUCCEEDED(hres = URLSubRegQueryA(szPath, szValue, TRUE, 
                                         szURL, ARRAYSIZE(szURL), URLSUB_ALL)))
#else
    TCHAR szPath[MAX_URL_STRING];
    if (SUCCEEDED(hres = URLSubRegQueryA(REGSTR_PATH_MAIN, pszName, TRUE, 
                                         szPath, ARRAYSIZE(szPath), URLSUB_ALL)))
#endif
    {
#ifdef UNICODE
        SHAnsiToUnicode(szURL,pszPath,cbPathSize); 
#else
        StrCpyN(pszPath, szPath, cbPathSize);
#endif
    }
    return hres;
}

HRESULT _SetStdLocation(LPTSTR szPath, UINT id)
{
    HRESULT hres = E_FAIL;
    HKEY hkey;
    TCHAR szPage[MAX_URL_STRING];
    TCHAR szNewPage[MAX_URL_STRING];

    DWORD dwSize = sizeof(szNewPage);
    BOOL bSearch = FALSE;

    // BUGBUG: Share this code!!!
    // BUGBUG: This is Internet Explorer Specific

    _GetStdLocation(szPage, ARRAYSIZE(szPage), IDS_STARTPAGE);

    if ( ParseURLFromOutsideSource(szPath, szNewPage, &dwSize, &bSearch) &&
            (StrCmp(szPage, szNewPage) != 0) )
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         REGSTR_PATH_MAIN,
                         0,
                         KEY_WRITE,
                         &hkey)==ERROR_SUCCESS)
        {
            DWORD cbSize = (lstrlen(szNewPage)+1)*sizeof(TCHAR);
            if (RegSetValueEx(hkey,
                              REGSTR_VAL_STARTPAGE,
                              0,
                              REG_SZ,
                              (LPBYTE)szNewPage, cbSize)==ERROR_SUCCESS)
            {
                hres = S_OK;
            }
            RegCloseKey(hkey);
        }
    }
    return hres;
}
