#include "precomp.hxx"
#pragma hdrstop

#include <shguidp.h>    // CLSID_MyDocuments, CLSID_ShellFSFolder
#include <shellp.h>     // SHCoCreateInstance
#include <shlguidp.h>   // IID_IResolveShellLink
#include "util.h"
#include "dll.h"
#include "resource.h"
#include "prop.h"

enum calling_app_type {
    APP_IS_UNKNOWN = 0,
    APP_IS_NORMAL,
    APP_IS_OFFICE
    };


class CMyDocsFolderLinkResolver : public IResolveShellLink
{
private:
    LONG m_cRef;
public:
    CMyDocsFolderLinkResolver() : m_cRef(1) { DllAddRef(); };
    ~CMyDocsFolderLinkResolver() { DllRelease(); };

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IResolveShellLink
    STDMETHOD(ResolveShellLink)(IUnknown* punk, HWND hwnd, DWORD fFlags);
};

STDMETHODIMP CMyDocsFolderLinkResolver::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsFolderLinkResolver, IResolveShellLink),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CMyDocsFolderLinkResolver::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsFolderLinkResolver::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CMyDocsFolderLinkResolver::ResolveShellLink(IUnknown* punk, HWND hwnd, DWORD fFlags)
{
    // No action needed to resolve a link to the mydocs folder:
    return S_OK;
}


// shell folder implementation for icon on the desktop. the purpouse of this object is
//      1) to give access to MyDocs high up in the name space 
//         this makes it easier for end users to get to MyDocs
//      2) allow for end user custimization of the real MyDocs folder
//         through the provided property page on this icon

// NOTE: this object agregates the file system folder so we get away with a minimal set of interfaces
// on this object. the real file system folder does stuff like IPersistFolder2 for us

class CMyDocsFolder : public IPersistFolder,
                      public IShellFolder2,
                      public IShellIconOverlay
{
private:

    friend HRESULT CMyDocuments_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    LONG                 m_cRef;

    IUnknown *           m_punk;        // points to IUnknown for shell folder in use...
    IShellFolder *       m_psf;         // points to shell folder in use...
    IShellFolder2 *      m_psf2;        // points to shell folder in use...
    IShellIconOverlay*   m_psio;        // points to shell folder in use...
    LPITEMIDLIST         m_pidl;        // copy of pidl passed to us in Initialize()
    calling_app_type     m_host;

    HRESULT RealInitialize(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlBindTo, LPTSTR pRootPath);
    calling_app_type _WhoIsCalling();

    CMyDocsFolder();
    ~CMyDocsFolder();

    HRESULT _GetFolder();
    HRESULT _GetFolder2();
    HRESULT _GetShellIconOverlay();
    void _FreeFolder();

    HRESULT _PathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath);
    HRESULT _PathToIDList(LPCTSTR pszPath, LPITEMIDLIST *ppidl);

    HRESULT _GetFolderOverlayInfo(int *pIndex, BOOL fIconIndex);

public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName,
                                ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIDList);
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwnd, REFIID riid, void **ppv);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHOD(SetNameOf)(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST* ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid);

    // IPersist, IPersistFreeThreadedObject
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // IPersistFolder2, IPersistFolder3, etc are all implemented by 
    // the folder we agregate

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex);
};

HRESULT CMyDocuments_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CMyDocsFolder* pMDF = new CMyDocsFolder();
    if (pMDF)
    {
        *ppunk = SAFECAST(pMDF, IShellFolder *);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

CMyDocsFolder::CMyDocsFolder() :
    m_cRef          (1),
    m_host          (APP_IS_UNKNOWN),
    m_psf           (NULL),
    m_psf2          (NULL),
    m_psio          (NULL),
    m_punk          (NULL),
    m_pidl          (NULL)
{
    DllAddRef();
}

CMyDocsFolder::~CMyDocsFolder()
{
    m_cRef = 1000;  // deal with agregation re-enter

    _FreeFolder();

    ILFree(m_pidl);

    DllRelease();
}

STDMETHODIMP CMyDocsFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CMyDocsFolder, IShellFolder, IShellFolder2),
        QITABENT(CMyDocsFolder, IShellFolder2),
        QITABENTMULTI(CMyDocsFolder, IPersist, IPersistFolder),
        QITABENT(CMyDocsFolder, IPersistFolder),
        QITABENT(CMyDocsFolder, IShellIconOverlay),
        // QITABENTMULTI2(CMyDocsFolder, IID_IPersistFreeThreadedObject, IPersist),           // IID_IPersistFreeThreadedObject
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && m_punk)
        hr = m_punk->QueryInterface(riid, ppv); // agregated guy
    return hr;
}

STDMETHODIMP_ (ULONG) CMyDocsFolder::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsFolder::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


// Determine who is calling us so that we can do app specific
// compatibility hacks when needed

calling_app_type CMyDocsFolder::_WhoIsCalling()
{
    // Check to see if we have the value already...
    if (m_host == APP_IS_UNKNOWN)
    {
        if (SHGetAppCompatFlags (ACF_APPISOFFICE) & ACF_APPISOFFICE)
        {
            m_host = APP_IS_OFFICE;
        } else {
            m_host = APP_IS_NORMAL;
        }
    }
    return m_host;
}

// IPersist methods
STDMETHODIMP CMyDocsFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_MyDocuments;
    return S_OK;
}

HRESULT _BindToIDListParent(LPCITEMIDLIST pidl, LPBC pbc, IShellFolder **ppsf, LPITEMIDLIST *ppidlLast)
{
    HRESULT hr;
    LPITEMIDLIST pidlTemp = ILClone(pidl);
    if (pidlTemp)
    {
        IShellFolder *psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlLast = ILFindLastID(pidlTemp);

            pidlLast->mkid.cb = 0;

            hr = psfDesktop->BindToObject(pidlTemp, pbc, IID_PPV_ARG(IShellFolder, ppsf));
            if (SUCCEEDED(hr))
            {
                if (ppidlLast)
                    *ppidlLast = ILFindLastID(pidl);
            }
            psfDesktop->Release();
        }
        ILFree(pidlTemp);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT _GetMyDocsPath(LPTSTR pszPath)
{
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, pszPath);
    if (S_FALSE == hr)
        hr = E_FAIL;    // empty converted to failure
    return hr;
}

HRESULT _ConfirmMyDocsPath(HWND hwnd)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = SHGetFolderPath(hwnd, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szPath);
    if (S_OK != hr)
    {
        TCHAR szTitle[MAX_PATH];

        // above failed, get unverified path
        SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);

        LPCTSTR pszMsg = PathIsNetworkPath(szPath) ? MAKEINTRESOURCE(IDS_CANT_FIND_MYDOCS_NET) :
                                                     MAKEINTRESOURCE(IDS_CANT_FIND_MYDOCS);

        PathCompactPath(NULL, szPath, 400);

        GetMyDocumentsDisplayName(szTitle, ARRAYSIZE(szTitle));

        ShellMessageBox(g_hInstance, hwnd, pszMsg, szTitle,
                        MB_OK | MB_ICONSTOP, szPath, szTitle);

        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user saw the message
    } else if (hr == S_FALSE)
        hr = E_FAIL;
    return hr;
}


// like SHGetPathFromIDList() except this uses the bind context to make sure
// we don't get into loops since there can be cases where there are multiple 
// instances of this folder that can cause binding loops.

HRESULT CMyDocsFolder::_PathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    *pszPath = 0;

    LPBC pbc;
    HRESULT hr = CreateBindCtx(NULL, &pbc);
    if (SUCCEEDED(hr))
    {
        // this bind context skips extension taged with our CLSID
        hr = pbc->RegisterObjectParam(STR_SKIP_BINDING_CLSID, SAFECAST(this, IShellFolder *));
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlLast;
            IShellFolder *psf;
            hr = _BindToIDListParent(pidl, pbc, &psf, &pidlLast);
            if (SUCCEEDED(hr))
            {
                STRRET strret;
                hr = psf->GetDisplayNameOf(pidlLast, SHGDN_FORPARSING, &strret);
                if (SUCCEEDED(hr))
                {
                    StrRetToBuf(&strret, pidlLast, pszPath, MAX_PATH);
                }
                psf->Release();
            }
        }
        pbc->Release();
    }
    return hr;
}

HRESULT CMyDocsFolder::_PathToIDList(LPCTSTR pszPath, LPITEMIDLIST *ppidl)
{
    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        LPBC pbc;
        hr = CreateBindCtx( 0, &pbc );
        if (SUCCEEDED(hr))
        {
            BIND_OPTS bo = {sizeof(bo), 0};
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;   // skip all junctions

            hr = pbc->SetBindOptions( &bo );
            if (SUCCEEDED(hr))
            {
                WCHAR szPath[MAX_PATH];
                SHTCharToUnicode(pszPath, szPath, ARRAYSIZE(szPath));

                hr = psfDesktop->ParseDisplayName(NULL, pbc, szPath, NULL, ppidl, NULL);
            }
            pbc->Release();
        }
        psfDesktop->Release();
    }
    return hr;
}

void CMyDocsFolder::_FreeFolder()
{
    if (m_punk)
    {
        SHReleaseInnerInterface((IShellFolder *)this, (IUnknown **)&m_psf);
        SHReleaseInnerInterface((IShellFolder *)this, (IUnknown **)&m_psf2);
        SHReleaseInnerInterface((IShellFolder *)this, (IUnknown **)&m_psio);
        m_punk->Release();
        m_punk = NULL;
    }
}

// verify that m_psf (agregated file system folder) has been inited

HRESULT CMyDocsFolder::_GetFolder()
{
    HRESULT hr;

    if (m_psf)
    {
        hr = S_OK;
    }
    else
    {
        hr = SHQueryInnerInterface((IShellFolder *)this, m_punk, IID_PPV_ARG(IShellFolder, &m_psf));
        if (SUCCEEDED(hr))
        {
            IPersistFolder3 *ppf3;
            hr = SHQueryInnerInterface((IShellFolder *)this, m_punk, IID_PPV_ARG(IPersistFolder3, &ppf3));
            if (SUCCEEDED(hr))
            {
                PERSIST_FOLDER_TARGET_INFO pfti = {0};
        
                pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
                pfti.csidl = CSIDL_PERSONAL | CSIDL_FLAG_PFTI_TRACKTARGET;

                hr = ppf3->InitializeEx(NULL, m_pidl, &pfti);
                SHReleaseInnerInterface((IShellFolder *)this, (IUnknown **)&ppf3);
            }
        }
    }
    return hr;
}

HRESULT CMyDocsFolder::_GetFolder2()
{
    HRESULT hr;
    if (m_psf2)
        hr = S_OK;
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
            hr = SHQueryInnerInterface((IShellFolder *)this, m_punk, IID_PPV_ARG(IShellFolder2, &m_psf2));
    }
    return hr;
}

HRESULT CMyDocsFolder::_GetShellIconOverlay()
{
    HRESULT hr;
    if (m_psio)
    {
        hr = S_OK;
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
        {
            hr = SHQueryInnerInterface((IShellFolder*)this, m_punk, IID_PPV_ARG(IShellIconOverlay, &m_psio));
        }
    }
    return hr;
}

// returns:
//      S_OK    -- goodness
//      S_FALSE freed the pidl, set to empty
//      E_OUTOFMEMORY

HRESULT _SetIDList(LPITEMIDLIST* ppidl, LPCITEMIDLIST pidl)
{
    HRESULT hr;

    if (*ppidl) 
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    if (pidl)
    {
        *ppidl = ILClone(pidl);
        hr = *ppidl ? S_OK : E_OUTOFMEMORY;
    }
    else
        hr = S_FALSE;   // success by empty

    return hr;
}

// IPersistFolder
HRESULT CMyDocsFolder::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hr;
    if (IsMyDocsIDList(pidl))
    {
        hr = _SetIDList(&m_pidl, pidl);
        if (SUCCEEDED(hr))
        {
            // agregate a file system folder object early so we can
            // delegate QI() to him that we don't implement
            hr = SHCoCreateInstance(NULL, &CLSID_ShellFSFolder, (IShellFolder *)this, IID_PPV_ARG(IUnknown, &m_punk));
        }
    }
    else
    {
        TCHAR szPathInit[MAX_PATH], szMyDocs[MAX_PATH];

        // we are being inited by some folder other than the one on the
        // desktop (from the old mydocs desktop.ini). if this the current users
        // MyDocs we will untag it now so we don't get called on this anymore

        SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szMyDocs);

        if (SUCCEEDED(_PathFromIDList(pidl, szPathInit)) &&
            lstrcmpi(szPathInit, szMyDocs) == 0)
        {
            MyDocsUnmakeSystemFolder(szMyDocs);
        }
        hr = E_FAIL;    // don't init on the file system folder anymore
    }
    return hr;
}

STDMETHODIMP CMyDocsFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName, 
                                             ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->ParseDisplayName(hwnd, pbc, pDisplayName, pchEaten, ppidl, pdwAttributes);
    return hr;
}

STDMETHODIMP CMyDocsFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIdList)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->EnumObjects(hwnd, grfFlags, ppEnumIdList);
    return hr;
}

STDMETHODIMP CMyDocsFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->BindToObject(pidl, pbc, riid, ppv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->BindToStorage(pidl, pbc, riid, ppv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->CompareIDs(lParam, pidl1, pidl2);
    return hr;
}

STDMETHODIMP CMyDocsFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hr;
    if (riid == IID_IResolveShellLink)
    {
        // No work needed to resolve a link to the mydocs folder, because it is a virtual
        // folder whose location is always tracked by the shell, so return our implementation
        // of IResolveShellLink - which does nothing when Resolve() is called
        CMyDocsFolderLinkResolver* pslr = new CMyDocsFolderLinkResolver;
        if (pslr)
        {
            hr = pslr->QueryInterface(riid, ppv);
            pslr->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (riid == IID_IShellLinkA || 
             riid == IID_IShellLinkW)
    {
        LPITEMIDLIST pidl;
        hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl);
        if (S_OK == hr)
        {
            IShellLink *psl;
            hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLink, &psl));
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIDList(pidl);
                if (SUCCEEDED(hr))
                {
                    hr = psl->QueryInterface(riid, ppv);
                }
                psl->Release();
            }
            ILFree(pidl);
        }
        else
            hr = E_FAIL;
    }
    else
    {
        if (IID_IShellView == riid)
        {
            // Since we aren't notified when the user renames the mydocs folder,
            // we need to update the sendto file more often then we would like to
            // (every time a we open a view of the MyDocs folder)
            UpdateSendToFile(FALSE);
        }

        hr = _GetFolder();
        if (SUCCEEDED(hr))
        {
            if (hwnd && (IID_IShellView == riid))
                hr = _ConfirmMyDocsPath(hwnd);

            if (SUCCEEDED(hr))
                hr = m_psf->CreateViewObject(hwnd, riid, ppv);
        }
    }

    return hr;
}


DWORD _GetRealMyDocsAttributes(DWORD dwAttributes)
{
    DWORD dwRet = 0;
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl);
    if (S_OK == hr)
    {
        IShellFolder *psf;
        LPITEMIDLIST pidlLast;
        hr = _BindToIDListParent(pidl, NULL, &psf, &pidlLast);
        if (SUCCEEDED(hr))
        {
            dwRet = dwAttributes;
            hr = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlLast, &dwRet);
            dwRet &= dwAttributes;

            psf->Release();
        }
        ILFree(pidl);
    }
    return dwRet;
}

BOOL _IsSelf(UINT cidl, LPCITEMIDLIST *apidl)
{
    return cidl == 0 || (cidl == 1 && (apidl == NULL || apidl[0] == NULL || ILIsEmpty(apidl[0])));
}

// these are the attributes from the real mydocs folder that we want to merge
// in with the desktop icons attributes

#define SFGAO_ATTRIBS_MERGE    (SFGAO_SHARE)

STDMETHODIMP CMyDocsFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut)
{
    HRESULT hr;
    if (_IsSelf(cidl, apidl))
    {
        DWORD dwRequested = *rgfInOut;

        *rgfInOut = MyDocsGetAttributes();
        
        if (dwRequested & SFGAO_ATTRIBS_MERGE)
            *rgfInOut |= _GetRealMyDocsAttributes(SFGAO_ATTRIBS_MERGE);

        // RegItem "CallForAttributes" gets us here...
        switch(_WhoIsCalling())
        {
        case APP_IS_OFFICE:
            *rgfInOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER | 
                           SFGAO_HASPROPSHEET | SFGAO_NONENUMERATED);
            break;
        }
        hr = S_OK;
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
            hr = m_psf->GetAttributesOf(cidl, apidl, rgfInOut);
    }
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *aidl, 
                                          REFIID riid, UINT *pRes, void **ppv)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->GetUIObjectOf(hwnd, cidl, aidl, riid, pRes, ppv);
    return hr;
}

HRESULT StrRetFromString(STRRET *pstrret, LPCTSTR pString)
{
#ifdef UNICODE
    HRESULT hr = SHStrDup(pString, &pstrret->pOleStr);
    if (SUCCEEDED(hr))
    {
        pstrret->uType = STRRET_WSTR;
    }
    return hr;
#else
    pstrret->uType = STRRET_CSTR;
    lstrcpyn(pstrret->cStr, pString, ARRAYSIZE(pstrret->cStr));
    return S_OK;
#endif
}

STDMETHODIMP CMyDocsFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName)
{
    HRESULT hr;
    if (_IsSelf(1, &pidl))
    {
        TCHAR szMyDocsPath[MAX_PATH];
        hr = _GetMyDocsPath(szMyDocsPath);
        if (SUCCEEDED(hr))
        {
            // RegItems "WantsFORPARSING" gets us here. allows us to control our parsing name
            LPTSTR pPath = ((uFlags & SHGDN_INFOLDER) ? PathFindFileName(szMyDocsPath) : szMyDocsPath);
            hr = StrRetFromString(pName, pPath);
        }
    }
    else
    {
        hr = _GetFolder();
        if (SUCCEEDED(hr))
            hr = m_psf->GetDisplayNameOf(pidl, uFlags, pName);
    }
    return hr;
}

STDMETHODIMP CMyDocsFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pName, DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = m_psf->SetNameOf(hwnd, pidl, pName, uFlags, ppidlOut);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDefaultSearchGUID(LPGUID lpGuid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->GetDefaultSearchGUID(lpGuid);
    return hr;
}

STDMETHODIMP CMyDocsFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->EnumSearches(ppenum);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->GetDefaultColumn(dwRes, pSort, pDisplay);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{    
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->GetDefaultColumnState(iColumn, pbState);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->GetDetailsEx(pidl, pscid, pv);
    return hr;
}

STDMETHODIMP CMyDocsFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetail)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->GetDetailsOf(pidl, iColumn, pDetail);
    return hr;
}

STDMETHODIMP CMyDocsFolder::MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = m_psf2->MapColumnToSCID(iCol, pscid);
    return hr;
}

HRESULT CMyDocsFolder::_GetFolderOverlayInfo(int *pIndex, BOOL fIconIndex)
{
    HRESULT hr;

    if (pIndex)
    {
        LPITEMIDLIST pidl;
        hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl);
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            LPITEMIDLIST pidlLast;
            hr = _BindToIDListParent(pidl, NULL, &psf, &pidlLast);
            if (SUCCEEDED(hr))
            {
                IShellIconOverlay* psio;
                hr = psf->QueryInterface(IID_PPV_ARG(IShellIconOverlay, &psio));
                if (SUCCEEDED(hr))
                {
                    if (fIconIndex)
                        hr = psio->GetOverlayIconIndex(pidlLast, pIndex);
                    else
                        hr = psio->GetOverlayIndex(pidlLast, pIndex);

                    psio->Release();
                }

                psf->Release();
            }

            ILFree(pidl);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CMyDocsFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;

    if (_IsSelf(1, &pidl))
    {
        if (pIndex && *pIndex == OI_ASYNC)
            hr = E_PENDING;
        else
            hr = _GetFolderOverlayInfo(pIndex, FALSE);
    }
    else
    {
        // forward to aggregated dude
        if (SUCCEEDED(_GetShellIconOverlay()))
        {
            hr = m_psio->GetOverlayIndex(pidl, pIndex);
        }
    }

    return hr;
}

STDMETHODIMP CMyDocsFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    HRESULT hr = E_FAIL;

    if (_IsSelf(1, &pidl))
    {
        hr = _GetFolderOverlayInfo(pIconIndex, TRUE);
    }
    else
    {
        // forward to aggregated dude
        if (SUCCEEDED(_GetShellIconOverlay()))
        {
            hr = m_psio->GetOverlayIconIndex(pidl, pIconIndex);
        }
    }

    return hr;
}

HRESULT _GetUIObjectForMyDocs(REFIID riid, void **ppv)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl);
    if (S_OK == hr)
    {
        IShellFolder *psf;
        LPITEMIDLIST pidlLast;
        hr = _BindToIDListParent(pidl, NULL, &psf, &pidlLast);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidlLast, riid, NULL, ppv);
            psf->Release();
        }
        ILFree(pidl);
    }
    else if (S_FALSE == hr)
        hr = E_FAIL;
    return hr;
}


// send to "My Documents" handler

class CMyDocsDropTarget : public IDropTarget, IPersistFile
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFile
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
    STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName);

private:
    CMyDocsDropTarget();
    ~CMyDocsDropTarget();
    HRESULT _InitTarget();
    friend HRESULT CMyDocsDropTarget_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    LONG m_cRef;
    IDropTarget *_pdtgt;
};

CMyDocsDropTarget::CMyDocsDropTarget() : m_cRef(1)
{
    DllAddRef();
}

CMyDocsDropTarget::~CMyDocsDropTarget()
{
    if (_pdtgt)
        _pdtgt->Release();
    DllRelease();
}

STDMETHODIMP CMyDocsDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsDropTarget, IDropTarget),
        QITABENT(CMyDocsDropTarget, IPersistFile), 
        QITABENTMULTI(CMyDocsDropTarget, IPersist, IPersistFile),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMyDocsDropTarget::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CMyDocsDropTarget::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;
    delete this;
    return 0;
}

HRESULT _CreateViewObjectForMyDocs(REFIID riid, void **ppv)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl);
    if (S_OK == hr)
    {
        IShellFolder *psf;
        hr = SHBindToObject(NULL, IID_IShellFolder, pidl, (void **)&psf);
        if (SUCCEEDED(hr))
        {
            hr = psf->CreateViewObject(NULL, riid, ppv);
            psf->Release();
        }
        ILFree(pidl);
    }
    else if (S_FALSE == hr)
        hr = E_FAIL;
    return hr;
}

HRESULT CMyDocsDropTarget::_InitTarget()
{
    if (_pdtgt)
        return S_OK;
    return _CreateViewObjectForMyDocs(IID_PPV_ARG(IDropTarget, &_pdtgt));
}

STDMETHODIMP CMyDocsDropTarget::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect )
{
    *pdwEffect &= ~DROPEFFECT_MOVE;     // don't let this be destructive
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->DragEnter(pDataObject, grfKeyState, pt, pdwEffect);
    return hr;
}

STDMETHODIMP CMyDocsDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    *pdwEffect &= ~DROPEFFECT_MOVE;     // don't let this be destructive
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->DragOver(grfKeyState, pt, pdwEffect);
    return hr;
}

STDMETHODIMP CMyDocsDropTarget::DragLeave()
{
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->DragLeave();
    return hr;
}

STDMETHODIMP CMyDocsDropTarget::Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= ~DROPEFFECT_MOVE;     // don't let this be destructive
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->Drop(pDataObject, grfKeyState, pt, pdwEffect);
    return hr;
}

STDMETHODIMP CMyDocsDropTarget::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_MyDocsDropTarget;
    return S_OK;
}

STDMETHODIMP CMyDocsDropTarget::IsDirty(void)
{
    return S_OK;        // no
}


STDMETHODIMP CMyDocsDropTarget::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    if (_pdtgt)
        return S_OK;
    UpdateSendToFile(FALSE);    // refresh the send to target (in case the desktop icon was renamed)
    return S_OK;
}

STDMETHODIMP CMyDocsDropTarget::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return S_OK;
}

STDMETHODIMP CMyDocsDropTarget::SaveCompleted(LPCOLESTR pszFileName)
{
    return S_OK;
}

STDMETHODIMP CMyDocsDropTarget::GetCurFile(LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;
    return E_NOTIMPL;
}

HRESULT CMyDocsDropTarget_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CMyDocsDropTarget* pdt = new CMyDocsDropTarget( );
    if (pdt)
    {
        *ppunk = SAFECAST(pdt, IDropTarget *);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}


// properyt page and context menu shell extension

class CMyDocsProp : public IShellPropSheetExt, IShellExtInit
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *lpdobj, HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

private:
    CMyDocsProp();
    ~CMyDocsProp();
    void _AddExtraPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

    friend HRESULT CMyDocsProp_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    LONG m_cRef;
};

CMyDocsProp::CMyDocsProp() : m_cRef(1)
{
    DllAddRef();
}

CMyDocsProp::~CMyDocsProp()
{
    DllRelease();
}

STDMETHODIMP CMyDocsProp::QueryInterface( REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsProp, IShellPropSheetExt), 
        QITABENT(CMyDocsProp, IShellExtInit), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CMyDocsProp::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsProp::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CMyDocsProp::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkey)
{
    return S_OK;
}

// {f81e9010-6ea4-11ce-a7ff-00aa003ca9f6}
const CLSID CLSID_CShare = {0xf81e9010, 0x6ea4, 0x11ce, 0xa7, 0xff, 0x00, 0xaa, 0x00, 0x3c, 0xa9, 0xf6 };
// {1F2E5C40-9550-11CE-99D2-00AA006E086C}
CLSID CLSID_RShellExt = {0x1F2E5C40, 0x9550, 0x11CE, 0x99, 0xD2, 0x00, 0xAA, 0x00, 0x6E, 0x08, 0x6C };

const CLSID *c_rgFilePages[] = {
    &CLSID_ShellFileDefExt,
    &CLSID_CShare,
    &CLSID_RShellExt,
};

const CLSID *c_rgDrivePages[] = {
    &CLSID_ShellDrvDefExt,
    &CLSID_CShare,
    &CLSID_RShellExt,
};

// support net share properties (if mydocs is \\server\share)
#if 0
const CLSID *c_rgNetSharePages[] = {
    &CLSID_ShellFileDefExt,
    &CLSID_CShare,
    &CLSID_RShellExt,
};
#endif // 0

// add optional pages to Explore/Options.

void CMyDocsProp::_AddExtraPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    IDataObject *pdtobj;

    if (SUCCEEDED(_GetUIObjectForMyDocs(IID_PPV_ARG(IDataObject, &pdtobj))))
    {
        TCHAR szPath[MAX_PATH];
        SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);
        BOOL fDriveRoot = PathIsRoot(szPath) && !PathIsUNC(szPath);
        const CLSID** pCLSIDs = fDriveRoot ? c_rgDrivePages : c_rgFilePages;
        int nCLSIDs = fDriveRoot ? ARRAYSIZE(c_rgDrivePages) : ARRAYSIZE(c_rgFilePages);
        for (int i = 0; i < nCLSIDs; i++)
        {
            IShellPropSheetExt *pspse;
            HRESULT hr = SHCoCreateInstance(NULL, pCLSIDs[i], NULL, IID_PPV_ARG(IShellPropSheetExt, &pspse));
            if (SUCCEEDED(hr))
            {
                IShellExtInit *psei;
                if (SUCCEEDED(pspse->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei))))
                {
                    hr = psei->Initialize(NULL, pdtobj, NULL);
                    psei->Release();
                }

                if (SUCCEEDED(hr))
                    pspse->AddPages(pfnAddPage, lParam);
                pspse->Release();
            }
        }
    }
}

STDMETHODIMP CMyDocsProp::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HRESULT hr = S_OK;

    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_TARGET);
    psp.pfnDlgProc = TargetDlgProc;

    HPROPSHEETPAGE hPage = CreatePropertySheetPage( &psp );
    if (hPage)
    {
        pfnAddPage( hPage, lParam );
        _AddExtraPages(pfnAddPage, lParam);
    }
    return hr;
}

STDMETHODIMP CMyDocsProp::ReplacePage( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT CMyDocsProp_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CMyDocsProp* pmp = new CMyDocsProp( );
    if (pmp)
    {
        *ppunk = SAFECAST(pmp, IShellExtInit *);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}
