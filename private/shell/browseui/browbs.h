#ifndef _browbs_h
#define _browbs_h

#define WANT_CBANDSITE_CLASS
#include "bandsite.h"

class CBrowserBandSite :
    public CBandSite,
    public IExplorerToolbar
{
public:
    CBrowserBandSite();

    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { return CBandSite::QueryInterface(riid, ppvObj);};
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CBandSite::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { return CBandSite::Release();};

    // *** IOleCommandTarget ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IWinEventHandler ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();

    // *** IBandSite methods ***
    virtual STDMETHODIMP SetBandSiteInfo(const BANDSITEINFO * pbsinfo);

    // *** IDeskBarClient methods ***
    virtual STDMETHODIMP SetModeDBC(DWORD dwMode);

    // *** IExplorerToolbar ***
    virtual STDMETHODIMP SetCommandTarget(IUnknown* punkCmdTarget, const GUID* pguidButtonGroup, DWORD dwFlags);
    virtual STDMETHODIMP AddStdBrowserButtons(void) { return E_NOTIMPL; };
    virtual STDMETHODIMP AddButtons(const GUID* pguidButtonGroup, UINT nButtons, const TBBUTTON* lpButtons);
    virtual STDMETHODIMP AddString(const GUID * pguidButtonGroup, HINSTANCE hInst, UINT_PTR uiResID, LONG_PTR *pOffset);
    virtual STDMETHODIMP GetButton(const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton);
    virtual STDMETHODIMP GetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT* pfState);
    virtual STDMETHODIMP SetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT fState);
    virtual STDMETHODIMP AddBitmap(const GUID* pguidButtonGroup, UINT uiBMPType, UINT uiCount, TBADDBITMAP* ptb,
                                    LRESULT* pOffset, COLORREF rgbMask) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetBitmapSize(UINT* uiID) { return E_NOTIMPL; };
    virtual STDMETHODIMP SendToolbarMsg(const GUID* pguidButtonGroup, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, LRESULT *plRes) { return E_NOTIMPL; };
    virtual STDMETHODIMP SetImageList(const GUID* pguidCmdGroup, HIMAGELIST himlNormal, HIMAGELIST himlHot, HIMAGELIST himlDisabled);
    virtual STDMETHODIMP ModifyButton( const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton) { return E_NOTIMPL; };

protected:

    virtual void _OnCloseBand(DWORD dwBandID);
    virtual LRESULT _OnBeginDrag(NMREBAR* pnm);            
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual HRESULT _Initialize(HWND hwndParent);
    virtual IDropTarget* _WrapDropTargetForBand(IDropTarget* pdtBand);
    virtual HRESULT v_InternalQueryInterface(REFIID riid, void **ppvObj);
    virtual DWORD _GetWindowStyle(DWORD* pdwExStyle);
    virtual HMENU _LoadContextMenu();
    LRESULT _OnCDNotify(LPNMCUSTOMDRAW pnm);
    virtual void _Close();
    HRESULT _TrySetFocusTB(int iDir);
    virtual HRESULT _CycleFocusBS(LPMSG lpMsg);
    LRESULT _OnHotItemChange(LPNMTBHOTITEM pnmtb);
    LRESULT _OnNotifyBBS(LPNMHDR pnm);
    virtual void _BandInfoFromBandItem(REBARBANDINFO *prbbi, LPBANDITEMDATA pbid, BOOL fBSOnly);
    virtual void _ShowBand(LPBANDITEMDATA pbid, BOOL fShow);
    virtual void _UpdateAllBands(BOOL fBSOnly, BOOL fNoAutoSize);

    virtual int _ContextMenuHittest(LPARAM lParam, POINT* ppt);

    HFONT   _GetTitleFont(BOOL fForceRefresh);
    void    _CalcHeights();
    void    _InitLayout();
    void    _UpdateLayout();
    void    _UpdateToolbarFont();

    void _CreateTBRebar();
    void _InsertToolbarBand();
    void _UpdateToolbarBand();
    void _CreateTB();
    void _RemoveAllButtons();

    void _UpdateHeaderHeight(int iBand);

    void _SizeCloseTB();
    void _PositionToolbars(LPPOINT ppt);
    void _CreateCloseTB();

    void _DrawEtchline(HDC hdc, LPRECT prc, int iOffset, BOOL fVertical);

    BITBOOL _fTheater:1;
    BITBOOL _fNoAutoHide:1;
    BITBOOL _fToolbar:1;    // do we have a toolbar for the current band?

    HWND                _hwndTBRebar;
    HWND                _hwndTB;
    HWND                _hwndCloseTB;
    IOleCommandTarget*  _pCmdTarget;
    GUID                _guidButtonGroup;

    HFONT   _hfont;
    UINT    _uTitle;
    UINT    _uToolbar;
    DWORD   _dwBandIDCur;   // the currently visible band
};
#define BROWSERBAR_ICONWIDTH 16
#define BROWSERBAR_FONTSIZE 18

#ifndef UNIX
#define BROWSERBAR_TITLEHEIGHT 22
#else
#define BROWSERBAR_TITLEHEIGHT 24
#endif

#define BROWSERBAR_TOOLBARHEIGHT 24

#endif // _browbs_h
