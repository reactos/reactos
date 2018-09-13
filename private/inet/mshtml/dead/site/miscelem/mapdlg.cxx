//=-----------------------------------------------------------------------=
//
// File:     mapdlg.cxx
//
// Contents: CMapEditDialog class
//
//=-----------------------------------------------------------------------=


#include "texthdrs.hxx"
#include "element.hxx"
#include "headers.hxx"
#include "download.hxx" // For CImgCtx
#define  WINCOMMCTRLAPI
#include "commctrl.h"   // For Toolbar schtuff
#include "collect.h"
#include "mapdlg.hxx"
#include "emap.hxx"
#include "imgelem.hxx"
#include "ebody.hxx"
#include "irange.hxx"


// BUGBUG(t-johnha): These are temporarily defined here and in 
// forms3r.rc.  They should be merged into siterc.h

#define IDR_MAPEDIT_CONTEXT_MENU 0x6046
#define IDR_MAPEDIT_DIALOG_MENU  0x6047
#define IDR_MAPEDIT_TOOLBAR      0x6048
#define IDB_MAPEDIT_TOOLBAR      0x6049

#define IDM_MAPEDIT_NEWRECT      9000
#define IDM_MAPEDIT_NEWCIRCLE    9001
#define IDM_MAPEDIT_NEWPOLY      9002
#define IDM_MAPEDIT_AUTOLINK     9003
#define IDM_MAPEDIT_PROPERTIES   9004
#define IDM_MAPEDIT_DELETEALL    9005
#define IDM_HELP_IMAGEMAP        9006
#define IDM_MAPEDIT_SELECT       9007

#define MAPEDIT_DIALOG_CAPTION _T("Editing Image Map - ")


HRESULT ShowPropertyDialog(int cUnk, 
                           IUnknown **apUnk,
                           HWND hwndOwner,
                           IServiceProvider *pServiceProvider,
                           LCID lcid);
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Initialization Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//=-----------------------------------------------------------------------=
//
// Function:    Init
//
// Synopsis:    Initializes the state of the dialog box, called before
//              the window is created
//
// Arguments:   IImgElemtn *pImg - Pointer to the Img element that called us.
//
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::Init(IImgElement *pImg)
{
    HRESULT hr;
    BSTR bstrUseMap = NULL, bstrSRC = NULL;
    CMapElement *pMap;
    IDispatch *pDisp = NULL;
    const TCHAR pszMapTextFmt[] = _T("<<MAP NAME=\"<0s>\"><</MAP>");
    SIZE sizeImg;

    // Set up our flags and pointers.
    _fCreating = 0;
    _fMoving = 0;
    _nDrawMode = SELECT_MODE;
    _nSizing = 0;
    _pareaCurrent = NULL;
    _fSelected = 0;
    _nIndex = -1;
    _pMap = NULL;
    _fAutoURL = 1;
    _fToolLock = 0;
    _fSBHorz = _fSBVert = 1;

    // Create our GDI objects
    _hpenXOR = (HPEN)CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
    _hbrHollow = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    _hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);

    // Get the image and relevant info
    pImg->get_src(&bstrSRC);

    GetImgCtx(_pDoc, bstrSRC, &_pImgCtx);
    if(!bstrSRC || !_pImgCtx)
    {
        hr = E_FAIL;
        _hr = E_FAIL;
        OnClose();
        goto Cleanup;
    }

    _pImgCtx->GetState(&sizeImg);
    _rcImg.left = _rcImg.top = 0;
    _rcImg.right = sizeImg.cx;
    _rcImg.bottom = sizeImg.cy;


    // And the map would be useful, too.
    hr = pImg->get_useMap(&bstrUseMap);
    if(hr)
        goto Cleanup;

    if(bstrUseMap && bstrUseMap[0] != '\0')
    {
        IGNORE_HR(_pDoc->EnsureMap(bstrUseMap, (CElement **)&pMap));
        if(pMap)
        {
            hr = THR(pMap->QueryInterface(IID_IMapElement, (void **)&_pMap));
        }
    }

    if(!_pMap)
    {
        TCHAR *pchMapText;
        BSTR bstrMapText;
        IBodyElement *pBody;
        ITxtRange    *pRange;

        if(!bstrUseMap)
        {
            bstrUseMap = SysAllocString(_T("#Foo"));
            pImg->put_useMap(bstrUseMap);
        }

        hr = THR(_pDoc->get_body(&pBody));
        if(hr)
            goto Cleanup;


        hr = THR(pBody->createTextRange(&pRange));
        if(hr)
            goto Cleanup;

        ClearInterface(&pBody);

        hr = THR(pRange->collapse(FALSE));
        if(hr)
            goto Cleanup;

        hr = THR(Format(FMT_OUT_ALLOC, 
                        &pchMapText, 
                        0, 
                        //_T("<<MAP NAME=\"<0s>\"><</MAP>"), 
                        (LPTSTR)pszMapTextFmt,
                        bstrUseMap + 1));
        if(hr)
            goto Cleanup;

        bstrMapText = SysAllocString(pchMapText);

        hr = THR(pRange->put_htmlText(bstrMapText));
        if(hr)
            goto Cleanup;

        ReleaseInterface(pRange);
        SysFreeString(bstrMapText);
        delete pchMapText;

        if(bstrUseMap && bstrUseMap[0] != '\0')
        {
            IGNORE_HR(_pDoc->EnsureMap(bstrUseMap, (CElement **)&pMap));
            if(pMap)
            {
                hr = THR(pMap->QueryInterface(IID_IMapElement, 
                                              (void **)&_pMap));
            }
            else
            {
                OnClose();
            }
        }

    }

Cleanup:
    ReleaseInterface(pDisp);
    SysFreeString(bstrUseMap);
    SysFreeString(bstrSRC);
    RRETURN(hr);

}


//=-----------------------------------------------------------------------=
//
// Function:    InitDialog
//
// Synopsis:    Does intialization that must be done after a window has
//              been created.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::InitDialog()
{
    static const TBBUTTON tbButton[] =
    {
        {0, IDM_MAPEDIT_SELECT, TBSTATE_CHECKED, TBSTYLE_CHECK, 0L, 0},
        {1, IDM_MAPEDIT_NEWRECT, TBSTATE_CHECKED, TBSTYLE_CHECK, 0L, 0},
        {2, IDM_MAPEDIT_NEWCIRCLE, TBSTATE_CHECKED, TBSTYLE_CHECK, 0L, 0},
        {3, IDM_MAPEDIT_NEWPOLY, TBSTATE_CHECKED, TBSTYLE_CHECK, 0L, 0},
        {10, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
        {4, IDM_PROPERTIES, TBSTATE_PRESSED, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
        {5, IDM_UNDO, TBSTATE_PRESSED, TBSTYLE_BUTTON, 0L, 0},
        {6, IDM_REDO, TBSTATE_PRESSED, TBSTYLE_BUTTON, 0L, 0},
    };
    
    _hwndToolbar = CreateToolbarEx(_hwndDialog,
                                   WS_CHILD | WS_VISIBLE | BS_NOTIFY,
                                   IDR_MAPEDIT_TOOLBAR,
                                   5,
                                   GetResourceHInst(),
                                   IDB_MAPEDIT_TOOLBAR,
                                   (LPCTBBUTTON) &tbButton,
                                   ARRAY_SIZE(tbButton),
                                   16,
                                   16,
                                   16,
                                   16,
                                   sizeof(TBBUTTON));

    GetWindowRect(_hwndToolbar, &_rcToolbar);
    UpdateToolbar();

    // Cache common system metrics
    _nYFrame = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + 1;
    _nYSpace = _nYFrame + GetSystemMetrics(SM_CYCAPTION) +
               GetSystemMetrics(SM_CYMENU) +
               _rcToolbar.bottom - _rcToolbar.top;
    _nXFrame = GetSystemMetrics(SM_CXSIZEFRAME) * 2 + 1;

    GetClientRect(_hwndDialog, &_rcClient);

    InitScrollBars();
    InitSize();
    SetCaption();

    // Get our menus
    _hmenuCtx = LoadMenu(GetResourceHInst(),
        MAKEINTRESOURCE(IDR_MAPEDIT_CONTEXT_MENU));
    _hmenuCtx = GetSubMenu(_hmenuCtx, 0);

    _hmenuBar = LoadMenu(GetResourceHInst(), 
                         MAKEINTRESOURCE(IDR_MAPEDIT_DIALOG_MENU));

    // Make a horizontal scrollbar
    _hwndHScroll = CreateWindowEx(0,
                                  _T("scrollbar"),
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | SBS_HORZ,
                                  _rcClient.left,
                                  _rcClient.bottom - 
                                    GetSystemMetrics(SM_CYHSCROLL) - 
                                  (_rcToolbar.bottom - _rcToolbar.top),
                                  _rcClient.right - _rcClient.left,
                                  GetSystemMetrics(SM_CYHSCROLL),
                                  _hwndDialog,
                                  NULL,
                                  GetResourceHInst(),
                                  NULL);
    // And set it up.
    _siHorz.nPos = 0;
    _siHorz.cbSize = sizeof(SCROLLINFO);
    _siHorz.fMask = SIF_POS;
    SetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz, TRUE);

    // Ditto the vertical scrollbar
    _hwndVScroll = CreateWindowEx(0,
                                  _T("scrollbar"),
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | SBS_VERT,
                                  _rcClient.right - 
                                    GetSystemMetrics(SM_CXVSCROLL),
                                  _rcClient.top + _rcToolbar.bottom - 
                                    _rcToolbar.top,
                                  GetSystemMetrics(SM_CXVSCROLL),
                                  _rcClient.bottom - 
                                    (_rcToolbar.bottom - _rcToolbar.top),
                                  _hwndDialog,
                                  NULL,
                                  GetResourceHInst(),
                                  NULL);
    // And ditto the setup
    _siVert.nPos = 0;
    _siVert.cbSize = sizeof(SCROLLINFO);
    _siVert.fMask = SIF_POS;
    SetScrollInfo(_hwndVScroll, SB_CTL, &_siVert, TRUE);

    GetViewRect(&_rcClient);

    // Size grip is the little sizing rectangle in the bottom right
    _hwndSizeGrip = CreateWindowEx(0,
                                   _T("scrollbar"),
                                   NULL,
                                   SBS_SIZEBOX | SBS_SIZEGRIP | 
                                    WS_VISIBLE | WS_CHILD,
                                   _rcClient.right,
                                   _rcClient.bottom,
                                   GetSystemMetrics(SM_CXVSCROLL),
                                   GetSystemMetrics(SM_CYHSCROLL),
                                   _hwndDialog,
                                   NULL,
                                   GetResourceHInst(),
                                   NULL);

    SetMenu(_hwndDialog, _hmenuBar);

    RRETURN(S_OK);
}



//=-----------------------------------------------------------------------=
//
// Function:    InitSize
//
// Synopsis:    Sets the initial size of the dialog
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::InitSize()
{
    long lx, ly;

    if(_rcImg.right > GetSystemMetrics(SM_CXSCREEN) * 3 / 4)
    {
        lx = GetSystemMetrics(SM_CXSCREEN) * 3 / 4;
    }
    else
    {
        lx = _rcImg.right + _nXFrame;
    }

    if(_rcImg.bottom + _nYSpace > GetSystemMetrics(SM_CYSCREEN) * 3 / 4)
    {
        ly = GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
    }
    else
    {
        ly = _rcImg.bottom + _nYSpace;
    }

    MoveWindow(_hwndDialog, 0, 0, lx, ly, TRUE);

    RRETURN(S_OK);
}
       

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Initialization Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Parsing/Unparsing Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//=-----------------------------------------------------------------------=
// Function:    Parse
//
// Synopsos:    Parses the 3 strings given to it to set the state of a
//              current area
//
// Arguments:   BSTR bstrCOORDS - COORDS attribute from the area
//              BSTR bstrSHAPE  - SHAPE  attribute from the area
//              BSTR bstrHREF   - HREF   attribute from the area
//
//=-----------------------------------------------------------------------=
HRESULT 
CMapEditDialog::Parse(BSTR bstrCOORDS, BSTR bstrSHAPE, BSTR bstrHREF)
{
    HRESULT hr = S_OK;
    TCHAR  *pch, achBuff[1024];
    POINT pt;

    Assert(bstrCOORDS);
    Assert(bstrSHAPE);


    if(bstrHREF)
    {
        _fhref = 1;
    }
    else
    {
        _fhref = 0;
    }

    // Parse out the Shape
    if(!_tcsicmp(bstrSHAPE, _T("CIRCLE")))
    {
        _nShapeType = SHAPE_TYPE_CIRCLE;
    }
    else if(!_tcsicmp(bstrSHAPE, _T("POLY")))
    {
        _nShapeType = SHAPE_TYPE_POLY;
    }
    else
    {
        _nShapeType = SHAPE_TYPE_RECT;
    }


    //
    // Parse out the coordinates
    //

    // Copy the buffer for tokenizing
    _tcscpy(achBuff, bstrCOORDS);

    //
    // Grab the first token.  If _tcstok returns NULL,
    // we want to keep processing, because this means they
    // gave us an empty coordinate string.  Right now,
    // missing values are set to 0.
    //

    pch = _tcstok(achBuff, DELIMS);

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        NextNum(&_coords.Rect.left, &pch);
        NextNum(&_coords.Rect.top, &pch);
        NextNum(&_coords.Rect.right, &pch);
        NextNum(&_coords.Rect.bottom, &pch);

        hr = THR(UpdateRectangle());
        break;

    case SHAPE_TYPE_CIRCLE:
        NextNum(&_coords.Circle.lx, &pch);
        NextNum(&_coords.Circle.ly, &pch);
        NextNum(&_coords.Circle.lradius, &pch);

        break;

    case SHAPE_TYPE_POLY:
        if(_ptList.Size())
        {
            _ptList.DeleteMultiple(0, _ptList.Size() - 1);
        }

        while(pch)
        {
            NextNum(&pt.x, &pch);
            NextNum(&pt.y, &pch);

            hr = THR(_ptList.AppendIndirect(&pt));
            if(hr)
                goto Cleanup;
        }

        // We don't store the same point as first and last
        // BUGBUG: (t-johnha) Do we want to preserve it if they
        // did?  Doubt it.
        if((_ptList[0].x == _ptList[_ptList.Size() - 1].x) &&
           (_ptList[0].y == _ptList[_ptList.Size() - 1].y))
        {
            _ptList.Delete(_ptList.Size() - 1);
        }

        hr = THR(UpdatePolygon());

        break;
    }

Cleanup:
    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    ParseArea
// 
// Synopsis:    Retrieves the necessary attributes from the area and
//              calls Parse to parse them into the current area.
//
// Arguments:   IAreaElement - Interface to the area to parse
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::ParseArea(IAreaElement *pArea)
{
    BSTR bstrCOORDS = NULL;
    BSTR bstrSHAPE = NULL;
    BSTR bstrHREF = NULL;
    HRESULT hr;

    hr = THR(pArea->get_coords(&bstrCOORDS));
    if(hr)
        goto Cleanup;

    hr = THR(pArea->get_shape(&bstrSHAPE));
    if(hr)
        goto Cleanup;

    hr = THR(pArea->get_href(&bstrHREF));
    if(hr)
        goto Cleanup;

    Parse(bstrCOORDS, bstrSHAPE, bstrHREF);

Cleanup:
    SysFreeString(bstrCOORDS);
    SysFreeString(bstrSHAPE);
    SysFreeString(bstrHREF);

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    UnParse
//
// Synopsis:    Builds attribute strings from the current area's state
//              and sets the "real" areas attributes to those values.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::UnParse()
{
    BSTR bstrTemp;
    HRESULT hr;
    TCHAR   achTemp[1024];
    int     c;
    int     nOffset;
    POINT  *ppt;


    //
    // First do the shape
    //

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        bstrTemp = SysAllocString(_T("RECT"));
        break;

    case SHAPE_TYPE_CIRCLE:
        bstrTemp = SysAllocString(_T("CIRCLE"));
        break;

    case SHAPE_TYPE_POLY:
        bstrTemp = SysAllocString(_T("POLY"));
        break;
    }

    hr = THR(_pareaCurrent->put_shape(bstrTemp));
    SysFreeString(bstrTemp);
    if(hr)
        goto Cleanup;


    //
    // Then do the coordinates
    //

    *achTemp = 0;
    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        hr = THR(Format(0, 
                        &achTemp, 
                        ARRAY_SIZE(achTemp), 
                        _T("<0d>,<1d>,<2d>,<3d>"),
                        _coords.Rect.left,  
                        _coords.Rect.top,
                        _coords.Rect.right, 
                        _coords.Rect.bottom));
        if(hr)
            goto Cleanup;
        break;

    case SHAPE_TYPE_CIRCLE:
        hr = THR(Format(0, 
                        &achTemp, 
                        ARRAY_SIZE(achTemp), 
                        _T("<0d>,<1d>,<2d>"),
                        _coords.Circle.lx, 
                        _coords.Circle.ly, 
                        _coords.Circle.lradius));
        if(hr)
            goto Cleanup;
        break;

    case SHAPE_TYPE_POLY:
        nOffset = 0;

        //
        // nOffset keeps track of the point in achTemp that we're
        // filling at.  
        // CONSIDER: If Format had some way of telling us how many
        // characters it filled in, we wouldn't have to _tcslen 
        // each time to get the new offset position.
        //

        for(c = _ptList.Size(), ppt = _ptList; c > 0; c--, ppt++)
        {
            if(nOffset)
            {
                achTemp[nOffset++] = _T(',');
            }
            hr = THR(Format(0,
                            &(achTemp[nOffset]),
                            ARRAY_SIZE(achTemp)-nOffset,
                            _T("<0d>,<1d>"),
                            ppt->x,
                            ppt->y));
            if(hr)
                goto Overflow;

            nOffset = _tcslen(achTemp);
        }
    }

Overflow:
    bstrTemp = SysAllocString(achTemp);
    _pareaCurrent->put_coords(bstrTemp);
    SysFreeString(bstrTemp);

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Parsing/Unparsing Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Containment/Border checking code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//=-----------------------------------------------------------------------=
//
// Function:    CheckSelect
//
// Synopsis:    Handles mouse click selecting.  First it unselects the
//              current area, if there is one, and then checks for a new
//              selection at the given point
//
// Arguments:   POINT pt - The point to check for selection
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::CheckSelect(POINT pt)
{
    HDC     hDC;
    HRESULT hr = S_OK;

    // If they clicked on a grab handle, then don't do
    // anything.
    if(_fSelected && PointOnBorder(pt))
        goto Cleanup;

    hDC = GetSetDC();

    // UnSelect the current area
    if(_fSelected && !PointOnBorder(pt))
    {
        IGNORE_HR(DrawGrabHandles(hDC));
    }

    _fSelected = 0;

    // Find out where they clicked.
    hr = GetAreaContaining(pt, &_pareaCurrent);
    if(_fSelected)
    {
        // And show selection feedback
        IGNORE_HR(DrawGrabHandles(hDC));
    }

    ReleaseDC(_hwndDialog, hDC);

Cleanup:
    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    GetAreaContaining
//
// Synopsis:    Fills *ppArea with the area that contains the given 
//              point.  If no area does, then it sets *ppArea to NULL.
//
// Arguments:   POINT pt - The point to be checked
//              IAreaElement **ppArea - Pointer to area interface
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::GetAreaContaining(POINT pt, IAreaElement **ppArea)
{
    HRESULT hr;
    IAreaElement *pArea = NULL;
    IDispatch *pDisp = NULL;
    IElementCollection *pCollection = NULL;

    LONG    lCount, lIndex;
    VARIANT vt1, vt2;

    Assert(ppArea);
    if(*ppArea)
    {
        ClearInterface(ppArea);
    }

    _fSelected = 0;

    // Get the area collection
    hr = THR(_pMap->get_areas(&pCollection));
    if(hr)
        goto Cleanup;

    // We cycle through by index, so get size of collection
    hr = THR(pCollection->get_count(&lCount));
    if(hr)
        goto Cleanup;

    V_VT(&vt1) = VT_I4;

    for(lIndex = 0; lIndex < lCount; lIndex++)
    {
        // Get the lindex-th item from the collection.
        V_I4(&vt1) = lIndex;
        hr = pCollection->item(vt1, vt2, &pDisp);
        if(hr)
            goto Cleanup;

        hr = THR(pDisp->QueryInterface(IID_IAreaElement, (void **)&pArea));
        if(hr)
            goto Cleanup;

        // Read in the area to check for containment
        hr = THR(ParseArea(pArea));
        if(hr)
            goto Cleanup;

        if(Contains(pt, _coords, _nShapeType))
        {
            pArea->AddRef();
            *ppArea = pArea;
            //(*ppArea)->AddRef();
            _fSelected = 1;
            _nIndex = lIndex;
            ParseArea(_pareaCurrent);
            UnParse();

            goto Cleanup;
        }

        // Cleanup for next loop pass.
        ClearInterface(&pArea);
        ClearInterface(&pDisp);
    }

Cleanup:
    ReleaseInterface(pArea);
    ReleaseInterface(pDisp);
    ReleaseInterface(pCollection);
    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    PointOnRect
//
// Synopsis:    Returns which border of the rectangle, if any, the given 
//              point is on.
//
// Arguments:   POINT pt - The point to be checked
//
//=-----------------------------------------------------------------------=
int
CMapEditDialog::PointOnRect(POINT pt)
{
    LONG lCenterX, lCenterY;
    lCenterX = (_coords.Rect.left + _coords.Rect.right) / 2;
    lCenterY = (_coords.Rect.top + _coords.Rect.bottom) / 2;

    // Left-hand side
    if((_coords.Rect.left - CORNER_SIZE <= pt.x) &&
       (_coords.Rect.left + CORNER_SIZE >= pt.x))
    {
        if((_coords.Rect.top - CORNER_SIZE <= pt.y) &&
           (_coords.Rect.top + CORNER_SIZE >= pt.y))
        {
            return 1;           // Top Left
        }
        if((_coords.Rect.bottom - CORNER_SIZE <= pt.y) &&
           (_coords.Rect.bottom + CORNER_SIZE >= pt.y))
        {
            return 7;           // Bottom Left
        }
        if((lCenterY - CORNER_SIZE <= pt.y) && (lCenterY + CORNER_SIZE >= pt.y))
        {
            return 8;           // Center Left
        }
    }

    // Right-hand side
    if((_coords.Rect.right - CORNER_SIZE <= pt.x) &&
       (_coords.Rect.right + CORNER_SIZE >= pt.x))
    {
        if((_coords.Rect.top - CORNER_SIZE <= pt.y) &&
           (_coords.Rect.top + CORNER_SIZE >= pt.y))
        {
            return 3;           // Top Right
        }
        if((_coords.Rect.bottom - CORNER_SIZE <= pt.y) &&
           (_coords.Rect.bottom + CORNER_SIZE >= pt.y))
        {
            return 5;           // Bottom Right
        }
        if((lCenterY - CORNER_SIZE <= pt.y) && 
           (lCenterY + CORNER_SIZE >= pt.y))
        {
            return 4;           // Center Right
        }
    }

    // Center
    if((lCenterX - CORNER_SIZE <= pt.x) && (lCenterX + CORNER_SIZE >= pt.x))
    {
        if((_coords.Rect.top - CORNER_SIZE <= pt.y) &&
           (_coords.Rect.top + CORNER_SIZE >= pt.y))
        {
            return 2;           // Top Center
        }
        if((_coords.Rect.bottom - CORNER_SIZE <= pt.y) &&
           (_coords.Rect.bottom + CORNER_SIZE >= pt.y))
        {
            return 6;           // Bottom Center
        }
    }

    return 0;                   // None
}


//=-----------------------------------------------------------------------=
// 
// Function:    PointOnCircle
//
// Synopsis:    Checks to see if the given point is on the circle's border
//
// Arguments:   POINT pt - The point to be checked.
//
//=-----------------------------------------------------------------------=
int
CMapEditDialog::PointOnCircle(POINT pt)
{
    
    // Check if they're centered vertically
    if((_coords.Circle.ly - CORNER_SIZE <= pt.y) &&
       (_coords.Circle.ly + CORNER_SIZE >= pt.y))
    {
        if((_coords.Circle.lx - _coords.Circle.lradius - CORNER_SIZE <= pt.x)
           &&
           (_coords.Circle.lx - _coords.Circle.lradius + CORNER_SIZE >= pt.x))
        {
            return 1;                   // Left Center
        }

        if((_coords.Circle.lx + _coords.Circle.lradius - CORNER_SIZE <= pt.x)
           &&
           (_coords.Circle.lx + _coords.Circle.lradius + CORNER_SIZE >= pt.x))
        {
            return 1;                   // Right center
        }
    }
    // If not, are they centered horizontally
    else if((_coords.Circle.lx - CORNER_SIZE <= pt.x) &&
            (_coords.Circle.lx + CORNER_SIZE >= pt.x))
    {
        if((_coords.Circle.ly - _coords.Circle.lradius - CORNER_SIZE <= pt.y)
           &&
           (_coords.Circle.ly - _coords.Circle.lradius + CORNER_SIZE >= pt.y))
        {
            return 1;                   // Center Top
        }

        if((_coords.Circle.ly + _coords.Circle.lradius - CORNER_SIZE <= pt.y)
           &&
           (_coords.Circle.ly + _coords.Circle.lradius + CORNER_SIZE >= pt.y))
        {
            return 1;                   // Center Bottom
        }
    }
    return 0;
}


//=-----------------------------------------------------------------------=
//
// Function:    PointOnPoly
//
// Synopsis:    Returns the vertex (if any) that the given point is on.
//
// Arguments:   POINT pt - The point to be checked
//
//=-----------------------------------------------------------------------=
int CMapEditDialog::PointOnPoly(POINT pt)
{
    POINT *ppt;
    int c;
    LONG lSize;

    lSize = _ptList.Size();

    for(c = 0, ppt=_ptList; c < lSize; c++, ppt++)
    {
        if((ppt->x - CORNER_SIZE <= pt.x) &&
           (ppt->x + CORNER_SIZE >= pt.x) &&
           (ppt->y - CORNER_SIZE <= pt.y) &&
           (ppt->y + CORNER_SIZE >= pt.y))
        {
            return c+1;
        }
    }
    return 0;
}


//=-----------------------------------------------------------------------=
//
// Function:    PointOnBorder
//
// Synopsis:    Checks to see if the given point is on a border
//
// Arguments:   POINT pt - the point to check
//
//=-----------------------------------------------------------------------=
int
CMapEditDialog::PointOnBorder(POINT pt)
{
    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        return PointOnRect(pt);
        break;

    case SHAPE_TYPE_CIRCLE:
        return PointOnCircle(pt);
        break;

    case SHAPE_TYPE_POLY:
        return PointOnPoly(pt);
        break;

    default:
        Assert(FALSE && "Invalid Shape");
        return 0;
    } 
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Containment/Border checking code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Misc. Window Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//=-----------------------------------------------------------------------=
//
// Function:    InitScrollBars
//
// Synsopsis:   Sets the scrollbar page and max size to the correct
//              values.
// 
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::InitScrollBars()
{
    // Set scroll info common to horiz and vert
    _siHorz.cbSize = sizeof(SCROLLINFO);
    _siVert.cbSize = sizeof(SCROLLINFO);

    // Only set page and range - leave pos as it is.
    _siHorz.fMask = SIF_PAGE | SIF_RANGE;
    _siVert.fMask = SIF_PAGE | SIF_RANGE;
    _siHorz.nMin = 0;
    _siVert.nMin = 0;

    // Set horizontal bar
    _siHorz.nMax = _rcImg.right;
    _siHorz.nPage = _rcClient.right - _rcClient.left;

    SetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz, TRUE);

    // Set vertical bar
    _siVert.nMax = _rcImg.bottom;
    _siVert.nPage = _rcClient.bottom - _rcClient.top;

    SetScrollInfo(_hwndVScroll, SB_CTL, &_siVert, TRUE);

    _siHorz.fMask = SIF_ALL;
    _siVert.fMask = SIF_ALL;

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
// 
// Function:    GetSetDC
//
// Synopsis:    Gets the DC for the dialog window and sets the clipping
//              region to the client area
//
//=-----------------------------------------------------------------------=
HDC
CMapEditDialog::GetSetDC()
{
HDC hDC;
HRGN hClip;

    hDC = GetDC(_hwndDialog);

    SetROP2(hDC, R2_XORPEN);
    SelectObject(hDC, _hpenXOR);

    hClip = CreateRectRgn(_rcClient.left, 
                          _rcClient.top, 
                          _rcClient.right, 
                          _rcClient.bottom);
    SelectClipRgn(hDC, hClip);
    DeleteObject(hClip);

    return hDC;
}


//=-----------------------------------------------------------------------=
// 
// Function:    SetCaption()
//
// Synopsis:    Sets the window's caption
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::SetCaption()
{
    HRESULT hr;
    BSTR bstrUseMap = NULL;
    TCHAR *pchCaption = NULL;

    hr = THR(_pMap->get_name(&bstrUseMap));
    if(hr || !bstrUseMap)
        goto Cleanup;

    hr = THR(Format(FMT_OUT_ALLOC, 
                    &pchCaption, 
                    0, 
                    _T("<0s><1s>"), 
                    MAPEDIT_DIALOG_CAPTION, 
                    bstrUseMap));
    if(hr)
        goto Cleanup;

    SetWindowText(_hwndDialog, pchCaption);

Cleanup:
    SysFreeString(bstrUseMap);
    delete pchCaption;

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    DrawGrabHandles
//
// Synopsis:    Draws grab handles on the object in the given DC and rect
//
// Arguments:   HDC hDC - The device context to draw into
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::DrawGrabHandles(HDC hDC)
{
    RECT rc = _rcClient;

    SelectObject(hDC, _hbrWhite);

    // Calculate scrollbar offset
    rc.left -= _siHorz.nPos;
    rc.right -= _siHorz.nPos;
    rc.top -= _siVert.nPos;
    rc.bottom -= _siVert.nPos;

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        // Top left
        Rectangle(hDC, _coords.Rect.left + rc.left - CORNER_SIZE,       
                       _coords.Rect.top + rc.top - CORNER_SIZE,
                       _coords.Rect.left + rc.left + CORNER_SIZE,
                       _coords.Rect.top + rc.top + CORNER_SIZE);
        // Top Right
        Rectangle(hDC, _coords.Rect.right + rc.left - CORNER_SIZE, 
                       _coords.Rect.top + rc.top - CORNER_SIZE,
                       _coords.Rect.right + rc.left + CORNER_SIZE,
                       _coords.Rect.top + rc.top + CORNER_SIZE);
        // Bottom Right
        Rectangle(hDC, _coords.Rect.right + rc.left - CORNER_SIZE, 
                       _coords.Rect.bottom + rc.top - CORNER_SIZE,
                       _coords.Rect.right + rc.left + CORNER_SIZE,
                       _coords.Rect.bottom + rc.top + CORNER_SIZE);
        // Bottom left
        Rectangle(hDC, _coords.Rect.left + rc.left - CORNER_SIZE, 
                       _coords.Rect.bottom + rc.top - CORNER_SIZE,
                       _coords.Rect.left + rc.left + CORNER_SIZE,
                       _coords.Rect.bottom + rc.top + CORNER_SIZE);
        // Top Center
        Rectangle(hDC, (_coords.Rect.left + _coords.Rect.right)
                            / 2 + rc.left - CORNER_SIZE,
                        _coords.Rect.top + rc.top - CORNER_SIZE,
                       (_coords.Rect.left + _coords.Rect.right)
                            / 2 + rc.left + CORNER_SIZE,
                        _coords.Rect.top + rc.top + CORNER_SIZE);
        // Bottom Center
        Rectangle(hDC, (_coords.Rect.left + _coords.Rect.right)
                            / 2 + rc.left - CORNER_SIZE,
                        _coords.Rect.bottom + rc.top - CORNER_SIZE,
                       (_coords.Rect.left + _coords.Rect.right)
                            / 2 + rc.left + CORNER_SIZE,
                        _coords.Rect.bottom + rc.top + CORNER_SIZE);
        // Left Center
        Rectangle(hDC,  _coords.Rect.left + rc.left - CORNER_SIZE,
                       (_coords.Rect.top + _coords.Rect.bottom)
                            / 2 + rc.top - CORNER_SIZE,
                        _coords.Rect.left + rc.left + CORNER_SIZE,
                       (_coords.Rect.top + _coords.Rect.bottom)
                            / 2 + rc.top + CORNER_SIZE);
        // Right Center
        Rectangle(hDC,  _coords.Rect.right + rc.left - CORNER_SIZE,
                       (_coords.Rect.top + _coords.Rect.bottom)
                            / 2 + rc.top - CORNER_SIZE,
                        _coords.Rect.right + rc.left + CORNER_SIZE,
                       (_coords.Rect.top + _coords.Rect.bottom)
                            / 2 + rc.top + CORNER_SIZE);
        break;

    case SHAPE_TYPE_CIRCLE:
        // Left center
        Rectangle(hDC, 
            _coords.Circle.lx - _coords.Circle.lradius + rc.left - CORNER_SIZE,
            _coords.Circle.ly + rc.top - CORNER_SIZE,
            _coords.Circle.lx - _coords.Circle.lradius + rc.left + CORNER_SIZE,
            _coords.Circle.ly + rc.top + CORNER_SIZE);
        // Center top
        Rectangle(hDC,
            _coords.Circle.lx + rc.left - CORNER_SIZE,
            _coords.Circle.ly - _coords.Circle.lradius + rc.top - CORNER_SIZE,
            _coords.Circle.lx + rc.left + CORNER_SIZE,
            _coords.Circle.ly - _coords.Circle.lradius + rc.top + CORNER_SIZE);
        // Right Center
        Rectangle(hDC, 
            _coords.Circle.lx + _coords.Circle.lradius + rc.left - CORNER_SIZE,
            _coords.Circle.ly + rc.top - CORNER_SIZE,
            _coords.Circle.lx + _coords.Circle.lradius + rc.left + CORNER_SIZE,
            _coords.Circle.ly + rc.top + CORNER_SIZE);
        // Center Bottom
        Rectangle(hDC,
            _coords.Circle.lx + rc.left - CORNER_SIZE,
            _coords.Circle.ly + _coords.Circle.lradius + rc.top - CORNER_SIZE,
            _coords.Circle.lx + rc.left + CORNER_SIZE,
            _coords.Circle.ly + _coords.Circle.lradius + rc.top + CORNER_SIZE);
        break;

    case SHAPE_TYPE_POLY:
        LONG c;
        POINT *ppt;

        for(c = _ptList.Size(), ppt = _ptList; c > 0; ppt++, c--)
        {
            Rectangle(hDC, 
                ppt->x + rc.left - CORNER_SIZE,
                ppt->y + rc.top - CORNER_SIZE,
                ppt->x + rc.left + CORNER_SIZE,
                ppt->y + rc.top + CORNER_SIZE);
        }
        break;

    default:
        Assert(FALSE && "Invalid Shape");
    }

    SelectObject(hDC, _hbrHollow);

    RRETURN(S_OK);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Misc. Window Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT
CMapEditDialog::DrawCurrent(HDC hDC, RECT rc)
{

    // Offset for scrolling.
    rc.left -= _siHorz.nPos; 
    rc.right -= _siHorz.nPos;
    rc.top -= _siVert.nPos;
    rc.bottom -= _siVert.nPos;

    if(_fhref)
    {
        SelectObject(hDC, _hbrHollow);
    }
    else
    {
        SelectObject(hDC, _hbrWhite);
    }

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        Rectangle(hDC, 
            _coords.Rect.left + rc.left,
            _coords.Rect.top + rc.top, 
            _coords.Rect.right + rc.left, 
            _coords.Rect.bottom + rc.top);

        break;

    case SHAPE_TYPE_CIRCLE:
        Ellipse(hDC, _coords.Circle.lx - _coords.Circle.lradius + rc.left,
                     _coords.Circle.ly - _coords.Circle.lradius + rc.top,
                     _coords.Circle.lx + _coords.Circle.lradius + rc.left,
                     _coords.Circle.ly + _coords.Circle.lradius + rc.top);

        break;

    case SHAPE_TYPE_POLY:
        POINT *ppt;
        UINT c;
    
        if(_fhref)
        {
            MoveToEx(hDC, _ptList[0].x + rc.left, _ptList[0].y + rc.top, NULL);
            for(c = _ptList.Size(), ppt = &(_ptList[1]);
                c > 1;          // c > 1, because we MoveTo'd the first pt
                ppt++, c--)
            {
                LineTo(hDC, ppt->x + rc.left, ppt->y + rc.top);
            }

            //
            // If there are only 2 points in the polygon, we don't want to draw
            // the same line twice and end up with nothing!  However, we have 
            // to do one draw, because otherwise the first move will cause the 
            // very first line segment to be inverted.
            //

            if(_ptList.Size() != 2 ||
               _ptList[0].x != _ptList[1].x ||
               _ptList[0].y != _ptList[1].y)
            {
                LineTo(hDC, _ptList[0].x + rc.left, _ptList[0].y + rc.top);
            }
        }
        else
        {
            //
            // A region of 2 points does not get painted by PaintRgn,
            // so we do it manually.
            //

            if(_ptList.Size() == 2 &&
               !(_ptList[0].x == _ptList[1].x && 
                 _ptList[0].y == _ptList[1].y))
            {
                // Cookie usage here will prevent "spots" from appearing.
                // Areas are erased before a size operation and redrawn
                // afterwards.  The problem is that with a 3 pixel wide
                // line, the inital erasure of the null polygon actually
                // XORS a couple pixels, which get left floating as garbage.
                MoveToEx(hDC, 
                         _ptList[_ptList.Size() - 1].x + rc.left, 
                         _ptList[_ptList.Size() - 1].y + rc.top, NULL);
                LineTo(hDC, _ptList[0].x + rc.left, _ptList[0].y + rc.top);
            }

            //
            // We have to offset the region to account for scrolling
            // and for the toolbar
            //

            OffsetRgn(_coords.Polygon.hPoly, 
                      _rcClient.left - _siHorz.nPos, 
                      _rcClient.top - _siVert.nPos);
            PaintRgn(hDC, _coords.Polygon.hPoly);
            OffsetRgn(_coords.Polygon.hPoly,
                      _siHorz.nPos - _rcClient.left,
                      _siVert.nPos - _rcClient.top);
        }

        break;

    default:
        Assert(FALSE && "Invalid Shape");

        break;
    }

    SelectObject(hDC, _hbrHollow);

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    UpdateToolbar
//
// Synopsis:    Updates the state of the toolbar buttons
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::UpdateToolbar()
{
    static const UINT TBBtnInfo[] = {
        IDM_MAPEDIT_SELECT,
        IDM_MAPEDIT_NEWRECT,
        IDM_MAPEDIT_NEWCIRCLE,
        IDM_MAPEDIT_NEWPOLY,
        IDM_PROPERTIES,
        IDM_UNDO,
        IDM_REDO,
        0
    };
    int     cButton;
    HRESULT hr = S_OK;
    UINT uButton;

    for(cButton = 0; TBBtnInfo[cButton]; cButton++)
    {
        SendMessage(_hwndToolbar,
                    TB_SETSTATE,
                    (WPARAM) TBBtnInfo[cButton],
                    FALSE);
        if(TBBtnInfo[cButton] != IDM_PROPERTIES || _fSelected)
        {
            SendMessage(_hwndToolbar,
                        TB_ENABLEBUTTON,
                        (WPARAM) TBBtnInfo[cButton],
                        TRUE);
        }
    }

    switch(_nDrawMode)
    {
    case SELECT_MODE:
        uButton = IDM_MAPEDIT_SELECT;
        break;

    case SHAPE_TYPE_RECT:
    case SHAPE_TYPE_CIRCLE:
    case SHAPE_TYPE_POLY:
        uButton = _nDrawMode - SHAPE_TYPE_RECT + IDM_MAPEDIT_NEWRECT;
        break;
    }

    SendMessage(_hwndToolbar,
                TB_SETSTATE,
                (WPARAM) uButton,
                TRUE);

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    GetViewRect
//
// Synopsis:    Gets the usable view rectangle, accounting for the
//              scrollbar and toolbar child windows
//
// Arguments:   RECT *prc - filled with view rect
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::GetViewRect(RECT *prc)
{
    GetClientRect(_hwndDialog, prc);

    if(_hwndToolbar)
    {
        prc->top += _rcToolbar.bottom - _rcToolbar.top;
    }

    if(_hwndHScroll && _fSBHorz)
    {
        prc->bottom -= GetSystemMetrics(SM_CYHSCROLL);
    }

    if(_hwndVScroll && _fSBVert)
    {
        prc->right -= GetSystemMetrics(SM_CXVSCROLL);
    }

    RRETURN(S_OK);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Misc. Editing Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//=-----------------------------------------------------------------------=
// 
// Function:    StartSize
// 
// Synopsis:    Sets the initial size of the current area based upon
//              the point of creation.  Returns the border to size.
//
// Arguments:   LONG lx, ly - the point of creation
//    
//=-----------------------------------------------------------------------=
int
CMapEditDialog::StartSize(LONG lx, LONG ly)
{

    lx += _siHorz.nPos;
    ly += _siVert.nPos;

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        _coords.Rect.left = lx;
        _coords.Rect.top = ly;
        _coords.Rect.right = lx;
        _coords.Rect.bottom = ly; 

        return 5;                       // Size bottom right corner
        break;

    case SHAPE_TYPE_CIRCLE:
        _coords.Circle.lx = lx;
        _coords.Circle.ly = ly;
        _coords.Circle.lradius = 0;

        return 1;                       // Circles only have 1 border
        break;

    case SHAPE_TYPE_POLY:
        POINT pt;

        //
        // When we create a polygon, we need to create the initial point,
        // as well as the "virtual vertex" that we will size around.
        //

        pt.x = lx;
        pt.y = ly;
        _fCookie = 0;

        if(_ptList.Size())
        {
            _ptList.DeleteMultiple(0, _ptList.Size() - 1);
        }

        _ptList.AppendIndirect(&pt);    // First point
        _ptList.AppendIndirect(&pt);    // Virtual vertex

        return 2;                       // Start sizing the virtual vertex
        break;

    default:
        Assert(FALSE && "Invalid Shape");

        return 0;
        break;
    }

}

//=-----------------------------------------------------------------------=
// 
// Function:    SizeTo
//
// Synopsis:    Sizes the given border of the current area to the
//              given point.
//
// Arguments:   int nBorder - the border to size
//              LONG lx, ly - the point to size the border to
//              BOOL fShift - TRUE if the user is holding shift down
//                  while creating the area.  False otherwise.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::SizeTo(int nBorder, LONG lx, LONG ly, BOOL fShift)
{
    switch(_nShapeType)
    {
    // Rectangle Sizing
    case SHAPE_TYPE_RECT:
        switch(nBorder)
        {
        case 1:                         // Top Left
            _coords.Rect.left = lx;
            _coords.Rect.top = ly;
            break;

        case 2:                         // Top Center
            _coords.Rect.top = ly;
            break;

        case 3:                         // Top Right
            _coords.Rect.right = lx;
            _coords.Rect.top = ly;
            break;

        case 4:                         // Center Right
            _coords.Rect.right = lx;
            break;

        case 5:                         // Bottom Right
            _coords.Rect.right = lx;
            _coords.Rect.bottom = ly;

            //
            // When we're creating, this is the border we size.  
            // If shift is down, we want to constrain to a square,
            // 

            if(fShift)
            {
                if(abs(_coords.Rect.right - _coords.Rect.left) >
                   abs(_coords.Rect.bottom - _coords.Rect.top))
                {
                    if(_coords.Rect.bottom > _coords.Rect.top)
                    {
                        _coords.Rect.bottom = _coords.Rect.top + 
                            abs(_coords.Rect.right - _coords.Rect.left);
                    }
                    else
                    {
                        _coords.Rect.bottom = _coords.Rect.top -
                            abs(_coords.Rect.right - _coords.Rect.left);
                    }
                }
                else
                {
                    if(_coords.Rect.right > _coords.Rect.left)
                    {
                        _coords.Rect.right = _coords.Rect.left +
                            abs(_coords.Rect.bottom - _coords.Rect.top);
                    }
                    else
                    {
                        _coords.Rect.right = _coords.Rect.left -
                            abs(_coords.Rect.bottom - _coords.Rect.top);
                    }
                }
            }

            break;

        case 6:                         // Bottom Center
            _coords.Rect.bottom = ly;
            break;

        case 7:                         // Bottom Left
            _coords.Rect.left = lx;
            _coords.Rect.bottom = ly;
            break;

        case 8:                         // Center Left
            _coords.Rect.left = lx;
            break;

        default:
            Assert(FALSE && "Invalid Border");
        }
        break;

    // Circle Sizing
    case SHAPE_TYPE_CIRCLE:
        if(abs(_coords.Circle.lx - lx) > abs(_coords.Circle.ly - ly))
        {
            _coords.Circle.lradius = abs(_coords.Circle.lx - lx);
        }
        else
        {
            _coords.Circle.lradius = abs(_coords.Circle.ly - ly);
        }

    break;

    // Polygon Sizing
    case SHAPE_TYPE_POLY:
        POINT *ppt;

        ppt = &(_ptList[nBorder - 1]);
        ppt->x = lx;
        ppt->y = ly;

        break;

    
    default:
        Assert(FALSE && "Invalid Shape");
        break;
    }

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    MoveTo
//
// Synopsis:    Moves the shape by the specified x and y offsets
//
// Arguments:   LONG lXOff, lYOff - the x and y offsets
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::MoveTo(LONG lXOff, LONG lYOff)
{
    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        _coords.Rect.left += lXOff;
        _coords.Rect.top += lYOff;
        _coords.Rect.right += lXOff;
        _coords.Rect.bottom += lYOff;
        break;

    case SHAPE_TYPE_CIRCLE:
        _coords.Circle.lx += lXOff;
        _coords.Circle.ly += lYOff;
        break;

    case SHAPE_TYPE_POLY:
        POINT *ppt;
        int c=0;

        for(c = _ptList.Size(), ppt=_ptList; c > 0; c--, ppt++)
        {
            ppt->x += lXOff;
            ppt->y += lYOff;
        }

        break;
    }

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    UpdateRectangle
//
// Synopsis:    Updates the rectangle coordinates.
//
// Notes:       Should only be called if the region is actually a 
//              SHAPE_TYPE_RECT.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::UpdateRectangle()
{
    LONG ltemp;

    Assert(_nShapeType == SHAPE_TYPE_RECT);

    if(_coords.Rect.left > _coords.Rect.right)
    {
        ltemp = _coords.Rect.left;
        _coords.Rect.left = _coords.Rect.right;
        _coords.Rect.right = ltemp;
    }

    if(_coords.Rect.top > _coords.Rect.bottom)
    {
        ltemp = _coords.Rect.top;
        _coords.Rect.top = _coords.Rect.bottom;
        _coords.Rect.bottom = ltemp;
    }

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    UpdatePolygon
//
// Synopsis:    Updates the internal polygon region.
//
// Notes:       Should only be called if the region is actually a 
//              SHAPE_TYPE_POLY.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::UpdatePolygon()
{
    HRGN hNew;
    HRESULT hr;

    Assert(_nShapeType == SHAPE_TYPE_POLY);

    hNew = CreatePolygonRgn(_ptList, _ptList.Size(), ALTERNATE);

    if(hNew == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        DeleteObject(_coords.Polygon.hPoly);
        _coords.Polygon.hPoly = hNew;

        hr = S_OK;
    }

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    GetCursorClipRect
//
// Synopsis:    Gets the cursor clipping rectangle.
// 
// Arguments:   RECT *prcClip - To return the cliprect in.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::GetCursorClipRect(RECT *prcClip)
{
    POINT pt = {0,0};

    Assert(prcClip);

    // Calculate Client area to screen offset.
    ClientToScreen(_hwndDialog, &pt);

    *prcClip = _rcClient;
    
    prcClip->left   += pt.x;
    prcClip->top    += pt.y;
    prcClip->right  += pt.x;
    prcClip->bottom += pt.y;

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    AddCurrent
//
// Synopsis:    Adds the current area to the map
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::AddCurrent()
{
    IElementCollection *pCollection = NULL;
    HRESULT hr;
    VARIANT vt1, vt2;

    hr = THR(_pMap->get_areas(&pCollection));
    if(hr)
        goto Cleanup;

    V_VT(&vt2) = VT_I4;
    V_I4(&vt2) = 0;
    VariantInit(&vt1);

    hr = THR(pCollection->add(_pareaCurrent, vt1, vt2));


Cleanup:
    ReleaseInterface(pCollection);

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Misc. Editing Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Message Handlers
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
   
//=-----------------------------------------------------------------------=
//
// Function:    OnClose
//
// Synopsis:    Performs appropriate Cleanup and closes the dialog
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnClose()
{
    if(_pImgCtx)
    {
        _pImgCtx->Release();
        _pImgCtx = NULL;
    }

    DestroyMenu(_hmenuCtx);
    DestroyMenu(_hmenuBar);

    DestroyWindow(_hwndHScroll);
    DestroyWindow(_hwndVScroll);
    DestroyWindow(_hwndToolbar);

    DeleteObject(_hpenXOR);
    DeleteObject(_hbrHollow);
    DeleteObject(_hbrWhite);

    ClearInterface(&_pareaCurrent);
    ClearInterface(&_pMap);

    if(_hwndDialog)
    {
        EndDialog(_hwndDialog, 0);
        _hwndDialog = 0;
    }

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnCommand
//
// Synopsis:    Handles WM_COMMAND messages
//
// Arguments:   WORD wNotifyCode
//              WORD idiCtrl - command id
//              HWND hwndCtrl
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnCommand(WORD wNotifyCode, WORD idiCtrl, HWND hwndCtrl)
{
    HRESULT hr = S_OK;

    switch(idiCtrl)
    {
    case IDM_MAPEDIT_SELECT:
        _nDrawMode = SELECT_MODE;
        break;

    case IDM_MAPEDIT_NEWRECT:
        _nDrawMode = SHAPE_TYPE_RECT;
        break;

    case IDM_MAPEDIT_NEWCIRCLE:
        _nDrawMode = SHAPE_TYPE_CIRCLE;
        break;

    case IDM_MAPEDIT_NEWPOLY:
        _nDrawMode = SHAPE_TYPE_POLY;
        break;

    case IDM_DELETE:
        hr = THR(OnDelete());
        break;

    case IDM_MAPEDIT_DELETEALL:
        hr = THR(OnDeleteAll());
        break;

    case IDM_PROPERTIES:
    case IDM_MAPEDIT_PROPERTIES:
        hr = THR(OnShowProperties());
        break;

        // BUGBUG(t-johnha): Who's doing help?
    case IDM_HELP_IMAGEMAP:
        MessageBox(NULL, 
                   _T("Help Me!"), 
                   _T("Hit OK to destroy the world."), 
                   MB_OK);
        break;

    case IDM_MAPEDIT_AUTOLINK:
        _fAutoURL ^= 1;
        break;

        // BUGBUG(t-johnha): IDCANCEL is going to have to undo all changes from
        // this dialog session
    case IDCANCEL:
        _hr = S_FALSE;
        hr = THR(OnClose());
        break;

    case IDOK:
        hr = THR(OnClose());
        break;
    }

    UpdateToolbar();
    RRETURN1(hr, S_FALSE);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnContextMenu
//
// Synopsis:    Handles the WM_CONTEXTMENU message to pop up the C.M.
//
// Arguments:   CMessage *pMSg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnContextMenu(CMessage *pmsg)
{
    if(!_fCreating)
    {
        TrackPopupMenu(_hmenuCtx, 
                       TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                       pmsg->pt.x, 
                       pmsg->pt.y, 
                       0, 
                       _hwndDialog, 
                       NULL);
    }

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnDelete
//
// Synopsis:    Deletes the current area from the map
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnDelete()
{
    HDC  hDC;
    HRESULT hr = S_OK;
    IElementCollection *pCollection = NULL;

    Assert(_pMap);
    
    if(_fSelected)
    {
        hDC = GetSetDC();

        // Erase the area
        hr = THR(DrawCurrent(hDC, _rcClient));
        IGNORE_HR(DrawGrabHandles(hDC));

        // Remove it from the map and collection
        hr = THR(_pMap->get_areas(&pCollection));
        if(hr)
            goto Cleanup;

        pCollection->remove(_nIndex);

        // And set our state
        ClearInterface(&_pareaCurrent);

        _fSelected = 0;
        _nIndex = -1;

        ReleaseDC(_hwndDialog, hDC);
    }

Cleanup:
    ReleaseInterface(pCollection);
    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnDeleteAll
//
// Synopsis:    Deletes all areas from the map
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnDeleteAll()
{
    HRESULT       hr = S_OK;
    long          lcAreas;
    IElementCollection *pCollection = NULL;

    Assert(_pMap);

    hr = THR(_pMap->get_areas(&pCollection));
    if(hr)
        goto Cleanup;

    hr = THR(pCollection->get_count(&lcAreas));
    if(hr)
        goto Cleanup;

    for(; lcAreas > 0; lcAreas--)
    {

        pCollection->remove(lcAreas - 1);

    }

    ClearInterface(&_pareaCurrent);
    _fSelected = 0;
    _nIndex = -1;

    InvalidateRect(_hwndDialog, NULL, FALSE);
        

Cleanup:
    ReleaseInterface(pCollection);
    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnInitMenuPopup
//
// Synopsis:    Enables/Disables menu options as appropriate
//
// Arguments:   CMessage *pmsg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnInitMenuPopup(CMessage *pmsg)
{
    MENUITEMINFO mii;

    EnableMenuItem((HMENU)pmsg->wParam, 
                   IDM_PROPERTIES, 
                   _fSelected ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem((HMENU)pmsg->wParam, 
                   IDM_DELETE, 
                   _fSelected ? MF_ENABLED : MF_GRAYED);

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fState = _fAutoURL ? MFS_CHECKED : MFS_UNCHECKED;
    mii.wID = IDM_MAPEDIT_AUTOLINK;
    
    SetMenuItemInfo((HMENU)pmsg->wParam, IDM_MAPEDIT_AUTOLINK, FALSE, &mii);
 
    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
// 
// Function:    OnLButtonDblClk
//
// Synopsis:    Handles the WM_LBUTTONDBLCLK message - Ends a polygon
//                  if in creation mode, and tosses up properties if
//                  user double clicks inside an already created area.
//
// Arguments:   CMessage *pmsg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnLButtonDblClk(CMessage *pmsg)
{
    HDC hDC;
    HRESULT hr;


    Assert(_pMap);

    //
    // A double click terminates the definition of the polygon.
    // Because the first click will generate WM_Release and
    // UP messages, we need to remove the last point that was
    // added in.
    //

    if(_nShapeType == SHAPE_TYPE_POLY && _fCreating)
    {
        hDC = GetSetDC();

        // Erase the area
        hr = THR(DrawCurrent(hDC, _rcClient));
        IGNORE_HR(DrawGrabHandles(hDC));

        // Fix it up
        _ptList.Delete(_ptList.Size() - 1);
        --_nSizing;

        // Redraw it
        hr = THR(DrawCurrent(hDC, _rcClient));
        IGNORE_HR(DrawGrabHandles(hDC));

        ReleaseDC(pmsg->hwnd, hDC);

        // Don't allow the user to be stupid and make a 
        // polygon with only 2 points, because they won't
        // ever be able to select it.
        if(_ptList.Size() > 2)
        {
            _fCreating = FALSE;
            _nSizing = 0;

            AddCurrent();
            _nIndex = 0;
            _nDrawMode = SELECT_MODE;  // Turn off draw mode

            hr = THR(UpdatePolygon());

            ReleaseCapture();
            ClipCursor(NULL);
            
            hr = THR(UnParse());
            // BUGBUG (t-johnha): Check tool lock before we do this
            if(_fAutoURL)
            {
                OnShowProperties();
            }
        }
    }
    else
    {
        // BUGBUG (t-johnha): Is this what we want it to do on 
        // a double click?
        hr = THR(OnShowProperties());
        if(hr == S_FALSE)
        {
            // S_FALSE from ShowProps is Ok.
            hr = S_OK;
        }
    }

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnLButtonDown
//
// Synopsis:    Handles the WM_LBUTTONDOWN message, which starts the
//              creation of a new area, or the movement/sizing of an
//              existing area.
//
// Arguments:   CMessage *pmsg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnLButtonDown(CMessage *pmsg)
{
    HRESULT hr = S_OK;
    HDC     hDC;
    POINT   pt;
    RECT    rcClip;


    Assert(_pMap);
    
    SetCapture(pmsg->hwnd);

    // Offset for scrolling
    pt = pmsg->pt;
    pt.x = pt.x + _siHorz.nPos - _rcClient.left;
    pt.y = pt.y + _siVert.nPos - _rcClient.top;


    // We only want to check selection in select mode
    if(_nDrawMode == SELECT_MODE)
    {
        hr = THR(CheckSelect(pt));
    }
    else if(!_fCreating)
    {
        //
        // If they're starting a new shape,
        // we have to unselect the old shape, if applicable.
        //

        if(_fSelected && _nSizing == 0)
        {   
            hDC = GetSetDC();

            IGNORE_HR(DrawGrabHandles(hDC));

            ClearInterface(&_pareaCurrent);

            _fSelected = 0;
            _nIndex = -1;

            ReleaseDC(_hwndDialog, hDC);
        }
    }

    // Check to see if we have a shape selected
    if(_fSelected)
    {
    //////////////////////////////////////////////////////////////
    //                  Actions on  shapes                      //
    //////////////////////////////////////////////////////////////

        _nSizing = PointOnBorder(pt);

        //
        // If _nSizing is not zero, then they're resizing an
        // area, and we don't need to do anything else.  If 
        // it is zero, then they're moving the region, and we
        // need to set that up:
        //

        if(_nSizing == 0)
        {
            _ptMovePoint = pt;
            _fMoving = TRUE;
        }

        hr = THR(GetCursorClipRect(&rcClip));
        ClipCursor(&rcClip);
    }
    else
    {
    //////////////////////////////////////////////////////////////
    //                  New Shape Creation                      //
    //////////////////////////////////////////////////////////////
        if(!_fSelected && !_fCreating && _nDrawMode != SELECT_MODE)
        {
            _fCreating = TRUE;
            _ptMovePoint = pt;

            hr = THR(GetCursorClipRect(&rcClip));
            ClipCursor(&rcClip);

            ::CreateElement(ETAG_AREA, (CElement **)(&_pareaElement));

            hr = THR(_pareaElement->QueryInterface(IID_IAreaElement, 
                                                   (void **)&_pareaCurrent));
            if(hr == S_OK)
            {
                hr = THR(ParseArea(_pareaCurrent));
            }

            _nShapeType = _nDrawMode;

            // For a polygon, StartSize is going to create the first
            // vertex, as well as the first "virtual vertex" that will
            // follow the mouse cursor around.  The MouseCapture should
            // be retained during all of creation, and the shape should
            // be redrawn on every MouseMove, regardless of button 
            // status.
            _nSizing = StartSize(pt.x, pt.y);
            _fSelected = 1;
        }
        // Selection Mode
        else if(_nDrawMode == SELECT_MODE)
        {
            SetCapture(NULL);
            ClipCursor(NULL);
        }
    }

    UpdateToolbar();

    RRETURN(hr);
}

//=-----------------------------------------------------------------------=
//
// Function:    OnLButtonUp
//
// Synopsis:    Processes the WM_LBUTTONUP message, which ends a 
//              creation/size/move operation
//
// Arugments:   CMessage *pmsg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnLButtonUp(CMessage *pmsg)
{
    HRESULT hr = S_OK;
    POINT pt = pmsg->pt;
    HDC hDC;

    pt.x = pt.x + _siHorz.nPos - _rcClient.left;
    pt.y = pt.y + _siVert.nPos - _rcClient.top;

    if(_fCreating)                      // Defining a new area
    {
        // 
        // For a polygon, check for a single-click to start, and
        // if it wasn't, then add the point in.
        //

        if(_nShapeType == SHAPE_TYPE_POLY)
        {
            //
            // If this point is the same as where the polygon was
            // started, and they haven't defined any other points,
            // then we don't want to add this in.
            //

            if((pt.x != _ptMovePoint.x || pt.y != _ptMovePoint.y) ||
               (_ptList.Size() != 2))
            {
                //
                // Get and set up our DC
                //

                hDC = GetSetDC();

                // Erase the area
                hr = THR(DrawCurrent(hDC, _rcClient));
                IGNORE_HR(DrawGrabHandles(hDC));

                // Modify it
                hr = THR(_ptList.AppendIndirect(&pt));

                // Redraw it
                hr = THR(DrawCurrent(hDC, _rcClient));
                IGNORE_HR(DrawGrabHandles(hDC));

                // Release the DC
                ReleaseDC(pmsg->hwnd, hDC);

                ++_nSizing;
            }
        }

        // For a non-polygon, this means we're done.
        else if(_nShapeType != SHAPE_TYPE_POLY)
        {
            _fCreating = FALSE;
            _nSizing = 0;

            // Make sure rectangle coordinates are valid
            if(_nShapeType == SHAPE_TYPE_RECT)
            {
                hr = THR(UpdateRectangle());
            }

            AddCurrent();

            _nIndex = 0;
            _nDrawMode = SELECT_MODE; // Turn off draw mode.

            ReleaseCapture();
            ClipCursor(NULL);
            
            hr = THR(UnParse());
            // BUGBUG(t-johnha): Check tool lock before we do this.
            if(_fAutoURL)
            {
                OnShowProperties();
            }
        }
    }

    else if(_nSizing)                   // Resizing an area
    {
        _nSizing = 0;

        if(_nShapeType == SHAPE_TYPE_POLY)
        {
            hr = THR(UpdatePolygon());
        }
        else if(_nShapeType == SHAPE_TYPE_RECT)
        {
            hr = THR(UpdateRectangle());
        }

        ReleaseCapture();
        ClipCursor(NULL);
        
        hr = THR(UnParse());
    }

    else if(_fMoving)                   // Moving an area
    {
        _fMoving = FALSE;

        if(_nShapeType == SHAPE_TYPE_POLY)
        {
            hr = THR(UpdatePolygon());
        }

        ReleaseCapture();
        ClipCursor(NULL);
        
        hr = THR(UnParse());
    }

    UpdateToolbar();

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnMouseMove
//
// Synopsis:    Handles the WM_MOUSEMOVE message, during sizing/moving
//              operations.  As far as mousemove is concerened, creation
//              is a sizing operation.
//
// Arguments:   CMessage *pmsg - the Message (what a surprise)
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnMouseMove(CMessage *pmsg)
{
    POINT   pt;
    HDC     hDC;
    HRESULT hr = S_OK;

    pt = pmsg->pt;
    pt.x = pt.x + _siHorz.nPos - _rcClient.left;
    pt.y = pt.y + _siVert.nPos - _rcClient.top;

    if(GetCapture() == pmsg->hwnd && _fSelected)
    {
        hDC = GetSetDC();

        // Erase the area
        hr = THR(DrawCurrent(hDC, _rcClient));
        IGNORE_HR(DrawGrabHandles(hDC));

        // Moving the mouse during resizing
        if(_nSizing)
        {
            // We only care about the Shift-drag constraint
            // when we're creating a new area.
            hr = THR(SizeTo(_nSizing, 
                            pt.x, 
                            pt.y, 
                            pmsg->wParam & MK_SHIFT && _fCreating));
        }

        // Moving the mouse during a "move area" operation
        else if(_fMoving)
        {
            // MoveTo takes the offset to move the area by
            hr = THR(MoveTo(pt.x - _ptMovePoint.x, pt.y - _ptMovePoint.y));

            _ptMovePoint.x = pt.x;
            _ptMovePoint.y = pt.y;
        }

        if(_nShapeType == SHAPE_TYPE_POLY)
        {
            UpdatePolygon();
        }

        // Redraw the new area
        hr = THR(DrawCurrent(hDC, _rcClient));
        IGNORE_HR(DrawGrabHandles(hDC));

        ReleaseDC(pmsg->hwnd, hDC);
    }

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnPaint
//
// Synopsis:    Handles the WM_PAINT message to draw the map
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnPaint()
{
    PAINTSTRUCT ps;
    HRESULT hr;
    HDC hDC;
    IAreaElement *pIArea = NULL;
    IElementCollection *pCollection = NULL;
    LONG c;
    IDispatch *pDisp;
    HRGN hClip;
    VARIANT vt1, vt2;
    RECT rcTemp;

    Assert(_pMap);

    GetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz);
    GetScrollInfo(_hwndVScroll, SB_CTL, &_siVert);
        
    rcTemp.left   = _rcImg.left   - _siHorz.nPos + _rcClient.left;
    rcTemp.top    = _rcImg.top    - _siVert.nPos + _rcClient.top;
    rcTemp.right  = _rcImg.right  - _siHorz.nPos + _rcClient.left;
    rcTemp.bottom = _rcImg.bottom - _siVert.nPos + _rcClient.top;


    // Set things up
    hDC = BeginPaint(_hwndDialog, &ps);
    GetPalette(NULL, hDC);

    // Draw the image
    _pImgCtx->Draw(hDC, (RECTL *)&rcTemp, NULL);

    SetROP2(hDC, R2_XORPEN);
    SelectObject(hDC, _hpenXOR);
    SelectObject(hDC, _hbrHollow);

    // Set up the clipping region
    hClip = CreateRectRgn(_rcClient.left, 
                          _rcClient.top, 
                          _rcClient.right, 
                          _rcClient.bottom);
    SelectClipRgn(hDC, hClip);


    hr = THR(_pMap->get_areas(&pCollection));
    if(hr)
        goto Cleanup;

    hr = THR(pCollection->get_count(&c));
    if(hr)
        goto Cleanup;

    V_VT(&vt1) = VT_I4;
    for(; c > 0; c--)
    {
        V_I4(&vt1) = c - 1;
        hr = THR(pCollection->item(vt1, vt2, &pDisp));
        if(hr)
            goto Error;

        hr = pDisp->QueryInterface(IID_IAreaElement, (void **)&pIArea);
        if(hr)
            goto Error;

        ClearInterface(&pDisp);

        hr = THR(ParseArea(pIArea));
        hr = THR(DrawCurrent(hDC, _rcClient));

        ClearInterface(&pIArea);
    }

    if(_fSelected)
    {
        hr = THR(ParseArea(_pareaCurrent));
        IGNORE_HR(DrawGrabHandles(hDC));
    }


Cleanup:
    EndPaint(_hwndDialog, &ps);
    DeleteObject(hClip);
    ReleaseInterface(pCollection);

Error:
    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnRButtonDown
//
// Synopsis:    Handles WM_RBUTTONDOWN messages prior to context menu, so
//              that the appropriate area is selected and grab-handled.
//
// Arugments:   CMessage *pmsg - the message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnRButtonDown(CMessage *pmsg)
{
    HRESULT hr;
    POINT pt;

    pt = pmsg->pt;
    pt.x = pt.x + _siHorz.nPos - _rcClient.left;
    pt.y = pt.y + _siVert.nPos - _rcClient.top;


    hr = THR_NOTRACE(CheckSelect(pt));

    RRETURN(hr);
}


//=-----------------------------------------------------------------------=
// 
// Function:    OnScroll
//
// Synopsis:    Handles the WM_VSCROLL and WM_HSCROLL messages to scroll
//              the dialog's client area.
//
// Arguments:   CMessage *pmsg - the Message
//              long lPos - The position that was scrolled to..  This is
//                  necessary because of the sneaky usage of the 
//                  high word of wParam (high word of a word. clever.)
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnScroll(CMessage *pmsg, long lPos)
{
    SCROLLINFO siScrollBar;
    RECT       rcUpdate;
    int        dx, dy;

    GetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz);
    GetScrollInfo(_hwndVScroll, SB_CTL, &_siVert);

    // dx will hold the change, and then we'll set dx and
    // dy appropriately after determing the type of scroll.
    dx = dy = 0;

    switch(pmsg->message)
    {
    case WM_VSCROLL:
        siScrollBar = _siVert;
        break;

    case WM_HSCROLL:
        siScrollBar = _siHorz;
        break;
    }

    switch(LOWORD(pmsg->wParam))
    {
    case SB_LINEUP:
        if(siScrollBar.nPos > siScrollBar.nMin)
        {
            --siScrollBar.nPos;
            dx = 1;
        }
        break;

    case SB_LINEDOWN:
        if(siScrollBar.nPos + (int)siScrollBar.nPage < siScrollBar.nMax)
        {
            ++siScrollBar.nPos;
            dx = -1;
        }
        break;

    case SB_PAGEUP:
        siScrollBar.nPos -= siScrollBar.nPage;
        dx = siScrollBar.nPage;

        if(siScrollBar.nPos < siScrollBar.nMin)
        {
            dx -= (siScrollBar.nPos - siScrollBar.nMin);
            siScrollBar.nPos = siScrollBar.nMin;
        }
        break;

    case SB_PAGEDOWN:
        siScrollBar.nPos += siScrollBar.nPage;
        dx = 0 - siScrollBar.nPage;

        if(siScrollBar.nPos + (int)siScrollBar.nPage > siScrollBar.nMax)
        {
            dx -= siScrollBar.nMax - siScrollBar.nPage - siScrollBar.nPos;
            siScrollBar.nPos = siScrollBar.nMax - siScrollBar.nPage;
        }
        break;

    case SB_THUMBTRACK:
        dx = siScrollBar.nPos - lPos;
        siScrollBar.nPos = lPos;
        break;

    }

    switch(pmsg->message)
    {
    case WM_VSCROLL:
        SetScrollInfo(_hwndVScroll, SB_CTL, &siScrollBar, TRUE);
        dy = dx;
        dx = 0;
        break;

    case WM_HSCROLL:
        SetScrollInfo(_hwndHScroll, SB_CTL, &siScrollBar, TRUE);
        dy = 0;
        break;
    }

    if(dx || dy)
    {
        ScrollWindowEx(_hwndDialog, 
                       dx, 
                       dy, 
                       NULL, 
                       &_rcClient, 
                       NULL, 
                       &rcUpdate, 
                       SW_ERASE);
        InvalidateRect(_hwndDialog, &rcUpdate, FALSE);
    }

    GetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz);
    GetScrollInfo(_hwndVScroll, SB_CTL, &_siVert);

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnSetCursor
//
// Synopsis:    Yup, you guessed it.  Handles WM_SETCURSOR messages.
//              returns false is the mouse was outside the client area
//              so that the cursor will get set appropriately.
//
// Arguments:   CMessage *pmsg - the Holy grail, aka the Message.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnSetCursor(CMessage *pmsg)
{
    HRESULT hr = S_OK;
    int     nBorder;
    HCURSOR hCursor;
    LPCTSTR alpCursors[4] = 
        {IDC_SIZEWE, IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW};
    POINT   pt;

    pt = pmsg->pt;
    
    GetCursorPos(&pt);
    ScreenToClient(_hwndDialog, &pt);
    if(pt.x > _rcClient.right || pt.y > _rcClient.bottom)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    pt.x = pt.x + _siHorz.nPos - _rcClient.left;
    pt.y = pt.y + _siVert.nPos - _rcClient.top;

    if(_fSelected)
    {
        nBorder = PointOnBorder(pt);
        if(nBorder == 0)
        {
            hCursor = ::LoadCursor(NULL, IDC_ARROW);
        }
        else
        {
            switch(_nShapeType)
            {
            case SHAPE_TYPE_RECT:
                hCursor = ::LoadCursor(NULL, alpCursors[nBorder % 4]);
                break;

            case SHAPE_TYPE_CIRCLE:     // Fall Through
            case SHAPE_TYPE_POLY:       
                hCursor = ::LoadCursor(NULL, IDC_SIZEALL);
                break;

            }
        }
    }
    else
    {
        hCursor = ::LoadCursor(NULL, IDC_ARROW);
    }

    ::SetCursor(hCursor);

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//=-----------------------------------------------------------------------=
// 
// Function:    OnShowProperties
//
// Synopsis:    Shows the properties of the currently selected area
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnShowProperties()
{
    HRESULT           hr = S_OK;
    HDC               hDC;
    IServiceProvider *pSP = NULL;
    HWND              hwnd;

    THR_NOTRACE(_pDoc->_pClientSite->QueryInterface(
            IID_IServiceProvider,
            (void **)&pSP));

    hwnd = _pDoc->DoEnableModeless(FALSE);

    hr = THR(ShowPropertyDialog(
        1, 
        _fSelected ? (IUnknown **)&_pareaCurrent : (IUnknown **)&_pMap,
        hwnd,
        pSP,
        LOCALE_USER_DEFAULT));

    _pDoc->DoEnableModeless(TRUE);

    if(hr)
        goto Error;
    //
    // Refresh the changes:
    //

    hDC = GetSetDC();

    // Erase the area as it was
    hr = THR(DrawCurrent(hDC, _rcClient));
    IGNORE_HR(DrawGrabHandles(hDC));

    // Get the changes
    hr = THR(ParseArea(_pareaCurrent));
    if(hr)
        goto Error;

    // And redraw it.
    hr = THR(DrawCurrent(hDC, _rcClient));
    IGNORE_HR(DrawGrabHandles(hDC));

Error:
    ReleaseInterface(pSP);
    RRETURN1(hr, S_FALSE);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnSize
//
// Synopsis:    Handles WM_SIZE messages.  Note that when a scrollbar is
//              all the way "down", if we make the window bigger, we 
//              effectively have to scroll the window.  
//
// Arguments:   CMessage *pmsg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnSize(CMessage *pmsg)
{
    RECT rcUpdate, rc;
    int dx, dy;

    dx = dy = 0;

    GetViewRect(&_rcClient);
    GetClientRect(_hwndDialog, &rc);

    IGNORE_HR(InitScrollBars());
    // BUGBUG(t-johnha): Do I need these?
    GetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz);
    GetScrollInfo(_hwndVScroll, SB_CTL, &_siVert);

    _fSBVert = (rc.bottom - rc.top - (_rcToolbar.bottom - _rcToolbar.top) < 
               _rcImg.bottom);
    if(_fSBVert)
    {
        _fSBHorz = (rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL) < 
                   _rcImg.right);
    }
    else
    {
        _fSBHorz = (rc.right - rc.left < _rcImg.right);
    }
    if(_fSBHorz)
    {
        _fSBVert = (rc.bottom - rc.top - GetSystemMetrics(SM_CYHSCROLL) - 
                   (_rcToolbar.bottom - _rcToolbar.top) < _rcImg.bottom);
    }

    if(!_fSBVert)
    {
        GetWindowRect(_hwndDialog, &rc);
        ShowWindow(_hwndVScroll, SW_HIDE);
        if(rc.right - rc.left - _nXFrame >= 
           _rcImg.right)
        {
            MoveWindow(_hwndDialog,
                       rc.left,
                       rc.top,
                       _rcImg.right + _nXFrame,
                       rc.bottom - rc.top,
                       TRUE);
            InvalidateRect(_hwndDialog, &_rcClient, FALSE);
        }
    }
    else
    {
        ShowWindow(_hwndVScroll, SW_SHOW);
    }
    if(!_fSBHorz)
    {
        GetWindowRect(_hwndDialog, &rc);
        ShowWindow(_hwndHScroll, SW_HIDE);
        if(rc.bottom - rc.top - _nYSpace >= _rcImg.right)
        {
            MoveWindow(_hwndDialog,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       _rcImg.bottom + _nYSpace,
                       TRUE); 
            InvalidateRect(_hwndDialog, &_rcClient, FALSE);
        }
    }
    else
    {
        ShowWindow(_hwndHScroll, SW_SHOW);
    }

    if(!(_fSBHorz && _fSBVert))
    {
        ShowWindow(_hwndSizeGrip, SW_HIDE);
    }
    else
    {
        ShowWindow(_hwndSizeGrip, SW_SHOW);
    }

    //
    // If a scrollbar is at the bottom position, and we size large,
    // we actually want to scroll.
    //

    // Scroll right?
    if(_siHorz.nPos + _siHorz.nPage >= (UINT)_siHorz.nMax &&
        _rcClient.right > _nOldWidth)
    {
        dx = _rcClient.right - _nOldWidth;
    }

    // Scroll down?
    if(_siVert.nPos + _siVert.nPage >= (UINT)_siVert.nMax &&
        _rcClient.bottom > _nOldHeight)
    {
        dy = _rcClient.bottom - _nOldHeight;
    }

    GetViewRect(&_rcClient);

    MoveWindow(_hwndHScroll, 
               _rcClient.left,
               _rcClient.bottom,
               _rcClient.right,
               GetSystemMetrics(SM_CYHSCROLL),
               TRUE);
    MoveWindow(_hwndVScroll, 
               _rcClient.right,
               _rcClient.top,
               GetSystemMetrics(SM_CXVSCROLL),
               _rcClient.bottom - _rcClient.top,
               TRUE);
    MoveWindow(_hwndSizeGrip,
               _rcClient.right,
               _rcClient.bottom,
               GetSystemMetrics(SM_CXVSCROLL),
               GetSystemMetrics(SM_CYHSCROLL),
               TRUE);

   if(dx || dy)
    {
        ScrollWindowEx(_hwndDialog, 
                       dx, 
                       dy, 
                       &_rcClient, 
                       &_rcClient, 
                       NULL, 
                       &rcUpdate, 
                       SW_ERASE);
        InvalidateRect(_hwndDialog, &rcUpdate, FALSE);
    }

    SendMessage(_hwndToolbar, TB_AUTOSIZE, 0, 0);

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    OnGetMinMaxInfo
//
// Synopsis:    Handles WM_GETMINMAXINFO messages to restrict the size
//              of the window to no larger than the image.
//
// Arguments:   CMessage *pmsg - the Messsage
//
//=-----------------------------------------------------------------------=
HRESULT 
CMapEditDialog::OnGetMinMaxInfo(CMessage *pmsg)
{
    MINMAXINFO *pmmi;
    RECT rc;

    pmmi = (LPMINMAXINFO)pmsg->lParam;
    
    GetClientRect(_hwndDialog, &rc);
    rc.top += _rcToolbar.bottom - _rcToolbar.top;

    //BUGBUG(t-johnha): Are these necessary?  repeats
    // onsize.
    _fSBVert = (rc.bottom - rc.top < _rcImg.bottom);
    if(_fSBVert)
    {
        _fSBHorz = (rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL) < 
                    _rcImg.right);
    }
    else
    {
        _fSBHorz = (rc.right - rc.left < _rcImg.right);
    }
    if(_fSBHorz)
    {
        _fSBVert = (rc.bottom - rc.top - GetSystemMetrics(SM_CYHSCROLL) < 
                    _rcImg.bottom);
    }


    pmmi->ptMaxTrackSize.x = _rcImg.right + _nXFrame;
    pmmi->ptMaxTrackSize.y = _rcImg.bottom + _nYSpace;                             
    pmmi->ptMaxSize = pmmi->ptMaxTrackSize;

    if(_siVert.nPage < (UINT)_siVert.nMax)
    {
        pmmi->ptMaxTrackSize.x += GetSystemMetrics(SM_CXVSCROLL);
    }
    if(_siHorz.nPage < (UINT)_siHorz.nMax)
    {
        pmmi->ptMaxTrackSize.y += GetSystemMetrics(SM_CYHSCROLL);
    }


    _nOldWidth = _rcClient.right;
    _nOldHeight = _rcClient.bottom;

    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
// 
// Function:    OnSysCommand
//
// Synopsis:    Handles the WM_SYSCOMMAND message,
//              mainly to check for a maximize op.
//
// Arguments:   CMessage *pmsg - the Message
//
//=-----------------------------------------------------------------------=
HRESULT
CMapEditDialog::OnSysCommand(CMessage *pmsg)
{
    switch(pmsg->wParam)
    {
    case SC_MAXIMIZE:
        if(GetSystemMetrics(SM_CXSCREEN) > _rcImg.right)
        {
            _rcClient.right = _rcImg.right;
            _siHorz.nPage = _siHorz.nMax + 1;
            SetScrollInfo(_hwndHScroll, SB_CTL, &_siHorz, TRUE);
        }
        if(GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYCAPTION) > 
           _rcImg.bottom)
        {
            _rcClient.bottom = _rcImg.bottom;
            _siVert.nPage = _siVert.nMax + 1;
            SetScrollInfo(_hwndVScroll, SB_CTL, &_siVert, TRUE);
        }
        break;
    }

    RRETURN(S_OK);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Message Handlers
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



//=-----------------------------------------------------------------------=
//
// Function:    DlgProc
//
// Synopsis:    Handles all the messages
//
// Arguments:   HWND hwnd - Window handle
//              UINT msg - The message
//              WPARAM wParam
//              LPARAM lParam
//
//=-----------------------------------------------------------------------=
BOOL CALLBACK
CMapEditDialog::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMapEditDialog *pDialog = (CMapEditDialog *)GetWindowLong(hwnd, DWL_USER);
    CMessage   *pmsg = NULL;

    if(pDialog)
    {
        pmsg = new CMessage(pDialog->_hwndDialog, msg, wParam, lParam);

        pmsg->pt.x = LOWORD(lParam);
        pmsg->pt.y = HIWORD(lParam);
    }

    switch(msg)
    {
    case WM_CLOSE:
        pDialog->OnClose();
        break;

    case WM_COMMAND:
        pDialog->OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        break;

    case WM_CONTEXTMENU:
        pDialog->OnContextMenu(pmsg);
        break;

    case WM_GETMINMAXINFO:
        pDialog->OnGetMinMaxInfo(pmsg);
        break;

    case WM_HSCROLL:
        pDialog->OnScroll(pmsg, HIWORD(wParam));
        break;

    case WM_INITDIALOG:
        SetWindowLong(hwnd, DWL_USER, lParam);
        pDialog = (CMapEditDialog *)lParam;
        pDialog->_hwndDialog = hwnd;
        pDialog->_hr = S_OK;
        pDialog->InitDialog();
        return FALSE;
        break;

    case WM_INITMENUPOPUP:
        pDialog->OnInitMenuPopup(pmsg);
        break;

    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_DELETE:
            pDialog->OnDelete();
            break;
        }
        break;

    case WM_LBUTTONDBLCLK:
        pDialog->OnLButtonDblClk(pmsg);
        break;

    case WM_LBUTTONDOWN:
        pDialog->OnLButtonDown(pmsg);
        break;

    case WM_LBUTTONUP:
        pDialog->OnLButtonUp(pmsg);
        break;

    case WM_MOUSEMOVE:
        pDialog->OnMouseMove(pmsg);
        break;

    case WM_PAINT:
        pDialog->OnPaint();
        break;

    case WM_RBUTTONDOWN:
        pDialog->OnRButtonDown(pmsg);
        break;

    case WM_SETCURSOR:
        if(pDialog->OnSetCursor(pmsg) == S_FALSE)
            goto NoHandle;
        break;

    case WM_SIZE:
        pDialog->OnSize(pmsg);
        break;

    case WM_SYSCOMMAND:
        pDialog->OnSysCommand(pmsg);
        goto NoHandle;
        break;

    case WM_VSCROLL:
        pDialog->OnScroll(pmsg, HIWORD(wParam));
        break;

    default:
        goto NoHandle;
        break;
    }

 
    delete pmsg;
    return TRUE;

NoHandle:
    delete pmsg;
    return FALSE;

}



//=-----------------------------------------------------------------------=
//
// Function:    DoMapEditDialog
//
// Synopsis:    Initializes and runs the Image Map Editor dialog
//
// Arguments:   HWND hwnd - Handle to window for dialog
//
//=-----------------------------------------------------------------------=
int
DoMapEditDialog(HWND hwnd, CSite *pSite)
{
    HRESULT        hr;
    CMapEditDialog dlgMapEdit;
    IImgElement  *pImg;

    hr = THR((pSite)->QueryInterface(IID_IImgElement, (void **)&pImg));
    if(hr)
        goto Error;

    dlgMapEdit._pDoc = (pSite)->_pDoc;
    hr = THR(dlgMapEdit.Init(pImg));
    if(hr)
        goto Error;

    ReleaseInterface(pImg);

    DialogBoxParam(GetResourceHInst(),
                   MAKEINTRESOURCE(IDR_MAPEDIT_DIALOG),
                   hwnd,
                   &CMapEditDialog::DlgProc,
                   (LPARAM)&dlgMapEdit);

    // Refresh the new map on the image.
    pSite->Invalidate(NULL, 0);


Error:
    return -1;
}


