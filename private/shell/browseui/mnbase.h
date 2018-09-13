#ifndef MNBASE
#define MNBASE


// Characters for _DrawMenuGlyph
#define CH_MENUARROWA    '8'
#define CH_MENUARROW     TEXT(CH_MENUARROWA)
#define CH_MENUCHECKA    'a'
#define CH_MENUCHEVRONA  187

class CMenuBand;    // Forward Declare
class CMenuBandMetrics;

#define LIST_GAP 8      // from Observation

class CMenuToolbarBase: public IWinEventHandler, public IObjectWithSite
{
public:

    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, void ** ppvSite);

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd) PURE;
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);

    // Other public methods

    CMenuToolbarBase(CMenuBand* pmb, DWORD dwFlags);

    // Returns the HWND and Converts the pt to child.
    virtual void v_ForwardMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void v_SendMenuNotification(UINT idCmd, BOOL fClear) PURE;
    virtual BOOL v_TrackingSubContextMenu() PURE;
    virtual void v_Show(BOOL fShow, BOOL fForceUpdate);
    virtual BOOL v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons) PURE;
    virtual void v_UpdateButtons(BOOL fNegotiateSize) PURE;
    virtual HRESULT v_GetSubMenu(int iCmd, const GUID* pguidService, REFIID riid, void** pObj) PURE;
    virtual HRESULT v_CallCBItem(int idtCmd, UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;
    virtual HRESULT v_GetState(int idtCmd, LPSMDATA psmd) PURE;
    virtual HRESULT v_ExecItem(int iCmd) PURE;
    virtual DWORD v_GetFlags(int iCmd) PURE;
    virtual void v_Refresh() PURE;
    virtual void v_Close();
    virtual void v_OnEmptyToolbar();
    virtual void v_OnDeleteButton(LPVOID pData) {};
    virtual HRESULT v_InvalidateItem(LPSMDATA psmd, DWORD dwFlags) 
        { return E_NOTIMPL; };

    virtual void NegotiateSize();
    virtual void SetWindowPos(LPSIZE psize, LPRECT prc, DWORD dwFlags);
    virtual void GetSize(SIZE*);
    virtual void CreateToolbar(HWND hwndParent);
    virtual void SetParent(HWND hwndParent);
    virtual HRESULT GetShellFolder(LPITEMIDLIST* ppidl, REFIID riid, void** ppvObj) {return E_FAIL;};
    virtual HRESULT GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags) { return E_FAIL; };
    virtual HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags) { return E_FAIL;};
    virtual void Expand(BOOL fExpand) {};

    HRESULT GetSubMenu(int idCmd, GUID* pguidService, REFIID riid, void** ppvObj);

    HRESULT PositionSubmenu(int idCmd);
    void Activate(BOOL fActivate);
    void SetMenuBandMetrics(CMenuBandMetrics* pmbm);
    void PostPopup(int idCmd, BOOL bSetItem, BOOL bInitialSelect);
    void PopupClose(void);
    HRESULT PopupOpen(int nCmd);
    void PopupHelper(int idCmd, BOOL bInitialSelect);
    void KillPopupTimer();
    void SetToTop(BOOL bToTop);
    void EmptyToolbar();        // override
    DWORD GetFlags(void)          { return _dwFlags; };
    BOOL DontShowEmpty()           { return _fDontShowEmpty; };
    void DontShowEmpty(BOOL fDontShowEmpty) { _fDontShowEmpty = BOOLIFY(fDontShowEmpty); };
    BOOL GetChevronID()               { return _idCmdChevron;  };
    int GetValidHotItem(int iDir, int iIndex, int iCount, DWORD dwFlags);
    BOOL SetHotItem(int iDir, int iIndex, int iCount, DWORD dwFlags);
    void SetKeyboardCue();
    virtual ~CMenuToolbarBase() {};

    BOOL IsEmpty()      { return _fEmpty; };

    HWND        _hwndMB;
   
protected:
    static void s_FadeCallback(DWORD dwStep, LPVOID pvParam);

    virtual void v_CalcWidth(int* pcxMin, int* pcxMax);
    virtual int  v_GetDragOverButton() PURE;
    virtual HRESULT v_GetInfoTip(int iCmd, LPTSTR psz, UINT cch) PURE;
    virtual HRESULT v_CreateTrackPopup(int idCmd, REFIID riid, void** ppvObj) PURE;
    
    // Window Proc Overrides
    LRESULT _DropDownOrExec(UINT idCmd, BOOL bKeyboard);
    LRESULT _OnCustomDraw(NMCUSTOMDRAW * pnmcd);
    void    _PaintButton(HDC hdc, int idCmd, LPRECT prc, DWORD dwMBIF);
    LRESULT _OnWrapHotItem(NMTBWRAPHOTITEM* pnmwh);
    LRESULT _OnWrapAccelerator(NMTBWRAPACCELERATOR* pnmwa);
    LRESULT _OnDupAccelerator(NMTBDUPACCELERATOR* pnmda);
    virtual LRESULT _OnHotItemChange(NMTBHOTITEM * pnmhot);
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual LRESULT _OnDropDown(LPNMTOOLBAR pnmtb);
    virtual LRESULT _OnTimer( WPARAM wParam );
    virtual void _FlashChevron();
    virtual LRESULT _DefWindowProcMB(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL    _OnKey(BOOL bDown, UINT vk, UINT uFlags);
    void    _OnSelectArrow(int iDir);
    void    _FireEvent(BYTE bEvent);

    // Utility Functions
    void    _DoPopup(int idCmd, BOOL bInitialSelect);
    virtual void    _SetFontMetrics();
    virtual void    _SetToolbarState();
    void    _PressBtn(int idBtn, BOOL bDown);
    HRESULT _SetMenuBand(IShellMenu* psm);
    BOOL    _SetTimer(int nTimer);

    void    _DrawMenuArrowGlyph( HDC hdc, RECT * prc, COLORREF rgbText );
    void    _DrawMenuGlyph( HDC hdc, HFONT hFont, RECT * prc, 
                               CHAR ch, COLORREF rgbText,
                               LPSIZE psize);

    int     _CalcChevronSize();
    void    _DrawChevron(HDC hdc, LPRECT prect, BOOL fFocus, BOOL fSelected);

    BOOL    _HandleObscuredItem(int idCmd);

    CMenuBand*  _pcmb;
    DEBUG_CODE (int _cRef);     // To debug references to the sub objects.
    DWORD       _dwFlags;           // SMSET_* flags
    UINT        _uIconSizeMB;
    UINT        _nItemTimer;
    int         _idCmdChevron;     // -1 if no chevron exists
    int         _cPromotedItems;    // Number of promoted items.
    BYTE        _cFlashCount;
    int         _idCmdLastClicked;
    int         _iLastClickedTime;
    int         _idCmdDragging;        

    BITBOOL     _fHasDemotedItems: 1;
    BITBOOL     _fVerticalMB: 1;
    BITBOOL     _fTopLevel: 1;
    BITBOOL     _fEmpty : 1;
    BITBOOL     _fHasSubMenu: 1;
    BITBOOL     _fEditMode : 1;
    BITBOOL     _fClickHandled: 1;
    BITBOOL     _fProcessingWrapHotItem: 1;
    BITBOOL     _fEmptyingToolbar : 1;
    BITBOOL     _fMulticolumnMB : 1;
    BITBOOL     _fExpandTimer: 1;   // There is an expand timer.
    BITBOOL     _fIgnoreHotItemChange: 1;
    BITBOOL     _fShowMB: 1;
    BITBOOL     _fFirstTime: 1;
    BITBOOL     _fHasDrop: 1;
    BITBOOL     _fRefreshInfo: 1;
    BITBOOL     _fDontShowEmpty: 1;
    BITBOOL     _fSuppressUserMonitor: 1;
    BITBOOL     _fHorizInVerticalMB: 1;     // TRUE: Don't set EX_Vertical on the Toolbar
};


int     GetButtonCmd(HWND hwndTB, int iPos);
void*   ItemDataFromPos(HWND hwndTB, int iPos);
BOOL    SetHotItem(HWND hwnd, int iDir, int iIndex, int iCount, DWORD dwFlags);
long    GetIndexFromChild(BOOL fTop, int iIndex);

// UEM Parameters
#define UEM_TIMEOUT         0
#define UEM_HOT_ITEM        1
#define UEM_HOT_FOLDER      2

#define UEM_RESET           -1

#endif  // MNBASE
