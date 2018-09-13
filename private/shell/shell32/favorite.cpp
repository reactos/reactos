#include "shellprv.h"

#include "favorite.h"
#include "ids.h"
#include "views.h"
#include "prop.h"           // COL_DATA

// 
// Columns
//

typedef enum
{
    ICOL_FAV_LASTVISIT = 0,
    ICOL_FAV_LASTMOD,
    ICOL_FAV_NUMVISITS,
    ICOL_FAV_SPLAT,
};

const COL_DATA c_fav_cols[] = {
    {ICOL_FAV_LASTVISIT,    IDS_FAV_COL_LAST_VISIT,     18, LVCFMT_LEFT,    &SCID_LASTVISITED},
    {ICOL_FAV_LASTMOD,      IDS_FAV_COL_LAST_MOD,       18, LVCFMT_LEFT,    &SCID_LASTMODIFIED},
    {ICOL_FAV_NUMVISITS,    IDS_FAV_COL_COUNT_VISIT,    10, LVCFMT_RIGHT,   &SCID_VISITCOUNT},
    {ICOL_FAV_SPLAT,        IDS_FAV_COL_SPLAT,          25, LVCFMT_LEFT,    &SCID_STATUS}
};


//==========================================================================================
// Class Definitions
//==========================================================================================


class CFavFolderViewCB;

typedef struct _FavExtra
{
    LPITEMIDLIST pidl;      // The pidl the information belongs to
    IPropertyStorage * ppropstgSite;

    FILETIME     ftVisit;   // The last time we visited the site...
} FAVEXTRA, * PFAVEXTRA;

#define FavExtra_IsValidSite(pfe)   ((pfe) && (pfe->ppropstgSite))

typedef struct  _FavDSA
{
    HWND        hwnd;       // the main hwnd for a view...
    CFavFolderViewCB *pffvcb;   // Pointer to callback.
} FAVDSA, *PFAVDSA;



//
// Definition of the IEnumSFVViews wrapper class
//
// Note this will delegate calls to the real UEnumSFVViews and will
// add the thumbnail view on the end of the list

HRESULT CFavViewEnum_Create( IEnumSFVViews * pDefEnum, IEnumSFVViews ** ppEnum );

class CFavViewEnum : public IEnumSFVViews
{
    public:
        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (REFIID riid, void ** ppv);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** IEnumSFVViews methods ***
        STDMETHOD(Next)(ULONG celt, SFVVIEWSDATA **ppData, ULONG *pceltFetched);
        STDMETHOD(Skip)(ULONG celt);
        STDMETHOD(Reset)();
        STDMETHOD(Clone)(IEnumSFVViews **ppenum);
        
    protected:
        CFavViewEnum( IEnumSFVViews * pEnum );
        ~CFavViewEnum();

        friend HRESULT CFavViewEnum_Create( IEnumSFVViews * pDefEnum, IEnumSFVViews ** ppEnum );
        LONG m_cRef;
        IEnumSFVViews * m_pesfvv;
        int m_iAddView;
};

//
// Definition of the Favorites class
//
//  Note this extension only works on Nashville shells, because it
//  requires the fstree's new support for aggregation in IShellFolder.

class CFavorites : public IShellFolder,
                   public IPersistFolder2
{
public:
    CFavorites();
    ~CFavorites();

    // IUnknown methods
    
    virtual STDMETHODIMP  QueryInterface(REFIID riid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    // IShellFolder methods
    virtual STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    virtual STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    virtual STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    virtual STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void ** ppv);
    virtual STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void **ppv);
    virtual STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
    virtual STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);
    virtual STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    virtual STDMETHODIMP SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IPersist
    virtual STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IPersistFolder
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    virtual STDMETHODIMP GetCurFolder(LPITEMIDLIST *pidl);

    // This is Gross, but there is no clean way for example for the
    // IContextMenu to Get to the IShellView or the callback... So add a
    // register/unregister site and search function to get to it...

    STDMETHODIMP_(BOOL) RegisterFolderViewCB(HWND, CFavFolderViewCB *);
    STDMETHODIMP_(BOOL) UnRegisterFolderViewCB(HWND);
    STDMETHODIMP_(CFavFolderViewCB *) QueryFolderViewCBFromHwnd(HWND);

    // implemented here so we can get at it from both IShellDetails and from the view callback...
    STDMETHOD(GetDetailsOf)(IShellDetails * psd, LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);

protected:
    friend class CFavFolderViewCB;
    friend class CFavFolderContextMenu;
    friend class CFavoritesDetails;

    LONG            m_cRef;
    LPITEMIDLIST    m_pidl;
    IUnknown *      m_punkFSFolder;      // The inside unknown...
    IShellFolder *  m_psfFSFolder;       // The underlying shell folder...
    HDPA            m_hdpaExtra;         // The extra stuff
    HDSA            m_hdsaFolderViewCBs; //
    int             m_iFirstAddedCol;    // BUGBUG: per view data in the folder
    
private:
    // Other methods

    STDMETHODIMP_(PFAVEXTRA) GetPointerToExtraData(LPCITEMIDLIST);
    STDMETHODIMP_(DWORD)    GetVisitCount(LPCITEMIDLIST);
    STDMETHODIMP            GetSplatInfo(LPCITEMIDLIST);
    STDMETHODIMP            GetStringProp(LPCITEMIDLIST pidl, PROPID propid, LPTSTR pszBuf, DWORD cchBuf);
    STDMETHODIMP            GetFileTimePropStr(LPCITEMIDLIST pidl, PROPID propid, LPTSTR pszBuf, DWORD cchBuf);
    STDMETHODIMP            GetFileTimeProp(LPCITEMIDLIST pidl, PROPID propid, LPFILETIME pft);

    // Other Helper functions.

    STDMETHODIMP_(void)     ClearCachedData(void);
    STDMETHODIMP            OpenPropertyStorage(LPCITEMIDLIST pidl, PFAVEXTRA pfe);
};


HRESULT CFavoritesDetails_CreateInstance(CFavorites * pFav, IShellDetails ** ppv);

// IshellDetails Favorties objects...
class CFavoritesDetails : public IShellDetails
{
    public:
        CFavoritesDetails( CFavorites * pFav, HRESULT * pHr );
        
        // *** IUnknown methods ***
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
        STDMETHOD_(ULONG,AddRef)();
        STDMETHOD_(ULONG,Release)();

        // IshellDetails
        STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);
        STDMETHOD(ColumnClick)(UINT iColumn);

    protected:
        ~CFavoritesDetails();
        IShellDetails * m_psdFolder;
        CFavorites * m_pFav;
        LONG m_cRef;
};

//
// Definition of Favorites folder view callback
//


class CFavFolderViewCB : public IShellFolderViewCB
{
public:
    friend class CFavFolderContextMenu;

    CFavFolderViewCB(CFavorites *, IShellFolderView *);
    ~CFavFolderViewCB();

    STDMETHODIMP_(void) SetMainCB(IShellFolderViewCB *psfvcb);

    // IUnknown methods

    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    // IShellFolderViewCB methods
    virtual STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    // Helper methods
    STDMETHODIMP GetViews( SHELLVIEWID * pDefID, IEnumSFVViews ** ppEnum );
    STDMETHODIMP GetDetailsOf(int iCol, DETAILSINFO * pdi);
    STDMETHODIMP_(void) HandleMergeMenu(UINT, QCMINFO*);

    LONG                m_cRef;
    CFavorites *        m_pFav;
    IShellFolderView *  m_psfv;
    IShellFolderViewCB *m_psfvcb;   // The one we are trying to subclass
    HWND                m_hwndMain;  // The main Window
    UINT                m_idCmdFirstAdded;  // First Added command to main menu
    IShellDetails     * m_psdFolder;
};


//
// Definition of Favorites context menu handler class
//

class CFavFolderContextMenu : public IContextMenu3, IObjectWithSite
{
public:

    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    // IContextMenu
    virtual STDMETHODIMP QueryContextMenu (HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast,  UINT uFlags);
    virtual STDMETHODIMP InvokeCommand (LPCMINVOKECOMMANDINFO lpici);
    virtual STDMETHODIMP GetCommandString (UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IContextMenu2
    virtual STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    virtual STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);

    // IObjectWithSite
    virtual STDMETHODIMP SetSite(IUnknown*);
    virtual STDMETHODIMP GetSite(REFIID, void**);

    friend HRESULT CFavFolderContextMenu_CreateInstance(CFavorites *pFav, HWND hwnd, REFIID riid, void **);

private:

    CFavFolderContextMenu(CFavorites *, HWND, HRESULT *);
    ~CFavFolderContextMenu();

    LONG            m_cRef;
    HWND            m_hwndOwner;
    CFavorites *    m_pFav;
    IContextMenu2 * m_pcmInner;
    UINT            m_idCmdFirst;
};



STDAPI CFavorites_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hres;

    CFavorites* ptfld = new CFavorites();
    if (ptfld) 
    {
        hres = ptfld->QueryInterface(riid, ppv);
        ptfld->Release();
    }
    else
    {
        *ppv = NULL;
        hres = E_OUTOFMEMORY;
    }

    return hres;
}




/*----------------------------------------------------------
Purpose: Helper function to read a single propvariant from
         the given property storage

Returns: 
Cond:    --
*/
HRESULT ReadProp(IPropertyStorage * ppropstg, PROPID propid, PROPVARIANT *ppropvar)
{
    PROPSPEC prspec;

    prspec.ulKind = PRSPEC_PROPID;
    prspec.propid = propid;

    return ppropstg->ReadMultiple(1, &prspec, ppropvar);
}

//==========================================================================================
// CFavorites Implemention
//==========================================================================================

CFavorites::CFavorites(void) : m_cRef(1)
{
    DllAddRef();

    m_pidl = NULL;
    m_psfFSFolder = NULL;
    m_punkFSFolder = NULL;
    m_hdpaExtra = NULL;  // Create when we need it...
    m_hdsaFolderViewCBs = NULL; // Created when needed...
    m_iFirstAddedCol = -1;
}

CFavorites::~CFavorites(void)
{
    TraceMsg(TF_FAV_FULL, "CFavorites::~CFavorites called");

    AddRef();    // Make sure it does not cycle back to us...

    if (m_pidl)
        ILFree(m_pidl);

    if (m_punkFSFolder)
        m_punkFSFolder->Release();

    ClearCachedData();

    if (m_hdsaFolderViewCBs)
        DSA_Destroy(m_hdsaFolderViewCBs);

    DllRelease();
}


STDMETHODIMP CFavorites::OpenPropertyStorage(LPCITEMIDLIST pidl, PFAVEXTRA pfe)
{
    IPropertySetStorage * ppropsetstg;
    HRESULT hres = BindToStorage(pidl, NULL, IID_IPropertySetStorage,  (void **)&ppropsetstg);
    if (SUCCEEDED(hres))
    {
        hres = ppropsetstg->Open(FMTID_InternetSite, STGM_READ, &pfe->ppropstgSite);
        ppropsetstg->Release();
    }

    return hres;
}

STDMETHODIMP CFavorites::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFavorites, IShellFolder), // IID_IShellFolder
        QITABENT(CFavorites, IPersistFolder),   // IID_IPersistFolder
        QITABENTMULTI(CFavorites, IPersist, IPersistFolder),    // IID_IPersistFolder
        QITABENTMULTI(CFavorites, IPersist, IPersistFolder2),   // IID_IPersistFolder2
        { 0 },
    };
    HRESULT hres = QISearch(this, qit, riid, ppv);
    if (FAILED(hres) && m_punkFSFolder)
        hres = m_punkFSFolder->QueryInterface(riid, ppv);
    return hres;
}

STDMETHODIMP_(ULONG) CFavorites::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFavorites::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP CFavorites::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    return m_psfFSFolder->ParseDisplayName(hwnd, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);
}

STDMETHODIMP CFavorites::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    return m_psfFSFolder->EnumObjects(hwnd, grfFlags, ppenum);
}

STDMETHODIMP CFavorites::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    // Lets try to pick off Binds to IShellFolder and if it is a directory
    // use our implementation...

    HRESULT hr = m_psfFSFolder->BindToObject(pidl, pbc, riid, ppv);

    if (SUCCEEDED(hr))
    {
        //
        // make sure that all FS Folders are wrapped by us
        // but only FS folders are wrapped.  all files or junctions
        // should defer to the psfInner here.
        //
            
        CLSID clsid;
        if (SUCCEEDED(IUnknown_GetClassID((IUnknown *)*ppv, &clsid))
        &&  IsEqualGUID(CLSID_ShellFSFolder, clsid))
        {
            IPersistFolder *ppf;

            ((IUnknown *)*ppv)->Release();
            *ppv = NULL;
            
            hr = CFavorites_CreateInstance(NULL, IID_IPersistFolder, (void **)&ppf);
            if (SUCCEEDED(hr))
            {   
                LPITEMIDLIST pidlFull;
                hr = SHILCombine(m_pidl, pidl, &pidlFull);
                if (SUCCEEDED(hr))
                {
                    hr = ppf->Initialize(pidlFull);
                    ILFree(pidlFull);

                    if (SUCCEEDED(hr))
                        hr= ppf->QueryInterface(riid, ppv);
                }
                ppf->Release();
            }
        }
    }
    return hr;
}

STDMETHODIMP CFavorites::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return m_psfFSFolder->BindToStorage(pidl, pbc, riid, ppv);
}

#define ComparisonResult(a, b) \
    ((a) < (b) ? ResultFromShort(-1) : \
     (a) > (b) ? ResultFromShort(+1) : \
                 ResultFromShort( 0))

STDMETHODIMP CFavorites::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    PROPID pid;

    if (m_iFirstAddedCol >= 0)
    {
        switch ((lParam & SHCIDS_COLUMNMASK) - m_iFirstAddedCol)
        {
        case ICOL_FAV_NUMVISITS:
            {
                DWORD dwNumVisits1 = this->GetVisitCount(pidl1);
                DWORD dwNumVisits2 = this->GetVisitCount(pidl2);
                // macro, don't inline this
                HRESULT hres = ComparisonResult(dwNumVisits1, dwNumVisits2);
                if (hres)
                    return hres;
                goto DefaultSort;
            }
            break;

        case ICOL_FAV_LASTVISIT:
            pid = PID_INTSITE_LASTVISIT;
            goto FinishFileTime;

        case ICOL_FAV_LASTMOD:
            pid = PID_INTSITE_LASTMOD;

        FinishFileTime:
            {
                FILETIME ft1, ft2;
                if (SUCCEEDED(this->GetFileTimeProp(pidl1, pid, &ft1)) &&
                    SUCCEEDED(this->GetFileTimeProp(pidl2, pid, &ft2)))
                {
                    HRESULT hres = CompareFileTime(&ft1, &ft2);
                    if (hres)
                        return hres;
                }
                goto DefaultSort;
            }

        case ICOL_FAV_SPLAT:
            {
                HRESULT hres1 = this->GetSplatInfo(pidl1);
                HRESULT hres2 = this->GetSplatInfo(pidl1);
                if (hres1 != hres2)
                    return ComparisonResult(hres1, hres2);
                goto DefaultSort;
            }

        default:
            // Some column we aren't in charge of - let the superclass do it
            break;
        }
    }

DefaultSort:
    return m_psfFSFolder->CompareIDs(lParam, pidl1, pidl2);
}

#undef ComparisonResult

STDMETHODIMP CFavorites::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    // override the implemention to allow us to add in our own columns...

    if (IsEqualIID(riid, IID_IShellView))
    {
        HRESULT hres = m_psfFSFolder->CreateViewObject(hwnd, riid, ppv);
        if (SUCCEEDED(hres))
        {
            IShellFolderView *psfv;
            if (SUCCEEDED(((IUnknown *)*ppv)->QueryInterface(IID_IShellFolderView, (void **)&psfv)))
            {
                IShellFolderViewCB *psfvcbWrap;
                CFavFolderViewCB *psfvcb = new CFavFolderViewCB(this, psfv);
                if (psfvcb)
                {
                    if (SUCCEEDED(psfv->SetCallback(psfvcb, &psfvcbWrap)))
                    {
                        psfvcb->SetMainCB(psfvcbWrap);
                        psfvcbWrap->Release();
                    }
                    psfvcb->Release();  // Release ownership
                }
                psfv->Release();
            }
        }

        return hres;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        return CFavFolderContextMenu_CreateInstance(this, hwnd, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IShellDetails))
    {
        return CFavoritesDetails_CreateInstance(this, (IShellDetails **)ppv); 
    }

    return m_psfFSFolder->CreateViewObject(hwnd, riid, ppv);
}

STDMETHODIMP CFavorites::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut)
{
    return m_psfFSFolder->GetAttributesOf(cidl, apidl, rgfInOut);
}

STDMETHODIMP CFavorites::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, 
                                       REFIID riid, UINT * prgfInOut, void **ppv)
{
    return m_psfFSFolder->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
}


STDMETHODIMP CFavorites::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
{
    return m_psfFSFolder->GetDisplayNameOf(pidl, uFlags, lpName);
}

STDMETHODIMP CFavorites::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpszName, 
                                   DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    return m_psfFSFolder->SetNameOf(hwndOwner, pidl, lpszName, uFlags, ppidlOut);
}

STDMETHODIMP CFavorites::GetClassID(IN LPCLSID lpClassID)
{
    *lpClassID = CLSID_FavoritesFolder;
    return S_OK;
}

STDMETHODIMP CFavorites::Initialize(IN LPCITEMIDLIST pidl)
{
    m_pidl = ILClone(pidl);
    if (!m_pidl)
        return E_OUTOFMEMORY;

    // Ok we need to get The Shell Folder for the underlying File system Folder...
    IUnknown *punk;
    HRESULT hres = QueryInterface(IID_IUnknown, (void **)&punk);
    if (SUCCEEDED(hres))
    {
        hres = SHCoCreateInstance(NULL, &CLSID_ShellFSFolder, punk,
                                  IID_IUnknown, (void **)&m_punkFSFolder);
        punk->Release();
    }
    if (SUCCEEDED(hres))
    {
        hres = m_punkFSFolder->QueryInterface(IID_IShellFolder, (void **)&m_psfFSFolder);
        if (SUCCEEDED(hres))
        {
            m_psfFSFolder->Release();    // We need to balance back out...
        }
        
        IPersistFolder *pipf;
        hres = m_punkFSFolder->QueryInterface(IID_IPersistFolder, (void **)&pipf);
        if (SUCCEEDED(hres))
        {
            hres = pipf->Initialize(pidl);
            pipf->Release();
        }
    }

    TraceMsg(TF_FAV_FULL, "CFavorites::Initialize End");
    return hres;
}

STDMETHODIMP CFavorites::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(m_pidl, ppidl);
}

int CALLBACK FavExtra_CompareIDs(void * pv1, void * pv2, LPARAM lParam)
{
    IShellFolder *psf = (IShellFolder *)lParam;
    PFAVEXTRA pfehdr1 = (PFAVEXTRA)pv1;
    PFAVEXTRA pfehdr2 = (PFAVEXTRA)pv2;
    HRESULT hres = psf->CompareIDs(0, pfehdr1->pidl, pfehdr2->pidl);

    ASSERT(SUCCEEDED(hres));
    return (short)SCODE_CODE(hres);   // (the short cast is important!)
}

STDMETHODIMP_(BOOL) CFavorites::RegisterFolderViewCB(HWND hwnd, CFavFolderViewCB *pffvcb)
{
    BOOL bRet = TRUE;
    FAVDSA favdsa;

    TraceMsg(TF_FAV_FULL, "CFavorites::RegisterFolderViewCB h:%x p:%x", hwnd, pffvcb);

    if (!m_hdsaFolderViewCBs)
    {
        m_hdsaFolderViewCBs = DSA_Create(sizeof(FAVDSA), 4);
        if (!m_hdsaFolderViewCBs)
            bRet = FALSE;
    }

    if (bRet)
    {
        // Check for any duplicates
        int i = DSA_GetItemCount(m_hdsaFolderViewCBs);

        while (--i >= 0)
        {
            PFAVDSA pfavdsa = (PFAVDSA)DSA_GetItemPtr(m_hdsaFolderViewCBs, i);
            if ((pfavdsa->hwnd == hwnd) || (pfavdsa->pffvcb == pffvcb))
            {
                // Make sure we have one to one...
                pfavdsa->hwnd = hwnd;
                pfavdsa->pffvcb = pffvcb;
                break;
            }
        }

        if (0 > i)
        {
            favdsa.hwnd = hwnd;
            favdsa.pffvcb = pffvcb;
            bRet = (0 <= DSA_AppendItem(m_hdsaFolderViewCBs, &favdsa));
        }
    }
    return bRet;
}

STDMETHODIMP_(BOOL) CFavorites::UnRegisterFolderViewCB(HWND hwnd)
{
    TraceMsg(TF_FAV_FULL, "CFavorites::UnRegisterFolderViewCB h:%x", hwnd);
    if (!m_hdsaFolderViewCBs)
        return FALSE;
    int i = DSA_GetItemCount(m_hdsaFolderViewCBs);
    while (--i >= 0)
    {
        PFAVDSA pfavdsa = (PFAVDSA)DSA_GetItemPtr(m_hdsaFolderViewCBs, i);
        if (pfavdsa->hwnd == hwnd)
        {
            DSA_DeleteItem(m_hdsaFolderViewCBs, i);
            return TRUE;
        }
    }

    return FALSE;
}

STDMETHODIMP_(CFavFolderViewCB *) CFavorites::QueryFolderViewCBFromHwnd(HWND hwnd)
{
    TraceMsg(TF_FAV_FULL, "CFavorites::QueryFolderViewCBFromHwnd h:%x", hwnd);
    if (!m_hdsaFolderViewCBs)
        return NULL;
    int i = DSA_GetItemCount(m_hdsaFolderViewCBs);

    while (--i >= 0)
    {
        PFAVDSA pfavdsa = (PFAVDSA)DSA_GetItemPtr(m_hdsaFolderViewCBs, i);
        if (pfavdsa->hwnd == hwnd)
        {
            pfavdsa->pffvcb->AddRef();  // Increment the usage count just in case...
            return pfavdsa->pffvcb;
        }
    }
    return NULL;
}


/*----------------------------------------------------------
Purpose: This function returns the pointer to a cached structure
         which contains extra data for the given pidl.

Returns: 
Cond:    --
*/
STDMETHODIMP_(PFAVEXTRA) CFavorites::GetPointerToExtraData(LPCITEMIDLIST pidl)
{
    PFAVEXTRA pfe = NULL;

    // Allocate the cache if necessary
    if (!m_hdpaExtra)
        m_hdpaExtra = DPA_Create(8);

    if (m_hdpaExtra)
    {
        FAVEXTRA fe = {(LPITEMIDLIST)pidl, 0};
        int idpa = DPA_Search(m_hdpaExtra, &fe, 0, FavExtra_CompareIDs, (LPARAM)m_psfFSFolder, DPAS_SORTED);

        // Is this pidl's data cached already? 
        if (-1 != idpa)
        {
            // Yes
            pfe = (PFAVEXTRA)DPA_FastGetPtr(m_hdpaExtra, idpa);
            TraceMsg(TF_FAV_CACHE, "GET #%d=%08X (%08X %08X)", idpa, pfe, pfe->pidl, pfe->ppropstgSite);
        }
        else
        {
            STRRET strret;
            TCHAR szPath[MAX_PATH];

            if (SUCCEEDED(m_psfFSFolder->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret)) &&
                SUCCEEDED(StrRetToBuf(&strret, pidl, szPath, SIZECHARS(szPath))))
            {
                WIN32_FIND_DATA finddata;
                HANDLE hfind = FindFirstFile(szPath, &finddata);
                if (INVALID_HANDLE_VALUE != hfind)
                {
                    FindClose(hfind);
                    pfe = (PFAVEXTRA)LocalAlloc(LPTR, SIZEOF(*pfe));
                    if (pfe)
                    {
                        pfe->pidl = ILClone(pidl);
                        pfe->ftVisit = finddata.ftLastAccessTime;

                        if (pidl)
                            OpenPropertyStorage(pidl, pfe);

                        // Cache it
                        DPA_SortedInsertPtr(m_hdpaExtra, pfe, 0, FavExtra_CompareIDs, (LPARAM)m_psfFSFolder, DPAS_INSERTBEFORE, pfe);
                        TraceMsg(TF_FAV_CACHE, "ADD #%d=%08X (%08X %08X)", DPA_GetPtrCount(m_hdpaExtra)-1, pfe, pfe->pidl, pfe->ppropstgSite);
                    }
                }
            }
        }
    }

    return pfe;
}


/*----------------------------------------------------------
Purpose: Return the visit count

Returns: 
Cond:    --
*/
STDMETHODIMP_(DWORD) CFavorites::GetVisitCount(LPCITEMIDLIST pidl)
{
    DWORD dwRet = 0;
    PFAVEXTRA pfe = GetPointerToExtraData(pidl);
    PROPVARIANT propvar;

    if (FavExtra_IsValidSite(pfe) && 
        SUCCEEDED(ReadProp(pfe->ppropstgSite, PID_INTSITE_VISITCOUNT, &propvar)))
    {
        if ( VT_UI4 == propvar.vt )
            dwRet = propvar.ulVal;

        PropVariantClear( &propvar );
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Return the info for the Splat(expected to be a string
         value) and copy its value to the supplied buffer.

Returns:
    S_OK if we should splat.
    S_FALSE if we should not splat.
    E_FAIL if we're screwed.

Cond:    --
*/
STDMETHODIMP CFavorites::GetSplatInfo(LPCITEMIDLIST pidl)
{
    HRESULT hres = E_FAIL;
    PROPVARIANT propvar;
    PFAVEXTRA pfe = GetPointerToExtraData(pidl);

    if (FavExtra_IsValidSite(pfe) &&
        SUCCEEDED(ReadProp(pfe->ppropstgSite, PID_INTSITE_FLAGS, &propvar)))
    {
        if ((VT_UI4 == propvar.vt) && (propvar.ulVal & PIDISF_RECENTLYCHANGED))
        {
            hres = S_OK;    // recently changed
        }
        else
        {
            hres = S_FALSE; // not recently changed
        }

        PropVariantClear( &propvar );
    }
    return hres;
}

/*----------------------------------------------------------
Purpose: Return the specified property (expected to be a string
         value) and copy its value to the supplied buffer.

Returns: 
Cond:    --
*/
STDMETHODIMP CFavorites::GetStringProp(LPCITEMIDLIST pidl, PROPID propid, LPTSTR pszBuf, DWORD cchBuf)
{
    HRESULT hres = E_FAIL;
    PFAVEXTRA pfe = GetPointerToExtraData(pidl);
    PROPVARIANT propvar;

    ASSERT(pszBuf);
    ASSERT(0 < cchBuf);

    *pszBuf = 0;

    if (FavExtra_IsValidSite(pfe))
    {
        hres = ReadProp(pfe->ppropstgSite, propid, &propvar);
        if (SUCCEEDED(hres))
        {
            if (VT_LPWSTR == propvar.vt)
                SHUnicodeToTChar(propvar.pwszVal, pszBuf, cchBuf);
            else if (VT_LPSTR == propvar.vt)
                SHAnsiToTChar(propvar.pszVal, pszBuf, cchBuf);
            PropVariantClear(&propvar);
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Return the specified property (expected to be a filetime
         value) and copy a displayable version to the supplied buffer.

Returns: 
Cond:    --
*/
STDMETHODIMP CFavorites::GetFileTimeProp(LPCITEMIDLIST pidl, PROPID propid, FILETIME * pft)
{
    HRESULT hres = E_FAIL;
    PFAVEXTRA pfe = GetPointerToExtraData(pidl);
    PROPVARIANT propvar;

    pft->dwLowDateTime = 0;
    pft->dwHighDateTime = 0;

    if (FavExtra_IsValidSite(pfe))
    {
        hres = ReadProp(pfe->ppropstgSite, propid, &propvar);
        if (SUCCEEDED(hres))
        {
            if (VT_FILETIME == propvar.vt)
            {
                *pft = propvar.filetime;
            }
            
            PropVariantClear(&propvar);
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Destroy the cache of extra data items

Returns: 
Cond:    --
*/
STDMETHODIMP_(void) 
CFavorites::ClearCachedData(void)
{
    TraceMsg(TF_FAV_CACHE, "CLEAR start");
    if (m_hdpaExtra)
    {
        int idpa = DPA_GetPtrCount(m_hdpaExtra);
        PFAVEXTRA pfe;
        TraceMsg(TF_FAV_CACHE, "CLEAR count = %d", idpa);
        while (--idpa >= 0)
        {
            pfe = (PFAVEXTRA)DPA_FastGetPtr(m_hdpaExtra, idpa);
            ASSERT(pfe);
            TraceMsg(TF_FAV_CACHE, "CLEAR GET #%d=%08X (%08X %08X)", idpa, pfe, pfe->pidl, pfe->ppropstgSite);

            if (pfe->pidl)
                ILFree(pfe->pidl);

            if (pfe->ppropstgSite)
                pfe->ppropstgSite->Release();

            LocalFree(pfe);
        }
        DPA_Destroy(m_hdpaExtra);
        m_hdpaExtra = NULL;
    }
    TraceMsg(TF_FAV_CACHE, "CLEAR end");
}


//==========================================================================================
// CFavFolderViewCB Implemention
//==========================================================================================

/*----------------------------------------------------------
Purpose: Helper to add to the view menu

Returns: 
Cond:    --
*/
UINT 
_AddArrangeMenuItems(
    HMENU hmenu, 
    int id)
{
    MENUITEMINFO mii = {sizeof(MENUITEMINFO), MIIM_SUBMENU};

    if (!GetMenuItemInfo(hmenu, SFVIDM_MENU_ARRANGE, FALSE, &mii))
        return 0;

    TCHAR   szTemp[128];    // Should not be any menu items bigger than this...

    // So lets add our own ones in
    int i;
    HMENU hSubMenu = mii.hSubMenu;
    for (i=0;;i++)
    {
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.cch = 0;    // ????
        if (!GetMenuItemInfo(hSubMenu, i, TRUE, &mii) ||
                (mii.fType == MFT_SEPARATOR))
            break;
    }

    // Will need to loop for all columns added, but for now one will
    // do...
    TraceMsg(TF_FAV_FULL, "CFavFolderContextMenu::HandleMergeMenu Add Ours starting at %d", id);
    LoadString(g_hinst, IDS_FAV_COL_SORT_BY_LAST_VISIT,
            szTemp, ARRAYSIZE(szTemp));
    InsertMenu(hSubMenu, i, MF_BYPOSITION, id, szTemp);

    return 1;
}


CFavFolderViewCB::CFavFolderViewCB(CFavorites *pFav, IShellFolderView * psfv) : m_cRef(1)
{
    m_pFav = pFav;
    m_pFav->AddRef();

    m_psfv = psfv;
    m_psfvcb = NULL;
    m_hwndMain = NULL;
}

CFavFolderViewCB::~CFavFolderViewCB(void)
{
    m_pFav->UnRegisterFolderViewCB(m_hwndMain);
    m_pFav->Release();

    if ( m_psfvcb )
        m_psfvcb->Release();
        
    if ( m_psdFolder )
        m_psdFolder->Release();
}


STDMETHODIMP_(void) CFavFolderViewCB::SetMainCB(IN IShellFolderViewCB *psfvcb)
{
    if (m_psfvcb)
        m_psfvcb->Release();
    m_psfvcb = psfvcb;
    if (m_psfvcb)
        m_psfvcb->AddRef();
}


STDMETHODIMP CFavFolderViewCB::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFavFolderViewCB, IShellFolderViewCB), // IID_IShellFolderViewCB
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFavFolderViewCB::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFavFolderViewCB::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFavFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;

    if (m_psfvcb)
    {
        switch (uMsg)
        {
        case SFVM_MERGEMENU:
        {
            QCMINFO *pqcmi = (QCMINFO*)lParam;
            UINT idCmdFirst = pqcmi->idCmdFirst;
            hres = m_psfvcb->MessageSFVCB(uMsg, wParam, lParam);
            if (SUCCEEDED(hres))
            {
                HandleMergeMenu(idCmdFirst, pqcmi);
            }
            return hres;
        }

        case SFVM_INVOKECOMMAND:
            TraceMsg(TF_FAV_FULL, "CFavFolderViewCB::MessageSFVCB Invoke C:%x", wParam);
            if (wParam == m_idCmdFirstAdded)
            {
                m_psfv->Rearrange(m_pFav->m_iFirstAddedCol);
                return NOERROR;
            }
            break;

        case SFVM_HWNDMAIN:
            m_hwndMain = (HWND)lParam;
            m_pFav->RegisterFolderViewCB(m_hwndMain, this);
            break;

        case SFVM_REFRESH:
            // post refresh....
            if ( (BOOL) wParam == FALSE )
                m_pFav->ClearCachedData();
            break;

        case SFVM_GETVIEWS:
            // get the views supported
            return GetViews( (SHELLVIEWID *) wParam, (IEnumSFVViews **) lParam );
        }

        hres = m_psfvcb->MessageSFVCB(uMsg, wParam, lParam);
    }
    else
        hres = E_NOTIMPL;
    return hres;
}

STDMETHODIMP CFavFolderViewCB::GetDetailsOf(int iCol, DETAILSINFO * pdi)
{
    SHELLDETAILS rgDetails;
    HRESULT hr = NOERROR;

    if ( ! m_psdFolder )
    {
        // cache the IShellDetails ...
        hr = m_pFav->m_psfFSFolder->CreateViewObject( NULL, IID_IShellDetails, (void **) & m_psdFolder );
        if ( FAILED( hr ))
        {
            return hr;
        }
    }
    
    hr = m_pFav->GetDetailsOf(m_psdFolder, pdi->pidl, iCol, &rgDetails);
    if ( SUCCEEDED( hr ))
    {
        pdi->fmt = rgDetails.fmt;
        pdi->cxChar = rgDetails.cxChar;
        pdi->str = rgDetails.str;
    }

    return hr;
}

STDMETHODIMP_(void) CFavFolderViewCB::HandleMergeMenu(UINT idCmdFirstIn, QCMINFO * pqcmi)
{
    TraceMsg(TF_FAV_FULL, "CFavorites::HandleMergeMenu h:%x im=%x F:%x L:%x",
            pqcmi->hmenu, pqcmi->indexMenu, pqcmi->idCmdFirst, pqcmi->idCmdLast);
    UINT cCmdAdded = _AddArrangeMenuItems(pqcmi->hmenu, pqcmi->idCmdFirst);
    if (cCmdAdded)
    {
        m_idCmdFirstAdded =  pqcmi->idCmdFirst - idCmdFirstIn;
        pqcmi->idCmdFirst += cCmdAdded;
    }
}

STDMETHODIMP CFavFolderViewCB::GetViews( SHELLVIEWID * pDefID, IEnumSFVViews ** ppEnum )
{
    if ( !ppEnum || !pDefID )
    {
        return E_INVALIDARG;
    }
    
    HRESULT hr = m_psfvcb->MessageSFVCB( SFVM_GETVIEWS, (WPARAM) pDefID, (LPARAM) ppEnum );
    
    IEnumSFVViews * pStdEnum = *ppEnum;
    
    // create the wrapper and use it....
    HRESULT hr2 = CFavViewEnum_Create( *ppEnum, ppEnum );

    if ( SUCCEEDED( hr2 ) && pStdEnum )
    {
        // we have a new one, so release the ref on the old one (the wrapper
        // also has a ref)
        pStdEnum->Release();
    }

    if ( SUCCEEDED( hr ) && FAILED( hr2 ))
    {
        *ppEnum = pStdEnum;
    }
    if ( FAILED( hr ) && SUCCEEDED( hr2 ))
    {
        hr = hr2;
    }

    return hr;
}


CFavFolderContextMenu::CFavFolderContextMenu(CFavorites *pFav, HWND hwnd, HRESULT *phres) : m_cRef ( 1 )
{
    IContextMenu *pcmToWrap;
    *phres = pFav->m_psfFSFolder->CreateViewObject(hwnd, IID_IContextMenu, (void **)&pcmToWrap);
    if (SUCCEEDED(*phres))
    {
        m_hwndOwner = hwnd;

        m_pFav = pFav;
        m_pFav->AddRef();

        *phres = pcmToWrap->QueryInterface(IID_IContextMenu2, (void **)&m_pcmInner);
        m_idCmdFirst =  0;

        pcmToWrap->Release();
    }
}

HRESULT CFavFolderContextMenu_CreateInstance(CFavorites *pFav, HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    HRESULT hres;
    CFavFolderContextMenu *pMenu = new CFavFolderContextMenu(pFav, hwnd, &hres);
    if (pMenu)
    {
        if (SUCCEEDED(hres))
            hres = pMenu->QueryInterface(riid, ppv);
        pMenu->Release();
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}

CFavFolderContextMenu::~CFavFolderContextMenu(void)
{
    if (m_pFav)
        m_pFav->Release();
    if (m_pcmInner)
        m_pcmInner->Release();
}

STDMETHODIMP CFavFolderContextMenu::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFavFolderContextMenu, IObjectWithSite),   // IID_IObjectWithSite
        QITABENT(CFavFolderContextMenu, IContextMenu),  // IID_IContextMenu
        QITABENTMULTI(CFavFolderContextMenu, IContextMenu, IContextMenu2),  // IID_IContextMenu2
        QITABENTMULTI(CFavFolderContextMenu, IContextMenu2, IContextMenu3),  // IID_IContextMenu3
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFavFolderContextMenu::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFavFolderContextMenu::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFavFolderContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                                     UINT idCmdLast,  UINT uFlags)
{
    HRESULT hres = m_pcmInner->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    if (SUCCEEDED(hres))
    {
        UINT cCmdAdded;
        idCmdFirst += (SHORT)hres;
        m_idCmdFirst =  (SHORT)hres;
        cCmdAdded = _AddArrangeMenuItems(hmenu, idCmdFirst);
        return ResultFromShort((SHORT)hres + cCmdAdded);
    }
    return hres;
}

STDMETHODIMP CFavFolderContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    TraceMsg(TF_FAV_FULL, "CFavFolderContextMenu::InvokeCommand called %x", lpici->lpVerb);
    if (IS_INTRESOURCE(lpici->lpVerb))
    {
        UINT idCmd = LOWORD(lpici->lpVerb);
        if (idCmd == m_idCmdFirst)
        {
            // It is ours...
            TraceMsg(TF_FAV_FULL, "CFavFolderContextMenu::InvokeCommand SortByLastVisit");
            CFavFolderViewCB *pffvcb = m_pFav->QueryFolderViewCBFromHwnd(m_hwndOwner);
            if (pffvcb)
            {
                // Maybe should do this cleaner...
                pffvcb->m_psfv->Rearrange(m_pFav->m_iFirstAddedCol);
                pffvcb->Release();  // release our usage of it...
            }
        }
    }
    return m_pcmInner->InvokeCommand(lpici);
}

STDMETHODIMP CFavFolderContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT * pwReserved, LPSTR pszName, UINT cchMax)
{
    return m_pcmInner->GetCommandString(idCmd, uType, pwReserved, pszName, cchMax);
}

STDMETHODIMP CFavFolderContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return m_pcmInner->HandleMenuMsg(uMsg, wParam, lParam);
}

STDMETHODIMP CFavFolderContextMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    IContextMenu3 *pcm3;
    HRESULT hres = m_pcmInner->QueryInterface(IID_IContextMenu3, (void **)&pcm3);
    if (SUCCEEDED(hres))
    {
        hres = pcm3->HandleMenuMsg2(uMsg, wParam, lParam, plres);
        pcm3->Release();
    }
    return hres;
}

HRESULT CFavFolderContextMenu::SetSite(IUnknown* pUnk)
{
    return IUnknown_SetSite(m_pcmInner, pUnk);
}

HRESULT CFavFolderContextMenu::GetSite(REFIID riid, void**ppv)
{
    return IUnknown_GetSite(m_pcmInner, riid, ppv);
}

HRESULT CFavViewEnum_Create( IEnumSFVViews * pDefEnum, IEnumSFVViews ** ppEnum )
{
    CFavViewEnum * pViewEnum = new CFavViewEnum( pDefEnum );
    if ( !pViewEnum )
    {
        return E_OUTOFMEMORY;
    }

    *ppEnum = SAFECAST(pViewEnum, IEnumSFVViews *);
    return NOERROR;
}

CFavViewEnum::CFavViewEnum(IEnumSFVViews * pEnum) : m_cRef(1)
{
    m_pesfvv = pEnum;
    if ( pEnum )
        pEnum->AddRef();

    m_iAddView = 0;
}

CFavViewEnum::~CFavViewEnum()
{
    if ( m_pesfvv )
        m_pesfvv->Release();
}

STDMETHODIMP CFavViewEnum::QueryInterface (REFIID riid, void ** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFavViewEnum, IEnumSFVViews),  // IID_IEnumSFVViews
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFavViewEnum::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFavViewEnum::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFavViewEnum::Next(ULONG celt, SFVVIEWSDATA **ppData, ULONG *pceltFetched)
{
    ULONG celtFetched = 0;
    
    if ( !celt || !ppData || (celt > 1 && !pceltFetched ))
    {
        return E_INVALIDARG;
    }

    if ( !m_iAddView )
    {
        // return the Thumbnail view first 
        ppData[0] = (SFVVIEWSDATA *) SHAlloc(sizeof(SFVVIEWSDATA));
        if ( !ppData[0] )
        {
            return E_OUTOFMEMORY;
        }

        ppData[0]->idView = CLSID_ThumbnailViewExt;
        ppData[0]->idExtShellView = CLSID_ThumbnailViewExt;
        ppData[0]->dwFlags = SFVF_TREATASNORMAL | SFVF_NOWEBVIEWFOLDERCONTENTS;
        ppData[0]->lParam = 0x00000011;
        ppData[0]->wszMoniker[0] = 0;

        celtFetched ++;
        m_iAddView ++;
    }

    ULONG celtAditional = 0;
    HRESULT hr = S_FALSE;
    
    if ( celt - celtFetched > 0 && m_pesfvv )
    {
        hr = m_pesfvv->Next( celt - celtFetched,
                             ppData + celtFetched,
                             & celtAditional );
        celtFetched += celtAditional;
    }

    if (SUCCEEDED( hr ))
    {

        if ( celtFetched == celt )
        {
            hr = S_OK;
        }
    }
    else if ( celtFetched )
    {
        hr = S_FALSE;
    }

    if ( pceltFetched )
    {
        *pceltFetched = celtFetched;
    }
    
    return hr;
}

STDMETHODIMP CFavViewEnum::Skip(ULONG celt)
{
    if ( celt && !m_iAddView )
    {
        m_iAddView ++;
        celt --;
    }

    if ( celt && m_pesfvv )
    {
        return m_pesfvv->Skip( celt );
    }

    return (celt ? S_FALSE : S_OK );
}

STDMETHODIMP CFavViewEnum::Reset()
{
    m_iAddView = 0;

    if ( m_pesfvv )
    {
        m_pesfvv->Reset();
    }

    return NOERROR;
}

STDMETHODIMP CFavViewEnum::Clone(IEnumSFVViews **ppenum)
{
    IEnumSFVViews * pNewEnum = NULL;
    HRESULT hr = NOERROR;

    if ( m_pesfvv )
    {
        hr = m_pesfvv->Clone( & pNewEnum );
        if ( FAILED( hr ))
        {
            return hr;
        }
    }
    
    hr = CFavViewEnum_Create( pNewEnum, ppenum );
    if ( FAILED( hr ) && pNewEnum )
    {
        pNewEnum->Release();
    }
    return hr;
}

HRESULT CFavoritesDetails_CreateInstance(CFavorites * pFav, IShellDetails **ppv)
{
    HRESULT hr;
    CFavoritesDetails * pDetails = new CFavoritesDetails( pFav, &hr );
    if (pDetails)
    {
        if (SUCCEEDED(hr))
            *ppv = SAFECAST(pDetails, IShellDetails *);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

CFavoritesDetails::CFavoritesDetails( CFavorites * pFav, HRESULT * pHr) : m_cRef ( 1 )
{
    *pHr = pFav->m_psfFSFolder->CreateViewObject( NULL, IID_IShellDetails, (void **) &m_psdFolder );
    if (SUCCEEDED(*pHr))
    {
        // hold a ref to the favorites object.
        // we use the punk above to hold the ref...
        m_pFav = pFav;
        m_pFav->AddRef();
        pFav->m_iFirstAddedCol = -1;    // BUGBUG: per view data in the folder!
    }
}

CFavoritesDetails::~CFavoritesDetails()
{
    if ( m_psdFolder )
        m_psdFolder->Release();
    if ( m_pFav )
        m_pFav->Release();
}

STDMETHODIMP CFavoritesDetails::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFavoritesDetails, IShellDetails), // IID_IShellDetails
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_( ULONG ) CFavoritesDetails::AddRef ()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_( ULONG ) CFavoritesDetails::Release ()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;
    delete this;
    return 0;
}

STDMETHODIMP CFavoritesDetails::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    return m_pFav->GetDetailsOf( m_psdFolder, pidl, iColumn, pDetails );
}

STDMETHODIMP CFavorites::GetDetailsOf(IShellDetails *psd, LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    TCHAR szTmp[MAX_PATH];
    PROPID pid;
    FILETIME ft;

    // either we are an added column, or we don't know the added columns number
    if (((int) iColumn) < m_iFirstAddedCol || m_iFirstAddedCol == -1 )
    {
        HRESULT hres = psd->GetDetailsOf(pidl, iColumn, pDetails);

        if ( SUCCEEDED( hres ) || m_iFirstAddedCol != -1)
            return hres;

        m_iFirstAddedCol = iColumn;
    }

        
    iColumn -= m_iFirstAddedCol;
    
    /// empty string
    szTmp[0] = 0;
    
    if (iColumn >= ARRAYSIZE(c_fav_cols))
        return E_FAIL;   // Some error code.

    if (!pidl)
    {
        LoadString(g_hinst, c_fav_cols[iColumn].ids, szTmp, SIZECHARS(szTmp));
        pDetails->fmt = c_fav_cols[iColumn].iFmt;
        pDetails->cxChar = c_fav_cols[iColumn].cchCol;
    }
    else
    {
        // Need to fill in the details
        switch (iColumn)
        {
        case ICOL_FAV_NUMVISITS:
            {
                DWORD dwNumVisits = this->GetVisitCount(pidl);
                if (dwNumVisits)
                    wsprintf(szTmp, TEXT("%u"), dwNumVisits);
            }
            break;

        case ICOL_FAV_LASTVISIT:
            pid = PID_INTSITE_LASTVISIT;
            goto FinishFileTime;

        case ICOL_FAV_LASTMOD:
            pid = PID_INTSITE_LASTMOD;
        FinishFileTime:
            if (SUCCEEDED(this->GetFileTimeProp(pidl, pid, &ft)))
                SHFormatDateTime(&ft, NULL, szTmp, SIZECHARS(szTmp));
            break;

        case ICOL_FAV_SPLAT:
            if (this->GetSplatInfo(pidl) == S_OK)
                LoadString(g_hinst, IDS_FAV_SPLAT, szTmp, SIZECHARS(szTmp));
            break;

        default:
            return E_FAIL;
        }
    }
    return StringToStrRet(szTmp, &pDetails->str);
}

STDMETHODIMP CFavoritesDetails::ColumnClick(UINT iColumn)
{
    return S_FALSE;     // do default
}

STDAPI Favorites_Install(BOOL bInstall)
{
    TCHAR szPath[MAX_PATH];

    // get the path to the favorites folder. Create it if it is missing and we
    // are installing, otherwise don't bother.

    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, bInstall))
    {
        if (bInstall)
        {
            // BUGBUG kenwic 060399 #342156 We must nuke the ExtShellFolderView section to recover from an IE4 bug FIXED kenwic 060399
            SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_CLSID | FCSM_VIEWID, 0};
            fcs.pclsid = (CLSID *)&CLSID_FavoritesFolder;   // const -> non const
            fcs.pvid = NULL;
            SHGetSetFolderCustomSettings(&fcs, szPath, FCS_FORCEWRITE);
        }
        else
        {
            PathUnmakeSystemFolder(szPath);
        }
    }
    return NOERROR; 
}
