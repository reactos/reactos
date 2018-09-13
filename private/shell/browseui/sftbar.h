#ifndef CSFTOOLBAR
#define CSFTOOLBAR

#include "iface.h"
#include "bands.h"
#include "cwndproc.h"
#include "droptgt.h"

// Each CISFBand toolbar button lParam points to one of these.
class IBDATA
{
protected:

    DWORD        _dwFlags;      // Class specific flags
    BITBOOL      _fNoIcon:1;
    PORDERITEM   _poi;

public:
    IBDATA(PORDERITEM poi)                  { _poi = poi; }
    virtual ~IBDATA()                       { /* Don't Delete Me */ }

    LPITEMIDLIST GetPidl()                  { return _poi ? _poi->pidl : NULL; }
    void         SetOrderItem(PORDERITEM poi) { _poi = poi; }
    DWORD        GetFlags()                 { return _dwFlags; }
    void         SetFlags(DWORD dwFlags)    { _dwFlags = dwFlags; }
    BOOL         GetNoIcon()                { return _fNoIcon; }
    void         SetNoIcon(BOOL b)          { _fNoIcon = BOOLIFY(b); }
    PORDERITEM   GetOrderItem()             { return _poi ; }
};

typedef IBDATA * PIBDATA;

// Special HitTest results
#define IBHT_SOURCE         (-32768)
#define IBHT_BACKGROUND     (-32767)
#define IBHT_PAGER          (-32766)
#define IBHT_OUTSIDEWINDOW  (-32765)

class CSFToolbar :  public IWinEventHandler, 
                    public IShellChangeNotify, 
                    public CDelegateDropTarget, 
                    public IContextMenu, 
                    public IShellFolderBand,
                    public CNotifySubclassWndProc
{
public:
    // *** IUnknown methods (override) ***
    virtual STDMETHODIMP_(ULONG) AddRef(void)  PURE;
    virtual STDMETHODIMP_(ULONG) Release(void) PURE;
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent (HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plre);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IShellChangeNotify methods ***
    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // *** CDelegateDropTarget ***
    virtual HRESULT GetWindowsDDT (HWND * phwndLock, HWND * phwndScroll);
    virtual HRESULT HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD * pdwId, DWORD *pdwEffect);
    virtual HRESULT GetObjectDDT (DWORD dwId, REFIID riid, LPVOID * ppvObj);
    virtual HRESULT OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IContextMenu methods ***
    virtual STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);
    virtual STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    virtual STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // *** IShellFolderBand ***
    virtual STDMETHODIMP InitializeSFB(LPSHELLFOLDER psf, LPCITEMIDLIST pidl) { return SetShellFolder(psf, pidl); };
    virtual STDMETHODIMP SetBandInfoSFB(BANDINFOSFB * pbi)  { return E_NOTIMPL; };
    virtual STDMETHODIMP GetBandInfoSFB(BANDINFOSFB * pbi)  { return E_NOTIMPL; };

    // Toolbar Management
    virtual HRESULT SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidl);
    virtual void    EmptyToolbar();

protected:
    CSFToolbar();
    virtual    ~CSFToolbar();

    virtual    void _CreateToolbar(HWND hwndParent);
    virtual HWND _CreatePager(HWND hwndParent);
    void    _DestroyToolbar();
    virtual void    _FillToolbar();
    void    _UnregisterToolbar();
    void    _RegisterToolbar();
    void    _RegisterChangeNotify();
    void    _UnregisterChangeNotify();      // Unregisters 

    void    _Refresh();
    void    _ReleaseShellFolder();

    virtual BOOL _AddPidl(LPITEMIDLIST pidl, int index);
    virtual PIBDATA _AddOrderItemTB(PORDERITEM poi, int index, TBBUTTON* ptbb);
    virtual void _FillDPA(HDPA hdpa, HDPA hdpaSort, DWORD dwEnumFlags);
    virtual PIBDATA _CreateItemData(PORDERITEM poi);
    virtual HWND GetHWNDForUIObject() { return _hwndTB; };
    virtual HRESULT _LoadOrderStream() { return E_NOTIMPL; };
    virtual HRESULT _SaveOrderStream();
    virtual BOOL    _AllowDropOnTitle() { return FALSE; };
    virtual HRESULT _GetIEnumIDList(DWORD dwEnumFlags, IEnumIDList **ppenum);

    LPITEMIDLIST    _pidl;
    IShellFolder*    _psf;
    ITranslateShellChangeNotify*    _ptscn;

    HWND            _hwndPager;
    HWND            _hwndTB;
    HWND            _hwndToolTips;

    DWORD           _dwStyle;           // style bits to be ORd in when _hwndTB is created
    TBINSERTMARK    _tbim;
    int             _iDragSource;
    HDPA            _hdpaOrder;         // current order list (if non-default)
    HDPA            _hdpa;              // current set of items, mirrors _hwndTB content
    long            _lEvents;

    int             _iButtonCur;
    IContextMenu    *_pcmSF;
    IContextMenu2 * _pcm2;

    int             _nNextCommandID;
    int             _idCmdSF;
    int             _cxMin;
    int             _cxMax;
    HWND            _hwndDD;
    HWND            _hwndWorkerWindow;

    // Flags
    BITBOOL         _fNoShowText :1;    // TRUE iff no text with icon
    BITBOOL         _fShow :1;          // TRUE when ShowDW has happened
    BITBOOL         _fDirty :1;         // TRUE iff hidden contents modified
    BITBOOL         _fCheckIds :1;      // TRUE iff _GetCommandID has wrapped
    BITBOOL         _fFSNotify :1;      // TRUE to receive FS Notifications
    BITBOOL         _fFSNRegistered :1; // are we already registered?
    BITBOOL         _fAccelerators :1;  // whether to show & as accel or as &
    BITBOOL         _fAllowRename :1;   // TRUE to query _psf for IContextMenu of _pidl
    BITBOOL         _fDropping :1;      // TRUE while doing drop.
    BITBOOL         _fDropped :1;       // TRUE if we have reordered, _hdpaOrder may not have been created yet
    BITBOOL         _fNoNameSort :1;    // TRUE if band should _not_ sort icons by name
    BITBOOL         _fVariableWidth :1; 
    BITBOOL         _fNoIcons :1;       // turns off icons
    BITBOOL         _fVertical :1;      // TRUE: band is displayed vertically
    BITBOOL         _fMulticolumn : 1;
    BITBOOL         _fHasOrder: 1;
    BITBOOL         _fPSFBandDesktop :1;// TRUE iff _psfBand came from desktop
                                            //          this implies psfDesktop->BindToObject(_pidl)
                                            //          results in the correct ISF
    BITBOOL         _fRegisterChangeNotify: 1;  // TRUE: We will register for change notify.
    BITBOOL         _fAllowReorder: 1;
    BITBOOL         _fChangedOrder: 1;      // Only send change notifies if we actually changed the order
    UINT            _uIconSize : 2;     // Large/Small/Logo


    // Virtual Function Overrides for Window Subclass
    virtual LRESULT _OnHotItemChange(NMTBHOTITEM * pnmhot);
    virtual HRESULT OnTranslatedChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual LRESULT _OnTimer(WPARAM wParam);
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnCustomDraw(NMCUSTOMDRAW* pnmcd);
    virtual void _OnDragBegin(int iItem, DWORD dwPreferedEffect);
    virtual void _OnToolTipsCreated(NMTOOLTIPSCREATED* pnm);
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual LRESULT _OnCommand(WPARAM wParam, LPARAM lParam) { return 0; };
    virtual void _OnFSNotifyAdd(LPCITEMIDLIST pidl);
    virtual void _OnFSNotifyRemove(LPCITEMIDLIST pidl);
    virtual void _OnFSNotifyRename(LPCITEMIDLIST pidlFrom, LPCITEMIDLIST pidlTo);
    virtual void _OnFSNotifyUpdate(LPCITEMIDLIST pidl);
    virtual HRESULT _OnRenameFolder(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual HMENU _GetContextMenu(IContextMenu* pcm, int* pid);
    virtual void _OnDefaultContextCommand(int idCmd);
    virtual LRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam);
    
    // Helper Functions
    int     _GetCommandID();
    virtual void    _ObtainPIDLName(LPCITEMIDLIST pidl, LPTSTR psz, int cchMax);
    BOOL    _IsParentID(LPCITEMIDLIST pidl);
    BOOL    _IsChildID(LPCITEMIDLIST pidlChild, BOOL fImmediate);
    BOOL    _IsEqualID(LPCITEMIDLIST pidl);
    LPVOID  _GetUIObjectOfPidl(LPCITEMIDLIST pidl, REFIID riid);
    HMENU   _GetBaseContextMenu();
    HRESULT _GetTopBrowserWindow(HWND* phwnd);
    HRESULT _OnOpen(int id, BOOL fExplore);
    HRESULT _HandleSpecialCommand(IContextMenu* pcm, PPOINT ppt, int id, int idCmd);
    LRESULT _DoContextMenu(IContextMenu* pcm, LPPOINT ppt, int id, LPRECT prcExclude);
    void _SortDPA(HDPA hdpa);
    virtual HWND CreateWorkerWindow();

    BOOL_PTR InlineDeleteButton(int iIndex);

    static INT_PTR CALLBACK _RenameDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _OnRename(LPPOINT ppt, int id);
    UINT    _IndexToID(int iIndex);
    LPCITEMIDLIST _IDToPidl(UINT uiCmd, int *piPos = NULL);
    PIBDATA _IDToPibData(UINT uiCmd, int * piPos = NULL);
    PIBDATA _PosToPibData(UINT iPos);
    void    _RememberOrder();
    void    _UpdateButtons();
    void    _OnGetDispInfo(LPNMHDR pnm, BOOL fUnicode);
    LPITEMIDLIST _GetButtonFromPidl(LPCITEMIDLIST pidl, TBBUTTONINFO * ptbbi, int * pIndex);
    DWORD   _GetAttributesOfPidl(LPCITEMIDLIST pidl, DWORD dwAttribs);
    BOOL    _UpdateShowText(BOOL fNoShowText);



    // Virtual Helper Functions
    virtual int     _GetBitmap(int iCommandID, PIBDATA pibdata, BOOL fUseCache);
    virtual void    _SetDirty(BOOL fDirty);
    virtual HMENU   _GetContextMenu();
    virtual BOOL    _UpdateIconSize(UINT uIconSize, BOOL fUpdateButton);
    virtual HRESULT _TBStyleForPidl(LPCITEMIDLIST pidl, 
                                   DWORD * pdwTBStyle, DWORD* pdwTBState, DWORD * pdwMIFFlags, int* piIcon);
    virtual BOOL    _FilterPidl(LPCITEMIDLIST pidl);
    virtual int     _DefaultInsertIndex();
    virtual void    _ToolbarChanged() { };
    virtual void    _Dropped(int nIndex, BOOL fDroppedOnSource);
    virtual HRESULT _AfterLoad();
    virtual void    v_CalcWidth(int* pcxMin, int* pcxMax);
    virtual void    _SetToolbarState();
    virtual void    v_NewItem(LPCITEMIDLIST pidl) {};

    static void s_NewItem(LPVOID pvParam, LPCITEMIDLIST pidl);
};

BOOL TBHasImage(HWND hwnd, int iImageIndex);
LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif
