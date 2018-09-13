#include "shellprv.h"
#include "ids.h"
#include "infotip.h"
#include "datautil.h"
#include "caggunk.h"
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "shitemid.h"
#include "ole2dup.h"
#include "deskfldr.h"
#include "prop.h"
#pragma hdrstop

//
// HACKHACK: GUIDs for _IsInNameSpace hack
//

// {D6277990-4C6A-11CF-8D87-00AA0060F5BF}
const GUID CLSID_ScheduledTasks =  { 0xD6277990, 0x4C6A, 0x11CF, { 0x8D, 0x87, 0x00, 0xAA, 0x00, 0x60, 0xF5, 0xBF } };

// {7007ACC7-3202-11D1-AAD2-00805FC1270E}
const GUID CLSID_NetworkConnections = { 0x7007ACC7, 0x3202, 0x11D1, { 0xAA, 0xD2, 0x00, 0x80, 0x5F, 0xC1, 0x27, 0x0E } };

//
// stock regitems
//

// IDREGITEM as implemented in NT5 Beta 3, breaks the ctrlfldr IShellFolder of downlevel
// platforms.  The clsid is interpreted as the IDCONTROL's oName and oInfo and these offsets
// are way to big for the following buffer (cBuf).  On downlevel platform, when we are lucky,
// the offset is still in memory readable by our process, and we just do random stuff.  When
// unlucky we try to read memory which we do not have access to and crash.  The IDREGITEMEX
// struct solves this by putting padding between bOrder and the CLSID and filling these bytes
// with 0's.  When persisted, downlevel platform will interpret these 0's as oName, oInfo and
// as L'\0' at the beggining of cBuf.  A _bFlagsLegacy was also added to handle the NT5 Beta3
// persisted pidls. (stephstm, 7/15/99)

// Note: in the case where CRegFldr::_cbPadding == 0, IDREGITEMEX.rgbPadding[0] is at
//       same location as the IDREGITEM.clsid

#pragma pack(1)
typedef struct _IDREGITEMEX
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bOrder;
    BYTE    rgbPadding[16]; // at least 16 to store the clsid
} IDREGITEMEX;
typedef UNALIGNED IDREGITEMEX *LPIDREGITEMEX;
typedef const UNALIGNED IDREGITEMEX *LPCIDREGITEMEX;
#pragma pack()

extern "C" BOOL IsNameListedUnderKey(LPCTSTR pszFileName, LPCTSTR pszKey);

C_ASSERT(sizeof(IDREGITEMEX) == sizeof(IDREGITEM));

EXTERN_C const IDLREGITEM c_idlNet =
{
    {SIZEOF(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_NETWORK,
    { 0x208D2C60, 0x3AEA, 0x1069, 0xA2,0xD7,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_NetworkPlaces
    0,
} ;

EXTERN_C const IDLREGITEM c_idlDrives =
{
    {SIZEOF(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_DRIVES,
    { 0x20D04FE0, 0x3AEA, 0x1069, 0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_MyComputer
    0,
} ;

EXTERN_C const IDLREGITEM c_idlInetRoot =
{
    {SIZEOF(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_INETROOT,
    { 0x871C5380, 0x42A0, 0x1069, 0xA2,0xEA,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CSIDL_Internet
    0,
} ;

EXTERN_C const IDREGITEM c_aidlConnections[] =
{
    {SIZEOF(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_DRIVES,
    { 0x20D04FE0, 0x3AEA, 0x1069, 0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_MyComputer
    {SIZEOF(IDREGITEM), SHID_COMPUTER_REGITEM, 0,
    { 0x21EC2020, 0x3AEA, 0x1069, 0xA2,0xDD,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_ControlPanel
    {SIZEOF(IDREGITEM), SHID_CONTROLPANEL_REGITEM, 0,
    { 0x7007ACC7, 0x3202, 0x11D1, 0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E, },}, // CLSID_NetworkConnections
    { 0 },
};

typedef enum
{
    REG_ICOL_NAME = 0,
    REG_ICOL_TYPE,
    REG_ICOL_COMMENT,
} EnumRegCols;

const COL_DATA s_reg_cols[] = {
    {REG_ICOL_NAME,         IDS_NAME_COL,       20, LVCFMT_LEFT,    &SCID_NAME},
    {REG_ICOL_TYPE,         IDS_TYPE_COL,       20, LVCFMT_LEFT,    &SCID_TYPE},
    {REG_ICOL_COMMENT,      IDS_COMMENT_COL,    30, LVCFMT_LEFT,    &SCID_Comment},
};

//
// class that implements the regitems folder
//

// CLSID_RegFolder {0997898B-0713-11d2-A4AA-00C04F8EEB3E}
const GUID CLSID_RegFolder =  { 0x997898b, 0x713, 0x11d2, { 0xa4, 0xaa, 0x0, 0xc0, 0x4f, 0x8e, 0xeb, 0x3e } };

class CRegFolderEnum;     // forward

class CRegFolder : public CAggregatedUnknown,
                   public IShellFolder2,
                   public IContextMenuCB,
                   public IShellIconOverlay
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };

    //
    //  PowerDesk98 passes a CFSFolder to CRegFolder::Release(), so validate
    //  the pointer before proceeding down the path to iniquity.
    //
    STDMETHODIMP_(ULONG) Release(void) 
                { return _dwSignature == c_dwSignature ?
                            CAggregatedUnknown::Release() : 0; };

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName,
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
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags,
                           LPITEMIDLIST * ppidlOut);

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // *** IShellIconOverlay methods ***
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex);

    // *** IContextMenuCB methods ***
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, 
                     UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IRegItemFolder methods *** 
    STDMETHODIMP Initialize(REGITEMSINFO *pri);

protected:
    CRegFolder(IUnknown *punkOuter);
    ~CRegFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid,void **ppvObj);

    HRESULT _GetOverlayInfo(LPCIDLREGITEM pidlr, int *pIndex, BOOL fIconIndex);
    
    LPCITEMIDLIST _GetFolderIDList();
    LPCIDLREGITEM _AnyRegItem(UINT cidl, LPCITEMIDLIST apidl[]);
    int _ReqItemIndex(LPCIDLREGITEM pidlr);
    BYTE _GetOrder(LPCIDLREGITEM pidlr);
    LPITEMIDLIST _CreateSimpleIDList(const CLSID *pclsid, BYTE bFlags, BOOL bOrder);
    void _GetNameSpaceKey( LPCIDLREGITEM pidlr, LPTSTR pszKeyName);
    LPCIDLREGITEM _IsReg(LPCITEMIDLIST pidl);
    BOOL _IsInNameSpace(LPCIDLREGITEM pidlr);
    HDCA _ItemArray();
    HRESULT _InitFromMachine(IUnknown *punk, BOOL bEnum);
    LPITEMIDLIST _CreateIDList(const CLSID *pclsid);
    HRESULT _CreateAndInit(LPCIDLREGITEM pidlr, REFIID riid, void **ppunkOut);
    HRESULT _BindToItem(LPCIDLREGITEM pidlr, LPBC pbc, REFIID riid, void **ppv, BOOL bOneLevel);
    HRESULT _GetInfoTip(LPCIDLREGITEM pidlr, void **ppv);
    HRESULT _GetRegItemColumnFromRegistry(LPCIDLREGITEM pidlr, LPCTSTR pszColumnName, LPTSTR pszColumnData, int cchColumnData);
    void _GetClassKeys(LPCIDLREGITEM pidlr, HKEY *phkCLSID, HKEY *phkBase);
    HRESULT _GetDisplayNameFromSelf(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName);
    HRESULT _GetDisplayName(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName);
    HRESULT _DeleteRegItem(LPCIDLREGITEM pidlr);
    BOOL _GetDeleteMessage(LPCIDLREGITEM pidlr, LPTSTR pszMsg, int cchMax);

    HRESULT _ParseNextLevel(HWND hwnd, LPBC pbc, LPCIDLREGITEM pidlr,
                            LPOLESTR pwzRest, LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes);

    HRESULT _ParseGUIDName(HWND hwnd, LPBC pbc, LPOLESTR pwzDisplayName, 
                           LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes);

    HRESULT _ParseThroughItem(LPCIDLREGITEM pidlr, HWND hwnd, LPBC pbc,
                              LPOLESTR pszName, ULONG *pchEaten,
                              LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes);
    HRESULT _SetAttributes(LPCIDLREGITEM pidlr, BOOL bPerUser, DWORD dwMask, DWORD dwNewBits);
    ULONG _GetPerUserAttributes(LPCIDLREGITEM pidlr);
    ULONG _GetAttributesOf(LPCIDLREGITEM pidlr, DWORD dwAttributesNeeded);
    void _Delete(HWND hwnd, UINT uFlags, IDataObject *pdtobj);
    HRESULT _AssocCreate(LPCIDLREGITEM pidl, REFIID riid, void **ppv);
    //
    // inline methods
    //

    // Will probably not be expanded inline as _GetOrder is a behemoth of a fct
    void _FillIDList(const CLSID *pclsid, IDLREGITEM *pidlr)
    {
        pidlr->idri.cb = SIZEOF(pidlr->idri) + (WORD)_cbPadding;
        pidlr->idri.bFlags = _bFlags;
        _SetPIDLRCLSID(pidlr, pclsid);

        pidlr->idri.bOrder = _GetOrder((LPCIDLREGITEM)pidlr);

        _SetPIDLRcbNext(pidlr, 0);
    };

    BOOL _IsDesktop() { return ILIsEmpty(_GetFolderIDList()); }

    void _GetClassID(LPCITEMIDLIST pidl, CLSID *pclsid)
                { *pclsid = ((LPCIDLREGITEM)pidl)->idri.clsid; };

    int _MapToOuterColNo(int iCol);

    // CompareIDs Helpers
    int _CompareIDsOriginal(LPARAM lParam, LPCIDLREGITEM pidlr1, LPCIDLREGITEM pidlr2);
    int _CompareIDsFolderFirst(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    int _CompareIDsAlphabetical(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    BOOL _IsFolder(LPCITEMIDLIST pidl);
    HRESULT _CreateViewObjectFor(LPCIDLREGITEM pidlr, HWND hwnd, REFIID riid, void **ppv, BOOL bOneLevel);

private:
    inline UNALIGNED CLSID& _GetPIDLRCLSID(LPCIDLREGITEM pidlr);
    inline void _SetPIDLRCLSID(LPIDLREGITEM pidlr, const CLSID *pclsid);
    inline void _SetPIDLRcbNext(LPIDLREGITEM pidlr, USHORT cb);
    inline IDLREGITEM* _CreateAndFillIDLREGITEM(const CLSID *pclsid);

private:
    enum { c_dwSignature = 0x38394450 }; // "PD98" - PowerDesk 98 hack
    DWORD           _dwSignature;
    LPTSTR          _pszMachine;
    LPITEMIDLIST    _pidl;

    IShellFolder2   *_psfOuter;
    IShellIconOverlay *_psioOuter;

    IPersistFreeThreadedObject *_pftoCached;   // cached pointer of last free threaded bind

    int             _iCmp;              // compare multiplier used to revers the sort order
    LPCTSTR         _pszRegKey;
    REGITEMSPOLICY*  _pPolicy;
    TCHAR           _chRegItem;         // parsing prefix, must be TEXT(':')
    BYTE            _bFlags;            // flags field for PIDL construction
    DWORD           _dwDefAttributes;   // default attributes for items
    int             _nRequiredItems;    // # of required items
    DWORD           _dwSortAttrib;      // sorting attributes
    DWORD           _cbPadding;         // see comment in views.h      
    BYTE            _bFlagsLegacy;      // see comment in views.h

    CLSID           _clsidAttributesCache;
    ULONG           _dwAttributesCache;
    ULONG           _dwAttributesCacheValid;
    CLSID           _clsidNameCache;
    DWORD           _dwFlagsNameCache;
    TCHAR           _szNameCache[64];
    REQREGITEM      *_aReqItems;

    friend DWORD CALLBACK _RegFolderPropThreadProc(void *pv);
    friend HRESULT CRegFolder_CreateInstance(REGITEMSINFO *pri, IUnknown *punkOuter, REFIID riid, void **ppv);
    friend CRegFolderEnum;
};  

class CRegFolderEnum : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; };
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum) { return E_NOTIMPL; };

protected:
    CRegFolderEnum(CRegFolder* prf, DWORD grfFlags, IEnumIDList* pesf, HDCA dcaItems, 
        REGITEMSPOLICY* pPolicy);
    ~CRegFolderEnum();

private:
    LONG         _cRef;
    CRegFolder*  _prf;          // reg item folder
    IEnumIDList* _peidl;
    DWORD        _grfFlags;     // guy we are wrapping
    HDCA         _hdca;         // DCA of regitem objects
    REGITEMSPOLICY* _pPolicy;    // controls what items are visible

    INT          _iCur;

    friend CRegFolder;
};

//
// Construction / Destruction and aggregation
//

CRegFolder::CRegFolder(IUnknown *punkOuter) : 
    _dwSignature(c_dwSignature),
    CAggregatedUnknown(punkOuter),
    _pidl(NULL),
    _pszMachine(NULL),
    _psfOuter(NULL)
{
    DllAddRef();
}

CRegFolder::~CRegFolder()
{
    IUnknown *punkCached = (IUnknown *)InterlockedExchangePointer((void**) &_pftoCached, NULL);
    if (punkCached)
        punkCached->Release();
        
    if (_pidl)
        ILFree(_pidl);

    Str_SetPtr(&_pszMachine, NULL);

    if (_aReqItems)
        LocalFree(_aReqItems);

    SHReleaseOuterInterface(_GetOuter(), (IUnknown **)&_psfOuter);     // release _psfOuter
    SHReleaseOuterInterface(_GetOuter(), (IUnknown **)&_psioOuter);

    DllRelease();
}

HRESULT CRegFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CRegFolder, IShellFolder, IShellFolder2), // IID_IShellFolder
        QITABENT(CRegFolder, IShellFolder2),                    // IID_IShellFolder2
        QITABENT(CRegFolder, IShellIconOverlay),                // IID_IShellIconOverlay
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && IsEqualIID(CLSID_RegFolder, riid))
    {
        *ppv = this;                // not ref counted
        hr = S_OK;
    }
    return hr;
}


//
// get the pidl used to initialize this namespace
//

LPCITEMIDLIST CRegFolder::_GetFolderIDList()
{
    if (!_pidl)
        SHGetIDListFromUnk(_psfOuter, &_pidl);

    return _pidl;
}


//
// check to see if this pidl is a regitem
//

LPCIDLREGITEM CRegFolder::_IsReg(LPCITEMIDLIST pidl)
{
    LPCIDLREGITEM pidlr = (LPCIDLREGITEM)pidl;
    if (pidl && !ILIsEmpty(pidl) && pidlr->idri.bFlags == _bFlags)
    {
        return pidlr;
    }
    else
    {
        // We needed to add padding to the Control Panel regitems.  There was CP
        // regitems out there without the padding.  If there is padding and we fail
        // the above case then maybe we are dealing with one of these. (stephstm)
        if (_cbPadding && _bFlagsLegacy)
        {
            if (pidl && !ILIsEmpty(pidl) && pidlr->idri.bFlags == _bFlagsLegacy)
            {
                return pidlr;
            }
        }
    }
    return NULL;
}

//
// returns a ~REFERENCE~ to the CLSID in the pidlr.  HintHint: the ref has
// the same scope as the pidlr.  This is to replace the pidlr->idri.clsid
// usage. (stephstm)
//

UNALIGNED CLSID& CRegFolder::_GetPIDLRCLSID(LPCIDLREGITEM pidlr)
{
#ifdef DEBUG
    if (_cbPadding && (_bFlagsLegacy != pidlr->idri.bFlags))
    {
        LPIDREGITEMEX pidriex = (LPIDREGITEMEX)&(pidlr->idri);

        for (DWORD i = 0; i < _cbPadding; ++i)
        {
            ASSERT(0 == pidriex->rgbPadding[i]);
        }
    }
#endif

    return (pidlr->idri.bFlags != _bFlagsLegacy) ?
        // return the new padded clsid
        ((UNALIGNED CLSID&)((LPIDREGITEMEX)&(pidlr->idri))->rgbPadding[_cbPadding]) :
        // return the old non-padded clsid
        (pidlr->idri.clsid);
}

// This fct is called only for IDREGITEMs created within this file.  It is not
// called for existing PIDL, so we do not need to check if it is a legacy pidl.
void CRegFolder::_SetPIDLRCLSID(LPIDLREGITEM pidlr, const CLSID *pclsid)
{
    LPIDREGITEMEX pidriex = (LPIDREGITEMEX)&(pidlr->idri);

    ((UNALIGNED CLSID&)pidriex->rgbPadding[_cbPadding]) = *pclsid;

    ZeroMemory(pidriex->rgbPadding, _cbPadding);
}

void CRegFolder::_SetPIDLRcbNext(LPIDLREGITEM pidlr, USHORT cb)
{
    LPIDREGITEMEX pidriex = (LPIDREGITEMEX)&(pidlr->idri);

    // We rely on the fact that we know that IDLREGITEM is a IDREGITEMEX followed
    // by a USHORT

    // Cool casting!
    ((USHORT)*(UNALIGNED USHORT*)&(pidriex->rgbPadding[_cbPadding + sizeof(CLSID)])) = cb;
}

IDLREGITEM* CRegFolder::_CreateAndFillIDLREGITEM(const CLSID *pclsid)
{
    IDLREGITEM* pidlRegItem = (IDLREGITEM*)_ILCreate(sizeof(IDLREGITEM) + _cbPadding);

    if (pidlRegItem)
    {
        _FillIDList(pclsid, pidlRegItem);
    }

    return pidlRegItem;
}

//
// Returns: ptr to the first reg item if there are any
//

LPCIDLREGITEM CRegFolder::_AnyRegItem(UINT cidl, LPCITEMIDLIST apidl[])
{
    for (UINT i = 0; i < cidl; i++) 
    {
        LPCIDLREGITEM pidlr = _IsReg(apidl[i]);
        if (pidlr)
            return pidlr;
    }
    return NULL;
}


int CRegFolder::_ReqItemIndex(LPCIDLREGITEM pidlr)
{
    const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment

    for (int i = _nRequiredItems - 1; i >= 0; i--)
    {
        if (IsEqualGUID(clsid, *_aReqItems[i].pclsid))
        {
            break;
        }
    }
    return i;
}


//
// a sort order of 0 means there is not specified sort order for this item
//

BYTE CRegFolder::_GetOrder(LPCIDLREGITEM pidlr)
{
    BYTE bRet;
    int i = _ReqItemIndex(pidlr);
    if (i != -1)
    {
        bRet = _aReqItems[i].bOrder;
    }
    else
    {
        HKEY hkey;
        TCHAR szKey[MAX_PATH], szCLSID[GUIDSTR_MAX];

        SHStringFromGUID(_GetPIDLRCLSID(pidlr), szCLSID, ARRAYSIZE(szCLSID));
        wsprintf(szKey, TEXT("CLSID\\%s"), szCLSID);

        bRet = 128;     // default for items that do not register a SortOrderIndex

        if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hkey) == ERROR_SUCCESS)
        {
            DWORD dwOrder, cbSize = SIZEOF(dwOrder);
            if (SHQueryValueEx(hkey, TEXT("SortOrderIndex"), NULL, NULL, (BYTE *)&dwOrder, &cbSize) == ERROR_SUCCESS)
            {

                // B#221890 - PowerDesk assumes that it can do this:
                //      Desktop -> First child -> Third child
                // and it will get the C: drive.  This means that
                // My Computer must be the first regitem.  So any items
                // in front of My Computer are put immediately behind.
                if ((SHGetAppCompatFlags(ACF_MYCOMPUTERFIRST) & ACF_MYCOMPUTERFIRST) &&
                    dwOrder <= SORT_ORDER_DRIVES)
                    dwOrder = SORT_ORDER_DRIVES + 1;

                bRet = (BYTE)dwOrder;
            }
            RegCloseKey(hkey);
        }
    }
    return bRet;
}


LPITEMIDLIST CRegFolder::_CreateIDList(const CLSID *pclsid)
{
    return (LPITEMIDLIST)_CreateAndFillIDLREGITEM(pclsid);
}


LPITEMIDLIST CRegFolder::_CreateSimpleIDList(const CLSID *pclsid, BYTE bFlags, BOOL bOrder)
{
    IDLREGITEM* pidlRegItem = (IDLREGITEM*)_CreateIDList(pclsid);

    if (pidlRegItem)
    {
        pidlRegItem->idri.cb = SIZEOF(pidlRegItem->idri) + (WORD)_cbPadding;
        pidlRegItem->idri.bFlags = bFlags;
        pidlRegItem->idri.bOrder = (BYTE) bOrder;

        _SetPIDLRCLSID(pidlRegItem, pclsid);

        _SetPIDLRcbNext(pidlRegItem, 0);
    }

    return (LPITEMIDLIST)pidlRegItem;
}


//
// validate that this item exists in this name space (look in the registry)
//

BOOL CRegFolder::_IsInNameSpace(LPCIDLREGITEM pidlr)
{
    TCHAR szKeyName[MAX_PATH];

    if (_ReqItemIndex(pidlr) >= 0)
        return TRUE;

    // HACKHACK: we will return TRUE for Printers, N/W connections and Scheduled tasks
    //           since they've been moved from My Computer to Control Panel and they
    //           don't really care where they live

    UNALIGNED CLSID& rclsid = _GetPIDLRCLSID(pidlr);

    if (IsEqualGUID(CLSID_Printers, rclsid) ||
        IsEqualGUID(CLSID_NetworkConnections, rclsid) ||
        IsEqualGUID(CLSID_ScheduledTasks, rclsid))
    {
        return TRUE;
    }

    _GetNameSpaceKey(pidlr, szKeyName);

    return (SHRegQueryValue(HKEY_LOCAL_MACHINE, szKeyName, NULL, NULL) == ERROR_SUCCESS) ||
           (SHRegQueryValue(HKEY_CURRENT_USER,  szKeyName, NULL, NULL) == ERROR_SUCCESS);
}


//
// We use a HDCA to store the CLSIDs for the regitems in this folder, this call returns
// that HDCA>
//

HDCA CRegFolder::_ItemArray()
{
    HDCA hdca = DCA_Create();
    if (hdca)
    {
        int i;
        for (i = 0; i < _nRequiredItems; i++)
        {
            DCA_AddItem(hdca, *_aReqItems[i].pclsid);
        }

        DCA_AddItemsFromKey(hdca, HKEY_LOCAL_MACHINE, _pszRegKey);
        DCA_AddItemsFromKey(hdca, HKEY_CURRENT_USER,  _pszRegKey);
    }
    return hdca;
}


//
// Given our cached machine name, attempt get the object on that machine.
//

HRESULT CRegFolder::_InitFromMachine(IUnknown *punk, BOOL bEnum)
{
    if (_pszMachine)
    {
        IRemoteComputer * premc;
        HRESULT hres = punk->QueryInterface(IID_PPV_ARG(IRemoteComputer, &premc));
        if (SUCCEEDED(hres))
        {
            WCHAR wszName[MAX_PATH];
            SHTCharToUnicode(_pszMachine, wszName, ARRAYSIZE(wszName));
            hres = premc->Initialize(wszName, bEnum);
            premc->Release();
        }
        return hres;
    }
    return NOERROR;
}

#define ExchangeFTO(_ppfto, _pfto)  ((IPersistFreeThreadedObject *)InterlockedExchangePointer((void**)_ppfto, _pfto))

//
// Given a pidl, lets get an instance of the namespace that provides it.
//  - handles caching accordingly
//

HRESULT CRegFolder::_CreateAndInit(LPCIDLREGITEM pidlr, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    //  try using the cached pointer
    IPersistFreeThreadedObject *pfto = ExchangeFTO(&_pftoCached, NULL);
    if (pfto)
    {
        CLSID clsid;
        if (SUCCEEDED(pfto->GetClassID(&clsid)) && IsEqualGUID(clsid, _GetPIDLRCLSID(pidlr)))
        {
            // if this fails, ppv will still be NULL
            // so we will create a new cache item...
            hr = pfto->QueryInterface(riid, ppv);
        }
    }

    //  cache failed, cocreate it ourself
    if (NULL == *ppv)
    {
        CLSID clsid = _GetPIDLRCLSID(pidlr); // alignment

        OBJCOMPATFLAGS ocf = SHGetObjectCompatFlags(NULL, &clsid);

        if (!(OBJCOMPATF_UNBINDABLE & ocf))
        {
            //
            //  HACKHACK - some regitems can only be CoCreated with IID_IShellFolder
            //  specifically the hummingbird shellext will DebugBreak() bringing
            //  down the shell...  but we can CoCreate() and then QI after...
            //
            
            hr = SHExtCoCreateInstance(NULL, &clsid, NULL, 
                (OBJCOMPATF_COCREATESHELLFOLDERONLY & ocf) ? IID_IShellFolder : riid , ppv);

            if (SUCCEEDED(hr))
            {
                IUnknown *punk = (IUnknown *)*ppv;  // avoid casts below

                if ((OBJCOMPATF_COCREATESHELLFOLDERONLY & ocf))
                {
                    hr = punk->QueryInterface(riid, ppv);
                    punk->Release();
                    punk = (IUnknown *)*ppv;  // avoid casts below
                }

                if (SUCCEEDED(hr))
                {
                    hr = _InitFromMachine(punk, FALSE);
                    if (SUCCEEDED(hr))
                    {
                        IPersistFolder *ppf;
                        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf))))
                        {
                            LPITEMIDLIST pidlAbs = ILCombine(_GetFolderIDList(), (LPCITEMIDLIST)pidlr);
                            if (pidlAbs)
                            {
                                hr = ppf->Initialize(pidlAbs);
                                ILFree(pidlAbs);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            ppf->Release();
                        }

                        if (SUCCEEDED(hr))
                        {
                            if (pfto)
                                pfto->Release();    //  we are going to replace the cache
                            if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFreeThreadedObject, &pfto))))
                            {
                                SHPinDllOfCLSID(&clsid);
                            }
                        }
                    }
                }
            }
        }
    }

    //  recache the pfto
    if (pfto)
    {
        pfto = ExchangeFTO(&_pftoCached, pfto);
        if (pfto)
            pfto->Release();    //  protect against race condition or re-entrancy
    }

    return hr;
}


//
// lets the reg item itself pick off the GetDisplayNameOf() impl for itself. this lets
// MyDocs on the desktop return c:\win\profile\name\My Documents as it's parsing name
//
// returns:
//      S_FALSE     do normal parsing, reg item did not handel
//
//

HRESULT CRegFolder::_GetDisplayNameFromSelf(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName)
{
    HRESULT hres = S_FALSE;     // normal case
    const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment

    if (((dwFlags & (SHGDN_FORADDRESSBAR | SHGDN_INFOLDER | SHGDN_FORPARSING)) == SHGDN_FORPARSING) &&
        SHQueryShellFolderValue(&clsid, TEXT("WantsFORPARSING")))
    {
        IShellFolder *psf;
        if (SUCCEEDED(_BindToItem(pidlr, NULL, IID_PPV_ARG(IShellFolder, &psf), TRUE)))
        {
            STRRET str;
            // pass NULL pidl (c_idlDesktop) to get display name of the folder itself
            hres = psf->GetDisplayNameOf(&c_idlDesktop, dwFlags, &str);
            if (SUCCEEDED(hres))
                hres = StrRetToBuf(&str, &c_idlDesktop, pszName, cchName);
            psf->Release();
        }
    }
    return hres;
}


//
// Given a pidl in the regitms folder, get the friendly name for this (trying the user
// store ones, then the global one).
//

#define GUIDSIZE    50

HRESULT CRegFolder::_GetDisplayName(LPCIDLREGITEM pidlr, DWORD dwFlags, LPTSTR pszName, UINT cchName)
{
    HKEY hkCLSID;

    *pszName = 0;

    if (IsEqualGUID(_GetPIDLRCLSID(pidlr), _clsidNameCache) && 
        (_dwFlagsNameCache == dwFlags))
    {
        StrCpyN(pszName, _szNameCache, cchName);
    }
    else
    {
        _clsidNameCache = CLSID_NULL;

        if (dwFlags & SHGDN_FORPARSING)
        {
            HRESULT hres = _GetDisplayNameFromSelf(pidlr, dwFlags, pszName, cchName);
            if (hres != S_FALSE)
                return hres;

            if (!(dwFlags & SHGDN_FORADDRESSBAR))
            {
                // Get the parent folder name
                TCHAR szParentName[MAX_PATH];
                szParentName[0] = 0;
                if (!(dwFlags & SHGDN_INFOLDER) && !ILIsEmpty(_GetFolderIDList()))
                {
                    SHGetNameAndFlags(_GetFolderIDList(), SHGDN_FORPARSING, szParentName, SIZECHARS(szParentName), NULL);
                    StrCatBuff(szParentName, TEXT("\\"), ARRAYSIZE(szParentName));                
                }

                // Win95 didn't support SHGDN_FORPARSING on regitems; it always
                // returned the display name.  Norton Unerase relies on this,
                // because it assumes that if the second character of the FORPARSING
                // name is a colon, then it's a drive.  Therefore, we can't return
                // ::{guid} or Norton will fault.  (I guess they didn't believe in
                // SFGAO_FILESYSTEM.)  So if we are Norton Unerase, then ignore
                // the FORPARSING flag; always get the name for display.
                // And good luck to any user who sets his computer name to something
                // with a colon as the second character...

                if (SHGetAppCompatFlags(ACF_OLDREGITEMGDN) & ACF_OLDREGITEMGDN)
                {

                    // In Application compatibility mode turn SHGDN_FORPARSING
                    // off and fall thru to the remainder of the function which
                    // avoids the ::{GUID} when required.
                    
                    dwFlags &= ~SHGDN_FORPARSING;
                }
                else
                {
                    // Get this reg folder name
                    TCHAR szFolderName[GUIDSIZE + 2];
                    szFolderName[0] = szFolderName[1] = _chRegItem;
                    SHStringFromGUID(_GetPIDLRCLSID(pidlr), szFolderName + 2, cchName - 2);

                    // Copy the full path into szParentName.
                    StrCatBuff(szParentName, szFolderName, ARRAYSIZE(szParentName));

                    // Copy the full path into the output buffer.
                    lstrcpyn(pszName, szParentName, cchName);
                    return S_OK;
                }
            }
        }

        // Check per-user settings first...
        if ((*pszName == 0) && SUCCEEDED(SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, TRUE, FALSE, &hkCLSID)))
        {
            LONG lLen = cchName * SIZEOF(TCHAR);
            SHRegQueryValue(hkCLSID, NULL, pszName, &lLen);
            RegCloseKey(hkCLSID);
        }

        // If we have to, use per-machine settings...
        if (*pszName == 0)
        {
            _GetClassKeys(pidlr, &hkCLSID, NULL);

            if (hkCLSID)
            {
                LONG lLen = cchName * SIZEOF(TCHAR);
                // Check per-language string first.
                SHLoadRegUIString(hkCLSID, TEXT("LocalizedString"), pszName, cchName);
                if (*pszName == 0)
                {
                    // If not found "LocalizedString" read the default string.
                    SHLoadRegUIString(hkCLSID, TEXT(""), pszName, cchName);
                }
                RegCloseKey(hkCLSID);
            }
        }

        // try the required item names, they might not be in the registry
        if (*pszName == 0)
        {
            int iItem = _ReqItemIndex(pidlr);
            if (iItem >= 0)
                LoadString(HINST_THISDLL, _aReqItems[iItem].uNameID, pszName, cchName);
        }

        if (*pszName)
        {
            if (_pszMachine && !(dwFlags & SHGDN_INFOLDER))
            {
                // szName now holds the item name, and _pszMachine holds the machine.
                LPTSTR pszRet = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), 
                                                                SkipServerSlashes(_pszMachine), pszName);
                if (pszRet)
                {
                    lstrcpyn(pszName, pszRet, cchName);
                    LocalFree(pszRet);
                }
            }

            if (lstrlen(pszName) < ARRAYSIZE(_szNameCache))
            {
                lstrcpy(_szNameCache, pszName);
                _clsidNameCache = _GetPIDLRCLSID(pidlr);
                _dwFlagsNameCache = dwFlags;
            }
        }
    }
    return *pszName ? S_OK : E_FAIL;
}


//
// get the HKEYs that map to the regitm.
//
// NOTE: this function returns a void so that the caller explicitly must check the keys
//       to see if they are non-null before using them.
//
void CRegFolder::_GetClassKeys(LPCIDLREGITEM pidlr, HKEY* phkCLSID, HKEY* phkBase)
{
    HRESULT hr;
    IQueryAssociations *pqa;
    
    if (phkCLSID)
        *phkCLSID = NULL;
    
    if (phkBase)
        *phkBase = NULL;

    hr = _AssocCreate(pidlr, IID_IQueryAssociations, (void **)&pqa);

    if (SUCCEEDED(hr))
    {
        if (phkCLSID)
        {
            hr = pqa->GetKey(0, ASSOCKEY_CLASS, NULL, phkCLSID);

            ASSERT((SUCCEEDED(hr) && *phkCLSID) || (FAILED(hr) && (*phkCLSID == NULL)));
        }

        if (phkBase)
        {
            hr = pqa->GetKey(0, ASSOCKEY_BASECLASS, NULL, phkBase);

            ASSERT((SUCCEEDED(hr) && *phkBase) || (FAILED(hr) && (*phkBase == NULL)));
        }

        pqa->Release();
    }
}

// {9EAC43C0-53EC-11CE-8230-CA8A32CF5494}
static const GUID GUID_WINAMP = { 0x9eac43c0, 0x53ec, 0x11ce, { 0x82, 0x30, 0xca, 0x8a, 0x32, 0xcf, 0x54, 0x94 } };

#define SZ_BROKEN_WINAMP_VERB   TEXT("OpenFileOrPlayList")

void _MaybeDoWinAmpHack(UNALIGNED REFGUID rguid)
{
    if (IsEqualGUID(rguid, GUID_WINAMP))
    {
        // WinAmp writes in "OpenFileOrPlayList" as default value under shell, but they
        // don't write a corresponding "OpenFileorPlayList" verb key.  So we need to whack
        // the registry into shape for them.  Otherwise, they won't get the default verb
        // they want (due to an NT5 change in CDefExt_QueryContextMenu's behavior).

        TCHAR szCLSID[GUIDSTR_MAX];
        SHStringFromGUID(rguid, szCLSID, ARRAYSIZE(szCLSID));

        TCHAR szRegKey[GUIDSTR_MAX + 40];
        wsprintf(szRegKey, TEXT("CLSID\\%s\\shell"), szCLSID);

        TCHAR szValue[ARRAYSIZE(SZ_BROKEN_WINAMP_VERB)+2];
        DWORD dwType;
        DWORD dwSize = SIZEOF(szValue);
        if (SHGetValue(HKEY_CLASSES_ROOT, szRegKey, NULL, &dwType, szValue, &dwSize) == 0)
        {
            if (dwType == REG_SZ && lstrcmp(szValue, SZ_BROKEN_WINAMP_VERB) == 0)
            {
                // Make "open" the default verb
                SHSetValue(HKEY_CLASSES_ROOT, szRegKey, NULL, REG_SZ, TEXT("open"), SIZEOF(TEXT("open")));
            }
        }
    }
}

HRESULT CRegFolder::_AssocCreate(LPCIDLREGITEM pidlr, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (void **)&pqa);
    if (SUCCEEDED(hr))
    {
        const CLSID clsid = pidlr->idri.clsid;    // alignment
        WCHAR szCLSID[GUIDSTR_MAX];
        ASSOCF flags = ASSOCF_INIT_NOREMAPCLSID;

         if ((_GetAttributesOf(pidlr, SFGAO_FOLDER) & SFGAO_FOLDER)
         && (!SHQueryShellFolderValue(&clsid, TEXT("HideFolderVerbs"))))
            flags |= ASSOCF_INIT_DEFAULTTOFOLDER;

        SHStringFromGUIDW(_GetPIDLRCLSID(pidlr), szCLSID, ARRAYSIZE(szCLSID));

        _MaybeDoWinAmpHack(_GetPIDLRCLSID(pidlr));

        hr = pqa->Init(flags, szCLSID, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = pqa->QueryInterface(riid, ppv);

        pqa->Release();
    }

    return hr;
}

//
// get the namespace key for this objec.
//

void CRegFolder::_GetNameSpaceKey(LPCIDLREGITEM pidlr, LPTSTR pszKeyName)
{
    TCHAR szClass[GUIDSTR_MAX];
    SHStringFromGUID(_GetPIDLRCLSID(pidlr), szClass, ARRAYSIZE(szClass));
    wsprintf(pszKeyName, TEXT("%s\\%s"), _pszRegKey, szClass);
}


//
// the user is trying to delete an object from a regitem folder, therefore
// lets look in the IDataObject to see if that includes any regitems, if
// so then handle their deletion, before passing to the outer guy to 
// handle the other objects.
//

#define MAX_REGITEM_WARNTEXT 1024

void CRegFolder::_Delete(HWND hwnd, UINT uFlags, IDataObject *pdtobj)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    if (pida)
    {
        TCHAR szItemWarning[MAX_REGITEM_WARNTEXT];
        UINT i;
        UINT nregfirst = (UINT)-1;
        UINT creg = 0;
        UINT cwarn = 0;
        UINT countfs = 0;
        LPCITEMIDLIST *ppidlFS = NULL;

        //
        // calc number of regitems and index of first
        //
        for (i = 0; i < pida->cidl; i++)
        {
            LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, i);
            LPCIDLREGITEM pidlr = _IsReg(pidl);
            if (pidlr)
            {
                if (_GetAttributesOf(pidlr, SFGAO_CANDELETE) & SFGAO_CANDELETE)
                {
                    creg++;
                    if (nregfirst == (UINT)-1)
                        nregfirst = i;

                    if ((cwarn < 2) && _GetDeleteMessage(pidlr, szItemWarning, ARRAYSIZE(szItemWarning)))
                    {
                        cwarn++;
                    }
                }
            }
            else
            {
                // alloc an alternate array for FS pidls 
                // for simplicitu over alloc in the case where there are reg items
                if (ppidlFS == NULL)
                    ppidlFS = (LPCITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * SIZEOF(LPCITEMIDLIST));
                if (ppidlFS)
                {
                    ppidlFS[countfs++] = pidl;
                }
            }
        }

        //
        // compose the confirmation message / ask the user / fry the items...
        //
        if (creg)
        {
            SHELLSTATE ss = {0};

            SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, FALSE);

            if ((uFlags & CMIC_MASK_FLAG_NO_UI) || ss.fNoConfirmRecycle)
            {
                for (i = 0; i < pida->cidl; i++)
                {
                    LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, i));
                    if (pidlr && (_GetAttributesOf(pidlr, SFGAO_CANDELETE) & SFGAO_CANDELETE))
                    {
                        _DeleteRegItem(pidlr);
                    }
                }
            }
            else
            {
                TCHAR szItemName[MAX_PATH];
                TCHAR szWarnText[1024 + MAX_REGITEM_WARNTEXT];
                TCHAR szWarnCaption[128];
                TCHAR szTemp[256];
                MSGBOXPARAMS mbp = {SIZEOF(MSGBOXPARAMS), hwnd,
                    HINST_THISDLL, szWarnText, szWarnCaption,
                    MB_YESNO | MB_USERICON, MAKEINTRESOURCE(IDI_NUKEFILE),
                    0, NULL, 0};

                //
                // so we can tell if we got these later
                //
                *szItemName = 0;
                *szWarnText = 0;

                //
                // if there is only one, mention it by name
                //
                if (creg == 1)
                {
                    TCHAR szTemp[256];
                    LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, nregfirst));

                    if (SUCCEEDED(_GetDisplayName(pidlr, SHGDN_NORMAL, szItemName, ARRAYSIZE(szItemName))))
                    {
                        int idString = (creg == pida->cidl) ?
                            IDS_CANTRECYCLEREGITEMS_NAME :
                            IDS_CANTRECYCLEREGITEMS_INCL_NAME;

                        LoadString(HINST_THISDLL, idString, szTemp, ARRAYSIZE(szTemp));
                        wsprintf(szWarnText, szTemp, szItemName);
                    }
                }

                //
                // otherwise, say "these items..." or "some of these items..."
                //
                if (!*szWarnText)
                {
                    int idString = (creg == pida->cidl) ? IDS_CANTRECYCLEREGITEMS_ALL : IDS_CANTRECYCLEREGITEMS_SOME;
                    LoadString(HINST_THISDLL, idString, szWarnText, ARRAYSIZE(szWarnText));

                    //
                    // we just loaded a very vague message
                    // don't confuse the user any more by adding random text
                    // if these is a special warning, force it to show separately
                    //
                    if (cwarn == 1)
                        cwarn++;
                }
                lstrcat(szWarnText, TEXT("\r\n\n"));

                //
                // if there is exactly one special warning message, add it in
                //
                if (cwarn == 1)
                {
                    lstrcat(szWarnText, szItemWarning);
                    lstrcat(szWarnText, TEXT("\r\n\n"));
                }

                //
                // add the question "are you sure..."
                //
                if ((pida->cidl == 1) && *szItemName)
                {
                    TCHAR szTemp2[256];
                    LoadString(HINST_THISDLL, IDS_CONFIRMDELETEDESKTOPREGITEM, szTemp2, ARRAYSIZE(szTemp2));
                    wsprintf(szTemp, szTemp2, szItemName);
                }
                else
                {
                    LoadString(HINST_THISDLL, IDS_CONFIRMDELETEDESKTOPREGITEMS, szTemp, ARRAYSIZE(szTemp));
                }
                lstrcat(szWarnText, szTemp);

                //
                // finally, the message box caption (also needed in loop below)
                //
                LoadString(HINST_THISDLL, IDS_CONFIRMDELETE_CAPTION, szWarnCaption, ARRAYSIZE(szWarnCaption));

                // make sure the user is cool with it
                if (MessageBoxIndirect(&mbp) == IDYES)
                {
                    // go ahead and delete the reg items
                    for (i = 0; i < pida->cidl; i++)
                    {
                        LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, i));
                        if (pidlr && (_GetAttributesOf(pidlr, SFGAO_CANDELETE) & SFGAO_CANDELETE))
                        {
                            if ((cwarn > 1) && _GetDeleteMessage(pidlr, szItemWarning, ARRAYSIZE(szItemWarning)))
                            {
                                if (FAILED(_GetDisplayName(pidlr, SHGDN_NORMAL, szItemName, ARRAYSIZE(szItemName))))
                                    lstrcpy(szItemName, szWarnCaption);

                                MessageBox(hwnd, szItemWarning, szItemName, MB_OK | MB_ICONINFORMATION);
                            }
                            _DeleteRegItem(pidlr);
                        }
                    }
                }
            }
        }

        // now delete the fs objects
        if (ppidlFS)
        {
            InvokeVerbOnItems(hwnd, c_szDelete, uFlags, (IShellFolder *)this, countfs, ppidlFS);
            LocalFree((HANDLE)ppidlFS);
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }
}


//
// Delete a regitem given its pidl.
//

HRESULT CRegFolder::_DeleteRegItem(LPCIDLREGITEM pidlr)
{
    HRESULT hres = E_ACCESSDENIED;

    if (_GetAttributesOf(pidlr, SFGAO_CANDELETE) & SFGAO_CANDELETE)
    {
        const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment

        if (SHQueryShellFolderValue(&clsid, TEXT("HideAsDeletePerUser")))
        {
            // clear the non enuerated bit to hide this item
            hres = _SetAttributes(pidlr, TRUE, SFGAO_NONENUMERATED, SFGAO_NONENUMERATED);
        }
        else if (SHQueryShellFolderValue(&clsid, TEXT("HideAsDelete")))
        {
            // clear the non enuerated bit to hide this item
            hres = _SetAttributes(pidlr, FALSE, SFGAO_NONENUMERATED, SFGAO_NONENUMERATED);
        }
        else
        {
            // remove from the key to delete it
            TCHAR szKeyName[MAX_PATH];

            _GetNameSpaceKey(pidlr, szKeyName);

            if ((RegDeleteKey(HKEY_CURRENT_USER,  szKeyName) == ERROR_SUCCESS) ||
                (RegDeleteKey(HKEY_LOCAL_MACHINE, szKeyName) == ERROR_SUCCESS))
            {
                hres = S_OK;
            }
        }

        if (SUCCEEDED(hres))
        {
            // tell the world
            LPITEMIDLIST pidlAbs = ILCombine(_GetFolderIDList(), (LPCITEMIDLIST)pidlr);
            if (pidlAbs)
            {
                SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlAbs, NULL);
                ILFree(pidlAbs);
            }
        }
    }
    return hres;
}


// 
// Get the prompt to be displayed if the user tries to delete the regitem,
// this is stored both globally (HKLM) and as s user configured preference.
//

BOOL CRegFolder::_GetDeleteMessage(LPCIDLREGITEM pidlr, LPTSTR pszMsg, int cchMax)
{
    HKEY hk;
    TCHAR szKeyName[MAX_PATH];

    *pszMsg = 0;

    _GetNameSpaceKey(pidlr, szKeyName);
    if ((RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hk) == ERROR_SUCCESS) ||
        (RegOpenKey(HKEY_CURRENT_USER,  szKeyName, &hk) == ERROR_SUCCESS))
    {
        SHLoadRegUIString(hk, REGSTR_VAL_REGITEMDELETEMESSAGE, pszMsg, cchMax);
        RegCloseKey(hk);
    }
    return *pszMsg != 0;
}


HRESULT CRegFolder::_GetRegItemColumnFromRegistry(LPCIDLREGITEM pidlr, LPCTSTR pszColumnName, LPTSTR pszColumnData, int cchColumnData)
{
    HKEY hkCLSID;
    HRESULT hres = E_FAIL;
    
    _GetClassKeys(pidlr, &hkCLSID, NULL);

    *pszColumnData = 0; // Default string

    if (hkCLSID)
    {
        DWORD dwLen = cchColumnData * SIZEOF(*pszColumnData);

        if (SHQueryValueEx(hkCLSID, pszColumnName, NULL, NULL, (BYTE *)pszColumnData, &dwLen) == ERROR_SUCCESS)
        {
            hres = S_OK;
        }
    
		// FIXED kenwic 052699 #342955
		RegCloseKey(hkCLSID);
	}
    return hres;
}

HRESULT CRegFolder::_CreateViewObjectFor(LPCIDLREGITEM pidlr, HWND hwnd, REFIID riid, void **ppv, BOOL bOneLevel)
{
    IShellFolder *psf;
    HRESULT hr = _BindToItem(pidlr, NULL, IID_PPV_ARG(IShellFolder, &psf), bOneLevel);
    if (SUCCEEDED(hr))
    {
        hr = psf->CreateViewObject(hwnd, riid, ppv);
        psf->Release();
    }
    else
        *ppv = NULL;
    return hr;
}

// Geta an infotip object for the namespace

HRESULT CRegFolder::_GetInfoTip(LPCIDLREGITEM pidlr, void **ppv)
{
    HKEY hkCLSID;
    HRESULT hr = E_FAIL;
    
    _GetClassKeys(pidlr, &hkCLSID, NULL);

    if (hkCLSID)
    {
        DWORD dwQuery, lLen = SIZEOF(dwQuery);

        // let the regitem code compute the info tip if it wants to...
        if (SHQueryValueEx(hkCLSID, TEXT("QueryForInfoTip"), NULL, NULL, (BYTE *)&dwQuery, &lLen) == ERROR_SUCCESS)
        {
            hr = _CreateViewObjectFor(pidlr, NULL, IID_IQueryInfo, ppv, TRUE);
        }
        else
        {
            hr = E_FAIL;
        }

        // fall back to reading it from the registry
        if (FAILED(hr))
        {
            TCHAR szText[INFOTIPSIZE];
            DWORD lLen = SIZEOF(szText);

            if ((SHQueryValueEx(hkCLSID, TEXT("InfoTip"), NULL, NULL, (BYTE *)szText, &lLen) == ERROR_SUCCESS) &&
                szText[0])
            {
                hr = CreateInfoTipFromText(szText, IID_IQueryInfo, ppv); //The InfoTip COM object
            }
        }

        RegCloseKey(hkCLSID);
    }

    return hr;
}


//-----------------------------------------------------------------------------
// IShellFolder methods
//-----------------------------------------------------------------------------

//
// Parse a regitem name, eg ::{GUID} and pass the remainder of the name to 
// the outer namespace.
//

HRESULT CRegFolder::_ParseGUIDName(HWND hwnd, LPBC pbc, LPOLESTR pwzDisplayName, 
                                   LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    TCHAR szDisplayName[GUIDSTR_MAX+10];
    HRESULT hres;
    CLSID clsid;
    LPOLESTR pwzNext;

    // Note that we add 2 to skip the RegItem identifier characters
    pwzDisplayName += 2;
    for (pwzNext = pwzDisplayName; *pwzNext && *pwzNext != TEXT('\\'); pwzNext++)
    {
        // Skip to a '\\'
    }

    OleStrToStrN(szDisplayName, ARRAYSIZE(szDisplayName), pwzDisplayName, (int)(pwzNext - pwzDisplayName));

    // szDisplayName is NOT NULL terminated, but 
    // SHCLSIDFromString doesn't seem to mind.
    hres = SHCLSIDFromString(szDisplayName, &clsid);
    if (SUCCEEDED(hres))
    {
        IDLREGITEM* pidlRegItem = _CreateAndFillIDLREGITEM(&clsid);
        
        if (pidlRegItem)
        {
            if (_IsInNameSpace(pidlRegItem) || (BindCtx_GetMode(pbc, 0) & STGM_CREATE))
            {
                hres = _ParseNextLevel(hwnd, pbc, pidlRegItem, pwzNext, ppidlOut, pdwAttributes);
            }
            else
                hres = E_INVALIDARG;

            ILFree((LPITEMIDLIST)pidlRegItem);
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
    }
    return hres;
}

//
// ask a (known) regitem to parse a displayname
//

HRESULT CRegFolder::_ParseThroughItem(LPCIDLREGITEM pidlr, HWND hwnd, LPBC pbc,
                                      LPOLESTR pszName, ULONG *pchEaten,
                                      LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    IShellFolder *psfItem;
    HRESULT hres = _BindToItem(pidlr, pbc, IID_PPV_ARG(IShellFolder, &psfItem), FALSE);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidlRight;
        hres = psfItem->ParseDisplayName(hwnd, pbc, pszName, pchEaten,
                                         &pidlRight, pdwAttributes);
        if (SUCCEEDED(hres))
        {
            hres = SHILCombine((LPCITEMIDLIST)pidlr, pidlRight, ppidlOut);
            ILFree(pidlRight);
        }
        psfItem->Release();
    }
    return hres;
}

//
// Parse through the GUID to the namespace below
//

HRESULT CRegFolder::_ParseNextLevel(HWND hwnd, LPBC pbc, LPCIDLREGITEM pidlr,
                                    LPOLESTR pwzRest, LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    if (!*pwzRest)
    {
        // base case for recursive calls
        // pidlNext should be a simple pidl.
        ASSERT(!ILIsEmpty((LPCITEMIDLIST)pidlr) && ILIsEmpty(_ILNext((LPCITEMIDLIST)pidlr)));
        if (pdwAttributes && *pdwAttributes)
            *pdwAttributes = _GetAttributesOf(pidlr, *pdwAttributes);
        return SHILClone((LPCITEMIDLIST)pidlr, ppidlOut);
    }

    ASSERT(*pwzRest == TEXT('\\'));

    ++pwzRest;

    IShellFolder *psfNext;
    HRESULT hres = _BindToItem(pidlr, pbc, IID_PPV_ARG(IShellFolder, &psfNext), FALSE);
    if (SUCCEEDED(hres))
    {
        ULONG chEaten;
        LPITEMIDLIST pidlRest;
        hres = psfNext->ParseDisplayName(hwnd, pbc, pwzRest, &chEaten, &pidlRest, pdwAttributes);
        if (SUCCEEDED(hres))
        {
            hres = SHILCombine((LPCITEMIDLIST)pidlr, pidlRest, ppidlOut);
            SHFree(pidlRest);
        }
        psfNext->Release();
    }
    return hres;
}

BOOL _FailForceReturn(HRESULT hres)
{
    switch (hres)
    {
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME):
    case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
    case HRESULT_FROM_WIN32(ERROR_CANCELLED):
        return TRUE;
    }
    return FALSE;
}


HRESULT CRegFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, 
                                     ULONG *pchEaten, LPITEMIDLIST *ppidlOut, ULONG *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;

    *ppidlOut = NULL;

    if ( pszName )
    {
        // ::{guid} lets you get the pidl for a reg item

        if (*pszName && pszName[0] == _chRegItem &&
            pszName[1] == _chRegItem)
        {
            hr = _ParseGUIDName(hwnd, pbc, pszName, ppidlOut, pdwAttributes);
        }
        else
        {
            // let our inner folder give it a try

            hr = _psfOuter->ParseDisplayName(hwnd, pbc, pszName, pchEaten, ppidlOut, pdwAttributes);
            if (SUCCEEDED(hr) || 
                _FailForceReturn(hr) ||
                SHSkipJunctionBinding(pbc, NULL))
                return hr;

            // loop over all of the items
            HDCA hdca = _ItemArray();
            if (hdca)
            {
                for (int i = 0; i < DCA_GetItemCount(hdca); i++)
                {
                    IDLREGITEM* pidlRegItem = _CreateAndFillIDLREGITEM(DCA_GetItem(hdca, i));

                    if (pidlRegItem)
                    {
                        if (_GetAttributesOf(pidlRegItem, SFGAO_BROWSABLE) & SFGAO_BROWSABLE)
                        {
                            hr = _ParseThroughItem(pidlRegItem, hwnd, pbc, pszName, 
                                                     pchEaten, ppidlOut, pdwAttributes);
                            if (SUCCEEDED(hr))
                                break;
                        }

                        ILFree((LPITEMIDLIST)pidlRegItem);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                DCA_Destroy(hdca);
            }
        }
    }

    if (FAILED(hr))
        TraceMsg(TF_WARNING, "CRegFolder::ParseDisplayName(), hr:%x %hs", hr, pszName);

    return hr;
}

//-----------------------------------------------------------------------------

HRESULT CRegFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    IEnumIDList *penumOuter;

    HRESULT hr = _psfOuter->EnumObjects(hwnd, grfFlags, &penumOuter);
    if (SUCCEEDED(hr))
    {
        CRegFolderEnum *preidl = new CRegFolderEnum(this, grfFlags, penumOuter, 
            _ItemArray(), _pPolicy);
        if (preidl)
        {
            *ppenum = SAFECAST(preidl, IEnumIDList*);
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;

        if (penumOuter)
            penumOuter->Release();       // _psfOuter returned S_FALSE
    }
    return hr;
}

//-----------------------------------------------------------------------------

//
// Handle binding to the inner namespace, or the regitem accordingly given its pidl.
//

HRESULT CRegFolder::_BindToItem(LPCIDLREGITEM pidlr, LPBC pbc, REFIID riid, void **ppv, BOOL bOneLevel)
{
    LPITEMIDLIST pidlAlloc;

    *ppv = NULL;

    LPCITEMIDLIST pidlNext = _ILNext((LPCITEMIDLIST)pidlr);
    if (ILIsEmpty(pidlNext))
    {
        pidlAlloc = NULL;
        bOneLevel = TRUE;   // we know for sure it is one level
    }
    else
    {
        pidlAlloc = ILCloneFirst((LPCITEMIDLIST)pidlr);
        if (!pidlAlloc)
            return E_OUTOFMEMORY;

        pidlr = (LPCIDLREGITEM)pidlAlloc;   // a single item IDLIST
    }

    HRESULT hr;
    if (bOneLevel)
    {
        hr = _CreateAndInit(pidlr, riid, ppv);    // create on riid to avoid loads on interfaces not supported
    }
    else
    {
        IShellFolder *psfNext;
        hr = _CreateAndInit(pidlr, IID_PPV_ARG(IShellFolder, &psfNext));
        if (SUCCEEDED(hr))
        {
            hr = psfNext->BindToObject(pidlNext, pbc, riid, ppv);
            psfNext->Release();
        }
    }

    if (pidlAlloc)
        ILFree(pidlAlloc);      // we allocated in this case

    return hr;
}

HRESULT CRegFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hres;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
        hres = _BindToItem(pidlr, pbc, riid, ppv, FALSE);
    else
        hres = _psfOuter->BindToObject(pidl, pbc, riid, ppv);
    return hres;
}

HRESULT CRegFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hres;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        *ppv = NULL;
        hres = E_NOINTERFACE;
    }
    else
        hres = _psfOuter->BindToObject(pidl, pbc, riid, ppv);
    return hres;
}

// I can't believe there is no "^^"
#define LOGICALXOR(a, b) (((a) && !(b)) || (!(a) && (b)))

BOOL CRegFolder::_IsFolder(LPCITEMIDLIST pidl)
{
    BOOL fRet = FALSE;

    if (pidl)
    {
        ULONG uAttrib = SFGAO_FOLDER;
            
        HRESULT hres = GetAttributesOf(1, &pidl, &uAttrib);

        if (SUCCEEDED(hres) && (SFGAO_FOLDER & uAttrib))
            fRet = TRUE;            
    }

    return fRet;
}

// requires simple pidls
int CRegFolder::_CompareIDsOriginal(LPARAM lParam, LPCIDLREGITEM pidlr1, LPCIDLREGITEM pidlr2)
{
    int iRes;

    // Put all RegItem's first if _iCmp==1, last if -1

    if (!LOGICALXOR(pidlr1, pidlr2))
    {
        // Both are RegItem's

        // All of the required items come first, in reverse
        // order (to make this simpler)
        int iItem1 = _ReqItemIndex(pidlr1);
        int iItem2 = _ReqItemIndex(pidlr2);

        if (iItem1 == -1 && iItem2 == -1)
        {
            TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

            _GetDisplayName(pidlr1, SHGDN_NORMAL, szName1, ARRAYSIZE(szName1));
            _GetDisplayName(pidlr2, SHGDN_NORMAL, szName2, ARRAYSIZE(szName2));

            iRes = lstrcmp(szName1, szName2);
        }
        else
        {
            iRes = iItem2 - iItem1;
        }

        //
        // Use the sorting key only if they are different.
        //
        if (iRes) 
        {
            BYTE    bOrder1, bOrder2;

            // If the bOrder values are less than 0x40, then they are the old values;
            // Therefore compute the new bOrder values for these cases.
            bOrder1 = (pidlr1->idri.bOrder <= 0x40) ? _GetOrder(pidlr1) : pidlr1->idri.bOrder;
            bOrder2 = (pidlr2->idri.bOrder <= 0x40) ? _GetOrder(pidlr2) : pidlr2->idri.bOrder;
            // Do they both have a bOrder member?
            if (!LOGICALXOR(bOrder1, bOrder2))
            {
                // Yes, either they both have or none of them has one

                // If they both have one use them
                if (bOrder1 && bOrder2)
                    iRes = bOrder1 - bOrder2;
            }
            else
            {
                // No, only one of them has it
                iRes = bOrder1 ? -1 : 1;
            }
        }

//review: to fit old behavior, we don't care about _iCmp
        iRes *= _iCmp;
    }
    else
    {
        // No, only one of them is a regitem
        iRes = (pidlr1 ? -1 : 1);
    }

    return iRes;
}

// Alphabetical (doesn't care about folder, regitems, ...)
int CRegFolder::_CompareIDsAlphabetical(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRes = 0;

    // Do we have only one ptr?
    if (!LOGICALXOR(pidl1, pidl2))
    {
        // No,  either we have two or none.
        if (pidl1 && pidl2)
        {
            LPITEMIDLIST pidlFirst1 = ILCloneFirst(pidl1);
            LPITEMIDLIST pidlFirst2 = ILCloneFirst(pidl2);
            if (pidlFirst1 && pidlFirst2)
            {
                HRESULT hres = E_FAIL;
                STRRET strret;
                IShellFolder2* psf2;
                TCHAR szName1[MAX_PATH];
                TCHAR szName2[MAX_PATH];
                if (SUCCEEDED(QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
                {
                    SHELLDETAILS sd;
                    hres = psf2->GetDetailsOf(pidlFirst1, (UINT)lParam, &sd);
                    if (SUCCEEDED(hres))
                    {
                        StrRetToBuf(&sd.str, &c_idlDesktop, szName1, ARRAYSIZE(szName1));

                        hres = psf2->GetDetailsOf(pidlFirst2, (UINT)lParam, &sd);
                        if (SUCCEEDED(hres))
                        {
                            StrRetToBuf(&sd.str, &c_idlDesktop, szName2, ARRAYSIZE(szName2));
                            iRes = lstrcmp(szName1, szName2);
                        }
                    }
                    psf2->Release();
                }

                // If the handler does support IShellFolder2, or something failed,
                // Then revert to old behavior

                if (FAILED(hres))
                {
                    hres = GetDisplayNameOf(pidlFirst1, SHGDN_NORMAL, &strret);

                    if (SUCCEEDED(hres))
                    {
                        StrRetToBuf(&strret, &c_idlDesktop, szName1, ARRAYSIZE(szName1));

                        hres = GetDisplayNameOf(pidlFirst2, SHGDN_NORMAL, &strret);

                        if (SUCCEEDED(hres))
                        {
                            StrRetToBuf(&strret, &c_idlDesktop, szName2, ARRAYSIZE(szName2));

                            iRes = lstrcmp(szName1, szName2);
                        }
                    }
                }
            }
            else
            {
                iRes = pidlFirst1 ? 1 : -1;
            }

            if (pidlFirst1)
                ILFree(pidlFirst1);

            if (pidlFirst2)
                ILFree(pidlFirst2);
        }
        // else iRes already = 0
    }
    else
    {
        // Yes, the one which is non-NULL is first
        iRes = (pidl1 ? -1 : 1);
    }

    return iRes;
}

// Folders comes first, and are ordered in alphabetical order among themselves,
// then comes all the non-folders again sorted among thmeselves
int CRegFolder::_CompareIDsFolderFirst(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRes = 0;

    BOOL fIsFolder1 = _IsFolder(pidl1);
    BOOL fIsFolder2 = _IsFolder(pidl2);

    // Is there one folder and one non-folder?
    if (LOGICALXOR(fIsFolder1, fIsFolder2))
    {
        // Yes, the folder will be first
        iRes = fIsFolder1 ? -1 : 1;
    }
    else
    {
        // No, either both are folders or both are not.  One way or the other, go
        // alphabetically
        iRes = _CompareIDsAlphabetical(lParam, pidl1, pidl2);
    }

    return iRes;
}

HRESULT CRegFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = E_FAIL;

    int iRes = 1;

    // first we compare the first level only

    switch (_dwSortAttrib)
    {
        case RIISA_ORIGINAL:
        {
            LPCIDLREGITEM pidlr1 = _IsReg(pidl1);
            LPCIDLREGITEM pidlr2 = _IsReg(pidl2);

            if (!pidlr1 && !pidlr2)
            {
                // None of them is a regitem
//review: what are we doing here if none of them are regitem?

                return _psfOuter->CompareIDs(lParam, pidl1, pidl2);
            }
            else
                iRes = _CompareIDsOriginal(lParam, pidlr1, pidlr2);

            break;
        }

        case RIISA_FOLDERFIRST:
        {
            iRes = _CompareIDsFolderFirst(lParam, pidl1, pidl2);
            break;
        }

        case RIISA_ALPHABETICAL:
        {
            iRes = _CompareIDsAlphabetical(lParam, pidl1, pidl2);
            break;
        }
        default:
            break;
    }

    // did it succeeded but do not get a final result (multipart pidl)
    if (0 == iRes)
    {
        LPCIDLREGITEM pidlr1 = _IsReg(pidl1);
        LPCIDLREGITEM pidlr2 = _IsReg(pidl2);

        if (pidlr1 && pidlr2)
        {
            // The names are the same; make sure the CLSID's are the same
            iRes = memcmp(&(_GetPIDLRCLSID(pidlr1)), &(_GetPIDLRCLSID(pidlr2)), SIZEOF(CLSID));
        }

        if (0 == iRes)
        {
            // If the class ID's really are the same, we'd better check the next level(s)
            return ILCompareRelIDs((IShellFolder *)this, pidl1, pidl2);
        }
    }
    else
    {
        // Flip (or not) the result according to _iCmp
        iRes *= _iCmp;
    }

    return ResultFromShort(iRes);
}

HRESULT CRegFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    return _psfOuter->CreateViewObject(hwnd, riid, ppv);
}

HRESULT CRegFolder::_SetAttributes(LPCIDLREGITEM pidlr, BOOL bPerUser, DWORD dwMask, DWORD dwNewBits)
{
    HKEY hk;
    HRESULT hres = SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), TEXT("ShellFolder"), bPerUser, TRUE, &hk);
    if (SUCCEEDED(hres))
    {
        DWORD err, dwValue = 0, cbSize = SIZEOF(dwValue);
        SHQueryValueEx(hk, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwValue, &cbSize);

        dwValue = (dwValue & ~dwMask) | (dwNewBits & dwMask);

        err = RegSetValueEx(hk, TEXT("Attributes"), 0, REG_DWORD, (BYTE *)&dwValue, SIZEOF(dwValue));
        hres = HRESULT_FROM_WIN32(err);
        RegCloseKey(hk);
    }

    _clsidAttributesCache = CLSID_NULL;

    return hres;
}

ULONG CRegFolder::_GetPerUserAttributes(LPCIDLREGITEM pidlr)
{
    DWORD dwAttribute = 0;
    HKEY hk;

    if (SUCCEEDED(SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), TEXT("ShellFolder"), TRUE, FALSE, &hk)))
    {
        DWORD cb = SIZEOF(dwAttribute);
        SHQueryValueEx(hk, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwAttribute, &cb);
        RegCloseKey(hk);
    }
    // we only allow these bits to change
    return dwAttribute & (SFGAO_NONENUMERATED | SFGAO_CANDELETE | SFGAO_CANMOVE);
}


#define SFGAO_REQ_MASK (SFGAO_NONENUMERATED | SFGAO_CANDELETE | SFGAO_CANMOVE)

ULONG CRegFolder::_GetAttributesOf(LPCIDLREGITEM pidlr, DWORD dwAttributesNeeded)
{
    DWORD dwAttributes = 0;
    BOOL bGuidMatch = IsEqualGUID(_GetPIDLRCLSID(pidlr), _clsidAttributesCache);

    if (bGuidMatch && ((dwAttributesNeeded & _dwAttributesCacheValid) == dwAttributesNeeded))
    {
        dwAttributes = _dwAttributesCache;
    }
    else
    {
        CLSID clsid = _GetPIDLRCLSID(pidlr); // alignment
        int iItem = _ReqItemIndex(pidlr);

        // if the guid didn't match, we need to start from scratch.
        // otherwise, we'll or back in the cahced bits...
        if (!bGuidMatch)
        {
            _dwAttributesCacheValid = 0;
            _dwAttributesCache = 0;
        }

        if (iItem >= 0)
        {
            dwAttributes = _aReqItems[iItem].dwAttributes;
            // per machine attributes allow items to be hidden per machine
            dwAttributes |= SHGetAttributesFromCLSID2(&clsid, 0, SFGAO_REQ_MASK) & SFGAO_REQ_MASK;
        }
        else
        {
            dwAttributes = SHGetAttributesFromCLSID2(&clsid, SFGAO_CANMOVE | SFGAO_CANDELETE, dwAttributesNeeded & ~_dwAttributesCacheValid);
        }
        dwAttributes |= _GetPerUserAttributes(pidlr);   // hidden per user
        dwAttributes |= _dwDefAttributes;               // per folder defaults
        dwAttributes |= _dwAttributesCache;

        _clsidAttributesCache = clsid;
        _dwAttributesCache = dwAttributes;
        _dwAttributesCacheValid |= dwAttributesNeeded | dwAttributes; // if they gave us more than we asked for, cache them 
    }
    return dwAttributes;
}

HRESULT CRegFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    UINT rgfOut = *prgfInOut;
    LPCITEMIDLIST *ppidl;

    // This is a special case for the folder as a whole, so I know nothing about it.
    if (!cidl)
        return _psfOuter->GetAttributesOf(cidl, apidl, prgfInOut);

    ppidl = (LPCITEMIDLIST*)LocalAlloc(LPTR, cidl * SIZEOF(LPCITEMIDLIST));
    if (ppidl)
    {
        int i;
        HRESULT hres = S_OK;
        LPCITEMIDLIST *ppidlEnd = ppidl + cidl;

        for (i = cidl - 1; i >= 0; --i)
        {
            LPCIDLREGITEM pidlr = _IsReg(apidl[i]);
            if (pidlr)
            {
                if (*prgfInOut & SFGAO_VALIDATE)
                {
                    if (!_IsInNameSpace(pidlr)) {
                        IUnknown *punk;
                        // validate by binding
                        hres = _BindToItem(pidlr, NULL, IID_IUnknown, (void **)&punk, FALSE);
                        if (FAILED(hres))
                            goto exit;      // Exit with failure code in <hres>
                        punk->Release();
                    }
                }
                rgfOut &= _GetAttributesOf(pidlr, *prgfInOut);
                cidl--;     // remove this from the list used below...
            }
            else
            {
                --ppidlEnd;
                *ppidlEnd = apidl[i];
            }
        }

        if (cidl)   // any non reg items left?
        {
            ULONG rgfThis = rgfOut;
            hres = _psfOuter->GetAttributesOf(cidl, ppidlEnd, &rgfThis);
            rgfOut &= rgfThis;
        }

    exit:
        LocalFree((HLOCAL)ppidl);

        *prgfInOut = rgfOut;
        return hres;
    }
    return E_OUTOFMEMORY;
}

HRESULT CRegFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, 
                                  REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr;
    
    *ppv = NULL;
    LPCIDLREGITEM pidlr = _AnyRegItem(cidl, apidl);
    if (pidlr)
    {
        if (IsEqualIID(riid, IID_IExtractIconA) ||
            IsEqualIID(riid, IID_IExtractIconW))
        {
            HKEY hkCLSID;
            LPCTSTR pszIconFile;
            int iDefIcon;
            int iItem = _ReqItemIndex(pidlr);

            if (iItem >= 0)
            {
                pszIconFile = _aReqItems[iItem].pszIconFile;
                iDefIcon = _aReqItems[iItem].iDefIcon;
            }
            else
            {
                pszIconFile = NULL;
                iDefIcon = II_FOLDER;
            }

            // try first to get a per-user icon
            hr = SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, TRUE, FALSE, &hkCLSID);
            if (SUCCEEDED(hr))
            {
                hr = SHCreateDefExtIconKey(hkCLSID, pszIconFile, iDefIcon, iDefIcon, GIL_PERCLASS, riid, ppv);
                if (hr == S_FALSE)
                {
                    ((IUnknown *)*ppv)->Release();    // Lose this bad guy
                    *ppv = NULL;
                }
                RegCloseKey(hkCLSID);
            }

            //
            // fall back to a per-class icon
            //
            if (*ppv == NULL)
            {
                SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, FALSE, FALSE, &hkCLSID);
                hr = SHCreateDefExtIconKey(hkCLSID, pszIconFile, iDefIcon, iDefIcon, GIL_PERCLASS, riid, ppv);
                RegCloseKey(hkCLSID);
            }
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            hr = CIDLData_CreateFromIDArray(_GetFolderIDList(), cidl, apidl, (IDataObject **)ppv);
        }
        else if (IsEqualIID(riid, IID_IQueryInfo))
        {
            hr = _GetInfoTip(pidlr, ppv);
        }
        else if (IsEqualIID(riid, IID_IQueryAssociations))
        {
            hr = _AssocCreate(pidlr, riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IContextMenu))
        {
            IShellFolder *psf;
            hr = _psfOuter->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                HKEY keys[2];

                _GetClassKeys(pidlr, &keys[0], &keys[1]);

                hr = CDefFolderMenu_Create2Ex(_GetFolderIDList(), hwnd,
                                               cidl, apidl, 
                                               psf, this,
                                               ARRAYSIZE(keys), keys, 
                                               (IContextMenu **)ppv);

                SHRegCloseKeys(keys, ARRAYSIZE(keys));
                psf->Release();
            }
        }
        else if (cidl == 1)
        {
            // blindly delegate unknown riid (IDropTarget, IShellLink, etc) through
            // APP COMPAT!  GetUIObjectOf does not support multilevel pidls, but
            // Symantec Internet Fast Find does a
            //
            //      psfDesktop->GetUIObjectOf(1, &pidlComplex, IID_IDropTarget, ...)
            //
            //  on a multilevel pidl and expects it to work.  I guess it worked by
            //  lucky accident once upon a time, so now it must continue to work,
            //  but only on the desktop.
            //
            hr = _CreateViewObjectFor(pidlr, hwnd, riid, ppv, !_IsDesktop());
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = _psfOuter->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    }
    return hr;
}

HRESULT CRegFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        HRESULT hres;
        LPCITEMIDLIST pidlNext = _ILNext(pidl);

        if (ILIsEmpty(pidlNext))
        {
            TCHAR szName[MAX_PATH];

            hres = _GetDisplayName(pidlr, dwFlags, szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hres))
                hres = StringToStrRet(szName, pStrRet);
        }
        else
        {
            IShellFolder *psfNext;
            hres = _BindToItem(pidlr, NULL, IID_PPV_ARG(IShellFolder, &psfNext), TRUE);
            if (SUCCEEDED(hres))
            {
                hres = psfNext->GetDisplayNameOf(pidlNext, dwFlags, pStrRet);
                //  If it returns an offset to the pidlNext, we should
                // change the offset relative to pidl.
                if (SUCCEEDED(hres) && pStrRet->uType == STRRET_OFFSET)
                    pStrRet->uOffset += (DWORD)((BYTE *)pidlNext - (BYTE *)pidl);

                psfNext->Release();
            }
        }
        return hres;
    }
    return _psfOuter->GetDisplayNameOf(pidl, dwFlags, pStrRet);
}

HRESULT CRegFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                              LPCOLESTR pszName, DWORD dwFlags, LPITEMIDLIST *ppidlOut)
{
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        HRESULT hres = E_INVALIDARG;
        HKEY hkCLSID;

        if (ppidlOut)
            *ppidlOut = NULL;

        _clsidNameCache = CLSID_NULL;

        // See if per-user entry exists...
        hres = SHRegGetCLSIDKey(_GetPIDLRCLSID(pidlr), NULL, TRUE, TRUE, &hkCLSID);

        // If no per-user, then use per-machine...
        if (FAILED(hres))
        {
            _GetClassKeys(pidlr, &hkCLSID, NULL);

            if (hkCLSID)
            {
                hres = S_OK;
            }
            else
            {
                hres = E_FAIL;
            }
        }

        if (SUCCEEDED(hres))
        {
            TCHAR szName[MAX_PATH];

            SHUnicodeToTChar(pszName, szName, ARRAYSIZE(szName));

            if (RegSetValue(hkCLSID, NULL, REG_SZ, szName, (lstrlen(szName) + 1) * SIZEOF(TCHAR)) == ERROR_SUCCESS)
            {
                LPITEMIDLIST pidlAbs = ILCombine(_GetFolderIDList(), pidl);
                if (pidlAbs)
                {
                    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlAbs, NULL);
                    ILFree(pidlAbs);
                }

                if (ppidlOut)
                    *ppidlOut = ILClone(pidl);  // name is not in the PIDL so old == new

                hres = NOERROR;
            }
            else
                hres = E_FAIL;

            RegCloseKey(hkCLSID);
        }
        return hres;
    }
    return _psfOuter->SetNameOf(hwnd, pidl, pszName, dwFlags, ppidlOut);
}

HRESULT CRegFolder::GetDefaultSearchGUID(LPGUID lpGuid)
{
    return _psfOuter->GetDefaultSearchGUID(lpGuid);
}   

HRESULT CRegFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    return _psfOuter->EnumSearches(ppenum);
}

HRESULT CRegFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return _psfOuter->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT CRegFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{
    return _psfOuter->GetDefaultColumnState(iColumn, pbState);
}

HRESULT CRegFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = E_NOTIMPL;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did;
            did.dwDescriptionId = SHDID_ROOT_REGITEM;
            did.clsid = _GetPIDLRCLSID(pidlr);
            hr = InitVariantFromBuffer(pv, &did, sizeof(did));
        }
        else
        {
            int iCol = FindSCID(s_reg_cols, ARRAYSIZE(s_reg_cols), pscid);
            if (iCol >= 0)
            {
                TCHAR szTemp[INFOTIPSIZE];
                szTemp[0] = 0;
                switch (s_reg_cols[iCol].iCol)
                {
                case REG_ICOL_NAME:
                    _GetDisplayName(pidlr, SHGDN_NORMAL, szTemp, ARRAYSIZE(szTemp));
                    break;

                case REG_ICOL_TYPE:
                    LoadString(HINST_THISDLL, IDS_DRIVES_REGITEM, szTemp, ARRAYSIZE(szTemp));
                    break;

                case REG_ICOL_COMMENT:
                    _GetRegItemColumnFromRegistry(pidlr, TEXT("InfoTip"), szTemp, ARRAYSIZE(szTemp));
                    break;
                }
                if (szTemp[0])
                    hr = InitVariantFromStr(pv, szTemp);
            }
        }
    }
    else
    {
        hr = _psfOuter->GetDetailsEx(pidl, pscid, pv);
    }
    return hr;
}

HRESULT CRegFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetail)
{
    HRESULT hr = E_FAIL;
    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        pDetail->str.uType = STRRET_CSTR;
        pDetail->str.cStr[0] = 0;
        hr = S_OK;

        SHCOLUMNID scid;
        if (SUCCEEDED(_psfOuter->MapColumnToSCID(iColumn, &scid)))
        {
            hr = MapSCIDToDetailsOf(this, pidl, &scid, pDetail);
        }
    }
    else
    {
        hr = _psfOuter->GetDetailsOf(pidl, iColumn, pDetail);
    }
    return hr;
}

HRESULT CRegFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    return _psfOuter->MapColumnToSCID(iColumn, pscid);
}

HRESULT CRegFolder::_GetOverlayInfo(LPCIDLREGITEM pidlr, int *pIndex, BOOL fIconIndex)
{
    HRESULT hr = E_FAIL;
    const CLSID clsid = _GetPIDLRCLSID(pidlr);    // alignment  
    if (SHQueryShellFolderValue(&clsid, TEXT("QueryForOverlay")))
    {
        IShellIconOverlay* psio;

        hr = _BindToItem(pidlr, NULL, IID_PPV_ARG(IShellIconOverlay, &psio), TRUE);
        if (SUCCEEDED(hr))
        {
            // NULL pidl means "I want to know about YOU, folder, not one of your kids"
            if (fIconIndex)
                hr = psio->GetOverlayIconIndex(NULL, pIndex);
            else
                hr = psio->GetOverlayIndex(NULL, pIndex);
            psio->Release();
        }
    }

    return hr;
}

// *** IShellIconOverlay methods ***
HRESULT CRegFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;

    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        hr = _GetOverlayInfo(pidlr, pIndex, FALSE);
    }
    else if (_psioOuter)
    {
        hr = _psioOuter->GetOverlayIndex(pidl, pIndex);
    }

    return hr;
}

HRESULT CRegFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIconIndex)
{
    HRESULT hr = E_FAIL;

    LPCIDLREGITEM pidlr = _IsReg(pidl);
    if (pidlr)
    {
        hr = _GetOverlayInfo(pidlr, pIconIndex, TRUE);
    }
    else if (_psioOuter)
    {
        hr = _psioOuter->GetOverlayIconIndex(pidl, pIconIndex);
    }

    return hr;
}

//-----------------------------------------------------------------------------
// CContextMenuCB
//-----------------------------------------------------------------------------

DWORD CALLBACK _RegFolderPropThreadProc(void *pv)
{
    PROPSTUFF *pdps = (PROPSTUFF *)pv;
    CRegFolder *prf = (CRegFolder *)pdps->psf;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdps->pdtobj, &medium);
    if (pida)
    {
        LPCIDLREGITEM pidlr = prf->_IsReg(IDA_GetIDListPtr(pida, 0));
        if (pidlr)
        {
            int iItem = prf->_ReqItemIndex(pidlr);
            if (iItem >= 0 && prf->_aReqItems[iItem].pszCPL)
                SHRunControlPanel(prf->_aReqItems[iItem].pszCPL, NULL);
            else
            {
                TCHAR szName[MAX_PATH];
                if (SUCCEEDED(prf->_GetDisplayName(pidlr, SHGDN_NORMAL, szName, ARRAYSIZE(szName))))
                {
                    HKEY hk;

                    prf->_GetClassKeys(pidlr, &hk, NULL);

                    if (hk)
                    {
                        SHOpenPropSheet(szName, &hk, 1, NULL, pdps->pdtobj, NULL, (LPCTSTR)pdps->pStartPage);
                        RegCloseKey(hk);
                    }
                }   
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return 0;
}

DWORD DisconnectDialogOnThread(void *pv)
{
    WNetDisconnectDialog(NULL, RESOURCETYPE_DISK);
    SHChangeNotifyHandleEvents();       // flush any drive notifications
    return 0;
}

HRESULT CRegFolder::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, 
                                       UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        {
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                // some ugly specal cases...
                if (HIDA_GetCount(medium.hGlobal) == 1)
                {
                    LPCIDLREGITEM pidlr = _IsReg(IDA_GetIDListPtr(pida, 0));
                    if (pidlr)
                    {
                        const CLSID clsid = _GetPIDLRCLSID(pidlr);  // alignment

                        if ((IsEqualGUID(clsid, CLSID_MyComputer) ||
                             IsEqualGUID(clsid, CLSID_NetworkPlaces)) &&
                            (GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS) &&
                             !SHRestricted(REST_NONETCONNECTDISCONNECT))
                        {
                            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DESKTOP_ITEM, 0, (LPQCMINFO)lParam);
                        }
#ifndef WINNT
                        else if (IsEqualGUID(clsid, CLSID_Printers))
                        {
                            // (iff network is installed)
                            if (GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS)
                            {
                                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DRIVES_PRINTERS, 0, (LPQCMINFO)lParam);
                            }
                        }
#endif
                    }
                }
                HIDA_ReleaseStgMedium(pida, &medium);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMANDEX:
        {
            DFMICS *pdfmics = (DFMICS *)lParam;
            switch (wParam)
            {
            case FSIDM_CONNECT:
                SHStartNetConnectionDialog(NULL, NULL, RESOURCETYPE_DISK);
                break;

            case FSIDM_DISCONNECT:
                SHCreateThread(DisconnectDialogOnThread, NULL, CTF_COINIT, NULL);
                break;

            case DFM_CMD_PROPERTIES:
                SHLaunchPropSheet(_RegFolderPropThreadProc, pdtobj, (LPCTSTR)pdfmics->lParam, this, NULL);
                break;

            case DFM_CMD_DELETE:
                _Delete(hwnd, pdfmics->fMask, pdtobj);
                break;

            default:
                // This is one of view menu items, use the default code.
                hres = S_FALSE;
                break;
            }
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}


//-----------------------------------------------------------------------------
// IRegItemsFolder methods
//-----------------------------------------------------------------------------

HRESULT CRegFolder::Initialize(REGITEMSINFO *pri) 
{
    ASSERT(pri != NULL);
    HRESULT hres = E_INVALIDARG;

    _pszRegKey        = pri->pszRegKey;
    _pPolicy          = pri->pPolicy;
    _chRegItem        = pri->cRegItem;
    _bFlags           = pri->bFlags;
    _iCmp             = pri->iCmp;
    _dwDefAttributes  = pri->rgfRegItems;
    _dwSortAttrib     = pri->dwSortAttrib;
    _cbPadding        = pri->cbPadding;
    _bFlagsLegacy     = pri->bFlagsLegacy;

    if ((RIISA_ORIGINAL == _dwSortAttrib) || 
        (RIISA_FOLDERFIRST == _dwSortAttrib) ||
        (RIISA_ALPHABETICAL == _dwSortAttrib))
    {
        Str_SetPtr(&_pszMachine, pri->pszMachine);    // save a copy of this

        _aReqItems = (REQREGITEM *)LocalAlloc(LPTR, SIZEOF(REQREGITEM)*pri->iReqItems);
        if ( !_aReqItems )
            return E_OUTOFMEMORY;

        memcpy(_aReqItems, pri->pReqItems, SIZEOF(REQREGITEM)*pri->iReqItems);
        _nRequiredItems = pri->iReqItems;

        // If we are aggregated, cache the _psioOuter and _psfOuter
        _QueryOuterInterface(IID_PPV_ARG(IShellIconOverlay, &_psioOuter));
        hres = _QueryOuterInterface(IID_PPV_ARG(IShellFolder2, &_psfOuter));
    }
    return hres;
}


//
// instance creation of the RegItems object
//

STDAPI CRegFolder_CreateInstance(REGITEMSINFO *pri, IUnknown *punkOuter, REFIID riid, void **ppv) 
{
    HRESULT hres;
    ULONG cRef;

    // we only suport being created as an agregate
    if (!punkOuter || !IsEqualIID(riid, IID_IUnknown))
    {
        ASSERT(0);
        return E_FAIL;
    }

    CRegFolder *prif = new CRegFolder(punkOuter);
    if ( !prif )
        return E_OUTOFMEMORY;

    hres = prif->Initialize(pri);           // initialize the regfolder
    if ( SUCCEEDED(hres) )
        hres = prif->_GetInner()->QueryInterface(riid, ppv);

    //
    //  If the Initalize and QueryInterface succeeded, this will drop
    //  the refcount from 2 to 1.  If they failed, then this will drop
    //  the refcount from 1 to 0 and free the object.
    //
    cRef = prif->_GetInner()->Release();

    //
    //  On success, the object should have a refcout of exactly 1.
    //  On failure, the object should have a refcout of exactly 0.
    //
    ASSERT(SUCCEEDED(hres) == (BOOL)cRef);

    return hres;
}


//-----------------------------------------------------------------------------
// CRegFolderEnum
//-----------------------------------------------------------------------------

CRegFolderEnum::CRegFolderEnum(CRegFolder* prf, DWORD grfFlags, IEnumIDList* peidl, HDCA hdca,
     REGITEMSPOLICY* pPolicy) :
    _cRef(1),
    _iCur(0),
    _grfFlags(grfFlags),
    _prf(prf),
    _peidl(peidl),
    _hdca(hdca),
    _pPolicy(pPolicy)
{
    _prf->AddRef();

    if (_peidl)
        _peidl->AddRef();

    DllAddRef();
}

CRegFolderEnum::~CRegFolderEnum()
{
    _prf->Release();

    if (_peidl)
        _peidl->Release();

    if (_hdca)
        DCA_Destroy(_hdca);

    DllRelease();
}

//
// IUnknown
//

STDMETHODIMP_(ULONG) CRegFolderEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CRegFolderEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CRegFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =  {
        QITABENT(CRegFolderEnum, IEnumIDList), // IID_IEnumIDList
        { 0 },
    };    
    return QISearch(this, qit, riid, ppv);
}


//
// IEnumIDList methods
//

HRESULT CRegFolderEnum::Next(ULONG celt, LPITEMIDLIST *ppidlOut, ULONG *pceltFetched)
{
    if (_hdca)
    {
        while (_iCur < DCA_GetItemCount(_hdca))
        {
            HRESULT hres;
            DWORD dwAttribItem;
            IDLREGITEM* pidlRegItem = NULL;

            _iCur++;

            // We're filling the regitem with class id clsid. If this is a
            // remote item, first invoke the class to see if it really wants
            // to be enumerated for this remote computer.
            if (_prf->_pszMachine)
            {
                IUnknown* punk;
                hres = DCA_CreateInstance(_hdca, _iCur-1, IID_IUnknown, (void **)&punk);
                if (SUCCEEDED(hres))
                {
                    hres = _prf->_InitFromMachine(punk, TRUE);
                    punk->Release();
                }

                if (FAILED(hres))
                    continue;
            }

            // Check policy restrictions
            if (_pPolicy != NULL)
            {
                TCHAR szGUID[64];
                TCHAR szName[256];
                SHStringFromGUID(*DCA_GetItem(_hdca, _iCur-1), szGUID, ARRAYSIZE(szGUID));
                if (FAILED(hres))
                    continue;
                szName[0] = 0;

                HKEY hkRoot;
                if (RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hkRoot) == ERROR_SUCCESS)
                {
                    HKEY hkCLSID;
                    if (RegOpenKey(hkRoot, szGUID, &hkCLSID) == ERROR_SUCCESS)
                    {
                        // Check per-language string first.
                        SHLoadRegUIString(hkCLSID, TEXT("LocalizedString"), szName, ARRAYSIZE(szName));
                        // If not found "LocalizedString" read the default string.
                        if (szName[0] == 0)
                            SHLoadRegUIString(hkCLSID, TEXT(""), szName, ARRAYSIZE(szName));
                        RegCloseKey(hkCLSID);
                    }
                    RegCloseKey(hkRoot);
                }
                if (szName[0] != 0)
                {
                    if (SHRestricted(_pPolicy->restAllow) && !IsNameListedUnderKey(szName, _pPolicy->pszAllow))
                        continue;
                    if (SHRestricted(_pPolicy->restDisallow) && IsNameListedUnderKey(szName, _pPolicy->pszDisallow))
                        continue;
                }
            }


            // Ok, actually enumerate the item

            pidlRegItem = _prf->_CreateAndFillIDLREGITEM(DCA_GetItem(_hdca, _iCur-1));

            if (pidlRegItem)
            {
                dwAttribItem = _prf->_GetAttributesOf(pidlRegItem, SFGAO_NONENUMERATED | SFGAO_FOLDER);

                if (dwAttribItem & SFGAO_NONENUMERATED)
                {
                    SHFree(pidlRegItem);
                    continue;
                }

                if ((_grfFlags & (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS)) != (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS))
                {
                    if (dwAttribItem & SFGAO_FOLDER)
                    {
                        if (!(_grfFlags & SHCONTF_FOLDERS))
                        {
                            SHFree(pidlRegItem);
                            continue;
                        }
                    }
                    else
                    {
                        if (!(_grfFlags & SHCONTF_NONFOLDERS))
                        {
                            SHFree(pidlRegItem);
                            continue;
                        }
                    }
                }

                *ppidlOut = (LPITEMIDLIST)pidlRegItem;

                hres = S_OK;
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hres) && pceltFetched)
                *pceltFetched = 1;

            return hres;
        }
    }

    // Either there is no DKA or we are done with it, so just pass along to
    // to the folder
    if (_peidl)
        return _peidl->Next(celt, ppidlOut, pceltFetched);

    *ppidlOut = NULL;
    if (pceltFetched)
        pceltFetched = 0;

    return S_FALSE;
}

STDMETHODIMP CRegFolderEnum::Reset()
{
    _iCur = 0;
    if (_peidl)
        return _peidl->Reset();

    return S_OK;
}
