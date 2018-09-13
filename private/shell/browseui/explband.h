#ifndef _explband_H
#define _explband_H

#include "onetree.h"
#include "cwndproc.h"
#include "bands.h"

HRESULT CExplorerBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

//=============================================================================
// TreeDropTarget : Class definition
//=============================================================================
class TreeDropTarget: public IDropTarget
{
public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    virtual STDMETHODIMP DragEnter(
        IDataObject *pdtobj,
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect);

    virtual STDMETHODIMP DragOver(
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect);

    virtual STDMETHODIMP DragLeave(void);

    virtual STDMETHODIMP Drop(
        IDataObject *pdtobj,
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect);


    virtual ~TreeDropTarget();
    

protected:
    
    void _ReleaseDataObject();
    void _ReleaseCurrentDropTarget();
    
    RECT                _rcLockWindow;
    HTREEITEM           _htiCur;        // current tree item (dragging over)
    IDropTarget *       _pdtgtCur;      // current drop target
    IDataObject *       _pdtobjCur;     // current data object
    DWORD               _dwEffectCur;   // current drag effect
    DWORD               _dwEffectIn;    // *pdwEffect passed-in on last Move/Enter
    DWORD               _grfKeyState;   // cached key state
    AUTO_SCROLL_DATA    _asd;
    POINT               _ptLast;        // last dragged over position
    HWND                _hwndDD;        // draw drag cursors here.

    DWORD               _dwLastTime;
};

//=============================================================================
// CExpBandItemWatch: Class definition
//=============================================================================
//
//  Helper class for keeping an eye on HTREEITEMs while we yield.
//  Also has special methods to faciliate safe enumeration while yielding.
//
//  Usage for just watching:
//
//      CExpBandItemWatch w(htiWatchMe);
//      if (_AttachItemWatch(&w)) {
//          BlahBlahMightYield();
//          if (w.StillValid()) {
//              KeepUsingIt();
//          }
//          _DetachItemWatch(&w);
//      } else {
//          // Out of memory error
//      }
//
//  Usage for enumerator:
//
//      CExpBandItemWatch w(htiStart);
//      if (_AttachItemWatch(&w)) {
//          while (w.Item()) {
//              BlahBlah(w.Item(), ...);
//              FunctionThatYields();
//              if (w.StillValid()) {
//                  BlahBlah(w.Item(), ...);
//              }
//              w.NextItem(pebOwner->_hwnd);        // Step to next item
//          }
//          _DetachItemWatch(&w);
//      } else {
//          // Out of memory error
//      }
//
//  Here's how it works:
//
//  Each pending enumeration is tracked by a CExpBandItemWatch structure.
//
//  _hti = the HTREEITEM being enumerated, if it's still valid.
//         Otherwise, it is the item *after* the enumerated item.
//  _fStale = FALSE if the _hti is still valid.
//            TRUE if the _hti is really the item *after* the original item.
//

class CExplorerBand;

class CExpBandItemWatch
{
    public:

    inline void Restart(HTREEITEM htiStart) { _hti = htiStart; _fStale = FALSE; };
    inline CExpBandItemWatch(HTREEITEM htiStart) { Restart(htiStart); };

    inline BOOL StillValid() { return !_fStale; }
    inline HTREEITEM Item() { ASSERT(StillValid()); return _hti; }

    void NextItem(HWND hwndTV) {
        if (!this->_fStale) {
            _hti = TreeView_GetNextSibling(hwndTV, _hti);
        }
        _fStale = FALSE;
    }

#ifdef DEBUG
    inline ~CExpBandItemWatch() { ASSERT(!_fAttached); }
#endif

private:
    friend CExplorerBand;               // Let CExplorerBand party on our state
    HTREEITEM _hti;                     // current item
    BOOL _fStale;                       // was the original item stale?
#ifdef DEBUG
    BOOL _fAttached;                    // Are we attached to the CExpBand?
#endif
};

//=============================================================================
// CExplorerBand : Class definition
//=============================================================================

class CExplorerBand : public CToolBand
        , public IWinEventHandler
        , public IDispatch
        , public IShellChangeNotify
        , public CNotifySubclassWndProc
{
public:
    CExplorerBand();
    virtual ~CExplorerBand();

    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void); 
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);

    // *** IObjectWithSite methods ***
    // BUGBUG should we use CObjectWithSite default impl?
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    
    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);
    
    // *** IPersistStream methods ***
    // (others use base class implementation) 
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) { *pClassID = CLSID_ExplorerBand; return S_OK;};
    virtual STDMETHODIMP Load(IStream *pStm) {return S_OK;};
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) { return S_OK;};
    
    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent (HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plre);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) {return E_NOTIMPL;}
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) {return E_NOTIMPL;}
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) {return E_NOTIMPL;}
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                  DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr);

    // *** IShellChangeNotify methods ***
    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

protected:
    HWND _GetTreeHWND() {return _hwnd;}
    HRESULT _CreateTree();
    friend TreeDropTarget;
    TreeDropTarget  _tdt;    
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:

    static int CALLBACK _ItemCompare(void *p1, void *p2, LPARAM lParam);

    BOOL _InvokeCommandOnItem(LPCTSTR pszCmd);
    BOOL _FilterMouseWheel(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL _OnCommand(WPARAM wParam, LPARAM lParam);
    HRESULT _QueryStatusExplorer(ULONG cCmds, OLECMD rgCmds[]);

    // tree functions
    
    static void CALLBACK _TreeTimerProc(HWND hWnd, UINT uMessage, UINT_PTR wTimer, DWORD dwTime);

    LPOneTreeNode _TreeGetFCTreeData(HTREEITEM hItem);
    void _TreeSetItemState(HTREEITEM hti, UINT stateMask, UINT state);
    void _TreeNukeCutState();
    LPSHELLFOLDER _TreeBindToFolder(HTREEITEM hItem);
    LPSHELLFOLDER _TreeBindToParentFolder(HTREEITEM hItem);
    LPITEMIDLIST _TreeGetFolderID(HTREEITEM hItem);
    void _TreeCompleteCallbacks(HTREEITEM hItem);
    void _TreeGetItemText(HTREEITEM hti, LPTSTR lpszText, int cbText);
    HTREEITEM _TreeHitTest(POINT pt, DWORD *pdwFlags);
    LRESULT _TreeContextMenu(LPPOINT ppt);
    LRESULT _TreeHandleRClick();
    BOOL  _TreeValidateNode(HTREEITEM hti);
    void _TreeHandleClick();
    LPITEMIDLIST _TreeGetAbsolutePidl(HTREEITEM hti);
    void _TreeHandleBeginDrag(BOOL fShowMenu, LPNM_TREEVIEW lpnmhdr);
    BOOL _TreeRealHandleSelChange();
    void _TreeHandleSelChange(BOOL fDelayed);
    HTREEITEM _TreeFindChildItem(HTREEITEM htiParent, LPCITEMIDLIST pidl);
    void _TreeInvalidateItemInfoEx(HTREEITEM hItem, BOOL fRecurse);
    void inline _TreeInvalidateItemInfo(HTREEITEM hItem) { _TreeInvalidateItemInfoEx(hItem, FALSE); }
    HTREEITEM _TreeInsertItem(HTREEITEM htiParent, LPOneTreeNode lpnKid);
    BOOL _TreeFillOneLevel(HTREEITEM htiParent,  BOOL bInvalOld);
    void _TreeSortChildren(HTREEITEM htiParent, LPOneTreeNode lpn);
    BOOL _TreeChildAttribs(LPOneTreeNode lpn, LPCITEMIDLIST pidlChild, DWORD *pdwInOutAttrs);
    HTREEITEM _TreeBuild(LPCITEMIDLIST pidlFull,
                         BOOL bExpand, BOOL bDontFail);
    HTREEITEM _TreeBuildChild(CExpBandItemWatch &w, LPCITEMIDLIST pidl);
    LRESULT _TreeHandleBeginLabelEdit(TV_DISPINFO *ptvdi);
    LRESULT _TreeHandleEndLabelEdit(TV_DISPINFO *ptvdi);
    void _TreeSetAllVisItemInfos();
    void _TreeGetDispInfo(TV_DISPINFO *lpnm);
    HTREEITEM _TreeGetItemFromIDList(LPCITEMIDLIST pidlFull, BOOL fNearest = FALSE);
    void _TreeRefresh(LPCITEMIDLIST pidl);
    void _TreeUpdateHasKidsButton(LPNM_TREEVIEW lpnmtv);
    LRESULT _TreeHandleExpanding(LPNM_TREEVIEW lpnmtv);
    void _TreeHandleDeleteItem(LPNM_TREEVIEW lpnmtv);
    void _TreeInvalidateImageIndex(HTREEITEM hItem, int iImage);
    HRESULT _TreeHandleFileSysChange(LONG lEvent, LPCITEMIDLIST c_pidl1, LPCITEMIDLIST c_pidl2);
    void _RenameInFolder(HTREEITEM htiParent, LPCITEMIDLIST pidl);
    void _HandleUpdateImage(LPSHChangeDWORDAsIDList pImage, LPCITEMIDLIST pidl);
    void _HandleServerDisconnect(LPCITEMIDLIST pidl);
    void _HandleUpdateDir(LPCITEMIDLIST pidl);
    HTREEITEM _TreeInvalidateFromIDList(LPCITEMIDLIST pidl, BOOL fRecurse);
    void _HandleMkDir(LPCITEMIDLIST pidl);
    void _HandleRmDir(LPCITEMIDLIST pidl);
    void _HandleRename(LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra);
    void _TreeKeyboardContextMenu() { _TreeContextMenu(NULL); };
    LRESULT _TreeOnNotify(LPNMHDR lpnmhdr);
    void _TreeRefreshOneLevel(HTREEITEM hItem, BOOL bRecurse);
    void _TreeTrimInvisible(HTREEITEM hItem);
    BOOL _TreeRefreshAll();
    void _TreeValidateCurrentSelection();
    UINT _TreeGetItemState(HTREEITEM hti);
    HDPA _TreeCreateHTIList(IShellFolder* psfParent, HTREEITEM htiParent);
    BOOL _DeleteHTIListItemsFromTree(HDPA hdpa);
    int _TreeHTIListFindChildItem(IShellFolder* psfParent, HDPA hdpa, LPCITEMIDLIST pidlChild);
    void _TreeUpdateDrives();
    void _TreeMediaRemoved(LPCITEMIDLIST pidl);
    void _CancelMenuMode();
    void _PopRecursion();       
    void _PushRecursion();
    HTREEITEM _TreeMayNavigateDueToDelete(LPCITEMIDLIST pidl);
    void _AddToLegacySFC(LPOneTreeNode lpn, IShellFolder *psf);
    BOOL _IsInSFC(LPOneTreeNode lpn);
    void _MaybeAddToLegacySFC(LPOneTreeNode lpn, LPCITEMIDLIST pidl, IShellFolder *psf);

#ifdef DEBUG
    BOOL _OTMatchesTV(LPCITEMIDLIST pidl, HTREEITEM hti);
#endif

    HRESULT _ConnectToBrowser(BOOL fConnect);
    void _OnNavigate();
    
    friend int CALLBACK _export HTIList_FolderIDCompare(HTREEITEM hItem1, HTREEITEM hItem2, LPARAM lParam);

    // Keeping track of watched items
    inline BOOL _AttachItemWatch(CExpBandItemWatch *pe)
    {
        int i = DPA_AppendPtr(_hdpaEnum, pe);
#ifdef DEBUG
        if (i != -1)
            pe->_fAttached = TRUE;
#endif
        return i != -1;
    }

    BOOL _DetachItemWatch(CExpBandItemWatch *pe);

    HTREEITEM           _htiCut;
    HWND                _hwndNextViewer;
    
    IContextMenu2*       _pcmTree;
    
    OTEnumInfo          _ei;
    UINT_PTR            _nSelChangeTimer;
    BOOL                _fExpandingItem         :1;
    BOOL                _fUpdateTree            :1;
    BOOL                _fInteractive           :1;
    BOOL                _fNoInteractive         :1; // huh?  why do we have both?
    BOOL                _fShowTitles            :1;
    BOOL                _fNavigated             :1; // have we navigated at least once?

    HACCEL              _haccTree;

    int                 _uRecurse;
    int                 _cRef;
    
    LPITEMIDLIST        _GetPidlCur();

    LPCITEMIDLIST       _pidlRoot(void);
    LPITEMIDLIST        _pidlRootify(LPITEMIDLIST pidlExt);
    LPITEMIDLIST        _pidlRootCache;
    DWORD               _dwcpCookie;
    long                _lEvents;
    HDPA                _hdpaEnum;
    LPTSTR              _pszUnedited;           // name of item before we started editing

    int                 _iInFSC;                // are we in a FileSysChange?
    HDSA                _hdsaLegacySFC;         // shellfolder cache for legacy shellextensions    
};

#endif // _explband_H
