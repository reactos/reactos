// contains c++ declarations for CDFFolder and CDFBrowse (and CDFMenuWrap)
// included by docfind.cpp and docfindx.cpp

#include "sfviewp.h"
#include "dspsprt.h"
#include "adoint.h"
#include "perhist.h"

#include "docfind.h"
#include "cowsite.h"
#include "defvphst.h"

//===========================================================================
// Forward definition of types and functions.
//===========================================================================
typedef class CDFFolder * LPDFFOLDER;
typedef class CDFBrowse * LPDFBROWSE;

//===========================================================================
// Each find thread that gets spawned will have an assoc. FINDTHREAD struct
//===========================================================================
typedef struct _findthread {
    LPDFBROWSE  pdfb;       // Ptr back to spawning doc find browse info
    BOOL        fContinue;  // Termination signal      
    int         iColSort;   // For Rowset queries we need to pass this along...
    UINT        iSearchCnt;
} FINDTHREAD, *LPFINDTHREAD;

typedef struct _DFPIDLList
{
    struct _DFPIDLList *pdfplNext;
    LPITEMIDLIST pidl;
} DFPIDLLIST, *LPDFPIDLLIST;

struct IDocFindFolder;
typedef struct _DFInit     // dfi
{
    IDocFindFileFilter      *pdfff;        // The file filter to use...
    LPITEMIDLIST            pidlStart;              // The idlist to start at
    LPITEMIDLIST            pidlSaveFile;           // The file that has data in it.
} DFINIT, *LPDFINIT;

//===========================================================================
// CDDocFindLVRange class definition
//===========================================================================
class CDocFindLVRange : public ILVRange
{
public:
    // IUnknown stuff...
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // ILVRange methods
    STDMETHODIMP IncludeRange(LONG iBegin, LONG iEnd);
    STDMETHODIMP ExcludeRange(LONG iBegin, LONG iEnd);    
    STDMETHODIMP InvertRange(LONG iBegin, LONG iEnd);
    STDMETHODIMP InsertItem(LONG iItem);
    STDMETHODIMP RemoveItem(LONG iItem);

    STDMETHODIMP Clear();
    STDMETHODIMP IsSelected(LONG iItem);
    STDMETHODIMP IsEmpty();
    STDMETHODIMP NextSelected(LONG iItem, LONG *piItem);
    STDMETHODIMP NextUnSelected(LONG iItem, LONG *piItem);
    STDMETHODIMP CountIncluded(LONG *pcIncluded);

    // Helperfunctions...
    void SetOwner(CDFFolder *pdff, DWORD dwMask)
        {
            // don't AddRef -- we're a member variable of the object punk points to
            m_pdff = pdff;
            m_dwMask = dwMask;
            m_cIncluded = 0;
        }
    void IncrementIncludedCount() 
        {m_cIncluded++;}
    void DecrementIncludedCount()
        {m_cIncluded--;}
protected:
    CDFFolder *m_pdff;  // IUnknown of object containing us
    DWORD     m_dwMask;  // The mask we use to know which "selection" bit we are tracking...
    LONG      m_cIncluded;  // count included... (selected)
};

//===========================================================================
// CDFFolder class definition
//===========================================================================

class CDFFolder : public IDocFindFolder,
                  public IShellFolder2,
                  public IShellIcon,
                  public IShellIconOverlay,
                  public IPersistFolder2
{

public:
    // constructor
    CDFFolder();
    ~CDFFolder();
    
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID,void **);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
        
    // *** IShellFolder methods ***
    STDMETHOD(ParseDisplayName)(HWND hwndOwner,
                                LPBC pbcReserved, LPOLESTR lpszDisplayName,
                                ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

    STDMETHOD(EnumObjects)( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList);

    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID * ppvOut);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID * ppvObj)
        { *ppvObj=NULL; return E_NOTIMPL;}

    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl,
                                ULONG * rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHOD(SetNameOf)(HWND hwndOwner, LPCITEMIDLIST pidl,
                                LPCOLESTR lpszName, DWORD uFlags,
                                LPITEMIDLIST * ppidlOut);

    // *** IShellFolder2 methods ***
    STDMETHOD(GetDefaultSearchGUID)(LPGUID lpGuid);
    STDMETHOD(EnumSearches)(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid);

    // *** IDocFindFolder methods ***
    STDMETHOD(SetDocFindFilter)(IDocFindFileFilter *pdfff);
    STDMETHOD(GetDocFindFilter)(IDocFindFileFilter **pdfff);

    STDMETHOD(AddPidl)(int i, LPITEMIDLIST pidl, DWORD dwItemID, ESFItem **ppItem);
    STDMETHOD(GetItem)(int iItem, ESFItem **ppItem);
    STDMETHOD(DeleteItem)(int iItem);
    STDMETHOD(GetItemCount)(INT *pcItems);
    STDMETHOD(ValidateItems)(int iItemFirst, int cItems, BOOL bSearchComplete);
    STDMETHOD(GetFolderListItemCount)(INT *pcCount);
    STDMETHOD(GetFolderListItem)(int iItem, DFFolderListItem **ppItem);
    STDMETHOD(SetItemsChangedSinceSort)();
    STDMETHOD(ClearItemList)();
    STDMETHOD(ClearFolderList)();
    STDMETHOD(AddFolderToFolderList)(LPITEMIDLIST pidl, BOOL fCheckForDup, int * piFolder);
    STDMETHOD(SetAsyncEnum)(IDFEnum *pdfEnumAsync);
    STDMETHOD(GetAsyncEnum)(IDFEnum **pdfEnumAsync);
    STDMETHOD(CacheAllAsyncItems)();
    STDMETHOD_(BOOL,AllAsyncItemsCached)();
    STDMETHOD(SetAsyncCount)(DBCOUNTITEM cCount);
    STDMETHOD(ClearSaveStateList)();
    STDMETHOD(GetStateFromSaveStateList)(DWORD dwItemID, DWORD *pdwState);
    STDMETHOD(MapFSPidlToDFPidl)(LPITEMIDLIST pidl, BOOL fMapToReal, LPITEMIDLIST *ppidl);
    STDMETHOD(GetParentsPIDL)(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlParent);
    STDMETHOD(SetControllerNotifyObject)(IDocFindControllerNotify *pdfcn);
    STDMETHOD(GetControllerNotifyObject)(IDocFindControllerNotify **ppdfcn);
    STDMETHOD(RememberSelectedItems)();
    STDMETHOD(SaveFolderList)(IStream *pstm);
    STDMETHOD(RestoreFolderList)(IStream *pstm);
    STDMETHOD(SaveItemList)(IStream *pstm);
    STDMETHOD(RestoreItemList)(IStream *pstm, int *pcItems);
    STDMETHOD(RestoreSearchFromSaveFile)(LPITEMIDLIST pidlSaveFile, IShellFolderView *psfv);
    STDMETHOD(SetEmptyText)(LPCTSTR pszText);
    STDMETHOD_(BOOL,IsSlow)();

    // *** IShellIcon methods ***
    STDMETHOD(GetIconOf)(LPCITEMIDLIST pidl, UINT flags, int *piIndex);

    // *** IShellIconOverlay methods ***
    STDMETHOD(GetOverlayIndex)(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHOD(GetOverlayIconIndex)(LPCITEMIDLIST pidl, int * pIndex);
  
    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // *** IPersistFolder methods ***
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // *** IPersistFolder2 methods ***
    STDMETHOD(GetCurFolder)(LPITEMIDLIST *ppidl);

public:
    

    friend HRESULT CDFFolder_Create(LPVOID * ppvOut, IDocFindFileFilter *pdfff);

    friend class CDocFindSFVCB; // CPPBUG: t-saml: legit???
    friend class CDFOleDBEnum;  // ""
    friend class CDFShellDetails;
    friend class CDocFindLVRange;
protected:

private:
    // Objects to manage selection and cut range information...
    CDocFindLVRange _dflvrSel;
    CDocFindLVRange _dflvrCut;

private:

    // Various helper functions
    HRESULT _CompareFolderIndexes(int iFolder1, int iFolder2);
    void _AddESFItemToSaveStateList(ESFItem *pesfi);
    LPITEMIDLIST _GetFullPidlForItem(LPCITEMIDLIST pidl);
    HRESULT _UpdateItemList();

    HRESULT _QueryItemInterface(LPCITEMIDLIST pidl, REFIID riid, void **ppv);

    // variables

    UINT                _cRef;
    int                 cbItem;     // Sizeof each item.
    // Should really make so its not public, but for now...
    HDPA                hdpaItems;  // Complete list of dpas...
    HDPA                hdpaPidf;   // DPA of Folder list items.
    IDocFindFileFilter  *pdfff; // Pointer to the FindFileFIlter interface
    BOOL                fItemsChangedSinceSort;  // Has the list changed since the last sort?
    BOOL                _fAllAsyncItemsCached;  // Have we already cached all of the items?
    BOOL                _fSearchComplete; 
    BOOL                _fInRefresh;    // true if received prerefresh callback but postrefresh

    LPITEMIDLIST        _pidl;
    // Stuff added to support async query.  
    IDFEnum             *_pDFEnumAsync;     // we have an async one, will need to call back for PIDLS and the like
    DBCOUNTITEM         _cAsyncItems;       // Count of async items
    int                 _iGetIDList;        // index of the last IDlist we retrieved in callback.
    HWND                _hwndLV;            // BUGBUG: should remove use of this...
    HDSA                _hdsaSaveStateForIDs;    // Async - Remember which items are selected when we sort
    int                 _cSaveStateSelected;    // Number of items in selection list which are selected
    IDocFindControllerNotify *_pdfcn;       // Sometimes need to let the "Controller" object know about things
    TCHAR               _szEmptyText[128] ; // empty results list text.
    CRITICAL_SECTION    _csSearch;        // use this!

public:
};

// We will only enable the rowset code to work on NT machines.
#ifdef WINNT
#define SAFECAST_IROWSETTWATCHNOTIFY(pdfb) SAFECAST(pdfb,IRowsetWatchNotify*)
#else
#define SAFECAST_IROWSETTWATCHNOTIFY(pdfb) NULL
#endif

void DocFind_SetObjectCount(IShellView *psv, int cItems, DWORD dwFlags);

class CDocFindSFVCB : public CBaseShellFolderViewCB
{
public:
    CDocFindSFVCB(IShellFolder* psf, CDFFolder* pDFFolder)
        : CBaseShellFolderViewCB(psf, NULL, 0), m_pDFFolder(pDFFolder), m_fIgnoreSelChange(FALSE),
        m_iColSort((UINT)-1), m_iFocused((UINT)-1), m_cSelected(0)
        {
            if (m_pDFFolder)
                m_pDFFolder->AddRef();
        }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT SetShellView(IShellView *psv);

protected:
    virtual ~CDocFindSFVCB();

private:
    CDFFolder* m_pDFFolder;
    UINT       m_iColSort;      // Which column are we sorting by
    BOOL       m_fIgnoreSelChange;      // Sort in process
    UINT       m_iFocused;      // Which item has the focus?
    UINT       m_cSelected;     // Count of items selected

    friend class CDocFindLVRange;

private:
    HRESULT SortOnColumn(UINT wP);	// Called from callback function...
    HRESULT OnMERGEMENU(DWORD pv, QCMINFO*lP);    
    HRESULT OnReArrange(DWORD pv, LPARAM lp);
    HRESULT OnGETWORKINGDIR(DWORD pv, UINT wP, LPTSTR lP);
    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP);
    HRESULT OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream**lP);
    HRESULT OnGETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST *ppidl);
    HRESULT OnGetItemIconIndex(DWORD pv, WPARAM iItem, int *piIcon);
    HRESULT OnSetItemIconOverlay(DWORD pv, WPARAM iItem, int dwOverlayState);
    HRESULT OnGetItemIconOverlay(DWORD pv, WPARAM iItem, int * pdwOverlayState);
    HRESULT OnSETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST pidl);
    HRESULT OnGetIndexForItemIDList(DWORD pv, int * piItem, LPITEMIDLIST pidl);
    HRESULT OnDeleteItem(DWORD pv, LPCITEMIDLIST pidl);
    HRESULT OnODFindItem(DWORD pv, int * piItem, NM_FINDITEM* pnmfi);
    HRESULT OnODCacheHint(DWORD pv, NMLVCACHEHINT* pnmlvc);
    HRESULT OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP);
    HRESULT OnGetEmptyText(DWORD pv, UINT cchTextMax, LPTSTR pszText);
    HRESULT OnHwndMain(DWORD pv, HWND hwndMain);
    HRESULT OnIsOwnerData(DWORD pv, BOOL *pfIsOwnerData);
    HRESULT OnSetISFV(DWORD pv, IShellFolderView* pisfv);
    HRESULT OnWindowCreated(DWORD pv, HWND hwnd);
    HRESULT OnWindowDestroy(DWORD pv, HWND wP);
    HRESULT OnGetODRangeObject(DWORD pv, WPARAM wWhich, ILVRange **pplvr);
    HRESULT OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP);
    HRESULT OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj);
    HRESULT OnOverrideItemCount(DWORD pv, UINT*lP);
    HRESULT OnGetIPersistHistory(DWORD pv, IPersistHistory **ppph);
    HRESULT OnRefresh(DWORD pv, BOOL fPreRefresh);
    HRESULT OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA *shtd);
} ;

CDocFindSFVCB* DocFind_CreateSFVCB(IShellFolder* psf, LPDFFOLDER pDFFolder);

//----------------------------------------------------------------------
// Class to save and restore find state on the travel log 
class CDocFindPersistHistory : public CDefViewPersistHistory
{
public:
    CDocFindPersistHistory();

    // *** Override functions within the base class...

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // *** IPersistHistory methods ***
    STDMETHOD(LoadHistory)(IStream *pStream, IBindCtx *pbc);
    STDMETHOD(SaveHistory)(IStream *pStream);


protected:
    IDocFindFolder*     _GetDocFindFolder();

} ;

// Helper functions to share code between Old and new...
BOOL DFB_handleUpdateDir(IDocFindFolder *pdfFolder, LPITEMIDLIST pidl, BOOL fCheckSubDirs);
void DFB_handleRMDir(IDocFindFolder *pdfFolder, IShellFolderView *psfv, LPITEMIDLIST pidl);
void DFB_UpdateOrMaybeAddPidl(IDocFindFolder *pdfFolder, IShellFolderView *psfv, int code, 
        LPITEMIDLIST pidl, LPITEMIDLIST pidlOld);
typedef struct tagDFBSave {
    LPITEMIDLIST pidlSaveFile;  // [in, out] most recent pidl saved to
    DWORD dwFlags;              // [in, out] current flag state
    int SortMode;               // [in]      current sort mode
} DFBSAVEINFO;
void DFB_Save(IDocFindFileFilter* pdfff, HWND hwndOwner, DFBSAVEINFO * pSaveInfo,
              IShellView* psv, IDocFindFolder* pDocfindFolder, IUnknown * pObject);
void DFB_HandleOpenContainingFolder(IShellFolderView* psfv, IDocFindFolder* pdff);
