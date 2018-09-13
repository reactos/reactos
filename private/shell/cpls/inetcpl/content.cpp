//*********************************************************************
//*          Microsoft Windows                                       **
//*        Copyright(c) Microsoft Corp., 1995                        **
//*********************************************************************

//
// content.cpp - "Content" property sheet
//

// HISTORY:
//
// 5/17/97  t-ashlm created

#include "inetcplp.h"
#include <wab.h>
#include <cryptui.h>
#include <msiehost.h>

#include <mluisupp.h>

//
// Private Functions and Structures
//

// WINTRUST / SOFTPUB
// definition from WINTRUST.H
// extern "C" BOOL WINAPI OpenPersonalTrustDBDialog(HWND hwndParent);
typedef BOOL (WINAPI *WINTRUSTDLGPROC)(HWND hwndParent);
WINTRUSTDLGPROC g_WinTrustDlgProc = (WINTRUSTDLGPROC)NULL;
#ifdef WALLET
BOOL IsWallet3Installed();
BOOL IsWalletAddressAvailable(VOID);
BOOL IsWalletPaymentAvailable(VOID);
#endif
HRESULT ShowModalDialog(HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, TCHAR* pchOptions, VARIANT *pvArgOut);
HCERTSTORE PFXImportCertStore(CRYPT_DATA_BLOB* pPFX, LPCWSTR szPassword, DWORD   dwFlags);
BOOL PFXExportCertStore(HCERTSTORE hStore, CRYPT_DATA_BLOB* pPFX, LPCWSTR szPassword, DWORD   dwFlags);
BOOL _AorW_GetFileNameFromBrowse(HWND hDlg,
                                 LPWSTR pszFilename,
                                 UINT cchFilename,
                                 LPCWSTR pszWorkingDir,
                                 LPCWSTR pszExt,
                                 LPCWSTR pszFilter,
                                 LPCWSTR pszTitle);

INT_PTR CALLBACK AutoSuggestDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WalletDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


//BUBUG: The following prototype should be rermoved when we have updated our Crypto API to latest version
BOOL WINAPI WTHelperIsInRootStore(PCCERT_CONTEXT pCertContext);

//////////////////////////////////////////////
// stolen from \inet\schannel\sspi\spreg.h
#define REG_SITECERT_BASE     TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\CertificationAuthorities")
#define REG_SITECERT_CERT_VAL TEXT("CACert")

#define SITECERTKEYLEN        80    // BUGBUG: should probably grab this value somewhere
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#include <initguid.h>

// Use the wallet "payment" guid for JIT (different for alpha and x86...)
#ifdef _ALPHA_
// {B7FB4D5C-9FBE-11D0-8965-0000F822DEA9}
DEFINE_GUID(CLSID_WalletPayment, 0xb7fb4d5c, 0x9fbe, 0x11d0, 0x89, 0x65, 0x0, 0x0, 0xf8, 0x22, 0xde, 0xa9);
#else
// {87D3CB66-BA2E-11CF-B9D6-00A0C9083362}
DEFINE_GUID(CLSID_WalletPayment, 0x87d3cb66, 0xba2e, 0x11cf, 0xb9, 0xd6, 0x0, 0xa0, 0xc9, 0x08, 0x33, 0x62);
#endif

// WAB GUID for JIT
DEFINE_GUID(CLSID_WAB, 0x32714800, 0x2E5F, 0x11d0, 0x8B, 0x85, 0x00, 0xAA, 0x00, 0x44, 0xF9, 0x41);

#define EKU_CODESIGN_OFF    0
#define EKU_EMAIL_OFF       1
#define EKU_CLIENT_OFF      2
#define EKU_SERVER_OFF      3
#define EKU_DISABLE_OFF     4


const LPSTR g_rgszEnhkeyUsage[] =
{
    szOID_PKIX_KP_CODE_SIGNING,
    szOID_PKIX_KP_EMAIL_PROTECTION,
    szOID_PKIX_KP_CLIENT_AUTH,
    szOID_PKIX_KP_SERVER_AUTH,
    szOID_YESNO_TRUST_ATTR,
    NULL
};


typedef struct {
    HWND hDlg;              // handle to window
    HRESULT hrUseRatings;   // error=not installed; S_OK=enabled; S_FALSE=disabled
    HINSTANCE   hWinTrust;      // WINTRUST/SOFTPUB library handle

} CONTENTPAGE, *LPCONTENTPAGE;

BOOL ContentDlgApplyNow(  LPCONTENTPAGE pCon     );
BOOL ContentDlgEnableControls(   IN HWND hDlg  );
BOOL ContentDlgInit(  IN HWND hDlg  );

VOID DisplayWalletPaymentDialog(HWND hWnd);
VOID DisplayWalletAddressDialog(HWND hWnd);

HRESULT ResetProfileSharing(HWND hwnd);
EXTERN_C HRESULT ClearAutoSuggestForForms(DWORD dwClear);


//
// SecurityDlgEnableControls()
//
// Does initalization for Security Dlg.
//
// History:
//
// 6/17/96  t-gpease   moved
//
BOOL ContentDlgEnableControls( IN HWND hDlg  )
{
    HKEY hkey=NULL;
    
    if( g_restrict.fRatings )
    {
        EnableWindow( GetDlgItem(hDlg, IDC_RATINGS_TURN_ON), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_RATINGS_BUTTON), FALSE );
#if 0   // don't diable the text
        EnableDlgItem( hDlg, IDC_RATINGS_TEXT, FALSE);
        EnableDlgItem( hDlg, IDC_ADVANCED_RATINGS_GROUPBOX, FALSE);
#endif
    }

    if( g_restrict.fCertif || g_restrict.fCertifPub)
        EnableWindow( GetDlgItem(hDlg, IDC_SECURITY_PUBLISHERS_BUTTON), FALSE );

    if( g_restrict.fCertif || g_restrict.fCertifPers || g_restrict.fCertifSite)
        EnableWindow( GetDlgItem(hDlg, IDC_SECURITY_SITES_BUTTON), FALSE );
        
    if( g_restrict.fProfiles )
    {
        EnableWindow(GetDlgItem(hDlg,  IDC_EDIT_PROFILE), FALSE);
    }

    if (hkey)
        RegCloseKey(hkey);
    
#ifdef WALLET
    if (g_restrict.fWallet)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PROGRAMS_WALLET_SETTINGS), FALSE);
    }
#endif

    return TRUE;
}


void InitRatingsButton(HWND hDlg, HRESULT hrEnabled)
{
    TCHAR szBuf[MAX_RES_LEN+1];

    UINT idString;
    BOOL fEnableSettingsButton;

    if (FAILED(hrEnabled)) {
        /* Ratings are not installed.  Disable the Settings button and
         * set the other button to say "Enable".
         */
        idString = IDS_RATINGS_TURN_ON;
        fEnableSettingsButton = FALSE;
    }
    else {
        idString = (hrEnabled == S_OK) ? IDS_RATINGS_TURN_OFF : IDS_RATINGS_TURN_ON;
        fEnableSettingsButton = TRUE;
    }
    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_RATINGS_BUTTON), fEnableSettingsButton);

    if (MLLoadString(
                   idString,
                   szBuf, sizeof(szBuf)) > 0) {
        SetDlgItemText(hDlg, IDC_RATINGS_TURN_ON, szBuf);
    }

}

//
// ContentDlgInit()
//
// Does initalization for Content Dlg.
//
//
BOOL ContentDlgInit( HWND hDlg)
{
    LPCONTENTPAGE pCon;

    pCon = (LPCONTENTPAGE)LocalAlloc(LPTR, sizeof(*pCon));
    if (!pCon)
    {
        EndDialog(hDlg, 0);
        return FALSE;   // no memory?
    }

    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pCon);

    // save the handle to the page
    pCon->hDlg = hDlg;

    // Load the Ratings DLL (if possible)
    g_hinstRatings = LoadLibrary(c_tszRatingsDLL);

    // if not..
    if (!g_hinstRatings)
        g_restrict.fRatings = TRUE;     // disable Ratings section


    // set ratings dialog items... 

    // if MSRATING.DLL not around, then don't do this call.  By not
    // doing this, it will keep the "Enable Ratings" text on the button 
    // but greyed off.
    if (g_hinstRatings)
        pCon->hrUseRatings = RatingEnabledQuery();

    InitRatingsButton(hDlg, pCon->hrUseRatings);


    // if we can't find WINTRUST or SOFTPUB disable the
    // "Publishers" button.
    pCon->hWinTrust = LoadLibrary(TEXT("wintrust.dll"));

    if ( pCon->hWinTrust )
    {
        g_WinTrustDlgProc =
                           (WINTRUSTDLGPROC) GetProcAddress(pCon->hWinTrust, "OpenPersonalTrustDBDialog");

        // didn't find the procecdure
        if (!g_WinTrustDlgProc)
        {
            // release library and try the other DLL.
            FreeLibrary(pCon->hWinTrust);

            //
            // We can also find the same function on NT machines (and
            // possibly future Win95s) in SOFTPUB.DLL so make another
            // check there too.
            //
            pCon->hWinTrust = LoadLibrary(TEXT("softpub.dll"));
        }
    }

    if (pCon->hWinTrust && !g_WinTrustDlgProc)
        g_WinTrustDlgProc = (WINTRUSTDLGPROC) 
                            GetProcAddress(pCon->hWinTrust, "OpenPersonalTrustDBDialog");

    // if after all this, we can't find the procedure...
    if (!g_WinTrustDlgProc)
    {
        // disable the button
        EnableDlgItem(hDlg, IDC_SECURITY_PUBLISHERS_BUTTON, FALSE);
    }

#ifdef WALLET
    EnableDlgItem(hDlg, IDC_PROGRAMS_WALLET_SETTINGS, TRUE);    
#endif

    ContentDlgEnableControls(hDlg);

    return TRUE;
}



//
// ContentOnCommand()
//
// Handles Content Dialog's window messages
//
// History:
//
// 6/17/96  t-gpease   created
//
void ContentOnCommand(LPCONTENTPAGE pCon, UINT id, UINT nCmd)
{
    switch (id) {

        case IDC_ADVANCED_RATINGS_BUTTON:
        {
            RatingSetupUI(pCon->hDlg, (LPCSTR) NULL);        
        }
        break; // IDC_ADVANCED_RATINGS_BUTTON

        case IDC_RATINGS_TURN_ON:
        {
            if (SUCCEEDED(RatingEnable(pCon->hDlg, (LPCSTR)NULL,
                                       pCon->hrUseRatings != S_OK))) 
            {
                pCon->hrUseRatings = RatingEnabledQuery();
                InitRatingsButton(pCon->hDlg, pCon->hrUseRatings);
            }
        }
        break;

        case IDC_SECURITY_SITES_BUTTON:
        {
            CRYPTUI_CERT_MGR_STRUCT ccm = {0};
            ccm.dwSize = sizeof(ccm);
            ccm.hwndParent = pCon->hDlg;
            CryptUIDlgCertMgr(&ccm);
//          if (!g_hinstCryptui)
//          {
//              EnableWindow(GetDlgItem(pCon->hDlg, IDC_SECURITY_SITES_BUTTON), FALSE);
//          }
        }
            break;

        case IDC_SECURITY_PUBLISHERS_BUTTON:
        {
            if (g_WinTrustDlgProc)
            {
                g_WinTrustDlgProc(pCon->hDlg);
            }
        }
        break;

#ifdef WALLET
        case IDC_PROGRAMS_WALLET_SETTINGS:
        {
            HRESULT hr = S_OK;

            // See if wallet is installed at all
            if (!IsWalletPaymentAvailable())
            {
                uCLSSPEC clsspec;

                clsspec.tyspec = TYSPEC_CLSID;
                clsspec.tagged_union.clsid = CLSID_WalletPayment;

                // If wallet isn't installed, ask user if they'd like to install it
                hr = FaultInIEFeature(NULL, &clsspec, NULL, FIEF_FLAG_FORCE_JITUI);
            }

            if (SUCCEEDED(hr))
            {
                // Wallet is installed
                if (IsWallet3Installed())
                {
                    // if wallet 3.0 is installed, we want to invoke the wallet UI directly
                    DisplayWalletPaymentDialog(pCon->hDlg);
                }
                else
                {
                    // otherwise we need to pop up this intermediate dialog
                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_WALLET_SETTINGS), pCon->hDlg, WalletDlgProc);
                }
            }
        }
        break;
#endif

        case IDC_AUTOSUGGEST_SETTINGS:
        {
            DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_AUTOSUGGEST_SETTINGS), pCon->hDlg, AutoSuggestDlgProc);
        }
        break;

        case IDC_EDIT_PROFILE:
        {
            HMODULE hInstWAB = NULL;
            LPWABOBJECT  lpWABObject = NULL;
            LPADRBOOK lpAdrBook = NULL;
            HRESULT hr=S_OK;

            // Ask user to JIT in WAB if it's not installed
            uCLSSPEC clsspec;

            clsspec.tyspec = TYSPEC_CLSID;
            clsspec.tagged_union.clsid = CLSID_WAB;

            // If WAB isn't installed, ask user if they'd like to install it
            hr = FaultInIEFeature(NULL, &clsspec, NULL, FIEF_FLAG_FORCE_JITUI);

            if (FAILED(hr))
            {
                break;
            }
                
            // Figure out the location of the wab dll and try opening it.
            TCHAR szWABDllPath[MAX_PATH];
            DWORD dwType = 0;
            ULONG cbData = sizeof(szWABDllPath) * sizeof(TCHAR);
            HKEY hKey = NULL;
            SBinary SBMe = { 0, 0};

            *szWABDllPath = '\0';
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
            {
                RegQueryValueEx( hKey, TEXT(""), NULL, &dwType, (LPBYTE) szWABDllPath, &cbData);
                RegCloseKey(hKey);
            }

            if (lstrlen(szWABDllPath) > 0 )
            {
                hInstWAB = LoadLibrary(szWABDllPath);
            }

            if (hInstWAB)
            {
                LPWABOPEN lpfnWABOpen = (LPWABOPEN) GetProcAddress(hInstWAB, "WABOpen");

                if (lpfnWABOpen)
                {
                    hr = lpfnWABOpen(&lpAdrBook, &lpWABObject, NULL, 0);

                    if (NULL == lpAdrBook || NULL == lpWABObject)
                        hr = E_UNEXPECTED;
                }
                else 
                {
                    hr = HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);  // Not the right dll anyway!!
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
            }

            DWORD dwAction = 0;

            // Good so far, call GetMe. WAB may create a new entry in this call.
            if (SUCCEEDED(hr))
            {
                hr = lpWABObject->GetMe(lpAdrBook, 0, &dwAction, &SBMe, 0);

                if (0 == SBMe.cb || NULL == SBMe.lpb)
                    hr = E_UNEXPECTED;
            }     

            // This shows the final UI. If WAB created a new entry in GetMe, they
            //   already showed this UI and we don't need to do it again.
            if (SUCCEEDED(hr) && !(dwAction & WABOBJECT_ME_NEW))
            {
                hr = lpAdrBook->Details(  (LPULONG) &pCon->hDlg,
                                          NULL,
                                          NULL,
                                          SBMe.cb,
                                          (LPENTRYID)SBMe.lpb,
                                          NULL,
                                          NULL,
                                          NULL,
                                          0);

            }
            if (lpWABObject)
            {
                if (SBMe.lpb != NULL)
                    lpWABObject->FreeBuffer(SBMe.lpb);

                lpWABObject->Release();
            }

            if (lpAdrBook)
                lpAdrBook->Release();

            if (hInstWAB)
                FreeLibrary(hInstWAB);
        }        
    }

} // ContentOnCommand()


/****************************************************************
Name: ContentDlgProc

SYNOPSIS: Set various security issue settings.

****************************************************************/

INT_PTR CALLBACK ContentDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPCONTENTPAGE pCon;

    if (uMsg == WM_INITDIALOG)
        return ContentDlgInit( hDlg );
    else
        pCon = (LPCONTENTPAGE) GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pCon)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_COMMAND:
            ContentOnCommand(pCon, LOWORD(wParam), HIWORD(wParam));
            return TRUE;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            ASSERT(lpnm);
            switch (lpnm->code) {
                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    SetWindowLongPtr( pCon->hDlg, DWLP_MSGRESULT, FALSE );
                    return TRUE;

                case PSN_APPLY:
                    break;
            }
            break;
        }

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            ASSERT(pCon);
            if (pCon)
            {
                if (pCon->hWinTrust)
                {
                    FreeLibrary(pCon->hWinTrust);
                    g_WinTrustDlgProc = NULL;
                }
                LocalFree(pCon);
            }
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            break;
    }
    return FALSE;
}


typedef struct tagSITECERTDIALOGINFO {    
    HWND hDlg;
    HWND hwndList;
    HWND hwndCombo;
    int  iSel;
    HCERTSTORE hCertStore;
    BOOL    fInitializing;
} SITECERTDIALOGINFO, *LPSITECERTDIALOGINFO;

BOOL _SearchKeyUsage(CERT_ENHKEY_USAGE *pUsage, LPSTR pszUsageIdentifier)
{
    DWORD   i;

    for (i = 0; i < pUsage->cUsageIdentifier; i++)
    {
        if (StrCmpA(pUsage->rgpszUsageIdentifier[i], pszUsageIdentifier) == 0)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

BOOL _IsKnownUsage(char *pszTest)
{
    char    **ppszKnown;

    ppszKnown = (char **)g_rgszEnhkeyUsage;

    while (*ppszKnown)
    {
        if (StrCmpA(*ppszKnown, pszTest) == 0)
        {
            return(TRUE);
        }

        ppszKnown++;
    }

    return(FALSE);
}

void __AddAllKnownEKU(PCCERT_CONTEXT pCert)
{
    char    **ppszKnown;

    ppszKnown = (char **)g_rgszEnhkeyUsage;

    while (*ppszKnown)
    {
       CertAddEnhancedKeyUsageIdentifier(pCert, *ppszKnown);

       ppszKnown++;
    }
}

BOOL _AnyKnownUsage(CERT_ENHKEY_USAGE *pUsage)
{
    DWORD   i;

    for (i = 0; i < pUsage->cUsageIdentifier; i++)
    {
        if (_IsKnownUsage(pUsage->rgpszUsageIdentifier[i]))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

BOOL _IsUsageEnabled(PCCERT_CONTEXT pCertContext, LPSTR pszUsageIdentifier, BOOL * pfFound)
{
    CERT_ENHKEY_USAGE   *pUsage;
    DWORD               cbUsage;


    *pfFound = FALSE;

    //
    // first, check the Extensions to see if we should even display it!
    //
    cbUsage = 0;
    CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, NULL, &cbUsage);

    if (cbUsage > 0)
    {
        //
        //  we have some... make sure ours is in the list
        //
        if (!(pUsage = (CERT_ENHKEY_USAGE *)LocalAlloc(LPTR, cbUsage)))
        {
            return(FALSE);
        }

        CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, pUsage, &cbUsage);

        if (!(_SearchKeyUsage(pUsage, pszUsageIdentifier)))
        {
            LocalFree((void *)pUsage);
            return(FALSE);
        }

        LocalFree((void *)pUsage);
    }

    *pfFound = TRUE;    // the cert should go in the list!

    //
    //  ethier there where no assertions made by the CA or we found it!  continue on...
    //

    //
    //  second, check the properties to see if we should check the box
    //
    cbUsage = 0;
    CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, NULL, &cbUsage);

    if (cbUsage > 0)
    {
        // 
        //  we have properties... make sure we aren't disabled
        //
        if (!(pUsage = (CERT_ENHKEY_USAGE *)LocalAlloc(LPTR, cbUsage)))
        {
            return(FALSE);
        }

        CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, pUsage, &cbUsage);

        if (_SearchKeyUsage(pUsage, g_rgszEnhkeyUsage[EKU_DISABLE_OFF]))
        {
            //
            //  the user has disabled the cert... keep it in the list un-checked
            //
            LocalFree((void *)pUsage);

            return(FALSE);
        }

        if (!(_SearchKeyUsage(pUsage, pszUsageIdentifier)))
        {
            //
            //  the user has set some, but, disabled this one... keep in the list un-checked
            //
            LocalFree((void *)pUsage);

            return(FALSE);
        }

        LocalFree((void *)pUsage);
    }

    return(TRUE);
}


BOOL SiteCert_InitListView(LPSITECERTDIALOGINFO pscdi)
{
    PCCERT_CONTEXT  pCertContext = NULL;
    
    // delete all items currently in the listview
    // we'll get called back via LVN_DELETEITEM with the lParam so we can free the cert context
    ListView_DeleteAllItems(pscdi->hwndList);
    
    pscdi->hCertStore = CertOpenSystemStoreA(NULL, "ROOT");
    
    if (pscdi->hCertStore)
    {
        LPSTR pszEnhkeyUsage;
        
        INT_PTR iSel;
        
        iSel = SendMessage(pscdi->hwndCombo, CB_GETCURSEL, 0,0);
        
        pszEnhkeyUsage = (LPSTR)SendMessage(pscdi->hwndCombo, CB_GETITEMDATA, iSel, 0);
       
        while (pCertContext = CertEnumCertificatesInStore(pscdi->hCertStore, pCertContext))
        {
            CHAR  szCertA[MAX_PATH];
            TCHAR szCert[MAX_PATH];
            DWORD cbszCert = ARRAYSIZE(szCertA);
            DWORD dwEnabled;
            BOOL fFound;

            dwEnabled = _IsUsageEnabled(pCertContext, (LPSTR)pszEnhkeyUsage, &fFound);     

            // if not found, then continue with next
            if (!fFound)
                continue;
            
            //ParseX509EncodedCertificateForListBoxEntry(pCertContext->pbCertEncoded, pCertContext->cbCertEncoded, szCert, &cbszCert);
            ParseX509EncodedCertificateForListBoxEntry((BYTE *)pCertContext, -1, szCertA, &cbszCert);
#ifdef UNICODE
            SHAnsiToUnicode(szCertA, szCert, ARRAYSIZE(szCert));
#else
            StrCpy(szCert, szCertA);
#endif
            LV_ITEM lvi = { 0 };

            lvi.mask       = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            lvi.iItem      = -1;
            lvi.pszText    = szCert; // (LPSTR)pCertContext->pCertInfo->Subject.pbData;
            lvi.cchTextMax = ARRAYSIZE(szCert); // pCertContext->pCertInfo->Subject.cbData;

            lvi.stateMask  = LVIS_STATEIMAGEMASK;
            lvi.state      = dwEnabled ? 0x00002000 : 0x00001000;
            lvi.lParam     = (LPARAM)CertDuplicateCertificateContext(pCertContext);
            
            // insert and set state
            ListView_SetItemState(pscdi->hwndList,
                                  ListView_InsertItem(pscdi->hwndList, &lvi),
                                  dwEnabled ? 0x00002000 : 0x00001000,
                                  LVIS_STATEIMAGEMASK);
            
        }
        // show the items
        ListView_RedrawItems(pscdi->hwndList, 0, ListView_GetItemCount(pscdi->hwndList));
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
////
////    08-Sep-1997: pberkman
////
////    PRIVATE function: _SiteCertAdjustProperties
////
////        based on what the user just checked/unchecked, set the 
////        appropriate OID usage or remove it.
////        
void _SiteCertAdjustProperties(LPSITECERTDIALOGINFO pscdi, NM_LISTVIEW *pListView)
{
    DWORD_PTR           dwSel;
    char                *pszOID;
    DWORD               cbUsage;
    CERT_ENHKEY_USAGE   *pUsage;


    //
    //  if we are in the initdialog get out!
    //
    if (pscdi->fInitializing)
    {
        return;
    }

    // 
    // make sure we have the property set
    //
    dwSel = SendMessage(pscdi->hwndCombo, CB_GETCURSEL, 0, 0);

    if (dwSel == CB_ERR)
    {
        return;
    }

    pszOID = (char*) SendMessage(pscdi->hwndCombo, CB_GETITEMDATA, (WPARAM)dwSel, 0);

    if (!(pszOID) || ((DWORD_PTR)pszOID == CB_ERR))
    {
        return;
    }

    if (pListView->uNewState & 0x00001000)  // unchecked
    {

        //
        //  the user unchecked one of the certs.
        //  
        //  1. if there are no properties, add all others -- HACKHACK!
        //
        cbUsage = 0;
        CertGetEnhancedKeyUsage((PCCERT_CONTEXT)pListView->lParam, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, 
                                NULL, &cbUsage);

        if (cbUsage == 0)
        {
            //  add all
            __AddAllKnownEKU((PCCERT_CONTEXT)pListView->lParam);

            //  remove this one
            CertRemoveEnhancedKeyUsageIdentifier((PCCERT_CONTEXT)pListView->lParam, pszOID);
        }
        else
        {
            if (!(pUsage = (CERT_ENHKEY_USAGE *)LocalAlloc(LPTR, cbUsage)))
            {
                return;
            }

            CertGetEnhancedKeyUsage((PCCERT_CONTEXT)pListView->lParam, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, 
                                        pUsage, &cbUsage);
            //
            //  2. if there are properties.
            //      a. if this is the last known one, and it matches this, delete it and add the "disable"
            //
            if (pUsage->cUsageIdentifier == 1)
            {
                if (StrCmpA(pUsage->rgpszUsageIdentifier[0], pszOID) ==  0)
                {
                    CertRemoveEnhancedKeyUsageIdentifier((PCCERT_CONTEXT)pListView->lParam, pszOID);
                    CertAddEnhancedKeyUsageIdentifier((PCCERT_CONTEXT)pListView->lParam, 
                                                        g_rgszEnhkeyUsage[EKU_DISABLE_OFF]);
                }
            }
            else
            {
                //
                //  b. if there are more than one, just try to remove this one
                //
                CertRemoveEnhancedKeyUsageIdentifier((PCCERT_CONTEXT)pListView->lParam, pszOID);
            }

            LocalFree((void *)pUsage);
        }

        return;
    }

    if (pListView->uNewState & 0x00002000)  // checked
    {
        CertAddEnhancedKeyUsageIdentifier((PCCERT_CONTEXT)pListView->lParam, pszOID);

        //
        //  just in case, remove the disable!
        //
        CertRemoveEnhancedKeyUsageIdentifier((PCCERT_CONTEXT)pListView->lParam, 
                                                    g_rgszEnhkeyUsage[EKU_DISABLE_OFF]);
    }
}

BOOL SiteCert_OnNotify(LPSITECERTDIALOGINFO pscdi, WPARAM wParam, LPARAM lParam)
{
    NM_LISTVIEW *pnmlv = (NM_LISTVIEW *)lParam;
    
    switch (pnmlv->hdr.code) {
        case LVN_ITEMCHANGED:
        {
            // check the current state of selection
            int iSel = ListView_GetNextItem(pscdi->hwndList, -1, LVNI_SELECTED);

            // check to see if we need to enable/disable the "DELETE" and "VIEW" buttons
            EnableWindow(GetDlgItem(pscdi->hDlg, IDC_DELETECERT), iSel != -1);
            EnableWindow(GetDlgItem(pscdi->hDlg, IDC_VIEWCERT), iSel != -1);
            
            if ((pnmlv->uChanged & LVIF_STATE) && (GetFocus() == pscdi->hwndList))
            {
                _SiteCertAdjustProperties(pscdi, pnmlv);
            }
            break;
        }
        case LVN_DELETEITEM:
            CertFreeCertificateContext((PCCERT_CONTEXT)pnmlv->lParam);
            break;
    }
    return TRUE;
}

typedef struct tagNEWSITECERTINFO
{
    LPVOID  lpvCertData;
    DWORD   cbCert;

    BOOL    fCertEnabled;
    BOOL    fNetworkClient;
    BOOL    fNetworkServer;
    BOOL    fSecureEmail;
    BOOL    fSoftwarePublishing; 

} NEWSITECERTINFO, *LPNEWSITECERTINFO;


BOOL NewSiteCert_AddCert(LPNEWSITECERTINFO pnsci)
{

    HCERTSTORE      hCertStore = NULL;
    PCCERT_CONTEXT  pCertContext;
    BOOL            fRet = FALSE;

    hCertStore = CertOpenSystemStoreA(NULL, "ROOT");

    if (hCertStore)
    {
        pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                    (LPBYTE)(pnsci->lpvCertData),
                                                    pnsci->cbCert);

        if (pCertContext)
        {
            if (CertCompareCertificateName(X509_ASN_ENCODING,
                                           &pCertContext->pCertInfo->Subject,
                                           &pCertContext->pCertInfo->Issuer))
            {

                CertFreeCertificateContext(pCertContext);
                
                fRet = CertAddEncodedCertificateToStore(hCertStore,
                                                        X509_ASN_ENCODING,
                                                        (LPBYTE)(pnsci->lpvCertData),
                                                        pnsci->cbCert,
                                                        CERT_STORE_ADD_REPLACE_EXISTING,
                                                        &pCertContext);
                if (fRet)
                {
#                   define l_USAGE_MAX      24

                    CERT_ENHKEY_USAGE ceku = {0};
                    LPSTR rgpszUsageIdentifier[l_USAGE_MAX];
                    
                    if (pnsci->fNetworkClient)
                    {
                        rgpszUsageIdentifier[ceku.cUsageIdentifier] = g_rgszEnhkeyUsage[EKU_CLIENT_OFF];
                        if (rgpszUsageIdentifier[ceku.cUsageIdentifier])
                            ceku.cUsageIdentifier++;
                    }
                    if (pnsci->fNetworkServer)
                    {
                        rgpszUsageIdentifier[ceku.cUsageIdentifier] = g_rgszEnhkeyUsage[EKU_SERVER_OFF];
                        if (rgpszUsageIdentifier[ceku.cUsageIdentifier])
                            ceku.cUsageIdentifier++;
                    }
                    if (pnsci->fSecureEmail)
                    {
                        rgpszUsageIdentifier[ceku.cUsageIdentifier] = g_rgszEnhkeyUsage[EKU_EMAIL_OFF];
                        if (rgpszUsageIdentifier[ceku.cUsageIdentifier])
                            ceku.cUsageIdentifier++;
                    }
                    if (pnsci->fSoftwarePublishing)
                    {
                        rgpszUsageIdentifier[ceku.cUsageIdentifier] = g_rgszEnhkeyUsage[EKU_CODESIGN_OFF];
                        if (rgpszUsageIdentifier[ceku.cUsageIdentifier])
                            ceku.cUsageIdentifier++;
                    }

                    if (!(pnsci->fCertEnabled))
                    {
                        // turn everything off!!!
                        rgpszUsageIdentifier[ceku.cUsageIdentifier] = g_rgszEnhkeyUsage[EKU_DISABLE_OFF];
                        if (rgpszUsageIdentifier[ceku.cUsageIdentifier])
                            ceku.cUsageIdentifier++;
                    }

                    //
                    //  now, add any "unknown" extensions that the CA may have put on just
                    //  so verification will succeed!
                    //
                    CERT_ENHKEY_USAGE   *pUsage;
                    DWORD               cbUsage;
                    DWORD               i;

                    pUsage  = NULL;
                    cbUsage = 0;
                    CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, NULL, &cbUsage);

                    if (cbUsage > 0)
                    {
                        if (pUsage = (PCERT_ENHKEY_USAGE)LocalAlloc(LMEM_FIXED, cbUsage))
                        {
                            CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, 
                                                    pUsage, &cbUsage);

                            for (i = 0; i < pUsage->cUsageIdentifier; i++)
                            {
                                if (ceku.cUsageIdentifier >= l_USAGE_MAX)
                                {
                                    break;
                                }

                                if (pUsage->rgpszUsageIdentifier[i])
                                {
                                    if (!(_IsKnownUsage(pUsage->rgpszUsageIdentifier[i])))
                                    {
                                        rgpszUsageIdentifier[ceku.cUsageIdentifier] = pUsage->rgpszUsageIdentifier[i];
                                        ceku.cUsageIdentifier++;
                                    }
                                }
                            }
                        }
                    }

                    ceku.rgpszUsageIdentifier = (LPSTR *)rgpszUsageIdentifier;
                    fRet = CertSetEnhancedKeyUsage(pCertContext, &ceku);
                    
                    if (pUsage)
                    {
                        LocalFree((void *)pUsage);
                    }

                    CertFreeCertificateContext(pCertContext);
                }
            }
        }
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return fRet;
}

//////////////////////////////////////////////////////////////////////////
////
////    15-Aug-1997: pberkman
////
////    PRIVATE function: NewSiteCert_SetAvailableAuthorityCheckboxes
////
////        set the check boxes in the "New Site Certificate" dialog box
////        based on the Authority Extensions and Properties.
////
////        if there are no Authority Ext or Prop's, then the certificate
////        has the potential for the user to enable for all.  Otherwise,
////        the user can ONLY select the ones that the issuer (or MS) has
////        entrusted the certificate for.
////        
typedef struct l_CERTUSAGES_
{
    char        *pszOID;
    DWORD       dwControlId;
    BOOL        fEnabled;

} l_CERTUSAGES;

BOOL NewSiteCert_SetAvailableAuthorityCheckboxes(HWND hDlg, LPNEWSITECERTINFO pnsci,
                                                 BOOL fInitialize)
{
    l_CERTUSAGES    asUsages[] =
    {
        szOID_PKIX_KP_CLIENT_AUTH,      IDC_CHECK_NETWORK_CLIENT,       FALSE,
        szOID_PKIX_KP_SERVER_AUTH,      IDC_CHECK_NETWORK_SERVER,       FALSE,
        szOID_PKIX_KP_EMAIL_PROTECTION, IDC_CHECK_SECURE_EMAIL,         FALSE,
        szOID_PKIX_KP_CODE_SIGNING,     IDC_CHECK_SOFTWARE_PUBLISHING,  FALSE,
        NULL, 0, FALSE
    };

    l_CERTUSAGES        *psUsages;
    PCCERT_CONTEXT      pCertContext;
    DWORD               cbUsage;
    PCERT_ENHKEY_USAGE  pUsage;
    
    if (fInitialize)
    {
        CheckDlgButton(hDlg, IDC_CHECK_ENABLE_CERT,         BST_CHECKED);
        
        CheckDlgButton(hDlg, IDC_CHECK_NETWORK_CLIENT,      BST_CHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_NETWORK_SERVER,      BST_CHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SECURE_EMAIL,        BST_CHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SOFTWARE_PUBLISHING, BST_CHECKED);
    }

    pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                (LPBYTE)(pnsci->lpvCertData),
                                                 pnsci->cbCert);

    if (!(pCertContext))
    {
        psUsages = &asUsages[0];

        while (psUsages->pszOID)
        {
            EnableWindow(GetDlgItem(hDlg, psUsages->dwControlId), TRUE);

            psUsages++;
        }

        return(FALSE);
    }

    cbUsage = 0;

    CertGetEnhancedKeyUsage(pCertContext, 0, NULL, &cbUsage);

    if (cbUsage < 1)
    {
        // none defined... leave all enabled.
        CertFreeCertificateContext(pCertContext);
        psUsages = &asUsages[0];

        while (psUsages->pszOID)
        {
            EnableWindow(GetDlgItem(hDlg, psUsages->dwControlId), TRUE);

            psUsages++;
        }

        return(TRUE);
    }

    if (!(pUsage = (PCERT_ENHKEY_USAGE)LocalAlloc(LMEM_FIXED, cbUsage)))
    {
        CertFreeCertificateContext(pCertContext);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!(CertGetEnhancedKeyUsage(pCertContext, 0, pUsage, &cbUsage)))
    {
        CertFreeCertificateContext(pCertContext);
        LocalFree(pUsage);
        return(FALSE);
    }

    if (pUsage->cUsageIdentifier == 0)
    {
        CertFreeCertificateContext(pCertContext);
        LocalFree(pUsage);
        // none defined... leave all enabled.
        return(TRUE);
    }

    CertFreeCertificateContext(pCertContext);

    for (int i = 0; i < (int)pUsage->cUsageIdentifier; i++)
    {
        psUsages = &asUsages[0];

        while (psUsages->pszOID)
        {
            if (StrCmpA(pUsage->rgpszUsageIdentifier[i], psUsages->pszOID) == 0)
            {
                psUsages->fEnabled = TRUE;
            }
            psUsages++;
        }
    }

    LocalFree(pUsage);

    psUsages = &asUsages[0];

    while (psUsages->pszOID)
    {
        if (fInitialize)
        {
            CheckDlgButton(hDlg, psUsages->dwControlId,      
                           (psUsages->fEnabled) ? BST_CHECKED : BST_UNCHECKED);
        }

        EnableWindow(GetDlgItem(hDlg, psUsages->dwControlId), psUsages->fEnabled);

        psUsages++;
    }

    return(TRUE);
}

void NewSiteCert_CenterDialog(HWND hDlg)
{
    RECT    rcDlg;
    RECT    rcArea;
    RECT    rcCenter;
    HWND    hWndParent;
    HWND    hWndCenter;
    DWORD   dwStyle;
    int     w_Dlg;
    int     h_Dlg;
    int     xLeft;
    int     yTop;

    GetWindowRect(hDlg, &rcDlg);

    dwStyle = (DWORD)GetWindowLong(hDlg, GWL_STYLE);

    if (dwStyle & WS_CHILD)
    {
        hWndCenter = GetParent(hDlg);

        hWndParent = GetParent(hDlg);

        GetClientRect(hWndParent, &rcArea);
        GetClientRect(hWndCenter, &rcCenter);
        MapWindowPoints(hWndCenter, hWndParent, (POINT *)&rcCenter, 2);
    }
    else
    {
        hWndCenter = GetWindow(hDlg, GW_OWNER);

        if (hWndCenter)
        {
            dwStyle = (DWORD)GetWindowLong(hWndCenter, GWL_STYLE);

            if (!(dwStyle & WS_VISIBLE) || (dwStyle & WS_MINIMIZE))
            {
                hWndCenter = NULL;
            }
        }

        SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);

        if (hWndCenter)
        {
            GetWindowRect(hWndCenter, &rcCenter);
        }
        else
        {
            rcCenter = rcArea;
        }
        
    }

    w_Dlg   = rcDlg.right - rcDlg.left;
    h_Dlg   = rcDlg.bottom - rcDlg.top;

    xLeft   = (rcCenter.left + rcCenter.right) / 2 - w_Dlg / 2;
    yTop    = (rcCenter.top + rcCenter.bottom) / 2 - h_Dlg / 2;
    
    if (xLeft < rcArea.left)
    {
        xLeft = rcArea.left;
    }
    else if ((xLeft + w_Dlg) > rcArea.right)
    {
        xLeft = rcArea.right - w_Dlg;
    }

    if (yTop < rcArea.top)
    {
        yTop = rcArea.top;
    }
    else if ((yTop + h_Dlg) > rcArea.bottom)
    {
        yTop = rcArea.bottom - h_Dlg;
    }

    SetWindowPos(hDlg, NULL, xLeft, yTop, -1, -1, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}


INT_PTR CALLBACK NewSiteCert_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPNEWSITECERTINFO pnsci = (LPNEWSITECERTINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            DWORD  dwFileSize;
            DWORD  cbRead;
            HANDLE hf;
            LPTSTR lpszCmdLine = (LPTSTR)lParam;
            DWORD  dwError;

            hf = CreateFile(lpszCmdLine, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,  NULL);
            if (hf == INVALID_HANDLE_VALUE)
            {
                dwError = GetLastError();
                goto initError;
            }

            dwFileSize = GetFileSize(hf, NULL);
            if (dwFileSize == (unsigned)-1)
                goto initError;

            pnsci = (LPNEWSITECERTINFO)LocalAlloc(LPTR, sizeof(*pnsci));
            if (!pnsci)
                goto initError;

            pnsci->lpvCertData = LocalAlloc(LPTR, dwFileSize);
            if (!pnsci->lpvCertData)
                goto initError;

            pnsci->cbCert      = dwFileSize;

            if (!ReadFile(hf, pnsci->lpvCertData, dwFileSize, &cbRead, NULL) || cbRead != dwFileSize)
                goto initError;

            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pnsci);  // save pointer to cert

            //
            //  ok check to make sure that 1) it's a cert file and 2) it's a root!
            //
            PCCERT_CONTEXT  pCertContext;

            dwError = S_FALSE;

            pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                        (LPBYTE)(pnsci->lpvCertData),
                                                        pnsci->cbCert);

            if (pCertContext)
            {
                if (CertCompareCertificateName(X509_ASN_ENCODING,
                                               &pCertContext->pCertInfo->Subject,
                                               &pCertContext->pCertInfo->Issuer))
                {
                    dwError = S_OK;
                }
             
                CertFreeCertificateContext(pCertContext);
            }

            if (dwError != S_OK)
            {
                goto initError;
            }

            NewSiteCert_SetAvailableAuthorityCheckboxes(hDlg, pnsci, TRUE);

            NewSiteCert_CenterDialog(hDlg);

            break;

initError:
            TCHAR   szTitle[MAX_PATH + 1];
            TCHAR   szError[MAX_PATH + 1];

            MLLoadShellLangString(IDS_CERT_FILE_INVALID, &szError[0], MAX_PATH);
            MLLoadShellLangString(IDS_ERROR, &szTitle[0], MAX_PATH);
            MessageBox(GetFocus(), &szError[0], &szTitle[0], MB_OK | MB_ICONERROR);

            EndDialog(hDlg, IDCANCEL);
            return FALSE;            
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    pnsci->fCertEnabled         = IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE_CERT);
                    pnsci->fNetworkClient       = IsDlgButtonChecked(hDlg, IDC_CHECK_NETWORK_CLIENT);
                    pnsci->fNetworkServer       = IsDlgButtonChecked(hDlg, IDC_CHECK_NETWORK_SERVER);
                    pnsci->fSecureEmail         = IsDlgButtonChecked(hDlg, IDC_CHECK_SECURE_EMAIL);
                    pnsci->fSoftwarePublishing  = IsDlgButtonChecked(hDlg, IDC_CHECK_SOFTWARE_PUBLISHING);

                    NewSiteCert_AddCert(pnsci);

                    EndDialog(hDlg, IDOK);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_VIEWCERT:
                    ShowX509EncodedCertificate(hDlg, (LPBYTE)pnsci->lpvCertData, pnsci->cbCert);
                    break;

                case IDC_CHECK_ENABLE_CERT:

                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        BOOL    fEnableCert;

                        fEnableCert = IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE_CERT);

                        if (!(fEnableCert))
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NETWORK_CLIENT),        fEnableCert);
                            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NETWORK_SERVER),        fEnableCert);
                            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SECURE_EMAIL),          fEnableCert);
                            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SOFTWARE_PUBLISHING),   fEnableCert);
                        }
                        else
                        {
                            NewSiteCert_SetAvailableAuthorityCheckboxes(hDlg, pnsci, FALSE);
                        }
                    }

                    return(FALSE);

                default:
                    return FALSE;
            }
            return TRUE;                
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            if (pnsci)
            {
                if (pnsci->lpvCertData)
                    LocalFree(pnsci->lpvCertData);
                LocalFree(pnsci);
            }
            break;
    }
    return FALSE;
}

STDAPI SiteCert_RunFromCmdLine(HINSTANCE hinst, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{

    if ((!lpszCmdLine) || (*lpszCmdLine == TEXT('\0')))
        return -1;

    DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_NEWSITECERT),
                   NULL, NewSiteCert_DlgProc, (LPARAM)lpszCmdLine);

    return 0;
}



// Helper function for ExportPFX
#define NUM_KNOWN_STORES 5
BOOL OpenAndAllocKnownStores(DWORD *pchStores, HCERTSTORE  **ppahStores)
{
    HCERTSTORE  hStore;
    int i;
    static const LPCTSTR rszStoreNames[NUM_KNOWN_STORES] = {
        TEXT("ROOT"), 
        TEXT("TRUST"),
        TEXT("CA"),
        TEXT("MY"),
        TEXT("SPC")
    };
    
    *pchStores = 0;

    if (NULL == ((*ppahStores) = (HCERTSTORE *) LocalAlloc(LPTR, sizeof(HCERTSTORE) * NUM_KNOWN_STORES)))
    {
         return (FALSE);
    }

    for (i=0; i< NUM_KNOWN_STORES; i++)
    {
        (*ppahStores)[i] = NULL;
        if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                    0,
                                    0,
                                    CERT_SYSTEM_STORE_CURRENT_USER |
                                    CERT_STORE_READONLY_FLAG |
                                    CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                    rszStoreNames[i]))
            (*ppahStores)[(*pchStores)++] = hStore;
    }
    
    return(TRUE);
}

// Helper function for ExportPFX
void CloseAndFreeKnownStores(HCERTSTORE  *pahStores)
{ 
    int i;

    for (i=0; i<NUM_KNOWN_STORES; i++)
    {
        if (pahStores[i] != NULL)
        {
           CertCloseStore(pahStores[i], 0);
        }
    }

    LocalFree(pahStores);
}



enum {PFX_IMPORT, PFX_EXPORT};

typedef struct 
{
    HWND            hDlg;                     // handle to window
    DWORD           dwImportExport;           // import or export?
    BOOL            fUseExisting;             // use existing cert if collision on import
    PCCERT_CONTEXT  pCertContext;    // context to export or NULL
    LPWSTR          pwszPassword;             // password for import/export
    LPWSTR          pwszPassword2;          // prompt user twice on exports!
    LPTSTR          pszPath;                 // file for import/export

} IMPORTEXPORT, *LPIMPORTEXPORT;

#define MAX_PASSWORD 32

// CreateCertFile: change working directory to "MyDocs", do CreateFile, restore old working directory
HANDLE CreateCertFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
        DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    TCHAR szOldDir[MAX_PATH];
    TCHAR szCertDir[MAX_PATH];
    HANDLE hFile;
    LPITEMIDLIST pidl;
    
    GetCurrentDirectory(ARRAYSIZE(szOldDir), szOldDir);
    if (SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl) == NOERROR)
    {
        SHGetPathFromIDList(pidl, szCertDir);
        SetCurrentDirectory(szCertDir);
        ILFree(pidl);                        
    }
    hFile = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, 
        dwCreationDistribution, dwFlagsAndAttributes, hTemplateFile);
    SetCurrentDirectory(szOldDir);
    
    return hFile;
}

//////////////////////////////////////////////////////////////////////////
//
//  09-Sep-1997 pberkman:
//          determine if the exact cert is in the passed store
//

BOOL __IsCertInStore(PCCERT_CONTEXT pCertContext, HCERTSTORE hStore)
{
    //
    //  can't do it the fast way -- do it the slow way!
    //
    BYTE            *pbHash;
    DWORD           cbHash;
    CRYPT_HASH_BLOB sBlob;
    PCCERT_CONTEXT  pWorkContext;

    cbHash = 0;

    if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, NULL, &cbHash)))
    {
        return(FALSE);
    }

    if (cbHash < 1)
    {
        return(FALSE);
    }

    if (!(pbHash = new BYTE[cbHash]))
    {
        return(FALSE);
    }

    if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, pbHash, &cbHash)))
    {
        delete pbHash;
        return(FALSE);
    }

    sBlob.cbData    = cbHash;
    sBlob.pbData    = pbHash;

    pWorkContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
                                              CERT_FIND_SHA1_HASH, &sBlob, NULL);

    delete pbHash;

    if (pWorkContext)
    {
        CertFreeCertificateContext(pWorkContext);
        return(TRUE);
    }

    return(FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
//  09-Sep-1997 pberkman:
//          importing a cert from a file.
//

BOOL ImportPFX(LPIMPORTEXPORT pImp)
{   
#   define MY_STORE         0
#   define CA_STORE         1
#   define ROOT_STORE       2
#   define MAX_STORE        3
    HCERTSTORE          pahStores[MAX_STORE];
    HCERTSTORE          hCertStore;
    BOOL                fAdded;
    DWORD               dwAddFlags;
    
    HANDLE              hFile;
    CRYPT_DATA_BLOB     sData;  
    
    BOOL                fRet;
    PCCERT_CONTEXT      pCertCtxt;
    DWORD               cbRead;
    DWORD               dwImportFlags;
    int                 i;

    fRet            = FALSE;
    dwImportFlags   = CRYPT_EXPORTABLE;
    pCertCtxt       = NULL;
    hCertStore      = NULL;

    for (i = 0; i < MAX_STORE; i++)
    {
        pahStores[i] = NULL;
    }

    ZeroMemory(&sData, sizeof(CRYPT_DATA_BLOB));
    
    hFile = CreateCertFile(pImp->pszPath, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        goto Cleanup;
    }
        
    dwAddFlags      = (pImp->fUseExisting) ? CERT_STORE_ADD_USE_EXISTING :
                                             CERT_STORE_ADD_REPLACE_EXISTING;
    
    sData.cbData = GetFileSize(hFile, NULL);
    sData.pbData = (PBYTE)LocalAlloc(LMEM_FIXED, sData.cbData);

    if (!(sData.pbData))
    {
        goto Cleanup;
    }

    if (!(ReadFile(hFile, sData.pbData, sData.cbData, &cbRead, NULL)))
    {
        goto Cleanup;
    }
    
    if ((pImp->pwszPassword) && (!(*pImp->pwszPassword)))     // if no password, use null.
    {
        pImp->pwszPassword = NULL;
    }
    
    if (!(hCertStore = PFXImportCertStore(&sData, pImp->pwszPassword, dwImportFlags)))
    {
        goto Cleanup;
    }

    //
    //  now we have in memory hStore enumerate the cert contexts
    //  and drop them into destination store
    //
    if (!(pahStores[MY_STORE]   = CertOpenSystemStoreA(NULL, "MY")) ||
        !(pahStores[CA_STORE]   = CertOpenSystemStoreA(NULL, "CA")) ||
        !(pahStores[ROOT_STORE] = CertOpenSystemStoreA(NULL, "ROOT")))
    {
        goto Cleanup;
    }

    while (pCertCtxt = CertEnumCertificatesInStore(hCertStore, pCertCtxt))
    {
        fAdded = FALSE;
        cbRead = 0;
        CertGetCertificateContextProperty(pCertCtxt, CERT_KEY_PROV_INFO_PROP_ID, NULL, &cbRead);
        
        if (cbRead > 0) // pfx added a public key prop
        {
            CertAddCertificateContextToStore(pahStores[MY_STORE], pCertCtxt, dwAddFlags, NULL);
            continue;
        }

        //
        //  first, check if we already have this cert in one of our stores
        //
        for (i = 0; i < MAX_STORE; i++)
        {
            if (__IsCertInStore(pCertCtxt, pahStores[i]))
            {
                //
                // the same cert, exactly, is already in one of our stores!
                //
                fAdded = TRUE;
                break;
            }
        }

        if (!(fAdded))
        {
            CertAddCertificateContextToStore(pahStores[CA_STORE], pCertCtxt, dwAddFlags, NULL);
        }
    }

    fRet = TRUE;

Cleanup:
    
    if (sData.pbData)
    {
        LocalFree(sData.pbData);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    if (hCertStore)
    {
        CertCloseStore(hCertStore, 0);
    }

    for (i = 0; i < MAX_STORE; i++)
    {
        if (pahStores[i])
        {
            CertCloseStore(pahStores[i], 0);
        }
    }
    
    return(fRet);
}

typedef PCCERT_CONTEXT (* PFNWTHELPER) (PCCERT_CONTEXT /* pChildContext */, 
                                        DWORD          /* chStores      */,
                                        HCERTSTORE *   /* pahStores     */,
                                        FILETIME *     /* psftVerifyAsOf*/,
                                        DWORD          /* dwEncoding    */,
                                        DWORD *        /* pdwConfidence */,
                                        DWORD *        /* pdwError      */ );



BOOL ExportPFX(LPIMPORTEXPORT pImp)
{
    BOOL                    fRet = FALSE;
    HANDLE                  hFile = NULL;
    CRYPT_DATA_BLOB         sData;  
    DWORD                   cbRead;
    HCERTSTORE              hSrcCertStore;        
    DWORD                   dwExportFlags = 4; //  4 == EXPORT_PRIVATE_KEYS;
    TCHAR                   szText[MAX_PATH], szTitle[80];
    PCCERT_CONTEXT pTempCertContext;
    HCERTSTORE	*phCertStores = NULL;
    DWORD		chCertStores = 0;
    DWORD		dwConfidence;
    DWORD		dwError;
    HINSTANCE hiWintrust = NULL;
    PFNWTHELPER WTHelperCertFindIssuerCertificate;
    

    if (!pImp->pCertContext)
        return FALSE;

    ZeroMemory(&sData, sizeof(CRYPT_DATA_BLOB));

    // create an in memory store
    hSrcCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                                  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                  0,
                                  0,
                                  NULL);

    if (!CertAddCertificateContextToStore(hSrcCertStore, pImp->pCertContext, CERT_STORE_ADD_REPLACE_EXISTING, NULL))
        goto Cleanup;
    
    // Load helper function from wintrust.dll
    hiWintrust = LoadLibrary(TEXT("WINTRUST.DLL"));
    WTHelperCertFindIssuerCertificate = (PFNWTHELPER) GetProcAddress(hiWintrust,"WTHelperCertFindIssuerCertificate");
    if (WTHelperCertFindIssuerCertificate)
    {
        // Load all the top level stores, so we can export from them if necessary
        if (OpenAndAllocKnownStores(&chCertStores, &phCertStores))
        {
            // Find the intermediate certifcates, and add them to the store that we will be exporting
            pTempCertContext = pImp->pCertContext;
            while (NULL != ( pTempCertContext = WTHelperCertFindIssuerCertificate(pTempCertContext,
                                                    chCertStores,
                                                    phCertStores,
                                                    NULL,
                                                    X509_ASN_ENCODING,
                                                    &dwConfidence,
                                                    &dwError)))
            {
                CertAddCertificateContextToStore(hSrcCertStore, pTempCertContext, CERT_STORE_ADD_REPLACE_EXISTING, NULL);

                // Break out if we find a root (self-signed) cert
                if (CertCompareCertificateName(X509_ASN_ENCODING,
                                               &pTempCertContext->pCertInfo->Subject,
                                               &pTempCertContext->pCertInfo->Issuer))
                    break;
            } 

            CloseAndFreeKnownStores(phCertStores);
        }
    }

    //
    //  This first call simply gets the size of the crypt blob
    //
    if (!PFXExportCertStore(hSrcCertStore, &sData, pImp->pwszPassword, dwExportFlags))
    {
        goto Cleanup;
    }

    //  Alloc based on cbData
    sData.pbData = (PBYTE)LocalAlloc(LMEM_FIXED, sData.cbData);

    //
    //  Now actually get the data
    //
    if (!(*pImp->pwszPassword))         // no password use null
        pImp->pwszPassword = NULL;
    
    if (!PFXExportCertStore(hSrcCertStore, &sData, pImp->pwszPassword, dwExportFlags))
    {
        goto Cleanup;
    }

    //  Open the PFX file
    hFile = CreateCertFile(pImp->pszPath,
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)  {
        goto Cleanup;
    }

    //  Write to it
    if (!WriteFile(hFile,
                   sData.pbData,
                   sData.cbData,
                   &cbRead,
                   NULL)) {
        goto Cleanup;
    }

    // Display message about certs exporting OK.
    MLLoadShellLangString(IDS_CERT_EXPORTOKTEXT, szText, ARRAYSIZE(szText));
    MLLoadShellLangString(IDS_CERT_EXPORTOKTITLE, szTitle, ARRAYSIZE(szTitle));

    MessageBox(pImp->hDlg, szText, szTitle, MB_ICONINFORMATION | MB_OK);

    fRet = TRUE;

Cleanup:
    if (hiWintrust)
        FreeLibrary(hiWintrust);
    if (hSrcCertStore)
        CertCloseStore(hSrcCertStore, 0);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (sData.pbData)
        LocalFree(sData.pbData);

    return fRet;
}
 
INT_PTR CALLBACK ImportExportDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPIMPORTEXPORT pImp;

    if (uMsg == WM_INITDIALOG)
    {
        pImp = (LPIMPORTEXPORT)lParam;    // this is passed in to us
        if (!pImp)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }

        // tell dialog where to get info
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pImp);

        // save handle to the page
        pImp->hDlg           = hDlg;

        // limit the password to 32 chars
        SendMessage(GetDlgItem(hDlg, IDC_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD, 0);

        //
        // 03-Oct-1997 pberkman: always verify password!
        //
        if (pImp->dwImportExport == PFX_EXPORT)
        {
            SendMessage(GetDlgItem(hDlg, IDC_PASSWORD2), EM_LIMITTEXT, MAX_PASSWORD, 0);
        }

        SHAutoComplete(GetDlgItem(hDlg, IDC_FILENAME), SHACF_DEFAULT);      // This control exists in both IDD_PFX_IMPORT and IDD_PFX_EXPORT
        
        // only set these on import, since they don't exist on export =)
        // =========================================================================
        //  03-Oct-1997 pberkman: no user decisions!
        //
        // if (pImp->dwImportExport == PFX_IMPORT)
        // {
        //     CheckRadioButton(hDlg, IDC_USE_EXISTING, IDC_USE_FILE, IDC_USE_EXISTING);                        
        // }            
        // ==========================================================================
        SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));

    }   // WM_INITDIALOG
    
    else
        pImp = (LPIMPORTEXPORT)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pImp)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CERT_BROWSE:
                {
                    TCHAR szFilenameBrowse[MAX_PATH];
                    TCHAR szExt[MAX_PATH];
                    TCHAR szFilter[MAX_PATH];
                    int   ret;
                    LPITEMIDLIST pidl;
                    TCHAR szWorkingDir[MAX_PATH];
                   
                    szFilenameBrowse[0] = 0;
                    MLLoadString(IDS_PFX_EXT, szExt, ARRAYSIZE(szExt));
                    int cchFilter = MLLoadString(IDS_PFX_FILTER, szFilter, ARRAYSIZE(szFilter)-1);

                    // Make sure we have a double null termination on the filter
                    szFilter[cchFilter + 1] = 0;

                    if (SHGetSpecialFolderLocation(hDlg, CSIDL_PERSONAL, &pidl) == NOERROR)
                    {
                        SHGetPathFromIDList(pidl, szWorkingDir);
                        ILFree(pidl);                        
                    }

                    ret = _AorW_GetFileNameFromBrowse(hDlg, szFilenameBrowse, ARRAYSIZE(szFilenameBrowse), szWorkingDir, 
                        szExt, szFilter, NULL);
                    
                    if (ret > 0)
                    {
                        SetDlgItemText(hDlg, IDC_FILENAME, szFilenameBrowse);
                    }
                    break;
                }
                
                case IDOK:
                {
                    TCHAR szPassword[MAX_PASSWORD];
                    TCHAR szPassword2[MAX_PASSWORD];
                    TCHAR szPath[MAX_PATH];
                    BOOL bRet;
                    
                    szPassword[0] = NULL;
                    GetWindowText(GetDlgItem(hDlg, IDC_PASSWORD), szPassword, ARRAYSIZE(szPassword));
                    GetWindowText(GetDlgItem(hDlg, IDC_FILENAME), szPath,     ARRAYSIZE(szPath));

                    //
                    //  03-Oct-1997 pberkman: always double check password!
                    //
                    if (pImp->dwImportExport == PFX_EXPORT)
                    {
                        szPassword2[0] = NULL;
                        GetWindowText(GetDlgItem(hDlg, IDC_PASSWORD2), szPassword2, ARRAYSIZE(szPassword2));

                        if (StrCmp(szPassword, szPassword2) != 0)
                        {
                            TCHAR   szTitle[MAX_PATH + 1];
                            TCHAR   szError[MAX_PATH + 1];

                            MLLoadShellLangString(IDS_PASSWORDS_NOMATCH, &szError[0], MAX_PATH);
                            MLLoadShellLangString(IDS_ERROR, &szTitle[0], MAX_PATH);
                            MessageBox(GetFocus(), &szError[0], &szTitle[0], MB_OK | MB_ICONERROR);

                            SetFocus(GetDlgItem(hDlg, IDC_PASSWORD));

                            break;
                        }
                    }

                    // Add a default extension on export
                    if (pImp->dwImportExport == PFX_EXPORT)
                        if (szPath[0] != TEXT('\0') && PathAddExtension(szPath, TEXT(".PFX")))
                            SetWindowText(GetDlgItem(hDlg, IDC_FILENAME), szPath);

#ifndef UNICODE
                    WCHAR wszPassword[MAX_PASSWORD];
                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPassword, -1, wszPassword, ARRAYSIZE(wszPassword));
                    pImp->pwszPassword = wszPassword;
#else
                    pImp->pwszPassword = szPassword;
#endif
                    pImp->pszPath = szPath;
                    
                    if (pImp->dwImportExport == PFX_IMPORT)
                    {
                        // =========================================================================
                        //  03-Oct-1997 pberkman: no user decisions!
                        //
                        // pImp->fUseExisting = IsDlgButtonChecked(hDlg, IDC_USE_EXISTING);
                        // =========================================================================
                        pImp->fUseExisting = FALSE;
                        bRet = ImportPFX(pImp);

                        if (!(bRet) && (GetLastError() == NTE_BAD_DATA))
                        {
                            // message....
                        }
                    }
                    else
                    {
                        bRet = ExportPFX(pImp);
                    }
                    
                    EndDialog(hDlg, bRet);
                    break;
                }
                
                case IDCANCEL:
                    EndDialog(hDlg, TRUE); // Cancel is not an error
                    break;

            }
            break;
            
        case WM_NOTIFY:
            break;
// No context sensitive help yet...
#if 0
        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
#endif
        case WM_DESTROY:
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            break;
    }
    return FALSE;
}


#ifdef UNIX
EXTERN_C
#endif
INT_PTR ImportExportPFX(HWND hwndParent, DWORD dwImportExport, LPBYTE pbCert, DWORD cbCert)
{
    IMPORTEXPORT    imp;

    if (pbCert)
    {
        CRYPT_HASH_BLOB  hashBlob;
        HCERTSTORE       hMy = CertOpenSystemStoreA(NULL, "MY");
        DWORD            cbSHA1Hash;
        LPBYTE           pbSHA1Hash;

        if (!hMy)
            return FALSE;
        
        if (CryptHashCertificate(NULL, 0, 0, pbCert, cbCert, NULL, &cbSHA1Hash))
        {
            pbSHA1Hash = (LPBYTE)LocalAlloc(LPTR, cbSHA1Hash);
            if (!pbSHA1Hash)
                return FALSE;

            if (CryptHashCertificate(NULL, 0, 0, pbCert, cbCert, pbSHA1Hash, &cbSHA1Hash))
            {                                
                hashBlob.cbData = cbSHA1Hash;
                hashBlob.pbData = pbSHA1Hash;
                imp.pCertContext = CertFindCertificateInStore(hMy, X509_ASN_ENCODING, 0, CERT_FIND_HASH, &hashBlob, NULL);
                if (!(imp.pCertContext))
                    return FALSE;
            }

            LocalFree(pbSHA1Hash);
        }

        CertCloseStore(hMy, 0);
    }

    imp.dwImportExport = dwImportExport;
    
    return DialogBoxParam(MLGetHinst(),
                   dwImportExport == PFX_IMPORT ? MAKEINTRESOURCE(IDD_PFX_IMPORT) : MAKEINTRESOURCE(IDD_PFX_EXPORT),
                   hwndParent, ImportExportDlgProc, (LPARAM)&imp);
}

//BUBUG: The following function should be rermoved when we have updated our Crypto API to latest
BOOL WINAPI WTHelperIsInRootStore(PCCERT_CONTEXT pCertContext)
{
    HCERTSTORE  hStore;

    if (!(hStore = CertOpenStore(   CERT_STORE_PROV_SYSTEM_A,
                                    0,
                                    NULL,
                                    CERT_SYSTEM_STORE_CURRENT_USER |
                                    CERT_STORE_READONLY_FLAG |
                                    CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                    "ROOT")))
    {
        return(FALSE);
    }


    //
    //  can't do it the fast way -- do it the slow way!
    //
    BYTE            *pbHash;
    DWORD           cbHash;
    CRYPT_HASH_BLOB sBlob;
    PCCERT_CONTEXT  pWorkContext;

    cbHash = 0;

    if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, NULL, &cbHash)))
    {
        CertCloseStore(hStore, 0);
        return(FALSE);
    }

    if (cbHash < 1)
    {
        CertCloseStore(hStore, 0);
        return(FALSE);
    }

    if (!(pbHash = new BYTE[cbHash]))
    {
        CertCloseStore(hStore, 0);
        return(FALSE);
    }

    if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, pbHash, &cbHash)))
    {
        delete pbHash;
        CertCloseStore(hStore, 0);
        return(FALSE);
    }

    sBlob.cbData    = cbHash;
    sBlob.pbData    = pbHash;

    pWorkContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
                                              CERT_FIND_SHA1_HASH, &sBlob, NULL);

    delete pbHash;

    if (pWorkContext)
    {
        CertFreeCertificateContext(pWorkContext);
        CertCloseStore(hStore, 0);
        return(TRUE);
    }

    CertCloseStore(hStore, 0);

    return(FALSE);
}


//============================================================================
const TCHAR c_szRegKeySMIEM[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
const TCHAR c_szRegValFormSuggest[] = TEXT("Use FormSuggest");
const TCHAR c_szRegValFormSuggestPW[] = TEXT("FormSuggest Passwords");
const TCHAR c_szRegValFormSuggestPWAsk[] = TEXT("FormSuggest PW Ask");

const TCHAR c_szYes[] = TEXT("yes");
const TCHAR c_szNo[] = TEXT("no");

inline void SetValueHelper(HWND hDlg, int id, LPTSTR *ppszData, DWORD *pcbData)
{
    if (IsDlgButtonChecked(hDlg, id))
    {
        *ppszData = (LPTSTR)c_szYes;
        *pcbData = sizeof(c_szYes);
    }
    else
    {
        *ppszData = (LPTSTR)c_szNo;
        *pcbData = sizeof(c_szNo);
    }
}

INT_PTR CALLBACK AutoSuggestDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CheckDlgButton(hDlg, IDC_AUTOSUGGEST_ENABLEADDR,
                (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE)) ?
                BST_CHECKED : BST_UNCHECKED);

            if (g_restrict.fFormSuggest)
            {
                EnableDlgItem(hDlg, IDC_AUTOSUGGEST_ENABLEFORM, FALSE);
            }
            else
            {
                CheckDlgButton(hDlg, IDC_AUTOSUGGEST_ENABLEFORM,
                    (SHRegGetBoolUSValue(c_szRegKeySMIEM, c_szRegValFormSuggest, FALSE, /*default:*/FALSE)) ?
                    BST_CHECKED : BST_UNCHECKED);
            }

            if (g_restrict.fFormPasswords)
            {
                EnableDlgItem(hDlg, IDC_AUTOSUGGEST_SAVEPASSWORDS, FALSE);
                EnableDlgItem(hDlg, IDC_AUTOSUGGEST_PROMPTPASSWORDS, FALSE);
            }
            else
            {
                CheckDlgButton(hDlg, IDC_AUTOSUGGEST_PROMPTPASSWORDS,
                    (SHRegGetBoolUSValue(c_szRegKeySMIEM, c_szRegValFormSuggestPWAsk, FALSE, /*default:*/TRUE)) ?
                    BST_CHECKED : BST_UNCHECKED);
            
                if (SHRegGetBoolUSValue(c_szRegKeySMIEM, c_szRegValFormSuggestPW, FALSE, /*default:*/TRUE))
                {
                    CheckDlgButton(hDlg, IDC_AUTOSUGGEST_SAVEPASSWORDS, BST_CHECKED);
                }
                else
                {
                    EnableDlgItem(hDlg, IDC_AUTOSUGGEST_PROMPTPASSWORDS, FALSE);
                }
            }
        }

        return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_AUTOSUGGEST_SAVEPASSWORDS:
                    EnableDlgItem(hDlg, IDC_AUTOSUGGEST_PROMPTPASSWORDS,
                        IsDlgButtonChecked(hDlg, IDC_AUTOSUGGEST_SAVEPASSWORDS));
                    break;
                    
                case IDC_AUTOSUGGEST_CLEARFORM:
                case IDC_AUTOSUGGEST_CLEARPASSWORDS:
                {
                    BOOL fPasswords = (LOWORD(wParam) == IDC_AUTOSUGGEST_CLEARPASSWORDS);
                    DWORD dwClear = (fPasswords) ?
                            IECMDID_ARG_CLEAR_FORMS_PASSWORDS_ONLY : IECMDID_ARG_CLEAR_FORMS_ALL_BUT_PASSWORDS;

                    if (IDOK == MsgBox(hDlg, ((fPasswords) ? IDS_CLEAR_FORMPASSWORDS : IDS_CLEAR_FORMSUGGEST), MB_ICONQUESTION, MB_OKCANCEL))
                    {
                        HCURSOR hOldCursor = NULL;
                        HCURSOR hNewCursor = NULL;

#ifndef UNIX
                        hNewCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
#else
                        // IEUNIX - Getting rid of redundant MAKEINTRESOURCE 
                        hNewCursor = LoadCursor(NULL, IDC_WAIT);
#endif

                        if (hNewCursor) 
                            hOldCursor = SetCursor(hNewCursor);

                        // Clear all strings
                        ClearAutoSuggestForForms(dwClear);

                        // Also reset profile assistant sharing (very discoverable here)
                        if (!g_restrict.fProfiles)
                        {
                            ResetProfileSharing(hDlg);
                        }

                        if(hOldCursor)
                            SetCursor(hOldCursor);
                    }
                }
                break;

                case IDOK:
                {
                    DWORD cbData; LPTSTR pszData;

                    SetValueHelper(hDlg, IDC_AUTOSUGGEST_ENABLEADDR, &pszData, &cbData);
                    SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST,
                        REG_SZ, pszData, cbData);

                    if (!g_restrict.fFormSuggest)
                    {
                        SetValueHelper(hDlg, IDC_AUTOSUGGEST_ENABLEFORM, &pszData, &cbData);
                        SHSetValue(HKEY_CURRENT_USER, c_szRegKeySMIEM, c_szRegValFormSuggest,
                            REG_SZ, pszData, cbData);
                    }

                    if (!g_restrict.fFormPasswords)
                    {
                        SetValueHelper(hDlg, IDC_AUTOSUGGEST_SAVEPASSWORDS, &pszData, &cbData);
                        SHSetValue(HKEY_CURRENT_USER, c_szRegKeySMIEM, c_szRegValFormSuggestPW,
                            REG_SZ, pszData, cbData);
                            
                        SetValueHelper(hDlg, IDC_AUTOSUGGEST_PROMPTPASSWORDS, &pszData, &cbData);
                        SHSetValue(HKEY_CURRENT_USER, c_szRegKeySMIEM, c_szRegValFormSuggestPWAsk,
                            REG_SZ, pszData, cbData);
                    }
                }
                // fall through
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                }
                break;
            }
        }
        return TRUE;

    case WM_HELP:                   // F1
        ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                    HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
    break;

    case WM_CONTEXTMENU:        // right mouse click
        ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                    HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
    break;

    case WM_DESTROY:
        break;

    }

    return FALSE;
}

#ifdef WALLET
// This intermediate dialog is only displayed for wallet 2.x users
INT_PTR CALLBACK WalletDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            EnableDlgItem(hDlg, IDC_PROGRAMS_WALLET_PAYBUTTON, IsWalletPaymentAvailable());
            EnableDlgItem(hDlg, IDC_PROGRAMS_WALLET_ADDRBUTTON, IsWalletAddressAvailable());
        }
        return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_PROGRAMS_WALLET_PAYBUTTON:
                    DisplayWalletPaymentDialog(hDlg);
                    break;

                case IDC_PROGRAMS_WALLET_ADDRBUTTON:
                    DisplayWalletAddressDialog(hDlg);
                    break;

                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                }
                break;
            }
        }
        return TRUE;

    case WM_HELP:                   // F1
        ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                    HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
    break;

    case WM_CONTEXTMENU:        // right mouse click
        ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                    HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
    break;

    case WM_DESTROY:
        break;

    }

    return FALSE;
}
#endif
