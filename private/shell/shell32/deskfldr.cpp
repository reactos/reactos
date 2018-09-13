#include "shellprv.h"
#include "sfviewp.h"
#include "deskfldr.h"
#include "fstreex.h"
#include "datautil.h"
#include "views.h"
#include "ids.h"
#include "caggunk.h"
#include "enum.h"
#include "shitemid.h"

#include "fstreex.h"
#include "drives.h"
#include "deskfldr.h"
#include "infotip.h"

#include "unicpp\deskhtm.h"

class CDesktopFolderEnum;
class CDesktopViewCallBack;


class CDesktopFolder : IShellFolder2, IPersistFolder2, IShellIcon, IShellIconOverlay
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj); // { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void)  { return 3; /* CAggregatedUnknown::AddRef(); */ };
    STDMETHODIMP_(ULONG) Release(void) { return 2; /* CAggregatedUnknown::Release(); */ };

    // IShellFolder methods
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut);

    // IShellFolder2 methods
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // IPersist
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    //IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IShellIcon methods
    STDMETHOD(GetIconOf)(LPCITEMIDLIST pidl, UINT flags, int *piIndex);

    // IShellIconOverlay methods
    STDMETHOD(GetOverlayIndex)(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHOD(GetOverlayIconIndex)(LPCITEMIDLIST pidl, int * pIndex);
  
    CDesktopFolder(IUnknown *punkOuter);
    HRESULT _Init();
    HRESULT _Init2();
    void _Destroy();

private:
//    HRESULT v_InternalQueryInterface(REFIID riid, void **ppv);
    ~CDesktopFolder();

    friend CDesktopFolderEnum;
    friend CDesktopViewCallBack;

    friend HRESULT CDesktop_HandleCommand(CDesktopFolder *pdf, HWND hwndOwner, WPARAM wparam, BOOL bExecute);
    friend HRESULT CDesktop_DFMCallBackBG(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

    IShellFolder2 *_GetItemFolder(LPCITEMIDLIST pidl);
    HRESULT _GetItemUIObject(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, UINT *prgfInOut, void **ppvOut);
    HRESULT _QueryInterfaceItem(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
    HRESULT _CreateEnum(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum);
    HRESULT _ChildParseDisplayName(LPCITEMIDLIST pidlLeft, HWND hwnd, IBindCtx *pbc, 
                LPWSTR pwzDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, DWORD *pdwAttributes);
    HRESULT _TrySpecialUrl(HWND hwnd, LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes);

    IShellFolder2 *_psfDesktop;      // "Desktop" shell folder (real files live here)
    IShellFolder2 *_psfAltDesktop;   // "Common Desktop" shell folder
    IUnknown *_punkReg;             // regitem inner folder (agregate)
    IContextMenu *_pcmDesktopExt;
};



class CDesktopFolderEnum : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // *** IEnumIDList methods ***
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumIDList **ppenum);
    
    CDesktopFolderEnum(CDesktopFolder *pdf, HWND hwnd, DWORD grfFlags);

private:
    ~CDesktopFolderEnum();

    LONG _cRef;
    BOOL _bUseAltEnum;
    IEnumIDList *_penumFolder;
    IEnumIDList *_penumAltFolder;
};

IShellFolderViewCB* Desktop_CreateSFVCB(CDesktopFolder* pdf);

//
// Global variables
//

SFEnumCacheData g_EnumCache = {0};


#ifdef WINNT
#define NETCPL  TEXT("NCPA.CPL")
#else
#define NETCPL TEXT("NETCPL.CPL")
#endif

//
// this is now perinstance so we can poke at it at runtime :)
//
REQREGITEM g_asDesktopReqItems[] =
{
    { 
        &CLSID_MyComputer,  IDS_DRIVEROOT,  
        TEXT("explorer.exe"), 0, SORT_ORDER_DRIVES, 
        SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR  | SFGAO_DROPTARGET | SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANMONIKER,
        TEXT("SYSDM.CPL")
    },
    { 
        &CLSID_NetworkPlaces, IDS_NETWORKROOT, 
        TEXT("shell32.dll"), -IDI_MYNETWORK, SORT_ORDER_NETWORK, 
        SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR | SFGAO_DROPTARGET | SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANMONIKER, 
        NETCPL,
    },
    { 
        &CLSID_Internet, IDS_INETROOT, 
        TEXT("mshtml.dll"),   0, SORT_ORDER_INETROOT, 
        SFGAO_BROWSABLE  | SFGAO_HASPROPSHEET | SFGAO_CANRENAME  | SFGAO_FOLDER, 
        TEXT("INETCPL.CPL")
    },
};

const UNALIGNED ITEMIDLIST c_idlDesktop = { { 0, 0 } };

#define DESKTOP_PIDL  ((LPITEMIDLIST)&c_idlDesktop)



BOOL CDesktop_IsDesktop(HWND hwnd)
{
    TCHAR szClassName[50];
    return GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName)) && 
        lstrcmpi(szClassName, TEXT(STR_DESKTOPCLASS)) == 0;
}


//
// single global instance of this CDesktopFolder object
//
CDesktopFolder *g_pDesktopFolder = NULL;

REGITEMSINFO g_riiDesktop =
{
    REGSTR_PATH_EXPLORER TEXT("\\Desktop\\NameSpace"),
    NULL,
    TEXT(':'),
    SHID_ROOT_REGITEM,
    1,
    SFGAO_CANLINK,
    ARRAYSIZE(g_asDesktopReqItems),
    g_asDesktopReqItems,
    RIISA_ORIGINAL,
    NULL,
    0,
    0,
};


void Desktop_InitRequiredItems(void)
{
    //  "NoNetHood" restruction -> always hide the hood.
    //  Otherwise, show the hood if either MPR says so or we have RNA.
    if (!(GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS) || SHRestricted(REST_NONETHOOD))
    {
        // Don't enumerate the "Net Hood" thing.
        g_asDesktopReqItems[CDESKTOP_REGITEM_NETWORK].dwAttributes |= SFGAO_NONENUMERATED;
    }
    else
    {
        // Do enumerate the "My Network" thing.
        g_asDesktopReqItems[CDESKTOP_REGITEM_NETWORK].dwAttributes &= ~SFGAO_NONENUMERATED;
    }

    //
    // "NoInternetIcon" restriction -> hide The Internet on the desktop
    //
    if (SHRestricted(REST_NOINTERNETICON))
    {
        g_asDesktopReqItems[CDESKTOP_REGITEM_INTERNET].dwAttributes &=  ~(SFGAO_BROWSABLE | SFGAO_FOLDER);
        g_asDesktopReqItems[CDESKTOP_REGITEM_INTERNET].dwAttributes |= SFGAO_NONENUMERATED;
    }

    // BUGBUG:: Word Perfect 7 faults when it enumerates the Internet item
    // in their background thread.  For now App hack specific to this app
    // later may need to extend...  Note: This app does not install on
    // NT so only do for W95...
    // it repros with Word Perfect Suite 8, too, this time on both NT and 95
    // so removing the #ifndef... -- reljai 11/20/97, bug#842 in ie5 db

    if (SHGetAppCompatFlags(ACF_CORELINTERNETENUM) & ACF_CORELINTERNETENUM)
    {
        g_asDesktopReqItems[CDESKTOP_REGITEM_INTERNET].dwAttributes &=  ~(SFGAO_BROWSABLE | SFGAO_HASSUBFOLDER | SFGAO_FOLDER);
        g_asDesktopReqItems[CDESKTOP_REGITEM_INTERNET].dwAttributes |= SFGAO_NONENUMERATED;
    }
}

CDesktopFolder::CDesktopFolder(IUnknown *punkOuter)
{
    DllAddRef();
}

CDesktopFolder::~CDesktopFolder()
{
    DllRelease();
}

// first phase of init (does not need to be seralized)

HRESULT CDesktopFolder::_Init()
{
    Desktop_InitRequiredItems();

    return CRegFolder_CreateInstance(&g_riiDesktop, SAFECAST(this, IShellFolder2 *), IID_IUnknown, (void **)&_punkReg);
}

// second phase of init (needs to be seralized)

HRESULT CDesktopFolder::_Init2()
{
    HRESULT hr;

    hr = SHCacheTrackingFolder(DESKTOP_PIDL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, &_psfDesktop);
    if (FAILED(hr))
    {
        DebugMsg(DM_TRACE, TEXT("Failed to create desktop IShellFolder!"));
        return hr;
    }


    if ( !SHRestricted(REST_NOCOMMONGROUPS))
    {
        hr = SHCacheTrackingFolder(DESKTOP_PIDL, CSIDL_COMMON_DESKTOPDIRECTORY, &_psfAltDesktop);
    }

    return hr;
}

// CLSID_ShellDesktop constructor

STDAPI CDesktop_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;

    if (g_pDesktopFolder)
    {
        hr = g_pDesktopFolder->QueryInterface(riid, ppv);
    }
    else
    {
        *ppv = NULL;

        // WARNING: the order of init of the desktop folder state is very important.
        // the creation of the sub folders, in particular _psfAltDesktop will
        // recurse on this function. we protect ourself from that here. the creation
        // of that also requires the above members to be inited.

        CDesktopFolder *pdf = new CDesktopFolder(punkOuter);
        if (pdf)
        {
            hr = pdf->_Init();
            if (SUCCEEDED(hr))
            {
                // NOTE: there is a race condition here where we have stored g_pDesktopFolder but
                // not initialized _psfDesktop & _psfAltDesktop. the main line code deals with
                // this by testing for NULL on these members.
                if (SHInterlockedCompareExchange((void **)&g_pDesktopFolder, pdf, 0))
                {
                    // Someone else beat us to creating the object.
                    // get rid of our copy, global already set (the race case)
                    pdf->_Destroy();    
                }
                else
                {
                    g_pDesktopFolder->_Init2();
                }
                hr = g_pDesktopFolder->QueryInterface(riid, ppv);
            }
            else
                pdf->_Destroy();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}


STDAPI SHGetDesktopFolder(IShellFolder **ppshf)
{
    return CDesktop_CreateInstance(NULL, IID_IShellFolder, (void **)ppshf);
}

IShellFolder2 *CDesktopFolder::_GetItemFolder(LPCITEMIDLIST pidl)
{
    if (_psfAltDesktop && FS_IsCommonItem(pidl))
        return _psfAltDesktop;
    return _psfDesktop;
}

HRESULT CDesktopFolder::_QueryInterfaceItem(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr;
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        hr = psf->QueryInterface(riid, ppv);
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

__inline BOOL _RegGetsFirstShot(REFIID riid)
{
    return (IsEqualIID(riid, IID_IShellFolder) ||
            IsEqualIID(riid, IID_IShellFolder2) ||
            IsEqualIID(riid, IID_IShellIconOverlay));
}

HRESULT CDesktopFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDesktopFolder, IShellFolder2),                        // IID_ISHELLFolder2
        QITABENTMULTI(CDesktopFolder, IShellFolder, IShellFolder2),     // IID_IShellFolder
        QITABENT(CDesktopFolder, IShellIcon),                           // IID_IShellIcon
        QITABENT(CDesktopFolder, IPersistFolder2),                      // IID_IPersistFolder2
        QITABENTMULTI(CDesktopFolder, IPersistFolder, IPersistFolder2), // IID_IPersistFolder
        QITABENTMULTI(CDesktopFolder, IPersist, IPersistFolder2),       // IID_IPersist
        QITABENT(CDesktopFolder, IShellIconOverlay),                    // IID_IShellIconOverlay
        { 0 },
    };

    if (IsEqualIID(riid, CLSID_ShellDesktop))
    {
        *ppv = this;     // class pointer (unrefed!)
        return S_OK;
    }
    else if (_punkReg && _RegGetsFirstShot(riid))
    {
        return _punkReg->QueryInterface(riid, ppv);
    }
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if ((E_NOINTERFACE == hr) && (NULL != _punkReg))
    {
        return _punkReg->QueryInterface(riid, ppv);
    }
    else
    {
        return hr;
    }
}


// During shell32.dll process detach, we will call here to do the final
// release of the IShellFolder ptrs which used to be left around for the
// life of the process.  This quiets things such as OLE's debug allocator,
// which detected the leak.


void CDesktopFolder::_Destroy()
{
    ATOMICRELEASE(_psfDesktop);
    ATOMICRELEASE(_psfAltDesktop);
    ATOMICRELEASE(_pcmDesktopExt);
    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
    delete this;
}

LPITEMIDLIST CreateMyComputerIDList()
{
    return ILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}")); // CLSID_MyComputer
}

LPITEMIDLIST CreateWebFoldersIDList()
{
    return ILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{BDEADF00-C265-11D0-BCED-00A0C90AB50F}")); // CLSID_MyComputer\CLSID_WebFolders
}

LPITEMIDLIST CreateMyNetPlacesIDList()
{
    return ILCreateFromPath(TEXT("::{208D2C60-3AEA-1069-A2D7-08002B30309D}")); // CLSID_NetworkPlaces
}

HRESULT CDesktopFolder::_ChildParseDisplayName(LPCITEMIDLIST pidlLeft, HWND hwnd, IBindCtx *pbc, 
                LPWSTR pwzDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, DWORD *pdwAttributes)
{
    IShellFolder *psfBind;
    HRESULT hr = QueryInterface(IID_IShellFolder, (void **)&psfBind);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psfRight;
        hr = psfBind->BindToObject(pidlLeft, pbc, IID_IShellFolder, (void **)&psfRight);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlRight;
            hr = psfRight->ParseDisplayName(hwnd, pbc, 
                pwzDisplayName, pchEaten, &pidlRight, pdwAttributes);
            if (SUCCEEDED(hr))
            {
                hr = SHILCombine(pidlLeft, pidlRight, ppidl);
                ILFree(pidlRight);
            }
            psfRight->Release();
        }
        psfBind->Release();
    }

    return hr;
}

STDAPI_(int) SHGetSpecialFolderID(LPCWSTR pszName);

HRESULT CDesktopFolder::_TrySpecialUrl(HWND hwnd, LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    PARSEDURLW pu = {0};
    pu.cbSize = SIZEOF(pu);

    if (SUCCEEDED(ParseURLW(pwzDisplayName, &pu)) && pu.nScheme == URL_SCHEME_SHELL)
    {
        int csidl = SHGetSpecialFolderID(pu.pszSuffix);

        if (-1 != csidl)
        {
            hr = SHGetFolderLocation(hwnd, csidl | CSIDL_FLAG_CREATE, NULL, 0, ppidl);
            if (SUCCEEDED(hr) && pdwAttributes && *pdwAttributes)
            {
                hr = SHGetNameAndFlags(*ppidl, 0, NULL, 0, pdwAttributes);

                if (FAILED(hr))
                {
                    ILFree(*ppidl);
                    *ppidl = NULL;
                }
            }
        }
    }
    else if (pu.nScheme != URL_SCHEME_FTP && pu.nScheme != URL_SCHEME_FILE)
    {
        //  string is an URL, webfolders might know what to do with it, as long as its not a file or ftp url.
        LPITEMIDLIST pidlWeb = CreateWebFoldersIDList();
        if (pidlWeb)
        {
            hr = _ChildParseDisplayName(pidlWeb, hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
            ILFree(pidlWeb);
        }
    }
    return hr;
}

//----------------------------------------------------------------------------
STDMETHODIMP CDesktopFolder::ParseDisplayName(HWND hwnd, 
                                       LPBC pbc, WCHAR *pwzDisplayName, ULONG *pchEaten,
                                       LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;

    *ppidl = NULL;      // assume error

    if (pwzDisplayName && *pwzDisplayName)
    {
        LPITEMIDLIST pidlLeft = NULL;
        USES_CONVERSION;
        LPTSTR szDisplayName = W2T(pwzDisplayName);

        ASSERT(hr == E_INVALIDARG);

        if ((InRange(szDisplayName[0], TEXT('A'), TEXT('Z')) || 
             InRange(szDisplayName[0], TEXT('a'), TEXT('z'))) && 
            szDisplayName[1] == TEXT(':'))
        {
            // The string contains a path, let "My Computer" figire it out.
            pidlLeft = CreateMyComputerIDList();
            if (pchEaten)
                *pchEaten = 0;
        }
        else if (PathIsUNC(szDisplayName))
        {
            // The path is UNC, let "World" figure it out.
            pidlLeft = CreateMyNetPlacesIDList();
        }
        else if (UrlIsW(pwzDisplayName, URLIS_URL) && !SHSkipJunctionBinding(pbc, NULL))
        {
            hr = _TrySpecialUrl(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
        }

        if (!pidlLeft && FAILED(hr))
        {
            //  when we request that something be created, we need to
            //  check both folders and make sure that it doesnt exist in 
            //  either one.  and then try and create it in the user folder
            BIND_OPTS bo = {SIZEOF(bo)};
            BOOL fCreate = FALSE;

            if (pbc && SUCCEEDED(pbc->GetBindOptions(&bo)) && 
                (bo.grfMode & STGM_CREATE))
            {
                fCreate = TRUE;
                bo.grfMode &= ~STGM_CREATE;
                pbc->SetBindOptions(&bo);
            }

            //  give the users desktop first shot.
            // This must be a desktop item, _psfDesktop may not be inited in
            // the case where we are called from ILCreateFromPath()
            if (_psfDesktop)
                hr = _psfDesktop->ParseDisplayName(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);

            //  if the normal desktop folder didnt pick it off, 
            //  it could be in the allusers folder.  give psfAlt a chance.
            if (FAILED(hr) && _psfAltDesktop)
            {
                hr = _psfAltDesktop->ParseDisplayName(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);

                //  mark this as a common item.  
                if (SUCCEEDED(hr))
                    FS_MakeCommonItem(*ppidl);
            }

            //  neither of the folders can identify an existing item
            //  so we should pass the create flag to the real desktop
            if (FAILED(hr) && fCreate && _psfDesktop)
            {
                bo.grfMode |= STGM_CREATE;
                pbc->SetBindOptions(&bo);
                hr = _psfDesktop->ParseDisplayName(hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
                //  when this succeeds, we know we got a magical ghost pidl...
            }
        }

        if (pidlLeft)
        {
            hr = _ChildParseDisplayName(pidlLeft, hwnd, pbc, pwzDisplayName, pchEaten, ppidl, pdwAttributes);
            ILFree(pidlLeft);
        }
    } 
    else if (pwzDisplayName)
    {
        // we used to return this pidl when passed an empty string
        // some apps (such as Wright Design) depend on this behavior,
        // so i'm reinstating it --ccooney
        hr = SHILClone((LPCITEMIDLIST)&c_idlDrives, ppidl);
    }


    return hr;
}

STDAPI_(void) UltRoot_Term()
{
    SFEnumCache_Terminate(&g_EnumCache);
    if (g_pDesktopFolder)
    {
        g_pDesktopFolder->_Destroy();
        g_pDesktopFolder = NULL;
    }
}


HRESULT CDesktopFolder::_CreateEnum(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    HRESULT hr;
    CDesktopFolderEnum *pesf = new CDesktopFolderEnum(this, hwnd, grfFlags);
    if (pesf)
    {
        hr = pesf->QueryInterface(IID_IEnumIDList, (void **)ppenum);
        pesf->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CDesktopFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    HRESULT hr;
    // should we cache this enum for later?
    if (IsMainShellProcess() && EnumCanCache(grfFlags))
    {
        IEnumIDList *penum;
        hr = _CreateEnum(hwnd, ENUMGRFFLAGS, &penum);
        if (SUCCEEDED(hr)) 
        {
            hr = SFEnumCache_Create(penum, grfFlags, &g_EnumCache, SAFECAST(this, IShellFolder *), ppenum);
            penum->Release();
        }
    }
    else
        hr = _CreateEnum(hwnd, grfFlags, ppenum);
    return hr;
}

STDMETHODIMP CDesktopFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->BindToObject(pidl, pbc, riid, ppvOut);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->BindToStorage(pidl, pbc, riid, ppvOut);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (pidl1 == NULL || pidl2 == NULL)
        return E_INVALIDARG;

    if (pidl1->mkid.cb == 0 && pidl2->mkid.cb == 0)
        return ResultFromShort(0);      // 2 empty IDLists, they are the same

    // If both objects aren't from the same directory, they won't match.
    if (_psfAltDesktop) 
    {
        if (FS_IsCommonItem(pidl1)) 
        {
            if (FS_IsCommonItem(pidl2)) 
                return _psfAltDesktop->CompareIDs(lParam, pidl1, pidl2);
            else 
                return ResultFromShort(-1);
        } 
        else 
        {
            if (FS_IsCommonItem(pidl2)) 
                return ResultFromShort(1);
            else if (_psfDesktop)
                return _psfDesktop->CompareIDs(lParam, pidl1, pidl2);
        }
    } 
    else 
    {
        if (_psfDesktop)
            return _psfDesktop->CompareIDs(lParam, pidl1, pidl2);
    }

    // If we have no _psfDesktop, we get here...
    return ResultFromShort(-1);
}

HRESULT _GetDesktopExtMenu(HWND hwndOwner, IContextMenu** ppcm)
{
    HRESULT hr;
    IShellBrowser *psb = FileCabinet_GetIShellBrowser(hwndOwner);
    if (psb) 
    {
        IServiceProvider* psp;
        hr = psb->QueryInterface(IID_IServiceProvider, (void **)&psp);
        if (SUCCEEDED(hr)) 
        {
            hr = psp->QueryService(SID_SDesktopExtMenu, IID_IContextMenu, (void **)ppcm);
            AssertMsg(SUCCEEDED(0), TEXT("WebBar: QueryService failed %x"), hr);
            psp->Release();
        } 
        else 
        {
            AssertMsg(0, TEXT("WebBar: _GetDesktopExtMenu QI(IIS_ISP) failed %x"), hr);
        }
        // No need to release psb
    } 
    else 
    {
        AssertMsg(0, TEXT("WebBar: _GetDesktopExtMenu GetIShellFolder(%x) failed"), hwndOwner);
        hr = E_UNEXPECTED;
    }
    return hr;
}

//----------------------------------------------------------------------------
HRESULT CDesktop_HandleCommand(CDesktopFolder *pdf, HWND hwndOwner, WPARAM wparam, BOOL bExecute)
{
    HRESULT hr = NOERROR;

    switch (wparam) {
    case FSIDM_SORTBYNAME:
    case FSIDM_SORTBYTYPE:
    case FSIDM_SORTBYSIZE:
    case FSIDM_SORTBYDATE:
        if (bExecute)
        {
            ShellFolderView_ReArrange(hwndOwner, wparam - FSIDM_SORT_FIRST);
        }
        break;

    case DFM_CMD_PROPERTIES:
    case FSIDM_PROPERTIESBG:
        if (bExecute)
        {
            // run the default applet in desk.cpl
            SHRunControlPanel( TEXT("desk.cpl"), hwndOwner );
        }
        break;

    case DFM_CMD_MOVE:
    case DFM_CMD_COPY:
        hr = E_FAIL;
        break;

    default:
        //
        // Dispatch Desktop Extended menus (for WebBar)
        //
        if (wparam >= FSIDM_DESKTOPEX_FIRST && wparam <= FSIDM_DESKTOPEX_LAST) 
        {
            if (pdf->_pcmDesktopExt) 
            {
                CMINVOKECOMMANDINFO cinfo = {
                        SIZEOF(cinfo), 0, hwndOwner,
                        (LPCSTR)wparam-FSIDM_DESKTOPEX_FIRST, NULL, NULL, SW_SHOWNORMAL, 0, NULL };
                pdf->_pcmDesktopExt->InvokeCommand(&cinfo);
                pdf->_pcmDesktopExt->Release();
                pdf->_pcmDesktopExt = NULL;
            }
        }

        // This is common menu items, use the default code.
        hr = S_FALSE;
        break;
    }

    return hr;
}

//
//----------------------------------------------------------------------------
// Helper function for CDesktop_DFMCallBackBG, adds an item for each desktop
// component to the Add Desktop Items popup menu
//
// Returns:
//  TRUE if at least one item was added to the menu
//  FALSE otherwise
//
#define MAX_COMPONENT_WIDTH 40
BOOL CDesktop_AddDesktopComponentsToMenu(HMENU hmenu)
{
    BOOL fRetVal = FALSE;
    IActiveDesktop * piad;

    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_IActiveDesktop, (void **)&piad)))
    {
        COMPONENTSOPT co;
        co.dwSize = sizeof(co);

        piad->GetDesktopItemOptions(&co, 0);

        if (co.fActiveDesktop && co.fEnableComponents)
        {
            int cComp, i;

            piad->GetDesktopItemCount(&cComp, 0);

            for (i = 0; i < cComp; i++)
            {
                COMPONENT comp;
                comp.dwSize = SIZEOF(comp);
    
                if (SUCCEEDED(piad->GetDesktopItem(i, &comp, 0)))
                {
                    MENUITEMINFO mii;
                    WCHAR * pwszName;
                    WCHAR wszName[MAX_COMPONENT_WIDTH];

                    // Insert a menu item for this component
                    mii.cbSize = sizeof(MENUITEMINFO);
                    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
                    mii.fType = MFT_STRING;

                    if (comp.wszFriendlyName[0])
                        pwszName = comp.wszFriendlyName;
                    else
                        pwszName = comp.wszSource;

                    // Truncate if necessary so the menu doesn't get too wide...
                    if (!PathIsFileSpecW(pwszName))
                    {
                        pwszName = PathFindFileNameW(pwszName);
                    }

                    PathCompactPathExW(wszName, pwszName, ARRAYSIZE(wszName), 0);

#ifdef UNICODE
                    mii.cch = lstrlen(wszName);
                    mii.dwTypeData = wszName;
#else
                    {
                        CHAR szName[MAX_COMPONENT_WIDTH];

                        WideCharToMultiByte( CP_ACP, 0, wszName, -1, (LPSTR)szName, ARRAYSIZE(szName), NULL, NULL);

                        mii.cch = lstrlen(szName);
                        mii.dwTypeData = szName;
                    }
#endif
                    mii.wID = i + SFVIDM_DESKTOPHTML_ADDSEPARATOR + 1;
                    mii.fState = comp.fChecked ? MFS_CHECKED : MFS_ENABLED;

                    InsertMenuItem(hmenu, -1, TRUE, &mii);

                    fRetVal = TRUE;
                }           
            }
        }

        piad->Release();
    } 

    return fRetVal;
}

STDAPI_(HMENU) CDesktop_GetActiveDesktopMenu()
{
    HMENU hmenuAD;

    // Load the menu and make the appropriate modifications
    if (hmenuAD = SHLoadMenuPopup(HINST_THISDLL, POPUP_SFV_BACKGROUND_AD))
    {
        // Get the present settings regarding HTML on desktop
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_DESKTOPHTML | SSF_HIDEICONS, FALSE);

        if (!ss.fDesktopHTML)
        {
            DeleteMenu(hmenuAD, SFVIDM_DESKTOPHTML_ICONS, MF_BYCOMMAND);
            DeleteMenu(hmenuAD, SFVIDM_DESKTOPHTML_LOCK, MF_BYCOMMAND);
            DeleteMenu(hmenuAD, SFVIDM_DESKTOPHTML_SYNCHRONIZE, MF_BYCOMMAND);
            DeleteMenu(hmenuAD, SFVIDM_DESKTOPHTML_ADDSEPARATOR, MF_BYCOMMAND);
        }
        else
        {
            CheckMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_WEBCONTENT, MF_BYCOMMAND | MF_CHECKED);

            if(SHRestricted(REST_FORCEACTIVEDESKTOPON))
                EnableMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_WEBCONTENT, MF_BYCOMMAND | MF_GRAYED);

            if (!ss.fHideIcons)
                CheckMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_ICONS, MF_BYCOMMAND | MF_CHECKED);

            if (GetDesktopFlags() & COMPONENTS_LOCKED)
                CheckMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_LOCK, MF_BYCOMMAND | MF_CHECKED);

            if (SHRestricted(REST_NOADDDESKCOMP) || SHRestricted(REST_NODESKCOMP))
                EnableMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_NEWITEM, MF_BYCOMMAND | MF_GRAYED);

            // Add a menu item to the "Add Desktop Item" popup for each Desktop Component installed
            if (SHRestricted(REST_NOCLOSEDESKCOMP) || SHRestricted(REST_NODESKCOMP) || !CDesktop_AddDesktopComponentsToMenu(hmenuAD))
                DeleteMenu(hmenuAD, SFVIDM_DESKTOPHTML_ADDSEPARATOR, MF_BYCOMMAND);
        }

    }

    return hmenuAD;
}

// static const QCMINFO_IDMAP 
static const struct {
    UINT max;
    struct {
        UINT id;
        UINT fFlags;
    } list[2];
} idMap = {
    2, {
        {FSIDM_FOLDER_SEP, QCMINFO_PLACE_BEFORE},
        {FSIDM_VIEW_SEP, QCMINFO_PLACE_AFTER},
    },
};

// background context menu callback
//
// Returns:
//      NOERROR, if successfully processed.
//      S_FALSE, if default code should be used.

HRESULT CDesktop_DFMCallBackBG(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = NOERROR;


    CDesktopFolder *pdf;
    psf->QueryInterface(CLSID_ShellDesktop, (void **)&pdf);

    ASSERT(pdf);

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & CMF_VERBSONLY))
        {
            HRESULT hresT;
            UINT idCmdFirst;
            LPQCMINFO pqcm;
            BOOL bDesktop = FALSE;

            // If desktop is in ExtendedView, then the hwnd we get for
            // the context menu is a child of the Desktop.
            while (hwnd)
            {
                bDesktop = CDesktop_IsDesktop(hwnd);
                if (bDesktop)
                    break;
                hwnd = GetParent(hwnd);
            }

            pqcm = (LPQCMINFO)lParam;

            // This needs to be saved before MergeMenu
            idCmdFirst = pqcm->idCmdFirst;
            pqcm->pIdMap = (QCMINFO_IDMAP *)&idMap;

            // HACK: Note the Desktop and FSView menus are the same
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_FSVIEW_BACKGROUND,
                POPUP_FSVIEW_POPUPMERGE, pqcm);

            if (bDesktop)
            {
                TCHAR szMenu[60];

                // HACK: We only want LargeIcons on the real desktop
                // so we remove the View menu
                DeleteMenu(pqcm->hmenu, SFVIDM_MENU_VIEW, MF_BYCOMMAND);

                // No Choose Columns either
                DeleteMenu(pqcm->hmenu, SFVIDM_VIEW_COLSETTINGS, MF_BYCOMMAND);

                // Only put on ActiveDesktop menu item if it isn't restricted.
                if ((SHRestricted(REST_FORCEACTIVEDESKTOPON) || (!SHRestricted(REST_NOACTIVEDESKTOP) && !SHRestricted(REST_CLASSICSHELL)))
                    && !SHRestricted(REST_NOACTIVEDESKTOPCHANGES))
                {
                    HMENU hmenuAD;
                    MENUITEMINFO mii;

                    if (hmenuAD = CDesktop_GetActiveDesktopMenu())
                    {
                        // Insert Active Desktop submenu at the top
                        mii.cbSize = sizeof(MENUITEMINFO);
                        mii.fMask = MIIM_SUBMENU | MIIM_TYPE;
                        mii.fType = MFT_STRING;
                        mii.hSubMenu = hmenuAD;
                        mii.cch = LoadString(HINST_THISDLL, IDS_ACTIVEDESKTOP, szMenu, ARRAYSIZE(szMenu));
                        mii.dwTypeData = szMenu;
                        InsertMenuItem(pqcm->hmenu, 0, TRUE, &mii);
                    }
                }

                // Give a chance to add menuitem(s) to the extended menu object.
                //
                ASSERT(pqcm->idCmdFirst < idCmdFirst+FSIDM_DESKTOPEX_FIRST);
                ASSERT(pqcm->idCmdLast >= idCmdFirst+FSIDM_DESKTOPEX_LAST);

                if (!pdf->_pcmDesktopExt)
                    hresT = _GetDesktopExtMenu(hwnd, &pdf->_pcmDesktopExt);

                if (pdf->_pcmDesktopExt)
                {
                    hresT = pdf->_pcmDesktopExt->QueryContextMenu(
                                pqcm->hmenu, 0,
                                idCmdFirst+FSIDM_DESKTOPEX_FIRST,
                                idCmdFirst+FSIDM_DESKTOPEX_LAST,
                                0);
                    if (SUCCEEDED(hresT))
                    {
                        pqcm->idCmdFirst = idCmdFirst + FSIDM_DESKTOPEX_FIRST + HRESULT_CODE(hresT);
                    }
                    else
                    {
                        DebugMsg(DM_ERROR, TEXT("WebBar: QueryContextMenu failed %x"), hresT);
                    }
                }
            }
            else
            {
                // HACK: We want no Properties for DesktopInExplorer
                DeleteMenu(pqcm->hmenu, FSIDM_PROPERTIESBG+idCmdFirst, MF_BYCOMMAND);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMAND:
    case DFM_VALIDATECMD:
        hr = CDesktop_HandleCommand(pdf, hwnd, wParam, uMsg == DFM_INVOKECOMMAND);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

STDMETHODIMP CDesktopFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppvOut)
{
    HRESULT hr;

    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = Desktop_CreateSFVCB(this);

        QueryInterface(IID_IShellFolder, (void **)&sSFV.pshf);   // in case we are agregated

        hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppvOut);

        if (sSFV.pshf)
            sSFV.pshf->Release();

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && _psfDesktop)
    {
        hr = _psfDesktop->CreateViewObject(hwnd, riid, ppvOut);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IShellFolder *psfTemp;
        hr = QueryInterface(IID_IShellFolder, (void **)&psfTemp);
        if (SUCCEEDED(hr))
        {
            HKEY hkNoFiles = NULL;

            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);

            hr = CDefFolderMenu_Create2(&c_idlDesktop, hwnd, 0, NULL,
                    psfTemp, CDesktop_DFMCallBackBG,
                    1, &hkNoFiles, (IContextMenu **)ppvOut);
            psfTemp->Release();
            RegCloseKey(hkNoFiles);
        }
    }
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfOut)
{
    if (IsSelf(cidl, apidl))
    {
        // the desktop's attributes
        *rgfOut &= SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_CANRENAME | SFGAO_CANMONIKER |
            SFGAO_HASSUBFOLDER | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR;
        return NOERROR;
    }

    IShellFolder2 *psf = _GetItemFolder(apidl[0]);
    if (psf)
        return psf->GetAttributesOf(cidl, apidl, rgfOut);
    return E_UNEXPECTED;
}

HRESULT CDesktopFolder::_GetItemUIObject(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                         REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    IShellFolder2 *psf = _GetItemFolder(apidl[0]);
    if (psf)
        return psf->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                    REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    HRESULT hr = E_NOINTERFACE;
    
    *ppvOut = NULL;

    if (IsSelf(cidl, apidl))
    {
        // for the desktop itself
        if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) 
        {
            hr = SHCreateDefExtIcon(NULL, II_DESKTOP, II_DESKTOP, GIL_PERCLASS, riid, ppvOut);
        }
        else if (IsEqualIID(riid, IID_IQueryInfo))
        {
            hr = CreateInfoTipFromText(MAKEINTRESOURCE(IDS_DESKTOP), riid, ppvOut); //The InfoTip COM object
        }
    }
    else
    {
        if (IsEqualIID(riid, IID_IContextMenu))
        {
            hr = _GetItemUIObject(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
        }
        else if (IsEqualIID(riid, IID_IDataObject) && cidl)
        {
            if (cidl == 1)
            {
                hr = _GetItemUIObject(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
            }
            else
            {
                LPITEMIDLIST *ppidl, pidlAlt;

                SHGetIDListFromUnk((IUnknown*)_psfAltDesktop, &pidlAlt);
            
                ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, sizeof(LPITEMIDLIST) * cidl);
                if (ppidl)
                {
                    for (UINT i = 0; i < cidl; i++) 
                    {
                        if (pidlAlt && FS_IsCommonItem(apidl[i])) 
                            ppidl[i] = ILCombine(pidlAlt, apidl[i]);
                        else 
                            ppidl[i] = ILClone(apidl[i]);
                    }
            
                    hr = CFSFolder_CreateDataObject(&c_idlDesktop, cidl, (LPCITEMIDLIST *)ppidl, (IDataObject **)ppvOut);
            
                    for (i = 0; i < cidl; i++)
                        ILFree(ppidl[i]);
            
                    LocalFree(ppidl);
                }
                else
                    hr = E_OUTOFMEMORY;

                if (pidlAlt)
                    ILFree(pidlAlt);
            }
        }
        else
        {
            hr = _GetItemUIObject(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
        }
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    HRESULT hr;

    if (IsSelf(1, &pidl))
    {
        if ((dwFlags & (SHGDN_FORPARSING | SHGDN_INFOLDER | SHGDN_FORADDRESSBAR)) == SHGDN_FORPARSING)
        {
            // note some ISV apps puke if we return a full name here but the
            // rest of the shell depends on this...
            TCHAR szPath[MAX_PATH];
            SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath);
            hr = StringToStrRet(szPath, pStrRet);
        }
        else
            hr = ResToStrRet(IDS_DESKTOP, pStrRet);   // display name, "Desktop"
    }
    else
    {
        IShellFolder2 *psf = _GetItemFolder(pidl);
        if (psf)
            hr = psf->GetDisplayNameOf(pidl, dwFlags, pStrRet);
        else
            hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                                       LPCOLESTR pszName, DWORD dwRes, LPITEMIDLIST *ppidlOut)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->SetNameOf(hwnd, pidl, pszName, dwRes, ppidlOut);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    return E_NOTIMPL;
}   

STDMETHODIMP CDesktopFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CDesktopFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (_psfDesktop)
        return _psfDesktop->GetDefaultColumn(dwRes, pSort, pDisplay);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDefaultColumnState(UINT iColumn, DWORD *pdwState)
{
    if (_psfDesktop)
        return _psfDesktop->GetDefaultColumnState(iColumn, pdwState);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->GetDetailsEx(pidl, pscid, pv);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    IShellFolder2 *psf = _GetItemFolder(pidl);
    if (psf)
        return psf->GetDetailsOf(pidl, iColumn, pDetails);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    if (_psfDesktop)
        return _psfDesktop->MapColumnToSCID(iColumn, pscid);
    return E_UNEXPECTED;
}

STDMETHODIMP CDesktopFolder::GetClassID(CLSID *pCLSID)
{
    *pCLSID = CLSID_ShellDesktop;
    return NOERROR;
}

STDMETHODIMP CDesktopFolder::Initialize(LPCITEMIDLIST pidl)
{
    return ILIsEmpty(pidl) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDesktopFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(&c_idlDesktop, ppidl);
}

STDMETHODIMP CDesktopFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex)
{
    IShellIcon *psi;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_IShellIcon, (void **)&psi);
    if (SUCCEEDED(hr))
    {
        hr = psi->GetIconOf(pidl, flags, piIndex);
        psi->Release();
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    IShellIconOverlay *psio;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_IShellIconOverlay, (void **)&psio);
    if (SUCCEEDED(hr))
    {
        hr = psio->GetOverlayIndex(pidl, pIndex);
        psio->Release();
    }
    return hr;
}

STDMETHODIMP CDesktopFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    IShellIconOverlay *psio;
    HRESULT hr = _QueryInterfaceItem(pidl, IID_IShellIconOverlay, (void **)&psio);
    if (SUCCEEDED(hr))
    {
        hr = psio->GetOverlayIconIndex(pidl, pIconIndex);
        psio->Release();
    }
    return hr;
}

//
// we only care about events that actually change the enumeration
// so updateimage and the like we don't care about
//
STDAPI_(void) Desktop_InvalidateEnumCache(LPCITEMIDLIST pidl, LONG lEvent)
{
    if (pidl) 
    {
        if (ILIsEmpty(pidl)) 
        {
            if (lEvent & SHCNE_DISKEVENTS) 
            {
                // change on the desktop itself
                // invalidate all
                SFEnumCache_Invalidate(&g_EnumCache, ENUMGRFFLAGS);
            }
        } 
        else if (ILIsEmpty(_ILNext(pidl))) 
        {
            // if it's the direct child of the desktop...

            if (lEvent & (SHCNE_RENAMEITEM | SHCNE_CREATE | SHCNE_DELETE))
                SFEnumCache_Invalidate(&g_EnumCache, SHCONTF_INCLUDEHIDDEN | SHCONTF_NONFOLDERS);
            else if (lEvent & (SHCNE_RENAMEFOLDER | SHCNE_MKDIR | SHCNE_RMDIR))
                SFEnumCache_Invalidate(&g_EnumCache, SHCONTF_INCLUDEHIDDEN | SHCONTF_FOLDERS);
            else if (lEvent & SHCNE_UPDATEITEM)
                SFEnumCache_Invalidate(&g_EnumCache, SHCONTF_INCLUDEHIDDEN | SHCONTF_FOLDERS | SHCONTF_NONFOLDERS);
        }
    }
}


// Sets *ppidl to the proper pidl to notify and
// returns TRUE iff pidl is a desktop/commondesktop item.
// In the commondesktop case, it munges the pidl and stores the newly allocated pidl in ppidlTemp
BOOL CDesktop_FSEventHelper(LPITEMIDLIST pidfDesktop, LPITEMIDLIST pidfCommonDesktop, LPCITEMIDLIST pidl, LPITEMIDLIST * ppidl, LPITEMIDLIST * ppidlTemp)
{
    BOOL fResend;

    if (NULL != (*ppidl = ILFindChild(pidfDesktop, pidl))) 
    {
        fResend = TRUE;
    } 
    else if (NULL != (*ppidl = ILFindChild(pidfCommonDesktop, pidl))) 
    {
        fResend = TRUE;

        if (!ILIsEmpty(*ppidl)) 
        {
            *ppidlTemp = ILClone(*ppidl);
            if (*ppidlTemp) 
            {
                FS_MakeCommonItem(*ppidlTemp);
                *ppidl = *ppidlTemp;
            }
        }
    } 
    else
    {
        fResend = FALSE;

        *ppidl = (LPITEMIDLIST)pidl;
    }

    return fResend;
}

class CDesktopViewCallBack : public CBaseShellFolderViewCB
{
public:
    CDesktopViewCallBack(CDesktopFolder* pdf);
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    CDesktopFolder* _pdf;

    HRESULT OnMERGEMENU(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_FSVIEW_POPUPMERGE, lP);
        return S_OK;
    }

    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP)
    {
        return CDesktop_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_INVOKECOMMAND, wP, 0);
    }

    HRESULT OnSupportsIdentity(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnGETHELPTEXT(DWORD pv, UINT wPl, UINT wPh, LPTSTR lP)
    {
#ifdef UNICODE
        return CDesktop_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_GETHELPTEXTW, MAKEWPARAM(wPl, wPh), (LPARAM)lP);
#else
        return CDesktop_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_GETHELPTEXT, MAKEWPARAM(wPl, wPh), (LPARAM)lP);
#endif
    }

    HRESULT OnGETCCHMAX(DWORD pv, LPCITEMIDLIST pidlItem, UINT *lP)
    {
        if (SIL_GetType(pidlItem) == SHID_ROOT_REGITEM) // evil, we should not have to know this
        {
            *lP = MAX_REGITEMCCH;
            return NOERROR;
        }
        else
        {
            IShellFolder2 *psf = _pdf->_GetItemFolder(pidlItem);
            if (psf)
            {
                CFSFolder *pfsf = FS_GetFSFolderFromShellFolder(psf);
                return CFSFolder_GetCCHMax(pfsf, FS_IsValidID(pidlItem), lP);
            }
        }
        return E_FAIL;
    }

    HRESULT OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj);

    HRESULT OnGetWorkingDir(DWORD pv, UINT wP, LPTSTR pszDir)
    {
        return SHGetSpecialFolderPath(NULL, pszDir, CSIDL_DESKTOPDIRECTORY, TRUE) ? S_OK : E_FAIL;
    }
};


#define DESKTOP_EVENTS \
    SHCNE_DISKEVENTS | \
    SHCNE_ASSOCCHANGED | \
    SHCNE_NETSHARE | \
    SHCNE_NETUNSHARE

CDesktopViewCallBack::CDesktopViewCallBack(CDesktopFolder* pdf) : 
    CBaseShellFolderViewCB(SAFECAST(pdf, IShellFolder *), (LPCITEMIDLIST)&c_idlDesktop, DESKTOP_EVENTS),
    _pdf(pdf)
{ 

}

IShellFolderViewCB* Desktop_CreateSFVCB(CDesktopFolder* pdf)
{
    return new CDesktopViewCallBack(pdf);
}


HRESULT CDesktopViewCallBack::OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj)
{
    *ppObj = NULL;

    CViewsList cViews;

    // Add base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("folder"));

    // Add 2nd base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory"));

    IBrowserService2* pbs;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SShellDesktop, IID_IUnknown, (void**)&pbs)))
    {   // It's the actual desktop
        pbs->Release();
        // Add this class stuff
        cViews.AddCLSID(&CLSID_ShellDesktop);
    }
    else    // Don't add the desktop.ini stuff for the actual desktop. Add it only if we are in folder view
    {
        TCHAR szHere[MAX_PATH];
        if (SHGetPathFromIDList(m_pidl, szHere))
        {
            TCHAR szIniFile[MAX_PATH];
            PathCombine(szIniFile, szHere, c_szDesktopIni);
            cViews.AddIni(szIniFile, szHere);
        }
    }

    cViews.GetDef(pvid);

    return CreateEnumCViewList(&cViews, ppObj);

    // Note the automatic destructor will free any views still left
}


STDMETHODIMP CDesktopViewCallBack::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMERGEMENU);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_SUPPORTSIDENTITY, OnSupportsIdentity);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGETCCHMAX);
    HANDLE_MSG(0, SFVM_GETVIEWS, OnGetViews);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGetWorkingDir);

    default:
        return E_FAIL;
    }

    return NOERROR;
}

CDesktopFolderEnum::CDesktopFolderEnum(CDesktopFolder *pdf, HWND hwnd, DWORD grfFlags) : 
    _cRef(1), _bUseAltEnum(FALSE)
{
    if (pdf->_psfDesktop)
        pdf->_psfDesktop->EnumObjects(hwnd, grfFlags, &_penumFolder);

    if (pdf->_psfAltDesktop) 
        pdf->_psfAltDesktop->EnumObjects(hwnd, grfFlags, &_penumAltFolder);
}

CDesktopFolderEnum::~CDesktopFolderEnum()
{
    if (_penumFolder)
        _penumFolder->Release();

    if (_penumAltFolder)
        _penumAltFolder->Release();
}

STDMETHODIMP CDesktopFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDesktopFolderEnum, IEnumIDList),                        // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDesktopFolderEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDesktopFolderEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDesktopFolderEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr;

    if (_bUseAltEnum)
    {
       if (_penumAltFolder) 
       {
           hr = _penumAltFolder->Next(celt, ppidl, pceltFetched);
           if (S_OK == hr)
           {
               ULONG i, iCount = pceltFetched ? *pceltFetched : 1;
               for (i = 0; i < iCount; i++)
                   FS_MakeCommonItem(ppidl[i]);
           }
       }
       else
           hr = S_FALSE;
    } 
    else if (_penumFolder)
    {
       hr = _penumFolder->Next(celt, ppidl, pceltFetched);
       if (S_OK != hr) 
       {
           _bUseAltEnum = TRUE;
           hr = Next(celt, ppidl, pceltFetched);  // recurse
       }
    }
    else
    {
        hr = S_FALSE;
    }

    if (hr == S_FALSE)
    {
        *ppidl = NULL;
        if (pceltFetched)
            *pceltFetched = 0;
    }

    return hr;
}


STDMETHODIMP CDesktopFolderEnum::Skip(ULONG celt) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CDesktopFolderEnum::Reset() 
{
    if (_penumFolder)
        _penumFolder->Reset();

    if (_penumAltFolder)
        _penumAltFolder->Reset();

    _bUseAltEnum = FALSE;
    return S_OK;
}

STDMETHODIMP CDesktopFolderEnum::Clone(IEnumIDList **ppenum) 
{
    *ppenum = NULL;
    return E_NOTIMPL;
}
