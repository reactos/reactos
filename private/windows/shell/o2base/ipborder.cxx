//+---------------------------------------------------------------------
//
//   File:       ipborder.cxx
//
//   Classes:    InPlaceBorder
//
//   Notes:      Use of this class limits windows to the use
//               of the non-client region for UIAtive borders
//               only: Standard (non-control window) scroll bars
//               are specifically NOT supported.
//
//   History:    14-May-93   CliffG        Created.
//
//------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define IPB_GRABFACTOR  3

#define SetWF(hwnd,wf)  SetWindowLong(hwnd, GWL_STYLE, \
            GetWindowLong(hwnd,GWL_STYLE) | (wf))
#define ClrWF(hwnd,wf)  SetWindowLong(hwnd, GWL_STYLE, \
            GetWindowLong(hwnd,GWL_STYLE) &~(wf))
#define TestWF(hwnd,wf) (GetWindowLong(hwnd,GWL_STYLE) & (wf))


    WORD InPlaceBorder::_cUsage = 0;
    HBRUSH InPlaceBorder::_hbrActiveCaption;
    HBRUSH InPlaceBorder::_hbrInActiveCaption;


//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::DrawFrame
//
//---------------------------------------------------------------
void
InPlaceBorder::DrawFrame(HWND hwnd)
{
    if(!_fUIActive)
        return;

    HDC hdc = GetWindowDC(hwnd);

    HBRUSH hbr;
    if(_fParentActive)
        hbr = _hbrActiveCaption;
    else
        hbr = _hbrInActiveCaption;
    RECT rcWhole;
    GetWindowRect(hwnd, &rcWhole);
    OffsetRect(&rcWhole, -rcWhole.left, -rcWhole.top);
    RECT rc;
    //Top
    rc = rcWhole;
    rc.bottom = rc.top + _cyFrame;
    FillRect(hdc, &rc, hbr);
    //Left
    rc = rcWhole;
    rc.right = rc.left + _cxFrame;
    FillRect(hdc, &rc, hbr);
    //Bottom
    rc = rcWhole;
    rc.top = rc.bottom - _cyFrame;
    FillRect(hdc, &rc, hbr);
    //Right
    rc = rcWhole;
    rc.left = rc.right - _cxFrame;
    FillRect(hdc, &rc, hbr);

    if(TestWF(hwnd, WS_THICKFRAME)) //Resizable window?
    {
        //
        //Draw grab handles at 4 corners of the border...
        //
        hbr = (HBRUSH)GetStockObject( BLACK_BRUSH );
        //TopLeft
        rc = rcWhole;
        rc.right = rc.left + _cxFrame;
        rc.bottom = rc.top + _cyFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
        rc.bottom = rc.top + _cyFrame;
        rc.right = rc.left + _cxFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
        //TopRight
        rc.right = rcWhole.right;
        rc.left = rc.right - _cxFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
        rc.left = rc.right - _cxFrame;
        rc.bottom = rc.top + _cyFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
        //BottomLeft
        rc = rcWhole;
        rc.top = rc.bottom - _cyFrame * IPB_GRABFACTOR;
        rc.right = rc.left + _cxFrame;
        FillRect(hdc, &rc, hbr);
        rc.top = rc.bottom - _cyFrame;
        rc.right = rc.left + _cxFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
        //BottomRight
        rc.right = rcWhole.right;
        rc.left = rc.right - _cxFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
        rc.right = rcWhole.right;
        rc.left = rc.right - _cxFrame;
        rc.top = rc.bottom - _cyFrame * IPB_GRABFACTOR;
        FillRect(hdc, &rc, hbr);
    }

    ReleaseDC(hwnd, hdc);
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::HitTest
//
//---------------------------------------------------------------
LONG
InPlaceBorder::HitTest(HWND hwnd, int x, int y)
{
    POINT pt = { x, y };
    RECT rcClient;

    GetWindowRect(hwnd, &rcClient);
    CalcClientRect(hwnd, &rcClient);
    if (PtInRect(&rcClient, pt))
        return HTCLIENT;

    // We're somewhere on the window frame.
    if (y >= rcClient.bottom)
    {
        if (x <= rcClient.left + _cxFrame * IPB_GRABFACTOR)
            return HTBOTTOMLEFT;
        if (x >= rcClient.right - _cxFrame * IPB_GRABFACTOR)
            return HTBOTTOMRIGHT;
        return HTBOTTOM;
    }
    else if (y <= rcClient.top)
    {
        if (x <= rcClient.left + _cxFrame * IPB_GRABFACTOR)
            return(HTTOPLEFT);
        if (x >= rcClient.right - _cxFrame * IPB_GRABFACTOR)
            return(HTTOPRIGHT);
        return HTTOP;
    }
    else if (x <= rcClient.left)
    {
        if (y <= rcClient.top + _cyFrame * IPB_GRABFACTOR)
            return HTTOPLEFT;
        if (y >= rcClient.bottom - _cyFrame * IPB_GRABFACTOR)
            return HTBOTTOMLEFT;
        return HTLEFT;
    }
    else
    {
        if (y <= rcClient.top + _cyFrame * IPB_GRABFACTOR)
            return HTTOPRIGHT;
        if (y >= rcClient.bottom - _cyFrame * IPB_GRABFACTOR)
            return HTBOTTOMRIGHT;
        return HTRIGHT;
    }

    return HTNOWHERE;
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::DefWindowProc
//
//---------------------------------------------------------------
LRESULT
InPlaceBorder::DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0L;
    //REVIEW code assumes an SDI app...
    if(_hwnd == NULL)
        return ::DefWindowProc(hwnd,msg,wParam,lParam);

    switch (msg)
    {
    case WM_WINDOWPOSCHANGED:
        if(_pSite != NULL && _cResizing == 0 && _fUIActive)
        {
            ++_cResizing;
            RECT rc;
            GetChildWindowRect(hwnd, &rc);
            InflateRect(&rc,-_cxFrame,-_cyFrame);
            _pSite->OnPosRectChange(&rc);
            _cResizing--;
        }
        break;
    case WM_NCCALCSIZE:     // lParam == LPRECT of window rect
        //
        //Turn off the WS_THICKFRAME style bit during
        //default processing so we keep ownership of visualization...
        //
        if (TestWF(hwnd, WS_THICKFRAME))
        {
            ClrWF(hwnd, WS_THICKFRAME);
            lRet = ::DefWindowProc(hwnd, msg, wParam,lParam);
            SetWF(hwnd, WS_THICKFRAME);
        }
        CalcClientRect(hwnd, (LPRECT)lParam);
        return lRet;

    case WM_NCHITTEST:      // lParam is POINT in screen cords
        return HitTest(hwnd, LOWORD(lParam), HIWORD(lParam));

    case WM_NCPAINT:
        DrawFrame(hwnd);
        return 0L;

    case WM_NCLBUTTONDOWN:
    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCACTIVATE:     // wParam == active state
    case WM_NCCREATE:       // Sent before WM_CREATE
    case WM_NCDESTROY:      // Sent before WM_DESTROY
        break;
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::InPlaceBorder
//
//---------------------------------------------------------------
InPlaceBorder::InPlaceBorder(void)
{
    _fParentActive = TRUE;
    _fUIActive = FALSE;
    _hwnd = NULL;
    _pSite = NULL;
    _cResizing = 0;
    _cxFrame = _cyFrame = IPBORDER_THICKNESS;
    if(_cUsage++ == 0)
    {
        //BUGBUG the following could fail and we would be hosed...
        _hbrActiveCaption = CreateHatchBrush(HS_BDIAGONAL,
            GetSysColor(COLOR_ACTIVECAPTION));
        _hbrInActiveCaption = CreateHatchBrush(HS_BDIAGONAL,
            GetSysColor(COLOR_WINDOWFRAME));
    }
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::~InPlaceBorder
//
//---------------------------------------------------------------
InPlaceBorder::~InPlaceBorder(void)
{
    if(--_cUsage == 0)
    {
        DeleteObject(_hbrActiveCaption);
        _hbrActiveCaption = NULL;
        DeleteObject(_hbrInActiveCaption);
        _hbrInActiveCaption = NULL;
    }
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::SetState
//
//Change the border state: reflected in the nonclient window border
//---------------------------------------------------------------
void
InPlaceBorder::SetUIActive(BOOL fUIActive)
{
    if(_hwnd == NULL)
        return;

    if(_fUIActive != fUIActive)
    {
        RECT rcClient;
        GetChildWindowRect(_hwnd, &rcClient);
        int cx = rcClient.right - rcClient.left;
        int cy = rcClient.bottom - rcClient.top;
        int x = rcClient.left;
        int y = rcClient.top;
        BOOL fResize = FALSE;
        if (fUIActive)
        {
            fResize = TRUE;
            cx += _cxFrame * 2;
            cy += _cyFrame * 2;
            x -= _cxFrame;
            y -= _cyFrame;
        }
        else if (_fUIActive)
        {
            fResize = TRUE;
            cx -= _cxFrame * 2;
            cy -= _cyFrame * 2;
            x += _cxFrame;
            y += _cyFrame;
        }
        if (fResize)
        {
            //
            //Set our state member up so CalcClientRect generates correct values,
            //then move the window (keeping client area same size and position)
            //
            _fUIActive = fUIActive;
            ++_cResizing;
            SetWindowPos( _hwnd, NULL, x, y, cx, cy, SWP_FRAMECHANGED |
                SWP_NOACTIVATE | SWP_NOZORDER);
            RedrawFrame();
            _cResizing--;
        }
        else
        {
            InvalidateFrame();
        }
    }
    _fUIActive = fUIActive;
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::SetSize
//
//---------------------------------------------------------------
void
InPlaceBorder::SetSize(HWND hwnd, RECT& rc)
{
    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;
    int x = rc.left;
    int y = rc.top;
    if(_fUIActive)
    {
        cx += _cxFrame * 2;
        cy += _cyFrame * 2;
        x -= _cxFrame;
        y -= _cyFrame;
    }
    ++_cResizing;
    MoveWindow(hwnd, x, y, cx, cy, TRUE);
    _cResizing--;
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::Erase
//
// Force border state to non-UIActive, managing the coresponding
// change in border appearance
//
//---------------------------------------------------------------
void
InPlaceBorder::Erase(void)
{
    if(_hwnd == NULL)
        return;
    SetUIActive(FALSE);
    _fParentActive = TRUE;
    InvalidateFrame();
    RedrawFrame();
}


//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::SetBorderSize
//
//---------------------------------------------------------------
void
InPlaceBorder::SetBorderSize( int cx, int cy )
{
    if(cx < 0)
        cx = 0;
    if(cy < 0)
        cy = 0;
    _cxFrame = cx;
    _cyFrame = cy;
    InvalidateFrame();
    RedrawFrame();
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::GetBorderSize
//
//---------------------------------------------------------------
void
InPlaceBorder::GetBorderSize( LPINT pcx, LPINT pcy )
{
    *pcx = _cxFrame;
    *pcy = _cyFrame;
}

//+---------------------------------------------------------------
//
//  Member: InPlaceBorder::Attach
//
//---------------------------------------------------------------
void
InPlaceBorder::Attach(HWND hwnd, BOOL fUIActive)
{
    if((_hwnd = hwnd) != NULL)
    {
        SetUIActive(fUIActive);
        InvalidateFrame();  //force first-time NCCALC
    }
}

void
InPlaceBorder::Bind(LPOLEINPLACESITE pSite, HWND hwnd, BOOL fUIActive)
{
    _pSite = NULL;
    if((_hwnd = hwnd) != NULL && (_pSite = pSite) != NULL)
    {
        SetUIActive(fUIActive);
        InvalidateFrame();  //force first-time NCCALC
    }
}

