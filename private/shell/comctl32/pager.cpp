//---------------------------------------------------------------------------------------
//  File : Pager.cpp
//  Description :
//        This file implements the pager control
//---------------------------------------------------------------------------------------
#include "ctlspriv.h"
#include "pager.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

#define MINBUTTONSIZE   12

//Timer Flags
#define PGT_SCROLL       1

void NEAR DrawScrollArrow(HDC hdc, LPRECT lprc, WORD wControlState);

#ifdef DEBUG
#if 0
extern "C" {
extern void _InvalidateRect(HWND hwnd, LPRECT prc, BOOL fInval);
extern void _RedrawWindow(HWND hwnd, LPRECT prc, HANDLE hrgn, UINT uFlags);
extern void _SetWindowPos(HWND hwnd, HWND hwnd2, int x, int y, int cx, int cy, UINT uFlags);
};
#define InvalidateRect(hwnd, prc, fInval) _InvalidateRect(hwnd, prc, fInval)
#define RedrawWindow(hwnd, prc, hrgn, uFlags) _RedrawWindow(hwnd, prc, hrgn, uFlags)
#define SetWindowPos(hwnd, hwnd2, x, y, cx, cy, uFlags) _SetWindowPos(hwnd, hwnd2, x, y, cx, cy, uFlags)
#endif
#endif

//Public Functions
//---------------------------------------------------------------------------------------
extern "C" {

//This function registers  the pager window class
BOOL InitPager(HINSTANCE hinst)
{
    WNDCLASS wc;
    TraceMsg(TF_PAGER, "Init Pager");

    if (!GetClassInfo(hinst, WC_PAGESCROLLER, &wc)) {
        wc.lpfnWndProc     = CPager::PagerWndProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon           = NULL;
        wc.lpszMenuName    = NULL;
        wc.hInstance       = hinst;
        wc.lpszClassName   = WC_PAGESCROLLER;
        wc.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;
        wc.style           = CS_GLOBALCLASS;
        wc.cbWndExtra      = sizeof(LPVOID);
        wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }
    return TRUE;
}

}; // extern "C"

//---------------------------------------------------------------------------------------
CPager::CPager()
{
    _clrBk = g_clrBtnFace;
    
    //Initialize Static Members
    _iButtonSize = (int) g_cxScrollbar * 3 / 4;
    if (_iButtonSize < MINBUTTONSIZE) {
        _iButtonSize = MINBUTTONSIZE;
    }

    _ptLastMove.x = -1;
    _ptLastMove.y = -1;

    _cLinesPerTimeout = 0;
    _cPixelsPerLine = 0;
    _cTimeout = GetDoubleClickTime() / 8;
}

//---------------------------------------------------------------------------------------
// Static Pager Window Procedure


LRESULT CPager::PagerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPager *pp = (CPager*)GetWindowPtr(hwnd, 0);
    if (uMsg == WM_CREATE) {
        ASSERT(!pp);
        pp = new CPager();
        if (!pp)
            return 0L;
    }

    if (pp) {
        return pp->v_WndProc(hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------------------
LRESULT CPager::PagerDragCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPager *pp = (CPager*)GetWindowPtr(hwnd, 0);

    if (pp) {
        return pp->_DragCallback(hwnd, uMsg, wParam, lParam);
    }
    return -1;
}


//---------------------------------------------------------------------------------------
// CControl Class Implementation
//---------------------------------------------------------------------------------------


LRESULT CControl::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    switch (uMsg) {

    case WM_CREATE:
        SetWindowPtr(hwnd, 0, this);
        CIInitialize(&ci, hwnd, (CREATESTRUCT*)lParam);
        return v_OnCreate();

    case WM_NCCALCSIZE:
        if (v_OnNCCalcSize(wParam, lParam, &lres))
            break;
        goto DoDefault;

    case WM_SIZE:
        v_OnSize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
        
    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&ci, lParam);

    case WM_NOTIFY:
        return v_OnNotify(wParam, lParam);
    
    case WM_STYLECHANGED:
        v_OnStyleChanged(wParam, lParam);
        break;

    case WM_COMMAND:
        return v_OnCommand(wParam, lParam);

    case WM_NCPAINT:
        v_OnNCPaint();
        goto DoDefault;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        _OnPaint((HDC)wParam);
        break;
        
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, 0, 0);
        delete this;
        break;

    case TB_SETPARENT:
        {
            HWND hwndOld = ci.hwndParent;

            ci.hwndParent = (HWND)wParam;
            return (LRESULT)hwndOld;
        }


    default:
        if (CCWndProc(&ci, uMsg, wParam, lParam, &lres))
            return lres;
DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lres;
}

//---------------------------------------------------------------------------------------
BOOL CControl::v_OnNCCalcSize(WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    return FALSE;
}

//---------------------------------------------------------------------------------------
DWORD CControl::v_OnStyleChanged(WPARAM wParam, LPARAM lParam)
{
    LPSTYLESTRUCT lpss = (LPSTYLESTRUCT) lParam;
    DWORD dwChanged = 0;    
    if (wParam == GWL_STYLE) {
        ci.style = lpss->styleNew;

        dwChanged = (lpss->styleOld ^ lpss->styleNew);
    } else if (wParam == GWL_EXSTYLE) {
        //
        // Save the new ex-style bits
        //
        dwChanged    = (lpss->styleOld ^ lpss->styleNew);
        ci.dwExStyle = lpss->styleNew;
    }

    TraceMsg(TF_PAGER, "cctl.v_osc: style=%x ret dwChged=%x", ci.style, dwChanged);
    return dwChanged;
}

//---------------------------------------------------------------------------------------
void CControl::_OnPaint(HDC hdc)
{
    if (hdc) {
        v_OnPaint(hdc);
    } else {
        PAINTSTRUCT ps;
        hdc = BeginPaint(ci.hwnd, &ps);
        v_OnPaint(hdc);
        EndPaint(ci.hwnd, &ps);
    }
}

//---------------------------------------------------------------------------------------
//  CPager Class Implementation
//---------------------------------------------------------------------------------------
inline int CPager::_GetButtonSize()
{
    return _iButtonSize;
}

//---------------------------------------------------------------------------------------

LRESULT CPager::_DragCallback(HWND hwnd, UINT code, WPARAM wp, LPARAM lp)
{
    LRESULT lres = -1;
    switch (code)
    {
    case DPX_DRAGHIT:
        if (lp)
        {
            POINT pt; 
            int iButton;
            pt.x = ((POINTL *)lp)->x;
            pt.y = ((POINTL *)lp)->y;

            MapWindowPoints(NULL, ci.hwnd, &pt, 1);

            iButton = _HitTest(pt.x, pt.y);

            if (iButton >= 0) 
            {
                if(!_fTimerSet)
                {
                    _fTimerSet = TRUE;
                    _iButtonTrack = iButton;
                    SetTimer(ci.hwnd, PGT_SCROLL, _cTimeout, NULL);
                }

            } else {
                _KillTimer();
                _iButtonTrack = -1;
            }
        }
        else
            lres = -1;
        break;

    case DPX_LEAVE:
        _KillTimer();
        _iButtonTrack = -1;
        break;

    default: 
        lres = -1;
        break;
    }
    return lres;
}

//---------------------------------------------------------------------------------------
void CPager::_NeedScrollbars(RECT rc)
{  
    int parentheight;
    int childheight;
    POINT ptPos = _ptPos;
   
    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
        FlipPoint(&ptPos);
    }
    
    //Get Parent Window height
    parentheight = RECTHEIGHT(rc);

    //Get Child Window height
    rc = _rcChildIdeal;
    if (ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }
    
    childheight = RECTHEIGHT(rc);

    TraceMsg(TF_PAGER, "cps.nsb: cyChild=%d cyParent=%d _yPos=%d", childheight, parentheight, ptPos.y);

    if (childheight < parentheight ) 
    {
        ptPos.y = 0;
    }

    int iButton = _HitTestCursor();
    //See if we need top scrollbar
    if (ptPos.y > 0 ) {

        // if this button is the one that is hot tracked and the style is not PGS_AUTOSCROLL
        // then we set the state to PGF_HOT otherwise the state is set to PGF_NORMAL
        _dwState[PGB_TOPORLEFT] |= PGF_NORMAL;
        _dwState[PGB_TOPORLEFT] &= ~PGF_GRAYED;

    } else {
        if (!(ci.style & PGS_AUTOSCROLL) && (iButton == PGB_TOPORLEFT || _iButtonTrack == PGB_TOPORLEFT)) {
            _dwState[PGB_TOPORLEFT] |= PGF_GRAYED;
        } else {
            _dwState[PGB_TOPORLEFT] = PGF_INVISIBLE;
        }
    }

    if (_dwState[PGB_TOPORLEFT] != PGF_INVISIBLE)
    {
        parentheight -= _GetButtonSize();
    }
    
    //See if we need botton scrollbar
    if ((childheight - ptPos.y) > parentheight ) {
        //We need botton scroll bar

        // if this button is the one that is hot tracked and the style is not PGS_AUTOSCROLL
        // then we set the state to PGF_HOT otherwise the state is set to PGF_NORMAL
        _dwState[PGB_BOTTOMORRIGHT] |= PGF_NORMAL;
        _dwState[PGB_BOTTOMORRIGHT] &= ~PGF_GRAYED;
        
    } else {
        
        if (!(ci.style & PGS_AUTOSCROLL) && (iButton == PGB_BOTTOMORRIGHT || _iButtonTrack == PGB_BOTTOMORRIGHT)) {
            _dwState[PGB_BOTTOMORRIGHT] |= PGF_GRAYED;
        } else {
            _dwState[PGB_BOTTOMORRIGHT] = PGF_INVISIBLE;
        }
    }
}
//---------------------------------------------------------------------------------------
BOOL CPager::v_OnNCCalcSize(WPARAM wParam, LPARAM lParam, LRESULT *plres)
{    
    *plres = DefWindowProc(ci.hwnd, WM_NCCALCSIZE, wParam, lParam ) ;
    if (wParam) {
        BOOL bHorzMirror = ((ci.dwExStyle & RTL_MIRRORED_WINDOW) && (ci.style & PGS_HORZ));
        DWORD dwStateOld[2];
        NCCALCSIZE_PARAMS* pnp = (NCCALCSIZE_PARAMS*)lParam;
        _rcDefClient = pnp->rgrc[0];
        InflateRect(&_rcDefClient, -_iBorder, -_iBorder);
        _GetChildSize();
        
        dwStateOld[0] = _dwState[0];
        dwStateOld[1] = _dwState[1];
        _NeedScrollbars(pnp->rgrc[0]);

        // invalidate only if something has changed to force a new size
        if ((dwStateOld[0] != _dwState[0] && (dwStateOld[0] == PGF_INVISIBLE || _dwState[0] == PGF_INVISIBLE)) ||
            (dwStateOld[1] != _dwState[1] && (dwStateOld[1] == PGF_INVISIBLE || _dwState[1] == PGF_INVISIBLE)) 
           ) {
            RedrawWindow(ci.hwnd, NULL,NULL,RDW_INVALIDATE|RDW_ERASE);
        }

        // Check and change for horizontal mode
        if( ci.style & PGS_HORZ ) {
            FlipRect(&(pnp->rgrc[0]));
        }
    
        if( _dwState[PGB_TOPORLEFT] != PGF_INVISIBLE ) {
            //
            // Check for RTL mirrored window
            // 
            if (bHorzMirror)
                pnp->rgrc[0].bottom -= _GetButtonSize();
            else
                pnp->rgrc[0].top += _GetButtonSize();
        } else
            pnp->rgrc[0].top += _iBorder;

        if( _dwState[PGB_BOTTOMORRIGHT] != PGF_INVISIBLE ) {
            //
            // Check for RTL mirrored window
            // 
            if (bHorzMirror)
                pnp->rgrc[0].top += _GetButtonSize();
            else
                pnp->rgrc[0].bottom -= _GetButtonSize();
        } else
            pnp->rgrc[0].bottom -= _iBorder;
   
        if (pnp->rgrc[0].bottom < pnp->rgrc[0].top)
            pnp->rgrc[0].bottom = pnp->rgrc[0].top;
        
        //Change back
        if( ci.style & PGS_HORZ ) {
            FlipRect(&(pnp->rgrc[0]));
        }
    }

    return TRUE;
}

int CPager::_HitTestCursor()
{
    POINT pt;
    GetCursorPos(&pt);
    return _HitTestScreen(&pt);
}

int CPager::_HitTestScreen(POINT* ppt)
{
    RECT rc, rc1;
    GetWindowRect(ci.hwnd, &rc);

    if (!PtInRect(&rc, *ppt)) {
        return -1;
    }
    //Get the button Rects;
    rc  = _GetButtonRect(PGB_TOPORLEFT);
    rc1 = _GetButtonRect(PGB_BOTTOMORRIGHT);

    
    if (PtInRect(&rc, *ppt)) {
        return (_dwState[PGB_TOPORLEFT] != PGF_INVISIBLE ? PGB_TOPORLEFT : -1);
    }else if (PtInRect(&rc1, *ppt)) {
        return (_dwState[PGB_BOTTOMORRIGHT] != PGF_INVISIBLE ? PGB_BOTTOMORRIGHT : -1);
    }

    return -1;
}

//---------------------------------------------------------------------------------------
int CPager::_HitTest(int x, int y)
{
    POINT pt;

    pt.x = x;
    pt.y = y;
    
    ClientToScreen(ci.hwnd, &pt);
    return _HitTestScreen(&pt);
}

//---------------------------------------------------------------------------------------
void CPager::_DrawBlank(HDC hdc, int button)
{
    RECT rc;
    UINT uFlags = 0;
    int iHeight;
    BOOL fRelDC  = FALSE;
    
    if (hdc == NULL) {
        hdc = GetWindowDC(ci.hwnd);
        fRelDC = TRUE;
    }
     
    GetWindowRect(ci.hwnd, &rc);
    MapWindowRect(NULL, ci.hwnd, &rc);

    // client to window coordinates    
    OffsetRect(&rc, -rc.left, -rc.top);

    //Check for horizontal mode
    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }

    iHeight = _dwState[button] == PGF_INVISIBLE ? _iBorder : _GetButtonSize();
    switch(button) {
    case PGB_TOPORLEFT:
        rc.bottom = rc.top + iHeight;
        break;

    case PGB_BOTTOMORRIGHT:
        rc.top = rc.bottom - iHeight;
        break;
    }

    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }

    FillRectClr(hdc, &rc, _clrBk);
    if (fRelDC)
        ReleaseDC(ci.hwnd, hdc);
}

//---------------------------------------------------------------------------------------
void CPager::_DrawButton(HDC hdc, int button)
{
    RECT rc;
    UINT uFlags = 0;
    BOOL fRelDC = FALSE;
    GetWindowRect(ci.hwnd, &rc);
    MapWindowRect(NULL, ci.hwnd, &rc);
    int state = _dwState[button];
    
    if (state == PGF_INVISIBLE)
        return;
    
     if (hdc == NULL) {
        hdc = GetWindowDC(ci.hwnd);
        fRelDC = TRUE;
     }
    
    if (state & PGF_GRAYED ) {
        uFlags |= DCHF_INACTIVE;
    } else if (state & PGF_DEPRESSED ) {
        uFlags |= DCHF_PUSHED;
    } else if (state & PGF_HOT ) {
        uFlags |=  DCHF_HOT;
    }

    // screen to window coordinates    
    OffsetRect(&rc, -rc.left, -rc.top);

    //Check for horizontal mode
    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }
    
    if( ci.style & PGS_HORZ ) 
        uFlags |= DCHF_HORIZONTAL;
    
    if (button == PGB_BOTTOMORRIGHT) 
        uFlags |= DCHF_FLIPPED;

    switch(button) {
    case PGB_TOPORLEFT:
        rc.bottom = rc.top + _GetButtonSize();
        rc.left  += _iBorder;
        rc.right -= _iBorder;
        break;

    case PGB_BOTTOMORRIGHT:
        rc.top = rc.bottom - _GetButtonSize();
        rc.left  += _iBorder;
        rc.right -= _iBorder;
        break;
    default:
        ASSERT(FALSE);
    }

    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }

    SetBkColor(hdc, _clrBk);
    DrawScrollArrow(hdc, &rc, uFlags);

    if (fRelDC)
        ReleaseDC(ci.hwnd, hdc);
}

//---------------------------------------------------------------------------------------
void CPager::v_OnNCPaint()
{
    HDC hdc = GetWindowDC(ci.hwnd);
    _DrawBlank(hdc, PGB_TOPORLEFT);
    _DrawButton(hdc, PGB_TOPORLEFT);
    
    _DrawBlank(hdc, PGB_BOTTOMORRIGHT);                        
    _DrawButton(hdc, PGB_BOTTOMORRIGHT);
    ReleaseDC(ci.hwnd, hdc);
}

//---------------------------------------------------------------------------------------
void CPager::v_OnPaint(HDC hdc)
{
    //There's nothing to paint in the client area.
}
//---------------------------------------------------------------------------------------
BOOL CPager::_OnPrint(HDC hdc, UINT uFlags)
{
    //We'll be partying with the hdc in this function so save it.
    int iDC = SaveDC(hdc);

    //Print only the Non Client Area.
    if (uFlags & PRF_NONCLIENT) {        
        int cx = 0;
        int cy = 0;
        RECT rc;


         //Draw the top/left button 
        _DrawBlank(hdc, PGB_TOPORLEFT);
        _DrawButton(hdc, PGB_TOPORLEFT);

        //Draw the bottom/left button
        _DrawBlank(hdc, PGB_BOTTOMORRIGHT);                        
        _DrawButton(hdc, PGB_BOTTOMORRIGHT);

        //Is the top button visible
        if (_dwState[PGB_TOPORLEFT] != PGF_INVISIBLE) {
            //yes, find the space taken
            if ( ci.style & PGS_HORZ ) {
                cx = _GetButtonSize();
            }else {
                cy = _GetButtonSize();
            }

        }
        //Restrict the child draw area to our client area    
        GetClientRect(ci.hwnd, &rc);
        IntersectClipRect(hdc, cx, cy, cx + RECTWIDTH(rc), cy + RECTHEIGHT(rc));  

        //Since We have drawn the non client area, Nuke the PRF_NONCLIENT flag         
        uFlags &= ~PRF_NONCLIENT;
        
    }

    //Pass it to the def window proc for default processing
    DefWindowProc(ci.hwnd, WM_PRINT, (WPARAM)hdc, (LPARAM)uFlags);
    //Restore the saved  DC 
    RestoreDC(hdc, iDC);
    return TRUE;
}

//---------------------------------------------------------------------------------------
LRESULT CPager::v_OnCommand(WPARAM wParam, LPARAM lParam)
{
    // forward to parent
    return SendMessage(ci.hwndParent, WM_COMMAND, wParam, lParam);
}
//---------------------------------------------------------------------------------------
LRESULT CPager::v_OnNotify(WPARAM wParam, LPARAM lParam)
{
    // forward to parent
    LPNMHDR lpNmhdr = (LPNMHDR)lParam;
    
    return SendNotifyEx(ci.hwndParent, (HWND) -1,
                         lpNmhdr->code, lpNmhdr, ci.bUnicode);
}


//---------------------------------------------------------------------------------------
DWORD CPager::v_OnStyleChanged(WPARAM wParam, LPARAM lParam)
{
    DWORD dwChanged = CControl::v_OnStyleChanged(wParam, lParam);

    if (dwChanged & PGS_DRAGNDROP) {
        if ((ci.style & PGS_DRAGNDROP) && !_hDragProxy) {

            _hDragProxy = CreateDragProxy(ci.hwnd, PagerDragCallback, TRUE);

        } else  if (! (ci.style & PGS_DRAGNDROP)  && _hDragProxy) {

            DestroyDragProxy(_hDragProxy);
        }
    }
    
    if (dwChanged)
        CCInvalidateFrame(ci.hwnd);     // SWP_FRAMECHANGED etc.
    return dwChanged;
}


//---------------------------------------------------------------------------------------

LRESULT CPager::v_OnCreate()
{
    if (ci.style & PGS_DRAGNDROP)
        _hDragProxy = CreateDragProxy(ci.hwnd, PagerDragCallback, TRUE);
    return TRUE;
}
//---------------------------------------------------------------------------------------
void CPager::_GetChildSize()
{
    if (_hwndChild) {

        RECT rc;
        NMPGCALCSIZE nmpgcalcsize;
        int width , height;
        rc = _rcDefClient;

        if( ci.style & PGS_HORZ ) {
            nmpgcalcsize.dwFlag = PGF_CALCWIDTH;
        } else {
            nmpgcalcsize.dwFlag  = PGF_CALCHEIGHT;
        }
        nmpgcalcsize.iWidth  = RECTWIDTH(rc);    // pager width
        nmpgcalcsize.iHeight = RECTHEIGHT(rc);  // best-guess for child

        CCSendNotify(&ci, PGN_CALCSIZE, &nmpgcalcsize.hdr);

        if( ci.style & PGS_HORZ ) {
            width  = nmpgcalcsize.iWidth;
            height = RECTHEIGHT(rc);
        } else {
            width  = RECTWIDTH(rc);
            height = nmpgcalcsize.iHeight;
        }

        GetWindowRect(_hwndChild, &rc);
        MapWindowRect(NULL, ci.hwnd, &rc);
        if( ci.style & PGS_HORZ ) {
            rc.top = _iBorder;
        } else {
            rc.left = _iBorder;
        }
        rc.right = rc.left + width;
        rc.bottom = rc.top + height;
        _rcChildIdeal = rc;
    }
}

//---------------------------------------------------------------------------------------
void CPager::v_OnSize(int x, int y)
{
    if (_hwndChild) {
        RECT rc = _rcChildIdeal;
        _SetChildPos(&rc, 0);   // SetWindowPos
    }
}

//---------------------------------------------------------------------------------------
//***   _SetChildPos -- SetWindowPos of child, w/ validation
// NOTES
//  'validation' means in sane state -- min size, and not off end.
//  WARNING: we don't update *prcChild.
//  BUGBUG what happens if we're called w/ NOMOVE or NOSIZE?
void CPager::_SetChildPos(IN RECT * prcChild, UINT uFlags)
{
    POINT ptPos = _ptPos;
    RECT rcChild = *prcChild;
    RECT rcPager;

    ASSERT(!(uFlags & SWP_NOMOVE));     // won't work

    // BUGBUG (scotth): is it okay that _hwndChild is NULL sometimes?
    //  If so, should this whole function be wrapped with if (_hwndChild)
    //  or just the call to SetWindowPos below?
    ASSERT(IS_VALID_HANDLE(_hwndChild, WND));

    rcPager = _rcDefClient;


    if ( ci.style & PGS_HORZ ) {
        FlipPoint(&ptPos);
        FlipRect(&rcChild);
        FlipRect(&rcPager);
    }

    
    int yNew = ptPos.y;

    if (RECTHEIGHT(rcChild) < RECTHEIGHT(rcPager)) {
        // force to min height

        // this handles the case where: i have an ISFBand that fills up the
        // whole pager, i stretch the pager width, and the ISFBand reformats
        // to take less height, so it shrinks its height and ends up shorter
        // than the pager.
        TraceMsg(TF_PAGER, "cps.s: h=%d h'=%d", RECTHEIGHT(rcChild), RECTHEIGHT(rcPager));
        ASSERT(!(uFlags & SWP_NOSIZE));     // won't work
        rcChild.bottom = rcChild.top + RECTHEIGHT(rcPager);
        yNew = 0;
    }

    // Maximum we can scroll is child height minus pager height.
    // Here rcPager also includes scrollbutton so  we need to add that also
    /*
          ___________  Button Width
         |
         V  ---------------- Max we can scroll (yMax)
         __ |
        /  \V
         - ---------pager-----------
        |  |-------------------------|--------------------------------
        | ||                         |                                |
        | ||    child                |                                |
        |  |-------------------------|--------------------------------
         - -------------------------
        \/\/
Border  |  |
   <-----  -------------->We need to take care of this gap.
       \-----------------------------/
        ^
        |______  RECTHEIGHT(rcChild) - RECTHEIGHT(rcPager)
       
            rcPager
     We need to add the difference between the button size and border to 
    */
    int yMax = RECTHEIGHT(rcChild) - RECTHEIGHT(rcPager) + (_GetButtonSize() - _iBorder);

    // make sure we don't end up off the top/end, and we always show
    // at least 1 page worth (if we have that much)
    // n.b. pager can override client's policy (BUGBUG?)
    if (yNew < 0) {
        // 1st page
        yNew = 0;
    } else if (yNew  > yMax) {
        // last page
        yNew = yMax;
    }

    int yOffset = yNew;
    
    // When the top button is grayed we do not want to display our child away from the button . 
    // it should be drawn right below the button. For this we tweak the position of the child window.

    //Check for the condition of grayed top button in which case we need to set position even behind
    // so that the child window falls below the grayed button
    if( _dwState[PGB_TOPORLEFT] & PGF_GRAYED )
    {
        yOffset += (_GetButtonSize() - _iBorder);
    }

    //yOffset is the tweaked value. Its just for making the child window to appear below the grayed button
    
    OffsetRect(&rcChild, 0, -yOffset - rcChild.top);

    //yNew is the actual logical positon of the window .
    ptPos.y = yNew;


    if (ci.style & PGS_HORZ) {
        // restore for copy and SWP
        FlipPoint(&ptPos);
        FlipRect(&rcChild);
    }

    _ptPos = ptPos;

    SetWindowPos(_hwndChild, NULL, rcChild.left, rcChild.top, RECTWIDTH(rcChild), RECTHEIGHT(rcChild), uFlags);

    return;
}
//---------------------------------------------------------------------------------------
//***   PGFToPGNDirection -- convert PGB_TOPORLEFT/btmorright to up/down/left/right
// NOTES
//  BUGBUG maybe PGN_* should we just take the PGF flags?
//  BUGBUG should make a macro (including some ordering magic)
DWORD CPager::_PGFToPGNDirection(DWORD dwDir)
{
    ASSERT(dwDir == PGB_TOPORLEFT || dwDir == PGB_BOTTOMORRIGHT);
    if (ci.style & PGS_HORZ) {
        return (dwDir == PGB_TOPORLEFT) ? PGF_SCROLLLEFT : PGF_SCROLLRIGHT;
    }
    else {
        return (dwDir == PGB_TOPORLEFT) ? PGF_SCROLLUP : PGF_SCROLLDOWN;
    }
}
//---------------------------------------------------------------------------------------
void CPager::_Scroll(DWORD dwDirection)
{
    RECT rc;
    NMPGSCROLL nmpgscroll;
    int iXoffset =0, iYoffset=0;
    WORD fwKeys = 0;
    int iNewPos ;
    
    // if grayed, you can't scroll.
    if (_dwState[dwDirection] & PGF_GRAYED)
        return;

    if (GetKeyState(VK_CONTROL) < 0 )
        fwKeys |= PGK_CONTROL;

    if (GetKeyState(VK_SHIFT) < 0 )
        fwKeys |= PGK_SHIFT;

    if (GetKeyState(VK_MENU) < 0 )
        fwKeys |= PGK_MENU;

    dwDirection = _PGFToPGNDirection(dwDirection);

    // set some defaults
    GetClientRect(ci.hwnd, &rc);
    nmpgscroll.fwKeys  = fwKeys;
    nmpgscroll.rcParent = rc;
    nmpgscroll.iXpos  = _ptPos.x;
    nmpgscroll.iYpos  = _ptPos.y;
    nmpgscroll.iDir   = dwDirection;

    int iScroll = (ci.style & PGS_HORZ) ? RECTWIDTH(rc) : RECTHEIGHT(rc);
    if (_cLinesPerTimeout)
        iScroll = _cLinesPerTimeout  * _cPixelsPerLine;

    nmpgscroll.iScroll = iScroll;

    // let client override
    CCSendNotify(&ci, PGN_SCROLL, &nmpgscroll.hdr);

    // do it
    switch (dwDirection)
    {
        case PGF_SCROLLDOWN:
            iNewPos = _ptPos.y + nmpgscroll.iScroll;
            break;

        case PGF_SCROLLUP:
            iNewPos = _ptPos.y - nmpgscroll.iScroll;
            break;

        case PGF_SCROLLRIGHT:
            iNewPos = _ptPos.x + nmpgscroll.iScroll;
            break;

        case PGF_SCROLLLEFT:
            iNewPos = _ptPos.x - nmpgscroll.iScroll;
            break;
    }

    _OnSetPos(iNewPos);

}
//---------------------------------------------------------------------------------------
void CPager::_OnLButtonChange(UINT uMsg,LPARAM lParam)
{
    POINT pt;
    int iButton;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    iButton = _HitTest(pt.x, pt.y);
    
    if( uMsg == WM_LBUTTONDOWN ) {

        // Check the button is valid and is not grayed 
        // if it is grayed then dont do anything
        if (iButton >= 0) {
            SetCapture(ci.hwnd);
            _fOwnsButtonDown = TRUE;
            _iButtonTrack = iButton;
            _dwState[iButton] |= PGF_DEPRESSED;
            _DrawButton(NULL, iButton);
            _Scroll(iButton);
            SetTimer(ci.hwnd, PGT_SCROLL, _cTimeout * 4, NULL);
        }
            
    } else {
        if (_iButtonTrack >= 0) {
            _dwState[_iButtonTrack] &= ~PGF_DEPRESSED;
            _DrawButton(NULL, _iButtonTrack);
            _iButtonTrack = -1;
        }
        _KillTimer();
        
        if (iButton < 0)
            _OnMouseLeave();
    }
}
//---------------------------------------------------------------------------------------
RECT  CPager :: _GetButtonRect(int iButton)
{
    RECT rc;

    GetWindowRect(ci.hwnd, &rc);

    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }

    //
    // Mirror the rects if the parent is mirrored
    //
    if (((ci.dwExStyle & RTL_MIRRORED_WINDOW) && (ci.style & PGS_HORZ))) {
        switch (iButton) {
        case PGB_TOPORLEFT:
            iButton = PGB_BOTTOMORRIGHT;
            break;

        case PGB_BOTTOMORRIGHT:
            iButton = PGB_TOPORLEFT;
            break;
        }
    }

    switch(iButton) {
    case PGB_TOPORLEFT:
        rc.bottom = rc.top +  _GetButtonSize();        
        rc.left  += _iBorder;
        rc.right -= _iBorder;
        break;
        
    case PGB_BOTTOMORRIGHT:
        rc.top  = rc.bottom - _GetButtonSize();
        rc.left  += _iBorder;
        rc.right -= _iBorder;
        break;
    }

    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
    }
    return rc;
}

//---------------------------------------------------------------------------------------
void CPager :: _OnMouseLeave()
{
    //Whether we leave the window (WM_MOUSELEAVE) or Leave one of the scroll buttons (WM_MOUSEMOVE)
    // We do the same thing. 

    // We are leaving the pager window.
    if (GetCapture() == ci.hwnd) {
        CCReleaseCapture(&ci);
    }

    // if we are tracking some button then release that mouse and that button
    if (_iButtonTrack >= 0)  {
        _iButtonTrack = -1;
    }
    
    if (_dwState[PGB_TOPORLEFT] & (PGF_HOT | PGF_DEPRESSED)) {
        _dwState[PGB_TOPORLEFT] &= ~(PGF_HOT | PGF_DEPRESSED);
        _DrawButton(NULL, PGB_TOPORLEFT);
    }
    
    if (_dwState[PGB_BOTTOMORRIGHT] & (PGF_HOT | PGF_DEPRESSED)) {
        _dwState[PGB_BOTTOMORRIGHT] &= ~(PGF_HOT | PGF_DEPRESSED);
        _DrawButton(NULL, PGB_BOTTOMORRIGHT);
    }

    _KillTimer();
    _fOwnsButtonDown = FALSE;
    //If any of the button is in gray state then it needs to be removed.
    if ((_dwState[PGB_TOPORLEFT] & PGF_GRAYED) || (_dwState[PGB_BOTTOMORRIGHT] & PGF_GRAYED))  {
        //This forces a recalc for scrollbars and removes those that are not needed
        CCInvalidateFrame(ci.hwnd);
    }
}


//---------------------------------------------------------------------------------------
void CPager::_OnMouseMove(WPARAM wParam, LPARAM lparam) 
{
    RECT rc;
    POINT pt;
    int iButton;

    pt.x = GET_X_LPARAM(lparam);
    pt.y = GET_Y_LPARAM(lparam);

    // Ignore zero-mouse moves
    if (pt.x == _ptLastMove.x && pt.y == _ptLastMove.y)
        return;

    _ptLastMove = pt;
    iButton = _HitTest(pt.x, pt.y);

    if (_iButtonTrack >= 0 ) 
    {        
        
        if (_dwState[_iButtonTrack] != PGF_INVISIBLE)
        {
            //Some Button is pressed right now
            ClientToScreen(ci.hwnd,  &pt);
            rc = _GetButtonRect(_iButtonTrack);

            DWORD dwOldState = _dwState[_iButtonTrack];
            if (PtInRect(&rc, pt)) 
            {
                _dwState[_iButtonTrack] |= PGF_DEPRESSED;
            } 
            else 
            {
                _dwState[_iButtonTrack] &= ~PGF_DEPRESSED;
            }
        
            if (dwOldState != _dwState[_iButtonTrack]) 
                _DrawButton(NULL, _iButtonTrack);
        }
        
        // if we were tracking it, but the mouse is up and gone
        if (GetCapture() == ci.hwnd && !((wParam & MK_LBUTTON) || (ci.style & PGS_AUTOSCROLL)) && iButton != _iButtonTrack)
            _OnMouseLeave();

    } 
    else 
    { 
        // No button  is pressed .
        if( iButton >= 0 ) 
        {

            //Capture the mouse so that we can keep track of when the mouse is leaving our button            
            SetCapture(ci.hwnd);
            
            // if the style is PGS_AUTOSCROLL then we dont make the button hot when hovering 
            // over button.

            //Is PGS_AUTOSCROLL set 
            _dwState[iButton] |= PGF_HOT;
            if (ci.style & PGS_AUTOSCROLL) 
            {
                _dwState[iButton] |= PGF_DEPRESSED;
            }

            //If the lbutton is down and the mouse is over one of the button then 
            // someone is trying to do drag and drop so autoscroll to help them.
            // Make sure the lbutton down did not happen in the  button before scrolling
            if ( ((wParam & MK_LBUTTON) && 
                  (_iButtonTrack < 0)) || 
                 (ci.style & PGS_AUTOSCROLL) ) 
            {
                _iButtonTrack = iButton;
                SetTimer(ci.hwnd, PGT_SCROLL, _cTimeout, NULL);
            }
            _DrawButton(NULL, iButton);
        }
        else
        {

            //Mouse is not over any button or it has left one of the scroll buttons.
            //In either case call _OnMouseLeave
           
            _OnMouseLeave();
        }
        
    }
}
//---------------------------------------------------------------------------------------
void CPager::_OnSetChild(HWND hwnd, HWND hwndChild)
{
    ASSERT(IS_VALID_HANDLE(hwndChild, WND));

    RECT rc;
    _hwndChild = hwndChild;
    _ptPos.x  = 0;
    _ptPos.y  = 0;
    _fReCalcSend = FALSE;
    if (GetCapture() == ci.hwnd)
    {
        CCReleaseCapture(&ci);
    }
    _iButtonTrack = -1;
    GetClientRect(hwnd, &rc);

    _OnReCalcSize();
}
//---------------------------------------------------------------------------------------
void CPager::_OnReCalcSize()
{
    RECT rc;
    CCInvalidateFrame(ci.hwnd);     // SWP_FRAMECHANGED etc.
    _fReCalcSend = FALSE;
    rc = _rcChildIdeal;
    _SetChildPos(&rc, 0);   // SetWindowPos

}
//---------------------------------------------------------------------------------------
void CPager::_OnSetPos(int iPos)
{
    RECT rc = _rcChildIdeal;

    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
        FlipPoint(&_ptPos);
    }

    int height;
    if (iPos < 0)
        iPos = 0;

    height = RECTHEIGHT(rc);

    if( iPos < 0  ||  iPos >  height || _ptPos.y == iPos ) {
        //Invalid Position specified or no change . Igonore it.
        return;
    }

    _ptPos.y = iPos;

    if( ci.style & PGS_HORZ ) {
        FlipRect(&rc);
        FlipPoint(&_ptPos);
    }

    CCInvalidateFrame(ci.hwnd);
    _SetChildPos(&rc , 0);
}

//---------------------------------------------------------------------------------------
int  CPager::_OnGetPos()
{
    if( ci.style  & PGS_HORZ ) {
        return _ptPos.x;
    }else{
        return _ptPos.y;
    }
}
//---------------------------------------------------------------------------------------
DWORD CPager::_GetButtonState(int iButton)
{
    
    DWORD dwState = 0;
    // Is the button id valid ?
    if ((iButton == PGB_TOPORLEFT) || (iButton == PGB_BOTTOMORRIGHT))
    {
        //yes , Get the current state of the button
        dwState = _dwState[iButton];
    }
    return dwState;
}
//---------------------------------------------------------------------------------------
void CPager::_OnTimer(UINT id)
{
    switch (id)
    {
    case PGT_SCROLL:
        if (_iButtonTrack >= 0)
        {
            // set it again because we do it faster every subsequent time
            SetTimer(ci.hwnd, PGT_SCROLL, _cTimeout, NULL);
            if (_HitTestCursor() == _iButtonTrack)
            {
                _Scroll(_iButtonTrack);
            }
            else if (!_fOwnsButtonDown) 
            {
                // if we don't own the mouse tracking (ie, the user didn't button down on us to begin with,
                // then we're done once we leave the button
                _OnMouseLeave();
            }
        }
        break;
    }
}

void CPager::_KillTimer()
{
    KillTimer(ci.hwnd, PGT_SCROLL);
    _fTimerSet = FALSE;
}
//---------------------------------------------------------------------------------------
int  CPager::_OnSetBorder(int iBorder)
{
    int iOld = _iBorder;
    int iNew = iBorder;

    //Border can't be negative
    if (iNew < 0 )
    {
        iNew = 0;
    }

    //Border can't be bigger than the button size
    if (iNew > _GetButtonSize())
    {
       iNew = _GetButtonSize();
    }
    
    _iBorder = iNew;
    CCInvalidateFrame(ci.hwnd);
    RECT rc = _rcChildIdeal;
    _SetChildPos(&rc, 0);   // SetWindowPos
    return iOld;
}

//---------------------------------------------------------------------------------------
int  CPager::_OnSetButtonSize(int iSize)
{
    int iOldSize = _iButtonSize;
    _iButtonSize = iSize;
        
    if (_iButtonSize < MINBUTTONSIZE) 
    {
        _iButtonSize = MINBUTTONSIZE;
    }

    // Border can't be bigger than button size
    if (_iBorder > _iButtonSize)
    {
        _iBorder = _iButtonSize;
    }

    CCInvalidateFrame(ci.hwnd);
    RECT rc = _rcChildIdeal;
    _SetChildPos(&rc, 0);   // SetWindowPos
    return iOldSize;

}

//---------------------------------------------------------------------------------------
LRESULT CPager::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case PGM_GETDROPTARGET:
        if (!_hDragProxy)
            _hDragProxy = CreateDragProxy(ci.hwnd, PagerDragCallback, FALSE);
        
        GetDragProxyTarget(_hDragProxy, (IDropTarget**)lParam);
        break;

    case PGM_SETSCROLLINFO:
        _cLinesPerTimeout = LOWORD(lParam);
        _cPixelsPerLine = HIWORD(lParam);
        _cTimeout = (UINT)wParam;
        break;
        
    case PGM_SETCHILD:
        _OnSetChild(hwnd, (HWND)lParam);
        break;

    case PGM_RECALCSIZE:
        if (!_fReCalcSend )
        {
            _fReCalcSend = TRUE;
            PostMessage(hwnd, PGMP_RECALCSIZE, wParam, lParam);
        }
        break;

    case PGMP_RECALCSIZE:
         _OnReCalcSize();
         break;

    case PGM_FORWARDMOUSE:
        // forward mouse messages
        _fForwardMouseMsgs = BOOLIFY(wParam);
        break;

        
    case PGM_SETBKCOLOR:
    {
        COLORREF clr = _clrBk;
        if ((COLORREF) lParam == CLR_DEFAULT)
            _clrBk = g_clrBtnFace;
        else
            _clrBk = (COLORREF)lParam;
        _fBkColorSet = TRUE;
        CCInvalidateFrame(ci.hwnd);
        //Force a paint
        RedrawWindow(ci.hwnd, NULL,NULL,RDW_INVALIDATE|RDW_ERASE);
        return clr;
    }

    case PGM_GETBKCOLOR:
        return (LRESULT)_clrBk;    

    case PGM_SETBORDER:
        return _OnSetBorder((int)lParam);

    case PGM_GETBORDER:
        return (LRESULT)_iBorder;
        
    case PGM_SETPOS:
        _OnSetPos((int)lParam);
        break;

    case PGM_GETPOS:
        return _OnGetPos();

    case PGM_SETBUTTONSIZE:
        return _OnSetButtonSize((int)lParam);

    case PGM_GETBUTTONSIZE:
        return _GetButtonSize();
    
    case PGM_GETBUTTONSTATE:
        return _GetButtonState((int)lParam);

    case WM_PRINT:
        return _OnPrint((HDC)wParam, (UINT)lParam);

    case WM_NCHITTEST:
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        if (_HitTestScreen(&pt) == -1)
            return HTTRANSPARENT;
        return HTCLIENT;
    }

    case WM_SYSCOLORCHANGE:
        if (!_fBkColorSet)
        {
            InitGlobalColors();
            _clrBk = g_clrBtnFace;
            CCInvalidateFrame(ci.hwnd);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(_hwndChild);
        return 0;

    case WM_LBUTTONDOWN:
        //Fall Through
    case WM_LBUTTONUP:
        if(!(ci.style & PGS_AUTOSCROLL)) {        
            _OnLButtonChange(uMsg,lParam);
        }
        break;

    case WM_MOUSEMOVE:
        // Only forward if the point is within the client rect of pager.
        if (_fForwardMouseMsgs && _hwndChild)
        {
            POINT pt;
            RECT rcClient;

            // BUGBUG (scotth): cache this
            GetClientRect(ci.hwnd, &rcClient);

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // Is this point in our client rect?
            if (PtInRect(&rcClient, pt))
            {
                // Yes; then convert coords and forward it
                pt.x += _ptPos.x;
                pt.y += _ptPos.y;

                SendMessage(_hwndChild, WM_MOUSEMOVE, wParam, MAKELPARAM(pt.x, pt.y));
            }
        }

        _OnMouseMove(wParam,lParam);
        break;

    case WM_MOUSELEAVE :
        _OnMouseLeave();
        break;

    case WM_ERASEBKGND:
    {
        LRESULT lres = CCForwardEraseBackground(ci.hwnd, (HDC) wParam);

        if (_iBorder) {
            // paint the borders
            RECT rc;
            RECT rc2;
            GetClientRect(ci.hwnd, &rc);
            rc2 = rc;
        
            if( ci.style & PGS_HORZ ) {
                FlipRect(&rc2);
            }
            rc2.right = rc2.left + _iBorder + 1;

            if( ci.style & PGS_HORZ ) {
                FlipRect(&rc2);
            }            
            FillRectClr((HDC)wParam, &rc2, _clrBk);
            rc2 = rc;

            if( ci.style & PGS_HORZ ) {
                FlipRect(&rc2);
            }
            rc2.left = rc2.right - _iBorder - 1;

            if( ci.style & PGS_HORZ ) {
                FlipRect(&rc2);
            }

            FillRectClr((HDC)wParam, &rc2, _clrBk);
        }
        return TRUE;
    }

    case WM_TIMER:
        _OnTimer((UINT)wParam);
        return 0;       

    case WM_SETTINGCHANGE:
        InitGlobalMetrics(wParam);
        _iButtonSize = (int) g_cxScrollbar * 3 / 4;
        if (_iButtonSize < MINBUTTONSIZE) {
            _iButtonSize = MINBUTTONSIZE;
        }
        break;

    case WM_DESTROY:
        if (_hDragProxy)
            DestroyDragProxy(_hDragProxy);
        break;
    }
    return CControl::v_WndProc(hwnd, uMsg, wParam, lParam);
}

//---------------------------------------------------------------------------------------
// call with cyCh == 0 to specify auto vsizing
BOOL DrawChar(HDC hdc, LPRECT lprc, UINT wState, TCHAR ch, UINT cyCh, BOOL fAlwaysGrayed, BOOL fTopAlign)
{
    COLORREF rgb;
    BOOL    fDrawDisabled = !fAlwaysGrayed && (wState & DCHF_INACTIVE);
    BOOL    fDrawPushed = wState & DCHF_PUSHED;
    // Bad UI to have a pushed disabled button
    ASSERT (!fDrawDisabled || !fDrawPushed);
    RECT rc = *lprc;
    UINT uFormat = DT_CENTER | DT_SINGLELINE;

    if (fAlwaysGrayed)
        rgb = g_clrBtnShadow;
    else if (fDrawDisabled)
        rgb = g_clrBtnHighlight;
    else 
        rgb = g_clrBtnText;
    
    rgb = SetTextColor(hdc, rgb);

    if (cyCh)
    {
        if (fTopAlign)
            rc.bottom = rc.top + cyCh;
        else
        {
            rc.top += ((RECTHEIGHT(rc) - cyCh) / 2);
            rc.bottom = rc.top + cyCh;
        }
        uFormat |= DT_BOTTOM;
    }
    else
        uFormat |= DT_VCENTER;

    if (fDrawDisabled || fDrawPushed)
        OffsetRect(&rc, 1, 1);

    DrawText(hdc, &ch, 1, &rc, uFormat);

    if (fDrawDisabled)
    {
        OffsetRect(&rc, -1, -1);
        SetTextColor(hdc, g_clrBtnShadow);
        DrawText(hdc, &ch, 1, &rc, uFormat);
    }

    SetTextColor(hdc, rgb);
    return(TRUE);
}

void DrawBlankButton(HDC hdc, LPRECT lprc, DWORD wControlState)
{
    BOOL fAdjusted;

    if (wControlState & (DCHF_HOT | DCHF_PUSHED) &&
        !(wControlState & DCHF_NOBORDER)) {
        COLORSCHEME clrsc;

        clrsc.dwSize = 1;
        if (GetBkColor(hdc) == g_clrBtnShadow) {
            clrsc.clrBtnHighlight = g_clrBtnHighlight;
            clrsc.clrBtnShadow = g_clrBtnText;
        } else
            clrsc.clrBtnHighlight = clrsc.clrBtnShadow = CLR_DEFAULT;

        // if button is both DCHF_HOT and DCHF_PUSHED, DCHF_HOT wins here
        CCDrawEdge(hdc, lprc, (wControlState & DCHF_HOT) ? BDR_RAISEDINNER : BDR_SUNKENOUTER,
                 (UINT) (BF_ADJUST | BF_RECT), &clrsc);
        fAdjusted = TRUE;
    } else {
        fAdjusted = FALSE;
    }

    if (!(wControlState & DCHF_TRANSPARENT))
        FillRectClr(hdc, lprc, GetBkColor(hdc));
    
    if (!fAdjusted)
        InflateRect(lprc, -g_cxBorder, -g_cyBorder);
}

//---------------------------------------------------------------------------------------
void DrawCharButton(HDC hdc, LPRECT lprc, UINT wControlState, TCHAR ch, UINT cyCh, BOOL fAlwaysGrayed, BOOL fTopAlign)
{
    RECT rc;
    CopyRect(&rc, lprc);

    DrawBlankButton(hdc, &rc, wControlState);

    if ((RECTWIDTH(rc) <= 0) || (RECTHEIGHT(rc) <= 0))
        return;

#if defined(UNIX)
    HBITMAP hbit;
    int x,y,width,height;

    x = rc.left + (rc.right  - rc.left) /2;
    y = rc.top  + (rc.bottom - rc.top ) /2;

    if (wControlState & (DCHF_INACTIVE | DCHF_PUSHED))
    {
        x++;
        y++;
    }

    UnixPaintArrow( hdc, 
         (wControlState & DCHF_HORIZONTAL), 
         (wControlState & DCHF_FLIPPED), 
         x,y,
         min(ARROW_WIDTH,  (rc.right  - rc.left)),
         min(ARROW_HEIGHT, (rc.bottom - rc.top ))
        );

#else
    
    int iOldBk = SetBkMode(hdc, TRANSPARENT);
    DrawChar(hdc, &rc, wControlState, ch, cyCh, fAlwaysGrayed, fTopAlign);
    SetBkMode(hdc, iOldBk);

#endif

}

// --------------------------------------------------------------------------------------
//
//  DrawScrollArrow
//
// --------------------------------------------------------------------------------------
void DrawScrollArrow(HDC hdc, LPRECT lprc, UINT wControlState)
{
#define szfnMarlett  TEXT("MARLETT")
    TCHAR ch = (wControlState & DCHF_HORIZONTAL) ? TEXT('3') : TEXT('5');

    //
    // Flip the direction arrow in case of a RTL mirrored DC,
    // since it won't be flipped automatically (textout!)
    //
    if (IS_DC_RTL_MIRRORED(hdc) && (wControlState & DCHF_HORIZONTAL))
        wControlState ^= DCHF_FLIPPED;

    LONG lMin = min(RECTWIDTH(*lprc), RECTHEIGHT(*lprc)) - (2 * g_cxBorder);  // g_cxBorder fudge notches font size down

    HFONT hFont = CreateFont(lMin, 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0, szfnMarlett);
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    if (wControlState & DCHF_FLIPPED)
        ch++;
    
    DrawCharButton(hdc, lprc, wControlState, ch, 0, FALSE, FALSE);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);   
}

//---------------------------------------------------------------------------------------

#define CX_EDGE         2
#define CX_LGEDGE       4

#define CX_INCREMENT    1
#define CX_DECREMENT    (-CX_INCREMENT)

#define MIDPOINT(x1, x2)        ((x1 + x2) / 2)
#define CHEVRON_WIDTH(dSeg)     (4 * dSeg)

void DrawChevron(HDC hdc, LPRECT lprc, DWORD dwFlags)
{
    RECT rc;
    CopyRect(&rc, lprc);

    // draw the border and background
    DrawBlankButton(hdc, &rc, dwFlags);

    // offset the arrow if pushed
    if (dwFlags & DCHF_PUSHED)
        OffsetRect(&rc, CX_INCREMENT, CX_INCREMENT);

    // draw the arrow
    HBRUSH hbrSave = SelectBrush(hdc, GetSysColorBrush(COLOR_BTNTEXT));
    int dSeg = (dwFlags & DCHF_LARGE) ? CX_LGEDGE : CX_EDGE;

    if (dwFlags & DCHF_HORIZONTAL)
    {
        // horizontal arrow
        int x = MIDPOINT(rc.left, rc.right - CHEVRON_WIDTH(dSeg));
        int yBase;

        if (dwFlags & DCHF_TOPALIGN)
            yBase = rc.top + dSeg + (2 * CX_EDGE);
        else
            yBase = MIDPOINT(rc.top, rc.bottom);

        for (int y = -dSeg; y <= dSeg; y++)
        {
            PatBlt(hdc, x, yBase + y, dSeg, CX_INCREMENT, PATCOPY);
            PatBlt(hdc, x + (dSeg * 2), yBase + y, dSeg, CX_INCREMENT, PATCOPY);

            x += (y < 0) ? CX_INCREMENT : CX_DECREMENT;
        }
    }
    else
    {
        // vertical arrow
        int y = rc.top + CX_INCREMENT;
        int xBase = MIDPOINT(rc.left, rc.right);

        for (int x = -dSeg; x <= dSeg; x++)
        {
            PatBlt(hdc, xBase + x, y, CX_INCREMENT, dSeg, PATCOPY);
            PatBlt(hdc, xBase + x, y + (dSeg * 2), CX_INCREMENT, dSeg, PATCOPY);

            y += (x < 0) ? CX_INCREMENT : CX_DECREMENT;
        }
    }

    // clean up
    SelectBrush(hdc, hbrSave);
}
