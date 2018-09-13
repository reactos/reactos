//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// utils.cpp 
//
//   Misc routines.
//
//   History:
//
//       6/25/97  tnoonan   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "persist.h"
#include "cdfview.h"
#include "xmlutil.h"
#include "bindstcb.h"
#include "dll.h"
#include "resource.h"
#include "chanapi.h"

#include <mluisupp.h>

typedef struct _tagDialogData {
    LPCWSTR pszwURL;
    IXMLDocument* pIXMLDocument;
    int nProgress;
} DIALOGDATA;


HRESULT
GetURLFromIni(
    LPCTSTR pszPath, 
    BSTR* pbstrURL
)
{
    ASSERT(pszPath);
    ASSERT(pbstrURL);

    HRESULT hr = E_FAIL;

    LPTSTR szFile    = TSTR_INI_FILE;
    LPTSTR szSection = TSTR_INI_SECTION;
    LPTSTR szKey     = TSTR_INI_URL;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szPath[MAX_PATH];

    StrCpyN(szPath, pszPath, ARRAYSIZE(szPath) - ARRAYSIZE(TSTR_INI_FILE));
    StrCat(szPath, szFile);

    if (GetPrivateProfileString(szSection, szKey, TEXT(""), szURL,
                                INTERNET_MAX_URL_LENGTH, szPath))
    {
        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
        if (SHTCharToUnicode(szURL, wszURL, ARRAYSIZE(wszURL)))
        {
            *pbstrURL = SysAllocString(wszURL);
            hr = S_OK;
        }       
    }

    return hr;

}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Name ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
GetNameAndURLAndSubscriptionInfo(
    LPCTSTR pszPath,
    BSTR* pbstrName,
    BSTR* pbstrURL,
    SUBSCRIPTIONINFO* psi
)
{
    ASSERT(pszPath);
    ASSERT(pbstrName);
    ASSERT(pbstrURL);

    HRESULT hr;

    *pbstrName = NULL;
    *pbstrURL  = NULL;

    CCdfView* pCCdfView = new CCdfView;

    if (pCCdfView)
    {
        WCHAR wszPath[MAX_PATH];

        if (SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath)))
        {
            hr = pCCdfView->Load(wszPath, 0);

            if (SUCCEEDED(hr))
            {
                IXMLDocument* pIXMLDocument;

                hr = pCCdfView->ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL);

                if (SUCCEEDED(hr))
                {
                    ASSERT(pIXMLDocument);

                    IXMLElement* pIXMLElement;
                    LONG         nIndex;

                    hr = XML_GetFirstChannelElement(pIXMLDocument,
                                                    &pIXMLElement, &nIndex);

                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pIXMLElement);

                        *pbstrName = XML_GetAttribute(pIXMLElement, XML_TITLE);

                        if (*pbstrName && 0 == **pbstrName)
                        {
                            SysFreeString(*pbstrName);
                            *pbstrName = NULL;
                        }

                        *pbstrURL  = XML_GetAttribute(pIXMLElement, XML_SELF);

                        if (*pbstrURL && 0 == **pbstrURL)
                        {
                            SysFreeString(*pbstrName);
                            *pbstrURL = NULL;
                        }

                        if (psi)
                            XML_GetSubscriptionInfo(pIXMLElement, psi);

                        pIXMLElement->Release();
                    }

                    pIXMLDocument->Release();
                }
            }
        }

        if (NULL == *pbstrName)
        {
            TCHAR szName[MAX_PATH];
            WCHAR wszName[MAX_PATH];

            StrCpyN(szName, pszPath, ARRAYSIZE(szName));
            PathStripPath(szName);
           
            if (SHTCharToUnicode(szName, wszName, ARRAYSIZE(wszName)))
                *pbstrName = SysAllocString(wszName);
        }

        if (NULL == *pbstrURL)
        {
            GetURLFromIni(pszPath, pbstrURL);
        }

        hr = (NULL != *pbstrName) && (NULL != **pbstrName) &&
             (NULL != *pbstrURL) && (NULL != **pbstrURL) ? S_OK : E_FAIL;
             
        if (FAILED(hr))
        {
            if (NULL != *pbstrName)
            {
                SysFreeString(*pbstrName);
                *pbstrName = NULL;
            }
            if (NULL != *pbstrURL)
            {
                SysFreeString(*pbstrURL);
                *pbstrURL = NULL;
            }
        }
        pCCdfView->Release();
    }

    return hr;
}

int CDFMessageBox(HWND hwnd, UINT idTextFmt, UINT idCaption, UINT uType, ...)
{
    TCHAR szCaption[80];
    TCHAR szTextFmt[256];
    LPTSTR pszText;
    int result;
    va_list va;
    
    va_start(va, uType);

    MLLoadString(idTextFmt, szTextFmt, ARRAYSIZE(szTextFmt));
    MLLoadString(idCaption, szCaption, ARRAYSIZE(szCaption));

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                  szTextFmt, 0, 0, (LPTSTR)&pszText, 0, &va);

    result = MessageBox(hwnd, pszText, szCaption, uType);

    LocalFree((HLOCAL)pszText);

    return result;
}

INT_PTR RefreshDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = TRUE;
    DIALOGDATA* pdd = (DIALOGDATA*) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pdd = (DIALOGDATA*)lParam;

        IMoniker* pIMoniker;

        if (SUCCEEDED(CreateURLMoniker(NULL, pdd->pszwURL, &pIMoniker)))
        {
            ASSERT(pIMoniker);

            IBindCtx* pIBindCtx;

            IBindStatusCallback* pIBindStatusCallback =
                          (IBindStatusCallback*) new CBindStatusCallback2(hDlg);

            if (pIBindStatusCallback)
            {
                if (SUCCEEDED(CreateBindCtx(0, &pIBindCtx)) )
                {
                    ASSERT(pIBindCtx);

                    if (SUCCEEDED(RegisterBindStatusCallback(pIBindCtx,
                                                           pIBindStatusCallback,
                                                           NULL, 0)))
                    {
                        IPersistMoniker* pIPersistMoniker;

                        if (SUCCEEDED(pdd->pIXMLDocument->QueryInterface(
                                                           IID_IPersistMoniker,
                                                    (void**)&pIPersistMoniker)))
                        {
                            ASSERT(pIPersistMoniker);

                            pIPersistMoniker->Load(FALSE, pIMoniker, pIBindCtx,
                                                   0);

                            pIPersistMoniker->Release();
                        }
                    }

                    pIBindCtx->Release();
                }

                pIBindStatusCallback->Release();
            }

            pIMoniker->Release();
        }

        Animate_Open(GetDlgItem(hDlg, IDC_DOWNLOADANIMATE), IDA_DOWNLOAD);
        Animate_Play(GetDlgItem(hDlg, IDC_DOWNLOADANIMATE), 0, -1, -1);

        //SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), PBM_SETRANGE32, 0, 100);
        pdd->nProgress = 0;

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

        case DOWNLOAD_PROGRESS:
            SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), PBM_DELTAPOS,
                        pdd->nProgress += 2, 0);
            break;

        case DOWNLOAD_COMPLETE:
            if (lParam)
            {
                SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), PBM_SETPOS,
                            100, 0);

                XML_DownloadImages(pdd->pIXMLDocument);
            }
            EndDialog(hDlg, lParam);
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, FALSE);
        break;

    case WM_DESTROY:
        break;

    default:
        fRet = FALSE;
    }

    return fRet;
}


BOOL DownloadCdfUI(HWND hwnd, LPCWSTR pszwURL, IXMLDocument* pIXMLDocument)
{
    BOOL fRet = FALSE;

    DIALOGDATA dd;

    dd.pszwURL = pszwURL;
    dd.pIXMLDocument = pIXMLDocument;

    if (hwnd)
    {
        DWORD dwCacheCount = g_dwCacheCount;

        INT_PTR nRes = DialogBoxParam(MLGetHinst(),
                                    (LPWSTR)MAKEINTRESOURCE(IDD_CHANNELREFRESH),
                                    hwnd,
                                    RefreshDlgProc,
                                    (LPARAM)&dd);

        if (-1 == nRes)
        {
            int err = GetLastError();
        }
        else if (TRUE == nRes)
        {
            TCHAR szURL[INTERNET_MAX_URL_LENGTH];

            if (SHUnicodeToTChar(pszwURL, szURL, ARRAYSIZE(szURL)))
            {
                FILETIME ftLastMod;

                URLGetLastModTime(szURL, &ftLastMod);
                Cache_EnterWriteLock();

                Cache_RemoveItem(szURL);

                if (SUCCEEDED(Cache_AddItem(szURL, pIXMLDocument, PARSE_NET,
                                            ftLastMod, dwCacheCount)))
                    fRet = TRUE;

                Cache_LeaveWriteLock();
            }

            BSTR bstrSSUrl;

            if (SUCCEEDED(XML_GetScreenSaverURL(pIXMLDocument, &bstrSSUrl)))
            {
                Channel_WriteScreenSaverURL(pszwURL, bstrSSUrl);
                SysFreeString(bstrSSUrl);
            }

        }
    }

    return fRet;
}

//  Checks if global state is offline
//  Stolen from webcheck utils.cpp

BOOL IsGlobalOffline(void)
{
    DWORD   dwState = 0, 
            dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
        
    if (InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if (dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

void SetGlobalOffline(BOOL fOffline)
{
    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));

    if (fOffline)
    {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    }
    else 
    {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
}

//
// Can the given url be subscribed?
//

BOOL
CanSubscribe(
    LPCWSTR pwszURL
)
{
    ASSERT(pwszURL);

    BOOL fRet = FALSE;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];

    if (SHUnicodeToTChar(pwszURL, szURL, ARRAYSIZE(szURL)))
    {
        TCHAR szCanonicalURL[INTERNET_MAX_URL_LENGTH];
        DWORD dwSize = ARRAYSIZE(szCanonicalURL);

        URL_COMPONENTS uc = {0};
        uc.dwStructSize = sizeof(URL_COMPONENTS);

        if (InternetCanonicalizeUrl(szURL, szCanonicalURL, &dwSize, ICU_DECODE)
            &&
            InternetCrackUrl(szCanonicalURL, 0, 0, &uc) 
            &&
            ((INTERNET_SCHEME_HTTP == uc.nScheme) ||
             (INTERNET_SCHEME_HTTPS == uc.nScheme)))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}
