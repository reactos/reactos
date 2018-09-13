//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       persist.cxx
//
//  Contents:   Implmentation of Office9 Thicket Save API
//
//----------------------------------------------------------------------------
#include "priv.h"

//#include "headers.hxx"
//#include "formkrnl.hxx"
#include <platform.h>
#include <mlang.h>
#include "resource.h"
#include "impexp.h"
#include "reload.h"
//#include <siterc.h>
#include "packager.h"
#include "iehelpid.h"
#include "thicket.h"
#include "apithk.h"

#include <mluisupp.h>
#include <mshtmcid.h>

#define NUM_OLE_CMDS                          1

#define SAVEAS_OK                             0x00000001
#define SAVEAS_NEVER_ASK_AGAIN                0x00000002

#define CODEPAGE_UNICODE                      0x000004B0
#define CODEPAGE_UTF8                         0x0000FDE9
#define UNICODE_TEXT                          TEXT("Unicode")

#define REGSTR_VAL_SAVEDIRECTORY              TEXT("Save Directory")
#define REGKEY_SAVEAS_WARNING_RESTRICTION     TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main")
#define REGVALUE_SAVEAS_WARNING               TEXT("NoSaveAsPOSTWarning")

#define WM_WORKER_THREAD_COMPLETED            WM_USER + 1000

#define MAX_ENCODING_DESC_LEN                 1024


const static DWORD aSaveAsHelpIDs[] =
{
    IDC_SAVE_CHARSET,   IDH_CHAR_SET_SAVE_AS,
    0,                  0
};

INT_PTR CALLBACK SaveAsWarningDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                      LPARAM lParam);

HRESULT SaveToThicket( HWND hwnd, LPCTSTR pszFileName, IHTMLDocument2 *pDoc,
                       UINT codepageSrc, UINT codepageDst,
                       UINT iPackageStyle );

HRESULT
FormsGetFileName(
        HWND hwndOwner,
        LPTSTR pstrFile,
        int cchFile,
        LPARAM lCustData,
        DWORD *pnFilterIndex,
        BOOL bForceHTMLOnly);
HRESULT
GetFileNameFromURL( LPWSTR pwszURL, LPTSTR pszFile, DWORD cchFile);

void ReportThicketError( HWND hwnd, HRESULT hr );

#define DOWNLOAD_PROGRESS  0x9001
#define DOWNLOAD_COMPLETE  0x9002
#define THICKET_TIMER      0x9003
#define THICKET_INTERVAL   1000

#define MDLGMSG(psz, x)         TraceMsg(0, "shd TR-MODELESS::%s %x", psz, x)

static DWORD s_dwInetComVerMS = 0;
static DWORD s_dwInetComVerLS = 0;

struct ThicketCPInfo
{
    UINT    cpSrc;
    UINT    cpDst;
    LPWSTR lpwstrDocCharSet;
};

class CThicketUI
{
public:
    CThicketUI(void) :
        _hDlg(NULL),
        _hWndProg(NULL),
        _iErrorDL(0),
        _hrDL(E_FAIL),
#ifndef NO_MARSHALLING
        _pstmDoc(NULL),
#else
        _pDoc(NULL),
#endif
        _pszFileName(NULL),
        _dwDLMax(0),
        _codepageSrc(0),
        _codepageDst(0),
        _iPackageStyle(PACKAGE_THICKET),
        _fCancel(FALSE) {};

    ~CThicketUI(void) 
    { 
#ifndef NO_MARSHALLING
        SAFERELEASE(_pstmDoc);
#endif
        SAFELOCALFREE(_pszFileName); 
    };

    // CThicketUI methods
    HRESULT SaveDocument( HWND hWnd, LPCTSTR pszFileName, IHTMLDocument2 *pDoc,
                          UINT codepageSrc, UINT codepageDst,
                          UINT iPackageStyle );

protected:
    static BOOL_PTR ThicketUIDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ThicketUIThreadProc( LPVOID );

    BOOL DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND                _hDlg;
    HWND                _hWndProg;
    int                 _iErrorDL;
    HRESULT             _hrDL;
#ifndef UNIX
    IStream             *_pstmDoc;      // marshalled IHTMLDocument2
#else
    IHTMLDocument2      *_pDoc;
#endif
    LPTSTR              _pszFileName;
    DWORD               _dwDLMax;
    UINT                _codepageSrc;
    UINT                _codepageDst;
    BOOL                _fThreadStarted;
    UINT                _iPackageStyle;
    BOOL                _fCancel;
};

HRESULT
CThicketUI::SaveDocument( HWND hWnd, LPCTSTR pszFileName, IHTMLDocument2 *pDoc,
                          UINT codepageSrc, UINT codepageDst,
                          UINT iPackageStyle)
{
    _pszFileName = StrDup(pszFileName);
    _codepageSrc = codepageSrc;
    _codepageDst = codepageDst;
    _iPackageStyle = iPackageStyle;

#ifndef NO_MARSHALLING
    // We don't do anything with pDoc until we're on the worker thread,
    // so marshall it.
    _hrDL = CoMarshalInterThreadInterfaceInStream(IID_IHTMLDocument2, pDoc, &_pstmDoc);

    if (SUCCEEDED(_hrDL))
#else
    _pDoc = pDoc;
#endif
    {
        // Needs to be modal cuz we're going to work with pDoc on the worker thread
        // so we don't want the user to navigate away from it, close the window, etc.
        DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SAVETHICKET),
                         hWnd, CThicketUI::ThicketUIDlgProc, (LPARAM)this);

     //   HWND hwnd = MLCreateDialogParamWrap(MLGetHinst(), MAKEINTRESOURCE(IDD_SAVETHICKET),
     //                                 NULL, CThicketUI::ThicketUIDlgProc, (LPARAM)this);
     //   if (!hwnd)
     //       _hrDL = E_FAIL;
    }

    return _hrDL;
}

BOOL_PTR
CThicketUI::ThicketUIDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL        fRet = FALSE;
    CThicketUI* ptui = NULL;

    if (msg == WM_INITDIALOG)
    {
        ptui = (CThicketUI*)lParam;
    }
    else
        ptui = (CThicketUI*)GetWindowLongPtr(hDlg, DWLP_USER);

    if (ptui)
    {
        fRet = ptui->DlgProc(hDlg, msg, wParam, lParam);

        if (msg == WM_DESTROY || msg == WM_WORKER_THREAD_COMPLETED)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            fRet = TRUE;
        }
    }

    return fRet;
}

BOOL
CThicketUI::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = TRUE;

    switch (msg)
    {
    case WM_INITDIALOG:
        _hDlg = hDlg;
        _hWndProg = GetDlgItem(hDlg, IDC_THICKETPROGRESS);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        _hrDL = S_FALSE;

#ifndef NO_MARSHALLING
        //_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ThicketUIThreadProc, this, 0, &_idThread);
        if (!(_fThreadStarted = SHQueueUserWorkItem(ThicketUIThreadProc,
                                                    this,
                                                    0,
                                                    (DWORD_PTR)NULL,
                                                    (DWORD_PTR *)NULL,
                                                    "shdocvw.dll",
                                                    TPS_LONGEXECTIME)))
            _hrDL = E_FAIL;
#else
        ThicketUIThreadProc((LPVOID)this);
#endif

        if (FAILED(_hrDL))
             EndDialog(hDlg, 0);
        else
        {
            ShowWindow(hDlg, SW_SHOWNORMAL);
            Animate_OpenEx(GetDlgItem(hDlg, IDD_ANIMATE), HINST_THISDLL, IDA_DOWNLOAD);
            ShowWindow(GetDlgItem(hDlg, IDD_DOWNLOADICON), SW_HIDE);
        }
        fRet = FALSE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            _fCancel = TRUE;
            // and wait for the worker thread to quit, polling at WM_TIMER
            break;

        default:
            break;
        }
        break;

    case WM_WORKER_THREAD_COMPLETED:
        _hrDL = (DWORD) wParam;
        EndDialog(hDlg,0);
        break;

    //case WM_CLOSE:
    //    KillTimer( hDlg, THICKET_TIMER );
    //    _fCancel = TRUE;
    //    while( _hrDL == S_FALSE );
    //    break;

    case WM_DESTROY:
        _fCancel = TRUE;
        while( _hrDL == S_FALSE )
        {
            Sleep(0);
        }
        break;

    default:
        fRet = FALSE;
    }

    return fRet;
}

DWORD WINAPI CThicketUI::ThicketUIThreadProc( LPVOID ppv )
{
    HRESULT hr = S_OK;
    CThicketUI* ptui = (CThicketUI *)ppv;

    ASSERT(ptui);

    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IHTMLDocument2 *pDoc = NULL;

#ifndef NO_MARSHALLING
        hr = CoGetInterfaceAndReleaseStream( ptui->_pstmDoc, IID_IHTMLDocument2,(LPVOID*)&pDoc);

        // CoGetInterfaceAndReleaseStream always releases the stream
        ptui->_pstmDoc = NULL;
#else
        pDoc = ptui->_pDoc;
        pDoc->AddRef();
#endif

        if (SUCCEEDED(hr))
        {
            CThicketProgress    tprog( ptui->_hDlg );
            CDocumentPackager   docPkgr(ptui->_iPackageStyle);

            hr = S_FALSE;

            hr = docPkgr.PackageDocument( pDoc, ptui->_pszFileName,
                          &ptui->_fCancel, &tprog,
                          0, 100,
                          ptui->_codepageDst );

            pDoc->Release(); // release marshalled interface
        }

        CoUninitialize();
    }

    PostMessage(ptui->_hDlg, WM_WORKER_THREAD_COMPLETED, hr, 0);

    return 0;
}


//+------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------
HRESULT
SaveToThicket( HWND hWnd, LPCTSTR pszFileName, IHTMLDocument2 *pDoc,
               UINT codepageSrc, UINT codepageDst, UINT iPackageStyle )
{
    HRESULT     hr;
    CThicketUI* ptui;

#ifdef OLD_THICKET
    LPTSTR      lpszURL;

    lpszURL = bstrDocURL;

    const   DWORD       dwMaxPathLen        = 24;

    URL_COMPONENTS urlComp;
    TCHAR   rgchUrlPath[MAX_PATH];
    TCHAR   rgchCanonicalUrl[MAX_URL_STRING];
    DWORD   dwLen;

    dwLen = ARRAYSIZE(rgchCanonicalUrl);

    hr = UrlCanonicalize( lpszURL, rgchCanonicalUrl, &dwLen, 0);
    if (FAILED(hr))
        return E_FAIL;

    ZeroMemory(&urlComp, sizeof(urlComp));

    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszUrlPath = rgchUrlPath;
    urlComp.dwUrlPathLength = ARRAYSIZE(rgchUrlPath);

    hr = InternetCrackUrl(rgchCanonicalUrl, lstrlen(rgchCanonicalUrl), ICU_DECODE, &urlComp);
    if (FAILED(hr))
        return E_FAIL;

    // Since this is not a snap-shot, saving the doc over itself is a no-op.
    // This means we can avoid some nasty issues with the save-over, safe-save,
    // et al, by short circuiting the save here.
    if ( StrCmpI(pszFileName, rgchUrlPath) == 0 )
    {
        if (PathFileExists(pszFileName))
            return S_OK;
        else
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

#endif //OLD_THICKET

    ptui = new CThicketUI;
    if (ptui)
    {
        hr = ptui->SaveDocument( hWnd, pszFileName, pDoc, codepageSrc, codepageDst, iPackageStyle );
        delete ptui;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//+------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void SaveBrowserFile( HWND hwnd, LPUNKNOWN punk )
{
    HRESULT         hr;
    TCHAR           szFileDst[MAX_PATH];
    DWORD           iFilter = 1;
    IHTMLDocument2  *pDoc;
    BSTR            bstrURL = NULL;
    ThicketCPInfo   tcpi;
    BSTR            bstrCharSet = NULL;
    BSTR            bstrTitle = NULL;
    BSTR            bstrMime = NULL;
    IOleCommandTarget *pOleCommandTarget = NULL;
    WCHAR          *pwzExt = NULL;
    OLECMD          pCmd[NUM_OLE_CMDS];
    ULONG           nCmds = NUM_OLE_CMDS;
    BOOL            bForceHTMLOnly = FALSE;

    static const WCHAR *wzImage = L" Image";
    

    hr = punk->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);
    if (FAILED(hr))
        goto Cleanup;

    if (FAILED(pDoc->get_URL( &bstrURL )))
        goto Cleanup;

    hr = pDoc->get_charset( &bstrCharSet );
    if (FAILED(hr))
        goto Cleanup;

    tcpi.cpSrc = CP_ACP;
    tcpi.lpwstrDocCharSet = bstrCharSet;

    // If it is an image file, then bring up trident to do the save.
    // BUGBUG: This is a crappy way to do this. We are hard-coding the
    // image types, so we know to put up the "Save as image" dialog.
    // We originally tried looking at the MIME type, but Trident returns
    // inconsistent MIME types to us (ex. under some platforms we get
    // "JPG Image" and under others we get "JPG File"!).

    ASSERT(bstrURL);

    pwzExt = bstrURL + lstrlenW(bstrURL);

    while (pwzExt > bstrURL && *pwzExt != L'.')
    {
        pwzExt--;
    }

    hr = pDoc->QueryInterface(IID_IOleCommandTarget,
                              (void **)&pOleCommandTarget);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pwzExt > bstrURL) {

        // Found a "dot". Now pwzExt points to what we think is the extension

        if (!StrCmpIW(pwzExt, L".JPG") ||
            !StrCmpIW(pwzExt, L".GIF") ||
            !StrCmpIW(pwzExt, L".BMP") ||
            !StrCmpIW(pwzExt, L".XBM") ||
            !StrCmpIW(pwzExt, L".ART") ||
            !StrCmpIW(pwzExt, L".PNG") ||
            !StrCmpIW(pwzExt, L".WMF") ||
            !StrCmpIW(pwzExt, L".TIFF") ||
            !StrCmpIW(pwzExt, L".JPEG"))
        {
            hr = pOleCommandTarget->Exec(&CGID_MSHTML, IDM_SAVEPICTURE, 0,
                                         NULL, NULL);

            // BUGBUG: Handle a failed HR here. It is very unlikely that
            // this will fail, yet regular save-as code (that follows)
            // will succeed. We always exit out of here, so we will
            // never get two UI dialogs thrown at the user. We should
            // come up with a good scheme to propagate an error dialog
            // to the user. Possible scenario: low disk space causing
            // a fail-out.

            goto Cleanup;
        }
    }

    // IE5 RAID #54672: Save-as has problems saving pages generated by POSTs
    // This code is to detect if the page was generated by POST data and
    // warn the user that saving may not work.

    pCmd[0].cmdID = SHDVID_PAGEFROMPOSTDATA;
    hr = pOleCommandTarget->QueryStatus(&CGID_ShellDocView, nCmds, pCmd, NULL);

    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (pCmd[0].cmdf & OLECMDF_LATCHED)
    {
        HKEY         hkeySaveAs = 0;
        DWORD        dwValue = 0;
        DWORD        dwSize = 0;
        INT_PTR      iFlags = 0;

        bForceHTMLOnly = TRUE;

        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         REGKEY_SAVEAS_WARNING_RESTRICTION, 0,
                         KEY_READ, &hkeySaveAs) == ERROR_SUCCESS)
        {
            dwSize = sizeof(DWORD);

            if (RegQueryValueEx(hkeySaveAs, REGVALUE_SAVEAS_WARNING, NULL,
                                NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
            {

                if (dwValue)
                {
                    // restriction set, don't show dialog
                    RegCloseKey(hkeySaveAs);
                    goto Continue;
                }
            }

            RegCloseKey(hkeySaveAs);
        }

        iFlags = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(DLG_SAVEAS_WARNING),
                                hwnd, SaveAsWarningDlgProc, (LPARAM)0);

        if (!(iFlags & SAVEAS_OK))
        {
            goto Cleanup;
        }
        
        if (iFlags & SAVEAS_NEVER_ASK_AGAIN)
        {
            HKEY                 hkey = 0;
            DWORD                dwNeverAsk = 1;

            if (RegOpenKeyEx(HKEY_CURRENT_USER,
                             REGKEY_SAVEAS_WARNING_RESTRICTION, 0,
                             KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
            {

                RegSetValueEx(hkey, REGVALUE_SAVEAS_WARNING, 0, REG_DWORD,
                              (CONST BYTE *)&dwNeverAsk,
                              sizeof(dwNeverAsk));
                RegCloseKey(hkey);
            }
        }
    }
    
Continue:

    // Suggest a file name
    
    szFileDst[0] = 0;

    // Our favorite candidate is the title,  fall back on the file name.
    hr = pDoc->get_title(&bstrTitle);
    if (SUCCEEDED(hr) && lstrlenW(bstrTitle))
    {
        StrCpyN(szFileDst, bstrTitle, ARRAYSIZE(szFileDst));
    }
    else
        hr = GetFileNameFromURL(bstrURL, szFileDst, ARRAYSIZE(szFileDst));

    if (FAILED(hr))
        goto Cleanup;

    PathCleanupSpec(NULL, szFileDst);

    hr = FormsGetFileName(hwnd, szFileDst, ARRAYSIZE(szFileDst),
                          (LONG_PTR)&tcpi, &iFilter, bForceHTMLOnly);

    if (hr==S_OK)
        hr = SaveToThicket( hwnd, szFileDst, pDoc, tcpi.cpSrc, tcpi.cpDst, iFilter);

Cleanup:

    if (FAILED(hr))
        ReportThicketError(hwnd, hr);

    if (pOleCommandTarget)
        pOleCommandTarget->Release();

    if (pDoc)
        pDoc->Release();

    if (bstrURL)
        SysFreeString(bstrURL);

    if (bstrCharSet)
        SysFreeString(bstrCharSet);

    if (bstrTitle)
        SysFreeString(bstrTitle);

    return;
}

void ReportThicketError( HWND hwnd, HRESULT hr )
{
    LPTSTR lpstrMsg = NULL;

    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
    case E_OUTOFMEMORY:
        lpstrMsg = MAKEINTRESOURCE(IDS_THICKETERRMEM);
        break;

    case E_ACCESSDENIED:
    case STG_E_ACCESSDENIED:
        lpstrMsg = MAKEINTRESOURCE(IDS_THICKETERRACC);
        break;

    case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
    case STG_E_MEDIUMFULL:
        lpstrMsg = MAKEINTRESOURCE(IDS_THICKETERRFULL);
        break;

    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
        lpstrMsg = MAKEINTRESOURCE(IDS_THICKETERRFNF);
        break;

    case E_ABORT:
        // Ray says we don't want a canceled message box.
        //lpstrMsg = MAKEINTRESOURCE(IDS_THICKETABORT);
        break;

    case E_FAIL:
    default:
        lpstrMsg = MAKEINTRESOURCE(IDS_THICKETERRMISC);
        break;
    }

    if ( lpstrMsg )
    {
        MLShellMessageBox(
                        hwnd,
                        lpstrMsg,
                        MAKEINTRESOURCE(IDS_THICKETERRTITLE),
                        MB_OK | MB_ICONERROR);
    }
}

//+--------------------------------------------------------------------------
//
//  File:       file.cxx
//
//  Contents:   Import/export dialog helpers
//
//  History:    16-May-95   RobBear     Taken from formtool
//
//---------------------------------------------------------------------------

const CHAR c_szNT4ResourceLocale[]      = ".DEFAULT\\Control Panel\\International";
const CHAR c_szWin9xResourceLocale[]    = ".Default\\Control Panel\\desktop\\ResourceLocale";
const CHAR c_szLocale[]                 = "Locale";

LANGID
MLGetShellLanguage()
{
    LANGID  lidShell = 0;

    // BUGBUG this fn is copied from shlwapi. there really should be a
    // shlwapi export. if MLGetUILanguage has any merit, then
    // MLGetShellLanguage has merit as well.

    if (IsOS(OS_NT5))
    {
        static LANGID (CALLBACK* pfnGetUserDefaultUILanguage)(void) = NULL;

        if (pfnGetUserDefaultUILanguage == NULL)
        {
            HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

            if (hmod)
                pfnGetUserDefaultUILanguage = (LANGID (CALLBACK*)(void))GetProcAddress(hmod, "GetUserDefaultUILanguage");
        }
        if (pfnGetUserDefaultUILanguage)
            lidShell = pfnGetUserDefaultUILanguage();
    }
    else
    {
        CHAR szLangID[12];
        DWORD cb, dwRet;

        cb = sizeof(szLangID) - 2*sizeof(szLangID[0]);  // avoid 2 byte buffer overrun
        if (IsOS(OS_NT))
            dwRet = SHGetValueA(HKEY_USERS, c_szNT4ResourceLocale, c_szLocale, NULL, szLangID + 2, &cb);
        else
            dwRet = SHGetValueA(HKEY_USERS, c_szWin9xResourceLocale, NULL, NULL, szLangID + 2, &cb);

        if (ERROR_SUCCESS == dwRet)
        {
            // IE uses a string rep of the hex value
            szLangID[0] = '0';
            szLangID[1] = 'x';
            StrToIntExA(szLangID, STIF_SUPPORT_HEX, (LPINT)&lidShell);
        }
    }

    return lidShell;
}

/*
 *  Stolen from Trident's src\core\cdutil\file.cxx
 */

// Hook procedure for open file dialog.

UINT_PTR APIENTRY SaveOFNHookProc(HWND hdlg,
                                  UINT uiMsg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    ULONG i, iCurSel;
    BOOL  bFoundEncoding = FALSE;
    WCHAR wzEncoding[MAX_ENCODING_DESC_LEN];

    switch (uiMsg)
    {
        // Populate the dropdown.
        case WM_INITDIALOG:
        {
            HRESULT hr;
            LPOPENFILENAME pofn = (LPOPENFILENAME)lParam;
            ThicketCPInfo *ptcpi = (ThicketCPInfo *)pofn->lCustData;
            IMultiLanguage2 *pMultiLanguage = NULL;
            IEnumCodePage  *pEnumCodePage = NULL;
            //UINT            codepageDefault = ptcpi->cp;
            MIMECSETINFO    csetInfo;
            LANGID          langid;

#ifdef UNIX
            SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)ptcpi);
#endif /* UNIX */

            hr = CoCreateInstance(
                    CLSID_CMultiLanguage,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IMultiLanguage2,
                    (void**)&pMultiLanguage);
            if (hr)
                break;

            hr = pMultiLanguage->GetCharsetInfo(ptcpi->lpwstrDocCharSet,&csetInfo);
            if (hr)
                break;

#ifndef UNIX
            // the shell combobox where this stuff shows up
            // doesn't know how to fontlink... so we have
            // to stay in the shell's codepage
            langid = MLGetShellLanguage();
#else
            langid = GetSystemDefaultLangID();
#endif /* UNIX */
            if (pMultiLanguage->EnumCodePages( MIMECONTF_SAVABLE_BROWSER | MIMECONTF_VALID,
                                               langid,
                                               &pEnumCodePage) == S_OK)
            {
                MIMECPINFO cpInfo;
                ULONG      ccpInfo;
                UINT       cpDefault;

                if (pMultiLanguage->GetCodePageInfo(csetInfo.uiInternetEncoding, langid, &cpInfo) == S_OK &&
                    !(cpInfo.dwFlags & MIMECONTF_SAVABLE_BROWSER))
                {
                    // If the codepage selected is not savable (eg JP_AUTO),
                    // use the family codepage.
                    cpDefault = cpInfo.uiFamilyCodePage;
                }
                else
                    cpDefault = csetInfo.uiInternetEncoding;

                ptcpi->cpSrc = csetInfo.uiInternetEncoding;

                if (cpDefault == CODEPAGE_UNICODE &&
                    pofn->nFilterIndex == PACKAGE_MHTML) {
                    cpDefault = CODEPAGE_UTF8;
                }

                for (i = 0; pEnumCodePage->Next(1, &cpInfo, &ccpInfo) == S_OK; ++i)
                {
                    TCHAR *lpszDesc;
                    INT_PTR iIdx;

                    if (cpInfo.uiCodePage == CODEPAGE_UNICODE &&
                        pofn->nFilterIndex == PACKAGE_MHTML) {
                        i--;
                        continue;
                    }

                    if (cpDefault == cpInfo.uiCodePage)
                    {
                       StrCpyNW(wzEncoding, cpInfo.wszDescription,
                                lstrlen(cpInfo.wszDescription) + 1);
                       bFoundEncoding = TRUE;
                    }

                    lpszDesc = cpInfo.wszDescription;

                    iIdx = SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                              CB_ADDSTRING, 0,
                                              (LPARAM)lpszDesc);
                    SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                       CB_SETITEMDATA, iIdx,
                                       (LPARAM)cpInfo.uiCodePage);
                }

                if (bFoundEncoding)
                {
                    INT_PTR iIndex = 0;

                    iIndex = SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                                CB_FINDSTRINGEXACT, -1,
                                                (LPARAM)wzEncoding);

                    SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_SETCURSEL,
                                       (WPARAM)iIndex, 0);
                }
                else
                {
                    // No encoding found! Bad error. Recover by selecting
                    // the first one.

                    SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_SETCURSEL,
                                       0, 0);
                }

            }
            SAFERELEASE(pEnumCodePage);
            SAFERELEASE(pMultiLanguage);
            break;
        }

#ifdef UNIX
        case WM_COMMAND:
        {
          switch (GET_WM_COMMAND_ID(wParam,lParam))
          {
            case IDOK:
            {
                 ThicketCPInfo *ptcpi = (ThicketCPInfo *)GetWindowLongPtr(hdlg,DWLP_USER);

                 ptcpi->cpDst = CP_ACP;
                 iCurSel = (int) SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET, CB_GETCURSEL, 0, 0);
                 ptcpi->cpDst =
                 (UINT)SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET, CB_GETITEMDATA,
                 (WPARAM)iCurSel, (LPARAM)0);

                 // To spare us from re-instantiating MLANG, we'll set the src and dest
                 // to CP_ACP if no change is indicated.
                 if (ptcpi->cpDst == ptcpi->cpSrc)
                    ptcpi->cpDst = ptcpi->cpSrc = CP_ACP;
           }
           break;
         }
        }
        break;
#endif /* UNIX */

        case WM_NOTIFY:
        {
            LPOFNOTIFY phdr = (LPOFNOTIFY)lParam;

            switch (phdr->hdr.code)
            {
                case CDN_FILEOK:
                {
                    LPOPENFILENAME pofn = (LPOPENFILENAME)phdr->lpOFN;
                    ThicketCPInfo *ptcpi = (ThicketCPInfo *)pofn->lCustData;

                    iCurSel = (int) SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET, CB_GETCURSEL, 0, 0);
                    ptcpi->cpDst = //*(UINT *)phdr->lpOFN->lCustData =
                        (UINT)SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET, CB_GETITEMDATA,
                                             (WPARAM)iCurSel, (LPARAM)0);
                }

                // HACK! This case is implemented to implement a hack for
                // IE5 RAID #60672. MIMEOLE cannot save UNICODE encoding,
                // so when the user selects MHTML saves, we should remove
                // this option.  This code should be removed when MIMEOLE
                // fixes their bug (targeted for NT5 RTM). Contact SBailey
                // for the status of this.

                case CDN_TYPECHANGE:
                {
                    LPOPENFILENAME pofn = (LPOPENFILENAME)phdr->lpOFN;
                    ThicketCPInfo *ptcpi = (ThicketCPInfo *)pofn->lCustData;
                    UINT uiCPSel, uiCP;
                    int iType = pofn->nFilterIndex;
                    UINT iCount;
                    int iCurSel;
                    int iSet = -1;

                    if (iType == PACKAGE_MHTML)
                    {
                        iCurSel = (int)SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_GETCURSEL, 0, 0);

                        uiCPSel = (UINT)SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET, CB_GETITEMDATA,
                                             (WPARAM)iCurSel, (LPARAM)0);
                         
                        // If you selected unicode, make it look like you
                        // really selected UTF-8

                        if (uiCPSel == CODEPAGE_UNICODE)
                        {
                            uiCPSel = CODEPAGE_UTF8;
                        }

                        i = (int) SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                                     CB_FINDSTRINGEXACT,
                                                     (WPARAM)0,
                                                     (LPARAM)UNICODE_TEXT);
                        if (i != CB_ERR)
                        {
                            SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                               CB_DELETESTRING, i, (LPARAM)0);
                        }

                        iCount = (int)SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                                         CB_GETCOUNT, 0, 0);

                        // Set selected item back

                        for (i = 0; i < iCount; i++)
                        {
                            uiCP = (UINT)SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_GETITEMDATA,
                                                            (WPARAM)i, (LPARAM)0);
                            if (uiCP == uiCPSel)
                            {
                                iSet = i;
                            }
                        }

                        if (iSet != 0xffffffff)
                        {
                            SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_SETCURSEL,
                                               (WPARAM)iSet, (LPARAM)0);
                        }
                    }
                    else
                    {
                        // Store current selection

                        iCurSel = (int)SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_GETCURSEL, 0, 0);

                        uiCPSel = (UINT)SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET, CB_GETITEMDATA,
                                             (WPARAM)iCurSel, (LPARAM)0);

                        // Add unicode back in, if it was removed

                        i = (int) SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                                     CB_FINDSTRINGEXACT,
                                                     (WPARAM)0,
                                                     (LPARAM)UNICODE_TEXT);

                        if (i == CB_ERR) {
                            // Unicode does not exist, add it back in
                            i = (int) SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                                         CB_ADDSTRING, 0,
                                                         (LPARAM)UNICODE_TEXT);
    
                            SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                               CB_SETITEMDATA, i,
                                               (LPARAM)CODEPAGE_UNICODE);

    
                            // Make sure the same encoding selected before is
                            // still selected.
                            iCount = (int)SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET,
                                                             CB_GETCOUNT, 0, 0);
                            for (i = 0; i < iCount; i++)
                            {
                                uiCP = (UINT)SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_GETITEMDATA,
                                                                (WPARAM)i, (LPARAM)0);
                                if (uiCP == uiCPSel)
                                {
                                    iSet = i;
                                }
                            }
    
                            if (iCurSel != 0xffffffff)
                            {
                                SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET, CB_SETCURSEL,
                                                   (WPARAM)iSet, (LPARAM)0);
                            }
                        }

                    }

                }
                break;
            }
        }
        break;

        case WM_HELP:
        {
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle,
                          c_szHelpFile,
                          HELP_WM_HELP,
                          (DWORD_PTR)aSaveAsHelpIDs);
        }
        break;

        case WM_CONTEXTMENU:
        {
            SHWinHelpOnDemandWrap((HWND) wParam,
                          c_szHelpFile, 
                          HELP_CONTEXTMENU,
                          (DWORD_PTR)aSaveAsHelpIDs);
        }
        break;
    }
    return (FALSE);
}

//
// Protect the naive users from themselves, if somebody enters a filename
// of microsoft.com when saving http://www.microsoft.com we don't want
// to save a .COM file since this will be interpreted as an executable.
// bad things will happen
//
void CleanUpFilename(LPTSTR pszFile, int iPackageStyle)
{
    //
    // If we find .COM as the file extension replace it with the file extension
    // of the filetype they are saving the file as
    //
    LPTSTR pszExt = PathFindExtension(pszFile);

    ASSERT(pszExt);
    if (StrCmpI(pszExt, TEXT(".COM")) == 0) // REVIEW any other file types???
    {
        //
        // Map the package style to a default extension. NOTE this relies on 
        // the fact that the filter index maps to the PACKAGE style enum
        // (as does the rest of the thicket code).
        //
        switch (iPackageStyle)
        {
        case PACKAGE_THICKET:
        case PACKAGE_HTML:
            StrCatBuff(pszFile, TEXT(".htm"), MAX_PATH); 
            break;

        case PACKAGE_MHTML:
            StrCatBuff(pszFile, TEXT(".mht"), MAX_PATH); 
            break;

        case PACKAGE_TEXT:
            StrCatBuff(pszFile, TEXT(".txt"), MAX_PATH); 
            break;

        default:
            ASSERT(FALSE);  // Unknown package type
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsGetFileName
//
//  Synopsis:   Gets a file name using either the GetOpenFileName or
//              GetSaveFileName functions.
//
//  Arguments:  [fSaveFile]   -- TRUE means use GetSaveFileName
//                               FALSE means use GetOpenFileName
//
//              [idFilterRes] -- The string resource specifying text in the
//                                  dialog box.  It must have the
//                                  following format:
//                            Note: the string has to be _one_ contiguous string.
//                                  The example is broken up to make it fit
//                                  on-screen. The verical bar ("pipe") characters
//                                  are changed to '\0'-s on the fly.
//                                  This allows the strings to be localized
//                                  using Espresso.
//
//          IDS_FILENAMERESOURCE, "Save Dialog As|         // the title
//                                 odg|                    // default extension
//                                 Forms3 Dialog (*.odg)|  // pairs of filter strings
//                                 *.odg|
//                                 Any File (*.*)|
//                                 *.*|"
//
//              [pstrFile]    -- Buffer for file name.
//              [cchFile]     -- Size of buffer in characters.
//
//  Modifies:   [pstrFile]
//
//----------------------------------------------------------------------------
#ifdef _MAC
extern "C" {
char * __cdecl _p2cstr(unsigned char *);
}
#endif

#define CHAR_DOT                TEXT('.')
#define CHAR_DOT_REPLACEMENT    TEXT('_')

void ReplaceDotsInFileName(LPTSTR pszFileName)
{
    ASSERT(pszFileName);

    while (*pszFileName)
    {
        if (*pszFileName == CHAR_DOT)
        {
            *pszFileName = CHAR_DOT_REPLACEMENT;
        }
        pszFileName++;
    }
}


HRESULT
FormsGetFileName(
        HWND hwndOwner,
        LPTSTR pstrFile,
        int cchFile,
        LPARAM lCustData,
        DWORD *pnFilterIndex,
        BOOL bForceHTMLOnly)
{
    HRESULT         hr  = S_OK;
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    LPTSTR          pstr;
    OPENFILENAME    ofn;
    TCHAR           achBuffer[4096];    //  Max. size of a string resource
    TCHAR *         cp;
    TCHAR *         pstrExt;
    int             cbBuffer;
    TCHAR           achPath[MAX_PATH];
    DWORD           dwType = REG_SZ;
    DWORD           cbData = MAX_PATH * sizeof(TCHAR);
    int             idFilterRes;


    // Initialize ofn struct
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hwndOwner;
    ofn.Flags           =   OFN_FILEMUSTEXIST   |
                            OFN_PATHMUSTEXIST   |
                            OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY    |
#ifndef UNIX
                            OFN_NOCHANGEDIR     |
                            OFN_EXPLORER;
#else
                            OFN_NOCHANGEDIR;
#endif /* UNIX */

    ofn.lpfnHook        = NULL;
    ofn.nMaxFile        = cchFile;
    ofn.lCustData       = lCustData;
    ofn.lpstrFile       = pstrFile;
#ifndef NO_IME
    // We add an extra control to the save file dialog.

    if (lCustData)
    {
        ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
        ofn.lpfnHook = SaveOFNHookProc;
        ofn.lpTemplateName = MAKEINTRESOURCE(IDD_ADDTOSAVE_DIALOG);
        ofn.hInstance = g_hinst;
    }

#endif

    //
    // Find the extension and set the filter index based on what the
    // extension is.  After these loops pstrExt will either be NULL if
    // we didn't find an extension, or will point to the extension starting
    // at the '.'

    pstrExt = pstrFile;
    while (*pstrExt)
        pstrExt++;
    while ( pstrExt >= pstrFile )
    {
        if( *pstrExt == TEXT('.') )
            break;
        pstrExt--;
    }
    if( pstrExt < pstrFile )
        pstrExt = NULL;

    // Load the filter spec.
    // BUGBUG: Convert table to stringtable for localization

    if ( SHRestricted2W( REST_NoBrowserSaveWebComplete, NULL, 0 ) )
        idFilterRes = IDS_NOTHICKET_SAVE;
    else if ( s_dwInetComVerMS != 0xFFFFFFFF )
    {
#ifndef UNIX
        if (s_dwInetComVerMS == 0)
        {
            TCHAR szPath[MAX_PATH];

            GetSystemDirectory( szPath, MAX_PATH );
            StrCatBuff( szPath, TEXT("\\INETCOMM.DLL"), MAX_PATH );
            if (FAILED(GetVersionFromFile(szPath, &s_dwInetComVerMS, &s_dwInetComVerLS)))
                s_dwInetComVerMS = 0xFFFFFFFF;
        }

        if (s_dwInetComVerMS >= 0x50000 && s_dwInetComVerMS != 0xFFFFFFFF)
            idFilterRes = IDS_THICKET_SAVE;
        else
            idFilterRes = IDS_NOMHTML_SAVE;
#else
        // on UNIX we don't have inetcomm.dll if oe is not installed
        {
           HINSTANCE hInetComm = NULL;

           if ((hInetComm = LoadLibrary(TEXT("INETCOMM.DLL"))))
           {
              idFilterRes = IDS_THICKET_SAVE;
              FreeLibrary(hInetComm);
           }
           else
              idFilterRes = IDS_NOMHTML_SAVE;
        }
#endif
    }
    else
        idFilterRes = IDS_THICKET_SAVE;

    cbBuffer = MLLoadShellLangString(idFilterRes, achBuffer, ARRAYSIZE(achBuffer));
    ASSERT(cbBuffer > 0);
    if ( ! cbBuffer )
        return E_FAIL;

    ofn.lpstrTitle = achBuffer;

    for ( cp = achBuffer; *cp; cp++ )
    {
        if ( *cp == TEXT('|') )
        {
            *cp = TEXT('\0');
        }
    }

    ASSERT(ofn.lpstrTitle);

    // Default extension is second string.
    pstr = (LPTSTR) ofn.lpstrTitle;
    while (*pstr++)
    {
    }

    // N.B. (johnv) Here we assume that filter index one corresponds with the default
    //  extension, otherwise we would have to introduce a default filter index into
    //  the resource string.
    ofn.nFilterIndex    = ((pnFilterIndex)? *pnFilterIndex : 1);
    ofn.lpstrDefExt     = pstr;

    // Filter is third string.
    while(*pstr++)
    {
    }

    ofn.lpstrFilter = pstr;

    // Try to match the extension with an entry in the filter list
    // If we match, remove the extension from the incoming path string,
    //   set the default extension to the one we found, and appropriately
    //   set the filter index.

    if (pstrExt && !bForceHTMLOnly)
    {
        // N.B. (johnv) We are searching more than we need to.

        int    iIndex = 0;
        const TCHAR* pSearch = ofn.lpstrFilter;

        while( pSearch )
        {
            if( StrStr( pSearch, pstrExt ) )
            {
                ofn.nFilterIndex = (iIndex / 2) + 1;
                ofn.lpstrDefExt = pstrExt + 1;

                // Remove the extension from the file name we pass in
                *pstrExt = TEXT('\0');

                break;
            }
            pSearch += lstrlen(pSearch);
            if( pSearch[1] == 0 )
                break;

            pSearch++;
            iIndex++;
        }
    }

    // Suggest HTML Only as default save-type

    if (bForceHTMLOnly)
    {
        // BUGBUG: These are hard-coded indices based on shdoclc.rc's
        // IDS_THICKET_SAVE, IDS_NOMHTML_SAVE, IDS_NOTHICKET_SAVE ordering.
        // This saves us the perf hit of doing string comparisons to find
        // HTML only

        switch (idFilterRes)
        {
            case IDS_NOTHICKET_SAVE:
                ofn.nFilterIndex = 1;
                break;

            case IDS_NOMHTML_SAVE:
                ofn.nFilterIndex = 2;
                break;

            default:
                ASSERT(idFilterRes == IDS_THICKET_SAVE);
                ofn.nFilterIndex = 3;
                break;
        }
    }

    if ( SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_MAIN, REGSTR_VAL_SAVEDIRECTORY,
                    &dwType, achPath, &cbData) != ERROR_SUCCESS ||
         !PathFileExists(achPath))
    {
        SHGetSpecialFolderPath(hwndOwner, achPath, CSIDL_PERSONAL, FALSE);
    }

    ofn.lpstrInitialDir = achPath;

    // We don't want to suggest dots in the filename
    ReplaceDotsInFileName(pstrFile);

    // Now, at last, we're ready to call the save file dialog
    fOK = GetSaveFileName(&ofn);

    // if working with the abbreviated format list, adjust the index
    if (idFilterRes == IDS_NOTHICKET_SAVE)
        ofn.nFilterIndex += 2;
    else if ( idFilterRes == IDS_NOMHTML_SAVE && ofn.nFilterIndex > 1 )
        ofn.nFilterIndex += 1;

    if (fOK)
    {
        //
        // Protect the naive users from themselves, if somebody enters a filename
        // of microsoft.com when saving http://www.microsoft.com we don't want
        // to save a .COM file since this will be interpreted as an executable.
        // bad things will happen
        //
        CleanUpFilename(pstrFile, ofn.nFilterIndex);

        TCHAR *lpszFileName;

        StrCpyN( achPath, pstrFile, ARRAYSIZE(achPath) );

        lpszFileName = PathFindFileName( achPath );
        *lpszFileName = 0;

        SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_MAIN, REGSTR_VAL_SAVEDIRECTORY,
                   REG_SZ, achPath, (lstrlen(achPath) * sizeof(TCHAR)));

        if (pnFilterIndex)
            *pnFilterIndex = ofn.nFilterIndex;

        if (ofn.nFilterIndex != PACKAGE_MHTML)
        {
            // we can only do this if we're not packaging MHTML
            // because MHTML requires that we tag with the explicit
            // charset, even if it was the default. unlike thicket
            // which inherits the charset from the original document,
            // MHTML must be explicitly tagged or else some system
            // charset tags will sneak in.

            ThicketCPInfo * ptcpi = (ThicketCPInfo *)lCustData;

            // To spare us from re-instantiating MLANG, we'll set the src and dest
            // to CP_ACP if no change is indicated.
            if (ptcpi->cpDst == ptcpi->cpSrc)
               ptcpi->cpDst = ptcpi->cpSrc = CP_ACP;
        }
    }
    else
    {
#ifndef WINCE
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
        }
        else
        {
            hr = S_FALSE;
        }
#else // WINCE
        hr = E_FAIL;
#endif // WINCE
    }

    return hr;
}

HRESULT
GetFileNameFromURL( LPWSTR pwszURL, LPTSTR pszFile, DWORD cchFile)
{
    HRESULT         hr = S_OK;
    PARSEDURLW      puw = {0};
    int             cchUrl;

    cchUrl = SysStringLen(pwszURL);

    if (cchUrl)
    {
        puw.cbSize = sizeof(PARSEDURLW);
        if (SUCCEEDED(ParseURLW(pwszURL, &puw)))
        {
            OLECHAR *pwchBookMark;
            DWORD   dwSize;
            INTERNET_CACHE_ENTRY_INFOW      ceiT;
            LPINTERNET_CACHE_ENTRY_INFOW    pcei = NULL;

            // Temporarily, null out the '#' in the url
            pwchBookMark = StrRChrW(puw.pszSuffix, NULL,'#');
            if (pwchBookMark)
            {
                *pwchBookMark = 0;
            }

            dwSize = sizeof(INTERNET_CACHE_ENTRY_INFO);
            if ( !GetUrlCacheEntryInfoW( pwszURL, &ceiT, &dwSize ) &&
                 GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                 (pcei = (LPINTERNET_CACHE_ENTRY_INFOW)new BYTE[dwSize]) != NULL &&
                 GetUrlCacheEntryInfoW( pwszURL, pcei, &dwSize ) )
            {
                StrCpyN(pszFile, PathFindFileName(pcei->lpszLocalFileName), cchFile);
                PathUndecorate(pszFile);
            }

            if(pcei)
                delete[] pcei;

            if (pwchBookMark)
                *pwchBookMark = '#';

            if ( !pszFile[0] )
            {
                OLECHAR *pwchQuery;
                TCHAR   szFileT[MAX_PATH];

                // Temporarily, null out the '?' in the url
                pwchQuery = StrRChrW(puw.pszSuffix, NULL,'?');
                if (pwchQuery)
                {
                    *pwchQuery = 0;
                }

                // IE5 bug 15055 - http://my.excite.com/?uid=B56E4E2D34DF3FED.save_uid
                // fails to save because we were passing "my.excite.com/" as the file
                // name to the file dialog. It doesn't like this.
                if (!pwchQuery || (pwchQuery[-1] != '/' && pwchQuery[-1] != '\\'))
                {
                    dwSize = ARRAYSIZE(szFileT);

                    StrCpyN(szFileT, PathFindFileName(puw.pszSuffix), dwSize);

                    if ( !InternetCanonicalizeUrl( szFileT, pszFile, &dwSize, ICU_DECODE | ICU_NO_ENCODE) )
                        StrCpyN(pszFile, szFileT, cchFile);

                    pszFile[cchFile - 1] = 0;
                }

                if (pwchQuery)
                    *pwchQuery = '?';
            }
        }
    }

    if (!pszFile[0])
    {
        MLLoadString(IDS_UNTITLED, pszFile, cchFile);
    }

    return hr;
}

INT_PTR CALLBACK SaveAsWarningDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                      LPARAM lParam)
{
    BOOL        fRet = FALSE;
    HRESULT     hr = S_OK;
    int         iFlags = 0;
    INT_PTR     bChecked = 0;

    switch (msg)
    {
        case WM_INITDIALOG:
            MessageBeep(MB_ICONEXCLAMATION);
            fRet = TRUE;

        case WM_COMMAND:
            bChecked = SendDlgItemMessage(hDlg, IDC_SAVEAS_WARNING_CB,
                                                  BM_GETCHECK, 0, 0 );
            iFlags = (bChecked) ? (SAVEAS_NEVER_ASK_AGAIN) : (0);

            switch (LOWORD(wParam))
            {
                case IDOK:
                    iFlags |= SAVEAS_OK;
                    // fall through

                case IDCANCEL:
                    EndDialog(hDlg, iFlags);
                    fRet = TRUE;
                    break;
            }
    
        default:
            fRet = FALSE;
    }

    return fRet;
}
