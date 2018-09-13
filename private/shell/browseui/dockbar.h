#ifndef DOCKBAR_H_
#define DOCKBAR_H_

#include "basebar.h"
#include <shlobjp.h>
#include <shlguidp.h>
//      local macros
//
// configurable constants

#define XXX_NEW         0       // 1=turn on work-in-progress
#define XXX_BROWSEROWNED    0   // 1:browser deskbar is owned (proto)
#define XXX_CHEEDESK    0       // 1:chee's desktop (vs. _fDesktop)
#define XXX_BTMFLOAT    0       // 0=allow dragging from non-desk-btm->float
#define XXX_HIDE        1       // 1=turn on autohide (work-in-progress)
#define XXX_HIDEALL     1       // 1=enable autohide in browsers (non-topmost)


#define XXX_CANCEL      0       // 1=use experimental CANCEL code
#define XXX_NEWSLIDE    0       // 1=use new SlideWindow code

#ifndef UNREFERENCED_PARM
#define UNREFERENCED_PARM(p)    (p)      // ARGUSED
#endif

//***   _PM, _PX -- lazy shorthands
// DESCRIPTION
//      _PM     check for p==NULL before doing p->m
//      _PX     check for p==NULL before doing EXPR(p)
//
#define _PM(p, m)       ((p) ? (p)->m : (-1))
#define _PX(p, x)       ((p) ? (x) : (-1))

#define BITS_SET(v, m)  (((v) & (m)) == (m))

//***   IN, OUT, INOUT --
//
#define IN
#define OUT
#define INOUT

#ifndef NOCDESKBAR
#include "dhuihand.h"

//========================================================================
// class CDeskBar (CDeskBar* pwbar)
// NOTES
//  we don't use CObjectWithSite because we want _ptbSite not _punkSite.
//========================================================================
class CDockingBar : public CBaseBar
                ,public IDockingWindow
                ,public IObjectWithSite     // n.b. *not* CObjectWithSite
                ,public IPersistStreamInit
                ,public IPersistPropertyBag
                ,public CDocHostUIHandler
{
public:
    // *** IUnknown -- disambiguate ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void)
        { return CBaseBar::AddRef(); }
    virtual STDMETHODIMP_(ULONG) Release(void)
        { return CBaseBar::Release(); }

    // *** IOleWindow -- disambiguate ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd)
        { return CBaseBar::GetWindow(lphwnd); }
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode)
        { return CBaseBar::ContextSensitiveHelp(fEnterMode); }

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt,
        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved) { return CBaseBar::CloseDW(dwReserved); }
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
        IUnknown* punkToolbarSite, BOOL fReserved);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    // BUGBUG is E_NOTIMPL ok?
    virtual STDMETHODIMP GetSite(REFIID riid, void** ppvSite) { ASSERT(0); *ppvSite = NULL; return E_NOTIMPL; };

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IPersistStreamInit ***
    //virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP IsDirty(void);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);
    virtual STDMETHODIMP InitNew(void);

    // *** IPersistPropertyBag ***
    virtual STDMETHODIMP Load(IPropertyBag *pPropBag,
                                           IErrorLog *pErrorLog);
    virtual STDMETHODIMP Save(IPropertyBag *pPropBag,
                        BOOL fClearDirty, BOOL fSaveAllProperties);

    // *** IInputObjectSite methods ***
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);
    
    // *** IDocHostUIHandler methods ***
    virtual STDMETHODIMP ShowContextMenu(DWORD dwID,
        POINT* ppt, IUnknown* cmdtReserved, IDispatch* pdispReserved);

protected:
    // Constructor & Destructor
    CDockingBar();
    virtual ~CDockingBar();
    
    void _Initialize();         // 2nd-phase ctor
    virtual void _SetModeSide(UINT eMode, UINT uSide, HMONITOR hMon, BOOL fNoMerge);
    virtual void _OnPostedPosRectChange();
    virtual void _GetChildPos(LPRECT prc);
    virtual void _GetStyleForMode(UINT eMode, LONG* plStyle, LONG* plExStyle, HWND* phwndParent);
    
    friend HRESULT CDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    friend HRESULT CDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    friend HRESULT BrowserBar_Create(IUnknown** ppunk, IUnknown** ppbs);

    // Private members
    HMENU _GetContextMenu();

    virtual LRESULT _OnCommand(UINT msg, WPARAM wparam, LPARAM lparam);
    void _ChangeTopMost(UINT eModeNew);
    void _ChangeWindowStateAndParent(UINT eModeNew);
    void _ChangeZorder();
    void _ResetZorder();
    virtual void _OnRaise(UINT flags);
#if XXX_BTMFLOAT && 0
    void _MayReWindow(BOOL fToFloat);
#endif
    
    virtual void _NotifyModeChange(DWORD dwMode);
    void _GetBorderRect(HMONITOR hMon, RECT* prc);
    HRESULT _NegotiateBorderRect(RECT* prcOut, RECT* prcReq, BOOL fCommit);
    virtual void _OnSize(void);
    void _InitPos4(BOOL fCtor);
    void _ProtoRect(RECT* prcOut, UINT eModeNew, UINT uSideNew, HMONITOR hMonNew, POINT* ptXY);
    void _NegotiateRect(UINT eModeNew, UINT uSideNew, HMONITOR hMonNew, RECT* rcReq, BOOL fCommit);
    void _MoveSizeHelper(UINT eModeNew, UINT eSideNew, HMONITOR hMonNew,
        POINT* ptTrans, RECT* rcFeed, BOOL fCommit, BOOL fMove);
    void _SetVRect(RECT* rcNew);
    void _Recalc(void);

    LRESULT _CalcHitTest(WPARAM wParam, LPARAM lParam);

    void _DragEnter(UINT uMsg, int xCursor, int yCursor, RECT* rcFeed);
    void _DragTrack(UINT uMsg, int xCursor, int yCursor, RECT* rcFeed, int eState);
    void _DragLeave(int x, int y, BOOL fCommit);
    void _ExecDrag(int eDragging);

    void _TrackSliding(int x, int y, RECT* rcFeed,
        BOOL fCommit, BOOL fMove);
    UINT _CalcDragPlace(POINT& pt, HMONITOR * hMon);
    void _SmoothDragPlace(UINT eModeNew, UINT eSideNew, HMONITOR hMonNew, INOUT POINT* pt, RECT* rcFeed);
    void _RemoveToolbar(DWORD dwFlags);

    HRESULT _TrackPopupMenu(const POINT* ppt);
    HMONITOR _SetNewMonitor(HMONITOR hMonNew);

    void _HideRegister(BOOL fToHide);
    void _DoHide(UINT uOpMask);
    virtual void _HandleWindowPosChanging(LPWINDOWPOS pwp);
    virtual void _GrowShrinkBar(DWORD dwDirection);

#if 0
    void _DoManHide(UINT uOpMask);
#endif
        // for _DoHide and _DoManHide
        enum aho {
            AHO_KILLDO  = 0x01,
            AHO_SETDO   = 0x02,
            AHO_KILLUN  = 0x04,
            AHO_SETUN   = 0x08,
            AHO_REG     = 0x10,
            AHO_UNREG   = 0x20,
            AHO_MOVEDO  = 0x40,
            AHO_MOVEUN  = 0x80
        };

        #define AHO_MNE         TEXT("ksKSrRmM")

    void _AppBarRegister(BOOL fRegister);
    void _AppBarOnSize();
    void _AppBarOnCommand(UINT idCmd);
    void _AppBarOnWM(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _AppBarOnPosChanged(PAPPBARDATA pabd);
    void _AppBarCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    int AppBarQueryPos(RECT* prcOut, UINT uEdge, HMONITOR hMon, const RECT* prcReq, PAPPBARDATA pabd, BOOL fCommit);
    void AppBarSetPos(UINT uEdge, const RECT* prcReq, PAPPBARDATA pabd);
    void AppBarSetPos0(UINT uEdge, const RECT* prcReq, PAPPBARDATA pabd);
    void AppBarQuerySetPos(RECT* prcOut, UINT uEdge, HMONITOR hMon, const RECT* prcReq, PAPPBARDATA pabd, BOOL fCommit);
    LRESULT _OnNCHitTest(WPARAM wParam, LPARAM lParam);

    void _AdjustToChildSize();

    // Window procedure
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnActivate(WPARAM wParam, LPARAM lParam);

    virtual BOOL _OnCloseBar(BOOL fConfirm);


#define RX_EDGE         0x01
#define RX_OPPOSE       0x02
#define RX_ADJACENT     0x04
#define RX_GETWH        0x08
#define RX_HIDE         0x10

    //
    // We need to do a get window in side this function so that we
    // can mirror the edges. I changed them to non-static. [samera]
    //
    int RectXform(RECT* prcOut, UINT uRxMask,
        const RECT* prcIn, RECT* prcBound, int iWH, UINT uSide, HMONITOR hMon);
    
    int RectGetWH(const RECT* prcReq, UINT uSide) {
        return RectXform(NULL, RX_GETWH, prcReq, NULL, -1, uSide, NULL);
    }

    void RectSetWH(RECT* prcReq, int iWH, UINT uSide) {
        RectXform(prcReq, RX_OPPOSE, prcReq, NULL, iWH, uSide, NULL);
        return;
    }

    // Member variables
    IDockingWindowSite*  _ptbSite;           // owner
    INT             _adEdge[4];             // edges' widths (or heights)
    RECT            _rcFloat;               // floating position
    HMONITOR        _hMon;                  // the monitor I am on
    POINT           _ptIdtUnHide;           // unhide hysteresis cursor pos

    // Variable initialized IPersistPropertyBag::Load
    // ...

    // Bit fields
    UINT            _uSide:3;               // edge we're on (ABE_*)
    // 3 states to initialization (4 w/ ctor)
    BITBOOL         _fInitSited:1;          // SetSite done
    UINT            _eInitLoaded:2;         // Load (or InitNew) done
    BITBOOL         _fInitShowed:1;         // Show done
#if ! XXX_CHEEDESK
    BITBOOL         _fDesktop:1;            // 1:hosted by desktop (vs. browser)
#endif
    UINT            _fDragging:2;           // we're dragging
    BITBOOL         _fWantHide:1;           // 1:autohide requested (in UI)
    BITBOOL         _fDeleteable:1;         // when we close we should signal our parent to delete our info
    BITBOOL         _fAppRegistered:1;      // Registered as an appbar

    // Member variables (drag&drop, sizing, ...)
    UINT            _uSidePending:3;        // ...
    BITBOOL         _fCanHide:1;            // 1:autohide granted (registered)
    BOOL            _fHiding:2;             // hide mode (HIDE_*)
    BITBOOL         _fIdtDoHide:1;          // 1:IDT_AUTOHIDE running
    BITBOOL         _fIdtUnHide:1;          // 1:IDT_AUTOUNHIDE running

    UINT            _eMode;               // mode we're in (WBM_*)
    UINT            _eModePending;        // pending drag state
    
    LPARAM          _xyPending;             // pending drag state
    RECT            _rcPending;             // ...
    HMONITOR        _hMonPending;           // pending monitor 
#ifdef DEBUG
    // BUGBUG temporary until we make browser tell us about activation
    BOOL            _fActive:1;             // 1:window is active
#endif

    
    // MOVE TO CBROWSERBAR    
    BITBOOL _fTheater :1;
    BITBOOL _fNoAutoHide :1;
    int _iTheaterWidth;
    // END MOVE TO CBROWSERBAR
};

#define WBM_IS_TOPMOST() (_eMode & WBM_TOPMOST)

#endif //NOCDESKBAR

//***   CASSERT -- compile-time assert
// DESCRIPTION
//      Like Assert, but checked at compile-time, and generates no code
//      Note that the expr must of course be constant...
#ifndef UNIX
#define CASSERT(e)      extern int dummy_array[(e)]
#else
#define CASSERT(e)      
#endif

//***   ABE_* -- helpers, etc. for ABE_*'s
//

//***   ABE_X* -- extended ABE_*'s
// DESCRIPTION
//      ABE_NIL: distinguished value (BUGBUG unused?).
//
//      ABE_XFLOATING: normally we carry around a (mode,side) pair.  this
//      works fine but is a pain in the (rare) case that we want to return
//      a pair.  so we have a 'special' side which means we're really
//      floating.  (alternatives considered included combining the two via
//      bit magic, or passing by reference.  none stood out as a great
//      sol'n, and we only use it one place.  a hack?  perhaps...)
#define ABE_NIL         ((UINT) 0x07)   // nil (-1 as a 3-bit field) (ugh!)
#define ABE_XFLOATING   ((UINT) 4)      // floating (undocked)
CASSERT((ABE_LEFT|ABE_RIGHT|ABE_TOP|ABE_BOTTOM) == 3);  // 0..3

#define ISABE_DOCK(abe) (0 <= (abe) && (abe) <= 3)

#define ABE_TO_IDM(abe) (IDM_AB_LEFT + (abe))
#define IDM_TO_ABE(abe) ((abe) - IDM_AB_LEFT)
CASSERT(IDM_AB_BOTTOM - IDM_AB_LEFT == 3);      // make sure LEFT is 0th

//***   ABE_HORIZ -- is ABE_* horizontal?
// #define ABE_HORIZ(e) ((e) == ABE_TOP || (e) == ABE_BOTTOM)
#define ABE_HORIZ(e)    ((e) & 1)
CASSERT(ABE_HORIZ(ABE_TOP) && ABE_HORIZ(ABE_BOTTOM));
CASSERT(! ABE_HORIZ(ABE_LEFT) && ! ABE_HORIZ(ABE_RIGHT));


#define APPBAR_CALLBACK (WM_USER + 73)  // BUGBUG bad bad bad


//***   WS_*, etc. -- window bits, etc. for various modes
// NOTES
//      REVIW Don't use SM_CAPTION because it doesn't work properly yet.
//      WS_XTOPMOST w/ an 'X' to avoid collision w/ WS_TOPMOST...

#define WS_NIL          (WS_POPUP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS)
#define WS_EX_NIL       (WS_EX_TOOLWINDOW /*|WS_EX_WINDOWEDGE*/)
#define PARENT_NIL      (HWND_DESKTOP)

#define WS_XTOPMOST     (WS_POPUP|WS_BORDER|WS_THICKFRAME|WS_CLIPCHILDREN|WS_CLIPSIBLINGS)
#define WS_EX_XTOPMOST  (WS_EX_TOOLWINDOW|WS_EX_WINDOWEDGE)
#define PARENT_XTOPMOST (HWND_DESKTOP)

#define WS_BTMMOST      (WS_POPUP|WS_BORDER|WS_THICKFRAME|WS_CLIPCHILDREN|WS_CLIPSIBLINGS)
#define WS_EX_BTMMOST   (WS_EX_TOOLWINDOW|WS_EX_WINDOWEDGE)
#define PARENT_BTMMOST() HWND_DESKTOP

#if 0
// 970208 keep this around for 1 week in case the autosize bug isn't fixed
#define WS_BTMMOST      WS_BBTMMOST
#define WS_EX_BTMMOST   WS_EX_BBTMMOST
#define PARENT_BTMMOST() PARENT_BBTMMOST()
#endif

#define WS_BFLOATING     ((/*WS_POPUP*/WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME |WS_THICKFRAME|WS_CLIPCHILDREN|WS_CLIPSIBLINGS) & ~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX))
#define WS_EX_BFLOATING  (/*WS_EX_PALETTEWINDOW | */ WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE)


#define WS_FLOATING     ((/*WS_POPUP*/WS_TILEDWINDOW|WS_THICKFRAME|WS_CLIPCHILDREN|WS_CLIPSIBLINGS) & ~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX))
#define WS_EX_FLOATING  (WS_EX_PALETTEWINDOW | WS_EX_WINDOWEDGE)
#define PARENT_FLOATING (HWND_DESKTOP)

#if XXX_BROWSEROWNED
#define WS_BBTMMOST     WS_FLOATING
#define WS_EX_BBTMMOST  (WS_EX_FLOATING|WS_EX_TOOLWINDOW)
#define PARENT_BBTMMOST PARENT_FLOATING
#else
// non-topmost
#define WS_BBTMMOST     (WS_CHILD/*|WS_BORDER|WS_THICKFRAME*/|WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#define WS_EX_BBTMMOST  (WS_EX_CLIENTEDGE /*|WS_EX_WINDOWEDGE*/)
#define PARENT_BBTMMOST() (_hwndSite)
#endif

#define ISWSX_EDGELESS(ws, wsx) \
    (!(((ws)&WS_THICKFRAME) || ((wsx)&WS_EX_WINDOWEDGE)))

#define ISWBM_EDGELESS(eMode) \
    ((eMode==WBM_BBOTTOMMOST && ISWSX_EDGELESS(WS_BBTMMOST, WS_EX_BBTMMOST)) \
    || (eMode==WBM_BOTTOMMOST && ISWSX_EDGELESS(WS_BTMMOST, WS_EX_BTMMOST)))

#define XY_NIL          ((LPARAM) (-1))

// drag state
#define DRAG_NIL        0       // nil
#define DRAG_MOVE       1       // moving
#define DRAG_SIZE       2       // sizing

extern void DBC_ExecDrag(IUnknown *pDbc, int eDragging);

#define WBMF_BROWSER    (0x0001000)        // hosted by browser (vs. by desktop)
#define WBM_BBOTTOMMOST (WBMF_BROWSER|WBM_BOTTOMMOST)
//#define WBM_BTOPMOST    (WBMF_BROWSER|WBM_TOPMOST)
#define WBM_BFLOATING   (WBMF_BROWSER|WBM_FLOATING)
#define WBM_BNIL        *** error! ***


//***   ISWBM_* -- check mode
#define ISWBM_FLOAT(eMode) \
                           (eMode & WBM_FLOATING)

#define ISWBM_BOTTOM(eMode) \
    ((eMode) == WBM_BOTTOMMOST || (eMode) == WBM_BBOTTOMMOST)

#if XXX_CHEEDESK
#define ISWBM_DESKTOP()     (!(_eMode & WBMF_BROWSER))
#else
#define ISWBM_DESKTOP()     (_fDesktop)
#endif

#if XXX_BROWSEROWNED
#define ISWBM_OWNED(eMode) ((!ISWBM_DESKTOP()) && \
    ((eMode) == WBM_BFLOATING || (eMode) == WBM_BBOTTOMMOST))
#else
#define ISWBM_OWNED(eMode) ((eMode) == WBM_BFLOATING)
#endif

#if XXX_HIDE
#if XXX_HIDEALL
#define ISWBM_HIDEABLE(eMode)   (((eMode) & ~WBMF_BROWSER) != WBM_FLOATING)
#else
#define ISWBM_HIDEABLE(eMode)   ((eMode) == WBM_TOPMOST)
#endif
#endif

#define CHKWBM_CHANGE(eModeNew, eModeCur) \
    (((eModeNew) & WBMF_BROWSER) == ((eModeCur) & WBMF_BROWSER) \
        || (eModeNew) == WBM_NIL || (eModeCur) == WBM_NIL) 


//***   timer stuff
#define IDT_POPUI       10
#define IDT_AUTOHIDE    11
#define IDT_AUTOUNHIDE  12

#define DLY_AUTOHIDE    500
#define DLY_AUTOUNHIDE  50

//***   hide state (_fHiding)
#define HIDE_FALSE      0       // must be FALSE
#define HIDE_AUTO       1       // currently hidden (due to autohide)
#define HIDE_MANUAL     2       // currently hidden (due to manual hide)

CASSERT(! HIDE_FALSE);


#if 0
//***   MKMS, MSTO* -- make/crack combined mode+side
//
#define MSTOMODE(ms)    (((UINT) (ms)) >> 8)
#define MSTOSIDE(ms)    (((UINT) (ms)) & 0x7)
#define MKMS(m, s)      ((((UINT) (m)) << 8) | (UINT) (s))
#endif

#if WEBBAR_APP
HMENU LoadMenuPopup(UINT id);
extern HINSTANCE        g_hinst;
#define HINST_THISDLL   g_hinst
#endif

// BUGBUG we can replace these once the portability layer is up and running.
#define Command_GetNotifyCode(wp,lp)    HIWORD(wp)
#define Command_GetHwndCtl(lp)          ((HWND)lp)
#define Command_GetID(wp)               LOWORD(wp)


CASSERT((ABE_LEFT|ABE_RIGHT|ABE_TOP|ABE_BOTTOM) == 3);  // must fit in _uSide:2

//***   MoveRect -- move left-top corner of rect to (x,y)
//
#define MoveRect(prc, x, y) \
    OffsetRect((prc), (x) - (prc)->left, (y) - (prc)->top)

#define AB_THEIGHT(rc)  (RECTHEIGHT(rc) * 10 / 100)     // 10%
#define AB_BHEIGHT(rc)  (RECTHEIGHT(rc) * 10 / 100)     // 10%
#define AB_LWIDTH(rc)   (40)                            // fixed width 40
#define AB_RWIDTH(rc)   ( RECTWIDTH(rc) * 35 / 100)     // 30%

void SlideWindow(HWND hwnd, RECT *prc, HMONITOR hMonClip, BOOL fShow);


class PropDataSet {
public:
    BOOL _fSet;
    DWORD _dwData;
};

class CDockingBarPropertyBag :
      public IPropertyBag
    , public IDockingBarPropertyBagInit
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IPropertyBag ***
    virtual HRESULT STDMETHODCALLTYPE Read( 
                                           /* [in] */ LPCOLESTR pszPropName,
                                           /* [out][in] */ VARIANT *pVar,
                                           /* [in] */ IErrorLog *pErrorLog);

    virtual HRESULT STDMETHODCALLTYPE Write( 
                                            /* [in] */ LPCOLESTR pszPropName,
                                            /* [in] */ VARIANT *pVar) {return E_NOTIMPL;};

    // *** IDockingBarPropertyBagInit
    virtual STDMETHODIMP SetDataDWORD(ENUMPROPDATA e, DWORD dwData) { _props[e]._fSet = TRUE; _props[e]._dwData = dwData; return S_OK; }
    
protected:
    CDockingBarPropertyBag() { _cRef = 1; };
    friend HRESULT CDockingBarPropertyBag_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

    UINT _cRef;
    
    PropDataSet _props[PROPDATA_COUNT];
    
}; 

#endif
