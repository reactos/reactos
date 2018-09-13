//+---------------------------------------------------------------------
//
//   File:       border.cxx
//
//   Contents:   Border Object Class
//
//   Classes:    OLEBorder
//
//------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


    BOOL OLEBorder::fInit = FALSE;
    HCURSOR OLEBorder::ahc[5] = { NULL, NULL, NULL, NULL, NULL };
    int OLEBorder::iPartMap[14] = {
        ICURS_STD,  // 0 NOWHERE
        ICURS_STD,  // 1 TOP
        ICURS_STD,  // 2 RIGHT
        ICURS_STD,  // 3 BOTTOM
        ICURS_STD,  // 4 LEFT
        ICURS_NESW, // 5 TOPRIGHT
        ICURS_NWSE, // 6 BOTTOMRIGHT
        ICURS_NESW, // 7 BOTTOMLEFT
        ICURS_NWSE, // 8 TOPLEFT
        ICURS_NS,   // 9 TOPHAND
        ICURS_WE,   //10 RIGHTHAND
        ICURS_NS,   //11 BOTTOMHAND
        ICURS_WE,   //12 LEFTHAND
        ICURS_STD   //13 INSIDE
    };

HCURSOR
OLEBorder::MapPartToCursor(USHORT usPart)
{
    if (usPart > MAX_OBPART)
        usPart = 0;
    return ahc[iPartMap[usPart]];
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::InitClass
//
//---------------------------------------------------------------
void
OLEBorder::InitClass( void )
{
    _fErased = TRUE;
    _state = 0;
    _sThickness = FBORDER_THICKNESS;
    _sMinHeight = FBORDER_MINHEIGHT;
    _sMinWidth = FBORDER_MINWIDTH;
    if(fInit)
        return;

    ahc[ICURS_NWSE] = LoadCursor( NULL, IDC_SIZENWSE );
    ahc[ICURS_NESW] = LoadCursor( NULL, IDC_SIZENESW );
    ahc[ICURS_NS] = LoadCursor( NULL, IDC_SIZENS );
    ahc[ICURS_WE] = LoadCursor( NULL, IDC_SIZEWE );
    ahc[ICURS_STD] = LoadCursor( NULL, IDC_SIZEALL );
    fInit = TRUE;
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::OLEBorder
//
//---------------------------------------------------------------
OLEBorder::OLEBorder( void )
{
    rect.top = 0;
    rect.left = 0;
    rect.bottom = 0;
    rect.right = 0;
    InitClass();
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::OLEBorder
//
//---------------------------------------------------------------
OLEBorder::OLEBorder( RECT& r )
{
    rect = r;
    InitClass();
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::~OLEBorder
//
//---------------------------------------------------------------
OLEBorder::~OLEBorder( void )
{
}


//+---------------------------------------------------------------
//
//  Member: OLEBorder::SetState
//
//---------------------------------------------------------------
USHORT
OLEBorder::SetState( HDC hdc, HWND hwnd, USHORT usBorderState )
{
    if (_state ^ usBorderState)
    {
        if (hdc != NULL)
        {
            _state |= OBSTYLE_RESERVED;
            Draw(hdc,hwnd);
        }
    }
    _state = usBorderState & (~OBSTYLE_RESERVED);
    if (hdc != NULL)
        Draw(hdc,hwnd);

    return _state;
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::Erase
//
//---------------------------------------------------------------
void
OLEBorder::Erase(HWND hwnd)
{
    RECT r;
    if (hwnd != NULL && !_fErased)
    {
        _fErased = TRUE;
        if(_state & OBSTYLE_DIAGONAL_FILL)
        {
            GetBorderRect(r,BP_INSIDE);
            InvalidateRect(hwnd,&r,TRUE);
        }
        else
        {
            for(int i = BP_TOP; i <= BP_LEFT; i++)
            {
                GetBorderRect(r,i);
                InvalidateRect(hwnd,&r,TRUE);
            }
        }
    }
    _state = _state & OBSTYLE_INSIDE;
}

void
OLEBorder::GetInsideBorder( RECT& rDest, int iEdge )
{
    int sHalf = _sThickness >> 1;
    int sMid;

    switch(iEdge)
    {
    case BP_TOP:
    case BP_BOTTOM:
        rDest.left = rect.left;
        rDest.right = rect.right;
        if (iEdge == BP_TOP)
        {
            rDest.top = rect.top;
            rDest.bottom = rect.top + _sThickness;
        }
        else
        {
            rDest.top = rect.bottom - _sThickness;
            rDest.bottom = rect.bottom;
        }
        break;

    case BP_RIGHT:
    case BP_LEFT:
        rDest.top = rect.top;
        rDest.bottom = rect.bottom;
        if (iEdge == BP_RIGHT)
        {
            rDest.left = rect.right - _sThickness;
            rDest.right = rect.right;
        }
        else
        {
            rDest.left = rect.left;
            rDest.right = rect.left + _sThickness;
        }
        break;

    case BP_TOPRIGHT:
    case BP_BOTTOMRIGHT:
        rDest.left = rect.right - _sThickness;
        rDest.right = rect.right;
        if (iEdge == BP_TOPRIGHT)
        {
            rDest.top = rect.top;
            rDest.bottom = rect.top + _sThickness;
        }
        else
        {
            rDest.top = rect.bottom - _sThickness;
            rDest.bottom = rect.bottom;
        }
        break;

    case BP_BOTTOMLEFT:
    case BP_TOPLEFT:
        rDest.left = rect.left;
        rDest.right = rect.left + _sThickness;
        if (iEdge == BP_BOTTOMLEFT)
        {
            rDest.top = rect.bottom - _sThickness;
            rDest.bottom = rect.bottom;
        }
        else
        {
            rDest.top = rect.top;
            rDest.bottom = rect.top + _sThickness;
        }
        break;

    case BP_TOPHAND:
    case BP_BOTTOMHAND:
        sMid = rect.left + ((rect.right - rect.left) >> 1);
        rDest.left = sMid - sHalf;
        rDest.right = sMid + sHalf;
        if (iEdge == BP_TOPHAND)
        {
            rDest.top = rect.top;
            rDest.bottom = rect.top + _sThickness;
        }
        else
        {
            rDest.top = rect.bottom - _sThickness;
            rDest.bottom = rect.bottom;
        }
        break;

    case BP_RIGHTHAND:
    case BP_LEFTHAND:
        sMid = rect.top + ((rect.bottom - rect.top) >> 1);
        rDest.top = sMid - sHalf;
        rDest.bottom = sMid + sHalf;
        if (iEdge == BP_LEFTHAND)
        {
            rDest.left = rect.left;
            rDest.right = rect.left + _sThickness;
        }
        else
        {
            rDest.left = rect.right - _sThickness;
            rDest.right = rect.right;
        }
        break;

    case BP_INSIDE:
    default:
        rDest = rect;
        break;
    }
}

void
OLEBorder::GetOutsideBorder( RECT& rDest, int iEdge )
{
    int sHalf = _sThickness >> 1;
    int sMid;

    switch(iEdge)
    {
    case BP_TOP:
    case BP_BOTTOM:
        rDest.left = rect.left - _sThickness;
        rDest.right = rect.right + _sThickness;
        if (iEdge == BP_TOP)
        {
            rDest.top = rect.top - _sThickness;
            rDest.bottom = rect.top + 1;
        }
        else
        {
            rDest.top = rect.bottom;
            rDest.bottom = rect.bottom + _sThickness;
        }
        break;

    case BP_RIGHT:
    case BP_LEFT:
        rDest.top = rect.top - _sThickness;
        rDest.bottom = rect.bottom + _sThickness;
        if (iEdge == BP_RIGHT)
        {
            rDest.left = rect.right;
            rDest.right = rect.right + _sThickness;
        }
        else
        {
            rDest.left = rect.left - _sThickness;
            rDest.right = rect.left + 1;
        }
        break;

    case BP_TOPRIGHT:
    case BP_BOTTOMRIGHT:
        rDest.left = rect.right;
        rDest.right = rect.right + _sThickness;
        if (iEdge == BP_TOPRIGHT)
        {
            rDest.top = rect.top - _sThickness;
            rDest.bottom = rect.top + 1;
        }
        else
        {
            rDest.top = rect.bottom;
            rDest.bottom = rect.bottom + _sThickness;
        }
        break;

    case BP_BOTTOMLEFT:
    case BP_TOPLEFT:
        rDest.left = rect.left - _sThickness;
        rDest.right = rect.left + 1;
        if (iEdge == BP_BOTTOMLEFT)
        {
            rDest.top = rect.bottom;
            rDest.bottom = rect.bottom + _sThickness;
        }
        else
        {
            rDest.top = rect.top - _sThickness;
            rDest.bottom = rect.top + 1;
        }
        break;

    case BP_TOPHAND:
    case BP_BOTTOMHAND:
        sMid = rect.left + ((rect.right - rect.left) >> 1);
        rDest.left = sMid - sHalf;
        rDest.right = sMid + sHalf;
        if (iEdge == BP_TOPHAND)
        {
            rDest.top = rect.top - _sThickness;
            rDest.bottom = rect.top + 1;
        }
        else
        {
            rDest.top = rect.bottom;
            rDest.bottom = rect.bottom + _sThickness;
        }
        break;

    case BP_RIGHTHAND:
    case BP_LEFTHAND:
        sMid = rect.top + ((rect.bottom - rect.top) >> 1);
        rDest.top = sMid - sHalf;
        rDest.bottom = sMid + sHalf;
        if (iEdge == BP_LEFTHAND)
        {
            rDest.left = rect.left - _sThickness;
            rDest.right = rect.left + 1;
        }
        else
        {
            rDest.left = rect.right;
            rDest.right = rect.right + _sThickness;
        }
        break;

    case BP_INSIDE:
    default: //inactive border
        rDest.left = rect.left - 1;
        rDest.right = rect.right + 1;
        rDest.top = rect.top - 1;
        rDest.bottom = rect.bottom + 1;
        break;
    }
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::GetBorderRect
//
//---------------------------------------------------------------
void
OLEBorder::GetBorderRect( RECT& rDest, int iEdge )
{
    if(_state & OBSTYLE_INSIDE)
        GetInsideBorder(rDest,iEdge);
    else
        GetOutsideBorder(rDest,iEdge);
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::SwitchCoords
//
//---------------------------------------------------------------
void
OLEBorder::SwitchCoords( HWND hwndFrom, HWND hwndTo )
{
    MapWindowPoints(hwndFrom, hwndTo, (LPPOINT)&rect, 2);
    MapWindowPoints(hwndFrom, hwndTo, &_pt, 1);
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::Draw
//
//---------------------------------------------------------------
void
OLEBorder::Draw( HDC hdc, HWND hwnd )
{
    if(hdc == NULL || (_state & ~OBSTYLE_INSIDE) == 0)
    {
        return;     //nothing to do!
    }

    RECT r;
    //
    //BUGBUG the following should be rewritten so any border style
    //       can be drawn in XOR mode...
    //
    if (_state & OBSTYLE_XOR)
    {
        if (_state & OBSTYLE_THICK)
        {
            PatBlt(hdc, rect.left - 1, rect.top - 1,
                rect.right - rect.left + 2, 3, PATINVERT);

            PatBlt(hdc, rect.left - 1, rect.bottom - 2,
                rect.right - rect.left + 2, 3, PATINVERT);

            PatBlt(hdc, rect.left - 1, rect.top + 2,
                3, rect.bottom - rect.top - 4, PATINVERT);

            PatBlt(hdc, rect.right - 2, rect.top + 2,
                3, rect.bottom - rect.top - 4, PATINVERT);
        }
        else
            DrawFocusRect(hdc,&rect);
        return;
    }

    HBRUSH hbrBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
    HBRUSH hbr;
    COLORREF clrref;
    int i;

    if (_state & OBSTYLE_RESERVED)
    {
        Erase(hwnd);
        return;
    }

    if (_state & OBSTYLE_ACTIVE)
    {
        clrref = GetSysColor(COLOR_ACTIVECAPTION);
    }
    else
    {
        clrref = GetSysColor(COLOR_WINDOWFRAME);
    }

    if ((_state & OBSTYLE_TYPEMASK) == OBSTYLE_DIAGONAL_FILL)
    {
        hbr = CreateHatchBrush(HS_DIAGCROSS,clrref);
        GetBorderRect(r,BP_INSIDE);
        FillRect(hdc,&r,hbr);
        DeleteObject(hbr);
    }
    else if ((_state & OBSTYLE_TYPEMASK) == OBSTYLE_SOLID_PEN)
    {
        GetBorderRect(r,BP_INSIDE);
        FrameRect(hdc,&r,hbrBlack);
    }

    if (_state & OBSTYLE_THICK)
    {
        if (_state & OBSTYLE_INSIDE)
            InflateRect(&r,-1,-1);
        else
            InflateRect(&r,1,1);
        FrameRect(hdc,&rect,hbrBlack);
    }

    if (_state & OBSTYLE_HANDLED)
    {
        for (i = BP_TOPRIGHT; i <= BP_TOPLEFT; i++)
        {
            GetBorderRect(r,i);
            FillRect(hdc,&r,hbrBlack);
        }
        for (i = BP_TOPHAND; i <= BP_LEFTHAND; i++)
        {
            GetBorderRect(r,i);
            FillRect(hdc,&r,hbrBlack);
        }
    }
    _fErased = FALSE;
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::QueryHit
//
//---------------------------------------------------------------
USHORT
OLEBorder::QueryHit( POINT point )
{
    RECT r = rect;
    USHORT usWhere = BP_NOWHERE;

    if ((_state & OBSTYLE_INSIDE) == 0)
        InflateRect(&r,_sThickness,_sThickness);
    if (PtInRect(&r,point))
    {
        usWhere = BP_INSIDE;
        //
        //test against the real inside to optimize this case...
        //
        InflateRect(&r,-_sThickness,-_sThickness);

        //
        // PtInRect counts the top and left borders as being inside, so
        // we must account for this.
        //
        r.left++;
        r.top++;
        if (!PtInRect(&r,point))
        {
            //
            //Search for the "handle" that was hit...
            //
            int i;
            for (i = BP_LEFTHAND; i >= BP_TOP; i--)
            {
                GetBorderRect(r,i);
                if (PtInRect(&r,point))
                {
                    usWhere = (USHORT)i;
                    break;
                }
            }
        }
    }
    return(usWhere);
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::QueryMoveCursor
//
//---------------------------------------------------------------
HCURSOR
OLEBorder::QueryMoveCursor( POINT ptCurrent, BOOL fMustMove )
{
    //
    //Stash part-hit info so we can do the right thing durring
    //upcomming move/size operation
    //
    if (fMustMove)
    {
        _usPart = BP_INSIDE;
    }
    else
    {
        _usPart = QueryHit(ptCurrent);
    }
    return MapPartToCursor(_usPart);
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::BeginMove
//
//---------------------------------------------------------------
HCURSOR
OLEBorder::BeginMove( HDC hdc, HWND hwnd, POINT ptStart,
        BOOL fMustMove )
{
    if(_state == 0 )
        _state = OBSTYLE_SOLID_PEN;
    SetState( hdc, hwnd, _state | OBSTYLE_XOR | OBSTYLE_THICK);
    _pt = ptStart;
    return QueryMoveCursor(ptStart,fMustMove);
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::UpdateMove
//
//---------------------------------------------------------------
RECT&
OLEBorder::UpdateMove( HDC hdc, HWND hwnd, POINT ptCurrent, BOOL fNewRegion )
{
    if ((ptCurrent.x == _pt.x) && (ptCurrent.y == _pt.y))
        return rect;

    RECT rTemp = rect;
    Draw(hdc,hwnd);
    if (fNewRegion)
    {
        rTemp.left = min(_pt.x,ptCurrent.x);
        rTemp.top = min(_pt.y,ptCurrent.y);
        rTemp.right = max(_pt.x,ptCurrent.x);
        rTemp.bottom = max(_pt.y,ptCurrent.y);
    }
    else
    {
        int xDelta = ptCurrent.x - _pt.x;
        int yDelta = ptCurrent.y - _pt.y;
        switch (_usPart)
        {
        case BP_INSIDE:
        case BP_TOP:
        case BP_BOTTOM:
        case BP_RIGHT:
        case BP_LEFT:
        default:
            OffsetRect(&rTemp,xDelta,yDelta);
            break;

        case BP_TOPRIGHT:
            rTemp.right += xDelta;
            rTemp.top += yDelta;
            break;

        case BP_BOTTOMRIGHT:
            rTemp.right += xDelta;
            rTemp.bottom += yDelta;
            break;

        case BP_BOTTOMLEFT:
            rTemp.bottom += yDelta;
            rTemp.left += xDelta;
            break;

        case BP_TOPLEFT:
            rTemp.top += yDelta;
            rTemp.left += xDelta;
            break;

        case BP_TOPHAND:
            rTemp.top += yDelta;
            break;

        case BP_BOTTOMHAND:
            rTemp.bottom += yDelta;
            break;

        case BP_RIGHTHAND:
            rTemp.right += xDelta;
            break;

        case BP_LEFTHAND:
            rTemp.left += xDelta;
            break;
        }
    }
    //
    //clip resize to repect minimum height & width specification
    //
    if((rTemp.right - rTemp.left >= _sMinWidth) &&
        (rTemp.bottom - rTemp.top >= _sMinHeight))
    {
        rect = rTemp;
        if (!fNewRegion)
        {
            _pt = ptCurrent;
        }
    }

    Draw(hdc,hwnd);

    return rect;
}

//+---------------------------------------------------------------
//
//  Member: OLEBorder::EndMove
//
//---------------------------------------------------------------
RECT&
OLEBorder::EndMove( HDC hdc, HWND hwnd, POINT ptCurrent, USHORT usBorderState )
{
    SetState( hdc, hwnd, usBorderState );
    return rect;
}
