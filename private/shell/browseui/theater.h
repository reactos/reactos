#ifndef _THEATER_H
#define _THEATER_H

#include "multimon.h"
#include "mmhelper.h"

// Metrics for this view mode
#define THEATER_CYTOOLBAR       26
#define CLOSEMIN_HEIGHT         16
#define CLOSEMIN_YOFFSET        1
#define CLOSEMIN_XOFFSET        1
#define CLOSEMIN_WIDTH          54
#define PROGRESS_WIDTH          (CLOSEMIN_WIDTH + 2)
#define PROGRESS_HEIGHT         6
#define PROGRESS_YPOS           (CLOSEMIN_HEIGHT + (2 * CLOSEMIN_YOFFSET) + 1)
#define BRAND_YOFFSET           0
#define BRAND_WIDTH             34
#define BRAND_HEIGHT            26
#define CX_HIT                  (GetSystemMetrics(SM_CXEDGE) * 3)
#define CX_BROWOVERLAP          (GetSystemMetrics(SM_CXEDGE) / 2)
#define CX_FLOATERSHOWN         (BRAND_WIDTH + CLOSEMIN_WIDTH + (2 * CLOSEMIN_XOFFSET))

#define SHORT_DELAY             90
#define LONG_DELAY              (4 * SHORT_DELAY)

// Theatre mode controls
#define TMC_PROGRESSBAR     1
#define TMC_BRANDBAND       2
#define TMC_STATUSBAR       3

#define TM_STATUS_PANES                2
#define TM_STATUS_PANE_NAVIGATION      0
#define TM_STATUS_PANE_SSL             1

class CShellBrowser2;

class CTheater:
   public IOleWindow, 
   public IOleCommandTarget, 
   public IServiceProvider
{
    
public:
    HWND GetMasterWindow() {return _hwndBrowser;};
    void Begin();
    CTheater(HWND hwnd, HWND hwndToolbar, IUnknown *punkOwner);
    ~CTheater();
    
    
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }
    
    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    
    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);

    HRESULT SetBrowserBar(IUnknown* punk, int cxHidden, int cxExpanded);
    void GetPreviousWindowPlacement(WINDOWPLACEMENT* pwp, LPRECT prc)  { *pwp = _wp; *prc = _rcOld;};
    void SetAutoHideToolbar(long val) { _fAutoHideToolbar = val; };            
    void RecalcSizing();

protected:
    friend class CShellBrowser2;
    // save data
    // we put the save state data in here so that the browser doesn't have to waste memory with this data when not in theater mode
    // also, because it's transient, don't use bitfields.  save code instead of memory for transient stuff
    
private:
    int _cyLast;
    HWND _hwndToolbar; // this is the toolbar window that we want to make sure the hidden window encompasses
    HWND _hwndBrowser; // this is the hwnd that we slide down
    HWND _hwndTaskbar;
    BOOL _fShown;
    BOOL _fTaskbarShown;
    BOOL _fDelay;
    BOOL _fAutoHideToolbar;
    BOOL _fInitialBrowserBar;
    BOOL _fAutoHideBrowserBar;    

    HHOOK   _hhook;
    IUnknown *_punkOwner;
    IUnknown *_punkBrowBar;
    HWND _hwndBrowBar;
    WINDOWPLACEMENT _wp;
    RECT _rcOld;
    
    UINT _cRef;
    
    IDeskBand* _pdbBrand;
    HWND _hwndClose;
    HWND _hwndFloater;      // the Floating pallete window
    HWND _hwndProgress;     // used in CShellBrowser(2) - maybe move to interface
    int _cActiveRef;        // the ref count for activating the Floatering palette

    static CAssociationList _al; // associate threadid with CTheater objects
    
    void _SwapParents(HWND hwndOld, HWND hwndNew);
    void _Initialize();
    void _SizeBrowser();
    void _SizeFloater();
    void _CreateCloseMinimize();
    void _PositionCloseRestore();
    void _DelayHideFloater();

    void _DelayHideToolbar();
    void _HideToolbar();
    void _ShowToolbar();
    void _ContinueHideToolbar();
    LRESULT _OnMsgHook(int nCode, WPARAM wParam, MOUSEHOOKSTRUCT *pmhs, BOOL fFake);

    BOOL _IsBrowserActive();
    
    void _ShowTaskbar();
    void _HideTaskbar();

    void _Unhide(int iWhich, UINT uDelay);
    int _iUnhidee;

    BOOL _PtNearWindow(POINT pt, HWND hwnd);
    BOOL _PtOnEdge(POINT pt, int iEdge);
    BOOL _GetWindowRectRel(HWND hWnd, LPRECT lpRect);

    BOOL_PTR _HasBegun() { return (BOOL_PTR)_hhook; }
    
    BOOL _fBrowBarShown;
    int _cxBrowBarShown;
    int _cxBrowBarHidden;
    void _ContinueHideBrowBar();
    void _HideBrowBar();
    void _ShowBrowBar();

    BOOL _CanHideWindow(HWND hwnd);

    BOOL _fFloaterShown;
    COLORREF _clrBrandBk;
    void _ContinueHideFloater();
    void _HideFloater();
    void _ShowFloater();

    void _SanityCheckZorder();
    void _OnCommand(UINT idCmd);

    static LRESULT _MsgHook(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT _FloaterWndProc(HWND  hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#define THID_ACTIVATE               1
#define THID_DEACTIVATE             2
#define THID_SETBROWSERBARWIDTH     3  // tells the browser bar what it's fixed width should be
#define THID_SETTOOLBARAUTOHIDE     4
#define THID_SETBROWSERBARAUTOHIDE  5
#define THID_TOOLBARACTIVATED       6
#define THID_ONINTERNET             7

// Explorer bar registry save structure
typedef struct _BROWBARSAVE {
    UINT uiWidthOrHeight;
    BOOL fAutoHide : 1;
} BROWBARSAVE;

#define SZ_REGVALUE_EXPLORERBARA  "ExplorerBar"
#define SZ_REGVALUE_EXPLORERBAR   TEXT(SZ_REGVALUE_EXPLORERBARA)


#endif
