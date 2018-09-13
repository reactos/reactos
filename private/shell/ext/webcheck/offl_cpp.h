#ifndef OFFLINE_CPP_H
#define OFFLINE_CPP_H

#ifdef __cplusplus

//  #include <debug.h>
//  #include <crtfree.h>

// Forward class declarations
class COfflineFolderEnum;
class COfflineFolder;
class COfflineObjectItem;
class COfflineDropTarget;

#define PROP_WEBCRAWL_SIZE      0x00000001
#define PROP_WEBCRAWL_FLAGS     0x00000002
#define PROP_WEBCRAWL_LEVEL     0x00000004
#define PROP_WEBCRAWL_ACTUALSIZE    0x00000008
#define PROP_WEBCRAWL_URL       0x00000010
#define PROP_WEBCRAWL_NAME      0x00000020
#define PROP_WEBCRAWL_EMAILNOTF 0x00000040
#define PROP_WEBCRAWL_PSWD      0x00000080
#define PROP_WEBCRAWL_UNAME     0x00000100
#define PROP_WEBCRAWL_DESKTOP   0x00000200
#define PROP_WEBCRAWL_RESCH     0x00000400
#define PROP_WEBCRAWL_COOKIE    0x00000800
#define PROP_WEBCRAWL_LAST      0x00001000
#define PROP_WEBCRAWL_STATUS    0x00002000
#define PROP_WEBCRAWL_CHANNEL   0x00004000
#define PROP_WEBCRAWL_PRIORITY  0x00008000
#define PROP_WEBCRAWL_GLEAM     0x00010000
#define PROP_WEBCRAWL_CHANGESONLY   0x00020000
#define PROP_WEBCRAWL_CHANNELFLAGS  0x00040000
#define PROP_WEBCRAWL_ALL       0x0007FFFF
#define PROP_WEBCRAWL_EXTERNAL  PROP_WEBCRAWL_ACTUALSIZE | PROP_WEBCRAWL_URL | \
                                PROP_WEBCRAWL_NAME | PROP_WEBCRAWL_EMAILNOTF | \
                                PROP_WEBCRAWL_RESCH | PROP_WEBCRAWL_LAST | \
                                PROP_WEBCRAWL_DESKTOP | PROP_WEBCRAWL_CHANNEL |\
                                PROP_WEBCRAWL_STATUS | PROP_WEBCRAWL_PRIORITY

#define PROP_GENERAL_MASK       PROP_WEBCRAWL_PSWD | PROP_WEBCRAWL_UNAME
#define PROP_RECEIVING_MASK     PROP_WEBCRAWL_SIZE | PROP_WEBCRAWL_FLAGS | \
                                PROP_WEBCRAWL_CHANGESONLY | PROP_WEBCRAWL_EMAILNOTF \
                                PROP_WEBCRAWL_PRIORITY | PROP_WEBCRAWL_LEVEL | \
                                PROP_WEBCRAWL_CHANNELFLAGS
#define PROP_SCHEDULE_MASK      PROP_WEBCRAWL_CHANNELFLAGS | PROP_WEBCRAWL_RESCH

// Forwawd declarations for create instance functions 
HRESULT COfflineObjectItem_CreateInstance(COfflineFolder *pOOFolder, UINT cidl, LPCITEMIDLIST *ppidl, REFIID riid, void **ppvOut);
HRESULT DoDeleteSubscription(POOEntry);
HRESULT FindURLProps(LPCTSTR, PROPVARIANT *);

SUBSCRIPTIONSCHEDULE GetGroup(BOOL bDesktop, const CLSID& clsidDest, 
                              DWORD fChannelFlags, const NOTIFICATIONCOOKIE& groupCookie);
inline SUBSCRIPTIONSCHEDULE GetGroup(POOEBuf pBuf)
{
    ASSERT(pBuf);
    return GetGroup(pBuf->bDesktop, pBuf->clsidDest, pBuf->fChannelFlags, pBuf->groupCookie);
}
inline SUBSCRIPTIONSCHEDULE GetGroup(POOEntry pooe)
{
    ASSERT(pooe);
    return GetGroup(pooe->bDesktop, pooe->clsidDest, pooe->fChannelFlags, pooe->groupCookie);
}

SUBSCRIPTIONTYPE GetItemCategory(BOOL bDesktop, const CLSID& clsidDest);
inline SUBSCRIPTIONTYPE GetItemCategory(POOEBuf pBuf)
{
    ASSERT(pBuf);
    return GetItemCategory(pBuf->bDesktop, pBuf->clsidDest);
}

inline SUBSCRIPTIONTYPE GetItemCategory(POOEntry pEntry)
{
    ASSERT(pEntry);
    return GetItemCategory(pEntry->bDesktop, pEntry->clsidDest);
}

//////////////////////////////////////////////////////////////////////////////
//
// COfflineFolderEnum Object
//
//////////////////////////////////////////////////////////////////////////////

class COfflineFolderEnum : public IEnumIDList
{
public:
    COfflineFolderEnum(DWORD grfFlags);
    
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList Methods 
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(LPENUMIDLIST *ppenum);

    // Helper functions

    static LPMYPIDL NewPidl(DWORD);
    static void FreePidl(LPMYPIDL);
    static void EnsureMalloc();
    static IMalloc *s_pMalloc;

    HRESULT Initialize(COfflineFolder *pFolder);

protected:
    ~COfflineFolderEnum();

    UINT                m_cRef;      // ref count
    UINT                m_grfFlags;  // enumeration flags 

private:
    ULONG               m_nCount;
    ULONG               m_nCurrent;
    SUBSCRIPTIONCOOKIE  *m_pCookies;
    COfflineFolder      *m_pFolder;
};

//////////////////////////////////////////////////////////////////////////////
//
// COfflineFolder Object
//
//////////////////////////////////////////////////////////////////////////////

class COfflineFolder : public IShellFolder, 
                       public IPersistFolder2,
                       public IContextMenu
{
    // COfflineFolder interfaces
    friend COfflineObjectItem;
    friend COfflineDropTarget;
    friend COfflineFolderEnum;
    friend HRESULT OfflineFolderView_CreateInstance(COfflineFolder *pOOFolder, LPCITEMIDLIST pidl, void **ppvOut);
    friend HRESULT OfflineFolderView_DidDragDrop(HWND, IDataObject *pdo, DWORD dwEffect);
    friend LPMYPIDL ScheduleDefault(LPCTSTR, LPCTSTR, COfflineFolder *);
        
public:
    COfflineFolder(void);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
   
    // IShellFolder methods
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbcReserved,
            LPOLESTR lpszDisplayName, ULONG *pchEaten,
            LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD grfFlags,
            LPENUMIDLIST *ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
            REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
            REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
            ULONG * rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
            REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
            LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IPersist Methods 
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IContextMenu Methods -- This handles the background context menus
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                  UINT idCmdLast, UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved,
                                  LPSTR pszName, UINT cchMax);

protected:
    ~COfflineFolder();

    UINT            _cRef;
    LPITEMIDLIST    _pidl;      //  Clone;
#define VIEWMODE_WEBCRAWLONLY   1
    UINT            viewMode;

};


class COfflineDetails : public IShellDetails
{
public:
    COfflineDetails(HWND hwndOwner);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellDetails Methods
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);
    STDMETHODIMP ColumnClick(UINT iColumn);

private:
    ~COfflineDetails() {}

    HWND    m_hwndOwner;
    ULONG   m_cRef;
};

////////////////////////////////////////////////////////////////////////////
//
// COfflineObjectItem Object
//
////////////////////////////////////////////////////////////////////////////

class COfflineObjectItem : public IContextMenu, 
                    #ifdef UNICODE
                           public IExtractIconA,
                    #endif
                           public IExtractIcon,
                           public IDataObject,
                           public IQueryInfo
{
    // COfflineObjectItem interfaces
    friend HRESULT OfflineFolderView_DidDragDrop(HWND, IDataObject *pdo, DWORD dwEffect);

public:
    COfflineObjectItem();
    HRESULT Initialize(COfflineFolder *pOOFolder, UINT cidl, LPCITEMIDLIST *ppidl);

    // IUnknown Methods
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID,void **);
    
    // IContextMenu Methods
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                  UINT idCmdLast, UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved,
                                  LPSTR pszName, UINT cchMax);

    // IDataObject Methods...
    STDMETHODIMP GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
    STDMETHODIMP GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut);
    STDMETHODIMP SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
    STDMETHODIMP DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink,
                            LPDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppEnum);
    
    // IDataObject helper functions
    HRESULT _CreateHDROP(STGMEDIUM *pmedium);
    HRESULT _CreateNameMap(STGMEDIUM *pmedium);
    HRESULT _CreateFileDescriptor(STGMEDIUM *pSTM);
    HRESULT _CreateFileContents(STGMEDIUM *pSTM, LONG lindex);
    HRESULT _CreateURL(STGMEDIUM *pSTM);
    HRESULT _CreatePrefDropEffect(STGMEDIUM *pSTM);

#ifdef UNICODE
    //  IExtractIconA
    STDMETHODIMP    GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
    STDMETHODIMP    Extract(LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);
#endif

    // IExtractIconT Methods
    STDMETHODIMP GetIconLocation(UINT, LPTSTR, UINT, int *, UINT *);
    STDMETHODIMP Extract(LPCTSTR, UINT, HICON *, HICON *, UINT);
   
    // IQueryInfo Methods
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR ** ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

protected:

    ~COfflineObjectItem();

    UINT	                _cRef;       // reference count
    COfflineFolder*         _pOOFolder;   // back pointer to our shell folder
    
    UINT                    _cItems;     // number of items we represent
    LPMYPIDL*               _ppooi;      // variable size array of items
    IUnknown                *m_pUIHelper;  // UI helper. For 1 item only.
    
        
};

class COfflineDropTarget : public IDropTarget
{
private:

    LPDATAOBJECT        m_pDataObj;
    ULONG               m_cRefs;
    DWORD               m_grfKeyStateLast;
    BOOL                m_fHasHDROP;
    BOOL                m_fHasSHELLURL;
    BOOL                m_fHasTEXT;
    DWORD               m_dwEffectLastReturned;
    HWND                m_hwndParent;

public:
    
    COfflineDropTarget(HWND hwndParent);
    ~COfflineDropTarget();

    STDMETHODIMP            QueryInterface      (REFIID riid,LPVOID FAR *ppv);
    STDMETHODIMP_(ULONG)    AddRef              ();
    STDMETHODIMP_(ULONG)    Release             ();
    STDMETHODIMP            DragEnter           (LPDATAOBJECT pDataObj, 
                                                 DWORD        grfKeyState,
                                                 POINTL       pt, 
                                                 LPDWORD      pdwEffect);
    STDMETHODIMP            DragOver            (DWORD        grfKeyState, 
                                                 POINTL       pt, 
                                                 LPDWORD      pdwEffect);
    STDMETHODIMP            DragLeave           ();
    STDMETHODIMP            Drop                (LPDATAOBJECT pDataObj,
                                                 DWORD        grfKeyState, 
                                                 POINTL       pt, 
                                                 LPDWORD      pdwEffect);
    DWORD                   GetDropEffect       (LPDWORD      pdwEffect);

};

#endif  // __cplusplus

#endif  // OFFLINE_CPP_H

