#include "shellprv.h"
#include "caggunk.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"
#include "dbgmem.h"
#include "idmk.h"
#include "fstreex.h"
#include "shobjprv.h"
#include "datautil.h"
#include "sfviewp.h"
#include "winnetp.h"    // RESOURCE_SHAREABLE
#include "prop.h"
#include "infotip.h"

extern "C"
{
#include "netview.h"

#ifdef DEBUG // For leak detection
extern void GetAndRegisterLeakDetection(void);
extern BOOL g_fInitTable;
extern LEAKDETECTFUNCS LeakDetFunctionTable;
#endif

// {22BEB58B-0794-11d2-A4AA-00C04F8EEB3E}
const GUID CLSID_CNetFldr = { 0x22beb58b, 0x794, 0x11d2, 0xa4, 0xaa, 0x0, 0xc0, 0x4f, 0x8e, 0xeb, 0x3e };

// idlist.c
void StrRetFormat(LPSTRRET pStrRet, LPCITEMIDLIST pidlRel, LPCTSTR pszTemplate, LPCTSTR pszAppend);

// in stdenum.cpp
void* CStandardEnum_CreateInstance(REFIID riid, BOOL bInterfaces, int cElement, int cbElement, void *rgElements,
                 void (WINAPI * pfnCopyElement)(void *, const void *, DWORD));

// Where WNet stores its policy
#define WNET_POLICY_KEY  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Network")

// netviewx.c
extern const IDataObjectVtbl c_CNETIDLDataVtbl;
extern const IDropTargetVtbl c_CPrintDropTargetVtbl;
extern const IDropTargetVtbl c_CNetRootTargetVtbl;

// fstreex.c
extern TCHAR const c_szDirectoryClass[];
}

// is a \\server\printer object

BOOL _IsPrintShare(LPCIDNETRESOURCE pidn)
{
    return NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SHARE && 
           NET_GetType(pidn) == RESOURCETYPE_PRINT;
}

STDAPI CNetFldr_CreateInstance(LPCITEMIDLIST pidlAbs, LPCITEMIDLIST pidlTarget, UINT uDisplayType, 
                               LPCIDNETRESOURCE pidnForProvider, LPCTSTR pszResName, 
                               REFIID riid, void **ppvOut);

STDAPI_(BOOL) CreateNetHoodShortcuts();

class CNetFolder : public CAggregatedUnknown, 
                   public IShellFolder2, 
                   public IPersistFolder3,
                   public IShellIconOverlay
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void** ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void** ppvOut);
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
    // *** IPersistFolderAlias methods ***
    STDMETHOD(InitializeEx)(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfai);
    STDMETHOD(GetFolderTargetInfo)(PERSIST_FOLDER_TARGET_INFO *ppfai);
    // *** IShellIconOverlay methods***
    STDMETHOD(GetOverlayIndex)(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHOD(GetOverlayIconIndex)(LPCITEMIDLIST pidl, int * pIconIndex);

protected:
    CNetFolder(IUnknown* punkOuter);
    ~CNetFolder();

    virtual HRESULT v_GetFileFolder(IShellFolder2 **ppsfFiles) 
                { *ppsfFiles = NULL; return E_NOTIMPL; };

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid, void** ppvObj);

    static HRESULT CALLBACK EnumCallBack(LPARAM lParam, void* pvData, UINT ecid, UINT index);
    HRESULT _OpenKeys(LPCIDNETRESOURCE pidn, HKEY ahkeys[]);
    LPCTSTR _GetProvider(LPCIDNETRESOURCE pidn, LPTSTR pszProvider, UINT cchProvider);
    DWORD _OpenEnum(HWND hwnd, DWORD grfFlags, LPNETRESOURCE pnr, HANDLE *phEnum);

    static HRESULT _CreateNetIDList(LPIDNETRESOURCE pidnIn, 
                                    LPCTSTR pszName, LPCTSTR pszProvider, LPCTSTR pszComment,
                                    LPITEMIDLIST *ppidl);

    static HRESULT _NetResToIDList(NETRESOURCE *pnr, 
                                   BOOL fAllowNull, BOOL fKeepProviderName, BOOL fKeepComment, 
                                   LPITEMIDLIST *ppidl, LPARAM lParam);

    static HRESULT _CreateEntireNetwork(LPITEMIDLIST *ppidl, LPARAM lParam);

    LPTSTR _GetNameForParsing(LPCWSTR pwszName, LPTSTR pszBuffer, INT cchBuffer, LPTSTR *ppszRegItem);
    HRESULT _ParseRest(LPBC pbc, LPCWSTR pszRest, LPITEMIDLIST* ppidl, DWORD* pdwAttributes);
    HRESULT _AddUnknownIDList(DWORD dwDisplayType, LPITEMIDLIST *ppidl);
    HRESULT _ParseSimple(LPBC pbc, LPWSTR pszName, LPITEMIDLIST* ppidl, DWORD* pdwAttributes);
    HRESULT _NetResToIDLists(NETRESOURCE *pnr, DWORD dwbuf, LPITEMIDLIST *ppidl);

    HRESULT _ParseNetName(HWND hwnd, LPBC pbc, LPCWSTR pwszName, ULONG* pchEaten, 
                              LPITEMIDLIST* ppidl, DWORD* pdwAttributes);
    LONG _GetFilePIDLType(LPCITEMIDLIST pidl);
    LPITEMIDLIST _AddProviderToPidl(LPITEMIDLIST pidl, LPCTSTR lpProvider);
    BOOL _MakeStripToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fNetObjects);

    LPFNDFMCALLBACK _GetCallbackType(LPCIDNETRESOURCE pidn)
                        { return _IsPrintShare(pidn) ? &PrinterDFMCallBack : &DFMCallBack; };

    static HRESULT CALLBACK GAOCallbackNetRoot(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut);

    LPITEMIDLIST _pidl;
    LPITEMIDLIST _pidlTarget; // pidl of where the folder is in the namespace
    LPCIDNETRESOURCE _pidnForProvider; // optional provider for this container...
    LPTSTR _pszResName;      // optional resource name of this container
    UINT _uDisplayType;      // display type of the folder
    IShellFolder2* _psfFiles;
    IUnknown* _punkInner;
    
private:
    friend HRESULT CNetFldr_CreateInstance(LPCITEMIDLIST pidlAbs, LPCITEMIDLIST pidlTarget,
                                           UINT uDisplayType,                                            
                                           LPCIDNETRESOURCE pidnForProvider, LPCTSTR pszResName, 
                                           REFIID riid, void** ppvOut);
    friend HRESULT CNetwork_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                                          IDataObject *pdtobj, UINT uMsg, 
                                          WPARAM wParam, LPARAM lParam);
    static DWORD CALLBACK _PropertiesThreadProc(void *pv);
    friend BOOL NET_GetProviderKeyName(IShellFolder* psf, LPTSTR pszName, UINT uNameLen);
    static HRESULT DFMCallBack(IShellFolder* psf, HWND hwnd,
                               IDataObject* pdtobj, UINT uMsg, 
                               WPARAM wParam, LPARAM lParam);
    static HRESULT PrinterDFMCallBack(IShellFolder* psf, HWND hwnd,
                                      IDataObject* pdtobj, UINT uMsg, 
                                      WPARAM wParam, LPARAM lParam);
    static HRESULT CALLBACK GAOCallbackNet(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut);

    BOOL _GetPathForShare(LPCIDNETRESOURCE pidn, LPTSTR pszPath);
    HRESULT _GetPathForItem(LPCIDNETRESOURCE pidn, LPTSTR pszPath);
    HRESULT _GetPathForItemW(LPCIDNETRESOURCE pidn, LPWSTR pszPath);
    HRESULT _CreateFolderForItem(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlTarget, LPCIDNETRESOURCE pidnForProvider, REFIID riid, void** ppv);
    HRESULT _GetFormatName(LPCIDNETRESOURCE pidn, STRRET* pStrRet);
    HRESULT _GetIconOverlayInfo(LPCIDNETRESOURCE pidn, int *pIndex, DWORD dwFlags);

#ifdef WINNT
    HKEY _OpenProviderTypeKey(LPCIDNETRESOURCE pidn);
#endif // WINNT
    HKEY _OpenProviderKey(LPCIDNETRESOURCE pidn);
    static void WINAPI _CopyEnumElement(void* pDest, const void* pSource, DWORD dwSize);
    HRESULT _GetNetResource(LPCIDNETRESOURCE pidn, NETRESOURCEW* pnr, int cb);
};  


class CNetRootFolder : public CNetFolder
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CNetFolder::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void)
                { return CNetFolder::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void)
                { return CNetFolder::Release(); };

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvObj)
        { return CNetFolder::BindToStorage(pidl, pbc, riid, ppvObj); };
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void** ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void** ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid)
        { return CNetFolder::GetDefaultSearchGUID(lpGuid); };
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH* ppenum)
        { return CNetFolder::EnumSearches(ppenum); };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
        { return CNetFolder::GetDefaultColumn(dwRes, pSort, pDisplay); };
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pbState)
        { return CNetFolder::GetDefaultColumnState(iColumn, pbState); };
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
        { return CNetFolder::GetDetailsEx(pidl, pscid, pv); };
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails)
        { return CNetFolder::GetDetailsOf(pidl, iColumn, pDetails); };
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
        { return CNetFolder::MapColumnToSCID(iColumn, pscid); };

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID* pClassID);
    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    // *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl)
        { return CNetFolder::GetCurFolder(ppidl); };
    // *** IPersistFolderAlias methods ***
    STDMETHOD(InitializeEx)(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfai)
        { return CNetFolder::InitializeEx(pbc, pidlRoot, ppfai); };
    STDMETHOD(GetFolderTargetInfo)(PERSIST_FOLDER_TARGET_INFO *ppfai)
        { return CNetFolder::GetFolderTargetInfo(ppfai); };


protected:
    CNetRootFolder(IUnknown* punkOuter) : CNetFolder(punkOuter) { };
    ~CNetRootFolder() { ASSERT(NULL != _spThis); _spThis = NULL; };

    BOOL v_HandleDelete(PLONG pcRef);
    HRESULT v_GetFileFolder(IShellFolder2 **ppsfFiles);

private:
    HRESULT _TryParseEntireNet(HWND hwnd, LPBC pbc, WCHAR *pwszName, LPITEMIDLIST *ppidl, DWORD *pdwAttributes);

    friend HRESULT CNetwork_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);
    static CNetRootFolder* _spThis;
};  

class CNetSFVCB : public CBaseShellFolderViewCB
{
public:
    CNetSFVCB(IShellFolder* psf, UINT uNetResType, LPCITEMIDLIST pidlMonitor, LONG lEvents) : 
    CBaseShellFolderViewCB(psf, pidlMonitor, lEvents), _uNetResType(uNetResType) {}
    ~CNetSFVCB() {}

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    UINT _uNetResType;

private:
    HRESULT OnMERGEMENU(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_NETWORK_POPUPMERGE, lP);
        return S_OK;
    }                                                                                                                 

    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP)
    {
        return CNetwork_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_INVOKECOMMAND, wP, 0);
    }

    HRESULT OnGETHELPTEXT(DWORD pv, UINT wPl, UINT wPh, LPTSTR lP)
    {
#ifdef UNICODE
        return CNetwork_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_GETHELPTEXTW, MAKEWPARAM(wPl, wPh), (LPARAM)lP);
#else
        return CNetwork_DFMCallBackBG(m_pshf, m_hwndMain, NULL, DFM_GETHELPTEXT, MAKEWPARAM(wPl, wPh), (LPARAM)lP);
#endif
    }

    HRESULT OnBACKGROUNDENUM(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream **pps)
    {
        LPCTSTR pszValName;

        switch (_uNetResType) {
        case RESOURCEDISPLAYTYPE_DOMAIN:
            pszValName = TEXT("NetDomainColsX");
            break;

        case RESOURCEDISPLAYTYPE_SERVER:
            pszValName = TEXT("NetServerColsX");
            break;

        default:
            return E_FAIL;
        }

        *pps = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, pszValName, (DWORD) wP);
        return *pps ? S_OK : E_FAIL;
    }

    HRESULT OnDEFITEMCOUNT(DWORD pv, UINT *pnItems)
    {
        // if we time out enuming items make a guess at how many items there
        // will be to make sure we get large icon mode and a reasonable window size

        switch (_uNetResType) {
        case RESOURCEDISPLAYTYPE_GENERIC:   // MyNetPlaces now has only a few items typically
        case RESOURCEDISPLAYTYPE_SERVER:    // printers & shares, usually only a few
            *pnItems = 10;   
            break;

        case RESOURCEDISPLAYTYPE_ROOT:    // usually only one or two items
            *pnItems = 5;
            break;
        }
        return S_OK;
    }

    HRESULT OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj);
    HRESULT OnGetZone(DWORD pv, DWORD * pdwZone);

    HRESULT OnDefViewMode(DWORD pv, FOLDERVIEWMODE* pvm)
    {
        //
        // force large icons for My Network Places.
        //

        if ( pvm && (_uNetResType == RESOURCEDISPLAYTYPE_GENERIC) )
            *pvm = FVM_ICON;

        return NOERROR;
    }
};

HRESULT CNetSFVCB::OnGetZone(DWORD pv, DWORD * pdwZone)
{
    if (pdwZone)
        *pdwZone = URLZONE_INTRANET; // default is "Local Intranet"

    return S_OK;    
}

HRESULT CNetSFVCB::OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj)
{
    *ppObj = NULL;

    CViewsList cViews;
    TCHAR szTemp[MAX_PATH];

    // Add base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("folder"));

    switch (_uNetResType)
    {
    case RESOURCEDISPLAYTYPE_NETWORK:
        if (NET_GetProviderKeyName(m_pshf, szTemp, ARRAYSIZE(szTemp)))
        {
            cViews.AddReg(HKEY_CLASSES_ROOT, szTemp);
        }
        break;

    case RESOURCEDISPLAYTYPE_DOMAIN:
        cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("NetDomain"));

        if (NET_GetProviderKeyName(m_pshf, szTemp, ARRAYSIZE(szTemp)))
        {
            lstrcat(szTemp, TEXT("\\Domain"));
            cViews.AddReg(HKEY_CLASSES_ROOT, szTemp);
        }
        break;

    case RESOURCEDISPLAYTYPE_SERVER:
        cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("NetServer"));

        if (NET_GetProviderKeyName(m_pshf, szTemp, ARRAYSIZE(szTemp)))
        {
            lstrcat(szTemp, TEXT("\\Server"));
            cViews.AddReg(HKEY_CLASSES_ROOT, szTemp);
        }
        break;

    case RESOURCEDISPLAYTYPE_ROOT:
        cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("Network"));
        break;

    case RESOURCEDISPLAYTYPE_GENERIC:
        // This is the "My Network Places" (net root)
        cViews.AddCLSID(&CLSID_NetworkPlaces);
        break;
    }

    cViews.GetDef(pvid);

    return CreateEnumCViewList(&cViews, ppObj);

    // Note the automatic destructor will free any views still left
}

STDMETHODIMP CNetSFVCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMERGEMENU);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, OnBACKGROUNDENUM);
    HANDLE_MSG(0, SFVM_GETCOLSAVESTREAM, OnGETCOLSAVESTREAM);
    HANDLE_MSG(0, SFVM_GETVIEWS, OnGetViews);
    HANDLE_MSG(0, SFVM_DEFITEMCOUNT, OnDEFITEMCOUNT);
    HANDLE_MSG(0, SFVM_GETZONE, OnGetZone);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDefViewMode);

    default:
        return E_FAIL;
    }

    return NOERROR;
}

STDAPI_(IShellFolderViewCB*) Net_CreateSFVCB(IShellFolder* psf, 
                                             UINT uNetResType, 
                                             LPCITEMIDLIST pidlMonitor, 
                                             LONG lEvents)
{
    return new CNetSFVCB(psf, uNetResType, pidlMonitor, lEvents);
}

// Define a collate order for the hood object types
#define _HOOD_COL_RON    0
#define _HOOD_COL_REMOTE 1
#define _HOOD_COL_FILE   2
#define _HOOD_COL_NET    3

typedef struct
{
    CNetFolder *pnf;     // CNetFolder object (this)
    HANDLE hEnum;
    DWORD grfFlags;
    LONG cItems;   // Count of items in buffer
    LONG iItem;    // Current index of the item in the buffer
    DWORD dwRemote;
    union {
        NETRESOURCE anr[0];
        BYTE szBuffer[8192];
    };
    IEnumIDList *peunk;  // used for enumerating file system items (links)
} ENUM_DATA;

// Flags for the dwRemote field
#define RMF_CONTEXT         0x00000001  // Entire network is being enumerated
#define RMF_SHOWREMOTE      0x00000002  // Return Remote Services for next enumeration
#define RMF_STOP_ENUM       0x00000004  // Stop enumeration
#define RMF_GETLINKENUM     0x00000008  // Hoodlinks enum needs to be fetched
#define RMF_SHOWLINKS       0x00000010  // Hoodlinks need to be shown
#define RMF_FAKENETROOT     0x00000020  // Don't enumerate the workgroup items

#define RMF_ENTIRENETSHOWN  0x40000000  // Entire network object shown
#define RMF_REMOTESHOWN     0x80000000  // Return Remote Services for next enumeration

const static ICONMAP c_aicmpNet[] = {
    { SHID_NET_NETWORK     , II_NETWORK      },
    { SHID_NET_DOMAIN      , II_GROUP        },
    { SHID_NET_SERVER      , II_SERVER       },
    { SHID_NET_SHARE       , (UINT)EIRESID(IDI_SERVERSHARE)  },
    { SHID_NET_DIRECTORY   , II_FOLDER       },
    { SHID_NET_PRINTER     , II_PRINTER      },
    { SHID_NET_RESTOFNET   , II_WORLD        },
    { SHID_NET_SHAREADMIN  , II_DRIVEFIXED   },
    { SHID_NET_TREE        , II_TREE         },
#ifdef WINNT
    { SHID_NET_NDSCONTAINER, (UINT)EIRESID(IDI_NDSCONTAINER) },
#endif
};

enum
{
    NKID_PROVIDERTYPE = 0,
    NKID_PROVIDER,
    NKID_NETCLASS,
    NKID_NETWORK,
    NKID_DIRECTORY,
    NKID_FOLDER
};

#define NKID_COUNT 6

enum
{
    ICOL_NAME = 0,
    ICOL_COMMENT,
};
#define ICOL_FIRST      ICOL_NAME

const COL_DATA s_net_cols[] = {
    {ICOL_NAME,     IDS_NAME_COL,       30, LVCFMT_LEFT,    &SCID_NAME},
    {ICOL_COMMENT,  IDS_COMMENT_COL,    30, LVCFMT_LEFT,    &SCID_Comment}
};

// This is one-entry cache for remote junctions resolution
TCHAR g_szLastAttemptedJunctionName[MAX_PATH] = {0};
TCHAR g_szLastResolvedJunctionName[MAX_PATH] = {0};

REGITEMSINFO g_riiNetRoot =
{
    REGSTR_PATH_EXPLORER TEXT("\\NetworkNeighborhood\\NameSpace"),
    NULL,
    TEXT(':'),
    SHID_NET_REGITEM,
    1,
    SFGAO_CANLINK,
    0,
    NULL,
    RIISA_ORIGINAL,
    NULL,
    0,
    0,
};

CNetRootFolder* CNetRootFolder::_spThis = NULL;

HRESULT CNetFldr_CreateInstance(LPCITEMIDLIST pidlAbs, LPCITEMIDLIST pidlTarget, UINT uDisplayType, 
                                LPCIDNETRESOURCE pidnForProvider, LPCTSTR pszResName, 
                                REFIID riid, void **ppvOut)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppvOut = NULL;

    if (!ILIsEmpty(pidlAbs))
    {
        CNetFolder* pNetF = new CNetFolder(NULL);
        if (NULL != pNetF)
        {
            pNetF->_uDisplayType = uDisplayType;

            if ( pidnForProvider )
            {
                //Make sure that the pidnProvider has provider information.
                ASSERT(NET_FHasProvider(pidnForProvider))

                //We are interested only in the provider informarion which is contained in the first entry.
                //Its enought if we clone only the first item in the pidl.
                pNetF->_pidnForProvider = (LPCIDNETRESOURCE)ILCloneFirst((LPCITEMIDLIST)pidnForProvider);                
            }

            if (pszResName && *pszResName)
                pNetF->_pszResName = StrDup(pszResName);
           
            pNetF->_pidl = ILClone(pidlAbs);
            pNetF->_pidlTarget = ILClone(pidlTarget);

            if ( pNetF->_pidl && (!pidlTarget || (pidlTarget && pNetF->_pidlTarget)) )
            {
                if (uDisplayType == RESOURCEDISPLAYTYPE_SERVER)
                {
                    // This is a remote computer. See if there are any remote
                    // computer registry items. If so, aggregate with the registry
                    // class.

                    REGITEMSINFO riiComputer =
                    {
                        REGSTR_PATH_EXPLORER TEXT("\\RemoteComputer\\NameSpace"),
                        NULL,
                        TEXT(':'),
                        SHID_NET_REMOTEREGITEM,
                        -1,
                        SFGAO_FOLDER | SFGAO_CANLINK,
                        0,      // no required reg items
                        NULL,
                        RIISA_ORIGINAL,
                        pszResName,
                        0,
                        0,
                    };

                    CRegFolder_CreateInstance(&riiComputer,
                                              (IUnknown*) (IShellFolder*) pNetF,
                                              IID_IUnknown,
                                              (void**) &pNetF->_punkInner);
                }
                else if ( uDisplayType == RESOURCEDISPLAYTYPE_ROOT )
                {
                    //
                    // this is the entire net icon, so lets create an instance of the regitem folder
                    // so we can merge in the items from there.
                    //

                    REGITEMSINFO riiEntireNet =
                    {
                        REGSTR_PATH_EXPLORER TEXT("\\NetworkNeighborhood\\EntireNetwork\\NameSpace"),
                        NULL,
                        TEXT(':'),
                        SHID_NET_REGITEM,
                        -1,
                        SFGAO_CANLINK,
                        0,      // no required reg items
                        NULL,
                        RIISA_ORIGINAL,
                        NULL,
                        0,
                        0,
                    };

                    CRegFolder_CreateInstance(&riiEntireNet,
                                              (IUnknown*) (IShellFolder*) pNetF,
                                              IID_IUnknown,
                                              (void**) &pNetF->_punkInner);
                }
                else
                {
                    ASSERT(hres == E_OUTOFMEMORY);
                }
                hres = pNetF->QueryInterface(riid, ppvOut);
            }
            pNetF->Release();
        }
        else
        {
            ASSERT(hres == E_OUTOFMEMORY);
        }
    }
    else
    {
        ASSERT(0);
        hres = E_INVALIDARG;
    }

    return hres;
}

HRESULT CNetwork_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    HRESULT hres = S_OK;
    ASSERT(NULL != ppv);

    // Must enter critical section to avoid racing against v_HandleDelete
    ENTERCRITICAL;

    if (NULL != CNetRootFolder::_spThis)
    {
        hres = CNetRootFolder::_spThis->QueryInterface(riid, ppv);
    }
    else
    {
        CNetRootFolder* pNetRootF = new CNetRootFolder(punkOuter);
        if (NULL != pNetRootF)
        {
            pNetRootF->_uDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
            ASSERT(NULL == pNetRootF->_punkInner);

            if (SHRestricted(REST_NOSETFOLDERS))
                g_riiNetRoot.iReqItems = 0;

            // create the regitems object, he has the NetRoot object as his outer guy.

            hres = CRegFolder_CreateInstance(&g_riiNetRoot,
                                             (IUnknown*) (IShellFolder2*) pNetRootF, 
                                             IID_IUnknown,
                                             (void**) &pNetRootF->_punkInner);

            // NOTE: not using SHInterlockedCompareExchange() because we have the critsec
            CNetRootFolder::_spThis = pNetRootF;
            hres = pNetRootF->QueryInterface(riid, ppv);

#ifdef DEBUG

            // these objects are created on the background thread, so to avoid
            // fake leak swhen that thread terminates we have to remove them from its memlist.

            // TO DO: transfer these pointers to main thread's memlist so that it shows up in the leak detection
            // because we DO leak them (only at process detach time). here's why:
            // we can't release _spThis at DLL detatch time since it calls other DLLs
            // that may be unloaded now. we just leak instead of crash

            GetAndRegisterLeakDetection();
            if (g_fInitTable)
            {
                // Remove the class pointer:
                LeakDetFunctionTable.pfnremove_from_memlist(pNetRootF);
                if ( NULL != pNetRootF->_punkInner )
                {
                    // Remove the _punkInner if it isn't the regfolder pointer
                    // (which means it is a delegate pointer - which may be allocated
                    // on our heap)
                    LeakDetFunctionTable.pfnremove_from_memlist(pNetRootF->_punkInner);
                }
            }
#endif
            // Release the self-reference, but keep the the _spThis pointer intact
            // (it will be reset to NULL in the destructor)
            pNetRootF->Release();
        }
        else
        {
            hres = E_OUTOFMEMORY;
            *ppv = NULL;
        }
    }

    LEAVECRITICAL;

    return hres;
}


CNetFolder::CNetFolder(IUnknown* punkOuter) : 
    CAggregatedUnknown (punkOuter)
{
    // Assert that we're still using a zero-init flag inside the new operator
    ASSERT(NULL == _pidl);
    ASSERT(NULL == _pidlTarget);
    ASSERT(NULL == _pidnForProvider);
    ASSERT(NULL == _pszResName);
    ASSERT(0 == _uDisplayType);
    ASSERT(NULL == _psfFiles);
    ASSERT(NULL == _punkInner);

    DllAddRef();
}


CNetFolder::~CNetFolder()
{
    ILFree(_pidl);
    ILFree(_pidlTarget);
    ILFree((LPITEMIDLIST)_pidnForProvider);
    
    if (NULL != _pszResName)
    {
        LocalFree(_pszResName);
    }

    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkInner);
    DllRelease();
}


HRESULT CNetFolder::v_InternalQueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CNetFolder, IShellFolder2),                                    // IID_IShellFolder2
        QITABENTMULTI(CNetFolder, IShellFolder, IShellFolder2),                 // IID_IShellFolder
        QITABENT(CNetFolder, IPersistFolder3),                              // IID_IPersistFolderAlias
        QITABENT(CNetFolder, IShellIconOverlay),                            // IID_IShellIconOverlay
        QITABENTMULTI(CNetFolder, IPersistFolder2, IPersistFolder3),        // IID_IPersistFolder2
        QITABENTMULTI(CNetFolder, IPersistFolder, IPersistFolder3),         // IID_IPersistFolder
        QITABENTMULTI(CNetFolder, IPersist, IPersistFolder3),               // IID_IPersist
        QITABENTMULTI2(CNetFolder, IID_IPersistFreeThreadedObject, IPersist),   // IID_IPersistFreeThreadedObject
        { 0 },
    };

    if (_punkInner && (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IShellFolder2)))
    {
        return _punkInner->QueryInterface(riid, ppv);
    }
    else if (IsEqualIID(riid, CLSID_CNetFldr))
    {
        *ppv = this;        // get class pointer (unrefed!)
        return S_OK;
    }
    else
    {
        HRESULT hr = QISearch(this, qit, riid, ppv);
        if ((E_NOINTERFACE == hr) && (NULL != _punkInner))
        {
            return _punkInner->QueryInterface(riid, ppv);
        }
        else
        {
            return hr;
        }
    }
}


BOOL CNetRootFolder::v_HandleDelete(PLONG pcRef)
{
    ASSERT(NULL != pcRef);

    ENTERCRITICAL;

    // Once inside the critical section things are slightly more stable.
    // CNetwork_CreateInstance won't be able to rescue the cached reference
    // (and bump the refcount from 0 to 1).  And we don't have to worry
    // about somebody Release()ing us down to zero a second time, since
    // no new references can show up.
    //
    // HOWEVER!  All those scary things could've happened WHILE WE WERE
    // WAITING TO ENTER THE CRITICAL SECTION.
    //
    // While we were waiting, somebody could've called CNetwork_CreateInstance,
    // which bumps the reference count back up.  So don't destroy ourselves
    // if our object got "rescued".
    //
    // What's more, while we were waiting, that somebody could've then
    // Release()d us back down to zero, causing us to be called on that
    // other thread, notice that the refcount is indeed zero, and destroy
    // the object, all on that other thread.  So if we are not the cached
    // instance, then don't destroy ourselves since that other thread did
    // it already.
    //
    // And even more, somebody might call CNetwork_CreateInstance again
    // and create a brand new object, which might COINCIDENTALLY happen
    // to have the same address as the old object we are trying to destroy
    // here.  But in that case, it's okay to destroy the new object because
    // it is indeed the case that the object's reference count is zero and
    // deserves to be destroyed.

    if (this == _spThis && 0 == *pcRef)
    {
        *pcRef = 1000; // protect against cached pointers bumping us up then down
        delete this;
    }
    LEAVECRITICAL;
    // return TRUE to indicate that we've implemented this function
    // (regardless of whether or not this object was actually deleted)
    return TRUE;
}


STDMETHODIMP CNetFolder::ParseDisplayName(HWND hwnd, LPBC pbc, WCHAR* pszName, ULONG* pchEaten,
                                          LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    return E_NOTIMPL;
}


// new for Win2K, this enables enuming the hidden admin shares
#ifndef RESOURCE_SHAREABLE
#define RESOURCE_SHAREABLE      0x00000006
#endif

//
//  in:
//      hwnd        NULL indicates no UI.
//      grfFlags     IShellFolder::EnumObjects() SHCONTF_ flags
//      pnr          in/out params
//
//
DWORD CNetFolder::_OpenEnum(HWND hwnd, DWORD grfFlags, LPNETRESOURCE pnr, HANDLE *phEnum)
{
    DWORD dwType = (grfFlags & SHCONTF_NETPRINTERSRCH) ? RESOURCETYPE_PRINT : RESOURCETYPE_ANY;
    DWORD dwScope = pnr ? RESOURCE_GLOBALNET : RESOURCE_CONTEXT;

    if ((_uDisplayType == RESOURCEDISPLAYTYPE_SERVER) &&
        (grfFlags & SHCONTF_SHAREABLE))
    {
        dwScope = RESOURCE_SHAREABLE;   // hidden admin shares for this server
    }

    DWORD err = WNetOpenEnum(dwScope, dwType, RESOURCEUSAGE_ALL, pnr, phEnum);

    if ((err != WN_SUCCESS) && hwnd)
    {
        // UI has been enabled
#ifndef WINNT
        //
        // If it failed, because the user has not been logged on,
        // give him/her a chance to logon at this moment.
        //
        if (err == WN_NOT_LOGGED_ON)
        {
            err = WNetLogon(pnr ? pnr->lpProvider : NULL, hwnd);
            if (err == WN_SUCCESS)
                err = WNetOpenEnum(dwScope, dwType, RESOURCEUSAGE_ALL, pnr, phEnum);
        }
#endif // !WINNT

        //
        //  If it failed because you are not authenticated yet,
        // we need to let the user loggin to this network resource.
        //
        // REVIEW: Ask LenS to review this code.
        if (err == WN_NOT_AUTHENTICATED || 
            err == ERROR_LOGON_FAILURE || 
            err == WN_BAD_PASSWORD || 
            err == WN_ACCESS_DENIED)
        {
            // Retry with password dialog box.
            err = WNetAddConnection3(hwnd, pnr, NULL, NULL, CONNECT_TEMPORARY | CONNECT_INTERACTIVE);
            if (err == WN_SUCCESS)
                err = WNetOpenEnum(dwScope, dwType, RESOURCEUSAGE_ALL, pnr, phEnum);
        }

        UINT idTemplate = pnr && pnr->lpRemoteName ? IDS_ENUMERR_NETTEMPLATE2 : IDS_ENUMERR_NETTEMPLATE1;   

        SHEnumErrorMessageBox(hwnd, idTemplate, err, pnr ? pnr->lpRemoteName : NULL, TRUE, MB_OK | MB_ICONHAND);
    }
    return err;
}



STDMETHODIMP CNetFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    ENUM_DATA *penet = (ENUM_DATA *)LocalAlloc(LPTR, SIZEOF(ENUM_DATA));
    if (penet)
    {
        DWORD err;
        TCHAR szProvider[MAX_PATH];

        AddRef();
        penet->pnf  = this;

        ASSERT(penet->anr[0].lpRemoteName == NULL);
        ASSERT(penet->dwRemote == 0);
        ASSERT(penet->peunk == NULL);
        ASSERT(penet->cItems == 0);
        ASSERT(penet->iItem == 0);

        penet->anr[0].lpProvider = (LPTSTR) _GetProvider(NULL, szProvider, ARRAYSIZE(szProvider));

        if (_uDisplayType != RESOURCEDISPLAYTYPE_ROOT &&
            _uDisplayType != RESOURCEDISPLAYTYPE_NETWORK)
        {
            penet->anr[0].lpRemoteName = _pszResName;
        }

        err = _OpenEnum(hwnd, grfFlags, &penet->anr[0],  &penet->hEnum);
        if (err == WN_SUCCESS)
        {
            penet->grfFlags = grfFlags;
            hr = SHCreateEnumObjects(hwnd, penet, EnumCallBack, ppenum);
        }
        else
        {
            Release();
            LocalFree(penet);
            hr = HRESULT_FROM_WIN32(err);
        }
    }
    return hr;
}


LPCIDNETRESOURCE NET_IsValidID(LPCITEMIDLIST pidl)
{
    if (pidl && !ILIsEmpty(pidl) && ((pidl->mkid.abID[0] & SHID_GROUPMASK) == SHID_NET))
        return (LPCIDNETRESOURCE)pidl;
    return NULL;
}

STDMETHODIMP CNetFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut)
{
    HRESULT hres;
    LPCIDNETRESOURCE pidn;

    *ppvOut = NULL;

    pidn = NET_IsValidID(pidl);
    if (pidn)
    {
        IShellFolder *psfJunction;
        LPITEMIDLIST pidlInit = NULL;
        LPITEMIDLIST pidlTarget = NULL;
        LPCITEMIDLIST pidlRight = _ILNext(pidl);
        BOOL fRightIsEmpty = ILIsEmpty(pidlRight);
        LPCIDNETRESOURCE pidnProvider = NET_FHasProvider(pidn) ? pidn :_pidnForProvider;

        hres = S_OK;

        // lets get the IDLISTs we are going to use to initialize the shell folder
        // if we are doing a single level bind then then ILCombine otherwise
        // be more careful.

        pidlInit = ILCombineParentAndFirst(_pidl, pidl, pidlRight);
        if ( _pidlTarget )
            pidlTarget = ILCombineParentAndFirst(_pidlTarget, pidl, pidlRight);

        if ( !pidlInit || (!pidlTarget && _pidlTarget) )
           hres = E_OUTOFMEMORY;

        // now create the folder object we are using, and either return that    
        // object to the caller, or continue the binding down.

        if ( SUCCEEDED(hres) )
        {
            hres = _CreateFolderForItem(pidlInit, pidlTarget, pidnProvider, 
                                        fRightIsEmpty ? riid : IID_IShellFolder, 
                                        fRightIsEmpty ? ppvOut : (void **)&psfJunction);

            if ( !fRightIsEmpty && SUCCEEDED(hres) )
            {
                hres = psfJunction->BindToObject(pidlRight, pbc, riid, ppvOut);
                psfJunction->Release();
            }        
        }

        ILFree(pidlInit);
        ILFree(pidlTarget);
    }
    else
    {
        hres = E_INVALIDARG;
    }

    return hres;
}


STDMETHODIMP CNetFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CNetFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = E_INVALIDARG;
    LPCIDNETRESOURCE pidn1 = NET_IsValidID(pidl1);
    LPCIDNETRESOURCE pidn2 = NET_IsValidID(pidl2);

    if ( pidn1 && pidn2 )
    {
        TCHAR szBuff1[MAX_PATH], szBuff2[MAX_PATH];

        switch (iCol & SHCIDS_COLUMNMASK)
        {
            case ICOL_COMMENT:
            {
                hres = ResultFromShort(lstrcmpi(NET_CopyComment(pidn1, szBuff1, ARRAYSIZE(szBuff1)), 
                                                NET_CopyComment(pidn2, szBuff2, ARRAYSIZE(szBuff2))));

                if ( hres != 0 )
                    return hres;

                // drop down into the name comparison
            }

            case ICOL_NAME:
            {
                // Compare by name.  This is the one case where we need to handle
                // simple ids in either place.  We will try to resync the items
                // if we find a case of this before do the compares.
                // Check for relative IDs.  In particular if one item is at
                // a server and the other is at RestOfNet then try to resync
                // the two
                //

                if ( NET_IsFake(pidn1) || NET_IsFake(pidn2) )
                {
                    // if either pidn1 or pidn2 is fake then we assume they are identical,
                    // this allows us to compare a simple net ID to a real net ID.  we
                    // assume that if this fails later then the world will be happy

                    hres = 0;
                }
                else
                {
                    // otherwise lets look at the names and provider strings accordingly

                    NET_CopyResName(pidn1, szBuff1, ARRAYSIZE(szBuff1));
                    NET_CopyResName(pidn2, szBuff2, ARRAYSIZE(szBuff2));
                    hres = ResultFromShort(lstrcmpi(szBuff1, szBuff2));

                    // If they're still identical, compare provider names.

                    if ( (hres == 0) && (iCol & SHCIDS_ALLFIELDS) )
                    {
                        LPCTSTR pszProv1 = _GetProvider(pidn1, szBuff1, ARRAYSIZE(szBuff1));
                        LPCTSTR pszProv2 = _GetProvider(pidn2, szBuff2, ARRAYSIZE(szBuff2));

                        if (pszProv1 && pszProv2)
                            hres = ResultFromShort(lstrcmp(pszProv1, pszProv2));
                        else
                        {
                            if (pszProv1 || pszProv2)
                                hres = ResultFromShort(pszProv1 ? 1 : -1);
                            else
                                hres = ResultFromShort(0);
                        }
                    }
                }

                // If they identical, compare the rest of IDs.

                if (hres == 0)
                    hres = ILCompareRelIDs((IShellFolder*)this, (LPCITEMIDLIST)pidn1, (LPCITEMIDLIST)pidn2);
            }
        }
    }

    return hres;
}

STDMETHODIMP CNetFolder::CreateViewObject(HWND hwnd, REFIID riid, void** ppvOut)
{
    HRESULT hres;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = Net_CreateSFVCB((IShellFolder*) this, _uDisplayType, _pidl,
            SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | 
            SHCNE_CREATE | SHCNE_DELETE | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM |
            SHCNE_MKDIR | SHCNE_RMDIR);

        QueryInterface(IID_IShellFolder, (void**) &sSFV.pshf);   // in case we are agregated

        hres = SHCreateShellFolderView(&sSFV, (IShellView**) ppvOut);

        if (sSFV.pshf)
            sSFV.pshf->Release();

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IShellFolder* psfOuter;
        hres = QueryInterface(IID_IShellFolder, (void**) &psfOuter);
        if (SUCCEEDED(hres))
        {
            hres = CDefFolderMenu_Create(_pidl, hwnd, 0, NULL, psfOuter, 
                CNetwork_DFMCallBackBG, NULL, NULL, (IContextMenu**) ppvOut);
            psfOuter->Release();
        }
    }
    else
    {
        hres = E_NOINTERFACE;
    }
    return hres;
}
    
STDMETHODIMP CNetFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    HRESULT hres;
    if (cidl == 0)
    {
        *prgfInOut &= (SFGAO_CANLINK | SFGAO_HASPROPSHEET | SFGAO_HASSUBFOLDER |
                       SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER);
        hres = S_OK;
    }
    else
    {
        hres = Multi_GetAttributesOf((IShellFolder2*) this, cidl, apidl, prgfInOut, GAOCallbackNet);
    }

    return hres;
}

STDMETHODIMP CNetFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET* pStrRet)
{
    HRESULT hres;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);
    if (pidn)
    {
        TCHAR szPath[MAX_PATH];
        LPCITEMIDLIST pidlNext = _ILNext(pidl);

        if (dwFlags & SHGDN_FORPARSING)
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                NET_CopyResName(pidn, szPath, ARRAYSIZE(szPath));
                if (ILIsEmpty(pidlNext))
                {
                    // we just need the last part of the display name (IN FOLDER)
                    LPTSTR pszT = StrRChr(szPath, NULL, TEXT('\\'));

                    if (!pszT)
                        pszT = szPath;
                    else
                        pszT++; // move past '\'
                    hres = StringToStrRet(pszT, pStrRet);
                }
                else
                {
                    hres = ILGetRelDisplayName((IShellFolder*) this, pStrRet, pidl, szPath, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_BACKSLASH));
                }
            }
            else
            {
                LPCITEMIDLIST pidlRight = _ILNext(pidl);

                if ( ILIsEmpty(pidlRight) )
                {
                    hres = _GetPathForItem(pidn, szPath);
                    if (SUCCEEDED(hres))
                    {
                        hres = StringToStrRet(szPath, pStrRet);
                    }
                }
                else
                {
                    IShellFolder *psfJunction;
                    //Get the pidn which has network provider information.
                    LPCIDNETRESOURCE pidnProvider = NET_FHasProvider(pidn) ? pidn :_pidnForProvider;
                    LPITEMIDLIST pidlInit, pidlTarget = NULL;

                    pidlInit = ILCombineParentAndFirst(_pidl, pidl, pidlRight);                    
                    if ( _pidlTarget )
                        pidlTarget = ILCombineParentAndFirst(_pidlTarget, pidl, pidlRight);
    
                    if ( !pidlInit || (_pidlTarget && !pidlTarget) )
                        return E_OUTOFMEMORY;

                    hres = _CreateFolderForItem(pidlInit, pidlTarget, pidnProvider, IID_IShellFolder, (void **)&psfJunction);
                    if ( SUCCEEDED(hres) )
                    {
                        hres = psfJunction->GetDisplayNameOf(pidlRight, dwFlags, pStrRet);
                        psfJunction->Release();
                    }

                    ILFree(pidlInit);
                    ILFree(pidlTarget);
                }
            }
        }
        else
        {
            hres = _GetFormatName(pidn, pStrRet);
            if (SUCCEEDED(hres) && !(dwFlags & SHGDN_INFOLDER) && (NET_GetFlags(pidn) & SHID_JUNCTION))
            {
                TCHAR szServer[MAX_PATH];
                LPITEMIDLIST pidlServer = _pidl;
                if (_pidlTarget)
                    pidlServer = _pidlTarget;

                SHGetNameAndFlags(pidlServer, SHGDN_FORPARSING, szServer, ARRAYSIZE(szServer), NULL);
                StrRetFormat(pStrRet, pidl, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), SkipServerSlashes(szServer));
            }
        }
    }
    else
        hres = E_INVALIDARG;

    return hres;
}


STDMETHODIMP CNetFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwRes, LPITEMIDLIST* ppidl)
{
    if (ppidl) 
        *ppidl = NULL;

    return E_NOTIMPL;   // not supported
}


STDMETHODIMP CNetFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                       REFIID riid, UINT* prgfInOut, void** ppvOut)
{
    HRESULT hres = E_INVALIDARG;
    LPCIDNETRESOURCE pidn = cidl ? NET_IsValidID(apidl[0]) : NULL;

    *ppvOut = NULL;

    if ((IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) && pidn)
    {
        UINT iIndex;

        if (_IsPrintShare(pidn))
            iIndex = (UINT)EIRESID(IDI_PRINTER_NET);
        else if (NET_IsRemoteFld(pidn))
            iIndex = II_RNA;
        else
            iIndex = SILGetIconIndex(apidl[0], c_aicmpNet, ARRAYSIZE(c_aicmpNet));

        hres = SHCreateDefExtIcon(NULL, iIndex, iIndex, GIL_PERCLASS, riid, ppvOut);
    }
    else if (IsEqualIID(riid, IID_IContextMenu) && pidn)
    {
        HKEY ahkeys[NKID_COUNT];

        hres = _OpenKeys(pidn, ahkeys);
        if (SUCCEEDED(hres))
        {
            IShellFolder* psfOuter;
            hres = QueryInterface(IID_IShellFolder, (void**) &psfOuter);
            if (SUCCEEDED(hres))
            {
                hres = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, 
                        psfOuter, _GetCallbackType(pidn), 
                        ARRAYSIZE(ahkeys), ahkeys, (IContextMenu**) ppvOut);
                psfOuter->Release();
            }

            SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
        }
    }
    else if (cidl && IsEqualIID(riid, IID_IDataObject))
    {
        // Point & Print printer installation assumes that the
        // netresources from CNETIDLData_GetData and the
        // pidls from CIDLData_GetData are in the same order.
        // Keep it this way.

        hres = CIDLData_CreateFromIDArray2(&c_CNETIDLDataVtbl, _pidl, cidl, apidl,
                                           (IDataObject**) ppvOut);
    }
    else if (pidn && IsEqualIID(riid, IID_IDropTarget))
    {
        // special support because this is an item (not a folder)
        if (_IsPrintShare(pidn))
        {
            hres = CIDLDropTarget_Create(hwnd, &c_CPrintDropTargetVtbl, (LPITEMIDLIST)pidn, (IDropTarget**) ppvOut);
        }
        else
        {
            IShellFolder* psf;

            hres = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hres))
            {
                hres = psf->CreateViewObject(hwnd, riid, ppvOut);
                psf->Release();
            }
        }

    }
    else if (pidn && IsEqualIID(riid, IID_IQueryInfo))
    {
        if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_ROOT)
        {
            hres = CreateInfoTipFromText(MAKEINTRESOURCE(IDS_RESTOFNETTIP), riid, ppvOut);
        }
        else
        {
            // Someday maybe have infotips for other things too
        }
    }

    return hres;
}


STDMETHODIMP CNetFolder::GetDefaultSearchGUID(LPGUID pguid)
{
    *pguid = SRCID_SFindComputer;
    return S_OK;
}


void WINAPI CNetFolder::_CopyEnumElement(void* pDest, const void* pSource, DWORD dwSize)
{
    if ( pDest && pSource )
        memcpy(pDest, pSource, dwSize);
}

STDMETHODIMP CNetFolder::EnumSearches(IEnumExtraSearch** ppenum)
{
    HRESULT hres = E_NOTIMPL;
    
    *ppenum = NULL;
    
    // if the restriction is set then this item should be enumerated from the registry
    // so we fail, else enumerate it
    // only enumerate if we actually have a network to search against
    if (!SHRestricted(REST_HASFINDCOMPUTERS) &&
        (GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS))
    {
        EXTRASEARCH *pxs = (EXTRASEARCH *)LocalAlloc(LPTR, sizeof(EXTRASEARCH));
        if (pxs)
        {
            pxs->guidSearch = SRCID_SFindComputer;
            if (LoadStringW(g_hinst, IDS_FC_NAME,     pxs->wszFriendlyName, SIZEOF(pxs->wszFriendlyName)))
            {      
                *ppenum = (IEnumExtraSearch*)CStandardEnum_CreateInstance(IID_IEnumExtraSearch, FALSE,
                            1, sizeof(EXTRASEARCH), pxs, _CopyEnumElement);
                if (*ppenum == NULL)
                {
                    LocalFree(pxs);
                    hres = E_OUTOFMEMORY;
                }
                else
                    hres = S_OK;
            }
        }
        else
            hres = E_OUTOFMEMORY;
    }

    return hres;
}


STDMETHODIMP CNetFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}


STDMETHODIMP CNetFolder::GetDefaultColumnState(UINT iColumn, DWORD* pdwState)
{
    HRESULT hr = S_OK;

    *pdwState = 0;
    
    if (iColumn < ARRAYSIZE(s_net_cols))
    {
        *pdwState |= SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    return hr;
}


STDMETHODIMP CNetFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    HRESULT hres = E_NOTIMPL;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);
    if (pidn)
    {
        if (IsEqualSCID(*pscid, SCID_NETRESOURCE))
        {
            // Office calls SHGetDataFromIDList() with a large buffer to hold all
            // of the strings in the NETRESOURCE structure, so we need to make sure
            // that our variant can hold enough data to pass back to it:
            BYTE rgBuffer[sizeof(NETRESOURCEW) + (4 * MAX_PATH * sizeof(WCHAR))];
            hres = _GetNetResource(pidn, (NETRESOURCEW*) rgBuffer, sizeof(rgBuffer));
            if (SUCCEEDED(hres))
            {
                hres = InitVariantFromBuffer(pv, rgBuffer, sizeof(rgBuffer));
                if (SUCCEEDED(hres))
                {
                    // Fixup pointers in structure to point within the variant
                    // instead of our stack variable (rgBuffer):
                    ASSERT(pv->vt == (VT_ARRAY | VT_UI1));
                    NETRESOURCEW* pnrw = (NETRESOURCEW*) pv->parray->pvData;
                    if (pnrw->lpLocalName)
                    {
                        pnrw->lpLocalName = (LPWSTR) ((BYTE*) pnrw +
                                                      ((BYTE*) pnrw->lpLocalName - rgBuffer));
                    }
                    if (pnrw->lpRemoteName)
                    {
                        pnrw->lpRemoteName = (LPWSTR) ((BYTE*) pnrw +
                                                       ((BYTE*) pnrw->lpRemoteName - rgBuffer));
                    }
                    if (pnrw->lpComment)
                    {
                        pnrw->lpComment = (LPWSTR) ((BYTE*) pnrw +
                                                    ((BYTE*) pnrw->lpComment - rgBuffer));
                    }
                    if (pnrw->lpProvider)
                    {
                        pnrw->lpProvider = (LPWSTR) ((BYTE*) pnrw +
                                                     ((BYTE*) pnrw->lpProvider - rgBuffer));
                    }
                }
            }
        }
        else if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did;

            switch(SIL_GetType(pidl) & SHID_TYPEMASK)
            {
                case SHID_NET_DOMAIN:
                    did.dwDescriptionId = SHDID_NET_DOMAIN;   
                    break;

                case SHID_NET_SERVER:
                    did.dwDescriptionId = SHDID_NET_SERVER;
                    break;

                case SHID_NET_SHARE:
                    did.dwDescriptionId = SHDID_NET_SHARE;    
                    break;

                case SHID_NET_RESTOFNET:
                    did.dwDescriptionId = SHDID_NET_RESTOFNET;
                    break;

                default:
                    did.dwDescriptionId = SHDID_NET_OTHER;      
                    break;
            }

            did.clsid = CLSID_NULL;
            hres = InitVariantFromBuffer(pv, &did, sizeof(did));
        }
        else if (IsEqualSCID(*pscid, SCID_Comment))
        {
            TCHAR szTemp[MAX_PATH];
            NET_CopyComment(pidn, szTemp, ARRAYSIZE(szTemp));
            hres = InitVariantFromStr(pv, szTemp);
        }
        else if (IsEqualSCID(*pscid, SCID_NAME))
        {
            TCHAR szTemp[MAX_PATH];
            
            NET_CopyResName(pidn, szTemp, ARRAYSIZE(szTemp));
            hres = InitVariantFromStr(pv, szTemp);
        }
    }
    else
    {
        IShellFolder2* psfFiles;
        hres = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hres))
            hres = psfFiles->GetDetailsEx(pidl, pscid, pv);            
    }

    return hres;
}


STDMETHODIMP CNetFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    TCHAR szTemp[MAX_PATH];
    LPCIDNETRESOURCE pidn;

    if (iColumn >= ARRAYSIZE(s_net_cols))
        return E_NOTIMPL;

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;

    if (!pidl)
    {
        pDetails->fmt = s_net_cols[iColumn].iFmt;
        pDetails->cxChar = s_net_cols[iColumn].cchCol;
        return ResToStrRet(s_net_cols[iColumn].ids, &pDetails->str);
    }

    pidn = NET_IsValidID(pidl);

    switch (iColumn)
    {
        case ICOL_NAME:
            if (pidn)
            {
                NET_CopyResName(pidn, szTemp, ARRAYSIZE(szTemp));
                return StringToStrRet(szTemp, &pDetails->str);
            }
            else
            {
                IShellFolder2* psfFiles;
                if (SUCCEEDED(v_GetFileFolder(&psfFiles)))
                    return psfFiles->GetDisplayNameOf(pidl, SHGDN_NORMAL, &pDetails->str);
            }
            break;

        case ICOL_COMMENT:
            if (pidn)
            {
                NET_CopyComment(pidn, szTemp, ARRAYSIZE(szTemp));
                return StringToStrRet(szTemp, &pDetails->str);
            }
            else 
            {
                IShellFolder2* psfFiles;
                if (SUCCEEDED(v_GetFileFolder(&psfFiles)))
                {
                    // call the nethood folder and get the comment field from that, we cannot
                    // use the column index we have, instead we must use GetDetailsEx and
                    // pass the unique columne ID.
                    return MapSCIDToDetailsOf(psfFiles, pidl, &SCID_Comment, pDetails);
                }
            }
            break;
    }
    return S_OK;
}


STDMETHODIMP CNetFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(s_net_cols, ARRAYSIZE(s_net_cols), iColumn, pscid);
}


// IPersist methods

STDMETHODIMP CNetFolder::GetClassID(CLSID* pCLSID)
{
    switch (_uDisplayType) 
    {
        case RESOURCEDISPLAYTYPE_ROOT:
            *pCLSID = CLSID_NetworkRoot;
            break;

        case RESOURCEDISPLAYTYPE_SERVER:
            *pCLSID = CLSID_NetworkServer;
            break;

        case RESOURCEDISPLAYTYPE_DOMAIN:
            *pCLSID = CLSID_NetworkDomain;
            break;

        case RESOURCEDISPLAYTYPE_SHARE:
            *pCLSID = CLSID_NetworkShare;
            break;

        default:
            *pCLSID = CLSID_NULL;
            break;
    }
    
    return S_OK;
}


// IPersistFolder method

STDMETHODIMP CNetFolder::Initialize(LPCITEMIDLIST pidl)
{
    ILFree(_pidl);
    ILFree(_pidlTarget);
    _pidl = _pidlTarget = NULL;

    return SHILClone(pidl, &_pidl);
}


// IPersistFolder2 method

STDMETHODIMP CNetFolder::GetCurFolder(LPITEMIDLIST* ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}


// IPersistFolder3 methods

STDMETHODIMP CNetFolder::InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *pfti)
{
    ILFree(_pidl);
    ILFree(_pidlTarget);
    _pidl = _pidlTarget = NULL;

    HRESULT hres = SHILClone(pidlRoot, &_pidl);
    if ( SUCCEEDED(hres) && pfti && pfti->pidlTargetFolder )
    {
        hres = SHILClone(pfti->pidlTargetFolder, &_pidlTarget);
    }

    return hres;
}

STDMETHODIMP CNetFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hres = S_OK;

    ZeroMemory(pfti, SIZEOF(*pfti));

    if ( _pidlTarget )
        hres = SHILClone(_pidlTarget, &pfti->pidlTargetFolder);
    
    pfti->dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?
    pfti->csidl = -1;

    return hres;
}


// IShellIconOverlay

HRESULT CNetFolder::_GetIconOverlayInfo(LPCIDNETRESOURCE pidn, int *pIndex, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;

    //
    // For netshare objects we want to get the icon overlay.
    // If the share is "pinned" to be available offline it will
    // have the "Offline Files" overlay.
    //
    if (RESOURCEDISPLAYTYPE_SHARE == NET_GetDisplayType(pidn))
    {
        TCHAR szPath[MAX_PATH];
        hres = _GetPathForItem(pidn, szPath);
        if (SUCCEEDED(hres))
        {
            IShellIconOverlayManager *psiom;
            hres = GetIconOverlayManager(&psiom);
            if (SUCCEEDED(hres))
            {
                WCHAR szPathW[MAX_PATH];
                SHTCharToUnicode(szPath, szPathW, ARRAYSIZE(szPathW));
                hres = psiom->GetFileOverlayInfo(szPathW, 0, pIndex, dwFlags);
                psiom->Release();
            }
        }
    }

    return hres;
}


STDMETHODIMP CNetFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hres = E_FAIL;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);

    if (NULL != pidn)
    {
        hres = _GetIconOverlayInfo(pidn, pIndex, SIOM_OVERLAYINDEX);
    }
    return hres;
}


STDMETHODIMP CNetFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hres = E_FAIL;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);

    if (NULL != pidn)
    {
        hres = _GetIconOverlayInfo(pidn, pIndex, SIOM_ICONINDEX);
    }
    return hres;
}



//
// Helper function to allow external callers to query information from a
// network pidl...
//
// NOTE NOTE - This function returns a NETRESOURCE structure whose string
// pointers are not valid.  On Win95 they were pointers back into the pidl's
// strings (even though the strings were copied into the supplied pv buffer.)
// Now we make the pointers really point into the buffer.
//
HRESULT CNetFolder::_GetNetResource(LPCIDNETRESOURCE pidn, NETRESOURCEW* pnr, int cb)
{
    TCHAR szStrings[3][MAX_PATH];
    LPWSTR psz, lpsz[3] = {NULL, NULL, NULL};
    int i, cchT;

    if (cb < SIZEOF(*pnr))
        return DISP_E_BUFFERTOOSMALL;

    ZeroMemory(pnr, cb);

    NET_CopyResName(pidn, szStrings[0], ARRAYSIZE(szStrings[0]));
    NET_CopyComment(pidn, szStrings[1], ARRAYSIZE(szStrings[1]));
    _GetProvider(pidn, szStrings[2], ARRAYSIZE(szStrings[2]));

    // Fill in some of the stuff first.
    // pnr->dwScope = 0;
    pnr->dwType = NET_GetType(pidn);
    pnr->dwDisplayType = NET_GetDisplayType(pidn);
    pnr->dwUsage = NET_GetUsage(pidn);
    // pnr->lpLocalName = NULL;

    // Now lets copy the strings into the buffer and make the pointers
    // relative to the buffer...
    psz = (LPWSTR)(pnr + 1);
    cb -= SIZEOF(*pnr);

    for (i = 0; i < ARRAYSIZE(szStrings); i++)
    {
        if (*szStrings[i])
        {
            cchT = (lstrlen(szStrings[i]) + 1) * SIZEOF(TCHAR);
            if (cchT <= cb)
            {
                SHTCharToUnicode(szStrings[i], psz, cb);
                lpsz[i] = psz;
                psz += cchT;
                cb -= cchT * SIZEOF(TCHAR);
            }
            else
            {
                // A hint that the structure is ok,
                // but the strings are missing
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }

    pnr->lpRemoteName = lpsz[0];
    pnr->lpComment    = lpsz[1];
    pnr->lpProvider   = lpsz[2];

    return S_OK;
}


// Replace all the space characters in the provider name with '_'.
void ReplaceSpacesWithUnderscore(LPTSTR psz)
{
    while (psz = StrChr(psz, TEXT(' ')))
    {
        *psz = TEXT('_');
        psz++;              // DBCS safe
    }
}

//
// This function opens a reg. database key based on the "network provider".
//
// Returns:        hkey
//
// The caller is responsibe to close the key by calling RegCloseKey().
//
HKEY CNetFolder::_OpenProviderKey(LPCIDNETRESOURCE pidn)
{
    TCHAR szProvider[MAX_PATH];

    if (_GetProvider(pidn, szProvider, ARRAYSIZE(szProvider)))
    {
        HKEY hkeyProgID = NULL;
        ReplaceSpacesWithUnderscore(szProvider);
        RegOpenKey(HKEY_CLASSES_ROOT, szProvider, &hkeyProgID);
        return hkeyProgID;
    }
    return NULL;
}

// for netview.cpp

STDAPI_(BOOL) NET_GetProviderKeyName(IShellFolder* psf, LPTSTR pszName, UINT uNameLen)
{
    CNetFolder* pThis;

    *pszName = 0;

    if (SUCCEEDED(psf->QueryInterface(CLSID_CNetFldr, (void**) &pThis)) &&
        pThis->_GetProvider(NULL, pszName, uNameLen))
    {
        ReplaceSpacesWithUnderscore(pszName);
    }
    return (BOOL)*pszName;
}

//
// This function opens a reg. database key based on the network provider type.
// The type is a number that is not localized, as opposed to the provider name
// which may be localized.
//
// Arguments:
//  pidlAbs -- Absolute IDList to a network resource object.
//
// Returns:        hkey
//
// Notes:
//  The caller is responsible to close the key by calling RegCloseKey().
//

#ifdef WINNT

HKEY CNetFolder::_OpenProviderTypeKey(LPCIDNETRESOURCE pidn)
{
    HKEY hkeyProgID = NULL;
    TCHAR szProvider[MAX_PATH];

    if (_GetProvider(pidn, szProvider, ARRAYSIZE(szProvider)))
    {
        // Now that we've got the provider name, get the provider id.
        DWORD dwType;
        if (WNetGetProviderType(szProvider, &dwType) == WN_SUCCESS)
        {
            // convert nis.wNetType to a string, and then open the key
            // HKEY_CLASSES_ROOT\Network\Type\<type string>

            TCHAR szRegValue[MAX_PATH];
            wsprintf(szRegValue, TEXT("Network\\Type\\%d"), HIWORD(dwType));
            RegOpenKey(HKEY_CLASSES_ROOT, szRegValue, &hkeyProgID);
        }
    }

    return hkeyProgID;
}

#endif // WINNT

HRESULT CNetFolder::_OpenKeys(LPCIDNETRESOURCE pidn, HKEY ahkeys[NKID_COUNT])
{
    // See if there is a key specific to the type of Network object...
    ahkeys[0] = ahkeys[1] = ahkeys[2] = ahkeys[3] = ahkeys[4] = ahkeys[5] = NULL;
#ifdef WINNT
    ahkeys[NKID_PROVIDERTYPE] = _OpenProviderTypeKey(pidn);
#endif
    ahkeys[NKID_PROVIDER] = _OpenProviderKey(pidn);

    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SHARE)
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("NetShare"), &ahkeys[NKID_NETCLASS]);
    else if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SERVER)
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("NetServer"), &ahkeys[NKID_NETCLASS]);

    RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Network"), &ahkeys[NKID_NETWORK]);

    // make sure it is not a printer before adding "Folder" or "directory"

    if (!_IsPrintShare(pidn))
    {
        // Shares should also support directory stuff...
        if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SHARE)
            RegOpenKey(HKEY_CLASSES_ROOT, c_szDirectoryClass, &ahkeys[NKID_DIRECTORY]);

        RegOpenKey(HKEY_CLASSES_ROOT, c_szFolderClass, &ahkeys[NKID_FOLDER]);
    }

    return S_OK;
}

#ifdef WINNT
#define WNFMT_PLATFORM  WNFMT_ABBREVIATED | WNFMT_INENUM
#else
#define WNFMT_PLATFORM  WNFMT_INENUM    // Win95 net providers improperly implement spec, and shell does too
#endif // WINNT

//
//  This function retrieves the formatted (display) name of the specified network object.
//
HRESULT CNetFolder::_GetFormatName(LPCIDNETRESOURCE pidn, STRRET* pStrRet)
{
    TCHAR szName[MAX_PATH];

    NET_CopyResName(pidn, szName, ARRAYSIZE(szName));

    if ((NET_GetDisplayType(pidn) != RESOURCEDISPLAYTYPE_ROOT) && 
        (NET_GetDisplayType(pidn) != RESOURCEDISPLAYTYPE_NETWORK))
    {
        TCHAR szDisplayName[MAX_PATH], szProvider[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(szDisplayName);

        LPCTSTR pszProvider = _GetProvider(pidn, szProvider, ARRAYSIZE(szProvider));
        if ( pszProvider )
        {   
            DWORD dwRes = WNetFormatNetworkName(pszProvider, szName, szDisplayName, &dwSize, WNFMT_PLATFORM, 8 + 1 + 3);
            if ( dwRes == WN_SUCCESS )            
                lstrcpy(szName, szDisplayName);
        }
    }

    return StringToStrRet(szName, pStrRet);
}

//
// resolve non-UNC share names (novell) to UNC style names
//
// returns:
//      TRUE    translated the name
//      FALSE   didn't translate (maybe error case)
//

BOOL CNetFolder::_GetPathForShare(LPCIDNETRESOURCE pidn, LPTSTR pszPath)
{
    NETRESOURCE nr;
    DWORD err, dwRedir, dwResult;
    TCHAR szAccessName[MAX_PATH], szRemoteName[MAX_PATH], szProviderName[MAX_PATH];
    UINT cbAccessName;

    NET_CopyResName(pidn, szRemoteName, ARRAYSIZE(szRemoteName));
    if (NULL != _pszResName)
    {
        //
        // Combine the folder name with the share name
        // to create a UNC path.
        //
        // Borrow the szProviderName[] buffer for a bit.
        //
        PathCombine(szProviderName, _pszResName, szRemoteName);

        //
        // To be safe: UNC prefix implies that name is available using FS access
        // Theoretically it also should be routed to MPR, but it is late to do this
        //
        if (PathIsUNC(szProviderName))
        {
            lstrcpy(pszPath, szProviderName);
            return FALSE;
        }
        szProviderName[0] = TEXT('\0');

    }

    // Check cache
    ENTERCRITICAL;
    if (lstrcmpi(g_szLastAttemptedJunctionName, szRemoteName) == 0)
    {
        // cache hit
        lstrcpy(pszPath, g_szLastResolvedJunctionName);

        LEAVECRITICAL;
        return TRUE;
    }
    LEAVECRITICAL;

    memset(&nr, 0, SIZEOF(NETRESOURCE));

    nr.lpRemoteName = szRemoteName;
    nr.lpProvider = (LPTSTR) _GetProvider(pidn, szProviderName, ARRAYSIZE(szProviderName));
    nr.dwType = NET_GetType(pidn);
    nr.dwUsage = NET_GetUsage(pidn);
    nr.dwDisplayType = NET_GetDisplayType(pidn);

    dwRedir = CONNECT_TEMPORARY;

    // Prepare access name buffer and net resource request buffer
    cbAccessName = SIZEOF(szAccessName);        // BUGBUG verify this is cb, not cch
    szAccessName[0] = 0;

    err = WNetUseConnection(NULL, &nr, NULL, NULL, dwRedir, szAccessName, (LPDWORD) &cbAccessName, &dwResult);

    if ((WN_SUCCESS != err) || !szAccessName[0])
    {
        lstrcpy(pszPath, szRemoteName);
        return FALSE;
    }

    // Get the return name
    lstrcpy(pszPath, szAccessName);

    // Update cache entry
    // BUGBUG We also want to record insuccessful resolution, although
    // it is not really important, as we come to resolving code often
    // only if it succeeded at least once.

    {
        ENTERCRITICAL;

        lstrcpy(g_szLastAttemptedJunctionName, szRemoteName);
        lstrcpy(g_szLastResolvedJunctionName, szAccessName);

        LEAVECRITICAL;
    }

    return TRUE;
}

// in:
//      pidn    may be multi-level net resource pidl like
//              [entire net] [provider] [server] [share] [... file sys]
//           or [server] [share] [... file sys]

HRESULT CNetFolder::_GetPathForItem(LPCIDNETRESOURCE pidn, LPTSTR pszPath)
{
    *pszPath = 0;

    // loop down
    for (; !ILIsEmpty((LPCITEMIDLIST)pidn) ; pidn = (LPCIDNETRESOURCE)_ILNext((LPCITEMIDLIST)pidn))
    {
        if (NET_GetFlags(pidn) & SHID_JUNCTION)     // \\server\share or strike/sys
        {
            _GetPathForShare(pidn, pszPath);
            break;  // below this we don't know about any of the PIDLs
        }
        else
        {
            NET_CopyResName(pidn, pszPath, MAX_PATH);
        }
    }
    return *pszPath ? S_OK : E_NOTIMPL;
}

HRESULT CNetFolder::_GetPathForItemW(LPCIDNETRESOURCE pidn, LPWSTR pszPath)
{
#ifdef UNICODE
    return _GetPathForItem(pidn, pszPath);
#else // UNICODE
    TCHAR szPath[MAX_PATH];
    HRESULT hres = _GetPathForItem(pidn, szPath);
    if (SUCCEEDED(hres))
        SHTCharToUnicode(szPath, pszPath, MAX_PATH);
    else
        *pszPath = 0;
    return hres;
#endif // UNICODE
}


// in:
//  pidl
//
// takes the last items and create a folder for it, assuming the first section is the 
// used to initialze.  the riid and ppv are used to return an object.
//

HRESULT CNetFolder::_CreateFolderForItem(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlTarget, LPCIDNETRESOURCE pidnForProvider, REFIID riid, void** ppv)
{
    LPCITEMIDLIST pidlLast = ILFindLastID(pidl);
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidlLast);

    if ( !pidn )
        return E_INVALIDARG;

    if (NET_IsRemoteFld(pidn))
    {
        // note: I think this is dead functionality. it was used in NT4 but we can't find
        // the impl of this CLSID_Remote anymore...
        IPersistFolder * ppf;
        HRESULT hres = SHCoCreateInstance(NULL, &CLSID_Remote, NULL, IID_IPersistFolder, (void **)&ppf);
        if (SUCCEEDED(hres))
        {
            hres = ppf->Initialize(pidl);
            if (SUCCEEDED(hres))
                hres = ppf->QueryInterface(riid, ppv);
            ppf->Release();
        }
        return hres;
    }
    else if (NET_GetFlags(pidn) & SHID_JUNCTION)     // \\server\share or strike/sys
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};

        pfti.pidlTargetFolder = (LPITEMIDLIST)pidlTarget;    
        _GetPathForItemW(pidn, pfti.szTargetParsingName);
        pfti.csidl = -1;
        pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?

        return CFSFolder_CreateFolder(NULL, pidl, &pfti, riid, ppv);
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        NET_CopyResName(pidn, szPath, ARRAYSIZE(szPath));

        return CNetFldr_CreateInstance(pidl, pidlTarget, NET_GetDisplayType(pidn), pidnForProvider, szPath, riid, ppv);
    }
}


// find the share part of a UNC
//  \\server\share
//  return pointer to "share" or pointer to empty string if none

LPCTSTR PathFindShareName(LPCTSTR pszUNC)
{
    LPCTSTR psz = SkipServerSlashes(pszUNC);
    if (*psz)
    {
        psz = StrChr(psz + 1, TEXT('\\'));
        if (psz)
            psz++;
        else
            psz = TEXT("");
    }
    return psz;
}


//
// To be called back from within SHCreateEnumObjects
//
// lParam       - LPDEFENUM
// pvData       - pointer to ENUMNETWORK constructed by CNetRoot_EnumObjects
// ecid         - enumeration command (event)
// index        - LPDEFENUM->iCur (unused)

HRESULT CALLBACK CNetFolder::EnumCallBack(LPARAM lParam, void *pvData, UINT ecid, UINT index)
{
    HRESULT hres = S_OK;
    ENUM_DATA *penet = (ENUM_DATA *)pvData;

    if (ecid == ECID_SETNEXTID)
    {
        // Time to stop enumeration?
        if (penet->dwRemote & RMF_STOP_ENUM)
            return S_FALSE;       // Yes

        //
        // should we try and get the links enumerator?
        //

        if ( penet->dwRemote & RMF_GETLINKENUM )
        {
            CreateNetHoodShortcuts();
		
            IShellFolder2* psfNetHood;                                                                                             
            if (SUCCEEDED(penet->pnf->v_GetFileFolder(&psfNetHood)))
                psfNetHood->EnumObjects(NULL, penet->grfFlags, &penet->peunk);

            if (penet->peunk)
                penet->dwRemote |= RMF_SHOWLINKS;

            penet->dwRemote &= ~RMF_GETLINKENUM;
        }

        //
        // should we be showing the links?
        //

        if (penet->dwRemote & RMF_SHOWLINKS)
        {
            if (penet->peunk)
            {
                ULONG celtFetched;
                LPITEMIDLIST pidl;

                hres = penet->peunk->Next(1, &pidl, &celtFetched);
                if (hres == S_OK && celtFetched == 1)
                {
                    ASSERT(pidl);
                    CDefEnum_SetReturn(lParam, pidl);
                    return S_OK;       // Added link
                }
            }

            penet->dwRemote &= ~RMF_SHOWLINKS; // Done enumerating links
        }

        hres = S_OK;

        // Do we add the remote folder?
        // (Note: as a hack to ensure that the remote folder is added
        // to the 'hood despite what MPR says, RMF_SHOWREMOTE can be
        // set without RMF_CONTEXT set.)
        if ((penet->dwRemote & RMF_SHOWREMOTE) && !(penet->dwRemote & RMF_REMOTESHOWN))
        {
            // Yes
            // Only try to put the remote entry in once.
            penet->dwRemote |= RMF_REMOTESHOWN;

            // Is this not the Context container?
            // (See note above as to why we are asking this question.)
            if ( !(penet->dwRemote & RMF_CONTEXT) ) 
            {
                // Yes; stop after the next time
                penet->dwRemote |= RMF_STOP_ENUM;
            }

            // We have fallen thru because the remote services is not
            // installed.

            // Is this not the Context container AND the remote folder
            // is not installed?
            if ( !(penet->dwRemote & RMF_CONTEXT) ) 
            {
                // Yes; nothing else to enumerate
                return S_FALSE;
            }
        }

        if ( penet->dwRemote & RMF_FAKENETROOT )
        {
            if ( !(penet->dwRemote & RMF_ENTIRENETSHOWN) )
            {                           
                _CreateEntireNetwork(NULL, lParam);         // fake entire net
                penet->dwRemote |= RMF_ENTIRENETSHOWN;
            }
            else
            {
                return S_FALSE;         // no more to enumerate
            }
        }
        else
        {
            while (TRUE)
            {
                ULONG err = WN_SUCCESS;
                LPNETRESOURCE pnr;
                if (penet->iItem >= penet->cItems)
                {
                    DWORD dwSize = SIZEOF(penet->szBuffer);

                    // Figure that on average no item over 128 bytes...
                    //penet->cItems = sizeof(penet->szBuffer) >> 7;
                    penet->cItems = -1;
                    penet->iItem = 0;

                    err = WNetEnumResource(penet->hEnum, (DWORD*)&penet->cItems, penet->szBuffer, &dwSize);
                    DebugMsg(DM_TRACE, TEXT("Net EnumCallback: err=%d Count=%d"),
                        err, penet->cItems);
                }

                pnr = &penet->anr[penet->iItem++];
                // Output some debug messages to help us track
#ifdef NET_TRACE
                DebugMsg(DM_TRACE, TEXT("Net EnumCallback: err=%d s=%d, t=%d, dt=%d, u=%d, %s"),
                    err, pnr->dwScope, pnr->dwType, pnr->dwDisplayType,
                    pnr->dwUsage, pnr->lpRemoteName ? pnr->lpRemoteName : TEXT("[NULL]"));
#endif //NET_TRACE
                // Note: the <= below is correct as we already incremented the index...
                if (err == WN_SUCCESS && (penet->iItem <= penet->cItems))
                {
                    // decide if the thing is a folder or not
                    ULONG grfFlagsItem = ((pnr->dwUsage & RESOURCEUSAGE_CONTAINER) || 
                                          (pnr->dwType == RESOURCETYPE_DISK) ||
                                          (pnr->dwType == RESOURCETYPE_ANY)) ?
                                            SHCONTF_FOLDERS : SHCONTF_NONFOLDERS;

                    // If this is the context enumeration, we want to insert the
                    // Remote Services after the first container.
                    // Remember that we need to return the Remote Services
                    // in the next iteration.
                    //
                    if ((pnr->dwUsage & RESOURCEUSAGE_CONTAINER) &&
                        (penet->dwRemote & RMF_CONTEXT))
                    {
                        penet->dwRemote |= RMF_SHOWREMOTE;
                    }

                    if ((penet->pnf->_uDisplayType == RESOURCEDISPLAYTYPE_SERVER) &&
                        (penet->grfFlags & SHCONTF_SHAREABLE))
                    {
                        // filter out ADMIN$ and IPC$, lame, based on str len
                        if (lstrlen(PathFindShareName(pnr->lpRemoteName)) > 2)
                            grfFlagsItem = 0;
                    }


                    // Check if we found requested type of net resource.
                    if (penet->grfFlags & grfFlagsItem)
                    {
                        // Yes.
                        ASSERT(lParam);     // else we leak here
                        if ( SUCCEEDED(_NetResToIDList(pnr, FALSE, TRUE, (penet->grfFlags & SHCONTF_NONFOLDERS), NULL, lParam)) )
                        {
                            break;
                        }
                    }
                }
                else if (err == WN_NO_MORE_ENTRIES) 
                {
                    hres = S_FALSE; // no more element
                    break;
                }
                else 
                {
                    DebugMsg(DM_ERROR, TEXT("sh ER - WNetEnumResource failed (%lx)"), err);
                    hres = E_FAIL;
                    break;
                }
            }
        }
    }
    else if (ecid == ECID_RELEASE)
    {
        penet->pnf->Release();              // release the "this" ptr we have

        if (penet->peunk)
            penet->peunk->Release();

        if ( penet->hEnum )
            WNetCloseEnum(penet->hEnum);

        LocalFree((HLOCAL)penet);
    }
    return hres;
}


// get the provider for an item or the folder itself. since some items don't have the
// provider stored we fall back to the folder to get the provider in that case
//
//  in:
//      pidn    item to get provider for. if NULL get provider for the folder
//
// returns:
//      NULL        no provider in the item or the folder
//      non NULL    address of passed in buffer

LPCTSTR CNetFolder::_GetProvider(LPCIDNETRESOURCE pidn, LPTSTR pszProvider, UINT cchProvider)
{
    if (pidn && NET_CopyProviderName(pidn, pszProvider, cchProvider))
        return pszProvider;

    if ( _pidnForProvider )
    {
        NET_CopyProviderName( _pidnForProvider, pszProvider, cchProvider );
        return pszProvider;
    }
    return NULL;
}


// construct a net idlist either copying the existing data from a pidl or 
// from a NETRESOURCE structure

HRESULT CNetFolder::_CreateNetIDList(LPIDNETRESOURCE pidnIn, 
                                     LPCTSTR pszName, LPCTSTR pszProvider, LPCTSTR pszComment, 
                                     LPITEMIDLIST *ppidl)
{
    LPBYTE pb;
    UINT cbmkid = SIZEOF(IDNETRESOURCE)-SIZEOF(CHAR);
    UINT cchName, cchProvider, cchComment, cbProviderType = 0;
    LPIDNETRESOURCE pidn;
    WORD wNetType = 0;
    BOOL fUnicode = FALSE;
    UINT  cchAnsiName, cchAnsiProvider, cchAnsiComment;
    CHAR szAnsiName[MAX_PATH], szAnsiProvider[MAX_PATH], szAnsiComment[MAX_PATH];

    ASSERT(ppidl != NULL);
    *ppidl = NULL;

    if (!pszName)
        pszName = c_szNULL;     // For now put in an empty string...

    if ( pszProvider )
        cbProviderType += SIZEOF(WORD);
    
#ifdef WINNT
    //
    // Win9x shipped with one set of provider name which are 
    // different on NT.  Therefore lets convert the NT one to
    // something that Win9x can understand.
    //

    if (pszProvider)
    {
        DWORD dwRes, dwType;
        INT i;

        cbProviderType = SIZEOF(WORD);
        dwRes = WNetGetProviderType(pszProvider, &dwType);
        if (dwRes == WN_SUCCESS)
        {
            wNetType = HIWORD(dwType);
            for (i=0; i < c_cProviders; i++)
            {
                if (c_rgProviderMap[i].wNetType == wNetType)
                {
                    pszProvider = c_rgProviderMap[i].lpName;
                    break;
                }
            }
        }
    }
#endif

    // compute the string lengths ready to build an IDLIST

    cchName = lstrlen(pszName)+1;
    cchProvider = pszProvider ? lstrlen(pszProvider)+1 : 0;
    cchComment = pszComment ? lstrlen(pszComment)+1 : 0;

    cchAnsiName = 0;
    cchAnsiProvider = 0;
    cchAnsiComment = 0;

    fUnicode  = !DoesStringRoundTrip(pszName, szAnsiName, ARRAYSIZE(szAnsiProvider));
    cchAnsiName = lstrlenA(szAnsiName)+1;

    if ( pszProvider )
    {
        fUnicode |= !DoesStringRoundTrip(pszProvider, szAnsiProvider, ARRAYSIZE(szAnsiProvider));
        cchAnsiProvider = lstrlenA(szAnsiProvider)+1;
    }

    if ( pszComment )
    {
        fUnicode |= !DoesStringRoundTrip(pszComment, szAnsiComment, ARRAYSIZE(szAnsiComment));
        cchAnsiComment = lstrlenA(szAnsiComment)+1;
    }

    // allocate and fill the IDLIST header

    cbmkid += cbProviderType+cchAnsiName + cchAnsiProvider + cchAnsiComment;

    if ( fUnicode )
        cbmkid += (SIZEOF(WCHAR)*(cchName+cchProvider+cchComment));

    pidn = (LPIDNETRESOURCE)_ILCreate(cbmkid + SIZEOF(USHORT));
    if (!pidn)
        return E_OUTOFMEMORY;

    pidn->cb = (WORD)cbmkid;
    pidn->bFlags = pidnIn->bFlags;
    pidn->uType = pidnIn->uType;
    pidn->uUsage = pidnIn->uUsage;

    if (pszProvider)
        pidn->uUsage |= NET_HASPROVIDER;

    if (pszComment)
        pidn->uUsage |= NET_HASCOMMENT;

    pb = (LPBYTE) pidn->szNetResName;

    //
    // write the ANSI strings into the IDLIST
    //

    StrCpyA((PSTR) pb, szAnsiName);
    pb += cchAnsiName;

    if ( pszProvider )
    {
        StrCpyA((PSTR) pb, szAnsiProvider);
        pb += cchAnsiProvider;
    }

    if ( pszComment )
    {
        StrCpyA((PSTR) pb, szAnsiComment);
        pb += cchAnsiComment;
    }

    // if we are going to be UNICODE then lets write those strings also.
    // Note that we must use unaligned string copies since the is no
    // promse that the ANSI strings will have an even number of characters
    // in them.

#ifdef UNICODE
    if ( fUnicode )
    {
        pidn->uUsage |= NET_UNICODE;
      
        ualstrcpyW((UNALIGNED WCHAR *)pb, pszName);
        pb += cchName*SIZEOF(WCHAR);

        if ( pszProvider )
        {
            ualstrcpyW((UNALIGNED WCHAR *)pb, pszProvider);
            pb += cchProvider*SIZEOF(WCHAR);
        }

        if ( pszComment )
        {
            ualstrcpyW((UNALIGNED WCHAR *)pb, pszComment);
            pb += cchComment*SIZEOF(WCHAR);
        }
    }
#endif

    //
    // and the trailing provider type
    //

    if (cbProviderType)
    {
        // Store the provider type
        pb = (LPBYTE)pidn + pidn->cb - SIZEOF(WORD);
        *((UNALIGNED WORD *)pb) = wNetType;
    }

    *ppidl = (LPITEMIDLIST)pidn;
    return S_OK;
}


// wrapper for converting a NETRESOURCE into an IDLIST via _CreateNetPidl

HRESULT CNetFolder::_NetResToIDList(NETRESOURCE *pnr, 
                                    BOOL fAllowNull, BOOL fKeepProviderName, BOOL fKeepComment, 
                                    LPITEMIDLIST *ppidl, LPARAM lParam)
{
    NETRESOURCE nr = *pnr;
    LPITEMIDLIST pidl;
    LPTSTR pszName, pszProvider, pszComment;
    IDNETRESOURCE idn;
    LPTSTR psz;

    if ( ppidl )
        *ppidl = NULL;

    switch (pnr->dwDisplayType) 
    {
        case RESOURCEDISPLAYTYPE_NETWORK:
            pszName = pnr->lpProvider;
            break;

        case RESOURCEDISPLAYTYPE_ROOT:
            pszName =pnr->lpComment;
            break;

        default:
        {
            // check the name for a NULL string (returned sometimes in NT4 domains)
            pszName = pnr->lpRemoteName;
            if ( !fAllowNull && !*pszName )
                return E_FAIL;

            // pretty stuff after the "\\"
            psz = (LPTSTR)SkipServerSlashes(pnr->lpRemoteName);
            if ( *psz )
                PathMakePretty(psz);
            break;
        }
    }

    pszProvider = fKeepProviderName ? nr.lpProvider:NULL;
    pszComment = fKeepComment ? nr.lpComment:NULL;
       
    idn.bFlags = (BYTE)(SHID_NET | (pnr->dwDisplayType & 0x0f));
    idn.uType  = (BYTE)(pnr->dwType & 0x0f);
    idn.uUsage = (BYTE)(pnr->dwUsage & 0x0f);

    // Is the current resource a share of some kind and not a container
    if ((pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE || pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SHAREADMIN) &&
        !(pnr->dwUsage & RESOURCEUSAGE_CONTAINER))
    {
        // If so, remember to delegate children of this folder to FSFolder
        idn.bFlags |= (BYTE)SHID_JUNCTION;    // \\server\share type thing
    }

    HRESULT hres = _CreateNetIDList(&idn, pszName, pszProvider, pszComment, &pidl);
    if ( SUCCEEDED(hres) )
    {
        if ( lParam )
            CDefEnum_SetReturn(lParam, (LPITEMIDLIST)pidl);
        if ( ppidl )
            *ppidl = pidl;
    }

    return hres;
}


HRESULT CNetFolder::_CreateEntireNetwork(LPITEMIDLIST *ppidl, LPARAM lParam)
{
    TCHAR szPath[MAX_PATH];
    NETRESOURCE nr = {0};

    // We need to add the Rest of network entry.  This is psuedo
    // bogus, as we should either always do it ourself or have
    // MPR always do it, but here it goes...
    LoadString(HINST_THISDLL, IDS_RESTOFNET, szPath, ARRAYSIZE(szPath));
    nr.dwDisplayType = RESOURCEDISPLAYTYPE_ROOT;
    nr.dwType = RESOURCETYPE_ANY;
    nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    nr.lpComment = szPath;

    return _NetResToIDList(&nr, TRUE, FALSE, FALSE, ppidl, lParam);     // allows NULL namess
}

//===========================================================================
//
// To be called back from within CDefFolderMenu
//
STDAPI CNetwork_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                              IDataObject *pdtobj, UINT uMsg, 
                              WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;
    CNetFolder* pThis;

    if (FAILED(psf->QueryInterface(CLSID_CNetFldr, (void**) &pThis)))
        return E_UNEXPECTED;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & (CMF_VERBSONLY | CMF_DVFILE)))
        {
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_NETWORK_BACKGROUND,
                    POPUP_NETWORK_POPUPMERGE, (LPQCMINFO)lParam);
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        switch(wParam)
        {
        case FSIDM_SORTBYNAME:
        case FSIDM_SORTBYCOMMENT:
            ShellFolderView_ReArrange(hwnd, (wParam == FSIDM_SORTBYNAME) ? 0 : 1);
            break;

        case FSIDM_PROPERTIESBG:
            hres = SHPropertiesForPidl(hwnd, pThis->_pidl, (LPCTSTR)lParam);
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


//===========================================================================
//
// To be called back from within CDefFolderMenu
//
STDAPI CNetFolder::DFMCallBack(IShellFolder* psf, HWND hwnd,
                                  IDataObject* pdtobj, UINT uMsg, 
                                  WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (pdtobj)
        {
            STGMEDIUM medium;
            LPIDA pida;
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            UINT idCmdBase = pqcm->idCmdFirst; // must be called before merge
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_NETWORK_ITEM, 0, pqcm);

            pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                if (pida->cidl > 0)
                {
                    LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)IDA_GetIDListPtr(pida, 0);

                    // Only enable "connect" command if the first one is a share.
                    if (pidn)
                    {
                        ULONG rgf = 0;
                        if( NET_GetFlags(pidn) & SHID_JUNCTION &&
                            !SHRestricted( REST_NONETCONNECTDISCONNECT ) )
                        {
                            EnableMenuItem(pqcm->hmenu, idCmdBase + FSIDM_CONNECT,
                                MF_CHECKED | MF_BYCOMMAND);
                        }
                    }
                }
                HIDA_ReleaseStgMedium(pida, &medium);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        switch(wParam)
        {
        case DFM_CMD_PROPERTIES:
            SHLaunchPropSheet(_PropertiesThreadProc, pdtobj, (LPCTSTR)lParam, psf, NULL);
            break;

        case DFM_CMD_LINK:
            {
                hres = S_FALSE; // do the default shortcut stuff
                CNetFolder* pThis;
                if (SUCCEEDED(psf->QueryInterface(CLSID_CNetFldr, (void**) &pThis)))
                {
                    // net hood special case.  in this case we want to create the shortuct
                    // in the net hood, not offer to put this on the desktop
                    IShellFolder2* psfFiles;
                    if (SUCCEEDED(pThis->v_GetFileFolder(&psfFiles)))
                    {
                        FS_CreateLinks(hwnd, psfFiles, pdtobj, (LPCTSTR)lParam, CMIC_MASK_FLAG_NO_UI);
                        hres = S_OK;    // we created the links
                    }
                }
            }
            break;

        case FSIDM_CONNECT:
            if (pdtobj)
            {
                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
                if (pida)
                {
                    UINT iidl;
                    for (iidl = 0; iidl < pida->cidl; iidl++)
                    {
                        LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)IDA_GetIDListPtr(pida, iidl);

                        // Only execute "connect" on shares.
                        if (NET_GetFlags(pidn) & SHID_JUNCTION)
                        {
                            TCHAR szName[MAX_PATH];
                            LPTSTR pszName = NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
                            DWORD err = SHStartNetConnectionDialog(hwnd, pszName, RESOURCETYPE_DISK);
                            DebugMsg(DM_TRACE, TEXT("CNet FSIDM_CONNECT (%s, %x)"), szName, err);

                            // events will get generated automatically
                        }
                    }
                    HIDA_ReleaseStgMedium(pida, &medium);
                }
            }
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

STDAPI CNetFolder::PrinterDFMCallBack(IShellFolder* psf, HWND hwnd,
                                      IDataObject* pdtobj, UINT uMsg, 
                                      WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        //
        //  Returning S_FALSE indicates no need to get verbs from
        // extensions.
        //
        hres = S_FALSE;
        break;

    // if anyone hooks our context menu, we want to be on top (Open)
    case DFM_MERGECONTEXTMENU_TOP:
        if (pdtobj)
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;

            // insert verbs
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_NETWORK_PRINTER, 0, pqcm);
#ifndef WINNT
            //
            // WINNT does not support Capturing print ports, so no
            // need to check.
            //
            if (!(GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS))
            {
                // remove "map" if no net
                DeleteMenu(pqcm->hmenu, pqcm->idCmdFirst + FSIDM_CONNECT_PRN, MF_BYCOMMAND);
            }
#endif
            SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        switch(wParam)
        {
        case DFM_CMD_PROPERTIES:
            SHLaunchPropSheet(_PropertiesThreadProc, pdtobj, (LPCTSTR)lParam, psf, NULL);
            break;

        case DFM_CMD_LINK:
            // do the default create shortcut crap
            return S_FALSE;

        case FSIDM_OPENPRN:
        case FSIDM_NETPRN_INSTALL:
#ifndef WINNT
        case FSIDM_CONNECT_PRN:
#endif
        {
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                UINT action, i;

                // set up the operation we are going to perform
                switch (wParam) {
                case FSIDM_OPENPRN:
                    action = PRINTACTION_OPENNETPRN;
                    break;
                case FSIDM_NETPRN_INSTALL:
                    action = PRINTACTION_NETINSTALL;
                    break;
                default: // FSIDM_CONNECT_PRN
                    action = (UINT)-1;
                    break;
                }

                for (i = 0; i < pida->cidl; i++)
                {
                    LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)IDA_GetIDListPtr(pida, i);

                    // Only execute command for a net print share
                    if (_IsPrintShare(pidn))
                    {
                        TCHAR szName[MAX_PATH];
                        NET_CopyResName(pidn,szName,ARRAYSIZE(szName));

#ifndef WINNT // PRINTQ
                        if (action == (UINT)-1)
                        {
                            SHNetConnectionDialog(hwnd, szName, RESOURCETYPE_PRINT);
                        }
                        else
#endif
                        {
                            SHInvokePrinterCommand(hwnd, action, szName, NULL, FALSE);
                        }
                    }
                } // for (i...
                HIDA_ReleaseStgMedium(pida, &medium);
            } // if (medium.hGlobal)
            break;
        } // case ID_NETWORK_PRINTER_INSTALL, FSIDM_CONNECT_PRN

        } // switch(wparam)
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}


//
// REVIEW: Almost identical code in fstreex.c
//
DWORD CALLBACK CNetFolder::_PropertiesThreadProc(void *pv)
{
    PROPSTUFF* pps = (PROPSTUFF *)pv;
    CNetFolder* pThis;
    if (SUCCEEDED(pps->psf->QueryInterface(CLSID_CNetFldr, (void**) &pThis)))
    {
        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pps->pdtobj, &medium);
        if (pida)
        {
            // Yes, do context menu.
            HKEY ahkeys[NKID_COUNT];
            HRESULT hres = pThis->_OpenKeys((LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, 0), ahkeys);
            if (SUCCEEDED(hres))
            {
                LPTSTR pszCaption = SHGetCaption(medium.hGlobal);
                SHOpenPropSheet(pszCaption, ahkeys, ARRAYSIZE(ahkeys),
                                &CLSID_ShellNetDefExt,
                                pps->pdtobj, NULL, pps->pStartPage);
                if (pszCaption)
                    SHFree(pszCaption);

                SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
            }

            HIDA_ReleaseStgMedium(pida, &medium);
        }
    }
    return S_OK;
}

STDAPI CNetFolder::GAOCallbackNet(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut)
{
    LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)pidl;
    ULONG rgfOut = SFGAO_CANLINK | SFGAO_HASPROPSHEET | SFGAO_HASSUBFOLDER |
                   SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER;

    if (NET_GetFlags(pidn) & SHID_JUNCTION)
    {
        if ((NET_GetType(pidn) == RESOURCETYPE_DISK) || 
            (NET_GetType(pidn) == RESOURCETYPE_ANY))
            rgfOut |= SFGAO_FILESYSTEM | SFGAO_DROPTARGET;
        else
            rgfOut &= ~SFGAO_FILESYSANCESTOR;
    }

    if (_IsPrintShare(pidn))
    {
        rgfOut |= SFGAO_DROPTARGET; // for drag and drop printing
        rgfOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_CANMONIKER | SFGAO_HASSUBFOLDER);
    }

    if (NET_IsRemoteFld(pidn))
    {
        rgfOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM | SFGAO_CANMONIKER);
    }

    *prgfInOut = rgfOut;
    return S_OK;

}

// This is only used by the CNetRootFolder subclass, but because we can only QI for
// CLSID_NetFldr, and we can't access protected members of any CNetFolder instance 
// from a member function of CNetRootFolder, we'll make it belong to CNetFolder

HRESULT CALLBACK CNetFolder::GAOCallbackNetRoot(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut)
{
    CNetFolder* pNetF;
    HRESULT hres = psf->QueryInterface(CLSID_CNetFldr, (void**) &pNetF);
    if (SUCCEEDED(hres))
    {
        if (NET_IsValidID(pidl))
        {
            hres = pNetF->CNetFolder::GetAttributesOf(1, &pidl, prgfInOut);
        }
        else 
        {
            IShellFolder2* psfFiles;
            hres = pNetF->v_GetFileFolder(&psfFiles);
            if (SUCCEEDED(hres))
                hres = psfFiles->GetAttributesOf(1, &pidl, prgfInOut);
        }
    }
    return hres;
}

// this is called by netfind.c

STDMETHODIMP CNetwork_EnumSearches(IShellFolder2* psf2, LPENUMEXTRASEARCH *ppenum)
{
    HRESULT hres;
    if (NULL != psf2)
    {
        CNetFolder* pNetF;
        hres = psf2->QueryInterface(CLSID_CNetFldr, (void**) &pNetF);
        if (SUCCEEDED(hres))
            hres = pNetF->EnumSearches(ppenum);
    }
    else
    {
        hres = E_INVALIDARG;
    }
    return hres;
}


// given the resulting ppidl and a pszRest continue to parse through and add in the remainder
// of the file system path.

HRESULT CNetFolder::_ParseRest(LPBC pbc, LPCWSTR pszRest, LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    HRESULT hres = S_OK;
    if ( pszRest && pszRest[0] )
    {
        IShellFolder* psfBind;
        // need to QI to get the agregated case
        hres = QueryInterface(IID_IShellFolder, (void**) &psfBind);
        if (SUCCEEDED(hres))
        {
            IShellFolder* psfSub;
            // pass down to pick off stuff below including regitems and file names

            hres = psfBind->BindToObject(*ppidl, NULL, IID_IShellFolder, (void**) &psfSub);
            if (SUCCEEDED(hres))
            {
                LPITEMIDLIST pidlSubDir;

                // skip leading \ if there is one present
                if ( pszRest[0] == L'\\' )
                    pszRest++;

                hres = psfSub->ParseDisplayName(NULL, pbc, (LPWSTR)pszRest, NULL, &pidlSubDir, pdwAttributes);
                if (SUCCEEDED(hres))
                {
                    hres = SHILAppend(pidlSubDir, ppidl);
                }
                psfSub->Release();
            }
            psfBind->Release();
        }
    }
    else
    {
        if ( pdwAttributes )
        {
            LPCITEMIDLIST apidlLast[1] = { ILFindLastID(*ppidl) };
            hres = GetAttributesOf(1, apidlLast, pdwAttributes);
        }
    }

    return hres;
}


// handle parsing a net name into a IDLIST structure that represents that NETRESOURCE
// up to the root.  we loop using each NETRESOURCE and calling NetResourceGetParent,
// we assume that if we receive a ERROR_BAD_NET_NAME we are done.

HRESULT CNetFolder::_NetResToIDLists(NETRESOURCE *pnr, DWORD dwbuf, LPITEMIDLIST *ppidl)
{
    HRESULT hres = S_OK;

    do
    {
        LPITEMIDLIST pidlT;
        hres = _NetResToIDList(pnr, FALSE, TRUE, TRUE, &pidlT, 0);
        if ( SUCCEEDED(hres) )
        {
            hres = SHILPrepend(pidlT, ppidl);
            if ( FAILED(hres) )
            {
                ILFree(pidlT);
            }
            else
            {
                // lets get the resource parent, if the display type is ROOT
                // then there is no point as we are already at the top of the
                // chain, calling for root will just fail (or on Win9x cause 
                // us to loop forever, eat all the stack and die!).

                if ( (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_NETWORK) ||
                      (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_ROOT) ||
                       (WNetGetResourceParent(pnr, pnr, &dwbuf) != WN_SUCCESS) )
                {
                    break;
                }
            }
        }
    }
    while ( SUCCEEDED(hres) );

    return hres;
}


// get the parsable network name from the object

LPTSTR CNetFolder::_GetNameForParsing(LPCWSTR pwszName, LPTSTR pszBuffer, INT cchBuffer, LPTSTR *ppszRegItem)
{
    LPTSTR pszRegItem = NULL;
    INT cSlashes = 0;

    *ppszRegItem = NULL;
    
    SHUnicodeToTChar(pwszName, pszBuffer, cchBuffer);    

    // remove the trailing \ if there is one, NTLanMan barfs if we pass a string containing it

    INT cchPath = lstrlen(pszBuffer)-1;
    if ( (cchPath > 2) && (pszBuffer[cchPath] == TEXT('\\')) )
        pszBuffer[cchPath] = TEXT('\0');

    // lets walk the name, look for \:: squence to signify the start of a regitem name,
    // and if the number of slashes is > 2 then we should bail
    
    LPTSTR pszUNC = pszBuffer+2;    
    while ( pszUNC && *pszUNC && (cSlashes < 2) )
    {
        if ( (pszUNC[0] == TEXT('\\')) && 
                (pszUNC[1] == TEXT(':')) && (pszUNC[2] == TEXT(':')) )
        {
            *ppszRegItem = pszUNC;
            break;
        }

        pszUNC = StrChr(pszUNC+1, TEXT('\\'));
        cSlashes++;
    }

    return pszUNC;
}


HRESULT CNetFolder::_ParseNetName(HWND hwnd, LPBC pbc, 
                                  LPCWSTR pwszName, ULONG* pchEaten,
                                  LPITEMIDLIST *ppidl, DWORD *pdwAttrib)
{
    HRESULT hres;
    NETRESOURCE nr = { 0 };
    struct _NRTEMP 
    {
        NETRESOURCE nr;
        TCHAR szBuffer[1024];
    } nrOut = { 0 };
    TCHAR szPath[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    DWORD dwres, dwbuf = SIZEOF(nrOut.szBuffer);
    LPTSTR pszServerShare = NULL;
    LPTSTR pszRestOfName = NULL;
    LPTSTR pszFakeRestOfName = NULL;
    LPTSTR pszRegItem = NULL;

    // validate the name before we start cracking it...

    if ( !PathIsUNCW(pwszName) )
        return HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME);

    pszFakeRestOfName = _GetNameForParsing(pwszName, szPath, ARRAYSIZE(szPath), &pszRegItem);

    nr.lpRemoteName = szPath;
    nr.lpProvider = (LPTSTR)_GetProvider(NULL, szTemp, ARRAYSIZE(szTemp));
    nr.dwType = RESOURCETYPE_ANY;

    // if there is a regitem string then we must truncate at it, otherwise MPR will
    // try and parse it as part of the name

    if ( pszRegItem )
        *pszRegItem = TEXT('\0');

#ifdef WINNT
    dwres = WNetGetResourceInformation(&nr, &nrOut.nr, &dwbuf, &pszRestOfName);    
    if ( WN_SUCCESS != dwres )
#endif
    {
        TCHAR cT;
        LPTSTR pszTemp;

        // truncate the string at the \\server\share to try and parse the name,
        // note at this point if MPR resolves the alias on a Novel server this could
        // get very confusing (eg. \\strike\foo\bah may resolve to \\string\bla,
        // yet our concept of what pszRestOfName will be wrong!
    
        if ( pszFakeRestOfName )
        {
            cT = *pszFakeRestOfName;
            *pszFakeRestOfName = TEXT('\0');
        }

        dwres = WNetGetResourceInformation(&nr, &nrOut.nr, &dwbuf, &pszTemp);    
        if ( dwres != WN_SUCCESS )
        {
            // we failed to get the net connection information using the truncated
            // string, therefore lets try and add a connection.  if that works
            // then we can assume that the connection exists, and we should
            // just fake up a NR output structure.

            dwres = WNetAddConnection3(NULL, &nr, NULL, NULL, 0);
            if ( dwres == WN_SUCCESS )
            {
                nrOut.nr = nr;
                nrOut.nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
                nrOut.nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
                
                StrCpy(szTemp, nrOut.nr.lpRemoteName);        // copy away the \\server\share string
                nrOut.nr.lpRemoteName = szTemp;
            }
        }

        if ( pszFakeRestOfName )
            *pszFakeRestOfName = cT;

        pszRestOfName = pszFakeRestOfName;
    }

    if ( WN_SUCCESS == dwres )
    {
        WCHAR wszRestOfName[MAX_PATH] = { 0 };

        if ( pszRestOfName )
            SHTCharToUnicode(pszRestOfName, wszRestOfName, ARRAYSIZE(wszRestOfName));

        // assume we are truncating at the regitem and parsing through

        if ( pszRegItem )
            pszRestOfName = pszRegItem;

        // attempt to convert the NETRESOURCE to a string to IDLISTS by walking the
        // parents, then add in Entire Network

        hres = _NetResToIDLists(&nrOut.nr, dwbuf, ppidl);
        if ( SUCCEEDED(hres) )
        {
            LPITEMIDLIST pidlT;
            hres = _CreateEntireNetwork(&pidlT, 0);     
            if ( SUCCEEDED(hres) )
            {
                hres = SHILPrepend(pidlT, ppidl);
                if ( FAILED(hres) )
                    ILFree(pidlT);
            }
        }

        // if we have a local string then lets continue to parse it by binding to 
        // its parent folder, otherwise we just want to return the attributes

        if ( SUCCEEDED(hres) )
            hres = _ParseRest(pbc, wszRestOfName, ppidl, pdwAttrib);
    }
    else
    {
        hres = HRESULT_FROM_WIN32(dwres);
    }

    if ( FAILED(hres) )
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    return hres;
}


//
// simple name parsing for the network paths.  this makes big assumptions about the
// \\server\share format we are given, and the type of IDLISTs to return.
//

HRESULT CNetFolder::_AddUnknownIDList(DWORD dwDisplayType, LPITEMIDLIST *ppidl)
{
    HRESULT hres = E_OUTOFMEMORY;
    NETRESOURCE nr = { 0 };
    LPITEMIDLIST pidlT;

    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwDisplayType = dwDisplayType;
    nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    nr.lpRemoteName = TEXT("\0");               // null name means fake item

    hres = _NetResToIDList(&nr, TRUE, FALSE, FALSE, &pidlT, 0);
    if ( SUCCEEDED(hres) )
    {
        hres = SHILAppend(pidlT, ppidl);
        if ( FAILED(hres) )
            ILFree(pidlT);
    }

    return hres;
}

HRESULT CNetFolder::_ParseSimple(LPBC pbc, LPWSTR pszName, LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    HRESULT hres = S_OK;
    NETRESOURCE nr = {0};
    TCHAR szName[MAX_PATH];
    LPTSTR psz;
    LPWSTR pszSlash;
    LPCITEMIDLIST pidlMapped;
    LPITEMIDLIST pidlT;
    USES_CONVERSION;

    *ppidl = NULL;

    if ( !PathIsUNCW(pszName) )
        return E_INVALIDARG;

    // see if there is a UNC root mapping for this, if so we are golden

    SHUnicodeToTChar(pszName, szName, ARRAYSIZE(szName));
    psz = (LPTSTR)NPTMapNameToPidl(szName, &pidlMapped);
    if (pidlMapped)
    {
        hres = SHILClone(_ILNext(pidlMapped), ppidl); // skip the MyNetPlaces part
        pszSlash = (LPWSTR)pszName + (psz - szName);
    }
    else
    {
        // create the entire network IDLIST, provider and domain elements

        hres = _CreateEntireNetwork(ppidl, 0);

#ifdef WINNT
        if ( SUCCEEDED(hres) )
            hres = _AddUnknownIDList(RESOURCEDISPLAYTYPE_NETWORK, ppidl);
#endif

        if ( SUCCEEDED(hres) )
            hres = _AddUnknownIDList(RESOURCEDISPLAYTYPE_DOMAIN, ppidl);

        // create the server IDLIST

        if ( SUCCEEDED(hres) )
        {
            pszSlash = StrChrW(pszName+2, L'\\');

            if ( pszSlash )
                *pszSlash = L'\0';

            nr.dwScope = RESOURCE_GLOBALNET;
            nr.dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
            nr.dwType = RESOURCETYPE_DISK;
            nr.dwUsage = RESOURCEUSAGE_CONTAINER;
            nr.lpRemoteName = W2T(pszName);

            hres = _NetResToIDList(&nr, FALSE, FALSE, FALSE, &pidlT, 0);
            if ( SUCCEEDED(hres) )
                hres = SHILAppend(pidlT, ppidl);

            if ( pszSlash )
                *pszSlash = L'\\';

            // if we have a trailing \ then lets add in the share part of the IDLIST

            if ( SUCCEEDED(hres) && pszSlash )
            {
                pszSlash = StrChrW(pszSlash+1, L'\\');
                if (pszSlash)
                    *pszSlash = L'\0';

                nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
                nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
                nr.lpRemoteName = W2T(pszName);

                hres = _NetResToIDList(&nr, FALSE, FALSE, FALSE, &pidlT, 0);
                if ( SUCCEEDED(hres) )
                    hres = SHILAppend(pidlT, ppidl);

                if (pszSlash)
                    *pszSlash = L'\\';
            }
        }
    }

    if ( FAILED(hres) )
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }
    else
    {
        hres = _ParseRest(pbc, pszSlash, ppidl, pdwAttributes);
    }
    
    return hres;
}


// try parsing out the EntireNet or localised version.  if we find that object then try and
// parse through that to the regitems or other objects which live below.   this inturn
// will cause an instance of CNetFolder to be created to generate the other parsing names.
//
// returns:
//      S_FALSE         - not rest of net, try something else
//      S_OK            - was rest of net, use this
//      FAILED(hres)    - error result, return

HRESULT CNetRootFolder::_TryParseEntireNet(HWND hwnd, LPBC pbc, WCHAR *pwszName, LPITEMIDLIST *ppidl, DWORD *pdwAttributes)
{
    HRESULT hres = S_FALSE; // skip, not rest of net
 
    *ppidl = NULL;

    if ( !PathIsUNCW(pwszName) )
    {
        const WCHAR szEntireNetwork[] = L"EntireNetwork";
        WCHAR szRestOfNet[128];
        INT cchRestOfNet = LoadStringW(HINST_THISDLL, IDS_RESTOFNET, szRestOfNet, ARRAYSIZE(szRestOfNet));
       
        BOOL fRestOfNet = !StrCmpNIW(szRestOfNet, pwszName, cchRestOfNet);
        if ( !fRestOfNet && !StrCmpNIW(szEntireNetwork, pwszName, ARRAYSIZE(szEntireNetwork)-1) ) 
        {
            fRestOfNet = TRUE;
            cchRestOfNet = ARRAYSIZE(szEntireNetwork)-1;
        }
        
        if ( fRestOfNet )
        {
            hres = _CreateEntireNetwork(ppidl, 0);
            if (SUCCEEDED(hres))
            {
                if (pdwAttributes)
                    GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttributes);

                hres = S_OK;
            }

            // 
            // if we find extra stuff after the name then lets bind and continue the parsing
            // from there on.  this is needed so the we can access regitems burried inside
            // entire net.
            //
            // eg:  EntireNetwork\\::{clsid}
            //

            if ( SUCCEEDED(hres) && 
                    (pwszName[cchRestOfNet] == L'\\') && pwszName[cchRestOfNet+1] )
            {
                IShellFolder *psfRestOfNet;
                hres = BindToObject(*ppidl, NULL, IID_IShellFolder, (void **)&psfRestOfNet);
                if ( SUCCEEDED(hres) )
                {
                    LPITEMIDLIST pidl;
                    hres = psfRestOfNet->ParseDisplayName(hwnd, pbc, 
                                                          pwszName+cchRestOfNet+1, 
                                                          NULL,  
                                                          &pidl, 
                                                          pdwAttributes);
                    if  ( SUCCEEDED(hres) )
                        hres = SHILAppend(pidl, ppidl);                        

                    psfRestOfNet->Release();
                }
            }
        }
    }

    return hres;
}


// CNetRootFolder::ParseDisplayname
//  - swtich based on the file system context to see if we need to do a simple parse or not,
//  - check for "EntireNet" and delegate parsing as required.

STDMETHODIMP CNetRootFolder::ParseDisplayName(HWND hwnd, LPBC pbc, WCHAR* pszName, ULONG* pchEaten,
                                              LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    HRESULT hres = _TryParseEntireNet(hwnd, pbc, pszName, ppidl, pdwAttributes);
    if (hres == S_OK)
        return hres;

    hres = SHIsFileSysBindCtx(pbc, NULL);
    if (S_OK == hres)
    {
        hres = _ParseSimple(pbc, pszName, ppidl, pdwAttributes);
    }
    else
    {
        hres = _ParseNetName(hwnd, pbc, pszName, pchEaten, ppidl, pdwAttributes);
        if ((HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME) == hres))
        {
            IShellFolder2 *psfFiles;
            if (SUCCEEDED(v_GetFileFolder(&psfFiles)))
                hres = psfFiles->ParseDisplayName(hwnd, pbc, pszName, pchEaten, ppidl, pdwAttributes);
        }
    }
    return hres;
}



STDMETHODIMP CNetRootFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenum)
{
    HRESULT hres = E_OUTOFMEMORY;
    ENUM_DATA *penet = (ENUM_DATA *)LocalAlloc(LPTR, SIZEOF(ENUM_DATA));
    if (penet)
    {
        // 
        // get the link enumerator on the first call into the net enumerator
        //

        AddRef();
        penet->pnf = this;

        penet->dwRemote |= RMF_GETLINKENUM;

        //
        // Do we enumerate the workgroup?
        //
    
        if ( !SHRestricted(REST_ENUMWORKGROUP) )
        {
            DWORD dwValue, dwSize = SIZEOF(dwValue);

            // Don't enumerate the workgroup, if the restriction says so
            penet->dwRemote |= RMF_FAKENETROOT;

            // Check the WNet policy to see if we should be showing the entire net object, if not 
            // mark it as shown so that the enumerator doesn't return it.
            if ( ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, WNET_POLICY_KEY, TEXT("NoEntireNetwork"), 
                                                                            NULL, (void *)&dwValue, &dwSize) )
            {
                if ( dwValue )
                    penet->dwRemote |= RMF_ENTIRENETSHOWN;
            }
        }

        //
        // if we are not faking the net root then lets call _OpenEnum, otherwise lets ignore
        //

        if (!(penet->dwRemote & RMF_FAKENETROOT))
        {
            DWORD err = _OpenEnum(hwnd, grfFlags, NULL, &penet->hEnum);

            // Always add the remote folder to the 'hood
            if (WN_SUCCESS != err)
            {
                // Yes; still show remote anyway (only)
                penet->dwRemote |= RMF_SHOWREMOTE;
            }
            else
            {
                // No; allow everything to be enumerated in the 'hood.
                penet->dwRemote |= RMF_CONTEXT;
            }
        }

        penet->grfFlags = grfFlags;
        hres = SHCreateEnumObjects(hwnd, penet, EnumCallBack, ppenum);
    }
    return hres;
}

STDMETHODIMP CNetRootFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut)
{
    HRESULT hres;
    if (NET_IsValidID(pidl))
        hres = CNetFolder::BindToObject(pidl, pbc, riid, ppvOut);
    else
    {
        IShellFolder2* psfFiles;
        hres = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hres))
            hres = psfFiles->BindToObject(pidl, pbc, riid, ppvOut);
    }
    return hres;
}

STDMETHODIMP CNetRootFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = E_INVALIDARG;

    // First obtain the collate type of the pidls and their respective
    // collate order.

    LONG iColateType1 = _GetFilePIDLType(pidl1);
    LONG iColateType2 = _GetFilePIDLType(pidl2);

    if (iColateType1 == iColateType2) 
    {
        // pidls are of same type.
        if (iColateType1 == _HOOD_COL_FILE)  // two file system pidls
        {
            SHCOLUMNID     scid;
            IShellFolder2* psfFiles;
            
            if (SUCCEEDED(v_GetFileFolder(&psfFiles)) &&
                SUCCEEDED(MapColumnToSCID((UINT)iCol & SHCIDS_COLUMNMASK, &scid)))
            {
                if (IsEqualSCID(scid, SCID_NAME))
                {
                    //  Name is a no-brainer; delegate directly to the fs folder
                    return psfFiles->CompareIDs(iCol, pidl1, pidl2);
                }
                else
                {
                    //  Other columns are extended columns.   Let FS folder
                    //  acquire the extended column data abstractly from handlers,
                    //  then we'll compare the values ourselves.

                    //  BUGBUG [scotthan]: We're limited to VT_BSTR values.
                    VARIANT var1 = {0}, var2 = {0};
                    if (FAILED(psfFiles->GetDetailsEx(pidl1, &scid, &var1)) || var1.vt != VT_BSTR)
                        VariantClear(&var1);

                    if (FAILED(psfFiles->GetDetailsEx(pidl2, &scid, &var2)) || var2.vt != VT_BSTR)
                        VariantClear(&var2);

                    if (var1.bstrVal && var2.bstrVal) 
                        hres = ResultFromShort(StrCmpW(var1.bstrVal, var2.bstrVal));
                    else if (var1.bstrVal)
                        hres = ResultFromShort(1);
                    else if (var2.bstrVal)
                        hres = ResultFromShort(-1);
                    else
                        hres = ResultFromShort(0);

                    VariantClear(&var1);
                    VariantClear(&var2);
                }
            }
        }
        else 
        {
            // pidls same and are not of type file,
            // so both must be a type understood
            // by the CNetwork class - pass on to compare.

            return CNetFolder::CompareIDs(iCol, pidl1, pidl2);
        }
    }
    else 
    {
        // ensure that entire network ends up at the head of the list

        LPCIDNETRESOURCE pidn1 = NET_IsValidID(pidl1);
        LPCIDNETRESOURCE pidn2 = NET_IsValidID(pidl2);

        if ( (pidn1 && (NET_GetDisplayType(pidn1) == RESOURCEDISPLAYTYPE_ROOT)) ||
              (pidn2 && (NET_GetDisplayType(pidn2) == RESOURCEDISPLAYTYPE_ROOT)) )
        {
            if ( iColateType1 == _HOOD_COL_FILE )
                return ResultFromShort(1);
            else
                return ResultFromShort(-1);
        }

        // pidls are not of same type, so have already been correctly
        // collated (consequently, sorting is first by type and
        // then by subfield).

        return ResultFromShort(((iColateType2 - iColateType1) > 0) ? 1 : -1);
    }
    return hres;
}

STDMETHODIMP CNetRootFolder::CreateViewObject(HWND hwnd, REFIID riid, void** ppvOut)
{
    ASSERT(ILIsEqual(_pidl, (LPCITEMIDLIST)&c_idlNet));

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        return CIDLDropTarget_Create(hwnd, &c_CNetRootTargetVtbl, _pidl, (IDropTarget**) ppvOut);
    }
    return CNetFolder::CreateViewObject(hwnd, riid, ppvOut);
}


STDMETHODIMP CNetRootFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    HRESULT hres;

    if (cidl == 0)
    {
        //
        // The user can rename links in the hood.
        //
        hres = CNetFolder::GetAttributesOf(cidl, apidl, prgfInOut);
        *prgfInOut |= SFGAO_CANRENAME;
    }
    else
    {
        hres = Multi_GetAttributesOf((IShellFolder2*) this, cidl, apidl, prgfInOut, GAOCallbackNetRoot);
    }
    return hres;
}

STDMETHODIMP CNetRootFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET* pStrRet)
{
    HRESULT hres;
    if (NET_IsValidID(pidl) || IsSelf(1, &pidl))
    {
        hres = CNetFolder::GetDisplayNameOf(pidl, dwFlags, pStrRet);
    }
    else
    {
        IShellFolder2* psfFiles;
        hres = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hres))
            hres = psfFiles->GetDisplayNameOf(pidl, dwFlags, pStrRet);
    }
    return hres;
}

STDMETHODIMP CNetRootFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName,
                                       DWORD dwRes, LPITEMIDLIST* ppidl)
{
    HRESULT hres;
    if (NET_IsValidID(pidl))
    {
        hres = CNetFolder::SetNameOf(hwnd, pidl, lpszName, dwRes, ppidl);
    }
    else
    {
        IShellFolder2* psfFiles;
        hres = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hres))
            hres = psfFiles->SetNameOf(hwnd, pidl, lpszName, dwRes, ppidl);
    }
    return hres;
}

STDMETHODIMP CNetRootFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                           REFIID riid, UINT* prgfInOut, void** ppvOut)
{
    HRESULT hres = E_INVALIDARG;
    LPCIDNETRESOURCE pidn = cidl ? NET_IsValidID(apidl[0]) : NULL;
    BOOL fStriped = FALSE;

    ASSERT(ILIsEqual(_pidl, (LPCITEMIDLIST)&c_idlNet));

    *ppvOut = NULL;

    if (pidn)
    {
        fStriped = _MakeStripToLikeKinds(&cidl, &apidl, TRUE);

        if (IsEqualIID(riid, IID_IContextMenu))
        {
            HKEY ahkeys[NKID_COUNT];

            hres = _OpenKeys(pidn, ahkeys);
            if (SUCCEEDED(hres))
            {
                IShellFolder* psfOuter;
                hres = QueryInterface(IID_IShellFolder, (void**) &psfOuter);
                if (SUCCEEDED(hres))
                {
                    hres = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, 
                                                  psfOuter, _GetCallbackType(pidn),
                                                  ARRAYSIZE(ahkeys), ahkeys, (IContextMenu**) ppvOut);
                    psfOuter->Release();
                }
                SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
            }
        }
        else
            hres = CNetFolder::GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
    }
    else
    {
        fStriped = _MakeStripToLikeKinds(&cidl, &apidl, FALSE);

        IShellFolder2* psfFiles;
        hres = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hres))
            hres = psfFiles->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    if (fStriped)
        LocalFree((HLOCAL)apidl);
    return hres;

}

STDMETHODIMP CNetRootFolder::GetClassID(CLSID* pCLSID)
{
    *pCLSID = CLSID_NetworkPlaces;
    return S_OK;
}

STDMETHODIMP CNetRootFolder::Initialize(LPCITEMIDLIST pidl)
{
    ASSERT(ILIsEqual(pidl, (LPCITEMIDLIST)&c_idlNet));
    // Only allow the Net root on the desktop
    HRESULT hres = !IsIDListInNameSpace(pidl, &CLSID_NetworkPlaces) || !ILIsEmpty(_ILNext(pidl)) ? E_INVALIDARG : S_OK;
    if (SUCCEEDED(hres))
    {
        hres = SHILClone(pidl, &_pidl);
    }
    return hres;
}

LONG CNetFolder::_GetFilePIDLType(LPCITEMIDLIST pidl)
{
    if (NET_IsValidID(pidl)) 
    {
        if (NET_IsRemoteFld((LPIDNETRESOURCE)pidl)) 
        {
            return _HOOD_COL_REMOTE;
        }
        if (NET_GetDisplayType((LPIDNETRESOURCE)pidl) == RESOURCEDISPLAYTYPE_ROOT) 
        {
            return _HOOD_COL_RON;
        }
        return _HOOD_COL_NET;
    }
    return _HOOD_COL_FILE;
}


/* This function adds a provider name to an IDLIST that doesn't already have one. */
/* A new IDLIST pointer is returned; the old pointer is no longer valid. */

LPITEMIDLIST CNetFolder::_AddProviderToPidl(LPITEMIDLIST pidl, LPCTSTR lpProvider)
{
    LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)pidl;

    if ( !NET_FHasProvider(pidn) )
    {
        LPITEMIDLIST pidlres;
        TCHAR szName[MAX_PATH], szComment[MAX_PATH];

        // construct a new IDLIST preserving the name, comment and other information

        NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
        NET_CopyComment(pidn, szComment, ARRAYSIZE(szComment));

        HRESULT hres = _CreateNetIDList(pidn, szName, lpProvider, szComment[0] ? szComment:NULL, &pidlres);
        if ( SUCCEEDED(hres) && !ILIsEmpty(_ILNext(pidl)) )
        {
            LPITEMIDLIST pidlT;
            hres = SHILCombine(pidlres, _ILNext(pidl), &pidlT);
            if ( SUCCEEDED(hres) )
            {
                ILFree(pidlres);
                pidlres = pidlT;
            }
        }

        // if we have a result, free the old PIDL and return the new

        if ( SUCCEEDED(hres) )
        {
            ILFree(pidl);
            pidl = pidlres;
        }
    }

    return pidl;
}

BOOL CNetFolder::_MakeStripToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fNetObjects)
{
    LPITEMIDLIST *apidl = (LPITEMIDLIST*)*papidl;
    int iidl, cidl = *pcidl;

    for (iidl = 0; iidl < cidl; iidl++)
    {
        if ((NET_IsValidID(apidl[iidl]) != NULL) != fNetObjects)
        {
            int cpidlHomo;
            LPCITEMIDLIST *apidlHomo = (LPCITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(LPCITEMIDLIST) * cidl);
            if (!apidlHomo)
                return FALSE;

            cpidlHomo = 0;
            for (iidl = 0; iidl < cidl; iidl++)
            {
                if ((NET_IsValidID(apidl[iidl]) != NULL) == fNetObjects)
                    apidlHomo[cpidlHomo++] = apidl[iidl];
            }

            // Setup to use the stripped version of the pidl array...
            *pcidl = cpidlHomo;
            *papidl = apidlHomo;
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT CNetRootFolder::v_GetFileFolder(IShellFolder2 **psf)
{
    HRESULT hres;

    hres = SHCacheTrackingFolder((LPCITEMIDLIST)&c_idlNet, CSIDL_NETHOOD, &_psfFiles);
    *psf = _psfFiles;
    return hres;
}
