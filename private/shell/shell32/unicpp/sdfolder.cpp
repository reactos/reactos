#include "stdafx.h"
#pragma hdrstop

HRESULT CFolder_Create2(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, CFolder **ppsdf)
{
    HRESULT hr;
    CFolder *psdf = new CFolder();
    if (psdf)
    {
        hr = psdf->Init(hwnd, pidl, psf);
        if (SUCCEEDED(hr))
            *ppsdf = psdf;
        else
            psdf->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CFolder_Create(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, REFIID riid, void **ppv)
{
    *ppv = NULL;

    CFolder *psdf;
    HRESULT hr = CFolder_Create2(hwnd, pidl, psf, &psdf);
    if (SUCCEEDED(hr))
    {
        hr = psdf->QueryInterface(riid, ppv);
        psdf->Release();
    }
    return hr;
}

// HRESULT CFolder_Create(HWND hwnd, LPITEMIDLIST pidl, IShellFolder *psf, CFolder **ppsdf)

CFolder::CFolder() :
    m_cRef (1), m_pidl(NULL), m_psf(NULL), m_psf2(NULL),
    CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_Folder2)
{
    DWORD dwExStyle;

    _fmt = 0;
    // Be sure that the OS is supporting the flags DATE_LTRREADING and DATE_RTLREADING
    if (g_bBiDiPlatform)
    {
        // Get the date format reading order
        LCID locale = GetUserDefaultLCID();
        if (   (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC)
            || (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
        {
            //Get the real list view windows ExStyle.
            // [msadek]; we shouldn't check for either WS_EX_RTLREADING OR RTL_MIRRORED_WINDOW
            // on localized builds we have both of them to display dirve letters,..etc correctly
            // on enabled builds we have none of them. let's check on RTL_MIRRORED_WINDOW only
            
            dwExStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
            if (dwExStyle & RTL_MIRRORED_WINDOW)
                _fmt = LVCFMT_RIGHT_TO_LEFT;
            else
                _fmt = LVCFMT_LEFT_TO_RIGHT;
        }
    }

    DllAddRef();
}

CFolder::~CFolder(void)
{
    ATOMICRELEASE(m_psd);
    ATOMICRELEASE(m_psf2);
    ATOMICRELEASE(m_psf);

    if (m_pidl)
        ILFree(m_pidl);

    // If we created an Application object release its site object...
    if (m_pidApp)
    {
        IUnknown_SetSite(SAFECAST(m_pidApp, IUnknown*), NULL);
        ATOMICRELEASE(m_pidApp);
    }

    DllRelease();
}

STDMETHODIMP CFolder::SetSite(IUnknown *punkSite)
{
    IUnknown_SetSite(SAFECAST(m_pidApp, IUnknown*), punkSite);
    return CObjectWithSite::SetSite(punkSite);
}

HRESULT CFolder::Init(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf)
{
    m_hwnd = hwnd;

    HRESULT hr = SHILClone(pidl, &m_pidl);
    if (SUCCEEDED(hr))
    {
        m_psf = psf;
        if (m_psf)
            m_psf->AddRef();
        else
            hr = SHBindToObject(NULL, IID_IShellFolder, m_pidl, (void**)&m_psf);
        if (m_psf)
            m_psf->QueryInterface(IID_IShellFolder2, (void **)&m_psf2);
    }
    return hr;
}

STDMETHODIMP CFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolder, Folder2),
        QITABENTMULTI(CFolder, Folder, Folder2),
        QITABENTMULTI(CFolder, IDispatch, Folder2),
        QITABENTMULTI(CFolder, IPersist, IPersistFolder2),
        QITABENTMULTI(CFolder, IPersistFolder, IPersistFolder2),
        QITABENT(CFolder, IPersistFolder2),
        QITABENT(CFolder, IObjectSafety),
        QITABENT(CFolder, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolder::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CFolder::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

//The Folder implementation
STDMETHODIMP CFolder::get_Application(IDispatch **ppid)
{
    // The Get application object takes care of security...
    HRESULT hres = S_OK;
    if (!m_pidApp)
        hres = ::GetApplicationObject(_dwSafetyOptions, _punkSite, &m_pidApp);
    if (m_pidApp)
    {    
        *ppid = m_pidApp;
        m_pidApp->AddRef();
    }
    return hres;
}

STDMETHODIMP CFolder::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

// returns:
//      S_OK    - success
//      S_FALSE - failure, but not a script error
    
STDMETHODIMP CFolder::_ParentFolder(Folder **ppdf)
{
    *ppdf = NULL;   // assume error

    if (ILIsEmpty(m_pidl))
        return S_FALSE;     // automation compat, let script check error

    LPITEMIDLIST pidl;
    HRESULT hres = SHILClone(m_pidl, &pidl);
    if (SUCCEEDED(hres))
    {
        ILRemoveLastID(pidl);
        hres = CFolder_Create(m_hwnd, pidl, NULL, IID_Folder, (void **)ppdf);
        ILFree(pidl);
    }
    return hres;
}

STDMETHODIMP CFolder::get_ParentFolder(Folder **ppdf)
{
    *ppdf = NULL;   // assume error

    if (_dwSafetyOptions && IsSafePage(_punkSite) != S_OK)
        return E_ACCESSDENIED;

    return _ParentFolder(ppdf);
}

STDMETHODIMP CFolder::get_Title(BSTR *pbs)
{
    *pbs = NULL;

    SHFILEINFO sfi;
    if (SHGetFileInfo((LPCTSTR)m_pidl, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_PIDL))
        *pbs = SysAllocStringT(sfi.szDisplayName);
    return NOERROR;
}

IShellDetails * CFolder::_GetShellDetails(void)
{
    if (!m_psd)
    {
        if (m_psf)
            m_psf->CreateViewObject(m_hwnd, IID_IShellDetails, (void **)&m_psd);
    }
    return m_psd;
}

STDMETHODIMP CFolder::Items(FolderItems **ppid)
{
    HRESULT hres;

    hres = CFolderItems_Create(this, FALSE, ppid);

    if (_dwSafetyOptions && SUCCEEDED(hres))
        hres = MakeSafeForScripting((IUnknown**)ppid);

    return hres;
}

STDMETHODIMP CFolder::ParseName(BSTR bName, FolderItem **ppfi)
{
    *ppfi = NULL;
    HRESULT hres = E_FAIL;

    // Aargh, lets be anal here and not allow them to do much...
    if (_dwSafetyOptions && IsSafePage(_punkSite) != S_OK)
        return E_ACCESSDENIED;

    ULONG chEaten;
    LPITEMIDLIST pidl;
    hres = m_psf->ParseDisplayName(m_hwnd, NULL, bName, &chEaten, &pidl, NULL);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidlLast = ILFindLastID(pidl);
        if (pidlLast == pidl)
        {
            hres = CFolderItem_Create(this, pidl, ppfi);
        }
        else
        {
            LPITEMIDLIST pidlFull = ILCombine(m_pidl, pidl);
            if (pidlFull)
            {
                CFolderItem_CreateFromIDList(m_hwnd, pidlFull, ppfi);
                ILFree(pidlFull);
            }
            else
                hres = E_OUTOFMEMORY;
        }
        ILFree(pidl);
    }

    return hres;
}

STDMETHODIMP CFolder::NewFolder(BSTR bName, VARIANT vOptions)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolder::MoveHere(VARIANT vItem, VARIANT vOptions)
{
    return _FileOperation(FO_MOVE, vItem, vOptions);
}

STDMETHODIMP CFolder::CopyHere(VARIANT vItem, VARIANT vOptions)
{
    return _FileOperation(FO_COPY, vItem, vOptions);
}

// get the IDList for an item from a VARIANT that is a FolderItem dispatch object


STDMETHODIMP CFolder::GetDetailsOf(VARIANT vItem, int iColumn, BSTR *pbs)
{
    TCHAR szBuf[INFOTIPSIZE];

    szBuf[0] = 0;

    LPCITEMIDLIST pidl = CFolderItem::_GetIDListFromVariant(&vItem); // returns an ALIAS

    if (iColumn == -1)  // infotip for the item
    {
        if (pidl)
            GetInfoTip(m_psf, pidl, szBuf, ARRAYSIZE(szBuf));
    }
    else
    {
        BOOL bUseDetails;
        SHELLDETAILS sd;

        sd.fmt = _fmt;
        sd.str.uType = STRRET_CSTR;
        sd.str.cStr[0] = 0;

        if (m_psf2)
            bUseDetails = (E_NOTIMPL == m_psf2->GetDetailsOf(pidl, iColumn, &sd));
        else
            bUseDetails = TRUE;

        if (bUseDetails)
        {
            IShellDetails* psd = _GetShellDetails();
            if (psd)
                psd->GetDetailsOf(pidl, iColumn, &sd);
        }

        StrRetToBuf(&sd.str, pidl, szBuf, ARRAYSIZE(szBuf));
    }

    *pbs = SysAllocStringT(szBuf);
    return *pbs ? NOERROR : E_OUTOFMEMORY;
}

HRESULT CFolder::get_Self(FolderItem **ppfi)
{
    Folder *psdf;
    HRESULT hres;
    
    if (ILIsEmpty(m_pidl))
    {
        psdf = this;
        psdf->AddRef();
        hres = S_OK;
    }
    else
        hres = _ParentFolder(&psdf);
        
    if (SUCCEEDED(hres))
    {
        hres = CFolderItem_Create((CFolder*)psdf, ILFindLastID(m_pidl), ppfi);
        if (SUCCEEDED(hres) && _dwSafetyOptions)
            hres = MakeSafeForScripting((IUnknown**)ppfi);
        psdf->Release();
    }
    else
        *ppfi = NULL;
    return hres;
}

BOOL _VerifyUNC(LPTSTR psz, ULONG cch)
{
    if (PathIsUNC(psz))
    {
        return TRUE;
    }
    else if (psz[1] == TEXT(':'))
    {
        TCHAR szLocalName[3] = { psz[0], psz[1], TEXT('\0') };

        // Call GetDriveType before WNetGetConnection, to avoid loading
        // MPR.DLL unless absolutely necessary.
        if (DRIVE_REMOTE == GetDriveType(szLocalName) &&
            NOERROR == WNetGetConnection(szLocalName, psz, &cch))
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT GetSharePath(LPCITEMIDLIST pidl, LPTSTR psz, ULONG cch)
{
    HRESULT hres = E_FAIL;
    
    if (SHGetPathFromIDList(pidl, psz))
    {
        if (_VerifyUNC(psz, cch))
            hres = S_OK;
        else 
        {
            //  check for folder shortcuts.
            IShellFolder *psf;
            if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (void **)&psf)))
            {
                IShellLink *psl;
                if (SUCCEEDED(psf->QueryInterface(IID_IShellLink, (void **)&psl)))
                {
                    if (SUCCEEDED(psl->GetPath(psz, cch, NULL, 0))
                    &&  _VerifyUNC(psz, cch))
                        hres = S_OK;
                    psl->Release();
                }
                psf->Release();
            }
        }
        if (SUCCEEDED(hres))
            PathStripToRoot(psz);
    }       

    return hres;
}

#include <cscuiext.h>

STDMETHODIMP CFolder::get_OfflineStatus(LONG *pul)
{
    TCHAR szShare[MAX_PATH];

    *pul = OFS_INACTIVE;  // default

    // Make sure we have a UNC \\server\share path.  Do this before
    // checking whether CSC is enabled, to avoid loading CSCDLL.DLL
    // unless absolutely necessary.
    if (SUCCEEDED(GetSharePath(m_pidl, szShare, ARRAYSIZE(szShare))))
    {
        *pul = GetOfflineShareStatus(szShare);
    }
    return S_OK;
}

STDMETHODIMP CFolder::Synchronize(void)
{
    HWND hwndCSCUI = FindWindow(STR_CSCHIDDENWND_CLASSNAME, STR_CSCHIDDENWND_TITLE);
    if (hwndCSCUI)
        PostMessage(hwndCSCUI, CSCWM_SYNCHRONIZE, 0, 0);
    return S_OK;
}

struct
{
    int csidlFolder;
    BOOL bHaveToShowWebViewBarricade;
} g_WebViewBarricadeFolderStatus[] = {  {CSIDL_WINDOWS, true},
                                        {CSIDL_SYSTEM, true},
                                        {CSIDL_SYSTEMX86, true},
                                        {CSIDL_PROGRAM_FILES, true},
                                        {CSIDL_PROGRAM_FILESX86, true}
                                     };

STDMETHODIMP CFolder::get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade)
{
    if (pbHaveToShowWebViewBarricade)
    {
        *pbHaveToShowWebViewBarricade = VARIANT_FALSE;
        BOOL bExitLoop = false;
        for (int i = 0; (i < ARRAYSIZE(g_WebViewBarricadeFolderStatus)) && !bExitLoop; i++)
        {
            LPITEMIDLIST pidlTemp = NULL;
            if (SUCCEEDED(SHGetFolderLocation(NULL, g_WebViewBarricadeFolderStatus[i].csidlFolder, NULL, 0, &pidlTemp)))
            {
                if (ILIsEqual(m_pidl, pidlTemp))
                {
                    // Careful!  VARIANT_TRUE != TRUE
                    *pbHaveToShowWebViewBarricade = g_WebViewBarricadeFolderStatus[i].bHaveToShowWebViewBarricade ? VARIANT_TRUE : VARIANT_FALSE;
                    bExitLoop = true;
                }
                ILFree(pidlTemp);
            }
        }
    }
    return S_OK;
}

STDMETHODIMP CFolder::DismissedWebViewBarricade()
{
    BOOL bExitLoop = false;
    for (int i = 0; (i < ARRAYSIZE(g_WebViewBarricadeFolderStatus)) && !bExitLoop; i++)
    {
        LPITEMIDLIST pidlTemp = NULL;
        if (SUCCEEDED(SHGetFolderLocation(NULL, g_WebViewBarricadeFolderStatus[i].csidlFolder, NULL, 0, &pidlTemp)))
        {
            if (ILIsEqual(m_pidl, pidlTemp))
            {
                g_WebViewBarricadeFolderStatus[i].bHaveToShowWebViewBarricade = false;
                bExitLoop = true;
            }
            ILFree(pidlTemp);
        }
    }
    return S_OK;
}

// Main function to do Move or Copy
HRESULT CFolder::_FileOperation(UINT wFunc, VARIANT vItem, VARIANT vOptions)
{
    // If in Safe mode we fail this one...
    if (_dwSafetyOptions  && IsSafePage(_punkSite) != S_OK)
        return E_ACCESSDENIED;

    // BUGBUG:: Not using options yet...
    SHFILEOPSTRUCT fileop = {m_hwnd, wFunc};
    int cch;

    if (vItem.vt == (VT_BYREF | VT_VARIANT) && vItem.pvarVal)
         vItem = *vItem.pvarVal;

     // We need to get the source files out of the variant.
     // Currently support string, or IDispatch (Either FolderItem or FolderItems)
    switch (vItem.vt)
    {
    case VT_BSTR:
        fileop.pFrom = (LPTSTR)LocalAlloc(LPTR, cch = (lstrlenW(vItem.bstrVal)+2) * sizeof(TCHAR));   // +2 for double null
        if (fileop.pFrom)
            SHUnicodeToTChar(vItem.bstrVal, (LPTSTR)fileop.pFrom, cch);
        break;
    case VT_DISPATCH:
        {
            BSTR bs;
            FolderItem *pfi;
            FolderItems *pfis;

            if (!vItem.pdispVal)
                break;

            if (SUCCEEDED(vItem.pdispVal->QueryInterface(IID_FolderItems, (void **)&pfis)))
            {
                // This is gross, but allocate N times MAX_PATH for buffer as to keep from
                // looping through the items twice.
                long cItems;

                pfis->get_Count(&cItems);
                fileop.pFrom = (LPTSTR)LocalAlloc(LPTR, ((cItems * MAX_PATH) + 1) * sizeof(TCHAR));
                if (fileop.pFrom)
                {
                    long i; 
                    VARIANT v = {VT_I4};
                    LPTSTR pszT = (LPTSTR)fileop.pFrom;
                    for (i = 0; i < cItems; i++)
                    {
                        v.lVal = i;
                        if (SUCCEEDED(pfis->Item(v, &pfi)))
                        {
                            if (SUCCEEDED(pfi->get_Path(&bs)))
                            {
                                cch = lstrlenW(bs);
                                SHUnicodeToTChar(bs, pszT, MAX_PATH);
                                SysFreeString(bs);
                                pszT += cch + 1;
                            }
            
                            pfi->Release();
                        }
                    }
                }

                pfis->Release();

                break;
            }
            else if (SUCCEEDED(vItem.pdispVal->QueryInterface(IID_FolderItem, (void **)&pfi)))
            {
                if (SUCCEEDED(pfi->get_Path(&bs)))
                {
                    fileop.pFrom = (LPTSTR)LocalAlloc(LPTR, cch = (lstrlenW(bs)+2) * sizeof(TCHAR));
                    if (fileop.pFrom)
                        SHUnicodeToTChar(bs, (LPTSTR)fileop.pFrom, cch);
                    SysFreeString(bs);
                }

                pfi->Release();
                break;
            }
        }
        break;
    default:
        return E_INVALIDARG;   // don't support that type of variable.
    }

    if (!fileop.pFrom)
         return E_OUTOFMEMORY;

    // Now setup the Destination...
    TCHAR szDest[MAX_PATH];
    fileop.pTo = szDest;
    SHGetPathFromIDList(m_pidl, szDest);

    // Allow flags to pass through...

    if (vOptions.vt == (VT_BYREF | VT_VARIANT) && vOptions.pvarVal)
         vOptions = *vOptions.pvarVal;

     // We need to get the source files out of the variant.
     // Currently support string, or IDispatch (Either FolderItem or FolderItems)
    switch (vOptions.vt)
    {
    case VT_I2:
        fileop.fFlags = (FILEOP_FLAGS)vOptions.iVal;
        break;
        // And fall through...

    case VT_I4:
        fileop.fFlags = (FILEOP_FLAGS)vOptions.lVal;
        break;
    }

    // Finally lets try to do the operation.
    int ret = SHFileOperation(&fileop);
    LocalFree((HLOCAL)fileop.pFrom);

    return ret ? HRESULT_FROM_WIN32(ret) : NOERROR;
}

STDMETHODIMP CFolder::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolder::Initialize(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return SHILClone(m_pidl, ppidl);
}

