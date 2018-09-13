//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// persist.cpp 
//
//   IPersistFolder for the cdfview class.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "persist.h"
#include "xmlutil.h"
#include "cdfview.h"
#include "bindstcb.h"
#include "chanapi.h"
#include "resource.h"
#include <winineti.h>  // MAX_CACHE_ENTRY_INFO_SIZE
#include "dll.h"
#define _SHDOCVW_
#include <shdocvw.h>

#include <mluisupp.h>

//
// Constructor and destructor

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::CPersist ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CPersist::CPersist(
    void
)
: m_bCdfParsed(FALSE)
{
    ASSERT(0 == *m_szPath);
    ASSERT(NULL == m_polestrURL);
    ASSERT(NULL == m_pIWebBrowser2);
    ASSERT(NULL == m_hwnd);
    ASSERT(NULL == m_pIXMLDocument);
    ASSERT(FALSE == m_fPendingNavigation);
    ASSERT(IT_UNKNOWN == m_rgInitType);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::CPersist ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CPersist::CPersist(
    BOOL bCdfParsed
)
: m_bCdfParsed(bCdfParsed)
{
    ASSERT(0 == *m_szPath);
    ASSERT(NULL == m_polestrURL);
    ASSERT(NULL == m_pIWebBrowser2);
    ASSERT(NULL == m_hwnd);
    ASSERT(NULL == m_pIXMLDocument);
    ASSERT(FALSE == m_fPendingNavigation);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::CPersist ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CPersist::~CPersist(
    void
)
{
    if (m_fPendingNavigation && m_pIWebBrowser2 && m_pIXMLDocument)
    {
    }

    if (m_polestrURL)
        CoTaskMemFree(m_polestrURL);

    if (m_pIWebBrowser2)
        m_pIWebBrowser2->Release();

    if (m_pIXMLDocument)
        m_pIXMLDocument->Release();

    return;
}


//
// IPersist methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::GetClassID ***
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
STDMETHODIMP
CPersist::GetClassID(
    LPCLSID lpClassID
)
{
    ASSERT(lpClassID);

    //
    // REVIEW:  Two possible class IDs CLSID_CDFVIEW & CLSID_CDF_INI
    //

    *lpClassID = CLSID_CDFVIEW;

    return S_OK;
}


//
// IPersistFile methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::IsDirty ***
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
STDMETHODIMP
CPersist::IsDirty(
    void
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::Load ***
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
STDMETHODIMP
CPersist::Load(
    LPCOLESTR pszFileName,
    DWORD dwMode
)
{
    ASSERT(pszFileName);

    HRESULT hr;

    if (SHUnicodeToTChar(pszFileName, m_szPath, ARRAYSIZE(m_szPath)))
    {
        hr = S_OK;

        QuickCheckInitType();
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

void CPersist::QuickCheckInitType( void )
{
    // if the path is a directory then
    // it has to be a Shellfolder we were initialised for.
    // we are calculating this here so that we can avoid hitting the disk
    // in GetInitType if at all possible.

    if (PathIsDirectory(m_szPath))
    {
        m_rgInitType = IT_INI;
    }
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::Save ***
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
STDMETHODIMP
CPersist::Save(
    LPCOLESTR pszFileName,
    BOOL fRemember
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::SaveCompleted ***
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
STDMETHODIMP
CPersist::SaveCompleted(
    LPCOLESTR pszFileName
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::GetCurFile ***
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
STDMETHODIMP
CPersist::GetCurFile(
    LPOLESTR* ppszFileName
)
{
    return E_NOTIMPL;
}


//
// IPersistFolder methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::Initialize ***
//
//
// Description:
//     This function is called with the fully qualified id list (location) of
//     the selected cdf file.
//
// Parameters:
//     [In]  pidl - The pidl of the selected cdf file.  This pidl conatins the
//                  full path to the CDF.
//
// Return:
//     S_OK if content for the cdf file could be created.
//     E_OUTOFMEMORY otherwise.
//
// Comments:
//     This function can be called more than once for a given folder.  When a
//     CDFView is being instantiated from a desktop.ini file the shell calls
//     Initialize once before it calls GetUIObjectOf asking for IDropTarget.
//     After the GetUIObjectOf call the folder is Released.  It then calls
//     Initialize again on a new folder.  This time it keeps the folder and it
//     ends up being displayed.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPersist::Initialize(
    LPCITEMIDLIST pidl
)
{
    ASSERT(pidl);
    ASSERT(0 == *m_szPath);
    HRESULT hr = SHGetPathFromIDList(pidl, m_szPath) ? S_OK : E_FAIL;

    QuickCheckInitType();
    
    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::Parse ***
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
CPersist::Parse(
    LPTSTR szURL,
    IXMLDocument** ppIXMLDocument
)
{
    ASSERT(szURL);

    HRESULT hr;

    DLL_ForcePreloadDlls(PRELOAD_MSXML);
    
    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLDocument, (void**)ppIXMLDocument);

    BOOL bCoInit = FALSE;

    if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoInit = TRUE;
        hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                              IID_IXMLDocument, (void**)ppIXMLDocument);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(*ppIXMLDocument);

        hr = XML_SynchronousParse(*ppIXMLDocument, szURL);
    }

    if (bCoInit)
        CoUninitialize();

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::ParseCdf ***
//
//
// Description:
//     Parses the cdf file associated with this folder.
//
// Parameters:
//     [In]  hwndOwner      - The parent window of any dialogs that need to be
//                            displayed.
//     [Out] ppIXMLDocument - A pointer that receives the xml document.
//
// Return:
//     S_OK if the cdf file was found and successfully parsed.
//     E_FAIL otherwise.
//
// Comments:
//     Uses the m_pidlRoot that was set during IPersistFolder::Initialize.
//     
////////////////////////////////////////////////////////////////////////////////
HRESULT
CPersist::ParseCdf(
    HWND hwndOwner,
    IXMLDocument** ppIXMLDocument,
    DWORD dwParseFlags
)
{
    ASSERT(ppIXMLDocument);

    HRESULT hr;

    if (*m_szPath)
    {
        INITTYPE it = GetInitType(m_szPath);

        switch(it)
        {
        case IT_FILE:
            hr = InitializeFromURL(m_szPath, ppIXMLDocument, dwParseFlags);
            break;

        case IT_INI:
            {
                TCHAR szURL[INTERNET_MAX_URL_LENGTH];

                if (ReadFromIni(TSTR_INI_URL, szURL, ARRAYSIZE(szURL)))
                {
                    hr = InitializeFromURL(szURL, ppIXMLDocument, dwParseFlags);
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            break;

        case IT_SHORTCUT:
        case IT_UNKNOWN:
            hr = E_FAIL;
            break;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // REVIEW: Properly notify user on failure to init.
    //

    if (FAILED(hr) && hwndOwner)
    {
        TCHAR szText[MAX_PATH];
        TCHAR szTitle[MAX_PATH];

        MLLoadString(IDS_ERROR_DLG_TEXT,  szText, MAX_PATH); 
        MLLoadString(IDS_ERROR_DLG_TITLE, szTitle, MAX_PATH);

        MessageBox(hwndOwner, szText, szTitle, MB_OK | MB_ICONWARNING); 
    }

    ASSERT((SUCCEEDED(hr) && *ppIXMLDocument) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::GetInitType ***
//
//
// Description:
//     Determines the method being used to designate the cdf file.
//
// Parameters:
//     [In]  szPath - The path passed in to IPersistFolder::Initialize.
//
// Return:
//     IT_INI if this instance is being created from a desktop.ini file
//     located in a right protected directory.
//     IT_FILE if this instance is being created from opening a cdf file.
//     IT_UNKNOWN if the method can not be determined.
//
// Comments:
//      
//
////////////////////////////////////////////////////////////////////////////////
INITTYPE
CPersist::GetInitType(
    LPTSTR szPath
)
{
    if ( m_rgInitType != IT_UNKNOWN )
    {
        return m_rgInitType;
    }
    
    ASSERT(szPath);

    INITTYPE itRet;

    if (PathIsDirectory(szPath))
    {
        itRet = IT_INI;
    }
    else
    {
        itRet = IT_FILE;
    }

    m_rgInitType = itRet;
    
    return itRet;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::InitializeFromURL ***
//
//
// Description:
//     Given an URL to a cdf an attempt is made to parse the cdf and initialize
//     the current (root) folder.
//
// Parameters:
//     [In]  szURL          - The URL of the cdf file.
//     [Out] ppIXMLDocument - A pointer that receives the xml document.
//
// Return:
//     S_OK if initializtion succeeded.
//     E_FAIL otherwise.
//
// Comments:
//     All other initialize methods eventually resolve to an URL and call this
//     methhod.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CPersist::InitializeFromURL(
    LPTSTR pszURL,
    IXMLDocument** ppIXMLDocument,
    DWORD dwParseFlags
)
{
    ASSERT(pszURL);
    ASSERT(ppIXMLDocument);

    HRESULT hr;

    TCHAR szCanonicalURL[INTERNET_MAX_URL_LENGTH];

    if (PathIsURL(pszURL))
    {
        ULONG cch = ARRAYSIZE(szCanonicalURL);

        if (InternetCanonicalizeUrl(pszURL, szCanonicalURL, &cch, 0))
            pszURL = szCanonicalURL;
    }

    //
    // Get an XML document object from the cache if it's there.  Otherwise
    // parse it and place it in the cache.
    //

    if (PARSE_REPARSE & dwParseFlags)
    {
        (void)Cache_RemoveItem(pszURL); 
        hr = E_FAIL;
    }
    else
    {
        hr = Cache_QueryItem(pszURL, ppIXMLDocument, dwParseFlags);

        if (SUCCEEDED(hr))
            TraceMsg(TF_CDFPARSE, "[XML Document Cache]"); 
    }

    if (FAILED(hr))
    {
        DWORD    dwCacheCount = g_dwCacheCount;
        FILETIME ftLastMod;

        if (dwParseFlags & PARSE_LOCAL)
        {
            TCHAR szLocalFile[MAX_PATH];

            hr = URLGetLocalFileName(pszURL, szLocalFile,
                                     ARRAYSIZE(szLocalFile), &ftLastMod);

            if (SUCCEEDED(hr))
            {
                hr = Parse(szLocalFile, ppIXMLDocument);
            }
            else
            {
                hr = OLE_E_NOCACHE;
            }
        }
        else
        {
            TraceMsg(TF_CDFPARSE, "[*** CDF parse enabled to hit net!!! ***]");

            hr = Parse(pszURL, ppIXMLDocument);


            URLGetLastModTime(pszURL, &ftLastMod);

            //
            // Stuff the images files into the cache.
            //

            if (SUCCEEDED(hr))
            {
                ASSERT(*ppIXMLDocument);

                XML_DownloadImages(*ppIXMLDocument);
            }
        }

        if (SUCCEEDED(hr))
        {
            Cache_AddItem(pszURL, *ppIXMLDocument, dwParseFlags, ftLastMod,
                          dwCacheCount);
        }
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(*ppIXMLDocument);

        m_bCdfParsed = TRUE;

        if (dwParseFlags & PARSE_REMOVEGLEAM)
            ClearGleamFlag(pszURL, m_szPath);
    }
    
    ASSERT((SUCCEEDED(hr) && m_bCdfParsed && *ppIXMLDocument) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::ReadFromIni ***
//
//
// Description:
//     Reads a string from the channel desktop.ini file.
//
// Parameters:
//     pszKey - The key to read.
//     szOut  - The result.
//     cch    - The size of the szout Buffer
//
// Return:
//     A bstr containing the value associated with the key.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CPersist::ReadFromIni(
    LPCTSTR pszKey,
    LPTSTR  szOut,
    int     cch
)
{
    ASSERT(pszKey);
    ASSERT(szOut || 0 == cch);

    BOOL fRet = FALSE;

    if (m_szPath && *m_szPath)
    {
        INITTYPE it = GetInitType(m_szPath);

        if (it == IT_INI)
        {
            LPCTSTR szFile    = TSTR_INI_FILE;
            LPCTSTR szSection = TSTR_INI_SECTION;
            LPCTSTR szKey     = pszKey;
            TCHAR   szPath[MAX_PATH];

            StrCpyN(szPath, m_szPath, ARRAYSIZE(szPath) - StrLen(szFile));
            StrCat(szPath, szFile);

            if (GetPrivateProfileString(szSection, szKey, TEXT(""), szOut, cch,
                                        szPath))
            {
                fRet = TRUE;
            }
        }
    }

    return fRet;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::ReadFromIni ***
//
//
// Description:
//     Reads a string from the channel desktop.ini file.
//
// Parameters:
//     pszKey - The key to read.
//
// Return:
//     A bstr containing the value associated with the key.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BSTR
CPersist::ReadFromIni(
    LPCTSTR pszKey
)
{
    ASSERT(pszKey);

    BSTR bstrRet = NULL;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];

    if (ReadFromIni(pszKey, szURL, ARRAYSIZE(szURL)))
    {
        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];

        if (SHTCharToUnicode(szURL, wszURL, ARRAYSIZE(wszURL)))
            bstrRet = SysAllocString(wszURL);
    }

    return bstrRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::IsUnreadCdf ***
//
//
// Description:
//
// Parameters:
//
// Return:
//
// Comments:
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CPersist::IsUnreadCdf(
    void
)
{
    BOOL fRet = FALSE;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];

    if (ReadFromIni(TSTR_INI_URL, szURL, ARRAYSIZE(szURL)))
    {
        fRet = IsRecentlyChangedURL(szURL);
    }

    return fRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CPersist::IsNewContent ***
//
//
// Description:
//
// Parameters:
//
// Return:
//
// Comments:
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CPersist::IsRecentlyChangedURL(
    LPCTSTR pszURL
)
{
    ASSERT(pszURL);

    BOOL fRet = FALSE;

    HRESULT hr;
    IPropertySetStorage* pIPropertySetStorage;

    hr = QueryInternetShortcut(pszURL, IID_IPropertySetStorage,
                               (void**)&pIPropertySetStorage);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIPropertySetStorage);

        IPropertyStorage* pIPropertyStorage;

        hr = pIPropertySetStorage->Open(FMTID_InternetSite, STGM_READWRITE,
                                        &pIPropertyStorage);
        
        if (SUCCEEDED(hr))
        {
            ASSERT(pIPropertyStorage);

            PROPSPEC propspec = { PRSPEC_PROPID, PID_INTSITE_FLAGS };
            PROPVARIANT propvar;

            PropVariantInit(&propvar);

            hr = pIPropertyStorage->ReadMultiple(1, &propspec, &propvar);

            if (SUCCEEDED(hr) && (VT_UI4 == propvar.vt))
            {
                fRet = propvar.ulVal & PIDISF_RECENTLYCHANGED;
            }
            else
            {
                PropVariantClear(&propvar);
            }

            pIPropertyStorage->Release();
        }

        pIPropertySetStorage->Release();
    }

    return fRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** ClearGleamFlag ***
//
//
// Description:
//
// Parameters:
//
// Return:
//
// Comments:
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
ClearGleamFlag(
    LPCTSTR pszURL,
    LPCTSTR pszPath
)
{
    ASSERT(pszURL);
    ASSERT(pszPath);

    HRESULT hr;

    IPropertySetStorage* pIPropertySetStorage;

    hr = QueryInternetShortcut(pszURL, IID_IPropertySetStorage,
                               (void**)&pIPropertySetStorage);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIPropertySetStorage);

        IPropertyStorage* pIPropertyStorage;

        hr = pIPropertySetStorage->Open(FMTID_InternetSite, STGM_READWRITE,
                                        &pIPropertyStorage);
        
        if (SUCCEEDED(hr))
        {
            ASSERT(pIPropertyStorage);

            PROPSPEC propspec = { PRSPEC_PROPID, PID_INTSITE_FLAGS };
            PROPVARIANT propvar;

            PropVariantInit(&propvar);

            hr = pIPropertyStorage->ReadMultiple(1, &propspec, &propvar);

            if (SUCCEEDED(hr) && (VT_UI4 == propvar.vt) &&
                (propvar.ulVal & PIDISF_RECENTLYCHANGED))
            {
                TCHAR  szHash[MAX_PATH];
                int   iIndex;
                UINT  uFlags;
                int   iImageIndex;
                
                HRESULT hr2 = PreUpdateChannelImage(pszPath, szHash, &iIndex,
                                                    &uFlags, &iImageIndex);

                propvar.ulVal &=  ~PIDISF_RECENTLYCHANGED;

                hr = pIPropertyStorage->WriteMultiple(1, &propspec, &propvar,
                                                      0);
                if (SUCCEEDED(hr))
                    hr = pIPropertyStorage->Commit(STGC_DEFAULT);

                TraceMsg(TF_GLEAM, "- Gleam Cleared %s", pszURL);

                if (SUCCEEDED(hr) && SUCCEEDED(hr2))
                {
                    WCHAR wszHash[MAX_PATH];
                    SHTCharToUnicode(szHash, wszHash, ARRAYSIZE(wszHash));

                    UpdateChannelImage(wszHash, iIndex, uFlags, iImageIndex);
                }

            }
            else
            {
                PropVariantClear(&propvar);
            }

            pIPropertyStorage->Release();
        }

        pIPropertySetStorage->Release();
    }

    return hr;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** URLGetLocalFileName ***
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
URLGetLocalFileName(
    LPCTSTR pszURL,
    LPTSTR szLocalFile,
    int cch,
    FILETIME* pftLastMod
)
{
    ASSERT(pszURL);
    ASSERT(szLocalFile || 0 == cch);

    HRESULT hr = E_FAIL;

    if (pftLastMod)
    {
        pftLastMod->dwLowDateTime  = 0;
        pftLastMod->dwHighDateTime = 0;
    }

    // by using the internal shlwapi function, we avoid loading WININET 
    // unless we really really need it...
    if (PathIsURL(pszURL))
    {
        PARSEDURL rgCrackedURL = {0};

        rgCrackedURL.cbSize = sizeof( rgCrackedURL );
        
        if ( SUCCEEDED( ParseURL( pszURL, &rgCrackedURL )))
        {
            switch(rgCrackedURL.nScheme)
            {
            case URL_SCHEME_HTTP:
            case URL_SCHEME_FTP:
            case URL_SCHEME_GOPHER:
                {
                    ULONG cbSize  = MAX_CACHE_ENTRY_INFO_SIZE;

                    INTERNET_CACHE_ENTRY_INFO* piceiAlloced = 
                    (INTERNET_CACHE_ENTRY_INFO*) new BYTE[cbSize];

                    if (piceiAlloced)
                    {
                        piceiAlloced->dwStructSize =
                                          sizeof(INTERNET_CACHE_ENTRY_INFO);

                        if (GetUrlCacheEntryInfoEx(pszURL, piceiAlloced,
                                                   &cbSize, NULL, NULL,
                                                   NULL, 0))
                        {
                            if (StrCpyN(szLocalFile,
                                        piceiAlloced->lpszLocalFileName, cch))
                            {
                                if (pftLastMod)
                                {
                                    *pftLastMod =
                                                 piceiAlloced->LastModifiedTime;
                                }

                                hr = S_OK;
                            }
                        }

                        delete [] piceiAlloced;
                    }
                }
                break;

            case URL_SCHEME_FILE:
                hr = PathCreateFromUrl(pszURL, szLocalFile, (LPDWORD)&cch, 0);
                break;

            }

        }
    }
    else
    {
        if (StrCpyN(szLocalFile, pszURL, cch))
            hr = S_OK;
    }

    return hr;
}

//
//  Get the last modified time of the URL.
//

HRESULT
URLGetLastModTime(
    LPCTSTR pszURL,
    FILETIME* pftLastMod
)
{
    ASSERT(pszURL);
    ASSERT(pftLastMod);

    pftLastMod->dwLowDateTime  = 0;
    pftLastMod->dwHighDateTime = 0;

    ULONG cbSize  = 0;

    if (!GetUrlCacheEntryInfoEx(pszURL, NULL, &cbSize, NULL, NULL, NULL, 0)
        && cbSize > 0)
    {
        INTERNET_CACHE_ENTRY_INFO* piceiAlloced =
                                  (INTERNET_CACHE_ENTRY_INFO*) new BYTE[cbSize];

        if (piceiAlloced)
        {
            piceiAlloced->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);

            if (GetUrlCacheEntryInfoEx(pszURL, piceiAlloced, &cbSize, NULL,
                                       NULL, NULL, 0))
            {
                *pftLastMod = piceiAlloced->LastModifiedTime;
            }

            delete [] piceiAlloced;
        }
    }

    return S_OK;
}

/*STDMETHODIMP
CPersist::IsDirty(
    void
)
{
    return E_NOTIMPL;
}*/

STDMETHODIMP
CPersist::Load(
    BOOL fFullyAvailable,
    IMoniker* pIMoniker,
    IBindCtx* pIBindCtx,
    DWORD grfMode
)
{
    ASSERT(pIMoniker);
    ASSERT(pIBindCtx);

    HRESULT hr;

    ASSERT(NULL == m_polestrURL);

    hr = pIMoniker->GetDisplayName(pIBindCtx, NULL, &m_polestrURL);

    if (SUCCEEDED(hr))
    {
        ASSERT(m_polestrURL);

        ASSERT(NULL == m_pIXMLDocument)

        DLL_ForcePreloadDlls(PRELOAD_MSXML);
        
        hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                              IID_IXMLDocument, (void**)&m_pIXMLDocument);

        if (SUCCEEDED(hr))
        {
            ASSERT(m_pIXMLDocument);

            CBindStatusCallback* pCBindStatusCallback = new CBindStatusCallback(
                                                                m_pIXMLDocument,
                                                                m_polestrURL);

            if (pCBindStatusCallback)
            {
                IBindStatusCallback* pPrevIBindStatusCallback;

                hr = RegisterBindStatusCallback(pIBindCtx,
                                     (IBindStatusCallback*)pCBindStatusCallback,
                                     &pPrevIBindStatusCallback, 0);

                if (SUCCEEDED(hr))
                {
                    pCBindStatusCallback->Init(pPrevIBindStatusCallback);

                    IPersistMoniker* pIPersistMoniker;

                    hr = m_pIXMLDocument->QueryInterface(IID_IPersistMoniker,
                                                     (void**)&pIPersistMoniker);

                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pIPersistMoniker);

                        hr = pIPersistMoniker->Load(fFullyAvailable, pIMoniker,
                                                    pIBindCtx, grfMode);
                        pIPersistMoniker->Release();
                    }
                }

                pCBindStatusCallback->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

STDMETHODIMP
CPersist::Save(
    IMoniker* pIMoniker,
    IBindCtx* pIBindCtx,
    BOOL fRemember
)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CPersist::SaveCompleted(
    IMoniker* pIMoniker,
    IBindCtx* pIBindCtx
)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CPersist::GetCurMoniker(
    IMoniker** ppIMoniker
)
{
    return E_NOTIMPL;
}
