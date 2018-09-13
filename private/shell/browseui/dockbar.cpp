#include "priv.h"
#include "sccls.h"
#include "resource.h"
#include "mshtmhst.h"
#include "inpobj.h"
#include "desktopp.h"   // DTRF_RAISE etc.

#define WANT_CBANDSITE_CLASS
#include "bandsite.h"

#include "deskbar.h"
#include "multimon.h"
#include "mmhelper.h"
#include "theater.h"

#include "mluisupp.h"

#define SUPERCLASS CBaseBar

#define DM_PERSIST      0               // trace IPS::Load, ::Save, etc.
#define DM_POPUI        0               // expando-UI (proto)
#define DM_MENU         0               // trace menu code
#define DM_DRAG         0               // drag move/size (terse)
#define DM_DRAG2        0               // ... (verbose)
#define DM_API          0               // trace API calls
#define DM_HIDE         DM_TRACE               // autohide
#define DM_HIDE2        DM_TRACE               // autohide (verbose)
#define DM_APPBAR       0               // SHAppBarMessage calls
#define DM_OLECT        0               // IOleCommandTarget calls
#define DM_FOCUS        0               // focus change
#define DM_RES          DM_WARNING      // resolution

#define ABS(i)  (((i) < 0) ? -(i) : (i))

#define RECTGETWH(uSide, prc)   (ABE_HORIZ(uSide) ? RECTHEIGHT(*prc) : RECTWIDTH(*prc))

//***   CDB_INITED -- has CDockingBar::_Initialize been called
//
#define CDB_INITED()   (_eInitLoaded && _fInitSited && _fInitShowed)

enum ips_e {
    IPS_FALSE,    // reserved, must be 0 (FALSE)
    IPS_LOAD,
    IPS_LOADBAG,
    IPS_INITNEW,
    IPS_LAST
};

CASSERT(IPS_FALSE == 0);
CASSERT(((IPS_LAST - 1) & 0x03) == (IPS_LAST - 1)); // 2-bit _eInitLoaded


//***   CXFLOAT -- distance from edge to 'float' zone 
// NOTES
//  pls forgive the lousy hungarian...
#define CXFLOAT()   GetSystemMetrics(SM_CXICON)
#define CYFLOAT()   GetSystemMetrics(SM_CYICON)
#define CXYHIDE(uSide)  2       // BUGBUG GetSystemMetrics(xxx)

#ifdef DEBUG
#if 0   // turn on to debug autohide boundary cases
int g_cxyHide = 8;
#undef  CXYHIDE
#define CXYHIDE(uSide)  g_cxyHide
#endif
#endif

#define CXSMSIZE()  GetSystemMetrics(SM_CXSMSIZE)
#define CYSMSIZE()  GetSystemMetrics(SM_CYSMSIZE)


#ifdef DEBUG
extern unsigned long DbStreamTell(IStream *pstm);
extern BOOL DbCheckWindow(HWND hwnd, RECT *prcExp, HWND hwndClient);
TCHAR *DbMaskToMneStr(UINT uMask, TCHAR *szMnemonics);
#else
#define DbStreamTell(pstm) 0
#define DbCheckWindow(hwnd, prcExp, hwndClient) 0
#define DbMaskToMneStr(uMask, szMnemonics) szMnemonics
#endif

//***   autohide -- design note
//
// here's an overview of how we do autohide.  see the code for details.
//
// only a few routines really know about it.  their behavior is driven
// by '_fHiding'.  when FALSE, they behave normally.  when TRUE, they
// do alternate 'fake' behavior.
//
// a 'real' or 'normal' rect is the full-size rect we display when not hidden.
// a 'fake' or 'tiny' rect is the very thin rect we display when hidden.
// (plus there's a '0-width' rect we register w/ the system when we're hidden).
//
// more specifically,
//
// when fHiding is TRUE, a few routines have alternate 'fake' behavior:
//      _ProtoRect      returns a 'tiny' rect rather than the 'real' rect
//      _NegotiateRect  is a NOOP (so we don't change the 'tiny' rect) 
//      _SetVRect       is a NOOP (so we don't save   the 'tiny' rect)
//      AppBarSetPos    is a NOOP (so we don't set    the 'tiny' rect)
// plus, a few routines handle transitions (and setup):
//      _DoHide         hide/unhide helper
//      _MoveSizeHelper detects and handles transitions
//      _HideReg        register autohide appbar w/ 0-width rect
// and finally, a few messages trigger the transitions:
//      unhide          WM_NCHITTEST on the 'tiny' rect starts the unhide.
//              actually it starts a timer (IDT_AUTOUNHIDE) so there's a
//              bit of hysteresis.
//      hide            WM_ACTIVATE(deact) starts a timer (IDT_AUTOHIDE)
//              which we use to poll for mouse leave events.  again, there
//              is some hysteresis, plus some additional heuristics for hiding.
//              WM_ACTIVATE(act) stops the timer.
//
// #if 0
// we also have 'manual hide'.  manual hide differs from autohide as follows:
//     autohide never negotiates space(*)   , manual hide always does
//     autohide is focus- and cursor- driven, manual hide is UI-driven
//     (*) actually it negotiates space of '0'.
// e.g. 'manual hide' is used for the BrowserBar (e.g. search results).
// however for now at least 'manual hide' is simply a ShowDW(FALSE).
// #endif

void CDockingBar::_AdjustToChildSize()
{
    if (_szChild.cx)
    {
        RECT rc, rcChild;
    
        GetWindowRect(_hwnd, &rc);
        GetClientRect(_hwndChild, &rcChild);

        // we need to change rc by the delta of prc-rcChild
        rc.right += _szChild.cx - RECTWIDTH(rcChild);
        rc.bottom += _szChild.cy - RECTHEIGHT(rcChild);

        _SetVRect(&rc);
    
        _Recalc();  // _MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);

        _szChild.cx = 0;
    }
}

void CDockingBar::_OnPostedPosRectChange()
{
    if (_ptbSite) {
        if (!_fDragging) {
            _AdjustToChildSize();
        }
    }
}

HMENU CDockingBar::_GetContextMenu()
{
    HMENU hmenu = LoadMenuPopup(MENU_WEBBAR);
    if (hmenu) {

        // _eMode
        if (!ISWBM_DESKTOP())
        {
            EnableMenuItem(hmenu, IDM_AB_TOPMOST, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hmenu, IDM_AB_AUTOHIDE, MF_BYCOMMAND | MF_GRAYED);
        }
        CheckMenuItem(hmenu, IDM_AB_TOPMOST, WBM_IS_TOPMOST() ? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));

        // hide
        // we use _fWantHide (not _fCanHide) to reflect what user asked
        // for, not what he got.  o.w. you can't tell what the state is
        // unless you actually get it.
        CheckMenuItem(hmenu, IDM_AB_AUTOHIDE,
            MF_BYCOMMAND | (_fWantHide ? MF_CHECKED : MF_UNCHECKED));

        CASSERT(PARENT_XTOPMOST == HWND_DESKTOP);   // for WM_ACTIVATE
        CASSERT(PARENT_BTMMOST() == HWND_DESKTOP);  // for WM_ACTIVATE
        if (_eMode & WBM_FLOATING)
        {
            // (for now) only desktop btm/topmost does autohide
            EnableMenuItem(hmenu, IDM_AB_AUTOHIDE, MF_BYCOMMAND | MF_GRAYED);
        }

#ifdef DEBUG
        // BUGBUG temporary until we make browser tell us about activation
        CheckMenuItem(hmenu, IDM_AB_ACTIVATE,
            MF_BYCOMMAND | (_fActive ? MF_CHECKED : MF_UNCHECKED));
#endif

    }
    return hmenu;
}

HRESULT CDockingBar::_TrackPopupMenu(const POINT* ppt)
{
    HRESULT hres = S_OK;

    HMENU hmenu = _GetContextMenu();
    if (hmenu)
    {
        TrackPopupMenu(hmenu, /*TPM_LEFTALIGN|*/TPM_RIGHTBUTTON,
            ppt->x, ppt->y, 0, _hwnd, NULL);
        DestroyMenu(hmenu);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

void CDockingBar::_HandleWindowPosChanging(LPWINDOWPOS pwp)
{
}

/***
 */
LRESULT CDockingBar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    POINT pt;
    DWORD pos;
    RECT rc;

    switch (uMsg) {
    case WM_CLOSE:
        _AppBarOnCommand(IDM_AB_CLOSE);   // _RemoveToolbar(0)
        break;

    case WM_DESTROY:
        if (_fAppRegistered)
            _AppBarRegister(FALSE);
        break;

    case APPBAR_CALLBACK:
        _AppBarCallback(hwnd, uMsg, wParam, lParam);
        return 0;

    case WM_CONTEXTMENU:
        if (_CheckForwardWinEvent(uMsg, wParam, lParam, &lres))
            break;
            
        if ((LPARAM)-1 == lParam)
        {
            GetClientRect(_hwnd, &rc);
            MapWindowRect(_hwnd, HWND_DESKTOP, &rc);
            pt.x = rc.left + (rc.right - rc.left) / 2;
            pt.y = rc.top + (rc.bottom - rc.top) / 2;
        }
        else
        {
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
        }
        _TrackPopupMenu(&pt);
        break;

    case WM_ENTERSIZEMOVE:
        ASSERT(_fDragging == 0);
        _fDragging = 0;         // reset if busted
        _xyPending = XY_NIL;
#if XXX_CANCEL
        GetWindowRect(hwnd, &_rcCapture);      // to detect cancel
#endif
        break;

    case WM_SYSCHAR:
        if (wParam == TEXT(' '))
        {
            HMENU hmenu;

            hmenu = GetSystemMenu(hwnd, FALSE);
            if (hmenu) {
                EnableMenuItem(hmenu, SC_RESTORE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MAXIMIZE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MINIMIZE, MFS_GRAYED | MF_BYCOMMAND);
            }
        }
        goto DoDefault;

    case WM_SIZING:     // BUGBUG goto DoDefault?
    case WM_MOVING:
        {
            LPRECT prc = (RECT*)lParam;

            pos = GetMessagePos();
            if (_fDragging == 0)
            {
                // 1st time
                _DragEnter(uMsg, GET_X_LPARAM(pos), GET_Y_LPARAM(pos), prc);
                ASSERT(_fDragging != 0);
            }
            else
            {
                // 2nd..Nth time
                _DragTrack(uMsg, GET_X_LPARAM(pos), GET_Y_LPARAM(pos), prc, 0);
            }
        }
        return 1;

    case WM_MOVE:       // xLeft , yTop
    case WM_SIZE:       // xWidth, yHeight
        if (_fDragging)
        {
            RECT rcTmp;

            CopyRect(&rcTmp, &_rcPending);
            _DragTrack(uMsg, GET_X_LPARAM(_xyPending), GET_Y_LPARAM(_xyPending),
                &rcTmp, 1);
        }

        _OnSize();    // BUGBUG only needed for WM_SIZE?
        break;
        
    case WM_EXITSIZEMOVE:
        _DragLeave(-1, -1, TRUE);
        ASSERT(_fDragging == 0);

        break;

    case WM_CHILDACTIVATE:
        if (_eMode == WBM_BFLOATING)
            SendMessage(_hwnd, WM_MDIACTIVATE, (WPARAM)_hwnd, 0);
        goto DoDefault;
        
    case WM_WINDOWPOSCHANGING:
        _HandleWindowPosChanging((LPWINDOWPOS)lParam);
        break;

    case WM_WINDOWPOSCHANGED:
Lfwdappbar:
        if (_fAppRegistered)
            _AppBarOnWM(uMsg, wParam, lParam);
        goto DoDefault;        // fwd on so we'll get WM_SIZE etc.

    case WM_TIMER:
        switch (wParam) {

        case IDT_AUTOHIDE:
            {
                ASSERT(_fWantHide && _fCanHide);

                GetCursorPos(&pt);
                GetWindowRect(hwnd, &rc);
                // add a bit of fudge so we don't hide when trying to grab the edge
                InflateRect(&rc, GetSystemMetrics(SM_CXEDGE) * 4,
                    GetSystemMetrics(SM_CYEDGE)*4);

                HWND hwndAct = GetActiveWindow();

                if (!PtInRect(&rc, pt) && hwndAct != hwnd &&
                  (hwndAct == NULL || ::GetWindowOwner(hwndAct) != hwnd))
                  {
                    // to hide, we need to be outside the inflated window,
                    // and we can't be active (for keyboard users).
                    // (heuristics stolen from tray.c)
                    // BUGBUG tray.c also checks TM_SYSMENUCOUNT == 0

                    _DoHide(AHO_KILLDO|AHO_MOVEDO);
                }
            }
            break;

        case IDT_AUTOUNHIDE:    // BUGBUG share code w/ IDT_AUTOHIDE
            ASSERT(_fWantHide && _fCanHide);

            if (_fHiding)
            {
                ASSERT(_fHiding == HIDE_AUTO);
                GetCursorPos(&pt);
                GetWindowRect(hwnd, &rc);
                if (PtInRect(&rc, pt))
                    _DoHide(AHO_KILLUN|AHO_MOVEUN|AHO_SETDO);
                else
        Lkillun:
                    _DoHide(AHO_KILLUN);
            }
            else
            {
                // if we mouse-over and then TAB very quickly, we can end
                // up getting a WM_ACT followed by a WM_TIMER (despite the
                // KillTimer inside OnAct).  if so we need to be careful
                // not to do an AHO_SETDO.  just to be safe we do an
                // AHO_KILLUN as well.
                TraceMsg(DM_HIDE, "cwb.WM_T: !_fHiding (race!) => AHO_KILLUN");
                goto Lkillun;
            }
            break;

        default:
            goto DoDefault;
        }

        break;

    case WM_NCLBUTTONDOWN:
    case WM_LBUTTONDOWN:
        goto DoDefault;


    case WM_ACTIVATE:
        _OnActivate(wParam, lParam);
        goto Lfwdappbar;

    case WM_GETMINMAXINFO:  // prevent it from getting too small
        // n.b. below stuff works for scheme 'win standard large'
        // but not for v. large edges.  not sure why, but we'll
        // have to fix it or the original bug will still manifest
        // on accessibility-enabled machines.

        // nt5:149535: resize/drag of v. small deskbar.
        // BUGBUG workaround USER hittest bug for v. small windows.
        // DefWndProc(WM_NCHITTEST) gives wrong result (HTLEFT) when
        // window gets too small.  so stop it from getting v. small.
        //
        // the below calc actually gives us slightly *more* than the
        // min size, but what the heck.  e.g. it gives 8+15+1=24,
        // whereas empirical tests give 20.  not sure why there's a
        // diff, but we'll use the bigger # to be safe.
        {
            RECT rcTmp = {100,100,100,100}; // arbitrary 0-sized rect
            LONG ws, wsx;
            HWND hwndTmp;

            _GetStyleForMode(_eMode, &ws, &wsx, &hwndTmp);
            AdjustWindowRectEx(&rcTmp, ws, FALSE, wsx);

            ((MINMAXINFO *)lParam)->ptMinTrackSize.x = RECTWIDTH(rcTmp)  + CXSMSIZE() + 1;
            ((MINMAXINFO *)lParam)->ptMinTrackSize.y = RECTHEIGHT(rcTmp) + CYSMSIZE() + 1;
            if (ISWBM_FLOAT(_eMode))
            {
                // nt5:169734 'close' button on v. small floating deskbar.
                // BUGBUG workaround USER 'close' button bug for v. small windows.
                // the button on a v. small TOOLWINDOW doesn't work.
                // empirically the below adjustment seems to work.
                ((MINMAXINFO *)lParam)->ptMinTrackSize.x += (CXSMSIZE() + 1) * 3 / 2;
                ((MINMAXINFO *)lParam)->ptMinTrackSize.y += (CYSMSIZE() + 1) * 3 / 2;
            }
            TraceMsg(DM_TRACE, "cwb.GMMI: x=%d", ((MINMAXINFO *)lParam)->ptMinTrackSize.x);
        }
        break;

    case WM_NCHITTEST:
        return _OnNCHitTest(wParam, lParam);

    case WM_WININICHANGE:
        // Active Desktop *broadcasts* a WM_WININICHANGE SPI_SETDESKWALLPAPER
        // message when starting up. If this message gets processed during
        // startup at just the right time, then the bands will notify their
        // preferred state, and we lose the persisted state.  Since the desktop
        // wallpaper changing is really of no interest to us, we filter it out here.
        //
        // REVIEW CDTURNER: Would we get a perf win by punting a larger class
        // of these wininichange messages? It seems like most won't affect
        // the contents of a deskbar...
        //
        if (SPI_SETDESKWALLPAPER == wParam)
            break;

        goto DoDefault;
        
    default:
DoDefault:
        return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
    }

    return lres;
}

LRESULT CDockingBar::_OnNCHitTest(WPARAM wParam, LPARAM lParam)
{
    if (_fHiding)
        _DoHide(AHO_SETUN);

    // get 'pure' hittest...
    LRESULT lres = _CalcHitTest(wParam, lParam);

    // ... and perturb it based on where we're docked
    BOOL fSizing = FALSE;
    if (ISWBM_FLOAT(_eMode)) {
        // standard sizing/moving behavior
        return lres;
    }
    else {
        // opposing edge sizes; any other edge moves
        ASSERT(ISABE_DOCK(_uSide));
        switch (_uSide) {
        case ABE_LEFT:
            //  
            // Mirror the edges (since we are dealing with screen coord)
            // if the docked-window parent is mirrored. [samera]
            //
            if (IS_WINDOW_RTL_MIRRORED(GetParent(_hwnd)))
                fSizing = (lres==HTLEFT);
            else
                fSizing = (lres==HTRIGHT);
            break;
        case ABE_RIGHT:
            //  
            // Mirror the edges (since we are dealing with screen coord)
            // if the docked-window parent is mirrored.
            //
            if (IS_WINDOW_RTL_MIRRORED(GetParent(_hwnd)))
                fSizing = (lres==HTRIGHT);
            else
                fSizing = (lres==HTLEFT);
            break;
        case ABE_TOP:
            fSizing = (lres==HTBOTTOM);
            break;
        case ABE_BOTTOM:
            fSizing = (lres==HTTOP);
            break;

        default: 
            ASSERT(0); 
            break;
        }
    }

    if (!fSizing) {
        lres = HTCAPTION;
    }
    return lres;
}

//***   _OnActivate --
//
void CDockingBar::_OnActivate(WPARAM wParam, LPARAM lParam)
{
    TraceMsg(DM_HIDE, "cwb.WM_ACTIVATE wParam=%x", wParam);
    if (_fCanHide) {
        ASSERT(_fHiding != HIDE_MANUAL);
        if (LOWORD(wParam) != WA_INACTIVE) {
            // activate
            TraceMsg(DM_HIDE, "cdb._oa:  WM_ACT(act) _fHiding=%d", _fHiding);
            // turn off timers for perf
            // nash:40992: unhide if hidden (e.g. TABed to hidden)
            _DoHide(AHO_KILLDO|AHO_MOVEUN);
        }
        else {
            // deactivate
            _DoHide(AHO_SETDO);         // restore
        }
    }

    return;
}

/***
 */
CDockingBar::CDockingBar() : _eMode(WBM_NIL), _uSide(ABE_RIGHT)
{
    ASSERT(_fIdtUnHide == FALSE);
    ASSERT(_fIdtDoHide == FALSE);
    _ptIdtUnHide.x = _ptIdtUnHide.y = -1;

    // set up worst-case defaults.  we'll end up using them for:
    //     - some of them for Load(bag)
    //     - all  of them for InitNew()
    // note that we might call _InitPos4 again in SetSite.
    _InitPos4(TRUE);

    return;
}

//***   _Initialize -- 2nd-phase ctor
// NOTES
//  we need any IPS::Load settings and also a site before we can init
//  ourself, so most initialization waits until here.
void CDockingBar::_Initialize()
{
    ASSERT(!_fInitShowed);
    ASSERT(_fInitSited && _eInitLoaded);
    ASSERT(!CDB_INITED());

    _fInitShowed = TRUE;

    // warning: delicate phase-ordering here...
    UINT eModeNew = _eMode;
    _eMode = WBM_NIL;
    UINT uSideNew = _uSide;
    _uSide = ABE_NIL;
    HMONITOR hMonNew = _hMon;
    _hMon = NULL;
    // 48463: beta reports fault on boot when we have deskbar+taskbar on
    // same edge (non-merged).  i'm guessing (no proof) that shdocvw isn't
    // init'ed enough early on during boot to handle doing a MergeBS, or
    // alternately that there's a race btwn the tray and desktop threads.
    //
    // plus in any case we shouldn't do the merge just because the guy did
    // a logoff/logon!
    _SetModeSide(eModeNew, uSideNew, hMonNew, /*fNoMerge*/_eInitLoaded == IPS_LOAD);

    _NotifyModeChange(0);

    // if we have a bar on the right and we drag a band from it to
    // the top, we end up getting a sequence:
    //      create deskbar; AddBand; SetSite; _Initialize
    // the AddBand of the (1st) band tries to do an autosize but
    // there's no site yet, so nothing happens.
    //
    // so we need to force it here.
    _AdjustToChildSize();

    if (_fWantHide) {
        _fWantHide = FALSE;
        _AppBarOnCommand(IDM_AB_AUTOHIDE);
    }

    ASSERT(CDB_INITED());

    return;
}

/***
 */
CDockingBar::~CDockingBar()
{
    ASSERT(!_fAppRegistered);   // make sure _ChangeTopMost(WBM_NIL) was called

    // make sure SetSite(NULL); was called
    ASSERT(!_ptbSite);
    return;
}


void CDockingBar::_GetChildPos(LPRECT prc)
{
    GetClientRect(_hwnd, prc);
}

//***   _OnSize -- compute size for OC, leaving room for toolbar (caption?)
//
void CDockingBar::_OnSize(void)
{
    RECT rc;

    if (!_hwndChild || !_eInitLoaded)
        return;

    ASSERT(IsWindow(_hwndChild));

    // don't resize on a hide (it's temporary and we don't want things
    // to jerk around or worse still do a destructive reformat)
    // BUGBUG: should suppress resizing here in theater mode autohide
    // too (see theater.cpp)
    if (_fHiding)
        return;

    _GetChildPos(&rc);
    // (used to do ISWBM_EDGELESS 'fake edge' adjustments here, someone
    // nuked them, but should be o.k. now that visuals are frozen *provided*
    // we don't go back to edgeless)

    SetWindowPos(_hwndChild, 0,
            rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
            SWP_NOACTIVATE|SWP_NOZORDER);
    //ASSERT(DbCheckWindow(_hwndChild, &rc, xxx));
}

//***   _CalcHitTest --
// NOTES
//      really only has to return an int (win16?)
LRESULT CDockingBar::_CalcHitTest(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;

    if (!(ISWBM_BOTTOM(_eMode) && ISWBM_EDGELESS(_eMode))) {
        // For non-btmmost, we can ask USER to perform the default
        // hit testing.
        lRet = DefWindowProcWrap(_hwnd, WM_NCHITTEST, wParam, lParam);
    } else {
        // For btmmost, we need to do it.
        // (possibly dead code if bottom is never edgeless)
        // (if so, compiler should optimize it out)

        //TraceMsg(DM_WARNING, "cdb.ro: edgeless!");

        RECT rc;
        GetWindowRect(_hwnd, &rc);
        UINT x = GET_X_LPARAM(lParam);
        UINT y = GET_Y_LPARAM(lParam);
        // actually SM_C?SIZEFRAME is too big, but we get away w/ it
        // since we've been initiated by a WM_NCHITTEST so we know
        // we're on *some* edge
        UINT cx = GetSystemMetrics(SM_CXSIZEFRAME);
        UINT cy = GetSystemMetrics(SM_CYSIZEFRAME);
        if (_eMode == WBM_BBOTTOMMOST)
            cx *= 2;

        lRet = HTCAPTION;
        if (x > rc.right-cx) {
            lRet = HTRIGHT;
        } else if (x < rc.left+cx) {
            lRet = HTLEFT;
        } else if (y < rc.top+cy) {
            lRet = HTTOP;
        } else if (y > rc.bottom-cy) {
            lRet = HTBOTTOM;
        }
    }

    return lRet;
}

//***
//
void CDockingBar::_DragEnter(UINT uMsg, int x, int y, RECT* rcFeed)
{
    ASSERT(_fDragging == 0);

    if ((!(_eMode & WBM_FLOATING)) && uMsg == WM_MOVING)
    {
        // BUGBUG workaround USER non-full-drag drag rect bug
        // by forcing rcFeed back to exact current location (rather than
        // leaving at initial offset that USER gave us).
        //
        // w/o this code a drag from right to top in non-full-drag mode
        // will leave drag-rect droppings at the top of the original
        //
        // BUGBUG but, this seems to make things *worse* if !ISWBM_DESKTOP(),
        // so we don't do it in that case...  (sigh).
        _MoveSizeHelper(_eMode, _uSide, _hMon, NULL, rcFeed, FALSE, FALSE);
    }

    _eModePending = _eMode;
    _uSidePending = _uSide;
    _xyPending = MAKELPARAM(x, y);
    _hMonPending = _hMon;
    ASSERT(rcFeed != 0);
    if (rcFeed != 0)
        CopyRect(&_rcPending, rcFeed);

#if XXX_CANCEL
    RECT rcTmp;

    GetWindowRect(_hwnd, &rcTmp);
    TraceMsg(DM_DRAG2,
        "cwb.de: rcTmp=(%d,%d,%d,%d) (%dx%d) _rcCapture=(%d,%d,%d,%d) (%dx%d)",
        rcTmp.left, rcTmp.top, rcTmp.right, rcTmp.bottom,
        RECTWIDTH(rcTmp), RECTHEIGHT(rcTmp),
        _rcCapture.left, _rcCapture.top, _rcCapture.right, _rcCapture.bottom,
        RECTWIDTH(_rcCapture), RECTHEIGHT(_rcCapture));
#endif

    switch (uMsg) {
    case WM_MOVING: _fDragging = DRAG_MOVE; break;
    case WM_SIZING: _fDragging = DRAG_SIZE; break;

    default: ASSERT(0); break;
    }

    if (_fDragging == DRAG_MOVE) {
        // turn off size negotiation to prevent horz/vert pblms.
        //
        // e.g. when we drag a floating guy to horz/vert, there's
        // a period of time during which we have a horz/vert size,
        // but still think we're floating, which screws up size
        // negotiation royally.
        _ExecDrag(DRAG_MOVE);
    }

    return;
}

//***
//
void CDockingBar::_DragTrack(UINT uMsg, int x, int y, RECT* rcFeed, int eState)
{
#if DM_API
    TraceMsg(DM_DRAG2,
        "cwb.dt: API s=%d xy=(%d,%d) rc=(%d,%d,%d,%d) (%dx%d)",
        eState, x, y,
        _PM(rcFeed,left), _PM(rcFeed,right), _PM(rcFeed,bottom), _PM(rcFeed,right),
        _PX(rcFeed,RECTWIDTH(*rcFeed)), _PX(rcFeed,RECTHEIGHT(*rcFeed)));
#endif

    ASSERT(_fDragging != 0);

    switch (eState) {
    case 0:     // WM_MOVING
        {
            BOOL fImmediate = ((!_fDesktop) && uMsg == WM_SIZING) ? TRUE:FALSE;

            // remember for eventual commit
            _xyPending = MAKELPARAM(x, y);
            ASSERT(rcFeed != NULL);
            CopyRect(&_rcPending, rcFeed);

            // snap and give feedback
            _TrackSliding(x, y, rcFeed, fImmediate, fImmediate);

            break;
        }
    case 1:     // WM_MOVE
        TraceMsg(DM_DRAG2,
            "cwb.dt: %s _xyPend=(%d,%d) xy=(%d,%d)",
            (_xyPending != MAKELPARAM(x, y)) ? "noop/cancel" : "commit",
            GET_X_LPARAM(_xyPending), GET_Y_LPARAM(_xyPending), x, y);

        break;

    default: ASSERT(0); break;
    }

    return;
}

//***
//
void CDockingBar::_DragLeave(int x, int y, BOOL fCommit)
{
#if DM_API
    TraceMsg(DM_DRAG,
        "cwb.dl: API xy=(%d,%d) fCommit=%d",
        x, y, fCommit);
#endif

    if (_fDragging == 0) {
        // when we're inside a browser and you move the browser window
        // we get WM_ENTERSIZEMOVE/ WM_EXITSIZEMOVE but never any
        // WM_MOVING/WM_MOVE/WM_SIZING/WM_SIZE
        return;
    }

    switch (_fDragging) {
    case DRAG_MOVE:
    case DRAG_SIZE:
        break;
    default: ASSERT(0); break;
    }

#if XXX_CANCEL
    RECT rcTmp;

    GetWindowRect(_hwnd, &rcTmp);
    TraceMsg(DM_DRAG2,
        "cwb.dl: rcTmp=(%d,%d,%d,%d) (%dx%d) _rcCapture=(%d,%d,%d,%d) (%dx%d)",
        rcTmp.left, rcTmp.top, rcTmp.right, rcTmp.bottom,
        RECTWIDTH(rcTmp), RECTHEIGHT(rcTmp),
        _rcCapture.left, _rcCapture.top, _rcCapture.right, _rcCapture.bottom,
        RECTWIDTH(_rcCapture), RECTHEIGHT(_rcCapture));
    TraceMsg(DM_DRAG2, "cwb.dl: %s",
        EqualRect(&rcTmp, &_rcCapture) ? "noop/cancel" : "commit");
#endif

    BOOL fCancel = FALSE;       // BUGBUG todo: cancel NYI

    if (!fCancel) {
        if (_fDragging == DRAG_MOVE) {
            // nt5:187720 do this *before* the final move.
            // o.w. addr band ends up w/ 80-high default rather than
            // snapped to correct/negotiated size.
            //
            // why are we able to turn this on here when in general it
            // had to be off during the drag?  well, the preview of the
            // drag went thru MoveSizeHelper which did a NotifyModeChange
            // which told our client what its orientation really is.  so
            // by now things should be in sync.

            // size negotiation had been turned off (to prevent horz/vert
            // pblms).  turn it on before the final move so that we'll
            // recalc correctly.
            _ExecDrag(0);
        }

        // (we're done w/ _rcPending so o.k. to pass it in and trash it)
        // fMove==TRUE even though USER has already done the move for us,
        // since it's only done the move not the resize (?).  if we use
        // fMove==FALSE we end up in the new location but w/ the old size,
        // despite the fact that rcFeed has been updated along the way.
        // this is because USER sets SWP_NOSIZE when it does the move.
        _TrackSliding(GET_X_LPARAM(_xyPending), GET_Y_LPARAM(_xyPending),
            &_rcPending, TRUE, TRUE);

        // if we got a preferred child sizewhild dragging, set ourselves to that now.
        // sizing up   (cx > min), _szChild.cx == 0 and call a noop.
        // sizing down (cx < min), _szChild.cx != 0 and call does something.
        //ASSERT(_szChild.cx == 0);   // 0 => _AdjustToChildSize is nop
        _AdjustToChildSize();
    }
    else {
        _MoveSizeHelper(_eMode, _uSide, _hMon, NULL, NULL, TRUE, FALSE);   // BUGBUG fMove?
    }

    _fDragging = 0;
    
    return;
}

#ifdef DEBUG
int g_dbNoExecDrag = 0;     // to play w/ ExecDrag w/o recompiling
#endif

void DBC_ExecDrag(IUnknown *pDBC, int eDragging)
{
    VARIANTARG vaIn = {0};      // VariantInit

    ASSERT(eDragging == DRAG_MOVE || eDragging == 0);

#ifdef DEBUG
    if (g_dbNoExecDrag)
        return;
#endif

    vaIn.vt = VT_I4;
    vaIn.lVal = eDragging;      // n.b. currently only 0/1 is supported 
    IUnknown_Exec(pDBC, &CGID_DeskBarClient, DBCID_ONDRAG, OLECMDEXECOPT_DONTPROMPTUSER, &vaIn, NULL);
    // VariantClear

    return;
}

void CDockingBar::_ExecDrag(int eDragging)
{
    DBC_ExecDrag(_pDBC, eDragging);
    return;
}

//***   _Recalc -- force recalc using current settings
//
void CDockingBar::_Recalc(void)
{
    _MoveSizeHelper(_eMode, _uSide, _hMon, NULL, NULL, TRUE, TRUE);
    return;
}

//***   _MoveSizeHelper -- shared code for menu and dragging forms of move/size
//
void CDockingBar::_MoveSizeHelper(UINT eModeNew, UINT eSideNew, HMONITOR hMonNew,
    POINT* ptTrans, RECT* rcFeed, BOOL fCommit, BOOL fMove)
{
    UINT eModeOld, eSideOld;

    RECT rcNew;

    // only desktop guys can go to TOPMOST
    ASSERT(eModeNew != WBM_TOPMOST || ISWBM_DESKTOP());

    eModeOld = _eMode;
    eSideOld = _uSide;

    ASSERT(CHKWBM_CHANGE(eModeNew, _eMode));
    _eModePending = eModeNew;   // for drag feedback, before commit
    _uSidePending = eSideNew;
    _hMonPending = hMonNew;

    if (fCommit)
    {
        // we need to be careful when we call _ChangeHide or we'll recurse
        BOOL fChangeHide = (_fWantHide &&
            (eSideNew != _uSide || eModeNew != _eMode || hMonNew != _hMon));

        if (fChangeHide)
            _DoHide(AHO_KILLDO|AHO_UNREG);

        _SetModeSide(eModeNew, eSideNew, hMonNew, FALSE);

        if (fChangeHide)
        {
            // don't do AHO_SETDO now, wait for WM_ACTIVATE(deactivate)
            _DoHide(AHO_REG);
        }
    }

    // negotiate (and possibly commit to negotiation)
    _ProtoRect(&rcNew, eModeNew, eSideNew, hMonNew, ptTrans);
    _NegotiateRect(eModeNew, eSideNew, hMonNew, &rcNew, fCommit);

    // commit
    if (fCommit)
        _SetVRect(&rcNew);

    // feedback
    if (rcFeed != 0)
    {
        CopyRect(rcFeed, &rcNew);
    }
    
    if (fMove)
    {
        // If we're in theater mode, out parent manages our width and
        // horizontal position, unless we're being forced to a new
        // size by szChild.
        if (_fTheater && !_fDragging)
        {
            RECT rcCur;
            GetWindowRect(_hwnd, &rcCur);
            rcNew.left = rcCur.left;
            rcNew.right = rcCur.right;
        }

        // aka ScreenToClient
        MapWindowPoints(HWND_DESKTOP, GetParent(_hwnd), (POINT*) &rcNew, 2);
        
        if (_fCanHide && eModeNew == eModeOld && eSideNew == eSideOld)
        {
            // if we're [un]hiding to the same state, we can do SlideWindow
            ASSERT(ISWBM_HIDEABLE(eModeNew));
            DAD_ShowDragImage(FALSE);   // unlock the drag sink if we are dragging.
            SlideWindow(_hwnd, &rcNew, _hMon, !_fHiding);
            DAD_ShowDragImage(TRUE);    // restore the lock state.
        }
        else
        {
            MoveWindow(_hwnd, rcNew.left, rcNew.top,
                       RECTWIDTH(rcNew), RECTHEIGHT(rcNew), TRUE);
        }
    }

    // WARNING: rcNew is no longer in consistent coords! (ScreenToClient)

    // notify the child of changes
    _NotifyModeChange(0);
}

void CDockingBar::_NotifyModeChange(DWORD dwMode)
{
    UINT eMode, uSide;

    eMode = ((_fDragging == DRAG_MOVE) ? _eModePending : _eMode);
    uSide = ((_fDragging == DRAG_MOVE) ? _uSidePending : _uSide);
    //hMon = ((_fDragging == DRAG_MOVE) ? _hMonPending : _hMon);

    if (ISWBM_FLOAT(eMode))
        dwMode |= DBIF_VIEWMODE_FLOATING;
    else if (!ABE_HORIZ(uSide))
        dwMode |= DBIF_VIEWMODE_VERTICAL;

    SUPERCLASS::_NotifyModeChange(dwMode);
    
}

void CDockingBar::_TrackSliding(int x, int y, RECT* rcFeed,
    BOOL fCommit, BOOL fMove)
{
    TraceMsg(DM_DRAG2,
        "cwb.ts: _TrackSliding(x=%d, y=%d, rcFeed=(%d,%d,%d,%d)(%dx%d), fCommit=%d, fMove=%d)",
        x, y,
        _PM(rcFeed,left), _PM(rcFeed,top), _PM(rcFeed,right), _PM(rcFeed,bottom),
        _PX(rcFeed,RECTWIDTH(*rcFeed)), _PX(rcFeed,RECTHEIGHT(*rcFeed)),
        fCommit, fMove);

    POINT pt = { x, y };
    UINT eModeNew, uSideNew;
    HMONITOR hMonNew;
    if (_fDragging == DRAG_MOVE) {
        // moving...

        if (fCommit) {
            // use last feedback position.
            // o.w. (if we recompute) we end up in the wrong place since
            // WM_MOVE gives us the (left,top), which often is in another
            // docking zone.
            ASSERT(x == GET_X_LPARAM(_xyPending) && y == GET_Y_LPARAM(_xyPending));
            //eModeNew = _eModePending;
            //uSideNew = _uSidePending;
        }

        //
        // figure out snap position,
        // and do a few special-case hacks to fix it up if necessary
        //
        uSideNew = _CalcDragPlace(pt, &hMonNew);
        if (uSideNew == ABE_XFLOATING) {
            // dock->float or float->float
            eModeNew = _eMode | WBM_FLOATING;
            uSideNew = _uSide;          // BUGBUG _uSidePending?
        }
        else {
            // float->dock or dock->dock
            eModeNew = _eMode & ~WBM_FLOATING;
        }

        TraceMsg(DM_DRAG2,
            "cwb.ts: (m,s) _x=(%d,%d) _xPend=(%d,%d) xNew=(%d,%d)",
            _eMode, _uSide, _eModePending, _uSidePending, eModeNew, uSideNew);

        // 970725: we now allow bottom->float (for the desktop, not browser)
        if (ISWBM_FLOAT(eModeNew) && ISWBM_BOTTOM(_eMode) && !ISWBM_DESKTOP()) {
            // special case: don't allow switch from BTMMOST to FLOATING
            ASSERT(CHKWBM_CHANGE(eModeNew, _eMode));
            eModeNew = _eModePending;   // the dead zone...
            uSideNew = _uSidePending;   // BUGBUG init case?
            hMonNew = _hMonPending;
            TraceMsg(DM_DRAG2,
                "cwb.ts: (m,s) btm->flt override     xNew=(%d,%d)",
                eModeNew, uSideNew);
            ASSERT(!ISWBM_FLOAT(eModeNew));
        }

        //
        // smooth things out so we don't jump around
        //
        _SmoothDragPlace(eModeNew, uSideNew, hMonNew, &pt, rcFeed);

        //
        // now do the move
        //
        // | with _eMode & WBMF_BROWSER because dragging around doesn't change the
        // browser owned bit
        _MoveSizeHelper(eModeNew | (_eMode & WBMF_BROWSER), uSideNew, hMonNew, 
            ISWBM_FLOAT(eModeNew) ? &pt : NULL, rcFeed, fCommit, fMove);
    }
    else {
        ASSERT(_fDragging == DRAG_SIZE);

        // truncate to max size if necessary
        _SmoothDragPlace(_eMode, _uSide, _hMon, NULL, rcFeed);

        if (!fCommit) {
            // USER does everything for us
            return;
        }
        ASSERT(MAKELPARAM(x, y) != XY_NIL);

        // BUGBUG hack: we're gonna commit so just blast it in here...
        RECT rcNew;

        GetWindowRect(_hwnd, &rcNew);   // BUGBUG already set?
        _SetVRect(&rcNew);
        _MoveSizeHelper(_eMode, _uSide, _hMon,
            NULL, NULL,                 // BUGBUG &rcNew?
            fCommit, fMove);
    }

    return;
}

/***    _CalcDragPlace -- compute where drag will end up
 * NOTES
 *      BUGBUG prelim version
 */
UINT CDockingBar::_CalcDragPlace(POINT& pt, HMONITOR * phMon)
{
    TraceMsg(DM_DRAG2,
        "cwb.cdp: _CalcDragPlace(pt=(%d,%d))",
        pt.x, pt.y);

    SIZE screen, error;
    UINT uHorzEdge, uVertEdge, uPlace;
    RECT rcDisplay;

    // Get the correct hMonitor.
    ASSERT(phMon);
    *phMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    // BUGBUG todo: make hwndSite and rcDisplay args, then this can be
    // a generic helper func
    _GetBorderRect(*phMon, &rcDisplay);

    // if we're outside our 'parent' (browser or desktop), we float.
    // this really only applies to the browser case, since we'll never
    // be outside the desktop (BUGBUG what about multi-monitor?).
    if (!PtInRect(&rcDisplay, pt) || !_ptbSite) {
        TraceMsg(DM_DRAG2,
            "cwb.cdp: pt=(%d,%d) uSideNew=%u",
            pt.x, pt.y, ABE_XFLOATING);
        return ABE_XFLOATING;
    }

    // if we're not w/in min threshhold from edge, we float
    {
        RECT rcFloat;   // BUGBUG can just use rcDisplay
        int cx = CXFLOAT();
        int cy = CYFLOAT();

        CopyRect(&rcFloat, &rcDisplay);
        InflateRect(&rcFloat, -cx, -cy);

        if (PtInRect(&rcFloat, pt)) {
            TraceMsg(DM_DRAG2,
                "cwb.cdp: pt=(%d,%d) uSideNew=%u",
                pt.x, pt.y, ABE_XFLOATING);
            return ABE_XFLOATING;
        }
    }

    //
    // re-origin at zero to make calculations simpler
    //
    screen.cx =  RECTWIDTH(rcDisplay);
    screen.cy = RECTHEIGHT(rcDisplay);
    pt.x -= rcDisplay.left;
    pt.y -= rcDisplay.top;

    //
    // are we closer to the left or right side of this display?
    //
    if (pt.x < (screen.cx / 2)) {
        uVertEdge = ABE_LEFT;
        error.cx = pt.x;
    }
    else {
        uVertEdge = ABE_RIGHT;
        error.cx = screen.cx - pt.x;
    }

    //
    // are we closer to the top or bottom side of this display?
    //
    if (pt.y < (screen.cy / 2)) {
        uHorzEdge = ABE_TOP;
        error.cy = pt.y;
    }
    else {
        uHorzEdge = ABE_BOTTOM;
        error.cy = screen.cy - pt.y;
    }

    //
    // closer to a horizontal or vertical edge?
    //
    uPlace = ((error.cy * screen.cx) > (error.cx * screen.cy))?
        uVertEdge : uHorzEdge;

    TraceMsg(DM_DRAG2,
        "cwb.cdp: pt=(%d,%d) uSideNew=%u",
        pt.x, pt.y, uPlace);

    return uPlace;
}

//***   _SmoothDragPlace -- do some magic to smooth out dragging
// ENTRY/EXIT
//      eModeNew        where we're snapping to
//      eSideNew        ...
//      [_eModePending] where we're snapping from
//      [_eSidePending] ...
//      pt              INOUT cursor position
//      rcFeed          USER's original drag feedback rect
// NOTES
//      this is the place to put excel-like heuristics.  e.g. when coming
//      back off the right side we could put the cursor at the top right of
//      the floating rect (rather than the top left) to allow us to float
//      as close as possible to the other side w/o docking.  hmm, but how
//      would we tell USER where to put the cursor...
//
void CDockingBar::_SmoothDragPlace(UINT eModeNew, UINT eSideNew, HMONITOR hMonNew,
    INOUT POINT* pt, RECT* rcFeed)
{
    if (_fDragging == DRAG_MOVE) {
        if (ISWBM_FLOAT(eModeNew) && ISWBM_FLOAT(_eModePending) && rcFeed != 0) {
            // use the feedback rect from USER to keep things smooth.
            // o.w. if we use the cursor position we'll jump at the
            // beginning (to move the left-top corner to the starting
            // cursor position).
            pt->x = rcFeed->left;
            pt->y = rcFeed->top;
        }
    }
    else {
        ASSERT(_fDragging == DRAG_SIZE);
        ASSERT(eModeNew == _eMode && eSideNew == _uSide && hMonNew == _hMon);
        if (!ISWBM_FLOAT(_eMode)) {
            // truncate to max size (1/2 of screen) if necessary

            int iWH;
            RECT rcScreen;

            // we'd like to use 1/2 of browser, not 1/2 of screen.  however
            // this causes pblms if you maximize, grow to 1/2, and restore.
            // then the 1st time you resize the bar it 'jumps' down to 1/2
            // of the *current* size from 1/2 of the old size.  kind of a
            // hack, sigh...
            //
            // also note that there's still a bug here: if you size down
            // the browser gradually, we don't go thru this logic, so you
            // end up w/ a bar width > browser width so you the right edge
            // is clipped and there's no way to size it down.  probably
            // when the browser resize is done we should re-smooth the bar.
            //_GetBorderRect(_hMon, &rcScreen);
            GetMonitorRect(_hMon, &rcScreen);   // aka GetSystemMetrics(SM_CXSCREEN)
            iWH = RECTGETWH(_uSide, &rcScreen);
            iWH /= 2;
            if (RECTGETWH(_uSide, rcFeed) > iWH) {
                TraceMsg(DM_TRACE, "cwb.ts: truncate iWH'=%d", iWH);
                RectXform(rcFeed, RX_OPPOSE, rcFeed, NULL, iWH, _uSide, NULL);
            }
        }
    }

    return;
}

/***
 */
LRESULT CDockingBar::_OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    if (_CheckForwardWinEvent(uMsg, wParam, lParam, &lres))
        return lres;
    
    if ((Command_GetID(wParam) <= IDM_AB_LAST) &&
        (Command_GetID(wParam) >= IDM_AB_FIRST)) {
        _AppBarOnCommand(Command_GetID(wParam));
    }

    return lres;
}

/***    CDockingBar::_AppBarRegister -- register/unregister AppBar
 * DESCRIPTION
 *      updates _fAppRegistered
 *      does nothings if it's already regisrered or unregistered
 */
void CDockingBar::_AppBarRegister(BOOL fRegister)
{
    APPBARDATA abd;

    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = _hwnd;

    if (fRegister && !_fAppRegistered) {
        abd.uCallbackMessage = APPBAR_CALLBACK;
        TraceMsg(DM_APPBAR, "cwb.abr: call ABM_NEW");
        BOOL bT = SHAppBarMessage(ABM_NEW, &abd);
        ASSERT(bT);
        if (bT) {
            _fAppRegistered = TRUE;

            // fake a callback to set initial state
            // #if XXX_TASKMAN
            TraceMsg(DM_APPBAR, "cwb.abr: fake ABN_STATECHANGE");
            _AppBarCallback(_hwnd, APPBAR_CALLBACK, ABN_STATECHANGE, 0);
            // #endif
        }
    }
    else if (!fRegister && _fAppRegistered) {
        TraceMsg(DM_APPBAR, "cwb.abr: call ABM_REMOVE");
        // n.b. sensitive phase ordering, must set flag before send message
        // since the message causes a bunch of callbacks
        _fAppRegistered = FALSE;
        SHAppBarMessage(ABM_REMOVE, &abd);
    }
}

//***   _SetVRect -- set our 'virtual rect' to reflect window state
//
void CDockingBar::_SetVRect(RECT* rcNew)
{
    UINT eModeNew, uSideNew;

    //ASSERT(_fDragging == 0);  // o.w. we should look at _xxxPending

    eModeNew = _eMode;
    uSideNew = _uSide;


    if (_fHiding && ISWBM_HIDEABLE(eModeNew)) {
        TraceMsg(DM_HIDE, "cwb.svr: _fHiding => suppress rcNew=(%d,%d,%d,%d)",
            rcNew->left, rcNew->top, rcNew->right, rcNew->bottom);
        return;
    }


    if (ISWBM_FLOAT(eModeNew)) {
        CopyRect(&_rcFloat, rcNew);
    }
    else {
        _adEdge[uSideNew] = ABE_HORIZ(uSideNew) ? RECTHEIGHT(*rcNew) : RECTWIDTH(*rcNew);
    }
    return;
}

//***   _ChangeTopMost -- switch back and forth btwn TopMost and BottomMost
// ENTRY/EXIT
//      eModeNew        new mode we're switching to
//
void CDockingBar::_ChangeTopMost(UINT eModeNew)
{
    BOOL fShouldRegister = (eModeNew & WBM_TOPMOST) && !(eModeNew & WBM_FLOATING);
    
    // here's what's legal...
//              to...................
// from         btm     top     float
// ----         ---     ---     -----
// btm(desk)    -       top+    y(1)    (1) force to top
// top          top-    -       'undock'
// float        y(2)    'dock'  -       (2) force to right
// btm(app)     -       x(3)    y(4)    (3) foster child (4) 'owned' window


#if 0
    // (1,4) going from BTMMOST to FLOATING is illegal (and NYI) unless desktop
    ASSERT(eModeNew != WBM_FLOATING || _eMode != WBM_BOTTOMMOST || ISWBM_DESKTOP());
#endif

    // (3) only desktop guys can go to TOPMOST
    ASSERT(eModeNew != WBM_TOPMOST || ISWBM_DESKTOP());

    // _uSide should always be laying around (even if floating)
    ASSERT(_eMode == WBM_NIL || ISABE_DOCK(_uSide));

    // note the ordering here, make sure window bits are right
    // before doing resume new or else new will have unexpected state
    _ChangeWindowStateAndParent(eModeNew);
    _eMode = eModeNew;
    _ChangeZorder();

    // resume new
    switch (_eMode) {
    case WBM_NIL:
        // dummy state for termination
        return;

    case WBM_BOTTOMMOST:
        _ResetZorder();
#if ! XXX_BROWSEROWNED
        // fall through
    case WBM_BBOTTOMMOST:
#endif
        break;
    }
    
    _AppBarRegister(fShouldRegister);
}

//***   _ChangeZorder -- set z-order appropriately
// NOTES
//  currently doesn't account for 'raised' mode (i.e. caller must call
//  _ChangeZorder before _ResetZorder)
void CDockingBar::_ChangeZorder()
{
    BOOL fWantTopmost = BOOLIFY(WBM_IS_TOPMOST());
    BOOL fIsTopmost = BOOLIFY(GetWindowLong(_hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST);
    if (fWantTopmost != fIsTopmost)
        SetWindowPos(_hwnd, fWantTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    return;
}

//***    _ResetZorder -- toggle DockingBars between 'normal' and 'raised' mode
// DESCRIPTION
//  queries desktop state and does appropriate '_OnRaise'
//
void CDockingBar::_ResetZorder()
{
    HRESULT hr;
    VARIANTARG vaIn = {0};      // VariantInit
    VARIANTARG vaOut = {0};     // VariantInit

    vaIn.vt = VT_I4;
    vaIn.lVal = DTRF_QUERY;
    hr = IUnknown_Exec(_ptbSite, &CGID_ShellDocView, SHDVID_RAISE, OLECMDEXECOPT_DONTPROMPTUSER,
        &vaIn, &vaOut);
    if (SUCCEEDED(hr) && vaOut.vt == VT_I4)
        _OnRaise(vaOut.lVal);
    // VariantClear
    return;
}

//***   _OnRaise -- handle desktop 'raise' command
// DESCRIPTION
//  changes DockingBar z-order depending on desktop raise state:
//      desktop     DockingBar
//      raised      force on top (so visible)
//      restored    return to normal
// NOTES
//  BUGBUG should we handle WBM_FLOATING too?
//  BUGBUG should add ZORD_xxx to deskbar.h and handle non-WBM_BOTTOMMOST
void CDockingBar::_OnRaise(UINT flags)
{
    HWND hwndZorder;

    if (_eMode != WBM_BOTTOMMOST)
        return;

    switch (flags) {
    case DTRF_RAISE:
        hwndZorder = HWND_TOPMOST;
        break;

    case DTRF_LOWER:
        hwndZorder = HWND_NOTOPMOST;
        break;
    
    default:
        ASSERT(0);
        return;
    }

    SetWindowPos(_hwnd, hwndZorder, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

    return;
}

#if XXX_BTMFLOAT && 0 // see 'NOTES'
//***   _MayReWindow -- change/restore window state for dragging
// DESCRIPTION
//      USER won't let us drag outside of the browser unless we reparent
//      ourselves.
// NOTES
//      need to call this *before* we enter USER's move/size loop.
//      i.e. on LBUTTONDOWN and on EXITSIZEMOVE.  this gets a bit
//      tricky due to CANCEL etc., since the LBUTTONUP may come thru
//      either before or after we're done.
//
void CDockingBar::_MayReWindow(BOOL fToFloat)
{

    if (ISWBM_DESKTOP() || _eMode != WBM_BOTTOMMOST)
        return;

    // do style bits 1st or re-parenting breaks
    SHSetWindowBits(_hwnd, GWL_STYLE, WS_CHILD | WS_POPUP, fToFloat ? WS_POPUP | WS_CHILD);

    if (!fToFloat) {
        // float->btm

        // nuke owner
        SHSetParentHwnd(_hwnd, NULL);

        // parent
        SetParent(_hwnd, _hwndSite);
    }


    if (fToFloat) {
        // btm->float, set owner

        // parent
        SetParent(_hwnd, PARENT_FLOATING);

        // set owner
        ASSERT(_hwndSite != NULL);
        SHSetParentHwnd(_hwnd, _hwndSite);
    }
}
#endif


void CDockingBar::_GetStyleForMode(UINT eMode, LONG* plStyle, LONG* plExStyle, HWND* phwndParent)
{
    switch (eMode) {
    case WBM_NIL:
        *plStyle = WS_NIL;
        *plExStyle= WS_EX_NIL;
        *phwndParent = PARENT_NIL;
        break;

    case WBM_BBOTTOMMOST:
        *plStyle = WS_BBTMMOST;
        *plExStyle= WS_EX_BBTMMOST;
        *phwndParent = PARENT_BBTMMOST();
        break;

    case WBM_BOTTOMMOST:
        *plStyle = WS_BTMMOST;
        *plExStyle= WS_EX_BTMMOST;
        *phwndParent = PARENT_BTMMOST();
        break;

    case WBM_BFLOATING:
        // BUGBUG todo: FLOATING NYI
        *plStyle = WS_BFLOATING;
        *plExStyle = WS_EX_BFLOATING;
        *phwndParent = _hwndSite;
        break;

    case (WBM_FLOATING | WBM_TOPMOST):
    case WBM_FLOATING:
        // BUGBUG todo: FLOATING NYI
        *plStyle = WS_FLOATING;
        *plExStyle = WS_EX_FLOATING;
        *phwndParent = PARENT_FLOATING;
        break;

    case WBM_TOPMOST:
        *plStyle = WS_XTOPMOST;
        *plExStyle= WS_EX_XTOPMOST;
        *phwndParent = PARENT_XTOPMOST;
        break;
    }
#ifdef DEBUG // {
    if (_eMode == eMode) {
        // style, exstyle
        ASSERT(BITS_SET(GetWindowLong(_hwnd, GWL_STYLE), *plStyle));
        ASSERT(BITS_SET(GetWindowLong(_hwnd, GWL_EXSTYLE), *plExStyle & ~WS_EX_TOPMOST));

        // id
        ASSERT(GetWindowLong(_hwnd, GWL_ID) == 0);

        // parent 
        ASSERT(GetParent(_hwnd) == *phwndParent ||
               (ISWBM_OWNED(_eMode) && GetParent(_hwnd)==_hwndSite));
    }
#endif // }
}

//***   _ChangeWindowStateAndParent --
// NOTES
//      todo: make table-driven (ws1, ws2, etc.)
//
void CDockingBar::_ChangeWindowStateAndParent(UINT eModeNew)
{
    LONG ws1, wsx1, ws2, wsx2;
    HWND hwnd;

    if (eModeNew == _eMode) {
        // same mode, nothing to do
        return;
    }

    //
    // nuke old bits
    //
    _GetStyleForMode(_eMode, &ws1, &wsx1, &hwnd);


    //
    // set new bits
    //
    _GetStyleForMode(eModeNew, &ws2, &wsx2, &hwnd);

    // if it's going to be owned by the browser, 
    // override hwnd to our site's hwnd
    if (eModeNew & WBMF_BROWSER)
        hwnd = _hwndSite;

    // style, exstyle
    // (SWB can't do WS_EX_TOPMOST, we do it in caller w/ SWP)
    SHSetWindowBits(_hwnd, GWL_STYLE, ws1|ws2 , ws2);
    SHSetWindowBits(_hwnd, GWL_EXSTYLE, (wsx1|wsx2) & ~WS_EX_TOPMOST, wsx2);

    // id
    // (unchanged)
    HWND hwndParent = GetParent(_hwnd); 
    if (hwndParent != hwnd) {
        if (hwndParent != HWND_DESKTOP) {
            // float->btm, nuke owner
            SHSetParentHwnd(_hwnd, NULL);
        }

        // parent
        SetParent(_hwnd, hwnd);

        if (hwnd == _hwndSite) {
            // btm->float, set owner
            ASSERT(_hwndSite != NULL);
            SHSetParentHwnd(_hwnd, _hwndSite);
        }
    }
    //
    // force redraw
    //
    SetWindowPos(_hwnd, NULL, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

    return;
}

//***   _SetNewMonitor --
// When the Desktop is our Docking Site, set the new monitor I am on
// and return the old monitor 
HMONITOR CDockingBar::_SetNewMonitor(HMONITOR hMonNew)
{
    HMONITOR hMonOld = NULL;
    if (ISWBM_DESKTOP() && _ptbSite)
    {
        IMultiMonitorDockingSite * pdds;
        HRESULT hresT = _ptbSite->QueryInterface(IID_IMultiMonitorDockingSite, (LPVOID *)&pdds);
        if (SUCCEEDED(hresT))
        {
            HMONITOR hMon;
            ASSERT(pdds);
            if (SUCCEEDED(pdds->GetMonitor(SAFECAST(this, IDockingWindow*), &hMon)))
            {
                if (hMon != hMonNew)
                {
                    pdds->RequestMonitor(SAFECAST(this, IDockingWindow*), &hMonNew);
                    pdds->SetMonitor(SAFECAST(this, IDockingWindow*), hMonNew, &hMonOld);
                    // These two should be the same, otherwise something wierd is happening -- dli
                    ASSERT(hMonOld == hMon);
                }
            }
            pdds->Release();
        }
    }
    
    return hMonOld;
}

//***   _GetBorderRect --
// NOTES
//      result in screen coordinates
//
void CDockingBar::_GetBorderRect(HMONITOR hMon, RECT* prc)
{
    if (!ISWBM_BOTTOM(_eMode)) {
        // BUGBUG todo: should use:
        // floating: _hwndSite (not strictly correct, but good enough)
        // topmost: UnionRect of:
        //     GetWindowRect(_hwndSite);        // non-appbar rect
        //     GetWindowRect(self)              // plus my personal appbar
        ASSERT(IsWindow(_hwndSite));
        if (ISWBM_DESKTOP())
            GetMonitorRect(hMon, prc);
        else
            GetWindowRect(_hwndSite, prc);  
#ifdef DEBUG
#if 0
        RECT rcTmp;

        // these asserts often fail.  e.g. when dragging topmost right->top.
        // weird: _hwndSite ends up being PROGMAN's hwnd.
        // weird: also, the GetWindowRect fails.
        ASSERT(_hwndSite == PARENT_XTOPMOST);
        // _hwndSite is PROGMAN
        GetWindowRect(PARENT_XTOPMOST, &rcTmp);
        ASSERT(EqualRect(prc, &rcTmp));
#endif
#endif
    }
    else if (_ptbSite) {
        HMONITOR hMonOld = _SetNewMonitor(hMon);
        _ptbSite->GetBorderDW(SAFECAST(this, IDockingWindow*), prc);
        if (hMonOld)
            _SetNewMonitor(hMonOld);
        ASSERT(_hwndSite != NULL);
        //ASSERT(GetParent(_hwnd) == _hwndSite);  // BUGBUG ISWBM_OWNED?
        // convert if necessary
        // aka ClientToScreen
        MapWindowPoints(_hwndSite, HWND_DESKTOP, (POINT*) prc, 2);
    }

    return;
}

//***   _HideRegister -- (un)register auto-hide w/ edge
// ENTRY/EXIT
//      fToHide         TRUE if turning AutoHide on, FALSE if turning off
//      _fCanHide       [OUT] TRUE if successfully set autohide on; o.w. FALSE 
//      other           pops up dialog if operation fails
//
void CDockingBar::_HideRegister(BOOL fToHide)
{
    BOOL fSuccess;
    APPBARDATA abd;

    if (! ISWBM_HIDEABLE(_eMode))
        return;

    // (try to) register or unregister it
    // n.b. we're allowed to do this even if we're not an AppBar
    // that's good, because we want at most one autohide deskbar 
    // on an edge regardless of mode
    abd.cbSize = SIZEOF(abd);
    abd.hWnd = _hwnd;
    abd.uEdge = _uSide;
    abd.lParam = fToHide;

    // BUGBUG should we do a ABM_GETAUTOHIDEBAR at some point?
    // (tray.c does, and so does the AB sample code...)
    //ASSERT(_fAppRegistered);
    fSuccess = (BOOL) SHAppBarMessage(ABM_SETAUTOHIDEBAR, &abd);

    // set our state
    _fCanHide = BOOLIFY(fSuccess);
    // BUGBUG how handle failure?

    // init some stuff
    if (fToHide)
    {
        if (_fCanHide)
        {
            RECT rc;

            ASSERT(_fCanHide);  // so we won't SetVRect

            ASSERT(!_fHiding);  // (paranoia)

            // force a '0-width' rectangle so we don't take up any space
            RectXform(&rc, RX_EDGE|RX_OPPOSE|RX_ADJACENT, &rc, NULL,
                0, _uSide, _hMon);

            switch (_eMode) {
            case WBM_TOPMOST:
                // negotiate/commit it
                APPBARDATA abd;
                abd.cbSize = sizeof(APPBARDATA);
                abd.hWnd = _hwnd;
                ASSERT(_fCanHide);
                // we used to do:
                //  _fCanHide = FALSE;  // hack: so we surrender AppBar's space
                //  AppBarQuerySetPos(&rc, _uSide, &rc, &abd, TRUE);
                //  _fCanHide = TRUE;   // hack: restore
                // but the instant we do the ABSetPos the shell does a recalc
                // by doing a ShowDW of all toolbars, which does a _Recalc,
                // which does a MSH, which ends up doing ProtoRect w/ our
                // 'temporary' _fCanHide=0, which ends up taking space (oops!).
                //
                // so instead we call the low-level ABQueryPos/ABSetPos guys
                // directly.
                AppBarQueryPos(&rc, _uSide, _hMon, &rc, &abd, TRUE);
                AppBarSetPos0(_uSide, &rc, &abd);
                break;
            }

            // do *not* start the hide here
            // it's up to the caller, since a) might want delay or
            // immediate and b) recursion pblms w/ _MoveSizeHelper
        }
        else
        {
            // BUGBUG do PostMessage a la tray.c?
            MLShellMessageBox(_hwnd,
                MAKEINTRESOURCE(IDS_ALREADYAUTOHIDEBAR),
                MAKEINTRESOURCE(IDS_WEBBARTITLE),
                MB_OK | MB_ICONINFORMATION);
            ASSERT(!_fCanHide);
        }
    }
    else
    {
        // do *not* start the unhide here
        // it's up to the caller, since a) might want delay or
        // immediate and b) recursion pblms w/ _MoveSizeHelper

        _fCanHide = FALSE;
    }

    return;
}

//***   IsNearPoint -- am i currently near specified point?
// ENTRY/EXIT
//  pptBase     (INOUT) IN previous cursor pos, OUT updated to current if !fNear
//  fNear       (ret) TRUE if near, o.w. FALSE
// NOTES
//  heuristic stolen from explorer/tray.c!TraySetUnhideTimer
//
BOOL IsNearPoint(/*INOUT*/ POINT *pptBase)
{
    POINT ptCur;
    int dx, dy, dOff, dNear;

    GetCursorPos(&ptCur);
    dx = pptBase->x - ptCur.x;
    dy = pptBase->y - ptCur.y;
    dOff = dx * dx + dy * dy;
    dNear = GetSystemMetrics(SM_CXDOUBLECLK) * GetSystemMetrics(SM_CYDOUBLECLK);
    if (dOff <= dNear)
        return TRUE;
    TraceMsg(DM_HIDE2, "cwb.inp: ret=0 dOff=%d dNear=%d", dOff, dNear);
    *pptBase = ptCur;
    return FALSE;
}

//***   _DoHide --
// DESCRIPTION
//      AHO_KILLDO              kill timer for 'do'   operation
//      AHO_SETDO               set  timer for 'do'   operation
//      AHO_KILLUN              kill timer for 'undo' operation
//      AHO_SETUN               set  timer for 'undo' operation
//      AHO_REG                 register
//      AHO_UNREG               unregister
//      AHO_MOVEDO              do the actual hide
//      AHO_MOVEUN              do the actual unhide
// NOTES
//  the _fIdtXxHide stuff stops us from doing a 2nd SetTimer before the
// 1st one comes in, which makes us never get the 'earlier' ticks.
//  this fixes nt5:142686: drag-over doesn't unhide.  it was caused by us
// getting a bunch of WM_NCHITTESTs in rapid succession (OLE asking us on
// a fast timer?).
//  BUGBUG i think there's a tiny race window on _fIdtXxHide (between the
// call to Set/Kill and the shadowing in _fIdtXxHide).  not sure we can
// even hit it, but if we do, i think the worst that happens is somebody
// doesn't hide or unhide for a while.
//
void CDockingBar::_DoHide(UINT uOpMask)
{
    TraceMsg(DM_HIDE, "cwb.dh enter(uOpMask=0x%x(%s))",
        uOpMask, DbMaskToMneStr(uOpMask, AHO_MNE));

    if (!ISWBM_HIDEABLE(_eMode)) {
        TraceMsg(DM_HIDE, "cwb.dh !ISWBM_HIDEABLE(_eMode) => suppress");
        return;
    }

    // nuke old timer
    if (uOpMask & AHO_KILLDO) {
        TraceMsg(DM_HIDE, "cwb.dh: KillTimer(idt_autohide)");
        KillTimer(_hwnd, IDT_AUTOHIDE);
        _fIdtDoHide = FALSE;
    }
    if (uOpMask & AHO_KILLUN) {
        TraceMsg(DM_HIDE, "cwb.dh: KillTimer(idt_autoUNhide)");
        KillTimer(_hwnd, IDT_AUTOUNHIDE);
        _fIdtUnHide = FALSE;
        _ptIdtUnHide.x = _ptIdtUnHide.y = -1;
    }

    if (uOpMask & (AHO_REG|AHO_UNREG)) {
        _HideRegister(uOpMask & AHO_REG);
    }

    if (uOpMask & (AHO_MOVEDO|AHO_MOVEUN)) {
        // tricky, tricky...
        // all the smarts are in _MoveSizeHelper, driven by _fHiding (and _fCanHide)
        // use correct one of (tiny,real)
        _fHiding = (uOpMask & AHO_MOVEDO) ? HIDE_AUTO : FALSE;

        TraceMsg(DM_HIDE, "cwb.dh: move _fHiding=%d", _fHiding);
        ASSERT(_fCanHide);                      // suppress SetVRect
        _Recalc();  // _MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);
    }

    // start new timer
    if (_fCanHide) {
        if (uOpMask & AHO_SETDO) {
            TraceMsg(DM_HIDE, "cwb.dh: SetTimer(idt_autohide) fAlready=%d", _fIdtDoHide);
            if (!_fIdtDoHide) {
                _fIdtDoHide = TRUE;
                SetTimer(_hwnd, IDT_AUTOHIDE, DLY_AUTOHIDE, NULL);
            }
        }
        if (uOpMask & AHO_SETUN) {
            TraceMsg(DM_HIDE, "cwb.dh: SetTimer(idt_autoUNhide) fAlready=%d", _fIdtUnHide);
            // IsNearPoint hysteresis prevents us from unhiding when we happen
            // to be passed over on the way to something unrelated
            if (!IsNearPoint(&_ptIdtUnHide) || !_fIdtUnHide) {
                _fIdtUnHide = TRUE;
                SetTimer(_hwnd, IDT_AUTOUNHIDE, DLY_AUTOUNHIDE, NULL);
            }
        }
    }
    else {
#ifdef DEBUG
        if ((uOpMask & (AHO_SETDO|AHO_SETUN))) {
            TraceMsg(DM_HIDE, "cwb.dh: !_fCanHide => suppress AHO_SET*");
        }
#endif
    }

    return;
}

//***   SlideWindow -- sexy slide effect
// NOTES
//      stolen from tray.c
void SlideWindow(HWND hwnd, RECT *prc, HMONITOR hMonClip, BOOL fShow)
{
    RECT rcMonitor, rcClip;
    BOOL fRegionSet = FALSE; 

    SetRectEmpty(&rcMonitor);
    if (GetNumberOfMonitors() > 1)
    {
        GetMonitorRect(hMonClip, &rcMonitor);
        // aka ScreenToClient
        MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (LPPOINT)&rcMonitor, 2);     }

    // Future: We could loop on the following code for the slide effect 
    IntersectRect(&rcClip, &rcMonitor, prc);
    if (!IsRectEmpty(&rcClip))
    {
        HRGN hrgnClip;

        // Change the clip region to be relative to the upper left corner of prc
        // NOTE: this is not converting rcClip to prc client coordinate
        OffsetRect(&rcClip, -prc->left, -prc->top);
        
        hrgnClip = CreateRectRgnIndirect(&rcClip);
        // LINTASSERT(hrgnClip || !hgnClip);    // 0 semi-ok for SetWindowRgn
        // nt5:149630: always repaint, o.w. auto-unhide BitBlt's junk
        // from hide position
        fRegionSet = SetWindowRgn(hwnd, hrgnClip, /*fRepaint*/TRUE);
    }
    MoveWindow(hwnd, prc->left, prc->top, RECTWIDTH(*prc), RECTHEIGHT(*prc), TRUE);

    // Turn off the region stuff if we don't hide any more
    if (fRegionSet && fShow)
        SetWindowRgn(hwnd, NULL, TRUE);

    return;
}

/***    AppBarQueryPos -- negotiate position
 * ENTRY/EXIT
 *      return  width (height) from docked edge to opposing edge
 */
int CDockingBar::AppBarQueryPos(RECT* prcOut, UINT uEdge, HMONITOR hMon, const RECT* prcReq,
    PAPPBARDATA pabd, BOOL fCommit)
{
    int iWH;

    ASSERT(ISWBM_DESKTOP());

    // snap to edge (in case another AppBar disappeared w/o us knowing),
    // readjust opposing side to reflect that snap,
    // and max out adjacent sides to fill up full strip.
    iWH = RectGetWH(prcReq, uEdge);
    
    RectXform(&(pabd->rc), RX_EDGE|RX_OPPOSE|RX_ADJACENT|(_fHiding ? RX_HIDE : 0), prcReq, NULL, iWH, uEdge, hMon);

    ASSERT(EqualRect(&(pabd->rc), prcReq));     // caller guarantees?

    // negotiate
    // if we're dragging we might not be registered yet (floating->docked)
    // in that case we'll just use the requested size (w/o negotiating).
    // ditto for if we're in the middle of a top/non-top mode switch.
    if (_fAppRegistered) {
        pabd->uEdge = uEdge;
        TraceMsg(DM_APPBAR, "cwb.abqp: call ABM_QUERYPOS");
        SHAppBarMessage(ABM_QUERYPOS, pabd);
    }

    // readjust opposing side to reflect the negotiation (which only
    // adjusts the moved edge-most side, not the opposing edge).
    // BUGBUG: (dli) need to find the right hmonitor to pass  in
    RectXform(prcOut, RX_OPPOSE, &(pabd->rc), NULL, iWH, uEdge, hMon);

    return RectGetWH(prcOut, uEdge);
}

//***   AppBarSetPos --
// NOTES
//      does *not* do _SetVRect and MoveWindow, that's up to caller
//
void CDockingBar::AppBarSetPos(UINT uEdge, const RECT* prcReq, PAPPBARDATA pabd)
{
    ASSERT(_eMode == WBM_TOPMOST);

    if (!_fCanHide && _fAppRegistered)
        AppBarSetPos0(uEdge, prcReq, pabd);

    return;
}

void CDockingBar::AppBarSetPos0(UINT uEdge, const RECT* prcReq, PAPPBARDATA pabd)
{
    CopyRect(&(pabd->rc), prcReq);
    pabd->uEdge = uEdge;

    TraceMsg(DM_APPBAR, "cwb.absp: call ABM_SETPOS");
    ASSERT(_fAppRegistered);
    SHAppBarMessage(ABM_SETPOS, pabd);

    // BUGBUG workaround explorer bug: during dragging we get:
    //  querypos*; wm_winposchanged; querypos; setpos
    // the lack of a wm_winposchanged at the end screws up the
    // autohide bring-to-top code.
    ASSERT(pabd->cbSize == sizeof(APPBARDATA));
    ASSERT(pabd->hWnd == _hwnd);
    TraceMsg(DM_APPBAR, "cwb.absp: call ABM_WINPOSCHGED");
    SHAppBarMessage(ABM_WINDOWPOSCHANGED, pabd);

    // n.b. _SetVRect and MoveWindow done by caller

    return;
}

//***   AppBarQuerySetPos --
//
void CDockingBar::AppBarQuerySetPos(RECT* prcOut, UINT uEdge, HMONITOR hMon, const RECT* prcReq,
    PAPPBARDATA pabd, BOOL fCommit)
{
    RECT rcTmp;

    if (prcOut == NULL)
        prcOut = &rcTmp;

    AppBarQueryPos(prcOut, uEdge, hMon, prcReq, pabd, fCommit);
    if (fCommit) {
        AppBarSetPos(uEdge, prcOut, pabd);
        ASSERT(EqualRect(prcOut, &(pabd->rc))); // callers assume prcOut correct
    }

    return;
}

void CDockingBar::_AppBarOnSize()
{
    RECT rc;
    APPBARDATA abd;

    ASSERT(_eMode == WBM_TOPMOST);
    ASSERT(ISABE_DOCK(_uSide));

    if (!_fAppRegistered)
        return;

    // don't commit until done
    if (_fDragging)
        return;

    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = _hwnd;

    GetWindowRect(_hwnd, &rc);
    AppBarQuerySetPos(NULL, _uSide, _hMon, &rc, &abd, TRUE);

    return;
}

void CDockingBar::_RemoveToolbar(DWORD dwFlags)
{
    if (_ptbSite) {
        // WM_DESTROY will do _ChangeTopMost(WBM_NIL) for us

        IDockingWindowFrame* ptbframe;
        HRESULT hresT=_ptbSite->QueryInterface(IID_IDockingWindowFrame, (LPVOID*)&ptbframe);
        if (SUCCEEDED(hresT)) {
            AddRef();   // guard against self destruction
            ptbframe->RemoveToolbar(SAFECAST(this, IDockingWindow*), dwFlags);
            ptbframe->Release();
            Release();
        }
    } else {
        CloseDW(0);
    }
}

void CDockingBar::_AppBarOnCommand(UINT idCmd)
{
    UINT eModeNew;

    switch (idCmd) {
    case IDM_AB_TOPMOST:
        eModeNew = _eMode ^ WBM_TOPMOST;
        _MoveSizeHelper(eModeNew, _uSide, _hMon, NULL, NULL, TRUE, TRUE);
        break;

    case IDM_AB_AUTOHIDE:
        if (_fWantHide)
        {
            // on->off
            _DoHide(AHO_KILLDO|AHO_UNREG);      // _ChangeHide
            _fWantHide = FALSE;
        }
        else
        {
            // off->on
            _fWantHide = TRUE;
            // don't do AHO_SETDO now, wait for WM_ACTIVATE(deactivate)
            _DoHide(AHO_REG);     // _ChangeHide
        }

        // force it to happen *now*
        // BUGBUG potential race condition w/ the AHO_SETDO above,
        // but worst case that should cause a 2nd redraw (?).
        _Recalc();  // _MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);

        if (SHIsChildOrSelf(GetActiveWindow(), _hwnd) != S_OK)
        {
            // nt5:148444: if we're already deactive, we need to kick off
            // the hide now.  this is needed e.g. for login when we load
            // persisted auto-hide deskbars.  they come up inactive so we
            // never get the initial deact to hide them.
            _OnActivate(MAKEWPARAM(WA_INACTIVE, FALSE), (LPARAM)(HWND)0);
        }

        break;
#ifdef DEBUG
    case IDM_AB_ACTIVATE:
        // BUGBUG temporary until we make browser tell us about activation

        // note that since we're faking this w/ a menu our (normal) assumption
        // in WM_ENTERMENU is bogus so make sure you keep the mouse over
        // the BrowserBar during activation or it will hide away out from under
        // you and the Activate won't work...
        _OnActivate(MAKEWPARAM(_fActive ? WA_INACTIVE : WA_ACTIVE, FALSE),
            (LPARAM) (HWND) 0);
        _fActive = !_fActive;
        break;
#endif

    case IDM_AB_CLOSE:
        _OnCloseBar(TRUE);
        break;

    default:
        MessageBeep(0);
        break;
    }
}

BOOL CDockingBar::_OnCloseBar(BOOL fConfirm)
{
    _RemoveToolbar(0);
    return TRUE;
}

void CDockingBar::_AppBarOnWM(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_WINDOWPOSCHANGED:
    case WM_ACTIVATE:
        {
            APPBARDATA abd;

            abd.cbSize = sizeof(APPBARDATA);
            abd.hWnd = _hwnd;
            abd.lParam = (long) NULL;
            if (uMsg == WM_WINDOWPOSCHANGED) {
                TraceMsg(DM_APPBAR, "cwb.WM_WPC: call ABM_WINPOSCHGED");
                SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
            }
            else {
                //if (LOWORD(wParam) != WA_INACTIVE)
                // just do it always, doesn't hurt...
                TraceMsg(DM_APPBAR, "cwb.WM_ACT: call ABM_ACTIVATE");
                SHAppBarMessage(ABM_ACTIVATE, &abd);
            }
        }
        break;

    default:
        ASSERT(0);
        break;
    }

    return;
}

// try to preserve our thinkness
void CDockingBar::_AppBarOnPosChanged(PAPPBARDATA pabd)
{
    RECT rcWindow;

    ASSERT(_eMode == WBM_TOPMOST);

    GetWindowRect(pabd->hWnd, &rcWindow);
    RectXform(&rcWindow, RX_EDGE|RX_OPPOSE, &rcWindow, NULL, RectGetWH(&rcWindow, _uSide), _uSide, _hMon);

    _Recalc();  // _MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);
    return;
}

/***    _InitPos4 -- initialize edge positions
 * ENTRY/EXIT
 *  fCtor       TRUE if called from constructor; o.w. FALSE
 */
void CDockingBar::_InitPos4(BOOL fCtor)
{
    RECT rcSite;

    TraceMsg(DM_PERSIST, "cdb.ip4(fCtor=%d) enter", fCtor);

    if (fCtor)
    {
        // set some worst-case defaults for the Load(bag) case
        _adEdge[ABE_TOP]    = 80;
        _adEdge[ABE_BOTTOM] = 80;
        _adEdge[ABE_LEFT]   = 80;
        _adEdge[ABE_RIGHT]  = 80;

        SetRect(&_rcFloat, 10, 10, 310, 310);       // BUGBUG todo: NYI
        _hMon = GetPrimaryMonitor();
    }
    else
    {
        // set up semi-reasonable defaults for the InitNew case
        ASSERT(_eInitLoaded == IPS_INITNEW);    // not req'd, but expected
        ASSERT(IsWindow(_hwndSite));
        GetWindowRect(_hwndSite, &rcSite);

        _adEdge[ABE_TOP]    = AB_THEIGHT(rcSite);
        _adEdge[ABE_BOTTOM] = AB_BHEIGHT(rcSite);
        _adEdge[ABE_LEFT]   = AB_LWIDTH(rcSite);
        _adEdge[ABE_RIGHT]  = AB_RWIDTH(rcSite);
        
        // BUGBUG: (dli) should we ask _hwndSite for it's hmonitor?
        _hMon = MonitorFromRect(&rcSite, MONITOR_DEFAULTTONULL);
        if (!_hMon)
        {
            POINT ptCenter;
            ptCenter.x = (rcSite.left + rcSite.right) / 2;
            ptCenter.y = (rcSite.top + rcSite.bottom) / 2;
            _hMon = MonitorFromPoint(ptCenter, MONITOR_DEFAULTTONEAREST);
        }

    }

    return;
}

/***    RectXform -- transform RECT
 * ENTRY/EXIT
 *      prcOut
 *      uRxMask
 *      prcIn           initial rect
 *      prcBound        bounding rect specifying min/max dimensions
 *      iWH
 *      uSide
 * DESCRIPTION
 *      RX_EDGE         set edgemost side  to extreme (0 or max)
 *      RX_OPPOSE       set opposing side  to edge + width
 *      RX_ADJACENT     set adjacent sides to extremes (0 and max)
 *      RX_GETWH        get distance to opposing side
 *
 *      Two common calls are:
 *      ...
 * NOTES
 *      Note that rcOut, rcIn, and rcSize can all be the same.
 *      BUGBUG do we ever use rcIn or can it be rcBound (or NULL)?
 */
int CDockingBar::RectXform(RECT* prcOut, UINT uRxMask,
    const RECT* prcIn, RECT* prcBound, int iWH, UINT uSide, HMONITOR hMon)
{
    RECT rcDef;
    int  iRet = 0;
    BOOL bMirroredWnd=FALSE;

    if (prcOut != prcIn && prcOut != NULL) {
        ASSERT(prcIn != NULL);  // used to do SetRect(prcOut,0,0,0,0)
        CopyRect(prcOut, prcIn);
    }

#ifdef DEBUG
    if (! (uRxMask & (RX_OPPOSE|RX_GETWH))) {
        ASSERT(iWH == -1);
        iWH = -1;       // try to force something to go wrong...
    }
#endif

    if (uRxMask & (RX_EDGE|RX_ADJACENT)) {
        if (prcBound == NULL) {
            prcBound = &rcDef;
            ASSERT(hMon);
            GetMonitorRect(hMon, prcBound);     // aka GetSystemMetrics(SM_CXSCREEN)
        }

        #define iXMin (prcBound->left)
        #define iYMin (prcBound->top)
        #define iXMax (prcBound->right);
        #define iYMax (prcBound->bottom);
    }

    if (uRxMask & (RX_EDGE|RX_OPPOSE|RX_HIDE|RX_GETWH)) {

        //
        // If docking is happening on a horizontal size, then...
        // 
        if ((ABE_LEFT == uSide) || (ABE_RIGHT == uSide)) {
            bMirroredWnd = (IS_WINDOW_RTL_MIRRORED(GetParent(_hwnd)));
        }

        switch (uSide) {
        case ABE_TOP:
            if (uRxMask & RX_EDGE)
                prcOut->top = iYMin;
            if (uRxMask & RX_OPPOSE)
                prcOut->bottom = prcOut->top + iWH;
            if (uRxMask & RX_HIDE)
                MoveRect(prcOut, prcOut->left, prcOut->top - iWH + CXYHIDE(uSide));
            if (uRxMask & RX_GETWH)
                iRet = RECTHEIGHT(*prcIn);

            break;
        case ABE_BOTTOM:
            if (uRxMask & RX_EDGE)
                prcOut->bottom = iYMax;
            if (uRxMask & RX_OPPOSE)
                prcOut->top = prcOut->bottom - iWH;
            if (uRxMask & RX_HIDE)
                MoveRect(prcOut, prcOut->left, prcOut->bottom - CXYHIDE(uSide));
            if (uRxMask & RX_GETWH)
                iRet = RECTHEIGHT(*prcIn);

            break;
        case ABE_LEFT:
            if (uRxMask & RX_EDGE)
                prcOut->left = iXMin;
            if (uRxMask & RX_OPPOSE) {
                //
                // If the parent of this docked window is mirrored, then it is placed and
                // aligned to the right. [samera]
                //
                if (bMirroredWnd)
                    prcOut->left = prcOut->right - iWH;
                else
                    prcOut->right = prcOut->left + iWH;
            }
            if (uRxMask & RX_HIDE)
                MoveRect(prcOut, prcOut->left - iWH + CXYHIDE(uSide), prcOut->top);
            if (uRxMask & RX_GETWH)
                iRet = RECTWIDTH(*prcIn);

            break;
        case ABE_RIGHT:
            if (uRxMask & RX_EDGE)
                prcOut->right = iXMax;
            if (uRxMask & RX_OPPOSE) {
                //
                // If the parent of this docked window is mirrored, then it is placed and
                // aligned to the left
                //
                if (bMirroredWnd)
                    prcOut->right = prcOut->left + iWH;
                else
                    prcOut->left = prcOut->right - iWH;
            }
            if (uRxMask & RX_HIDE)
                MoveRect(prcOut, prcOut->right - CXYHIDE(uSide), prcOut->top);
            if (uRxMask & RX_GETWH)
                iRet = RECTWIDTH(*prcIn);

            break;
        }
    }

    if (uRxMask & RX_ADJACENT) {
        if (uSide == ABE_LEFT || uSide == ABE_RIGHT) {
            prcOut->top    = iYMin;
            prcOut->bottom = iYMax;
        }
        else {
            prcOut->left   = iXMin;
            prcOut->right  = iXMax;
        }
    }

    return iRet;
}

//***   _ProtoRect -- create best-guess proto rect for specified location
//
void CDockingBar::_ProtoRect(RECT* prcOut, UINT eModeNew, UINT uSideNew, HMONITOR hMonNew, POINT* ptXY)
{
    if (ISWBM_FLOAT(eModeNew))
    {
        // start at last position/size, and move to new left-top if requested
        CopyRect(prcOut, &_rcFloat);
        if (ptXY != NULL)
            MoveRect(prcOut, ptXY->x, ptXY->y);

        // if we're (e.g.) floating on the far right and the display shrinks,
        // we need to reposition ourselves
        // BUGBUG perf: wish we could do this at resolution-change time but
        // WM_DISPLAYCHANGE comes in too early (before our [pseudo] parent
        // has changed).
        if (eModeNew == WBM_FLOATING)
        {
            // make sure we're still visible
            // BUGBUG todo: multi-mon
            RECT rcTmp;

            _GetBorderRect(hMonNew, &rcTmp);

            if (prcOut->left > rcTmp.right || prcOut->top > rcTmp.bottom)
            {
                // BUGBUG note we don't explicitly account for other toolbars
                // this may be a bug (though other apps seem to behave the
                // same way)
                MoveRect(prcOut,
                    prcOut->left <= rcTmp.right ? prcOut->left :
                        rcTmp.right - CXFLOAT(),
                    prcOut->top  <= rcTmp.bottom ? prcOut->top  :
                        rcTmp.bottom - CYFLOAT()
                );
            }

        }
    }
    else
    {
        ASSERT(ISABE_DOCK(uSideNew));
        if (_fCanHide && ISWBM_HIDEABLE(eModeNew))
        {
            // force a 'tiny' rectangle
            // (BUGBUG prcBound==NULL bogus for XXX_HIDEALL && XXX_BROWSEROWNED)
            RectXform(prcOut, RX_EDGE|RX_OPPOSE|RX_ADJACENT|(_fHiding ? RX_HIDE : 0),
                prcOut, NULL, _adEdge[uSideNew], uSideNew, hMonNew);
        }
        else
        {
            // get current rect, adjust opposing side per request
            _GetBorderRect(hMonNew, prcOut);    //BUGBUG _GetClientRect(prcOut);
            RectXform(prcOut, RX_OPPOSE, prcOut, NULL, _adEdge[uSideNew], uSideNew, hMonNew);

        }
    }

    return;
}

//***   _NegotiateRect --
// NOTES
//      will only return an approximate result in the non-commit case.
//
void CDockingBar::_NegotiateRect(UINT eModeNew, UINT uSideNew, HMONITOR hMonNew,
    RECT* rcReq, BOOL fCommit)
{
    switch (eModeNew) {
    case WBM_TOPMOST:
        APPBARDATA abd;
        abd.cbSize = sizeof(APPBARDATA);
        abd.hWnd = _hwnd;

        AppBarQuerySetPos(rcReq, uSideNew, hMonNew, rcReq, &abd, fCommit);
        if (_fCanHide)
        {
            // we did a query to adjust the adjacent sides (e.g. so we don't
            // cover up the 'start' menu when we unhide).  however that may
            // have also moved us in from the edge, which we don't want.
            // so snap back to edge.
            int iWH;

            iWH = RectGetWH(rcReq, uSideNew);
            RectXform(rcReq, RX_EDGE|RX_OPPOSE|(_fHiding ? RX_HIDE : 0), rcReq, NULL, iWH, uSideNew, hMonNew);
        }
        goto Ldefault;

    default:
    Ldefault:
        // everyone else just gives us what we want

        // but, we need to free up border
        _NegotiateBorderRect(NULL, NULL, fCommit);     // free up space

        break;

    case WBM_BOTTOMMOST:
    case WBM_BBOTTOMMOST:
        _NegotiateBorderRect(rcReq, rcReq, fCommit);
        break;
    }


    return;
}

void CDockingBar::_AppBarCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    APPBARDATA abd;

    ASSERT(_eMode == WBM_TOPMOST);

    abd.cbSize = sizeof(abd);
    abd.hWnd = hwnd;

    switch (wParam) {
    case ABN_FULLSCREENAPP:
        // when 1st  app goes   full-screen, move ourselves to BOTTOM;
        // when last app leaves full-screen, move ourselves back
        // todo: FullScreen(flg)
        SetWindowPos(hwnd,
            lParam ? HWND_BOTTOM : HWND_TOPMOST,
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        break;

    case ABN_POSCHANGED:
        TraceMsg(DM_APPBAR, "cwb.abcb: ABN_POSCHANGED");

        // note that we do this even if _fHiding.  while we want
        // to stay snapped to the edge as a 'tiny' rect, a change
        // in someone else *should* effect our adjacent edges.
        //
        // BUGBUG unfortunately this currently causes 'jiggle' of a hidden
        // guy when another appbar moves (due to a SlideWindow of a 0-width
        // hidden guy and a rounded-up 8-pixel wide guy).  when we switch
        // to explorer's new offscreen hide that should go away.
        _AppBarOnPosChanged(&abd);
        break;
    }

    return;
}

HRESULT CDockingBar::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDockingBar, IDockingWindow),        // IID_IDockingWindow
        QITABENT(CDockingBar, IObjectWithSite),       // IID_IObjectWithSite
        QITABENT(CDockingBar, IPersistStreamInit),    // IID_IPersistStreamInit
        QITABENTMULTI(CDockingBar, IPersistStream, IPersistStreamInit), // IID_IPersistStream
        QITABENTMULTI(CDockingBar, IPersist, IPersistStreamInit), // IID_IPersist
        QITABENT(CDockingBar, IPersistPropertyBag),   // IID_IPersistPropertyBag
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);

    return hres;
}

HRESULT CDockingBar::QueryService(REFGUID guidService,
                                REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL; // assume error

    //  Block IID_ITargetFrame, so we don't look like a frame of the
    //  window we are attached to
    if (IsEqualGUID(guidService, IID_ITargetFrame)
        ||IsEqualGUID(guidService, IID_ITargetFrame2)) {
        return hres;
    }

    hres = SUPERCLASS::QueryService(guidService, riid, ppvObj);
    if (FAILED(hres))
    {
        const GUID* pguidService = &guidService;
    
        if (IsEqualGUID(guidService, SID_SProxyBrowser)) {
            pguidService = &SID_STopLevelBrowser;
        }
    
        if (_ptbSite) {
            hres = IUnknown_QueryService(_ptbSite, *pguidService, riid, ppvObj);
        }
    }

    return hres;
}

void CDockingBar::_GrowShrinkBar(DWORD dwDirection)
{
    RECT    rcNew, rcOld;
    int     iMin;

    iMin = GetSystemMetrics(SM_CXVSCROLL) * 4;

    GetWindowRect(_hwnd, &rcNew);   
    rcOld = rcNew;
    
    switch(_uSide)
    {
        case ABE_TOP:
            if (VK_DOWN == dwDirection)
                rcNew.bottom += GetSystemMetrics(SM_CYFRAME);

            if (VK_UP == dwDirection)
                rcNew.bottom -= GetSystemMetrics(SM_CYFRAME);

            if ((rcNew.bottom - rcNew.top) < iMin)
                rcNew.bottom = rcNew.top + iMin;
            break;

        case ABE_BOTTOM:
            if (VK_UP == dwDirection)
                rcNew.top -= GetSystemMetrics(SM_CYFRAME);

            if (VK_DOWN == dwDirection)
                rcNew.top += GetSystemMetrics(SM_CYFRAME);

            if ((rcNew.bottom - rcNew.top) < iMin)
                rcNew.top = rcNew.bottom - iMin;
            break;

        case ABE_LEFT:
            if (VK_RIGHT == dwDirection)
                rcNew.right += GetSystemMetrics(SM_CXFRAME);

            if (VK_LEFT == dwDirection)
                rcNew.right -= GetSystemMetrics(SM_CXFRAME);

            if ((rcNew.right - rcNew.left) < iMin)
                rcNew.right = rcNew.left + iMin;
            break;
            
        case ABE_RIGHT:
            if (VK_LEFT == dwDirection)
                rcNew.left -= GetSystemMetrics(SM_CXFRAME);

            if (VK_RIGHT == dwDirection)
                rcNew.left += GetSystemMetrics(SM_CXFRAME);

            if ((rcNew.right - rcNew.left) < iMin)
                rcNew.left = rcNew.right - iMin;
            break;

    }

    if (!EqualRect(&rcOld, &rcNew))
    {
        int iWH;
        RECT rcScreen;

        // don't let the new size get > MonitorRect/2
        GetMonitorRect(_hMon, &rcScreen);   // aka GetSystemMetrics(SM_CXSCREEN)
        iWH = RECTGETWH(_uSide, &rcScreen);
        iWH /= 2;
        if (RECTGETWH(_uSide, &rcNew) > iWH) 
        {
            RectXform(&rcNew, RX_OPPOSE, &rcNew, NULL, iWH, _uSide, NULL);
        }

        _SetVRect(&rcNew);
        _Recalc();
    }
}


//*** CDockingBar::IOleCommandTarget::* {

HRESULT CDockingBar::Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt,
    VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL) {
        /*NOTHING*/
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup)) {
        switch (nCmdID) {
        case SHDVID_RAISE:
            ASSERT(pvarargIn && pvarargIn->vt == VT_I4);
            if (pvarargIn->vt == VT_I4 && pvarargIn->lVal != DTRF_QUERY) {
                _OnRaise(pvarargIn->lVal);
                return S_OK;
            }
            break;  // e.g. DTRF_QUERY
        default:
            // note that this means we may get OLECMDERR_E_UNKNOWNGROUP
            // rather than OLECMDERR_E_NOTSUPPORTED for unhandled guys...
            break;
        }
    } 
    else if (IsEqualGUID(CGID_DeskBarClient, *pguidCmdGroup)) 
    {
        if (DBCID_RESIZE == nCmdID)
        {
            _GrowShrinkBar(nCmdexecopt);
            return S_OK;
        }
    }
    
    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt,
        pvarargIn, pvarargOut);
}
// }

//*** CDockingBar::IDockingWindow::* {
//

HRESULT CDockingBar::SetSite(IUnknown* punkSite)
{
    ATOMICRELEASE(_ptbSite);

    if (punkSite)
    {
        HRESULT hresT;
        hresT = punkSite->QueryInterface(IID_IDockingWindowSite, (LPVOID*)&_ptbSite);

        IUnknown_GetWindow(punkSite, &_hwndSite);

        //
        // Check if we are under the desktop browser or not and set
        // the initial state correctly. (Always on top for Desktop)
        //
        IUnknown* punkT;
        hresT = punkSite->QueryInterface(SID_SShellDesktop, (LPVOID*)&punkT);
        if (SUCCEEDED(hresT))
        {
            _fDesktop = TRUE;
            punkT->Release();
        }

        if (!_fInitSited)
        {
            if (!_eInitLoaded)
            {
                // if we haven't initialized, do it now.
                InitNew();
                _eMode = WBM_BOTTOMMOST;
            }
                
            ASSERT(_eInitLoaded);
            if (_eInitLoaded == IPS_INITNEW)
            {
                _InitPos4(FALSE);
                _eMode = _fDesktop ? WBM_TOPMOST : WBM_BBOTTOMMOST;
            }
        }
        ASSERT(_eMode != WBM_NIL);
        // BUGBUG actually we could also be owned floating...
        ASSERT(ISWBM_DESKTOP() == _fDesktop);
        ASSERT(_fDesktop || _eMode == WBM_BBOTTOMMOST);
        ASSERT(ISWBM_DESKTOP() == _fDesktop);
        ASSERT(ISWBM_DESKTOP() || _eMode == WBM_BBOTTOMMOST);
    }

    _fInitSited = TRUE;     // done w/ 1st-time init

    return S_OK;
}

HRESULT CDockingBar::ShowDW(BOOL fShow)
{
    fShow = BOOLIFY(fShow);     // so comparisons and assigns to bitfields work

    // we used to early out if BOOLIFY(_fShow) == fShow.
    // however we now count on ShowDW(TRUE) to force a refresh
    // (e.g. when screen resolution changes CBB::v_ShowHideChildWindows
    // calls us)
    if (BOOLIFY(_fShow) == fShow)
        return S_OK;

    _fShow = fShow;

    if (!_fInitShowed)
    {
        ASSERT(_fInitSited && _eInitLoaded);
        _Initialize();
        ASSERT(_fInitShowed);
    }

    if (_fShow)
    {
        // BUGBUG _ChangeTopMost does this...
        // Tell itself to resize.

        // use _MoveSizeHelper (not just _NegotiateBorderRect) since we might
        // actually be moving to a new position...
        _Recalc();  // _MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);
        // _NegotiateBorderRect(NULL, NULL, FALSE)

        if (EVAL(_pDBC))
            _pDBC->UIActivateDBC(DBC_SHOW);

        // nt5:148444: SW_SHOWNA (vs. SW_SHOW) so we don't unhide on create
        // this fix will cause a new bug -- newly created bars don't have
        // focus (e.g. drag a band to floating, the new floating bar won't
        // have focus) -- but that should be the lesser of evils.
        //ShowWindow(_hwnd, ISWBM_FLOAT(_eMode) ? SW_SHOWNORMAL : SW_SHOWNA);
        ShowWindow(_hwnd, SW_SHOWNA);
        _OnSize();
    }
    else
    {
        ShowWindow(_hwnd, SW_HIDE);
        if (EVAL(_pDBC))
            _pDBC->UIActivateDBC(DBC_SHOWOBSCURE);
        UIActivateIO(FALSE, NULL);
        
        // Tell itself to resize.

        // don't call MoveSizeHelper here since it will do (e.g.)
        // negotiation, which will cause flicker and do destructive stuff.
        //_Recalc();  //_MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);
        _NegotiateBorderRect(NULL, NULL, TRUE);     // hide=>0 border space
    }

    return S_OK;
}

HRESULT CDockingBar::ResizeBorderDW(LPCRECT prcBorder,
    IUnknown* punkToolbarSite, BOOL fReserved)
{
    _Recalc();  // _MoveSizeHelper(_eMode, _uSide, NULL, NULL, TRUE, TRUE);
    return S_OK;    // BUGBUG _NegotiateBorderRect()?
}

HRESULT CDockingBar::_NegotiateBorderRect(RECT* prcOut, RECT* prcReq, BOOL fCommit)
{
    UINT eMode, uSide;
    HMONITOR hMon;
    int iWH;

    // BUGBUG should be params like MSH etc.
    eMode = ((_fDragging == DRAG_MOVE) ? _eModePending : _eMode);
    uSide = ((_fDragging == DRAG_MOVE) ? _uSidePending : _uSide);
    hMon = ((_fDragging == DRAG_MOVE) ? _hMonPending : _hMon);

    if (prcOut != prcReq && prcOut != NULL && prcReq != NULL)
        CopyRect(prcOut, prcReq);

    if (_ptbSite) {
        RECT rcRequest = { 0, 0, 0, 0 };

        if (_fShow && ISWBM_BOTTOM(eMode)) {
            
            iWH = RectGetWH(prcReq, uSide);
            ASSERT(iWH == _adEdge[uSide]);
            if ((!_fCanHide) && uSide != ABE_NIL)
                ((int*)&rcRequest)[uSide] = iWH;
                
            if (_fTheater) {
                // MOVE TO CBROWSERBAR

                
                // we override the left that we request from the browser, but
                // we need to notify theater what the user has requested for the expaneded width
                VARIANTARG v = { 0 };
                v.vt = VT_I4;
                v.lVal = rcRequest.left;
                IUnknown_Exec(_ptbSite, &CGID_Theater, THID_SETBROWSERBARWIDTH, 0, &v, NULL);
                _iTheaterWidth = v.lVal;
                
                // if we're in theater mode, we can only be on the left and we only grab left border
                ASSERT(uSide == ABE_LEFT);

                // if we're in autohide mode, we request no space
                if (!_fNoAutoHide)
                    rcRequest.left = 0;

                // END MOVE TO CBROWSERBAR                
            }
        }

        // BUGBUG leave alone (at 0 from HideRegister?) if _fHiding==HIDE_AUTO
        HMONITOR hMonOld = _SetNewMonitor(hMon);  

        _ptbSite->RequestBorderSpaceDW(SAFECAST(this, IDockingWindow*), &rcRequest);
        if (fCommit) {
            RECT rcMirRequest;
            LPRECT lprcRequest = &rcRequest; 

            if (IS_WINDOW_RTL_MIRRORED(_hwnd) && 
                !IS_WINDOW_RTL_MIRRORED(GetParent(_hwnd))) {
                // Swap left and right.
                rcMirRequest.left   = rcRequest.right;
                rcMirRequest.right  = rcRequest.left;
                rcMirRequest.top    = rcRequest.top;
                rcMirRequest.bottom = rcRequest.bottom;

                lprcRequest = &rcMirRequest;
            }
            _ptbSite->SetBorderSpaceDW(SAFECAST(this, IDockingWindow*), lprcRequest);
        }

        if (_fShow && ISWBM_BOTTOM(eMode) && !_fTheater) {
            // were'd we end up (as a real rect not just a size)?
            // start w/ our full border area, then apply negotiated width.
            // however that may have also moved us in from the edge, which
            // we don't want if we're autohide, so snap back to edge if so.
            _ptbSite->GetBorderDW(SAFECAST(this, IDockingWindow*), prcOut);

            // aka ClientToScreen
            MapWindowPoints(_hwndSite, HWND_DESKTOP, (POINT*) prcOut, 2);

            if ((!_fCanHide) && uSide != ABE_NIL)
                iWH = ((int*)&rcRequest)[uSide];
            
            RectXform(prcOut, (_fCanHide ? RX_EDGE : 0)|RX_OPPOSE|(_fHiding ? RX_HIDE : 0), prcOut, NULL, iWH, uSide, hMon);
        }
        
        if (hMonOld)
            _SetNewMonitor(hMonOld);

    }

    return S_OK;
}

// }

//*** CDockingBar::IPersistStream*::* {
//

HRESULT CDockingBar::IsDirty(void)
{
    return S_FALSE; // Never be dirty
}

//
// Persisted CDockingBar
//
struct SWebBar
{
    DWORD   cbSize;
    DWORD   cbVersion;
    UINT    uSide : 3;
    UINT    fWantHide :1;
    INT     adEdge[4];  // BUGBUG wordsize dependent
    RECT    rcFloat;    // BUGBUG ...
    POINT   ptSiteCenter; // Center of the docking site -- in case of multiple docking sites

    UINT    eMode;
    UINT    fAlwaysOnTop;

    RECT    rcChild;
};

#define SWB_VERSION 8

HRESULT CDockingBar::Load(IStream *pstm)
{
    SWebBar swb = {0};
    ULONG cbRead;

    TraceMsg(DM_PERSIST, "cwb.l enter(this=%x pstm=%x) tell()=%x", this, pstm, DbStreamTell(pstm));

    ASSERT(!_eInitLoaded);
    HRESULT hres = pstm->Read(&swb, SIZEOF(swb), &cbRead);
#ifdef DEBUG
    // just in case we toast ourselves (offscreen or something)...
    static BOOL fNoPersist = FALSE;
    if (fNoPersist)
        hres = E_FAIL;
#endif
    
    if (hres==S_OK && cbRead==SIZEOF(swb)) {
        // BUGBUG: this is not forward compatible!
        if (swb.cbSize==SIZEOF(SWebBar) && swb.cbVersion==SWB_VERSION) {

            _eMode = swb.eMode;
            _uSide = swb.uSide;
            _hMon  = MonitorFromPoint(swb.ptSiteCenter, MONITOR_DEFAULTTONEAREST);
            // don't call _SetModeSide, _MoveSizeHelper, etc. until *after* _Initialize
            _fWantHide = swb.fWantHide;
            memcpy(_adEdge, swb.adEdge, SIZEOF(_adEdge));
            _rcFloat = swb.rcFloat;
            _NotifyModeChange(0);

            // child (e.g. bandsite)
            ASSERT(_pDBC != NULL);
            if (_pDBC != NULL) {
                // BUGBUG require IPersistStreamInit?
                IPersistStream *ppstm;
                hres = _pDBC->QueryInterface(IID_IPersistStream, (LPVOID*)&ppstm);
                if (SUCCEEDED(hres)) {

                    // set the child size first because initialization layout might depend on it
                    SetWindowPos(_hwndChild, 0,
                                 swb.rcChild.left, swb.rcChild.top, RECTWIDTH(swb.rcChild), RECTHEIGHT(swb.rcChild),
                                 SWP_NOACTIVATE|SWP_NOZORDER);
                    
                    ppstm->Load(pstm);
                    ppstm->Release();
                }
            }

            _eInitLoaded = IPS_LOAD;    // BUGBUG what if OLFS of bands fails?
            TraceMsg(DM_PERSIST, "CDockingBar::Load succeeded");
        } else {
            TraceMsg(DM_ERROR, "CWB::Load failed swb.cbSize==SIZEOF(SWebBar) && swb.cbVersion==SWB_VERSION");
            hres = E_FAIL;
        }
    } else {
        TraceMsg(DM_ERROR, "CWB::Load failed (hres==S_OK && cbRead==SIZEOF(_adEdge)");
        hres = E_FAIL;
    }
    TraceMsg(DM_PERSIST, "cwb.l leave tell()=%x", DbStreamTell(pstm));
    return hres;
}

HRESULT CDockingBar::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hres;
    SWebBar swb = {0};
    RECT rcMonitor;

    swb.cbSize = SIZEOF(SWebBar);
    swb.cbVersion = SWB_VERSION;
    swb.uSide = _uSide;
    swb.eMode = _eMode;
    swb.fWantHide = _fWantHide;
    memcpy(swb.adEdge, _adEdge, SIZEOF(_adEdge));
    swb.rcFloat = _rcFloat;
    GetWindowRect(_hwndChild, &swb.rcChild);
    MapWindowRect(HWND_DESKTOP, _hwnd, &swb.rcChild);

    ASSERT(_hMon);
    GetMonitorRect(_hMon, &rcMonitor);
    swb.ptSiteCenter.x = (rcMonitor.left + rcMonitor.right) / 2;
    swb.ptSiteCenter.y = (rcMonitor.top + rcMonitor.bottom) / 2;
    
    hres = pstm->Write(&swb, SIZEOF(swb), NULL);
    if (SUCCEEDED(hres))
    {
        IPersistStream* ppstm;
        hres = _pDBC->QueryInterface(IID_IPersistStream, (LPVOID*)&ppstm);
        if (SUCCEEDED(hres))
        {
            hres = ppstm->Save(pstm, TRUE);
            ppstm->Release();
        }
    }
    
    return hres;
}

HRESULT CDockingBar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ULARGE_INTEGER cbMax = { SIZEOF(SWebBar), 0 };
    *pcbSize = cbMax;
    return S_OK;
}

HRESULT CDockingBar::InitNew(void)
{
    ASSERT(!_eInitLoaded);
    _eInitLoaded = IPS_INITNEW;
    TraceMsg(DM_PERSIST, "CDockingBar::InitNew called");

    // can't call _InitPos4 until set site in SetSite
    // don't call _SetModeSide, _MoveSizeHelper, etc. until *after* _Initialize

    // derived class (e.g. CBrowserBarApp) does the _Populate...

    // on first creation, before bands are added, but the bandsite IS created, we need to notify the bandsite of the new position
    _NotifyModeChange(0);
    return S_OK;
}

HRESULT CDockingBar::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    ASSERT(!_eInitLoaded);

    _eInitLoaded = IPS_LOADBAG;

    // TODO: We'll read following properties.
    //
    //  URL = "..."
    //  Mode = 0 - TopMost, 1 - Bottom, 2 - Undocked
    //  Side = 0 - Right, 1 - Top, 2 - Left, 3 - Bottom
    //  Left/Right/Top/Bottom = Initial docked size
    //


    UINT uSide;
    UINT eMode = _eMode;

    if (WBM_NIL == eMode)
        eMode = WBM_BOTTOMMOST;
    
    eMode = PropBag_ReadInt4(pPropBag, L"Mode", eMode);
    uSide = PropBag_ReadInt4(pPropBag, L"Side", _uSide);
    _adEdge[ABE_LEFT] = PropBag_ReadInt4(pPropBag, L"Left", _adEdge[ABE_LEFT]);
    _adEdge[ABE_RIGHT] = PropBag_ReadInt4(pPropBag, L"Right", _adEdge[ABE_RIGHT]);
    _adEdge[ABE_TOP] = PropBag_ReadInt4(pPropBag, L"Top", _adEdge[ABE_TOP]);
    _adEdge[ABE_BOTTOM] = PropBag_ReadInt4(pPropBag, L"Bottom", _adEdge[ABE_BOTTOM]);

    int x = PropBag_ReadInt4(pPropBag, L"X", _rcFloat.left);
    int y = PropBag_ReadInt4(pPropBag, L"Y", _rcFloat.top);
    OffsetRect(&_rcFloat, x - _rcFloat.left, y - _rcFloat.top);

    int cx = PropBag_ReadInt4(pPropBag, L"CX", RECTWIDTH(_rcFloat));
    int cy = PropBag_ReadInt4(pPropBag, L"CY", RECTHEIGHT(_rcFloat));
    _rcFloat.right = _rcFloat.left + cx;
    _rcFloat.bottom = _rcFloat.top + cy;

    // set up vars for eventual CDockingBar::_Initialize call
    ASSERT(!CDB_INITED());
    _eMode = eMode;
    _uSide = uSide;
    
    POINT pt = {x, y};
    // (dli) compute the new hMonitor 
    _hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

    // don't call _SetModeSide, _MoveSizeHelper, etc. until *after* _Initialize

    // derived class (e.g. CBrowserBarApp) does the _Populate...

    // on first creation, before bands are added, but the bandsite IS created, we need to notify the bandsite of the new position
    _NotifyModeChange(0);
    return S_OK;
}

HRESULT CDockingBar::Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    // We don't need to support this for now.
    return E_NOTIMPL;
}

// }

//*** CDockingBar::IDocHostUIHandler::* {
//

HRESULT CDockingBar::ShowContextMenu(DWORD dwID,
    POINT* ppt,
    IUnknown* pcmdtReserved,
    IDispatch* pdispReserved)
{
    if (dwID==0) {
        TraceMsg(DM_MENU, "cdb.scm: intercept");
        return _TrackPopupMenu(ppt);
    }
    return S_FALSE;
}


// }


void CDockingBar::_SetModeSide(UINT eMode, UINT uSide, HMONITOR hMonNew, BOOL fNoMerge) 
{
    _ChangeTopMost(eMode);
    _uSide = uSide;
    _hMon = hMonNew;
    _SetNewMonitor(hMonNew);
}


//***   CDockingBar::IInputObjectSite::* {

HRESULT CDockingBar::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    return UnkOnFocusChangeIS(_ptbSite, SAFECAST(this, IInputObject*), fSetFocus);
}

// }



////////////////////////////////////////////////////////////////
//
//  A deskbar property bag
//////
HRESULT CDockingBarPropertyBag_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDockingBarPropertyBag* p = new CDockingBarPropertyBag();
    if (p != NULL)
    {
        *ppunk = SAFECAST(p, IPropertyBag*);
        return S_OK;
    }

    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

ULONG CDockingBarPropertyBag::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CDockingBarPropertyBag::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDockingBarPropertyBag::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDockingBarPropertyBag, IPropertyBag),     // IID_IPropertyBag
        QITABENT(CDockingBarPropertyBag, IDockingBarPropertyBagInit),     // IID_IDockingBarPropertyBagInit
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


const WCHAR * const c_szPropNames[] = {
    L"Side",
    L"Mode",
    L"Left",
    L"Top",
    L"Right",
    L"Bottom",
    L"Deleteable",
    L"X",
    L"Y",
    L"CX",
    L"CY"
};


HRESULT CDockingBarPropertyBag::Read( 
                    /* [in] */ LPCOLESTR pszPropName,
                    /* [out][in] */ VARIANT *pVar,
                    /* [in] */ IErrorLog *pErrorLog)
{
    int epropdata;

    for (epropdata = 0; epropdata < (int)PROPDATA_COUNT; epropdata++) {
        if (!StrCmpW(pszPropName, c_szPropNames[epropdata])) {
            break;
        }
    }

    if (epropdata < PROPDATA_COUNT && 
        _props[epropdata]._fSet) {
        pVar->lVal = _props[epropdata]._dwData;
        pVar->vt = VT_I4;
        return S_OK;
    }
    
    return E_FAIL;
}




#ifdef DEBUG
//***   DbCheckWindow --
// NOTES
//  BUGBUG nuke the 'hwndClient' param and just use GetParent (but what
//  about 'owned' windows, does GetParent give the correct answer?)
BOOL DbCheckWindow(HWND hwnd, RECT *prcExp, HWND hwndClient)
{
    RECT rcAct;

    GetWindowRect(hwnd, &rcAct);
    hwndClient = GetParent(hwnd);   // BUGBUG nuke this param
    if (hwndClient != NULL) {
        // aka ClientToScreen
        MapWindowPoints(HWND_DESKTOP, hwndClient, (POINT*) &rcAct, 2);
    }
    if (!EqualRect(&rcAct, prcExp)) {
        TraceMsg(DM_TRACE,
            "cwb.dbcw: !EqualRect rcAct=(%d,%d,%d,%d) (%dx%d) rcExp=(%d,%d,%d,%d) (%dx%d) hwndClient=0x%x",
            rcAct.left, rcAct.top, rcAct.right, rcAct.bottom,
            RECTWIDTH(rcAct), RECTHEIGHT(rcAct),
            prcExp->left, prcExp->top, prcExp->right, prcExp->bottom,
            RECTWIDTH(*prcExp), RECTHEIGHT(*prcExp),
            hwndClient);
        return FALSE;
    }
    return TRUE;
}

//***   DbStreamTell -- get position in stream (low part only)
//
unsigned long DbStreamTell(IStream *pstm)
{
    if (pstm == 0)
        return (unsigned long) -1;

    ULARGE_INTEGER liEnd;

    pstm->Seek(c_li0, STREAM_SEEK_CUR, &liEnd);
    if (liEnd.HighPart != 0)
        TraceMsg(DM_TRACE, "DbStreamTell: hi!=0");
    return liEnd.LowPart;
}

//***   DbMaskToMneStr -- pretty-print a bit mask in mnemonic form
// ENTRY/EXIT
//  uMask       bit mask
//  szMne       mnemonics, sz[0] for bit 0 .. sz[N] for highest bit
//  return      ptr to *static* buffer
// NOTES
//  n.b.: non-reentrant!!!
TCHAR *DbMaskToMneStr(UINT uMask, TCHAR *szMnemonics)
{
    static TCHAR buf[33];       // BUGBUG non-reentrant!!!
    TCHAR *p;

    p = &buf[ARRAYSIZE(buf) - 1];       // point at EOS
    ASSERT(*p == '\0');
    for (;;) {
        if (*szMnemonics == 0) {
            ASSERT(uMask == 0);
            break;
        }

        --p;
        *p = (uMask & 1) ? *szMnemonics : TEXT('-');

        ++szMnemonics;
        uMask >>= 1;
    }

    return p;
}
#endif
