#ifndef HSFOLDER_H__
#define HSFOLDER_H__

#ifdef __cplusplus

#include <crtfree.h>
#include <debug.h>
#include "iface.h"

// Forward class declarations
class CHistCacheFolderEnum;
class CHistCacheFolder;
class CHistCacheItem;
class CHistDetails;
class CCacheDetails;
class CBaseDetails;

#define LOTS_OF_FILES (10)

// The number of "top" sites displayed in the "Most Frequently Viewed..." history view
#define NUM_TOP_SITES  20

///////////////////////
//
// Column definition for the History Folder Defview
//
enum {
    ICOLH_URL_NAME = 0,
    ICOLH_URL_TITLE,
    ICOLH_URL_LASTVISITED,
    ICOLH_URL_MAX         // Make sure this is the last enum item
};

#define ICOLH_DIR_MAX (ICOLH_URL_NAME+1)

///////////////////////
//
// Warn on Cookie deletion
//
enum {
    DEL_COOKIE_WARN = 0,
    DEL_COOKIE_YES,
    DEL_COOKIE_NO
};


#define INTERVAL_PREFIX_LEN (6)
#define INTERVAL_VERS_LEN (2)
#define INTERVAL_VERS (TEXT("01"))
#define OUR_VERS (1)
#define UNK_INTERVAL_VERS (0xFFFF)
#define RANGE_LEN (16)
#define INTERVAL_MIN_SIZE (RANGE_LEN+INTERVAL_PREFIX_LEN)
#define INTERVAL_SIZE (RANGE_LEN+INTERVAL_VERS_LEN+INTERVAL_PREFIX_LEN)
#define PREFIX_SIZE (RANGE_LEN+3)

//  NOTE: the interval is closed at the start but open at the end, that
//  is inclusion is time >= start and time < end
typedef struct _HSFINTERVAL
{
    FILETIME ftStart;
    FILETIME ftEnd;
    TCHAR  szPrefix[PREFIX_SIZE+1];
    USHORT usSign;
    USHORT usVers;
} HSFINTERVAL;


BOOL GetDisplayNameForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    LPTSTR szBuffer, int cbBufferLength);
HRESULT _ValueToIntervalW(LPCWSTR wzInterval, FILETIME *pftStart, FILETIME *pftEnd);

HRESULT AlignPidl(LPCITEMIDLIST* ppidl, BOOL* pfRealigned);
void    FreeRealignedPidl(LPCITEMIDLIST pidl);
HRESULT AlignPidlArray(LPCITEMIDLIST* apidl, int cidl, LPCITEMIDLIST** papidl,
                          BOOL* pfRealigned);
void    FreeRealignedPidlArray(LPCITEMIDLIST* apidl, int cidl);


//  DeleteEntries filter callback
typedef BOOL (*PFNDELETECALLBACK)(LPINTERNET_CACHE_ENTRY_INFO pceiWorking, LPVOID pDelData, LPITEMIDLIST *ppidlNotify);

// Forward declarations for create instance functions 
HRESULT CHistCacheItem_CreateInstance(CHistCacheFolder *pHCFolder, HWND hwndOwner, UINT cidl, LPCITEMIDLIST *ppidl, REFIID riid, void **ppvOut);

//////////////////////////////////////////////////////////////////////////////
//
// CHistCacheFolderEnum Object
//
//////////////////////////////////////////////////////////////////////////////

class CHistCacheFolderEnum : public IEnumIDList
{
public:
    CHistCacheFolderEnum(DWORD grfFlags, CHistCacheFolder *pHCFolder);
    
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList Methods 
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(LPENUMIDLIST *ppenum);

protected:
    ~CHistCacheFolderEnum();
    HRESULT _NextHistInterval(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    HRESULT _NextViewPart(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    HRESULT _NextViewPart_OrderToday(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    HRESULT _NextViewPart_OrderSite(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    HRESULT _NextViewPart_OrderFreq(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    HRESULT _NextViewPart_OrderSearch(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    class OrderedList* _GetMostFrequentPages();
    LPCTSTR _GetLocalHost(void);

    LONG                _cRef;      // ref count
    CHistCacheFolder    *_pHCFolder;// this is what we enumerate    
    UINT                _grfFlags;  // enumeration flags 
    UINT                _uFlags;    // local flags   
    LPINTERNET_CACHE_ENTRY_INFO _pceiWorking;        
    HANDLE              _hEnum;
    int              _cbCurrentInterval;     //  position in enum of time intervals
    int              _cbIntervals;
    HSFINTERVAL     *_pIntervalCache;

    HSFINTERVAL       *_pIntervalCur;
    class StrHash     *_pshHashTable;     // used to keep track of what I gave out
    class OrderedList *_polFrequentPages; // used to store most frequently viewed pgs
    IEnumSTATURL      *_pstatenum;        // used in search enumerator
    TCHAR   _szLocalHost[INTERNET_MAX_HOST_NAME_LENGTH]; //  "My Computer"  cached...

    static BOOL_PTR s_DoCacheSearch(LPINTERNET_CACHE_ENTRY_INFO pcei,
                                LPTSTR pszUserName, UINT uUserNameLen, CHistCacheFolderEnum *penum,
                                class _CurrentSearches *pcsThisThread, IUrlHistoryPriv *pUrlHistStg);
    static DWORD WINAPI s_CacheSearchThreadProc(CHistCacheFolderEnum *penum);
};


//////////////////////////////////////////////////////////////////////////////
//
// CHistCacheFolder Object
//
//////////////////////////////////////////////////////////////////////////////

class CHistCacheFolder : public IShellFolder2, 
                         public IShellIcon, 
                         public IPersistFolder2,
                         public IContextMenu,
                         public IHistSFPrivate,
                         public IShellFolderViewType,
                         public IShellFolderSearchable
{
    // CHistCacheFolder interfaces
    friend CBaseDetails;
    friend CHistDetails;
    friend CCacheDetails;
    friend CHistCacheFolderEnum;
    friend CHistCacheItem;
    friend HRESULT HistCacheFolderView_CreateInstance(CHistCacheFolder *pHCFolder, LPCITEMIDLIST pidl, void **ppvOut);
    friend HRESULT HistCacheFolderView_DidDragDrop(IDataObject *pdo, DWORD dwEffect);
    friend HRESULT CALLBACK HistCacheFolderView_ViewCallback(IShellView *psvOuter, 
        IShellFolder *psf, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
        FOLDER_TYPE FolderType);
        
public:
    CHistCacheFolder(FOLDER_TYPE FolderType);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
   
    // IShellFolder methods
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc,
            LPOLESTR lpszDisplayName, ULONG *pchEaten,
            LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD grfFlags,
            LPENUMIDLIST *ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc,
            REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc,
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

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid) { return E_NOTIMPL; };
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum) { *ppenum = NULL; return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails) PURE;
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid) { return E_NOTIMPL; };

    // IHistSFPrivate Methods
    STDMETHODIMP SetCachePrefix(LPCWSTR pszCachePrefix);
    STDMETHODIMP SetDomain(LPCWSTR pszDomain);
    STDMETHODIMP WriteHistory(LPCWSTR pszPrefixedUrl, FILETIME ftExpires, 
                              FILETIME ftModified, LPITEMIDLIST * ppidlSelect);
    STDMETHODIMP ClearHistory();

    // IShellIcon Methods 
    STDMETHODIMP GetIconOf(LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex);

    // IPersist Methods 
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IPersistFolder Methods
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2 Methods
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IContextMenu Methods -- This handles the background context menus
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                  UINT idCmdLast, UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved,
                                  LPSTR pszName, UINT cchMax);

    // IShellFolderViewType Methods
    STDMETHODIMP EnumViews(ULONG grfFlags, LPENUMIDLIST* ppenum);
    STDMETHODIMP GetDefaultViewName(DWORD uFlags, LPWSTR *ppwszName);
    STDMETHODIMP GetViewTypeProperties(LPCITEMIDLIST pidl, LPDWORD pdwFlags);
    STDMETHODIMP TranslateViewPidl(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlView,
                                   LPITEMIDLIST *pidlOut);

    // IShellFolderSearchable Methods
    STDMETHODIMP FindString(LPCWSTR pwszTarget, LPDWORD pdwFlags,
                            IUnknown *punkOnAsyncSearch,
                            LPITEMIDLIST *ppidlOut);
    STDMETHODIMP CancelAsyncSearch (LPCITEMIDLIST pidlSearch, LPDWORD pdwFlags);
    STDMETHODIMP InvalidateSearch  (LPCITEMIDLIST pidlSearch, LPDWORD pdwFlags);
    
protected:
    ~CHistCacheFolder();
    
    void _GetHistURLDispName(LPHEIPIDL phei, LPTSTR pszStr, UINT cchStr);

    HRESULT _CompareAlignedIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    HRESULT _CopyTSTRField(LPTSTR *ppszField, LPCTSTR pszValue);
    HRESULT _LoadIntervalCache();
    HRESULT _GetInterval(FILETIME *pftItem, BOOL fWeekOnly, HSFINTERVAL **pInterval);
    HRESULT _CreateInterval(FILETIME *pftStart, DWORD dwDays);
    HRESULT _PrefixUrl(LPCTSTR pszStrippedUrl,
                     FILETIME *pftLastModifiedTime,
                     LPTSTR pszPrefixedUrl,
                     DWORD cbPrefixedUrl);
    HRESULT _CopyEntries(LPCTSTR pszHistPrefix);
    HRESULT _DeleteEntries(LPCTSTR pszHistPrefix, PFNDELETECALLBACK pfnDeleteFilter, LPVOID pdcbData);
    HRESULT _DeleteInterval(HSFINTERVAL *pInterval);
    HRESULT _CleanUpHistory(FILETIME ftLimit, FILETIME ftTommorrow);
    HRESULT _ValidateIntervalCache();
    HRESULT _WriteHistory(LPCTSTR pszPrefixedUrl,
                                       FILETIME ftExpires, 
                                       FILETIME ftModified, 
                                       BOOL fSendNotify,
                                       LPITEMIDLIST * ppidlSelect);
    HRESULT _GetPrefixForInterval(LPCTSTR pszInterval, LPCTSTR *ppszCachePrefix);
    HRESULT _ExtractInfoFromPidl();
    HRESULT _ViewType_NotifyEvent(LPITEMIDLIST pidlRoot,
                                  LPITEMIDLIST pidlHost,
                                  LPITEMIDLIST pidlPage,
                                  LONG         wEventId);
    HRESULT _NotifyWrite(LPTSTR pszUrl, int cchUrl, FILETIME *pftModified, LPITEMIDLIST * ppidlSelect);
    HRESULT _NotifyInterval(HSFINTERVAL *pInterval, LONG lEventID);
    IUrlHistoryPriv *_GetHistStg();
    HRESULT _EnsureHistStg();
    HRESULT _GetUserName(LPTSTR pszUserName, DWORD cchUserName);
    HRESULT _GetInfoTip(LPCITEMIDLIST pidl, DWORD dwFlags, WCHAR **ppwszTip);
    HRESULT _DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl);
    LPITEMIDLIST _HostPidl(LPCTSTR pszHostUrl, HSFINTERVAL *pInterval);
    DWORD    _SearchFlatCacheForUrl(LPCTSTR pszUrl, LPINTERNET_CACHE_ENTRY_INFO pcei, LPDWORD pdwBuffSize);
    
    HRESULT _ViewPidl_BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    HRESULT _ViewType_BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    HRESULT _ViewType_CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    int     _View_ContinueCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    HRESULT _DeleteUrlFromBucket(LPCTSTR pszPrefixedUrl);
    HRESULT _ViewType_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl);
    HRESULT _ViewBySite_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl);
    HRESULT _ViewType_NotifyUpdateAll();
    HRESULT _ViewType_GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut);
    HRESULT _DeleteUrlHistoryGlobal(LPCTSTR pszUrl);
    DWORD   _GetHitCount(LPCTSTR pszUrl);
    LPHEIPIDL _CreateHCacheFolderPidlFromUrl(BOOL fOleMalloc, LPCTSTR pszPrefixedUrl);

    BOOL _IsLeaf();    
    
    LPCTSTR _GetLocalHost(void);

    LONG            _cRef;
    FOLDER_TYPE     _foldertype;
    TCHAR           *_pszCachePrefix;
    TCHAR           *_pszDomain;

    DWORD           _dwIntervalCached;
    FILETIME        _ftDayCached;
    int             _cbIntervals;
    HSFINTERVAL     *_pIntervalCache;
    BOOL            _fValidatingCache;

    UINT            _uFlags;    // copied from CacheFolder struct
    LPITEMIDLIST    _pidl;      // copied from CacheFolder struct
    LPITEMIDLIST    _pidlRest;  // suffix of _pidl
    IUrlHistoryPriv *_pUrlHistStg;  // used to get extended properties of history leafs

    UINT            _uViewType; // if this shell folder is implementing a special view
    UINT            _uViewDepth; // depth of the pidl

    const static DWORD    _rdwFlagsTable[];

    TCHAR   _szLocalHost[INTERNET_MAX_HOST_NAME_LENGTH]; //  "My Computer"  cached...

    class _CurrentSearches *_pcsCurrentSearch; // for CacheSearches
};

// called by our class factory
STDAPI  CHistCacheFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut, FOLDER_TYPE FolderType);


////////////////////////////////////////////////////////////////////////////
//
// CHistCacheItem Object
//
////////////////////////////////////////////////////////////////////////////

class CHistCacheItem : public IContextMenu, 
                       public IDataObject,
                       public IExtractIconA,
                       public IExtractIconW,
                       public IQueryInfo
{
    // CHistCacheItem interfaces
    friend HRESULT HistCacheFolderView_DidDragDrop(IDataObject *pdo, DWORD dwEffect);

public:
    CHistCacheItem();
    HRESULT Initalize(CHistCacheFolder *pHCFolder, HWND hwndOwner, UINT cidl, LPCITEMIDLIST *ppidl);

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IContextMenu Methods
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                  UINT idCmdLast, UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved,
                                  LPSTR pszName, UINT cchMax);
    

    // IQueryInfo Methods
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);
    
    // IExtractIconA Methods
    STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);
    STDMETHODIMP Extract(LPCSTR pcszFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);

    // IExtractIconW Methods
    STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pwzIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);
    STDMETHODIMP Extract(LPCWSTR pcwzFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);

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
    HRESULT _CreateHTEXT(STGMEDIUM *pmedium);
    HRESULT _CreateUnicodeTEXT(STGMEDIUM *pmedium);
    HRESULT _CreateHDROP(STGMEDIUM *pmedium);
    HRESULT _CreateFileDescriptorA(STGMEDIUM *pSTM);
    HRESULT _CreateFileDescriptorW(STGMEDIUM *pSTM);
    HRESULT _CreateFileContents(STGMEDIUM *pSTM, LONG lindex);
    HRESULT _CreateURL(STGMEDIUM *pSTM);
    HRESULT _CreatePrefDropEffect(STGMEDIUM *pSTM);

   
protected:

    ~CHistCacheItem();
    LPCTSTR _GetUrl(int nIndex);
    BOOL _ZoneCheck(int nIndex, DWORD dwUrlAction);

    LONG              _cRef;        // reference count
    FOLDER_TYPE       _foldertype;  // are we a history item or cache item
    CHistCacheFolder* _pHCFolder;   // back pointer to our shell folder
    
    UINT    _cItems;                // number of items we represent
    LPCEIPIDL*  _ppcei;             // variable size array of items
    HWND    _hwndOwner;     
    DWORD   _dwDelCookie;

private:
    HRESULT _AddToFavorites(int nIndex);    
};

//////////////////////////////////////////////////////////////////////
//  StrHash -- A generic string hasher
//             Stores (char*, void*) pairs
//  Marc Miller (t-marcmi), 1998

/*
 * TODO:
 *    provide a way to update/delete entries
 *    provice a way to specify a beginning table size
 *    provide a way to pass in a destructor function
 *      for void* values
 */
class StrHash {
public:
    StrHash(int fCaseInsensitive = 0);
    ~StrHash();
    void* insertUnique(LPCTSTR pszKey, int fCopy, void* pvVal);    
    void* retrieve(LPCTSTR pszKey);
#ifdef DEBUG
    void _RemoveHashNodesFromMemList();
    void _AddHashNodesFromMemList();
#endif // DEBUG
protected:
    class StrHashNode {
        friend class StrHash;
    protected:
        LPCTSTR pszKey;
        void*   pvVal;
        int     fCopy;
        StrHashNode* next;
    public:
        StrHashNode(LPCTSTR psz, void* pv, int fCopy, StrHashNode* next);
        ~StrHashNode();
    };
    // possible hash-table sizes, chosen from primes not close to powers of 2
    static const unsigned int   sc_auPrimes[];
    static const unsigned int   c_uNumPrimes;
    static const unsigned int   c_uFirstPrime;
    static const unsigned int   c_uMaxLoadFactor; // scaled by USHORT_MAX

    unsigned int nCurPrime; // current index into sc_auPrimes
    unsigned int nBuckets;
    unsigned int nElements;
    StrHashNode** ppshnHashChain;

    int _fCaseInsensitive;
    
    unsigned int        _hashValue(LPCTSTR, unsigned int);
    StrHashNode*        _findKey(LPCTSTR pszStr, unsigned int ubucketNum);
    unsigned int        _loadFactor();
    int                 _prepareForInsert();
private:
    // empty private copy constructor to prevent copying
    StrHash(const StrHash& strHash) { }
    // empty private assignment constructor to prevent assignment
    StrHash& operator=(const StrHash& strHash) { return *this; }
};

//////////////////////////////////////////////////////////////////////
/// OrderedList
class OrderedList {
public:
    class Element {
    public:
        friend  class OrderedList;
        virtual int   compareWith(Element *pelt) = 0;

        virtual ~Element() { }
    private:
        Element* next;
    };
    OrderedList(unsigned int uSize);
    ~OrderedList();
#if DEBUG
	void _RemoveElementsFromMemlist();
	void _AddElementsToMemlist();
#endif //DEBUg
    
    void     insert(Element *pelt);
    Element *removeFirst();
    Element *peek() { return peltHead; }

protected:
    Element       *peltHead; // points to smallest in list
    unsigned int   uSize;
    unsigned int   uCount;

public:
    // variable access functions
    unsigned int count() { return uCount; }
    BOOL         full()  { return (uSize && (uCount >= uSize)); }
private:
    OrderedList(const OrderedList& ol) { }
    OrderedList& operator=(const OrderedList& ol) { return *this; }
};


#endif // __cplusplus

#endif
