#include "shellprv.h"
#include "caggunk.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"
#include "datautil.h"
#include "shobjprv.h"
#include "control.h"
#include "sfviewp.h"
#include "drives.h"
#include "infotip.h"
#include "prop.h"           // COL_DATA

extern "C"
{
#include "fstreex.h"
}

extern "C" BOOL IsNameListedUnderKey(LPCTSTR pszFileName, LPCTSTR pszKey);

// our pidc type:
typedef struct _IDCONTROL
{
    USHORT  cb;
    int     idIcon;
    USHORT  oName;              // cBuf[oName] is start of NAME
    USHORT  oInfo;              // cBuf[oInfo] is start of DESCRIPTION
    CHAR    cBuf[MAX_PATH+MAX_CCH_CPLNAME+MAX_CCH_CPLINFO]; // cBuf[0] is the start of FILENAME
    USHORT  uTerm;
} IDCONTROL;
typedef UNALIGNED struct _IDCONTROL *LPIDCONTROL;

typedef struct _IDCONTROLW
{
    USHORT  cb;
    int     idIcon;
    USHORT  oName;              // if Unicode .cpl, this will be 0
    USHORT  oInfo;              // if Unicode .cpl, this will be 0
    CHAR    cBuf[2];            // if Unicode .cpl, cBuf[0] = '\0', cBuf[1] = magic byte
    DWORD   dwFlags;            // Unused; for future expansion
    USHORT  oNameW;             // cBufW[oNameW] is start of NAME
    USHORT  oInfoW;             // cBufW[oInfoW] is start of DESCRIPTION
    WCHAR   cBufW[MAX_PATH+MAX_CCH_CPLNAME+MAX_CCH_CPLINFO]; // cBufW[0] is the start of FILENAME
} IDCONTROLW;
typedef UNALIGNED struct _IDCONTROLW *LPIDCONTROLW;

// Unicode IDCONTROLs will be flagged by having oName = 0, oInfo = 0, 
// cBuf[0] = '\0', and cBuf[1] = UNICODE_CPL_SIGNATURE_BYTE

STDAPI ControlExtractIcon_CreateInstance(LPCTSTR pszSubObject, REFIID riid, void **ppv);

class CControlPanelFolder : public CAggregatedUnknown, IShellFolder2, IPersistFolder2
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
                { return CAggregatedUnknown::QueryInterface(riid, ppv); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, LPENUMIDLIST* ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void** ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void** ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH* ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid);

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID* pClassID);
    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    // *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl);

protected:
    CControlPanelFolder(IUnknown* punkOuter);
    ~CControlPanelFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid, void** ppv);

    static void GetExecName(LPIDCONTROL pidc, LPTSTR pszParseName, UINT cchParseName);
    static HRESULT GetModuleMapped(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule,
                                   UINT* pidNewIcon, LPTSTR pszApplet, UINT cchApplet);
    static void GetDisplayName(LPIDCONTROL pidc, LPTSTR pszName, UINT cchName);
    static void GetModule(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule);
    static void _GetDescription(LPIDCONTROL pidc, LPTSTR pszDesc, UINT cchDesc);
    static LPIDCONTROL _IsValid(LPCITEMIDLIST pidl);
    static LPIDCONTROLW _IsUnicodeCPL(LPIDCONTROL pidc);
    
private:
    friend HRESULT CControlPanel_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);

    static HRESULT CALLBACK DFMCallBack(IShellFolder *psf, HWND hwndView,
                                             IDataObject *pdtobj, UINT uMsg,
                                             WPARAM wParam, LPARAM lParam);

    LPITEMIDLIST _pidl;
    IUnknown* _punkReg;
};  

class CControlPanelEnum : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; };
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum) { return E_NOTIMPL; };

    CControlPanelEnum(UINT uFlags);
    ~CControlPanelEnum();

    HRESULT Init();

protected:
    LONG _cRef;

    ULONG _uFlags;

    int _iModuleCur;
    int _cControlsOfCurrentModule;
    int _iControlCur;
    int _cControlsTotal;
    int _iRegControls;

    MINST _minstCur;

    ControlData _cplData;
};

class CControlPanelViewCallback : public CBaseShellFolderViewCB
{
public:
    CControlPanelViewCallback(IUnknown *punkFolder, LPCITEMIDLIST pidl)
        : CBaseShellFolderViewCB(punkFolder, pidl, SHCNE_UPDATEITEM)
        { }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HRESULT OnMERGEMENU(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_CONTROLS_POPUPMERGE, lP);
        return S_OK;
    }

    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP)
    {
        return CControls_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_INVOKECOMMAND, wP, 0);
    }

    HRESULT OnGETHELPTEXT(DWORD pv, UINT id, UINT cch, LPTSTR psz)
    {
        LoadString(HINST_THISDLL, id + IDS_MH_FSIDM_FIRST, psz, cch);;
        return S_OK;
    }

    HRESULT OnDEFITEMCOUNT(DWORD pv, UINT*lP)
    {
        // if we time out enuming items make a guess at how many items there
        // will be to make sure we get large icon mode and a reasonable window size

        *lP = 20;
        return S_OK;
    }

    HRESULT OnSize(DWORD pv, UINT cx, UINT cy)
    {
        ResizeStatus(_punkSite, cx);
        return S_OK;
    }

    HRESULT OnGetPane(DWORD pv, LPARAM dwPaneID, DWORD *pdwPane)
    {
        if (PANE_ZONE == dwPaneID)
            *pdwPane = 2;
        return S_OK;
    }

    HRESULT OnGetHelpTopic( DWORD pv, SFVM_HELPTOPIC_DATA * phtd )
    {
        StrCpyW( phtd->wszHelpFile, L"cpanel.chm" );
        return S_OK;
    }
};


STDMETHODIMP CControlPanelViewCallback::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMERGEMENU);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_DEFITEMCOUNT, OnDEFITEMCOUNT);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_GETPANE, OnGetPane);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGetHelpTopic);

    default:
        return E_FAIL;
    }

    return NOERROR;
}


IShellFolderViewCB* Controls_CreateSFVCB(IUnknown *punkFolder, LPCITEMIDLIST pidl)
{
    return new CControlPanelViewCallback(punkFolder, pidl);
}



// column IDs
enum
{
    ICOL_NAME = 0,
    ICOL_COMMENT,
};

typedef enum
{
    CPL_ICOL_NAME = 0,
    CPL_ICOL_COMMENT,
} EnumCplCols;

const COL_DATA c_cpl_cols[] = {
    { CPL_ICOL_NAME,         IDS_NAME_COL,        20,    LVCFMT_LEFT,    &SCID_NAME},
    { CPL_ICOL_COMMENT,      IDS_COMMENT_COL,     20,    LVCFMT_LEFT,    &SCID_Comment},
};

#define PRINTERS_SORT_INDEX 45

const REQREGITEM c_asControlPanelReqItems[] =
{
    { &CLSID_Printers, IDS_PRINTERS, c_szShell32Dll, -IDI_PRNFLD, PRINTERS_SORT_INDEX, SFGAO_DROPTARGET | SFGAO_FOLDER, NULL},
};

CControlPanelFolder::CControlPanelFolder(IUnknown* punkOuter) :
    CAggregatedUnknown  (punkOuter),
    _pidl               (NULL),
    _punkReg            (NULL)
{
}

CControlPanelFolder::~CControlPanelFolder()
{
    if (NULL != _pidl)
    {
        ILFree(_pidl);
    }
    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
}

#define REGSTR_POLICIES_RESTRICTCPL REGSTR_PATH_POLICIES TEXT("\\Explorer\\RestrictCpl")
#define REGSTR_POLICIES_DISALLOWCPL REGSTR_PATH_POLICIES TEXT("\\Explorer\\DisallowCpl")

HRESULT CControlPanel_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppvOut)
{
    CControlPanelFolder* pcpf = new CControlPanelFolder(punkOuter);
    if (NULL != pcpf)
    {
        static REGITEMSPOLICY ripControlPanel =
        {
            REGSTR_POLICIES_RESTRICTCPL,
            REST_RESTRICTCPL,
            REGSTR_POLICIES_DISALLOWCPL,
            REST_DISALLOWCPL
        };
        REGITEMSINFO riiControlPanel =
        {
            REGSTR_PATH_EXPLORER TEXT("\\ControlPanel\\NameSpace"),
            &ripControlPanel,
            TEXT(':'),
            SHID_CONTROLPANEL_REGITEM_EX,  // note, we don't really have a sig
            1,
            SFGAO_CANLINK,
            ARRAYSIZE(c_asControlPanelReqItems),
            c_asControlPanelReqItems,
            RIISA_ALPHABETICAL,
            NULL,
            // we want everything from after IDREGITEM.bOrder to the first 2 cBuf bytes to be filled with 0's
            (FIELD_OFFSET(IDCONTROL, cBuf) + 2) - (FIELD_OFFSET(IDREGITEM, bOrder) + 1),
            SHID_CONTROLPANEL_REGITEM,
        };

        //
        //  we dont want to return a naked 
        //  control panel folder.  this should
        //  only fail with memory probs.
        //
        HRESULT hr = CRegFolder_CreateInstance(&riiControlPanel, (IUnknown*) (IShellFolder2*) pcpf,
                                  IID_IUnknown, (PVOID*) &(pcpf->_punkReg));
        if (SUCCEEDED(hr))                                  
        {                                  
            hr = pcpf->QueryInterface(riid, ppvOut);
        }
        
        pcpf->Release();
        return hr;
    }
    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}


HRESULT CControlPanelFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CControlPanelFolder, IShellFolder2),                        // IID_IShellFolder2
        QITABENTMULTI(CControlPanelFolder, IShellFolder, IShellFolder2),     // IID_IShellFolder
        QITABENT(CControlPanelFolder, IPersistFolder2),                      // IID_IPersistFolder2
        QITABENTMULTI(CControlPanelFolder, IPersistFolder, IPersistFolder2), // IID_IPersistFolder
        QITABENTMULTI(CControlPanelFolder, IPersist, IPersistFolder2),       // IID_IPersist
        { 0 },
    };

    if (_punkReg && (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IShellFolder2)))
    {
        return _punkReg->QueryInterface(riid, ppv);
    }
    else
    {
        return QISearch(this, qit, riid, ppv);
    }
}

LPIDCONTROL CControlPanelFolder::_IsValid(LPCITEMIDLIST pidl)
{
    //
    // BUGBUG - this sucks.  the original design had no signature
    // so we are left just trying to filter out the regitems that might
    // somehow get to us.  we used to SIL_GetType(pidl) != SHID_CONTROLPANEL_REGITEM)
    // but if somehow we had an icon index that had the low byte equal
    // to SHID_CONTROLPANEL_REGITEM (0x70) we would invalidate it. DUMB!
    //
    // so we will complicate the heuristics a little bit.  lets assume that
    // all icon indeces will range between 0xFF000000 and 0x00FFFFFF
    // (or -16777214 and 16777215, 16 million each way should be plenty).
    // of course this could easily get false positives, but there really 
    // isnt anything else that we can check against.
    //
    // we will also check a minimum size.
    //
    if (pidl && pidl->mkid.cb > FIELD_OFFSET(IDCONTROL, cBuf))
    {
        LPIDCONTROL pidc = (LPIDCONTROL)pidl;
        int i = pidc->idIcon & 0xFF000000;
        if (i == 0 || i == 0xFF000000)
            return pidc;
    }
    return NULL;
}

STDMETHODIMP CControlPanelFolder::ParseDisplayName(HWND hwnd, LPBC pbc,  WCHAR* pszName, 
                                                   ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttrib)
{
    *ppidl = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    if ((*prgfInOut & SFGAO_VALIDATE) && cidl)
    {
        HRESULT hr = E_INVALIDARG;

        // BUGBUG - we dont validate more than one in the array
        LPIDCONTROL pidc = _IsValid(*apidl);
        if (pidc)
        {
            TCHAR szModule[MAX_PATH];
            GetModuleMapped((LPIDCONTROL)*apidl, szModule, ARRAYSIZE(szModule), 
                NULL, NULL, 0);
            if (PathFileExists(szModule))
                hr = S_OK;
            else
                hr = E_FAIL;
        }

        return hr;
    }

    *prgfInOut &= SFGAO_CANLINK;
    return NOERROR;
}

STDMETHODIMP CControlPanelFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                                REFIID riid, UINT *pres, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPIDCONTROL pidc = cidl && apidl ? _IsValid(apidl[0]) : NULL;

    *ppv = NULL;

    if (pidc && (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
    {
        TCHAR achParams[MAX_PATH+1+32+1+MAX_CCH_CPLNAME]; // See wsprintf below
        TCHAR szModule[MAX_PATH], szName[MAX_CCH_CPLNAME];
        UINT idIcon;

        // Map the icon ID for upgraded win95 shortcuts to CPLs
        GetModuleMapped(pidc, szModule, ARRAYSIZE(szModule), &idIcon, szName, ARRAYSIZE(szName));

        // Use the applet name in the pid if we didn't override the name in GetModuleMapped
        if (*szName == 0)
            GetDisplayName(pidc, szName, ARRAYSIZE(szName));

        wsprintf(achParams, TEXT("%s,%d,%s"), szModule, idIcon, szName);

        hr = ControlExtractIcon_CreateInstance(achParams, riid, ppv);
    }
    else if (pidc && IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create(_pidl, hwnd, cidl, apidl, 
            (IShellFolder*) this, DFMCallBack, NULL, NULL, (IContextMenu**) ppv);
    }
    else if (pidc && IsEqualIID(riid, IID_IDataObject))
    {
        hr = CIDLData_CreateFromIDArray(_pidl, cidl, apidl, (IDataObject**) ppv);
    }
    else if (pidc && IsEqualIID(riid, IID_IQueryInfo))
    {
        TCHAR szTemp[MAX_CCH_CPLINFO];
        _GetDescription(pidc, szTemp, ARRAYSIZE(szTemp));
        hr = CreateInfoTipFromText(szTemp, riid, ppv);
    }

    return hr;
}

STDMETHODIMP CControlPanelFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    *pGuid = SRCID_SFileSearch;
    return S_OK;
}

STDMETHODIMP CControlPanelFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenumUnknown)
{
    CControlPanelEnum* pesf;

    *ppenumUnknown = NULL;

    if (!(grfFlags & SHCONTF_NONFOLDERS))
        return S_FALSE;

    pesf = new CControlPanelEnum(grfFlags);
    if (NULL != pesf)
    {
        // get list of module names
        if (SUCCEEDED(pesf->Init()))
        {
            *ppenumUnknown = (IEnumIDList*) pesf;
            return NOERROR;
        }
        pesf->Release();
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP CControlPanelFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPIDCONTROL pidc1 = _IsValid(pidl1);
    LPIDCONTROL pidc2 = _IsValid(pidl2);

    if (pidc1 && pidc2)
    {
        TCHAR szName1[max(MAX_CCH_CPLNAME, MAX_CCH_CPLINFO)];
        TCHAR szName2[max(MAX_CCH_CPLNAME, MAX_CCH_CPLINFO)];
        int iCmp;

        switch (iCol)
        {
        case ICOL_COMMENT:
            _GetDescription(pidc1, szName1, ARRAYSIZE(szName1));
            _GetDescription(pidc2, szName2, ARRAYSIZE(szName2));
                // They're both ANSI, so we can compare directly
            iCmp = lstrcmp(szName1, szName2);
            if (iCmp != 0)
                return ResultFromShort(iCmp);
            // Fall through if the help field compares the same...

        case ICOL_NAME:
        default:
            GetDisplayName(pidc1, szName1, ARRAYSIZE(szName1));
            GetDisplayName(pidc2, szName2, ARRAYSIZE(szName2));
            return ResultFromShort(lstrcmp(szName1, szName2));
        }
    }
    
    return E_INVALIDARG;
}

//
// background (no items) context menu callback
//

HRESULT CALLBACK CControls_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_CONTROLS_POPUPMERGE, (LPQCMINFO)lParam);
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case FSIDM_SORTBYNAME:
            ShellFolderView_ReArrange(hwnd, ICOL_NAME);
            break;

        case FSIDM_SORTBYCOMMENT:
            ShellFolderView_ReArrange(hwnd, ICOL_COMMENT);
            break;

        default:
            // This is one of view menu items, use the default code.
            hres = S_FALSE;
            break;
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}

STDMETHODIMP CControlPanelFolder::CreateViewObject(HWND hwnd, REFIID riid, void** ppv)
{
    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;
        HRESULT hres;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = Controls_CreateSFVCB(_punkReg, _pidl);

        QueryInterface(IID_IShellFolder, (void**) &sSFV.pshf);   // in case we are agregated

        hres = SHCreateShellFolderView(&sSFV, (IShellView**) ppv);

        if (sSFV.pshf)
            sSFV.pshf->Release();

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();

        return hres;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        return CDefFolderMenu_Create(NULL, hwnd, 0, NULL, 
            (IShellFolder*) this, CControls_DFMCallBackBG, NULL, NULL, (IContextMenu**) ppv);
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CControlPanelFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET* pstrret)
{
    LPIDCONTROL pidc = _IsValid(pidl);
    TCHAR szName[max(MAX_PATH, MAX_CCH_CPLNAME)];

    if (pidc)
    {
        if ((dwFlags & (SHGDN_FORPARSING | SHGDN_INFOLDER | SHGDN_FORADDRESSBAR)) == ((SHGDN_FORPARSING | SHGDN_INFOLDER)))
        {
            GetModule(pidc, szName, ARRAYSIZE(szName));
        }
        else
        {
            GetDisplayName(pidc, szName, ARRAYSIZE(szName));
        }
        return StringToStrRet(szName, pstrret);
    }
    return E_INVALIDARG;
}

STDMETHODIMP CControlPanelFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName,
                                            DWORD dwReserved, LPITEMIDLIST* ppidlOut)
{
    return E_FAIL;
}

STDMETHODIMP CControlPanelFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::GetDefaultColumnState(UINT iColumn, DWORD* pdwState)
{
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* psdret)
{
    HRESULT hr = E_INVALIDARG;
    if (pidl == NULL)
    {
        if (iColumn < ARRAYSIZE(c_cpl_cols))
        {
            psdret->fmt = c_cpl_cols[iColumn].iFmt;
            psdret->cxChar = c_cpl_cols[iColumn].cchCol;
            hr = ResToStrRet(c_cpl_cols[iColumn].ids, &psdret->str);
        }
    }
    else
    {
        LPIDCONTROL pidc = _IsValid(pidl);
        if (pidc)
        {
            TCHAR szTemp[max(max(MAX_PATH, MAX_CCH_CPLNAME), MAX_CCH_CPLINFO)];

            psdret->str.uType = STRRET_CSTR;
            psdret->str.cStr[0] = 0;

            switch (iColumn)
            {
            case ICOL_NAME:
                GetDisplayName(pidc, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_COMMENT:
                _GetDescription(pidc, szTemp, ARRAYSIZE(szTemp));
                break;

            default:
                szTemp[0] = 0;
                break;
            }
            hr = StringToStrRet(szTemp, &psdret->str);
        }
    }
    return hr;
}

STDMETHODIMP CControlPanelFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(c_cpl_cols, ARRAYSIZE(c_cpl_cols), iColumn, pscid);
}

STDMETHODIMP CControlPanelFolder::GetClassID(CLSID* pCLSID)
{
    *pCLSID = CLSID_ControlPanel;
    return S_OK;
}

STDMETHODIMP CControlPanelFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (NULL != _pidl)
    {
        ILFree(_pidl);
        _pidl = NULL;
    }

    return SHILClone(pidl, &_pidl);
}

STDMETHODIMP CControlPanelFolder::GetCurFolder(LPITEMIDLIST* ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}

//
// list of item context menu callback
//

HRESULT CALLBACK CControlPanelFolder::DFMCallBack(IShellFolder *psf, HWND hwndView,
                                                  IDataObject *pdtobj, UINT uMsg,
                                                  WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = E_NOTIMPL;

    if (pdtobj)
    {
        STGMEDIUM medium;

        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            hres = NOERROR;

            switch(uMsg)
            {
            case DFM_MERGECONTEXTMENU:
            {
                LPQCMINFO pqcm = (LPQCMINFO)lParam;
                int idCmdFirst = pqcm->idCmdFirst;

                if (g_bRunOnNT5 && GetAsyncKeyState(VK_SHIFT) < 0)
                {
                    // If the user is holding down shift, on NT5 we load the menu with both "Open" and "Run as..."
                    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_GENERIC_CONTROLPANEL_VERBS, 0, pqcm);
                }
                else
                {
                    // Just load the "Open" menu
                    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_GENERIC_OPEN_VERBS, 0, pqcm);
                }

                SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);

                //
                //  Returning S_FALSE indicates no need to get verbs from
                // extensions.
                //

                hres = S_FALSE;

                break;
            } // case DFM_MERGECONTEXTMENU

            case DFM_GETHELPTEXT:
                LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
                break;

            case DFM_GETHELPTEXTW:
                LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
                break;

            case DFM_INVOKECOMMAND:
            {
                int i;

                for (i = pida->cidl - 1; i >= 0; i--)
                {
                    LPIDCONTROL pidc = _IsValid(IDA_GetIDListPtr(pida, i));

                    if (pidc)
                    {
                        switch(wParam)
                        {
                            case FSIDM_OPENPRN:
                            case FSIDM_RUNAS:
                            {
                                TCHAR achParam[1 + MAX_PATH + 2 + MAX_CCH_CPLNAME]; // See wnsprintf in GetExecName
                                GetExecName(pidc, achParam, ARRAYSIZE(achParam));

                                SHRunControlPanelEx(achParam, hwndView, (wParam == FSIDM_RUNAS));
                                break;
                            }

                            default:
                                hres = S_FALSE;
                        } // switch(wParam)
                    }
                    else
                        hres = E_FAIL;
                    
                }

                break;
            } // case DFM_INVOKECOMMAND

            default:
                hres = E_NOTIMPL;
                break;
            } // switch (uMsg)

            HIDA_ReleaseStgMedium(pida, &medium);

        } // if (pida)

    } // if (pdtobj)
    return hres;
}

int MakeCPLCommandLine(LPCTSTR pszModule, LPCTSTR pszName, LPTSTR pszCommandLine, DWORD cchCommandLine)
{
    RIP(pszCommandLine);
    RIP(pszModule);
    RIP(pszName);
    
    return wnsprintf(pszCommandLine, cchCommandLine, TEXT("\"%s\",%s"), pszModule, pszName);
}

void CControlPanelFolder::GetExecName(LPIDCONTROL pidc, LPTSTR pszParseName, UINT cchParseName)
{
    TCHAR szModule[MAX_PATH], szName[MAX_CCH_CPLNAME];
    
    GetModuleMapped(pidc, szModule, ARRAYSIZE(szModule), NULL, szName, ARRAYSIZE(szName));

    // If our GetModuleMapped call didn't override the applet name, get it the old fashioned way
    if (*szName == 0)
        GetDisplayName(pidc, szName, ARRAYSIZE(szName));

    MakeCPLCommandLine(szModule, szName, pszParseName, cchParseName);
}

#ifdef WINNT

typedef struct _OLDCPLMAPPING
{
    LPCTSTR szOldModule;
    UINT    idOldIcon;
    LPCTSTR szNewModule;
    UINT    idNewIcon;
    LPCTSTR szApplet;
    // Put TEXT("") in szApplet to use the applet name stored in the cpl shortcut
} OLDCPLMAPPING, *LPOLDCPLMAPPING;

const OLDCPLMAPPING g_rgOldCPLMapping[] = 
{
    // Win95 shortcuts that don't work correctly
    // -----------------------------------------

    // Add New Hardware
    {TEXT("SYSDM.CPL"), 0xfffffda6, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},       
    // ODBC 32 bit
    {TEXT("ODBCCP32.CPL"), 0xfffffa61, TEXT("ODBCCP32.CPL"), 0xfffffa61, TEXT("@0")},
    // Mail
    {TEXT("MLCFG32.CPL"), 0xffffff7f, TEXT("MLCFG32.CPL"), 0xffffff7f, TEXT("@0")},
    // Modem
    {TEXT("MODEM.CPL"), 0xfffffc18, TEXT("TELEPHON.CPL"), (UINT) -100, TEXT("")},
    // Multimedia
    {TEXT("MMSYS.CPL"), 0xffffff9d, TEXT("MMSYS.CPL"), (UINT) -110, TEXT("")},
    // Network
    {TEXT("NETCPL.CPL"), 0xffffff9c, TEXT("NCPA.CPL"), 0xfffffc17, TEXT("@0")},
    // Password
    {TEXT("PASSWORD.CPL"), 0xfffffc18, TEXT("PASSWORD.CPL"), 0xfffffc18, TEXT("@0")},
    // Regional Settings
    {TEXT("INTL.CPL"), 0xffffff9b, TEXT("INTL.CPL"), (UINT) -200, TEXT("@0")},
    // System
    {TEXT("SYSDM.CPL"), 0xfffffda8, TEXT("SYSDM.CPL"), (UINT) -6, TEXT("")},
    // Users
    {TEXT("INETCPL.CPL"), 0xfffffad5, TEXT("INETCPL.CPL"), 0xfffffad5, TEXT("@0")},

    // NT4 Shortcuts that don't work
    // -----------------------------

    // Multimedia
    {TEXT("MMSYS.CPL"), 0xfffff444, TEXT("MMSYS.CPL"), 0xfffff444, TEXT("@0")},
    // Network
    {TEXT("NCPA.CPL"), 0xfffffc17, TEXT("NCPA.CPL"), 0xfffffc17, TEXT("@0")},
    // UPS
    {TEXT("UPS.CPL"), 0xffffff9c, TEXT("POWERCFG.CPL"), (UINT) -202, TEXT("@0")},

    // Synonyms for hardware management
    // Devices
    {TEXT("SRVMGR.CPL"), 0xffffff67, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
    // Ports
    {TEXT("PORTS.CPL"), 0xfffffffe,  TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
    // SCSI Adapters
    {TEXT("DEVAPPS.CPL"), 0xffffff52, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
    // Tape Devices
    {TEXT("DEVAPPS.CPL"), 0xffffff97, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
};

#endif

HRESULT CControlPanelFolder::GetModuleMapped(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule,
                                             UINT* pidNewIcon, LPTSTR pszApplet, UINT cchApplet)
{
    HRESULT hr = S_FALSE;

#ifdef WINNT
    LPTSTR pszFilename;
    UINT cchFilenameBuffer;
#endif

    GetModule(pidc, pszModule, cchModule);

#ifdef WINNT

    // Compare just the .cpl file name, not the full path: Get this file name from the full path
    pszFilename = PathFindFileName(pszModule);

    // Calculate the size of the buffer available for the filename
    cchFilenameBuffer = cchModule - (UINT)(pszFilename - pszModule);

    if (((int) pidc->idIcon <= 0) && (pszFilename))
    {
        int i;
        for (i = 0; i < ARRAYSIZE(g_rgOldCPLMapping); i++)
        {
            // See if the module names and old icon IDs match those in this
            // entry of our mapping
            if (((UINT) pidc->idIcon == g_rgOldCPLMapping[i].idOldIcon) &&
                (lstrcmpi(pszFilename, g_rgOldCPLMapping[i].szOldModule) == 0))
            {
                hr = S_OK;
                
                // Set the return values to those of the found item
                if (pidNewIcon != NULL)
                    *pidNewIcon = g_rgOldCPLMapping[i].idNewIcon;

                lstrcpyn(pszFilename, g_rgOldCPLMapping[i].szNewModule, cchFilenameBuffer);
                
                if (pszApplet != NULL)
                    lstrcpyn(pszApplet, g_rgOldCPLMapping[i].szApplet, cchApplet);


                break;
            }
        }
    }

#endif

    // Return old values if we didn't require a translation
    if (hr == S_FALSE)
    {
        if (pidNewIcon != NULL)
            *pidNewIcon = pidc->idIcon;

        if (pszApplet != NULL)
            *pszApplet = 0; //NULL String
    }

#ifdef WINNT
    //  If the .cpl file can't be found, this may be a Win95 shortcut specifying
    //  the old system directory - possibly an upgraded system.  We try to make
    //  this work by changing the directory specified to the actual system
    //  directory.  For example c:\windows\system\foo.cpl will become
    //  c:\winnt\system32\foo.cpl.
    //
    //  Note:   The path substitution is done unconditionally because if we
    //          can't find the file it doesn't matter where we can't find it...

    if ( !PathFileExists( pszModule ) )
    {
        TCHAR szNew[MAX_PATH];
        TCHAR szSystem[MAX_PATH];

        GetSystemDirectory(szSystem, ARRAYSIZE(szSystem));
        PathCombine(szNew, szSystem, pszFilename);
    
        lstrcpyn(pszModule, szNew, cchModule);
    }
#endif

    return hr;
}

// Unicode .cpl's will be flagged by having oName = 0, oInfo = 0,
// cBuf[0] = '\0', and cBuf[1] = UNICODE_CPL_SIGNATURE_BYTE

#define UNICODE_CPL_SIGNATURE_BYTE   (BYTE)0x6a

LPIDCONTROLW CControlPanelFolder::_IsUnicodeCPL(LPIDCONTROL pidc)
{
    ASSERT(_IsValid((LPCITEMIDLIST)pidc));
    
    if ((pidc->oName == 0) && (pidc->oInfo == 0) && (pidc->cBuf[0] == '\0') && (pidc->cBuf[1] == UNICODE_CPL_SIGNATURE_BYTE))
        return (LPIDCONTROLW)pidc;
    return NULL;
}

//
//  SHualUnicodeToTChar is like SHUnicodeToTChar except that it accepts
//  an unaligned input string parameter.
//
#ifdef UNICODE
#define SHualUnicodeToTChar(src, dst, cch) ualstrcpyn(dst, src, cch)
#else   // No ANSI platforms require alignment
#define SHualUnicodeToTChar                SHUnicodeToTChar
#endif

void CControlPanelFolder::GetDisplayName(LPIDCONTROL pidc, LPTSTR pszName, UINT cchName)
{
    LPIDCONTROLW pidcW = _IsUnicodeCPL(pidc);
    if (pidcW)
        SHualUnicodeToTChar(pidcW->cBufW + pidcW->oNameW, pszName, cchName);
    else
        SHAnsiToTChar(pidc->cBuf + pidc->oName, pszName, cchName);
}

void CControlPanelFolder::GetModule(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule)
{
    LPIDCONTROLW pidcW = _IsUnicodeCPL(pidc);
    if (pidcW)
        SHualUnicodeToTChar(pidcW->cBufW, pszModule, cchModule);
    else
        SHAnsiToTChar(pidc->cBuf, pszModule, cchModule);
}

void CControlPanelFolder::_GetDescription(LPIDCONTROL pidc, LPTSTR pszDesc, UINT cchDesc)
{
    LPIDCONTROLW pidcW = _IsUnicodeCPL(pidc);
    if (pidcW)
        SHualUnicodeToTChar(pidcW->cBufW + pidcW->oInfoW, pszDesc, cchDesc);
    else
        SHAnsiToTChar(pidc->cBuf + pidc->oInfo, pszDesc, cchDesc);
}

#undef SHualUnicodeToTChar

CControlPanelEnum::CControlPanelEnum(UINT uFlags) :
    _cRef                       (1),
    _uFlags                     (uFlags),
    _iModuleCur                 (0),
    _cControlsOfCurrentModule   (0),
    _iControlCur                (0),
    _cControlsTotal             (0),
    _iRegControls               (0)
{
    ZeroMemory(&_minstCur, sizeof(_minstCur));
    ZeroMemory(&_cplData, sizeof(_cplData));
}

CControlPanelEnum::~CControlPanelEnum()
{
    CPLD_Destroy(&_cplData);
}

HRESULT CControlPanelEnum::Init()
{
    HRESULT hr;
    if (CPLD_GetModules(&_cplData))
    {
        CPLD_GetRegModules(&_cplData);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CControlPanelEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = { 
        QITABENT(CControlPanelEnum, IEnumIDList), 
        { 0 } 
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CControlPanelEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CControlPanelEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

void CPL_FillIDC(LPIDCONTROLW pidc, LPTSTR pszModule, int idIcon, LPTSTR pszName, LPTSTR pszInfo)
{
#ifdef UNICODE
    CHAR    szModuleA[MAX_PATH];
    CHAR    szNameA[MAX_CCH_CPLNAME];
    CHAR    szInfoA[MAX_CCH_CPLINFO];
    BOOL    fUnicode = FALSE;
    LPSTR   pTmpA;
#endif
    UNALIGNED TCHAR* pTmp;

    ASSERT(lstrlen(pszModule) < MAX_PATH);
    ASSERT(lstrlen(pszName) < MAX_CCH_CPLNAME);
    ASSERT(lstrlen(pszInfo) < MAX_CCH_CPLINFO);

    pidc->idIcon = idIcon;

#ifdef UNICODE
    // See if any of the three string inputs cannot be represented as ANSI

    // First check pszModule
    if (!DoesStringRoundTrip(pszModule, szModuleA, ARRAYSIZE(szModuleA)))
    {
        // Must create a full Unicode IDL.  No need to test other inputs.
        fUnicode = TRUE;
        goto FillInIDL;
    }

    // Second, check pszName
    if (!DoesStringRoundTrip(pszName, szNameA, ARRAYSIZE(szNameA)))
    {
        // Must create a full Unicode IDL.  No need to test other inputs.
        fUnicode = TRUE;
        goto FillInIDL;
    }

    // Third, check pszInfo
    if (!DoesStringRoundTrip(pszInfo, szInfoA, ARRAYSIZE(szInfoA)))
    {
        // Must create a full Unicode IDL
        fUnicode = TRUE;
    }

FillInIDL:

    if (fUnicode)
    {
        pidc->oName = 0;
        pidc->oInfo = 0;
        pidc->cBuf[0] = '\0';
        pidc->cBuf[1] = UNICODE_CPL_SIGNATURE_BYTE;
        pidc->dwFlags = 0;

        ualstrcpy(pidc->cBufW, pszModule);

        pidc->oNameW = (USHORT)(ualstrlen(pidc->cBufW) + 1);
        pTmp = pidc->cBufW + pidc->oNameW;
        ualstrcpy(pTmp, pszName);

        pidc->oInfoW = (USHORT)(pidc->oNameW + ualstrlen(pTmp) + 1);
        pTmp = pidc->cBufW + pidc->oInfoW;
        ualstrcpy(pTmp, pszInfo);

        pidc->cb = (USHORT)(FIELD_OFFSET(IDCONTROLW, cBufW) + (pidc->oInfoW
                                                            + ualstrlen(pTmp)
                                                            + 1) * SIZEOF(WCHAR) );
    }
    else
    {
        // We can make an ANSI IDCONTROL
        lstrcpyA(pidc->cBuf, szModuleA);

        pidc->oName = (USHORT)(lstrlenA(pidc->cBuf) + 1);
        pTmpA = pidc->cBuf + pidc->oName;
        lstrcpyA(pTmpA, szNameA);

        pidc->oInfo = (USHORT)(pidc->oName + lstrlenA(pTmpA) + 1);
        pTmpA = pidc->cBuf + pidc->oInfo;
        lstrcpyA(pTmpA, szInfoA);

        pidc->cb = (USHORT)(FIELD_OFFSET(IDCONTROL, cBuf) + (pidc->oInfo
                                                          + lstrlenA(pTmpA)
                                                          + 1) * SIZEOF(CHAR) );
    }
#else
    lstrcpy(pidc->cBuf, pszModule);

    pidc->oName = (USHORT) (lstrlen(pidc->cBuf) + 1);
    pTmp = pidc->cBuf + pidc->oName;
    lstrcpy(pTmp, pszName);

    pidc->oInfo = (USHORT) (pidc->oName + lstrlen(pTmp) + 1);
    pTmp = pidc->cBuf + pidc->oInfo;
    lstrcpy(pTmp, pszInfo);

    pidc->cb = (USHORT)(FIELD_OFFSET(IDCONTROL, cBuf) + (pidc->oInfo
                                                      + lstrlen(pTmp)
                                                      + 1) * SIZEOF(CHAR) );
#endif // UNICODE / !UNICODE

    *(UNALIGNED USHORT *)((LPBYTE)(pidc) + pidc->cb) = 0;   // NULL terminate
}

static BOOL DoesCplPolicyAllow(LPCTSTR pszName, LPCTSTR pszName2)
{
    BOOL b;
    b = TRUE;
    if (SHRestricted(REST_RESTRICTCPL) && 
        !IsNameListedUnderKey(pszName, REGSTR_POLICIES_RESTRICTCPL) &&
        !IsNameListedUnderKey(pszName2, REGSTR_POLICIES_RESTRICTCPL)
        )
    {
        b = FALSE;
    }
    if (b && SHRestricted(REST_DISALLOWCPL) && (
        IsNameListedUnderKey(pszName, REGSTR_POLICIES_DISALLOWCPL) ||
        IsNameListedUnderKey(pszName2, REGSTR_POLICIES_DISALLOWCPL)
        ))
    {
        b = FALSE;
    }
    return b;
}

STDMETHODIMP CControlPanelEnum::Next(ULONG celt, LPITEMIDLIST* ppidlOut, ULONG* pceltFetched)
{
    IDCONTROLW idc;
    HRESULT hres;

    if (pceltFetched)
        *pceltFetched = 0;

    if (!(_uFlags & SHCONTF_NONFOLDERS))
        return S_FALSE;

    //
    // Loop through lpData->pRegCPLs and use what cached information we can.
    //

    while (_iRegControls < _cplData.cRegCPLs)
    {
        PRegCPLInfo pRegCPL = (PRegCPLInfo) DPA_GetPtr(_cplData.hRegCPLs, _iRegControls);
        PMODULEINFO pmi;
        int i;
        TCHAR szFilePath[MAX_PATH];
        LPTSTR pszFileName;

#ifdef WINNT
        // Debugging code to catch unaligned structs [stevecat]
        //
        // Of course Win95 did NOT dword-align the registry format, so
        // we always hit this assert on Win95 machines. But it's okay
        // there since we don't have to worry about alignment. [mikesh]
        //
        ASSERT(((INT_PTR)pRegCPL & 3)==0);
#endif
        lstrcpyn(szFilePath, REGCPL_FILENAME(pRegCPL), MAX_PATH);
        pszFileName = PathFindFileName(szFilePath);

        //
        // find this module in the hamiModule list
        //

        for (i = 0 ; i < _cplData.cModules ; i++)
        {
            pmi = (PMODULEINFO) DSA_GetItemPtr(_cplData.hamiModule, i);

            if (!lstrcmpi(pszFileName, pmi->pszModuleName))
                break;
        }

        if (i < _cplData.cModules)
        {
            LPCTSTR pszDisplayName = REGCPL_CPLNAME(pRegCPL);
            // If this cpl is not supposed to be displayed let's bail
            if (!DoesCplPolicyAllow(pszDisplayName, pszFileName))
            {
                _iRegControls++;
                // we have to set this bit, so that the cpl doesn't get reregistered
                pmi->flags |= MI_REG_ENUM;
                continue;
            }
            //
            // Get the module's creation time & size
            //

            if (!(pmi->flags & MI_FIND_FILE))
            {
                WIN32_FIND_DATA findData;
                HANDLE hFindFile = FindFirstFile(pmi->pszModule, &findData);
                if (hFindFile != INVALID_HANDLE_VALUE)
                {
                    pmi->flags |= MI_FIND_FILE;
                    pmi->ftCreationTime = findData.ftCreationTime;
                    pmi->nFileSizeHigh = findData.nFileSizeHigh;
                    pmi->nFileSizeLow = findData.nFileSizeLow;

                    //
                    //DebugMsg(DM_TRACE,"sh CPLS: got timestamps for %s", REGCPL_FILENAME(pRegCPL));
                    //

                    FindClose(hFindFile);
                }
                else
                {
                    //
                    // this module no longer exists...  Blow it away.
                    //

                    DebugMsg(DM_TRACE,TEXT("sh CPLS: very stange, couldn't get timestamps for %s"), REGCPL_FILENAME(pRegCPL));
                    goto RemoveRegCPL;
                }
            }

            if (pmi->ftCreationTime.dwLowDateTime != pRegCPL->ftCreationTime.dwLowDateTime || 
                pmi->ftCreationTime.dwHighDateTime != pRegCPL->ftCreationTime.dwHighDateTime || 
                pmi->nFileSizeHigh != pRegCPL->nFileSizeHigh || 
                pmi->nFileSizeLow != pRegCPL->nFileSizeLow)
            {
                //
                // this doesn't match -- remove it from pRegCPLs; it will
                // get enumerated below.
                //

                DebugMsg(DM_TRACE,TEXT("sh CPLS: timestamps don't match for %s"), REGCPL_FILENAME(pRegCPL));
                goto RemoveRegCPL;
            }

            //
            // we have a match: mark this module so we skip it below
            // and enumerate this cpl now
            //

            pmi->flags |= MI_REG_ENUM;

            CPL_FillIDC(&idc, pmi->pszModule, EIRESID(pRegCPL->idIcon), REGCPL_CPLNAME(pRegCPL), REGCPL_CPLINFO(pRegCPL));

            _iRegControls++;
            goto return_item;
        }
        else
        {
            //
            // hmm... this cpl's module must have been nuked off the disk
            //

            DebugMsg(DM_TRACE,TEXT("sh CPLS: %s not in module list!"), REGCPL_FILENAME(pRegCPL));
        }

RemoveRegCPL:
        //
        // Nuke this cpl entry from the registry
        //

        if (!(pRegCPL->flags & REGCPL_FROMREG))
            LocalFree(pRegCPL);

        DPA_DeletePtr(_cplData.hRegCPLs, _iRegControls);

        _cplData.cRegCPLs--;
        _cplData.fRegCPLChanged = TRUE;

    }

    //
    // Have we enumerated all the cpls in this module?
    //
    LPCPLMODULE pcplm;
    LPCPLITEM pcpli;
    do
    {
        while (_iControlCur >= _cControlsOfCurrentModule || // no more
               _cControlsOfCurrentModule < 0) // error getting modules
        {
            PMODULEINFO pmi;

            //
            // Have we enumerated all the modules?
            //

            if (_iModuleCur >= _cplData.cModules)
            {
                CPLD_FlushRegModules(&_cplData); // flush changes for next guy
                return S_FALSE;
            }

            //
            // Was this module enumerated from the registry?
            //

            pmi = (PMODULEINFO) DSA_GetItemPtr(_cplData.hamiModule, _iModuleCur);
            if (pmi->flags & MI_REG_ENUM)
            {
                //
                // Yes. Don't do anything.
                //DebugMsg(DM_TRACE,"sh CPLS: already enumerated module %s", pmi->pszModule);
                //
            }
            else
            {
                //
                // No. Load and init the module, set up counters.
                //DebugMsg(DM_TRACE,"sh CPLS: loading module %s", pmi->pszModule);
                //

                pmi->flags |= MI_CPL_LOADED;
                _cControlsOfCurrentModule = CPLD_InitModule(&_cplData, _iModuleCur, &_minstCur);
                _iControlCur = 0;
            }

            //
            // Point to next module
            //

            ++_iModuleCur;
        }

        //
        // We're enumerating the next control in this module
        //

        //
        // Add the control to the registry
        //

        CPLD_AddControlToReg(&_cplData, &_minstCur, _iControlCur);

        //
        // Get the control's pidl name
        //

        pcplm = FindCPLModule(&_minstCur);
        pcpli = (LPCPLITEM) DSA_GetItemPtr(pcplm->hacpli, _iControlCur);

        ++_iControlCur;
    } while (!DoesCplPolicyAllow(pcpli->pszName, pcplm->szModule));

    CPL_FillIDC(&idc, pcplm->szModule, EIRESID(pcpli->idIcon), pcpli->pszName, pcpli->pszInfo);
    
return_item:
    hres = SHILClone((LPCITEMIDLIST)&idc, ppidlOut);

    if (SUCCEEDED(hres))
    {
        ++_cControlsTotal;

        if (pceltFetched)
            *pceltFetched = 1;
    }

    return hres;
}

STDMETHODIMP CControlPanelEnum::Reset()
{
    _iModuleCur  = 0;
    _cControlsOfCurrentModule = 0;
    _iControlCur = 0;
    _cControlsTotal = 0;
    _iRegControls = 0;

    return NOERROR;
}

