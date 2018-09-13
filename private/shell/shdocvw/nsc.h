#ifndef _NSC_C
#define _NSC_C


#include "droptgt.h"
#include "iface.h"
#include "dpastuff.h"
#include "cwndproc.h"
#include "resource.h"
#include "inetnot.h"
#include "cowsite.h"
#include <shlobj.h>
#include <cfdefs.h> // LPCOBJECTINFO


#define ID_CONTROL  100            

typedef enum
{
    NSIF_HITEM              = 0x0001,
    NSIF_FOLDER             = 0x0002,
    NSIF_PARENTFOLDER       = 0x0004,
    NSIF_IDLIST             = 0x0008,
    NSIF_FULLIDLIST         = 0x0010,
    NSIF_ATTRIBUTES         = 0x0020
} NSI_FLAGS;

typedef enum
{
    NSSR_ENUMBELOWROOT  = 0x0001,
    NSSR_CREATEPIDL     = 0x0002
} NSSR_FLAGS;

typedef struct
{
    PORDERITEM  poi;
    BITBOOL     fPinned:1;      // is this url pinned in the cache?
    BITBOOL     fGreyed:1;      // draw the item greyed (if offline & not in cache)
    BITBOOL     fFetched:1;     // have we fetched the pinned/greyed state?
    BITBOOL     fDontRefetch:1; // can't be cached by wininet
    BOOL        fNavigable:1;   // item can be navigated to
} ITEMINFO;

// _FrameTrack flags
#define TRACKHOT        0x0001
#define TRACKEXPAND     0x0002
#define TRACKNOCHILD    0x0004

// _DrawItem flags
#define DIICON          0x0001
#define DIHOT           0x0004
#define DIFIRST         0x0020
#define DISUBITEM       0x0040
#define DILAST          0x0080
#define DISUBLAST       (DISUBITEM | DILAST)
#define DIACTIVEBORDER  0x0100
#define DISUBFIRST      (DISUBITEM | DIFIRST)
#define DIPINNED        0x0400                  // overlay pinned glyph
#define DIGREYED        0x0800                  // draw greyed
#define DIFOLDEROPEN    0x1000      
#define DIFOLDER        0x2000      //item is a folder
#define DIFOCUSRECT     0x4000

#define NSC_TVIS_MARKED 0x1000

// async icon/url extract flags
#define NSCICON_GREYED      0x0001
#define NSCICON_PINNED      0x0002
#define NSCICON_DONTREFETCH 0x0004

#define WM_NSCUPDATEICONINFO   WM_USER + 0x700

HRESULT GetNavTargetName(IShellFolder* pFolder, LPCITEMIDLIST pidl, LPTSTR pszUrl, UINT cMaxChars);
BOOL    MayBeUnavailableOffline(LPTSTR pszUrl);
INSCTree * CNscTree_CreateInstance(void);

STDAPI CNscTree_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);

class   CNSCBand;
class   CHistBand;
// class wrapper for tree control component of nscband.
class CNscTree :    public IShellChangeNotify, 
                    public CDelegateDropTarget, 
                    public CNotifySubclassWndProc, 
                    public CObjectWithSite, 
                    public INSCTree, 
                    public IShellFavoritesNameSpace, 
                    public IWinEventHandler, 
                    public IShellBrowser,
                    public IShellFolderFilterSite
{

        // Hopefully, this is a temporary kludge:  (for CHistBand)
    friend class CNSCBand; // we need to be able to easily refresh the tree
    friend class CHistBand;
    
    friend INSCTree * CNscTree_CreateInstance(void);
    
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //*** IObjectWithSite ***
    virtual STDMETHODIMP SetSite(IUnknown *punkSite);
//    virtual STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

    // *** INSCTree methods ***
    virtual STDMETHODIMP CreateTree(HWND hwndParent, DWORD dwStyles, HWND *phwnd);         // create window of tree view.
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidlRoot, DWORD grfFlags, DWORD dwFlags);           // init the treeview control with data.
    virtual STDMETHODIMP ShowWindow(BOOL fShow);
    virtual STDMETHODIMP Refresh(void);
    virtual STDMETHODIMP GetSelectedItem(LPITEMIDLIST * ppidl, int nItem);
    virtual STDMETHODIMP SetSelectedItem(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert, int nItem);
    virtual STDMETHODIMP GetNscMode(UINT * pnMode) { *pnMode = _mode; return S_OK;};
    virtual STDMETHODIMP SetNscMode(UINT nMode) { _mode = nMode; return S_OK;};
    virtual STDMETHODIMP GetSelectedItemName(LPWSTR pszName, DWORD cchName);
    virtual STDMETHODIMP BindToSelectedItemParent(REFIID riid, void **ppv, LPITEMIDLIST *ppidl);
    virtual STDMETHODIMP_(BOOL) InLabelEdit(void) {return _fInLabelEdit;};

    // IShellBrowser (Hack)
    virtual STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) {return E_NOTIMPL;};
    virtual STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenu, HWND hwnd) {return E_NOTIMPL;};
    virtual STDMETHODIMP RemoveMenusSB(HMENU hmenuShared) {return E_NOTIMPL;};
    virtual STDMETHODIMP SetStatusTextSB(LPCOLESTR lpszStatusText) {return E_NOTIMPL;};
    virtual STDMETHODIMP EnableModelessSB(BOOL fEnable) {return E_NOTIMPL;};
    virtual STDMETHODIMP TranslateAcceleratorSB(LPMSG lpmsg, WORD wID) {return E_NOTIMPL;};
    virtual STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetViewStateStream(DWORD grfMode, LPSTREAM  *ppStrm) {return E_NOTIMPL; };
    virtual STDMETHODIMP GetControlWindow(UINT id, HWND * lphwnd) {return E_NOTIMPL;};
    virtual STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret) {return E_NOTIMPL;};
    virtual STDMETHODIMP QueryActiveShellView(struct IShellView ** ppshv) {return E_NOTIMPL;};
    virtual STDMETHODIMP OnViewWindowActive(struct IShellView * ppshv) {return E_NOTIMPL;};
    virtual STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return E_NOTIMPL;};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) {return E_NOTIMPL;};

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd) {return E_NOTIMPL;};

    // *** IShellChangeNotify methods ***
    STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // *** IShellFavoritesNameSpace methods ***
    virtual STDMETHODIMP get_FOfflinePackInstalled(/*[out, retval]*/ VARIANT_BOOL *pVal) {return E_NOTIMPL;};
    virtual STDMETHODIMP Import(void) {return DoImportOrExport(TRUE);};
    virtual STDMETHODIMP Export(void) {return DoImportOrExport(FALSE);};
    virtual STDMETHODIMP Synchronize(void) {return E_NOTIMPL;};
    virtual STDMETHODIMP NewFolder(void);
    virtual STDMETHODIMP ResetSort(void);
    virtual STDMETHODIMP MoveSelectionDown(void) {MoveItemUpOrDown(FALSE); return S_OK;};
    virtual STDMETHODIMP MoveSelectionUp(void) {MoveItemUpOrDown(TRUE); return S_OK;};
    virtual STDMETHODIMP InvokeContextMenuCommand(BSTR strCommand);
    virtual STDMETHODIMP MoveSelectionTo(void);
    virtual STDMETHODIMP CreateSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool);
    virtual STDMETHODIMP DeleteSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool);
    virtual STDMETHODIMP SetRoot(BSTR bstrFullPath) {return E_NOTIMPL;};

    // We aren't really a dual interface, but we play one on tv.
    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) {return E_NOTIMPL;};
    virtual STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) {return E_NOTIMPL;};

    // *** CDelegateDropTarget methods ***
    virtual HRESULT GetWindowsDDT(HWND * phwndLock, HWND * phwndScroll);
    virtual HRESULT HitTestDDT(UINT nEvent, LPPOINT ppt, DWORD * pdwId, DWORD * pdwDropEffect);
    virtual HRESULT GetObjectDDT(DWORD dwId, REFIID riid, LPVOID * ppvObj);
    virtual HRESULT OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IShellFolderFilterSite methods ***
    virtual STDMETHODIMP SetFilter(IUnknown* punk);

    CNscTree();

protected:
    ~CNscTree();

    class CSelectionContextMenu : public IContextMenu2
    {
        friend CNscTree;
    protected:
        // *** IUnknown methods ***
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void) ;
        STDMETHODIMP_(ULONG) Release(void);

        // *** IContextMenu methods ***
        STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);
        STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
        STDMETHOD(GetCommandString)(UINT_PTR     idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) { return E_NOTIMPL; };
        // *** IContextMenu2 methods ***
        STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);


    protected:
        ~CSelectionContextMenu();
        IContextMenu *_QuerySelection();

        IContextMenu *_pcmSelection;
        IContextMenu2 *_pcm2Selection;
        ULONG         _ulRefs;
    public:
        CSelectionContextMenu() : _pcmSelection(NULL),_ulRefs(0) {}
    };

    friend class CSelectionContextMenu;
    CSelectionContextMenu _scm;

private:
    HRESULT _GetEnum(IShellFolder *psf, LPCITEMIDLIST pidlFolder, IEnumIDList **ppenum);
    BOOL _ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem);
    HWND _CreateTreeview();
    void _SubClass(LPCITEMIDLIST pidlRoot);
    void _UnSubClass(void);
    int _GetChildren(IShellFolder *psf, LPCITEMIDLIST pidl, ULONG ulAttrs);
    HRESULT _LoadSF(HTREEITEM htiRoot, LPCITEMIDLIST pidl, BOOL fAddingOnly, int * pcAdded, BOOL * pfOrdered);
    void _GetDefaultIconIndex(LPCITEMIDLIST pidl, ULONG ulAttrs, TVITEM *pitem, BOOL fFolder);
    LRESULT _OnEndLabelEdit(TV_DISPINFO *ptvdi);
    LRESULT _OnBeginLabelEdit(TV_DISPINFO *ptvdi);
    LPITEMIDLIST _CacheParentShellFolder(HTREEITEM hti, LPITEMIDLIST pidl);
    BOOL _CacheShellFolder(HTREEITEM hti);
    void _CacheDetails(IShellFolder *psf);
    void _ReleaseRootFolder(void );
    void _ReleasePidls(void);
    void _ReleaseCachedShellFolder(void);
    void _TvOnHide();
    void _TvOnShow();
    BOOL _ShouldAdd(LPCITEMIDLIST pidl);
    void _ReorderChildren(HTREEITEM htiParent);
    void _Sort(HTREEITEM hti, IShellFolder *psf);
    void MoveItemUpOrDown(BOOL fUp);
    HRESULT CreateNewFolder(HTREEITEM hti);
    BOOL MoveItemsIntoFolder(HWND hwndParent);
    HRESULT DoImportOrExport(BOOL fImport);
    HRESULT DoSubscriptionForSelection(BOOL fCreate);
    LRESULT _OnNotify(LPNMHDR pnm);
    HRESULT _OnPaletteChanged(WPARAM wPAram, LPARAM lParam);
    HRESULT _OnWindowCleanup(void);
    HRESULT _HandleWinIniChange(void);
    HRESULT _EnterNewFolderEditMode(LPCITEMIDLIST pidlNewFolder);
    HTREEITEM _AddItemToTree(HTREEITEM htiParent, LPCITEMIDLIST pidl, int cChildren, int iPos, HTREEITEM htiAfter = TVI_LAST
                        , BOOL fCheckForDups = TRUE, BOOL fMarked = FALSE);
    HTREEITEM _FindChild(IShellFolder *psf, HTREEITEM htiParent, LPCITEMIDLIST pidlChild);
    LPITEMIDLIST _GetFullIDList(HTREEITEM hti);
    ITEMINFO *_GetTreeItemInfo(HTREEITEM hti);
    PORDERITEM _GetTreeOrderItem(HTREEITEM hti);
    BOOL _SetRoot(LPCITEMIDLIST pidlRoot, int iExpandDepth, LPCITEMIDLIST pidlExpandTo, NSSR_FLAGS flags);
    void _OnGetInfoTip(NMTVGETINFOTIP *pnm);
    LRESULT _OnSetCursor(NMMOUSE* pnm);
    void _ApplyCmd(HTREEITEM hti, IContextMenu *pcm, UINT cmdId);
    HRESULT _QuerySelection(IContextMenu **ppcm, HTREEITEM *phti);
    HMENU   _CreateContextMenu(IContextMenu *pcm, HTREEITEM hti);
    LRESULT _OnContextMenu(short x, short y);
    void _OnBeginDrag(NM_TREEVIEW *pnmhdr);
    void _OnChangeNotify(LONG lEvent, LPCITEMIDLIST pidl
                                , LPCITEMIDLIST pidlExtra);
    HRESULT _OnDeleteItem(NM_TREEVIEW *pnm);
    void _OnGetDisplayInfo(TV_DISPINFO *pnm);
    HRESULT _ChangePidlRoot(LPCITEMIDLIST pidl);
    BOOL _IsExpandable(HTREEITEM hti);
    BOOL _OnItemExpandingMsg(NM_TREEVIEW *pnm);
    BOOL _OnItemExpanding(HTREEITEM htiToActivate, UINT action, BOOL fExpandedOnce);
    BOOL _OnSelChange();
    BOOL _FIsItem(IShellFolder * psf, LPCITEMIDLIST pidlTarget, HTREEITEM hti);
    HTREEITEM _FindFromRoot(HTREEITEM htiRoot, LPCITEMIDLIST pidl);
    HRESULT _OnSHNotifyRename(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNew);
    HRESULT _OnSHNotifyDelete(LPCITEMIDLIST pidl, int *piPosDeleted, HTREEITEM *phtiParent);
    void _OnSHNotifyUpdateItem(LPCITEMIDLIST pidl);
    HRESULT _OnSHNotifyUpdateDir(LPCITEMIDLIST pidl);
    HRESULT _OnSHNotifyCreate(LPCITEMIDLIST pidl, int iPosition, HTREEITEM htiParent);
    void _OnSHNotifyOnlineChange(HTREEITEM htiRoot, BOOL fGoingOnline);
    void _OnSHNotifyCacheChange(HTREEITEM htiRoot, DWORD_PTR dwChanged);

    HRESULT _IdlRealFromIdlSimple( IShellFolder * psf
                                 , LPCITEMIDLIST pidlSimple
                                 , LPITEMIDLIST * ppidlReal);
    void _DtRevoke();
    void _DtRegister();
    BOOL _FInTree(HDPA hdpa, int celt, HTREEITEM hti);
    BOOL _IsItemExpanded(HTREEITEM hti);
    HRESULT _UpdateDir(HTREEITEM hti);

    HRESULT _GetDisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, 
                              DWORD uFlags, LPSHELLDETAILS pdetails);
    HRESULT _ParentFromItem(LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild);
    HRESULT _CompareIDs(IShellFolder *psf, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);
    static int CALLBACK _TreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    static LRESULT CALLBACK _SubClassTreeWndProc(
                                  HWND hwnd, UINT uMsg, 
                                  WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

protected:
    //used for background thread icon + draw info extraction
    static void s_NscIconCallback(LPVOID pvData, UINT_PTR uId, int iIcon, int iOpenIcon, DWORD dwFlags, UINT uMagic);
    
private:

#ifdef DEBUG
    void TraceHTREE(HTREEITEM hti, LPCTSTR pszDebugMsg);
    void TracePIDL(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg);
    void TracePIDLAbs(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg);
#endif

    static int CALLBACK _TreeOrder(LPARAM lParam1, LPARAM lParam2
                                            , LPARAM lParamSort);
    BOOL _IsOrdered(HTREEITEM htiRoot);
    void _SelectPidl(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert = FALSE);
    HRESULT _InsertChild(HTREEITEM htiParent, IShellFolder *psfParent, LPCITEMIDLIST pidlChild, BOOL fExpand, int iPosition);
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);

    IStream *GetOrderStream(LPCITEMIDLIST pidl, DWORD grfMode);
    HRESULT _PopulateOrderList(HTREEITEM htiRoot);
    void _FreeOrderList(HTREEITEM htiRoot);

    void _Dropped(void);

    LRESULT _OnCDNotify(LPNMCUSTOMDRAW pnm);
    BOOL _IsTopParentItem(HTREEITEM hti);
    BOOL _MoveNode(HWND _hwndTree, int _iDragSrc, int iNewPos, LPCITEMIDLIST pidl);
    void _TreeInvalidateItemInfo(HTREEITEM hItem, UINT mask);
    void _InvalidateImageIndex(HTREEITEM hItem, int iImage);

    void _DrawItem(HTREEITEM hti, TCHAR * psz, HDC hdc, LPRECT prc
            , DWORD dwFlags, int iLevel, COLORREF clrbk, COLORREF clrtxt);
    void _DrawIcon(HTREEITEM hti,HDC hdc, int iLevel, RECT *prc, DWORD dwFlags);
    void _DrawActiveBorder(HDC hdc, LPRECT prc);

    void _UpdateActiveBorder(HTREEITEM htiSelected);
    void _MarkChildren(HTREEITEM htiParent, BOOL fOn);
    BOOL _IsMarked(HTREEITEM hti);

    void _UpdateItemDisplayInfo(HTREEITEM hti);
    void _TreeSetItemState(HTREEITEM hti, UINT stateMask, UINT state);
    void _TreeNukeCutState();
    BOOL _IsChannelFolder(HTREEITEM hti);

    BOOL IsExpandableChannelFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    BOOL _LoadOrder(HTREEITEM hti, LPCITEMIDLIST pidl, IShellFolder* psf, HDPA* phdpa);


    LONG                _cRef;
    HWND                _hwndParent;            // parent window to notify
    HWND                _hwndTree;              // tree or combo box
    HWND                _hwndNextViewer;
    DWORD               _style;
    DWORD               _grfFlags;              // Flags to filter what goes in the tree.
    DWORD               _dwFlags;               // Behavior Flags (NSS_*)
    BITBOOL             _fInitialized : 1;      // Has INSCTree::Initialize() been called at least once yet?
    BITBOOL             _fIsSelectionCached: 1; // If the WM_NCDESTROY has been processed, then we squired the selected pidl(s) in _pidlSelected
    BITBOOL             _fCacheIsDesktop : 1;   // state flags
    BITBOOL             _fAutoExpanding : 1;    // tree is auto-expanding
    BITBOOL             _fDTRegistered:1;       // have we registered as droptarget?
    BITBOOL             _fpsfCacheIsTopLevel : 1;   // is the cached psf a root channel ?
    BITBOOL             _fDragging : 1;         // one o items being dragged
    BITBOOL             _fStartingDrag : 1;     // starting to drag an item
    BITBOOL             _fDropping : 1;         // a drop occurred in the nsc
    BITBOOL             _fInSelectPidl : 1;     // we are performing a SelectPidl
    BITBOOL             _fInserting : 1;        // we're on the insertion edge.
    BITBOOL             _fInsertBefore : 1;     // a drop occurred in the nsc
    BITBOOL             _fClosing : 1;          // are we closing ?
    BITBOOL             _fRclick : 1;           // are we right clicking.
    BITBOOL             _fInLabelEdit:1;
    BITBOOL             _fCollapsing:1;         // is a node collapsing.
    BITBOOL             _fRefreshing:1;         // refreshing now.
    BITBOOL             _fOnline:1;             // is inet online?
    BITBOOL             _fWeChangedOrder:1;     // did we change the order?
    BITBOOL             _fHandlingShellNotification:1; //are we handing a shell notification?
    BITBOOL             _fSingleExpand:1;       // are we in single expand mode
    BITBOOL             _fHasFocus:1;           // does nsc have the focus?
    BITBOOL             _fIgnoreNextSelChange:1;// hack to get around treeview keydown bug
    BITBOOL             _fIgnoreNextItemExpanding:1; //hack to get around annoying single expand behavior
    BITBOOL             _fSubClassed:1;         // Have we subclassed the window yet?
    BITBOOL             _fAsyncDrop:1;          // async drop from outside or another inside folder.
    BITBOOL             _fOrdered:1;              // is root folder ordered.
    int                 _cxOldWidth;
    UINT                _mode;
    IContextMenu*       _pcm;                  // context menu currently being displayed
    IContextMenu2*      _pcmSendTo;            // deal with send to hack so messages tgo right place.
    LPITEMIDLIST        _pidlRoot;
    LPITEMIDLIST        _pidlSelected;          // Valid if _fIsSelectionCached is true.  Used for INSCTree::GetSelectedItem() after tree is gone.
    HTREEITEM           _htiCache;              // tree item associated with Current shell folder
    IShellFolder*       _psfCache;             // cache of the last IShellFolder I needed...
    IShellFolder2*      _psf2Cache;             // IShellDetails2 for _psfISD2Cache
    IShellFolderFilter* _pFilter;    
    ULONG               _ulDisplayCol;          // Default display col for _psfCache
    ULONG               _ulSortCol;             // Default sort col for _psfCache
    ULONG               _nChangeNotifyID;       // SHChangeNotify registration ID
    HDPA                _hdpaOrd;               // dpa order for current shell folder.
// drop target privates
    HTREEITEM           _htiCur;                // current tree item (dragging over)
    DWORD               _dwLastTime;
    DWORD               _dwEffectCur;           // current drag effect
    int                 _iDragSrc;              // dragging from this pos.
    int                 _iDragDest;             // destination drag point
    HTREEITEM           _htiDropInsert;         // parent of inserted item.
    HTREEITEM           _htiDragging;           // the tree item being dragged during D&D.
    HTREEITEM           _htiCut;                // Used for Clipboard and Visuals    
    LPITEMIDLIST        _pidlDrag;              // pidl of item being dragged from within.
    HTREEITEM           _htiFolderStart;        // what folder do we start in.
    HICON               _hicoPinned;            // drawn over items that are sticky in the inet cache
    HWND                _hwndDD;                // window to draw custom drag cursors on.
    HTREEITEM           _htiActiveBorder;       // the folder to draw the active border around
    CWinInetNotify      _inetNotify;            // hooks up wininet notifications (online/offline, etc)
    IShellTaskScheduler* _pTaskScheduler;       // background task icon/info extracter
    int                 _iDefaultFavoriteIcon;  // index of default favorite icon in system image list
    int                 _iDefaultFolderIcon;    // index of default folder icon in system image list
    HTREEITEM           _htiRenaming;           // hti of item being renamed in rename mode
    LPITEMIDLIST        _pidlNewFolderParent;   // where the new folder will be arriving (when user picks "Create New Folder")


    BYTE                _bSynchId;              // magic number for validating tree during background icon extraction

    enum {
        idCmdStart    = RSVIDM_LAST + 1,        // our private menu items end here
    };

};

// util macros.

#define GetPoi(p)   (((ITEMINFO *)p)->poi)
#define GetPii(p)   ((ITEMINFO *)p)

#include "nscband.h"

#endif  // _NSC_C
