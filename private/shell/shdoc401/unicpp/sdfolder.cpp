#include "stdafx.h"
#pragma hdrstop

//#include "security.h"
//#include "dspsprt.h"
//#include "sdspatch.h"
//#include "dutil.h"

#define TF_SHELLAUTO            TF_CUSTOM1
#undef TF_SHDLIFE
#define TF_SHDLIFE              TF_CUSTOM2

HRESULT CSDFolder_Create(HWND hwnd, LPITEMIDLIST pidl, IShellFolder *psf, CSDFolder **ppsdf)
{
    *ppsdf = new CSDFolder(hwnd, psf);
    if (*ppsdf)
    {
        if (!(*ppsdf)->Init(pidl))
        {
            (*ppsdf)->Release();
            return E_OUTOFMEMORY;
        }

    	return S_OK;
    }

    return E_OUTOFMEMORY;
}

CSDFolder::CSDFolder(HWND hwnd, IShellFolder *psf) :
    m_cRef (1), m_hwnd(hwnd), m_pidl(NULL), m_psf(psf),
    CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_Folder)
{
    TraceMsg(TF_SHDLIFE, "ctor CSDFolder");

    if (m_psf)
        m_psf->AddRef();

    DllAddRef();
}

CSDFolder::~CSDFolder(void)
{
    TraceMsg(TF_SHDLIFE, "dtor CSDFolder");

    DllRelease();

    ATOMICRELEASE(m_psd);
    ATOMICRELEASE(m_psf);

    if (m_pidl)
        ILFree(m_pidl);

    // If we created an Application object release its site object...
    if (m_pidApp)
    {
        IUnknown_SetSite(SAFECAST(m_pidApp, IUnknown*), NULL);
        ATOMICRELEASE(m_pidApp);
    }

    return;
}

STDMETHODIMP CSDFolder::SetSite(IUnknown *punkSite)
{
    IUnknown_SetSite(SAFECAST(m_pidApp, IUnknown*), punkSite);
    return CObjectWithSite::SetSite(punkSite);
}

BOOL CSDFolder::Init(LPITEMIDLIST pidl)
{
    m_pidl = ILClone(pidl);
    if (m_pidl == NULL)
        return FALSE;

    return TRUE;
}

STDMETHODIMP CSDFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDFolder, Folder),
        QITABENTMULTI(CSDFolder, IDispatch, Folder),
        QITABENT(CSDFolder, ISDGetPidl),
        QITABENT(CSDFolder, IObjectSafety),
        QITABENT(CSDFolder, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDFolder::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDFolder::Release(void)
{
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//The Folder implementation
STDMETHODIMP CSDFolder::get_Application(IDispatch **ppid)
{
    // The Get application object takes care of security...
    HRESULT hres = S_OK;
    if (!m_pidApp)
        HRESULT hres = ::GetApplicationObject(_dwSafetyOptions, _punkSite, &m_pidApp);
    if (m_pidApp)
    {    
        *ppid = m_pidApp;
        m_pidApp->AddRef();
    }

    return hres;
}

STDMETHODIMP CSDFolder::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CSDFolder::get_ParentFolder (Folder **ppdf)
{
    *ppdf = NULL;   // assume error
    if (m_pidl->mkid.cb == 0)
        return S_FALSE;

    if (_dwSafetyOptions && LocalZoneCheck(_punkSite) != S_OK)
        return E_ACCESSDENIED;

    LPITEMIDLIST pidl = ILClone(m_pidl);
    if (pidl)
    {
        ILRemoveLastID(pidl);
        CSDFolder *psdf;
        HRESULT hres = CSDFolder_Create(m_hwnd, pidl, NULL, &psdf);
        if (SUCCEEDED(hres))
        {
            hres = psdf->QueryInterface(IID_Folder, (void **)ppdf);
            psdf->Release();
        }
        ILFree(pidl);
        return hres;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP CSDFolder::get_Title(BSTR *pbs)
{
    *pbs = NULL;

    SHFILEINFO sfi;
    if (SHGetFileInfo((LPCTSTR)m_pidl, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_PIDL))
        *pbs = AllocBStrFromString(sfi.szDisplayName);
    return NOERROR;
}

LPSHELLFOLDER CSDFolder::_GetShellFolder(void)
{
    if (!m_psf)
    {
        IShellFolder *psfRoot;

        if (FAILED(CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER,
            IID_IShellFolder, (void **)&psfRoot)))
            return FALSE;


        if (!m_pidl || (m_pidl->mkid.cb == 0))
        {
            m_psf = psfRoot;
            return m_psf;
        }

        if (FAILED(psfRoot->BindToObject(m_pidl, NULL, IID_IShellFolder, (void **)&m_psf)))
            m_psf = NULL;   // incase some does not set it right..

        psfRoot->Release();
    }

    return m_psf;
}

IShellDetails * CSDFolder::_GetShellDetails(void)
{
    if (!m_psd)
    {
        LPSHELLFOLDER psf = _GetShellFolder();
        if (psf)
        {   
            psf->CreateViewObject(m_hwnd, IID_IShellDetails, (void **)&m_psd);
        }
    }

    return m_psd;
}

STDMETHODIMP CSDFolder::Items(FolderItems **ppid)
{
    *ppid = NULL;
    HRESULT hres = NOERROR;
    if (_GetShellFolder())
    {
        // For now don't allow enumeration in Safe mode...
        if (_dwSafetyOptions && LocalZoneCheck(_punkSite) != S_OK)
            return E_ACCESSDENIED;
        hres = CSDFldrItems_Create(this, FALSE, ppid);
    }
    return hres;
}

STDMETHODIMP CSDFolder::ParseName(BSTR bName, FolderItem **ppid)
{
    *ppid = NULL;
    HRESULT hres = E_FAIL;

    // Aargh, lets be anal here and not allow them to do much...
    if (_dwSafetyOptions && LocalZoneCheck(_punkSite) != S_OK)
        return E_ACCESSDENIED;

    LPSHELLFOLDER psf = _GetShellFolder();
    if (psf)
    {   
        ULONG chEaten;
        LPITEMIDLIST pidl;
        hres = psf->ParseDisplayName(m_hwnd, NULL, bName, &chEaten, &pidl, NULL);
        if (SUCCEEDED(hres))
        {
            LPITEMIDLIST pidlLast = ILFindLastID(pidl);
            if (pidlLast == pidl)
            {
                if (FAILED(hres = CSDFldrItem_Create(this, pidl, ppid)))
                    *ppid = NULL;
            }
            else
            {

                CSDFolder *psdf;
                LPITEMIDLIST pidlParent = ILCombine(m_pidl, pidl);

                ILRemoveLastID(pidlParent);

                hres = CSDFolder_Create(m_hwnd, pidlParent, NULL, &psdf);
                if (SUCCEEDED(hres))
                {
                    if (FAILED(hres = CSDFldrItem_Create(psdf, pidlLast, ppid)))
                        *ppid = NULL;
                    psdf->Release();
                }
                ILFree(pidlParent);
            }
            ILFree(pidl);
        }
    }

    return hres;
}

STDMETHODIMP CSDFolder::NewFolder(BSTR bName, VARIANT vOptions)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSDFolder::MoveHere(VARIANT vItem, VARIANT vOptions)
{
    return _FileOperation(FO_MOVE, vItem, vOptions);
}

STDMETHODIMP CSDFolder::CopyHere(VARIANT vItem, VARIANT vOptions)
{
    return _FileOperation(FO_COPY, vItem, vOptions);
}

STDMETHODIMP CSDFolder::GetDetailsOf(VARIANT vItem, int iColumn, BSTR * pbs)
{
    HRESULT hres = E_FAIL;

    *pbs = NULL;    // assume emtpy

    if (iColumn == -1)
    {
        // They want the info tip for the item if any...
        LPCITEMIDLIST pidl = VariantToConstIDList(&vItem);
        if (pidl)
        {
            LPSHELLFOLDER psf = _GetShellFolder();
            if (psf)
            {
                TCHAR szInfoTip[1024];
                GetInfoTip(psf, pidl, szInfoTip, ARRAYSIZE(szInfoTip));

                *pbs = AllocBStrFromString(szInfoTip);
                hres = *pbs ? S_OK : E_OUTOFMEMORY;
            }
        }
        else
            hres = S_OK;    // NULL pidl for multiple selection, don't fail
    }
    else
    {
        IShellDetails* psd = _GetShellDetails();
        if (psd)
        {
            LPCITEMIDLIST pidl = VariantToConstIDList(&vItem);

            // NULL pidl is valid it implies get the header text
            SHELLDETAILS sd;
            hres = psd->GetDetailsOf(pidl, iColumn, &sd);
            if (SUCCEEDED(hres))
            {
                *pbs = StrRetToBStr(pidl, &sd.str);
                if (!*pbs)
                    hres = E_OUTOFMEMORY;
            }
            else
            {
                // BUGBUG: (kinda)  Current, ISD:GDO returns E_NOTIMPL if you ask for a column
                //  that doesn't exist.  WebView doesn't like this error, so we hide all errors
                goto RetNulStr;

            }
        }
        else
        {
RetNulStr:
            // failure case, return empty string (no details)
            *pbs = AllocBStrFromString(TEXT(""));
            hres = *pbs ? S_OK : E_OUTOFMEMORY;
        }
    }

    return hres;
}

// Main function to do Move or Copy
HRESULT CSDFolder::_FileOperation(UINT wFunc, VARIANT vItem, VARIANT vOptions)
{
    // If in Safe mode we fail this one...
    if (_dwSafetyOptions  && LocalZoneCheck(_punkSite) != S_OK)
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
                                SHUnicodeToTCharCP(CP_ACP, bs, pszT, MAX_PATH);
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
                        SHUnicodeToTCharCP(CP_ACP, bs, (LPTSTR)fileop.pFrom, cch);
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

STDMETHODIMP CSDFolder::GetPidl(LPITEMIDLIST *ppidl)
{
    *ppidl = ILClone(m_pidl);
    return *ppidl ? NOERROR : E_OUTOFMEMORY;
}

