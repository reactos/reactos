//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************

//
// PROGRAMS.C - "Programs" property sheet UI handlers doe InetCpl
//
//

//
// History:
//
// 6/20/96  t-gpease    created
//

#include "inetcplp.h"

#include <mluisupp.h>
#include <advpub.h>

//
// Private Functions and Structures
//
BOOL ProgramsDlgInit( HWND hDlg);
void UpdateMailIconLabel();


typedef struct {
    HWND hDlg;          // dialog windows handle
    HWND hwndMail;      // Mail dropdown
    HWND hwndNews;      // News dropdown
    HWND hwndCalendar;  // Calendar dropdown
    HWND hwndContact;   // Contact dropdown
    HWND hwndCall;      // Internet call dropdown
    HWND hwndHtmlEdit;  // HTML Editors dropdown

    BOOL bAssociationCheck;     // Is IE the default browser?
#ifndef UNIX
    BOOL bIEIsFTPClient;  // Is IE the default FTP Client?
    IFtpInstaller * pfi;  // FTP Installer
#endif // UNIX

    int iHtmlEditor;
    int iMail;
    int iNews;
    int iCalendar;
    int iContact;
    int iCall;
    BOOL fChanged;

#ifdef UNIX
    HWND hwndVSource;   // View Source
    HWND hwndMailEdit;
    HWND hwndMailFind;
    HWND hwndNewsFind;
    HWND hwndNewsEdit;
    DWORD dwUseOEMail;
    DWORD dwUseOENews;
    HWND hwndEnableUseOEMail;
    HWND hwndEnableUseOENews;
    int  iVSource;
#endif

} PROGRAMSPAGE, *LPPROGRAMSPAGE;

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef WALLET
typedef int (*PFN_DISPLAYWALLETPAYDIALOG_PROC)(HWND, HINSTANCE, LPTSTR, int);
typedef int (*PFN_DISPLAYWALLETADDRDIALOG_PROC)(HWND, HINSTANCE, LPTSTR, int);
#endif

//
//
//
// "File Types" Dialog
//
//
//

// we only have to pull in one page.
#define NUM_FILETYPES_PAGES     1

static const char szAddFileTypesPS[] = "AddMIMEFileTypesPS";

#ifdef WALLET
// NOTE: This is dumb.  Wallet uses different GUIDs for Alpha and x86 versions
//
#ifdef _ALPHA_
static const TCHAR g_szWalletPaymentDirKey[] = TEXT("CLSID\\{B7FB4D5C-9FBE-11D0-8965-0000F822DEA9}\\InprocServer32");
static const TCHAR g_szWalletAddressDirKey[] = TEXT("CLSID\\{B7FB4D5C-9FBE-11D0-8965-0000F822DEA9}\\InprocServer32");
#else
static const TCHAR g_szWalletPaymentDirKey[] = TEXT("CLSID\\{87D3CB66-BA2E-11CF-B9D6-00A0C9083362}\\InprocServer32");
static const TCHAR g_szWalletAddressDirKey[] = TEXT("CLSID\\{87D3CB63-BA2E-11CF-B9D6-00A0C9083362}\\InprocServer32");
#endif

static const char g_szWalletPaymentFN[] = "DisplayWalletPaymentDialog";
static const char g_szWalletAddressFN[] = "DisplayWalletAddressDialog";
#endif


BOOL CALLBACK AddFileTypesPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    BOOL bResult;
    LPPROPSHEETHEADER ppsh = (LPPROPSHEETHEADER)lParam;

    bResult = (ppsh->nPages < NUM_FILETYPES_PAGES);

    if (bResult)
        ppsh->phpage[ppsh->nPages++] = hpage;

    return(bResult);
}

BOOL CreateFileTypesDialog(HWND hDlg)
{
    PROPSHEETPAGE       psp[NUM_FILETYPES_PAGES];  // only have "File Types" sheet
    PROPSHEETHEADER     psHeader;
    TCHAR               szFileTypes[MAX_PATH];

    typedef HRESULT (WINAPI *LPADDFILETYPESPS)(LPFNADDPROPSHEETPAGE pfnAddPage,
                                               LPARAM lparam);
    LPADDFILETYPESPS AddFileTypesPS;
    HINSTANCE hInstFTDll = NULL;

    // get the file name from resource
    TCHAR szDllFilename[SMALL_BUF_LEN+1];
    if (!MLLoadString(IDS_FTDLL_FILENAME,szDllFilename,ARRAYSIZE(szDllFilename)))
    {
        return FALSE;
    }

    // load the DLL
    hInstFTDll = LoadLibrary(szDllFilename);
    if (!hInstFTDll)
    {
        return FALSE;
    }

    // get Dialog Proc...
    if (!(AddFileTypesPS = (LPADDFILETYPESPS) GetProcAddress(hInstFTDll, szAddFileTypesPS)))
    {
        FreeLibrary(hInstFTDll);
        return FALSE;
    }

    memset(&psHeader,0,sizeof(psHeader));

    MLLoadString(IDS_FILETYPES, szFileTypes, ARRAYSIZE(szFileTypes));

    psHeader.dwSize = sizeof(psHeader);
    psHeader.dwFlags = PSH_NOAPPLYNOW | PSH_USECALLBACK ;
    psHeader.hwndParent = hDlg;
    psHeader.hInstance = MLGetHinst();
    psHeader.pszIcon = NULL;
    psHeader.nPages = 0;
    psHeader.ppsp = psp;
    psHeader.pszCaption = szFileTypes;

    // push the FileTypes page onto the sheet
    AddFileTypesPS(AddFileTypesPropSheetPage, (LPARAM)&psHeader);

    // don't care about the return value... wait for dialog to be done.
    PropertySheet(&psHeader);

    // all done... cleanup time...
    FreeLibrary(hInstFTDll);

    return TRUE;    // we succeeded
}


#ifdef WALLET
HINSTANCE GetWalletPaymentDProc(PFN_DISPLAYWALLETPAYDIALOG_PROC * ppfnDialogProc)
{
    TCHAR   szDLLFile[MAX_PATH];
    DWORD   dwType;
    DWORD   dwSize = SIZEOF(szDLLFile);
    HINSTANCE hInst = NULL;

    *ppfnDialogProc = NULL;
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, g_szWalletPaymentDirKey, NULL, &dwType, (LPVOID)szDLLFile, &dwSize))
    {
        hInst = LoadLibrary(szDLLFile);
        // Will Fail if OCX is not installed.
        if (hInst)
        {
            *ppfnDialogProc = (PFN_DISPLAYWALLETPAYDIALOG_PROC) GetProcAddress(hInst, g_szWalletPaymentFN);
        }
    }

    if (!*ppfnDialogProc && hInst)
    {
        FreeLibrary(hInst);
        hInst = NULL;
    }

    return hInst;
}


BOOL IsWalletPaymentAvailable(VOID)
{
    HINSTANCE hInst;
    PFN_DISPLAYWALLETPAYDIALOG_PROC pfnDialogProc;
    BOOL fIsAvailable = FALSE;

    hInst = GetWalletPaymentDProc(&pfnDialogProc);
    if (hInst)
    {
        fIsAvailable = TRUE;
        FreeLibrary(hInst);
    }

    return fIsAvailable;
}

VOID DisplayWalletPaymentDialog(HWND hWnd)
{
    HINSTANCE hInst;
    PFN_DISPLAYWALLETPAYDIALOG_PROC pfnDialogProc;

    hInst = GetWalletPaymentDProc(&pfnDialogProc);
    if (hInst)
    {
        (*pfnDialogProc)(hWnd, NULL, NULL, 0);
        FreeLibrary(hInst);
    }
}


HINSTANCE GetWalletAddressDProc(PFN_DISPLAYWALLETADDRDIALOG_PROC * ppfnDialogProc)
{
    TCHAR   szDLLFile[MAX_PATH];
    DWORD   dwType;
    DWORD   dwSize = SIZEOF(szDLLFile);
    HINSTANCE hInst = NULL;

    *ppfnDialogProc = NULL;
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, g_szWalletAddressDirKey, NULL, &dwType, (LPVOID)szDLLFile, &dwSize))
    {
        hInst = LoadLibrary(szDLLFile);
        // Will Fail if OCX is not installed.
        if (hInst)
        {
            *ppfnDialogProc = (PFN_DISPLAYWALLETADDRDIALOG_PROC) GetProcAddress(hInst, g_szWalletAddressFN);
        }
    }

    if (!*ppfnDialogProc && hInst)
    {
        FreeLibrary(hInst);
        hInst = NULL;
    }

    return hInst;
}

BOOL IsWallet3Installed()
{
    HINSTANCE hInst;
    PFN_DISPLAYWALLETADDRDIALOG_PROC pfnDialogProc;
    BOOL fWallet3 = FALSE;
    
    hInst = GetWalletAddressDProc(&pfnDialogProc);
    if (hInst)
    {
        CHAR chPath[MAX_PATH];
        
        if (GetModuleFileNameA(hInst, chPath, ARRAYSIZE(chPath)))
        {
            DWORD dwMSVer, dwLSVer;
            
            if (SUCCEEDED(GetVersionFromFile(chPath, &dwMSVer, &dwLSVer, TRUE)))
            {
                if (dwMSVer >= 3)
                {
                    fWallet3 = TRUE;
                }
            }
        }
        
        FreeLibrary(hInst);
    }

    return fWallet3;
}

BOOL IsWalletAddressAvailable(VOID)
{
    HINSTANCE hInst;
    PFN_DISPLAYWALLETADDRDIALOG_PROC pfnDialogProc;
    BOOL fIsAvailable = FALSE;

    hInst = GetWalletAddressDProc(&pfnDialogProc);
    if (hInst)
    {
        fIsAvailable = TRUE;
        FreeLibrary(hInst);
    }

    return fIsAvailable;
}

VOID DisplayWalletAddressDialog(HWND hWnd)
{
    HINSTANCE hInst;
    PFN_DISPLAYWALLETADDRDIALOG_PROC pfnDialogProc;

    hInst = GetWalletAddressDProc(&pfnDialogProc);
    if (hInst)
    {
        (*pfnDialogProc)(hWnd, NULL, NULL, 0);
        FreeLibrary(hInst);
    }
}
#endif

//
//
//
// "Programs" Tab
//
//
//


#ifndef UNIX
//
// RegPopulateComboBox()
//
// Takes an open HKEY (hkeyProtocol) and populates hwndCB with the friendly
// names of clients. The currently selected client is the "(default)" key of
// hkeyProtocol. The clients are sub-keys under the open key. The friendly
// names of the clients are in the "(default)" value of these sub-keys. This
// function also makes the currently selected client the selected item in
// hwndCB and returns the index number to the item.
//
// History:
//
// 7/ 8/96  t-gpease    created
//
UINT RegPopulateComboBox(HWND hwndCB, HKEY hkeyProtocol)
{
    TCHAR           szFriendlyName      [MAX_PATH];
    TCHAR           szKeyName           [MAX_PATH];
    TCHAR           szCurrent           [MAX_PATH];
    TCHAR           szFriendlyCurrent   [MAX_PATH];
    FILETIME        ftLastWriteTime;

    DWORD   i;              // Index counter

    HKEY    hkeyClient;
    DWORD   cb;

    // find the currently selected client
    cb = sizeof(szCurrent);
    if (RegQueryValueEx(hkeyProtocol, NULL, NULL, NULL, (LPBYTE)szCurrent, &cb)
        != ERROR_SUCCESS)
    {
        // if not found then blank the friendly name and keyname.
        szCurrent[0]=0;
        szFriendlyCurrent[0]=0;
    }

    // populate the dropdown
    for(i=0;                    // always start with 0
        cb=ARRAYSIZE(szKeyName),   // string size
        ERROR_SUCCESS==RegEnumKeyEx(hkeyProtocol, i, szKeyName, &cb, NULL, NULL, NULL, &ftLastWriteTime);
        i++)                    // get next entry
    {
        // get the friendly name of the client
        if (RegOpenKeyEx(hkeyProtocol, szKeyName, 0, KEY_READ, &hkeyClient)==ERROR_SUCCESS)
        {
            cb = sizeof(szFriendlyName);
            if (RegQueryValueEx(hkeyClient, NULL, NULL, NULL, (LPBYTE)szFriendlyName, &cb)
                == ERROR_SUCCESS)
            {
                // add name to dropdown
                SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)szFriendlyName);

                // check to see if it's the current default
                if (!StrCmp(szKeyName, szCurrent))
                {
                    // save its the friendly name which we'll use later to
                    // select the current client and what index it is.
                    StrCpyN(szFriendlyCurrent, szFriendlyName, ARRAYSIZE(szFriendlyCurrent));
                }
            }

            // close key
            RegCloseKey(hkeyClient);
        }

    }   // for

    // select current client and get index number... just in case listboxes are sorted we
    // are doing this last.
    return (unsigned int) SendMessage(hwndCB, CB_SELECTSTRING, (WPARAM) 0, (LPARAM) szFriendlyCurrent);
}   // RegPopulateComboBox()
#else
#define RegPopulateComboBox RegPopulateEditText
#endif /* UNIX */

//
// Adds the item and its associated HKEY in the combobox.  Stores the hkey
// as data associated with the item. Frees hkey if the item is already present
// or if an error occurs.
//
BOOL AddItemToEditorsCombobox
(
    HWND hwndCB, 
    LPTSTR pszFriendlyName, // friendly name of the app
    HKEY hkey               // location of assoc shell\edit verb
)
{
    ASSERT(pszFriendlyName);
    ASSERT(hkey);

    BOOL fRet = FALSE;

    // Only add if not already in combo
    if (SendMessage(hwndCB, CB_FINDSTRINGEXACT, -1, (LPARAM)pszFriendlyName) == CB_ERR)
    {
        // Add name to dropdown
        INT_PTR i = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)pszFriendlyName);
        if (i >= 0)
        {
            fRet = (SendMessage(hwndCB, CB_SETITEMDATA, i, (LPARAM)hkey) != CB_ERR);
        }
    }

    if (!fRet)
    {
        RegCloseKey(hkey);
    }
    return fRet;
}

//
// Adds the edit verb to the OpenWithList associated with .htm files.
//
void AddToOpenWithList(LPCTSTR pszFriendly, HKEY hkeyFrom, HKEY hkeyOpenWithList)
{
    ASSERT(pszFriendly);
    ASSERT(hkeyFrom);

    if (NULL == hkeyOpenWithList)
    {
        return;
    }

    TCHAR szBuf[MAX_PATH];
    StrCpyN(szBuf, pszFriendly, ARRAYSIZE(szBuf));
    StrCatBuff(szBuf, TEXT("\\shell\\edit"), ARRAYSIZE(szBuf));

    DWORD dwDisposition;
    HKEY hkeyDest;
    if (hkeyOpenWithList &&
        ERROR_SUCCESS == RegCreateKeyEx(hkeyOpenWithList, szBuf, 0,
                            NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyDest, &dwDisposition))
    {
        // Copy everything under shell if this item did not exist
        if (dwDisposition == REG_CREATED_NEW_KEY)
        {
            SHCopyKey(hkeyFrom, L"shell\\edit", hkeyDest, 0); 
        }
        RegCloseKey(hkeyDest);
    }
}

//
// Returns TRUE if the verb
//
BOOL IsHtmlStub
(
    HKEY hkeyVerb,  // reg location of the shell\verb\command
    LPCWSTR pszVerb // verb to check for ("edit" or "open")
)
{
    BOOL fRet = FALSE;

    // We don't display programs that are simple redirectors (such as Office's msohtmed.exe)
    TCHAR sz[MAX_PATH];
    if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, hkeyVerb, pszVerb, sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
    {
        // Get the MULTISZ list of known redirectors 
        TCHAR szRedir[MAX_PATH];
        ZeroInit(szRedir, ARRAYSIZE(szRedir)); // Protect against non-multisz strings in the reg
        DWORD dwType;
        DWORD cb = sizeof(szRedir) - 4;
        if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_DEFAULT_HTML_EDITOR, L"Stubs", &dwType, szRedir, &cb))
        {
            // Nothing in registry, so default to ignore the Office redirector
            StrCpyN(szRedir, L"msohtmed.exe\0", ARRAYSIZE(szRedir));
        }

        // Compare exe name with list of redirectors 
        LPCTSTR pszFile = PathFindFileName(sz);
        for (LPTSTR p = szRedir; *p != NULL; p += lstrlen(p) + 1)
        {
            if (StrCmpI(p, pszFile) == 0)
            {
                fRet = TRUE;
                break;
            }
        }
    }
    return fRet;
}


//
// Adds the html editors to the combobox. Looks for edit verbs associated
// with the .htm extension, the .htm OpenWithList, and the current default
// editor.
//
void PopulateEditorsCombobox(HWND hwndCB)
{
    //
    // Add items from the OpenWithList for .htm
    //
    DWORD dw;
    HKEY hkeyOpenWithList = NULL;
    TCHAR szOpenWith[MAX_PATH];
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, L".htm\\OpenWithList", 0,
                            NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyOpenWithList, &dw))
    {
        //
        // First add Notepad to the OpenWithList so we have at least one editor
        //
        TCHAR szPath[MAX_PATH];
        GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
        PathAddBackslash(szPath);
        StrCatBuff(szPath, L"Notepad.exe", ARRAYSIZE(szPath));
        TCHAR szNotepadFriendly[MAX_PATH];
        HKEY hkeyOpenWith;

        if (SUCCEEDED(AssocQueryString(ASSOCF_INIT_BYEXENAME | ASSOCF_VERIFY,
            ASSOCSTR_FRIENDLYAPPNAME, szPath, NULL, szNotepadFriendly, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szNotepadFriendly)))))
        {
            // If not already in the OpenWithList, add it
            if (ERROR_SUCCESS != RegOpenKeyEx(hkeyOpenWithList, szNotepadFriendly, 0, KEY_READ, &hkeyOpenWith))
            {
                // Compose command for the verb "c:\windows\notepad.exe %1"
                StrCatBuff(szPath, L" %1", ARRAYSIZE(szPath));

                // The verb goes under the friendly name
                TCHAR szKey[MAX_PATH];
                StrCpyN(szKey, szNotepadFriendly, ARRAYSIZE(szKey));
                StrCatBuff(szKey, L"\\shell\\edit\\command", ARRAYSIZE(szKey));
                SHSetValue(hkeyOpenWithList, szKey, NULL, REG_SZ, szPath, CbFromCch(lstrlen(szPath) + 1));
            }
            else
            {
                RegCloseKey(hkeyOpenWith);
            }
        }

        //
        // Next enumerate the entries in the OpenWithList
        //
        DWORD dwIndex = 0;
        DWORD dwSize = ARRAYSIZE(szOpenWith);
        while (ERROR_SUCCESS == RegEnumKeyEx(hkeyOpenWithList, dwIndex, szOpenWith, &dwSize, NULL, NULL, NULL, NULL))
        {
            if (ERROR_SUCCESS == RegOpenKeyEx(hkeyOpenWithList, szOpenWith, 0, KEY_READ, &hkeyOpenWith))
            {
                // We only accept items that have an edit verb
                TCHAR sz[MAX_PATH];
                if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, hkeyOpenWith, L"edit", sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
                {
                    // Note that we store hkeyOpenWith in the combo so don't close it
                    AddItemToEditorsCombobox(hwndCB, szOpenWith, hkeyOpenWith);
                }
                else
                {
                    RegCloseKey(hkeyOpenWith);
                }
            }

            ++dwIndex;
            dwSize = ARRAYSIZE(szOpenWith);
        }

        // hkeyOpenWithList is closed below
    }

    //
    // Add the editor associated with  .htm
    //
    HKEY hkeyHtm;

    //  BUGBUGTODO - should use AssocCreate(IQueryAssociations) here instead
    if (SUCCEEDED(AssocQueryKey(0, ASSOCKEY_SHELLEXECCLASS, L".htm", NULL, &hkeyHtm)))
    {
        TCHAR sz[MAX_PATH];
        if (!IsHtmlStub(hkeyHtm, L"edit") &&
             SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, hkeyHtm, L"edit", sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
        {
            AddItemToEditorsCombobox(hwndCB, sz, hkeyHtm);
            AddToOpenWithList(sz, hkeyHtm, hkeyOpenWithList);

            // Don't free the key we cached away
            hkeyHtm = NULL;
        }
        
        if (hkeyHtm)
        {
            RegCloseKey(hkeyHtm);
        }
    }

    //
    //  Get the default editor.  We check both hkcu & hklm.
    //
    HKEY hkeyDefault;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_DEFAULT_HTML_EDITOR, 0, KEY_READ, &hkeyDefault) ||
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DEFAULT_HTML_EDITOR, 0, KEY_READ, &hkeyDefault))
    {
        TCHAR sz[MAX_PATH];
        if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, hkeyDefault, L"edit", sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
        {
            // Add name to dropdown and save hkeyDefault in the combobox (so don't close it)
            AddItemToEditorsCombobox(hwndCB, sz, hkeyDefault);

            // Select this item
            SendMessage(hwndCB, CB_SELECTSTRING, -1, (LPARAM)sz);

            //
            // Make sure the default editor is in the htm OpenWithList so it doesn't dissapear
            // if we change it
            //
            AddToOpenWithList(sz, hkeyDefault, hkeyOpenWithList);
        }
        else
        {
            RegCloseKey(hkeyDefault);
        }

    }

    if (hkeyOpenWithList)
    {
        RegCloseKey(hkeyOpenWithList);
    }
}
    

//
// ProgramsDlgInit()
//
// Does initalization for Programs Dlg.
//

// History:
//
// 6/17/96  t-gpease   created
// 7/ 8/96  t-gpease   added Mail and News initialization
//
BOOL ProgramsDlgInit( HWND hDlg)
{
    LPPROGRAMSPAGE  pPrg;
    DWORD           dw;
    HKEY            hkey;

    pPrg = (LPPROGRAMSPAGE)LocalAlloc(LPTR, sizeof(*pPrg));
    if (!pPrg)
    {
        EndDialog(hDlg, 0);
        return FALSE;   // no memory?
    }

    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPrg);

    // save the handle to the page
    pPrg->hDlg = hDlg;

    //
    // Set default values.
    //

#ifndef UNIX
    pPrg->bAssociationCheck = TRUE;     // we want everybody to use IE!
    SUCCEEDED(CoCreateInstance(CLSID_FtpInstaller, NULL, CLSCTX_INPROC_SERVER, IID_IFtpInstaller, (void **) &pPrg->pfi));
    if (pPrg->pfi)
        pPrg->bIEIsFTPClient = ((S_OK == pPrg->pfi->IsIEDefautlFTPClient()) ? TRUE : FALSE);

#endif
    pPrg->iMail = -1;                   // nothing selected
    pPrg->iNews = -1;                   // nothing selected

#ifndef UNIX
    pPrg->bAssociationCheck = SHRegGetBoolUSValue(REGSTR_PATH_MAIN,REGSTR_VAL_CHECKASSOC,FALSE,TRUE);

    //
    // Get the html editors
    //
    pPrg->hwndHtmlEdit = GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_HTMLEDITOR_COMBO);
    PopulateEditorsCombobox(pPrg->hwndHtmlEdit);
    // Sundown: coercion to int because 32b is sufficient for cursor selection
    pPrg->iHtmlEditor = (int) SendMessage(pPrg->hwndHtmlEdit, CB_GETCURSEL, 0, 0);


    //
    // Get the Mail Clients
    //
    pPrg->hwndMail = GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_MAIL_COMBO);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_MAILCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
#else
    pPrg->hwndMail = GetDlgItem(pPrg->hDlg, IDC_EDIT_PROGRAMS_MAIL);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAILCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
#endif
    {
        // populate the combobox
        pPrg->iMail = RegPopulateComboBox(pPrg->hwndMail, hkey);

        // close the keys
        RegCloseKey(hkey);
    }

    //
    // Get the News Clients
    //
#ifndef UNIX
    pPrg->hwndNews = GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_NEWS_COMBO);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_NEWSCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
#else
    pPrg->hwndNews = GetDlgItem(pPrg->hDlg, IDC_EDIT_PROGRAMS_NEWS);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_NEWSCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
#endif
    {
        // populate the combobox
        pPrg->iNews = RegPopulateComboBox(pPrg->hwndNews, hkey);

        // close the keys
        RegCloseKey(hkey);
    }

    //
    // get the calendar clients
    //
#ifndef UNIX

    pPrg->hwndCalendar = GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_CALENDAR_COMBO);
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CALENDARCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
    {
        // populate the combobox
        pPrg->iCalendar = RegPopulateComboBox(pPrg->hwndCalendar, hkey);

        // close the keys
        RegCloseKey(hkey);
    }

    //
    // get the contacts clients
    //
    pPrg->hwndContact = GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_CONTACT_COMBO);
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTACTCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
    {
        // populate the combobox
        pPrg->iContact = RegPopulateComboBox(pPrg->hwndContact, hkey);

        // close the keys
        RegCloseKey(hkey);
    }

    //
    // get the internet call clients
    //
    pPrg->hwndCall = GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_CALL_COMBO);
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CALLCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
    {
        // populate the combobox
        pPrg->iCall = RegPopulateComboBox(pPrg->hwndCall, hkey);

        // close the keys
        RegCloseKey(hkey);
    }

    // Set dialog items
    CheckDlgButton(hDlg, IDC_CHECK_ASSOCIATIONS_CHECKBOX, pPrg->bAssociationCheck);

    HRESULT hrIEDefaultFTPClient = E_FAIL;
    if (pPrg->pfi)
    {
        hrIEDefaultFTPClient = pPrg->pfi->IsIEDefautlFTPClient();
        // Is this option not applicable because only the IE FTP client is installed?
        if (SUCCEEDED(hrIEDefaultFTPClient))
            CheckDlgButton(hDlg, IDC_PROGRAMS_IE_IS_FTPCLIENT, pPrg->bIEIsFTPClient);
    }
    if (FAILED(hrIEDefaultFTPClient))
    {
        // Yes, so remove the option.
        ShowWindow(GetDlgItem(hDlg, IDC_PROGRAMS_IE_IS_FTPCLIENT), SW_HIDE);
    }


    if( g_restrict.fMailNews )
    {
        EnableWindow( GetDlgItem(hDlg, IDC_PROGRAMS_MAIL_COMBO), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_PROGRAMS_NEWS_COMBO), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_PROGRAMS_CALL_COMBO), FALSE);
    }

    if ( g_restrict.fCalContact )
    {
        EnableWindow( GetDlgItem(hDlg, IDC_PROGRAMS_CALENDAR_COMBO), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_PROGRAMS_CONTACT_COMBO), FALSE);
    }

    EnableWindow( GetDlgItem(hDlg, IDC_RESETWEBSETTINGS), !g_restrict.fResetWebSettings );
    EnableWindow( GetDlgItem(hDlg, IDC_CHECK_ASSOCIATIONS_CHECKBOX), !g_restrict.fDefault );

#else /* !UNIX */

    pPrg->hwndVSource = GetDlgItem(pPrg->hDlg, IDC_EDIT_PROGRAMS_VSOURCE);
    pPrg->hwndMailFind = GetDlgItem(pPrg->hDlg, IDC_MAIL_FIND);
    pPrg->hwndMailEdit = GetDlgItem(pPrg->hDlg, IDC_MAIL_EDIT);
    pPrg->hwndNewsFind = GetDlgItem(pPrg->hDlg, IDC_NEWS_FIND);
    pPrg->hwndNewsEdit = GetDlgItem(pPrg->hDlg, IDC_NEWS_EDIT);
    pPrg->hwndEnableUseOEMail = GetDlgItem(pPrg->hDlg, IDC_OE_MAIL);
    pPrg->hwndEnableUseOENews = GetDlgItem(pPrg->hDlg, IDC_OE_NEWS);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_VSOURCECLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
    {
        // populate the combobox
        pPrg->iVSource = RegPopulateComboBox(pPrg->hwndVSource, hkey);

        // close the keys
        RegCloseKey(hkey);
    }

    // Set dialog items
    {
    // Look for the oexpress executable in the directory the
    // current process was executed from...  Use the value for the
    // excutable name as stored in the registry for msimn.exe
    DWORD dwType;
    TCHAR szPresentName[MAX_PATH];
    DWORD dwSize = sizeof(szPresentName)/sizeof(szPresentName[0]);
    if (SHGetValue(IE_USE_OE_PRESENT_HKEY, IE_USE_OE_PRESENT_KEY,
        IE_USE_OE_PRESENT_VALUE, &dwType, (void*)szPresentName, &dwSize) ||
        (dwType != REG_SZ) || LocalFileCheck(szPresentName))
    {
        // Disable user's access to OE settings
        EnableWindow(pPrg->hwndEnableUseOEMail, FALSE);
        EnableWindow(pPrg->hwndEnableUseOENews, FALSE);

        // Reset the internal state of the Use OE check buttons
        pPrg->dwUseOEMail = FALSE;
        pPrg->dwUseOENews = FALSE;
    }
    else
    {
        // Reset the values of the check button for use OE MAIL
        // Based on registry values
        dwSize = sizeof(pPrg->dwUseOEMail);
        if (SHGetValue(IE_USE_OE_MAIL_HKEY, IE_USE_OE_MAIL_KEY,
            IE_USE_OE_MAIL_VALUE, &dwType, (void*)&pPrg->dwUseOEMail,
            &dwSize) || (dwType != REG_DWORD))
        {
            // The default value for mail is FALSE
            pPrg->dwUseOEMail = FALSE;
        }
        // Reset the UI elements
        CheckDlgButton(hDlg, IDC_OE_MAIL, pPrg->dwUseOEMail ? BST_CHECKED :
            BST_UNCHECKED);
        EnableWindow(pPrg->hwndMail, pPrg->dwUseOEMail ? FALSE : TRUE);
        EnableWindow(pPrg->hwndMailEdit, pPrg->dwUseOEMail ? FALSE : TRUE);
        EnableWindow(pPrg->hwndMailFind, pPrg->dwUseOEMail ? FALSE : TRUE);

        // Repeat the above for the NEWS settings
        dwSize = sizeof(pPrg->dwUseOENews);
        if (SHGetValue(IE_USE_OE_NEWS_HKEY, IE_USE_OE_NEWS_KEY,
            IE_USE_OE_NEWS_VALUE, &dwType, (void*)&pPrg->dwUseOENews,
            &dwSize) || (dwType != REG_DWORD))
        {
            // The default value for News is FALSE
            pPrg->dwUseOENews = FALSE;
        }
        CheckDlgButton(hDlg, IDC_OE_NEWS, pPrg->dwUseOENews ? BST_CHECKED :
            BST_UNCHECKED);
        EnableWindow(pPrg->hwndNews, pPrg->dwUseOENews ? FALSE : TRUE);
        EnableWindow(pPrg->hwndNewsEdit, pPrg->dwUseOENews ? FALSE : TRUE);
        EnableWindow(pPrg->hwndNewsFind, pPrg->dwUseOENews ? FALSE : TRUE);
    }
    }

    if( g_restrict.fMailNews )
    {
        EnableWindow( GetDlgItem(hDlg, IDC_EDIT_PROGRAMS_MAIL), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_EDIT_PROGRAMS_NEWS), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_EDIT_PROGRAMS_VSOURCE), FALSE);

        EnableWindow( GetDlgItem(hDlg, IDC_OE_MAIL), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_OE_NEWS), FALSE);

        EnableWindow( GetDlgItem(hDlg, IDC_MAIL_FIND), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_NEWS_FIND), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_VSOURCE_FIND), FALSE);

        EnableWindow( GetDlgItem(hDlg, IDC_MAIL_EDIT), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_NEWS_EDIT), FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_VSOURCE_EDIT), FALSE);
    }

    SendMessage( pPrg->hwndMail,    EM_LIMITTEXT, MAX_PATH, 0 );
    SendMessage( pPrg->hwndNews,    EM_LIMITTEXT, MAX_PATH, 0 );
    SendMessage( pPrg->hwndVSource, EM_LIMITTEXT, MAX_PATH, 0 );

#endif /* !UNIX */

    return TRUE;    // success

} // ProgramsDlgInit()

//
// RegCopyKey()
//
// Copies all the keys from hkeySrc down into hkeyRoot:pszDest.
//
// History:
//
// 7/ 8/96  t-gpease    created
//
void RegCopyKey(HKEY hkeyRoot, const TCHAR *pszDest, HKEY hkeySrc)
{
    HKEY    hkeyDest;
    HKEY    hkey;
    DWORD   dw;
    DWORD   i;
    DWORD   cb;
    DWORD   cbData;
    DWORD   Type;
    TCHAR   szName[MAX_PATH];
    TCHAR   szData[MAX_URL_STRING+1];

    // open/create the destination key
    if (RegCreateKeyEx(hkeyRoot, pszDest,
                       NULL, 0, NULL, KEY_READ|KEY_WRITE, NULL, &hkeyDest, &dw) == ERROR_SUCCESS)
    {
        i=0;
        // copy values of the key
        while(1)
        {
            // find next value
            cb=ARRAYSIZE(szName);
            cbData=sizeof(szData);
            if (RegEnumValue(hkeySrc, i, szName, &cb, NULL, &Type, (LPBYTE)&szData, &cbData)!=ERROR_SUCCESS)
                break;  // not found... exit loop

            // make a copy of the value in new location
            RegSetValueEx(hkeyDest, szName, NULL, Type, (CONST BYTE *)szData, cbData);

#ifdef UNIX
            RegFlushKey(hkeyDest);
#endif

            // increase index count
            i++;

        }   // while

        // look for more sub-keys in the source
        for(i=0;
            cb=ARRAYSIZE(szName),
            RegEnumKey(hkeySrc, i, szName, cb)==ERROR_SUCCESS;
            i++)
        {
            // open the sub-key
            if (RegCreateKeyEx(hkeySrc, szName,
                               NULL, 0, NULL, KEY_READ, NULL, &hkey, &dw) == ERROR_SUCCESS)
            {
                // copy the sub-key
                RegCopyKey(hkeyDest, szName, hkey);

                // close the key
                RegCloseKey(hkey);

            }   // if RegCreateKey()

        }   // for

        // close the key
        RegCloseKey(hkeyDest);

    }   // if RegCreateKey()

} // RegCopyKey()

//
// CopyInfoTo()
//
// Copies the information needed to run a mailto: or news: protocol
//
// History:
//
// 7/ 8/96  t-gpease    created
//
void CopyInfoTo(const TCHAR *pszKeyName, HKEY hkeyClient)
{
    HKEY    hkey;
    TCHAR   szName[MAX_PATH];

    // create the protocol sub-key path
    StrCpyN(szName, TSZPROTOCOLSPATH, ARRAYSIZE(szName));
    int len = lstrlen(szName);
    StrCpyN(szName + len, pszKeyName, ARRAYSIZE(szName) - len);

    // make sure it has the protocol we are looking for
    if (RegOpenKeyEx(hkeyClient, szName, NULL, KEY_READ|KEY_WRITE, &hkey)
        ==ERROR_SUCCESS)
    {
        // Netscape Messenger registry patch: they are missing "URL Protocol" in the HKLM mailto branch 
        // and we should set this value, otherwise we get the 68992 bug.  The source is changed rather 
        // than the destination to protect against other programs copying the tree without the value.
        // Might as well check this for all clients rather than look for Netscape, since we don't
        // change any data if it exists.

        if (lstrcmpi(pszKeyName, TSZMAILTOPROTOCOL) == 0 && 
            RegQueryValueEx(hkey, TEXT("URL Protocol"), NULL, NULL, NULL, NULL) != ERROR_SUCCESS) 
        {    
            RegSetValueEx(hkey, TEXT("URL Protocol"), 0, REG_SZ, (BYTE *) TEXT(""), sizeof(TCHAR));
        }

        // start by deleting all the old info
        SHDeleteKey(HKEY_CLASSES_ROOT, pszKeyName);

        // recreate key and copy the protocol info
        RegCopyKey(HKEY_CLASSES_ROOT, pszKeyName, hkey);

        // close the key
        RegCloseKey(hkey);

    }   // if RegOpenKey()

}   // CopyInfoTo()


//
// FindClient()
//
// Finds the currently selected item in hwndComboBox and locates it
// in the szPath's subkeys. If found it will then make a call to copy
// the information to szProtocol key under HKCR.
//
// History:
//
// 7/ 8/96  t-gpease    created
//
void FindClient(LPCTSTR szProtocol, HWND hwndComboBox, int iSelected, LPCTSTR szPath)
{
    TCHAR           szFriendlyName[MAX_PATH];
    TCHAR           szKeyName[MAX_PATH];
    TCHAR           szCurrent[MAX_PATH];
    FILETIME        ftLastWriteTime;

    DWORD   i;              // Index counter
    HKEY    hkeyClient;
    HKEY    hkey;
    DWORD   dw;

    // get the name of the new client
    if (CB_ERR!=SendMessage(hwndComboBox, CB_GETLBTEXT, (WPARAM)iSelected, (LPARAM)szCurrent))
    {
        // got the friendly name... now lets find the internal name
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPath,
                           0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &hkey, &dw) == ERROR_SUCCESS)
        {
            DWORD   cb;

            // we must search all the sub-keys for the correct friendly name
            for(i=0;                    // always start with 0
                cb=ARRAYSIZE(szKeyName),   // string size
                ERROR_SUCCESS==RegEnumKeyEx(hkey, i, szKeyName, &cb, NULL, NULL, NULL, &ftLastWriteTime);
                i++)                    // get next entry
            {
                // get more info on the entry
                if (RegOpenKeyEx(hkey, szKeyName, 0, KEY_READ, &hkeyClient)==ERROR_SUCCESS)
                {
                    // get the friendly name of the client
                    cb = sizeof(szFriendlyName);
                    if (RegQueryValueEx(hkeyClient, NULL, NULL, NULL, (LPBYTE)szFriendlyName, &cb)
                        == ERROR_SUCCESS)
                    {
                        // is it the one we are looking for?
                        if (!StrCmp(szFriendlyName, szCurrent))
                        {
                            // yep... copy its info
                            CopyInfoTo(szProtocol, hkeyClient);

                            // make it the default handler
                            cb = (lstrlen(szKeyName) + 1)*sizeof(TCHAR);
                            RegSetValueEx(hkey, NULL, NULL, REG_SZ, (LPBYTE)szKeyName, cb);
                        }
                    }

                    // close key
                    RegCloseKey(hkeyClient);

                }   // if RegOpenKey()

            }   // for

            // close the keys
            RegCloseKey(hkey);

        }   // if RegCreateKeyEx()
    }
}   // FindClient()

//
// ProgramsDlgApplyNow()
//
// Enters "Programs" changes into registry.
//
// History:
//
// 6/20/96  t-gpease    created
// 7/ 8/96  t-gpease    added Mail and News
//
void ProgramsDlgApplyNow(LPPROGRAMSPAGE pPrg)
{
    int tmp;

    if (pPrg->fChanged)
    {
#ifndef UNIX
        // Did the user have a chance to change this option?
        if (pPrg->pfi)
        {
            HRESULT hrIEDefaultFTPClient = pPrg->pfi->IsIEDefautlFTPClient();
            if (IsWindowVisible(GetDlgItem(pPrg->hDlg, IDC_PROGRAMS_IE_IS_FTPCLIENT)))
            {
                // Yes, so see if they changed it.
                pPrg->bIEIsFTPClient = IsDlgButtonChecked(pPrg->hDlg, IDC_PROGRAMS_IE_IS_FTPCLIENT);

                // Did they user want IE as default and currently someone else is?
                if (pPrg->bIEIsFTPClient && (S_FALSE == hrIEDefaultFTPClient))
                    pPrg->pfi->MakeIEDefautlFTPClient();

                // Did they user NOT want IE as default and it currently is?
                if (!pPrg->bIEIsFTPClient && (S_OK == hrIEDefaultFTPClient))
                    pPrg->pfi->RestoreFTPClient();
            }
        }

        pPrg->bAssociationCheck = IsDlgButtonChecked(pPrg->hDlg, IDC_CHECK_ASSOCIATIONS_CHECKBOX);
        TCHAR szYesNo[5];

        StrCpyN(szYesNo, (pPrg->bAssociationCheck ? TEXT("yes") : TEXT("no")), ARRAYSIZE(szYesNo));

        SHRegSetUSValue(REGSTR_PATH_MAIN,
                        REGSTR_VAL_CHECKASSOC,
                        REG_SZ,
                        (LPVOID)szYesNo,
                        (lstrlen(szYesNo)+1)*sizeof(TCHAR),
                        SHREGSET_DEFAULT);
#else
        // Set if OE in use
        SHSetValue(IE_USE_OE_MAIL_HKEY, IE_USE_OE_MAIL_KEY,
            IE_USE_OE_MAIL_VALUE, REG_DWORD, (void*)&pPrg->dwUseOEMail,
            sizeof(pPrg->dwUseOEMail));

        // Set if OE in use
        SHSetValue(IE_USE_OE_NEWS_HKEY, IE_USE_OE_NEWS_KEY,
            IE_USE_OE_NEWS_VALUE, REG_DWORD, (void*)&pPrg->dwUseOENews,
            sizeof(pPrg->dwUseOENews));
#endif
        //
        // Save the new default editor
        //
#ifndef UNIX
        // See if the selection was changed
        tmp = (int) SendMessage(pPrg->hwndHtmlEdit, CB_GETCURSEL, 0, 0);
        if (tmp != pPrg->iHtmlEditor)
        {
            pPrg->iHtmlEditor = tmp;

            // Get the text and hkey for the selected item
            WCHAR szDefault[MAX_PATH];
            SendMessage(pPrg->hwndHtmlEdit, CB_GETLBTEXT, tmp, (LPARAM)szDefault);
            HKEY hkeyFrom = (HKEY)SendMessage(pPrg->hwndHtmlEdit, CB_GETITEMDATA, tmp, 0);

            if (hkeyFrom && (INT_PTR)hkeyFrom != CB_ERR)
            {
                //
                // Save the selected item as the default editor
                //
                DWORD dw;
                HKEY hkeyDest;
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_DEFAULT_HTML_EDITOR, 0,
                        NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyDest, &dw))
                {
                    // Update the name of the default editor
                    SHSetValue(hkeyDest, NULL, L"Description", REG_SZ, szDefault, CbFromCch(lstrlen(szDefault)+1));

                    // Delete the old shell command (and all subkeys).  This purges keys such as DDEEXEC.
                    SHDeleteKey(hkeyDest, L"shell");

                    // Update the verb of the default editor
                    HKEY hkeyEdit;
                    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyDest, L"shell\\edit", 0,
                            NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyEdit, &dw))
                    {
                        SHCopyKey(hkeyFrom, L"shell\\edit", hkeyEdit, 0);
                        RegCloseKey(hkeyEdit);
                    }
                    RegCloseKey(hkeyDest);
                }

                //
                // Also update Office's default editor
                //
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Shared\\HTML\\Default Editor", 0,
                        NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyDest, &dw))
                {
                    // Delete the old shell command (and all subkeys).  This purges keys such as DDEEXEC.
                    SHDeleteKey(hkeyDest, L"shell\\edit");

                    // Update the verb of the default editor
                    HKEY hkeyEdit;
                    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyDest, L"shell\\edit", 0,
                            NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyEdit, &dw))
                    {
                        SHCopyKey(hkeyFrom, L"shell\\edit", hkeyEdit, 0);
                        RegCloseKey(hkeyEdit);
                    }
                    RegCloseKey(hkeyDest);
                }

                //
                // Finally, update the edit verb for .htm files so that the shell honors the change.
                // But if .htm uses a stub like moshtmed, we leave it alone and assume that it
                // uses one of the above keys as the default editor.
                //
                HKEY hkeyHtm;
                if (SUCCEEDED(AssocQueryKey(0, ASSOCKEY_SHELLEXECCLASS, L".htm", NULL, &hkeyHtm)))
                {
                    if (!IsHtmlStub(hkeyHtm, L"edit"))
                    {
                        // Delete the old shell command (and all subkeys).  This purges keys such as DDEEXEC.
                        SHDeleteKey(hkeyHtm, L"shell\\edit");

                        // Copy the edit verb to the .htm
                        HKEY hkeyEdit;
                        if (ERROR_SUCCESS == RegCreateKeyEx(hkeyHtm, L"shell\\edit", 0,
                                NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkeyEdit, &dw))
                        {
                            SHCopyKey(hkeyFrom, L"shell\\edit", hkeyEdit, 0);
                            RegCloseKey(hkeyEdit);
                        }
                    }
    
                    RegCloseKey(hkeyHtm);
                }
            }
        }
#endif //!UNIX

        //
        // Save Mail Client Info
        //

        // is there a new client?
#ifndef UNIX
        tmp = (int) SendMessage(pPrg->hwndMail, CB_GETCURSEL, 0, 0);
        if (tmp!=pPrg->iMail)
        {
            pPrg->iMail = tmp;

            // find it and copy its info
            FindClient(TSZMAILTOPROTOCOL, pPrg->hwndMail, tmp, REGSTR_PATH_MAILCLIENTS);

            //Update the mail icon label
            UpdateMailIconLabel();
#else
        if (pPrg->iMail)
        {
            // copy its info
            FindEditClient(TSZMAILTOPROTOCOL, pPrg->hDlg, IDC_EDIT_PROGRAMS_MAIL,
               REGSTR_PATH_MAILCLIENTS);
#endif
            // tell the world that something has changed
            SendBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_MAILCLIENTS);
        }

        //
        // Save News Client Info
        //

        // is there a new client?
#ifndef UNIX
        tmp = (int) SendMessage(pPrg->hwndNews, CB_GETCURSEL, 0, 0);
        if (tmp!=pPrg->iNews)
        {
            pPrg->iNews = tmp;

            // find it and copy its info
            FindClient(TSZNEWSPROTOCOL, pPrg->hwndNews, tmp, REGSTR_PATH_NEWSCLIENTS);
            FindClient(TEXT("snews"), pPrg->hwndNews, tmp, REGSTR_PATH_NEWSCLIENTS);
            FindClient(TEXT("nntp"), pPrg->hwndNews, tmp, REGSTR_PATH_NEWSCLIENTS);
#else
        if (pPrg->iNews)
        {
            // copy its info
            FindEditClient(TSZNEWSPROTOCOL, pPrg->hDlg, IDC_EDIT_PROGRAMS_NEWS,
               REGSTR_PATH_NEWSCLIENTS);
            //FindEditClient(TEXT("snews"), pPrg->hwndNews, REGSTR_PATH_NEWSCLIENTS);
            //FindEditClient(TEXT("nntp"), pPrg->hwndNews, REGSTR_PATH_NEWSCLIENTS);
#endif

            // tell the world that something has changed
            SendBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_NEWSCLIENTS);
        }

        //
        // Save Internet Call Client info
        //

        // is there a new client?
#ifndef UNIX
        tmp = (int) SendMessage(pPrg->hwndCall, CB_GETCURSEL, 0, 0);
        if (tmp!=pPrg->iCall)
        {
            pPrg->iCall = tmp;

            // find it and copy its info
            FindClient(TSZCALLTOPROTOCOL, pPrg->hwndCall, tmp, REGSTR_PATH_CALLCLIENTS);

            // tell the world that something has changed
            SendBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_CALLCLIENTS);
        }
#else
        if (pPrg->iVSource)
        {
            // copy its info
            FindEditClient(TSZVSOURCEPROTOCOL, pPrg->hDlg, IDC_EDIT_PROGRAMS_VSOURCE,
               REGSTR_PATH_VSOURCECLIENTS);

            // tell the world that something has changed
            SendBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_VSOURCECLIENTS);
        }
#endif


#ifndef UNIX
        //
        // Save Contacts Client Info
        //

        // is there a new client?
        tmp = (int) SendMessage(pPrg->hwndContact, CB_GETCURSEL, 0, 0);
        if (tmp!=pPrg->iContact)
        {
            pPrg->iContact = tmp;

            // find it and copy its info
            FindClient(TSZLDAPPROTOCOL, pPrg->hwndContact, tmp, REGSTR_PATH_CONTACTCLIENTS);

            // tell the world that something has changed
            SendBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_CONTACTCLIENTS);
        }

        //
        // Save Calendar Client Info
        //

        // is there a new client?
        tmp = (int) SendMessage(pPrg->hwndCalendar, CB_GETCURSEL, 0, 0);
        if (tmp!=pPrg->iCalendar)
        {
            pPrg->iCalendar = tmp;

            // find it and copy its info
            FindClient(TSZCALENDARPROTOCOL, pPrg->hwndCalendar, tmp, REGSTR_PATH_CALENDARCLIENTS);

            // tell the world that something has changed
            SendBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_CALENDARCLIENTS);
        }
#endif
        UpdateAllWindows();
        pPrg->fChanged = FALSE;
    }
} // ProgramsDlgApplyNow()

    
extern HRESULT ResetWebSettings(HWND hwnd, BOOL *pfChangedHomePage);

#ifndef UNIX
//
// ProgramsOnCommand()
//
// Handles "Programs" property page's window commands
//
// History:
//
// 6/20/96  t-gpease    created
//
void ProgramsOnCommand(LPPROGRAMSPAGE pPrg, UINT id, UINT nCmd)
{

    switch (id) {

        case IDC_PROGRAMS_HTMLEDITOR_COMBO:
        {
            INT_PTR tmp;

            // Is there a new editor?
            tmp = SendMessage(pPrg->hwndHtmlEdit, CB_GETCURSEL, 0, 0);
            if (tmp != pPrg->iHtmlEditor)
            {
                ENABLEAPPLY(pPrg->hDlg);
                pPrg->fChanged = TRUE;
            }
        }
        break;

        case IDC_PROGRAMS_NEWS_COMBO:
        {
            INT_PTR tmp;
            // is there a new client?
            tmp = SendMessage(pPrg->hwndNews, CB_GETCURSEL, 0, 0);
            if (tmp != pPrg->iNews)
            {
                ENABLEAPPLY(pPrg->hDlg);
                pPrg->fChanged = TRUE;
            }
        }
        break;


        case IDC_PROGRAMS_MAIL_COMBO:
        {
            INT_PTR tmp;
            // is there a new client?
            tmp = SendMessage(pPrg->hwndMail, CB_GETCURSEL, 0, 0);
            if (tmp != pPrg->iMail)
            {
                ENABLEAPPLY(pPrg->hDlg);
                pPrg->fChanged = TRUE;
            }
        }
        break;

        case IDC_PROGRAMS_CALENDAR_COMBO:
        {
            INT_PTR tmp;
            // is there a new client?
            tmp = SendMessage(pPrg->hwndCalendar, CB_GETCURSEL, 0, 0);
            if (tmp != pPrg->iCalendar)
            {
                ENABLEAPPLY(pPrg->hDlg);
                pPrg->fChanged = TRUE;
            }
        }
        break;

        case IDC_PROGRAMS_CONTACT_COMBO:
        {
            INT_PTR tmp;
            // is there a new client?
            tmp = SendMessage(pPrg->hwndContact, CB_GETCURSEL, 0, 0);
            if (tmp != pPrg->iContact)
            {
                ENABLEAPPLY(pPrg->hDlg);
                pPrg->fChanged = TRUE;
            }
        }
        break;

        case IDC_PROGRAMS_CALL_COMBO:
        {
            INT_PTR tmp;
            // is there a new client?
            tmp = SendMessage(pPrg->hwndCall, CB_GETCURSEL, 0, 0);
            if (tmp != pPrg->iCall)
            {
                ENABLEAPPLY(pPrg->hDlg);
                pPrg->fChanged = TRUE;
            }
        }
        break;

        case IDC_RESETWEBSETTINGS:
            {
                BOOL fReloadHomePage;
                ResetWebSettings(pPrg->hDlg,&fReloadHomePage);
                if (fReloadHomePage)
                    g_fReloadHomePage = TRUE;
            }
            break;

        case IDC_PROGRAMS_IE_IS_FTPCLIENT:
        case IDC_CHECK_ASSOCIATIONS_CHECKBOX:
            ENABLEAPPLY(pPrg->hDlg);
            pPrg->fChanged = TRUE;
            break;
    } // switch

} // ProgramsOnCommand()

#else /* !UNIX */

//
// ProgramsOnCommand()
//
// Handles "Programs" property page's window commands
//
// History:
//
// 6/20/96  t-gpease    created
//
void ProgramsOnCommand(LPPROGRAMSPAGE pPrg, UINT id, UINT nCmd)
{

    switch (id) {

        case IDC_EDIT_PROGRAMS_MAIL:
        {
            if (nCmd == EN_CHANGE)
            {
                if (pPrg->iMail != -1)
                {
                    ENABLEAPPLY(pPrg->hDlg);
                    pPrg->iMail = 1;  //change has occoured
                    pPrg->fChanged = TRUE;
                }
            }
        }
        break;

        case IDC_OE_MAIL:
        {
            pPrg->dwUseOEMail = IsDlgButtonChecked(pPrg->hDlg,
                IDC_OE_MAIL) == BST_CHECKED;

            ENABLEAPPLY(pPrg->hDlg);
            pPrg->fChanged = TRUE;

            EnableWindow(pPrg->hwndMail, !pPrg->dwUseOEMail);
            EnableWindow(pPrg->hwndMailEdit, !pPrg->dwUseOEMail);
            EnableWindow(pPrg->hwndMailFind, !pPrg->dwUseOEMail);
        }
        break;

        case IDC_EDIT_PROGRAMS_NEWS:
        {
            if (nCmd == EN_CHANGE)
            {
                if (pPrg->iNews != -1)
                {
                    ENABLEAPPLY(pPrg->hDlg);
                    pPrg->iNews = 1;  //change has occoured
                    pPrg->fChanged = TRUE;
                }
            }
        }
        break;

        case IDC_OE_NEWS:
        {
            pPrg->dwUseOENews = IsDlgButtonChecked(pPrg->hDlg,
                IDC_OE_NEWS) == BST_CHECKED;

            ENABLEAPPLY(pPrg->hDlg);
            pPrg->fChanged = TRUE;

            EnableWindow(pPrg->hwndNews, !pPrg->dwUseOENews);
            EnableWindow(pPrg->hwndNewsEdit, !pPrg->dwUseOENews);
            EnableWindow(pPrg->hwndNewsFind, !pPrg->dwUseOENews);
        }
        break;

        case IDC_EDIT_PROGRAMS_VSOURCE:
        {
            if (nCmd == EN_CHANGE)
            {
                if (pPrg->iVSource != -1)
                {
                    ENABLEAPPLY(pPrg->hDlg);
                    pPrg->iVSource = 1;  //change has occoured
                    pPrg->fChanged = TRUE;
                }
            }
        }
        break;

        case IDC_MAIL_EDIT:
        {
            HKEY hkeyProtocol;
            DWORD dw;

            if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAILCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkeyProtocol, &dw) != ERROR_SUCCESS)
                break;
            EditScript(hkeyProtocol);
            RegCloseKey(hkeyProtocol);
        }
        break;

        case IDC_NEWS_EDIT:
        {
            HKEY hkeyProtocol;
            DWORD dw;

            if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_NEWSCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkeyProtocol, &dw) != ERROR_SUCCESS)
                break;
            EditScript(hkeyProtocol);
            RegCloseKey(hkeyProtocol);
        }
        break;

        case IDC_VSOURCE_EDIT:
        {
            HKEY hkeyProtocol;
            DWORD dw;

            if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_VSOURCECLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkeyProtocol, &dw) != ERROR_SUCCESS)
                break;
            EditScript(hkeyProtocol);
            RegCloseKey(hkeyProtocol);
        }
        break;

        case IDC_MAIL_FIND:
        {
            HKEY hkeyProtocol;
            DWORD dw;

                if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAILCLIENTS,
                           0, NULL, 0, KEY_READ, NULL, &hkeyProtocol, &dw) != ERROR_SUCCESS)
                    break;
            FindScript(pPrg->hwndMail, hkeyProtocol);
            RegCloseKey(hkeyProtocol);
        }
        break;

        case IDC_NEWS_FIND:
        {
            HKEY hkeyProtocol;
            DWORD dw;

            if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_NEWSCLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkeyProtocol, &dw) != ERROR_SUCCESS)
                break;
            FindScript(pPrg->hwndNews, hkeyProtocol);
            RegCloseKey(hkeyProtocol);
        }
        break;

        case IDC_VSOURCE_FIND:
        {
            HKEY hkeyProtocol;
            DWORD dw;

            if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_VSOURCECLIENTS,
                       0, NULL, 0, KEY_READ, NULL, &hkeyProtocol, &dw) != ERROR_SUCCESS)
                break;
            FindScript(pPrg->hwndVSource, hkeyProtocol);
            RegCloseKey(hkeyProtocol);
        }
        break;
    } // switch

} // ProgramsOnCommand()

#endif  /* !UNIX */

//
// ProgramsDlgProc()
//
// Handles window messages sent to the Programs Property Sheet.
//
// History:
//
// 6/20/96  t-gpease    created
//
INT_PTR CALLBACK ProgramsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam)
{
    LPPROGRAMSPAGE pPrg;

    if (uMsg == WM_INITDIALOG)
        return ProgramsDlgInit( hDlg );
    else
        pPrg= (LPPROGRAMSPAGE) GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pPrg)
        return FALSE;

    switch (uMsg) {

        case WM_COMMAND:
            ProgramsOnCommand(pPrg, LOWORD(wParam), HIWORD(wParam));
            return TRUE;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            ASSERT(lpnm);
            switch (lpnm->code)
            {
                case PSN_KILLACTIVE:
#ifdef UNIX
                    if (!(FoundProgram(pPrg->hDlg, IDC_EDIT_PROGRAMS_MAIL) &&
                          FoundProgram(pPrg->hDlg, IDC_EDIT_PROGRAMS_NEWS) &&
                          FoundProgram(pPrg->hDlg, IDC_EDIT_PROGRAMS_VSOURCE)))
                    {
                        MessageBox(pPrg->hDlg, TEXT("One (or more) program is not found or not executable."), NULL, MB_OK);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return TRUE;

#endif //UNIX
                case PSN_QUERYCANCEL:
                case PSN_RESET:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return TRUE;

                case PSN_APPLY:
                    //
                    // Save Programs Dlg Stuff.
                    //
                    ProgramsDlgApplyNow(pPrg);
                    break;
            }
        }
        break;

        case WM_HELP:                   // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
        {
#ifndef UNIX
            // Free the data stored the HTML editor combo
            int iMax = (int) SendMessage(pPrg->hwndHtmlEdit, CB_GETCOUNT, 0, 0);
            HKEY hkey;
            for (int i = 0; i < iMax; ++i)
            {
                hkey = (HKEY) SendMessage(pPrg->hwndHtmlEdit, CB_GETITEMDATA, i, 0);
                if (hkey && (INT_PTR)hkey != CB_ERR)
                {
                    RegCloseKey(hkey);
                }
            }

            if (pPrg)
            {
                if (pPrg->pfi)
                    pPrg->pfi->Release();
                LocalFree(pPrg);
            }
#endif
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG)NULL);
            break;
        }
    }
    return FALSE;
}

static const TCHAR c_szMailIcon[] = TEXT("Software\\Microsoft\\MailIcon");
static const TCHAR c_szMailIconGuid[] = TEXT("CLSID\\{dacf95b0-0a3f-11d1-9389-006097d503d9}");
static const TCHAR c_szKeyMail[] = TEXT("Software\\Clients\\Mail");
static const TCHAR c_szRegFmt2[] = TEXT("%s\\%s");
static const TCHAR c_szFormatClient[] = TEXT("FormatClient");
static const TCHAR c_szFormatNoClient[] = TEXT("FormatNoClient");
static       WCHAR c_wszMailIconGuid[] = L"::{dacf95b0-0a3f-11d1-9389-006097d503d9}";

void UpdateMailIconLabel()
{
    TCHAR szOldLabel[MAX_PATH];
    TCHAR szNewLabel[MAX_PATH];
    TCHAR szDefClient[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    DWORD cbSize;
    HKEY  hKey;

    *szNewLabel = 0;
    *szOldLabel = 0;

    // check if the mail icon is even installed
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szMailIcon, 0, KEY_READ, &hKey))
    {
        cbSize = sizeof(szOldLabel);
        // get the current mail icon label
        if (ERROR_SUCCESS == RegQueryValue(HKEY_CLASSES_ROOT, c_szMailIconGuid, szOldLabel, (PLONG)&cbSize))
        {
            cbSize = sizeof(szDefClient);
            // get the default client's reg key
            if (ERROR_SUCCESS == RegQueryValue(HKEY_LOCAL_MACHINE, c_szKeyMail, szDefClient, (PLONG)&cbSize) && cbSize)
            {
                wnsprintf(szTemp, ARRAYSIZE(szTemp), c_szRegFmt2, c_szKeyMail, szDefClient);
                cbSize = sizeof(szDefClient);
                // get the default client's display name
                if (ERROR_SUCCESS == RegQueryValue(HKEY_LOCAL_MACHINE, szTemp, szDefClient, (PLONG)&cbSize) && cbSize)
                {
                    cbSize = sizeof(szTemp);
                    // get the mail icon label format string
                    if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_szFormatClient, 0, NULL, (LPBYTE)szTemp, &cbSize))
                    {
                        wnsprintf(szNewLabel, ARRAYSIZE(szNewLabel), szTemp, szDefClient);
                    }
                }
            }
            else
            {
                cbSize = sizeof(szNewLabel);
                // get the mail icon label format string
                RegQueryValueEx(hKey, c_szFormatNoClient, 0, NULL, (LPBYTE)szNewLabel, &cbSize);
            }
        }
        // if the above succeeded, and the label is different
        if (*szNewLabel && StrCmp(szNewLabel, szOldLabel))
        {
             IShellFolder *psf;

            // set the new label
            RegSetValue(HKEY_CLASSES_ROOT, c_szMailIconGuid, REG_SZ, szNewLabel, (lstrlen(szNewLabel)+1)*sizeof(TCHAR));

            // let the shell know that it changed
            if (SUCCEEDED(SHGetDesktopFolder(&psf)))
            {
                LPITEMIDLIST pidl;
                ULONG        chEaten;

                if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, c_wszMailIconGuid, &chEaten, &pidl, NULL)))
                {
                    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidl, NULL);
                    SHFree(pidl);
                }
                psf->Release();
            }
        }
        RegCloseKey(hKey);
    }
}

