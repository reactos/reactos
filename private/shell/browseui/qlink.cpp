#include "priv.h"
#include "sccls.h"
#include "itbar.h"
#include "itbdrop.h"
#include "qlink.h"
#include "resource.h"
#include "../lib/dpastuff.h"

#include "bands.h"
#include "isfband.h"
#include "bandprxy.h"
#include "uemapp.h"

#include "mluisupp.h"

#define SUPERCLASS CISFBand

#define DM_PERSIST      DM_TRACE        // trace IPS::Load, ::Save, etc.


class CQuickLinks : public CISFBand
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CISFBand::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return CISFBand::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IPersistStream methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pstm, BOOL fClearDirty);
    
    // *** IDockingWindow methods (override) ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw) { return CISFBand::CloseDW(dw); };
    
    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite) { return CISFBand::SetSite(punkSite); };

    // *** IOleCommandTarget ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                              DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                              VARIANTARG *pvarargOut);
    
    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IDelegateDropTarget ***
    virtual HRESULT OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);
protected:    
    CQuickLinks();
    virtual ~CQuickLinks();

    HRESULT _GetTitleW(LPWSTR pwzTitle, DWORD cchSize);
    HRESULT _InternalInit(void);

    virtual HRESULT _LoadOrderStream();
    virtual HRESULT _SaveOrderStream();
    virtual BOOL    _AllowDropOnTitle() { return TRUE; };
    virtual HRESULT _GetIEnumIDList(DWORD dwEnumFlags, IEnumIDList **ppenum);

private:    
    BITBOOL    _fIsInited :1;
    BITBOOL    _fSingleLine :1;

    friend HRESULT CQuickLinks_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);    // for ctor
};

#define MAX_QL_SITES            5   // Number of Sites on the quick link bar

#define SZ_REGKEY_SPECIALFOLDERS  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")

HRESULT SHGetSpecialFolderPathEx(LPTSTR pszPath, DWORD cchSize, DWORD dwCSIDL, LPCTSTR pszFolderName)
{
    HRESULT hr = S_OK;
    
    if (SHGetSpecialFolderPath(NULL, pszPath, CSIDL_FAVORITES, TRUE))
        return hr;

    cchSize *= sizeof(TCHAR);   // Count of chars to count of bytes.
    if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_SPECIALFOLDERS, pszFolderName, NULL, pszPath, &cchSize))
        hr = E_FAIL;

    TraceMsg(TF_BAND|TF_GENERAL, "CQuickLinks SHGetSpecialFolderPath(CSIDL_FAVORITES), Failed so getting Fav dir from registry. Path=%s; hr=%#8lx", pszPath, hr);
    
    return hr;
}

#define LINKS_FOLDERNAME_KEY   TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar")
#define LINKS_FOLDERNAME_VALUE TEXT("LinksFolderName")

// _GetTitleW and QuickLinks_GetFolder call this. 
// if we ever go back to ANSI days we'll get a build break
// right now we are saving some space by not having A version that's not used
void QuickLinks_GetName(LPTSTR pszName, DWORD cchSize, BOOL bSetup)
{
    DWORD cb = cchSize * SIZEOF(TCHAR);
    // try to get the name of the folder from the registry (in case of upgrade to a different
    // language we cannot use the resource)
    if (SHGetValue(HKEY_CURRENT_USER, LINKS_FOLDERNAME_KEY, LINKS_FOLDERNAME_VALUE, NULL, (void *)pszName, &cb) != ERROR_SUCCESS)
    {
        // no luck, try the HKLM if we are doing per user registration, maybe setup stored the old links folder name there
        cb = cchSize * SIZEOF(TCHAR);
        if (!bSetup || SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), TEXT("LinkFolderName"), NULL, (void *)pszName, &cb) != ERROR_SUCCESS)
        {
            // if everything else fails load it from the resource
            MLLoadString(IDS_QLINKS, pszName, cchSize);
        }
    }
}

HRESULT QuickLinks_GetFolder(LPTSTR pszPath, DWORD cchSize, BOOL bSetup = FALSE)
{
    TCHAR szQuickLinks[MAX_PATH];

    if (SUCCEEDED(SHGetSpecialFolderPathEx(pszPath, cchSize, CSIDL_FAVORITES, TEXT("Favorites"))))
    {
        QuickLinks_GetName(szQuickLinks, ARRAYSIZE(szQuickLinks), bSetup);
        PathCombine(pszPath, pszPath, szQuickLinks);
        return S_OK;
    }    
    
    return E_FAIL;
}

#define QL_BUFFER (MAX_QL_TEXT_LENGTH + MAX_URL_STRING + MAX_TOOLTIP_STRING + 3)

//
// Load strings needed for quick links
//
// returns TRUE/FALSE if it was user specified (false used to load the default
// urls, but now we leave that to branding dll
BOOL QLLoadLinkName(HUSKEY hUSKey, int i, LPTSTR pszTitle, UINT cchTitle, LPTSTR pszURL, UINT cchURL)
{
    CHAR szScratch[QL_BUFFER];
    DWORD dwcbData = SIZEOF(szScratch);
    CHAR * pszTemp;
    TCHAR szRegValues[5];

    // In IE3, links did not have its own folder.  Instead, links were stored in the registry as a
    // set of binary streams of ANSI strings.
    wnsprintf(szRegValues, ARRAYSIZE(szRegValues), TEXT("%d"), i+1);
    if (hUSKey && 
        (ERROR_SUCCESS == SHRegQueryUSValue(hUSKey, szRegValues, NULL, (LPBYTE)szScratch, &dwcbData, FALSE, NULL, 0)))
    {
        int nNULLs = 0;
        pszTemp = szScratch;
        DWORD j;
        for (j = 0; j < dwcbData; j++)
        {
#ifdef MAINWIN
            // Because of the limitations of the MainWin registry, we'll put '*' instead of '\0'.
            if (*pszTemp == TEXT('*'))
                *pszTemp = '\0';
#endif
            nNULLs += (UINT)(*pszTemp++ == TEXT('\0'));
        }

        // make sure we have 3 strings with a double NULL at the end
        if (nNULLs > 3)
        {
            pszTemp = szScratch;
            SHAnsiToTChar(pszTemp, pszTitle, cchTitle);
            pszTemp += lstrlenA(pszTemp) + 1;
            SHAnsiToTChar(pszTemp, pszURL, cchURL);
            return TRUE;
        }
    }
    return FALSE;
}

void ImportQuickLinks()
{
    TCHAR szQuickLinksDir[MAX_PATH];

    if (FAILED(QuickLinks_GetFolder(szQuickLinksDir, ARRAYSIZE(szQuickLinksDir), TRUE)))
        return;
        
    // need to write the folder name to the registry so we can use it w/ different plug ui languages
    LPTSTR pszQLinks;
    DWORD cb;

    PathRemoveBackslash(szQuickLinksDir);
    pszQLinks = PathFindFileName(szQuickLinksDir);
    if (pszQLinks)
    {
        cb = (lstrlen(pszQLinks)+1) * sizeof(TCHAR);
        SHSetValue(HKEY_CURRENT_USER, LINKS_FOLDERNAME_KEY, LINKS_FOLDERNAME_VALUE, REG_SZ, (void *)pszQLinks, cb);
    }

    if (!PathFileExists(szQuickLinksDir) &&
        CreateDirectory(szQuickLinksDir, NULL))
    {
        HUSKEY hUSKey = NULL;

        SHRegOpenUSKey(TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar\\Links"), KEY_READ, NULL, &hUSKey, FALSE);

        for (int i = 0; i < MAX_QL_SITES; i++)
        {
            // this was user specified...  convert it
            LPITEMIDLIST pidl;
            TCHAR szURLTemp[MAX_URL_STRING];
            TCHAR szTitle[MAX_QL_TEXT_LENGTH];

            if (QLLoadLinkName(hUSKey, i, szTitle, ARRAYSIZE(szTitle), szURLTemp, ARRAYSIZE(szURLTemp)))
            {
                WCHAR szURL[MAX_URL_STRING];
                if (SUCCEEDED(URLSubstitution(szURLTemp, szURL, ARRAYSIZE(szURL), URLSUB_ALL)) &&
                    SUCCEEDED(IECreateFromPath(szURL, &pidl)))
                {
                    CreateShortcutInDir(pidl, szTitle, szQuickLinksDir, NULL, FALSE);
                    ILFree(pidl);
                }
            }
        }
        // we found the key, there's something to migrate
        if (hUSKey)
            SHRegCloseUSKey(hUSKey);

        // all converted, delete the key
        SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar\\Links"));
    }
    else
    {
        // ie4 -> ieX upgrade
        // create a value in hkcu\sw\ms\ie\toolbar to indicate that we should preserve the order from the links stream
        // not the one from the favorites\links that we are using for links starting w/ ie5
        BOOL bVal = TRUE;
        // we don't care if this fails. if it does we'll just just favorites\links order stream
        SHSetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar"), 
                   TEXT("SaveLinksOrder"), REG_BINARY, (DWORD *)&bVal, SIZEOF(bVal));
    }
}

HRESULT CQuickLinks::_InternalInit(void)
{
    if (!_fIsInited && !_psf)
    {
        LPITEMIDLIST pidlQLinks;
        TCHAR szPath[MAX_PATH];

        QuickLinks_GetFolder(szPath, ARRAYSIZE(szPath));
        if (!PathFileExists(szPath))
            CreateDirectory(szPath, NULL);
        if (SUCCEEDED(IECreateFromPath(szPath, &pidlQLinks)))
        {
            InitializeSFB(NULL, pidlQLinks);
            ILFree(pidlQLinks);
        }
    }
    _fIsInited = TRUE;

    return S_OK;
}

CQuickLinks::CQuickLinks() :
    SUPERCLASS()
{
    
#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in CQuickLinks ctor");
        DEBUG_BREAK;
    }
#endif
    
    ASSERT(!_fIsInited);
    _fCascadeFolder = TRUE;
    _fVariableWidth = TRUE;

    _pguidUEMGroup = &UEMIID_BROWSER;
    
}

CQuickLinks::~CQuickLinks()
{
}

STDAPI CQuickLinks_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    CQuickLinks * p = new CQuickLinks();
    if (p) 
    {
        *ppunk = SAFECAST(p, IDeskBand *);
        return NOERROR;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CQuickLinks::_LoadOrderStream()
{
    HRESULT hr = E_FAIL;

    if (_pidl && _psf) {
        IStream* pstm = OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, _pidl, REG_SUBKEY_FAVORITESA, STGM_READ);

        if (pstm) {
            OrderList_Destroy(&_hdpaOrder);

            hr = OrderList_LoadFromStream(pstm, &_hdpaOrder, _psf);

            pstm->Release();
        }
    }

    return hr;
}

HRESULT CQuickLinks::_SaveOrderStream()
{
    HRESULT hr = E_FAIL;

    if (_pidl && (_hdpa || _hdpaOrder)) {
        IStream* pstm = OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, _pidl, REG_SUBKEY_FAVORITESA, STGM_CREATE | STGM_WRITE);

        if (pstm) {
            hr = OrderList_SaveToStream(pstm, (_hdpa ? _hdpa : _hdpaOrder), _psf);

            pstm->Release();
        }
    }

    if (SUCCEEDED(hr))
        hr = SUPERCLASS::_SaveOrderStream();

    return hr;
}

HRESULT CQuickLinks::_GetIEnumIDList(DWORD dwEnumFlags, IEnumIDList **ppenum)
{
    HRESULT hres;
    
    ASSERT(_psf);
    // Pass in a NULL hwnd so the enumerator does not show any UI while
    // we're filling a band.    
    hres = IShellFolder_EnumObjects(_psf, NULL, dwEnumFlags, ppenum);
    // we could have failed because our folder does not exist
    // that can happen if someone delted/renamed links while there is 
    // stream in the registry that saves the pidl - we get the pidl and
    // bind to it (bind does not hit the disk so it succeeds even though
    // the file does not exist
    if (FAILED(hres) && hres != E_OUTOFMEMORY)
    {
        TCHAR szPath[MAX_PATH];

        ASSERT(_pidl);
        if (SHGetPathFromIDList(_pidl, szPath) && !PathFileExists(szPath))
        {
            LPITEMIDLIST pidlQLinks;
            
            QuickLinks_GetFolder(szPath, ARRAYSIZE(szPath));
            if (!PathFileExists(szPath))
                CreateDirectory(szPath, NULL);
            if (SUCCEEDED(IECreateFromPath(szPath, &pidlQLinks)))
            {
                if (SUCCEEDED(InitializeSFB(NULL, pidlQLinks)))
                {
                    hres = _psf->EnumObjects(NULL, dwEnumFlags, ppenum);
                }
                ILFree(pidlQLinks);
            }
        }
    }
    return hres;
}

//*** CQuickLinks::IPersistStream
HRESULT CQuickLinks::Load(IStream *pstm)
{
    HRESULT hr = S_OK;

    hr = SUPERCLASS::Load(pstm);

    // This forces a refresh
    _fIsInited = FALSE;
    ATOMICRELEASE(_psf);
    _InternalInit();



    // if we are on our first run through (i.e. this reg key exists)
    // we load the order stream from our old location (used in ie4) and avoid overwriting it w/ favorites stream
    // so user can have their custom order preserved on upgrade (they are more likely to customize links bar 
    // order then favorites/links so we picked that one)
    if (SHGetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar"), 
                   TEXT("SaveLinksOrder"), NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        SHDeleteValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar"), TEXT("SaveLinksOrder"));
        // must persist old order stream in the new location (fav/links)
        _SaveOrderStream();
    }
    else
    {
        _LoadOrderStream();
    }

    return hr;
}

HRESULT CQuickLinks::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hr = SUPERCLASS::Save(pstm, fClearDirty);

    _SaveOrderStream();

    return hr;
}

HRESULT CQuickLinks::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_QuickLinks;
    return S_OK;
}

// *** IUnknown Interface ***
HRESULT CQuickLinks::QueryInterface(REFIID riid, void **ppvObj)
{
    return SUPERCLASS::QueryInterface(riid, ppvObj);
}

// command target
STDMETHODIMP CQuickLinks::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = S_FALSE;
    if (pguidCmdGroup)
    {
        if (IsEqualGUID(*pguidCmdGroup, CLSID_QuickLinks)) 
        {
            switch (nCmdID) 
            {
            case QLCMD_SINGLELINE:
                _fSingleLine = (nCmdexecopt == 1);
                return S_OK;
            }
        }
        else if (IsEqualGUID(*pguidCmdGroup, CGID_ISFBand))
        {
            switch(nCmdID)
            {
            case ISFBID_SETORDERSTREAM:
                hres = SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                _SaveOrderStream();
                break;
            }
        }
    }

    if (hres ==  S_FALSE)
        hres = SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    return hres;
}

// *** IDockingWindow Interface ***
HRESULT CQuickLinks::ShowDW(BOOL fShow)
{
    if (fShow)
        _InternalInit();

    return SUPERCLASS::ShowDW(fShow);
}

    
HRESULT CQuickLinks::_GetTitleW(LPWSTR pwzTitle, DWORD cchSize)
{
    QuickLinks_GetName(pwzTitle, cchSize, FALSE);
    return S_OK;
}


HRESULT CQuickLinks::GetBandInfo(DWORD dwBandID, DWORD fViewMode, DESKBANDINFO* pdbi) 
{
   HRESULT hres = SUPERCLASS::GetBandInfo(dwBandID, fViewMode, pdbi);
   
   if (_hwndTB && _fSingleLine) {
       LRESULT lButtonSize = SendMessage(_hwndTB, TB_GETBUTTONSIZE, 0, 0L);
       pdbi->ptMinSize.y = HIWORD(lButtonSize);
       
       pdbi->dwModeFlags &= ~DBIMF_VARIABLEHEIGHT;
   }
   return hres;
    
}

HRESULT CQuickLinks::OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr;
    BOOL    fIsSafe = TRUE;

    // if we are not the source of the drop and the Links folder does not exist we need to create it
    if (_iDragSource == -1)
    {
        TCHAR szPath[MAX_PATH];

        QuickLinks_GetFolder(szPath, ARRAYSIZE(szPath));
        if (!PathFileExists(szPath))
            CreateDirectory(szPath, NULL);

        LPITEMIDLIST pidl;

        if (SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0)))
        {
            fIsSafe = IEIsLinkSafe(_hwnd, pidl, ILS_LINK);
            ILFree(pidl);
        }

    }

    if (fIsSafe)
    {
        hr = SUPERCLASS::OnDropDDT(pdt, pdtobj, pgrfKeyState, pt, pdwEffect);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}
