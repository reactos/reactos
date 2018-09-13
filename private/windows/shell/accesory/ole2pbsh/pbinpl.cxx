//+---------------------------------------------------------------------
//
//   File:       pbinpl.cxx
//
//   Contents:   OLE2 Server Class code
//
//   Classes:
//               PBInPlace
//
//------------------------------------------------------------------------

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>      // common dialog boxes

#include <ole2.h>
#include <o2base.hxx>     // the base classes and utilities

#include "pbs.hxx"

    TCHAR szIPWndClass[] = L"PBrushIPWnd";

extern "C" {
    void EnablePickMenu(BOOL bEnable);
}

//+---------------------------------------------------------------
//
//  Member:    PBInPlace::SetPaintWindowPos
//
//  Synopsis:  Set position of Paint window within us
//
//---------------------------------------------------------------
void
PBInPlace::SetPaintWindowPos(void)
{
    gprcApp[iFrame] = _rcFrame;
    InflateRect(&gprcApp[iFrame], -UIBORDER_WIDTH, -UIBORDER_WIDTH);
    gprcApp[iPaint] = gprcApp[iFrame];
    ResetPaintWindow(); //call back into old PBrush code to take care of window state
    CalcWnds(NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW);
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::ClassInit (static)
//
//  Synopsis:   Load static resources and register windows class
//
//  Arguments:  [pClass] -- an initialized ClassDescriptor
//
//  Returns:    TRUE iff window class sucesfully registered
//
//---------------------------------------------------------------

BOOL
PBInPlace::ClassInit(LPCLASSDESCRIPTOR pClass)
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = IPWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;          // for pointer to OPInPlace object
    wc.hInstance = pClass->_hinst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);   //was NULL
    wc.hbrBackground = NULL;    // we draw our own background
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szIPWndClass;
    return RegisterClass(&wc) != NULL;
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::Create (static)
//
//  Synopsis:   Create a new, fully initialize sub-object
//
//  Arguments:  [pPBCtrl] --  pointer to control we are a part of
//              [pClass] -- pointer to initialized class descriptor
//              [ppUnkCtrl] -- (out parameter) pObj's controlling unknown
//              [ppObj] -- (out parameter) the new sub-object
//
//  Returns:    NOERROR iff sucessful
//
//---------------------------------------------------------------

HRESULT
PBInPlace::Create( LPPBCTRL pPBCtrl,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPPBINPLACE FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    // create an object
    HRESULT hr;
    LPPBINPLACE pObj;
    pObj = new PBInPlace((LPUNKNOWN)(LPOLEOBJECT)pPBCtrl);
    if (pObj == NULL)
    {
        hr =  E_OUTOFMEMORY;
    }
    else
    {
        // initialize it
        if (OK(hr = pObj->Init(pPBCtrl, pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = pObj;
        }
        else
        {
            pObj->_PrivUnk.Release();    //destroy the object
        }
    }
    if(OK(hr))
    {
        hr = NOERROR;
    }
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     PBInPlace::PBInPlace
//
//  Synopsis:   Construct a new IP sub-object
//
//  Arguments:  [pUnkOuter] -- Unknown to aggregate with
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // `this' argument to base-member init. list.
PBInPlace::PBInPlace(LPUNKNOWN pUnkOuter):
        _PrivUnk(this)  // controlling unknown holds a pointer to us
{
    _pUnkOuter = (pUnkOuter != NULL) ? pUnkOuter : (LPUNKNOWN)&_PrivUnk;
    _hwndDropTarget = NULL;
    SetRectEmpty(&_rcVis);
}
#pragma warning(default:4355)


//+---------------------------------------------------------------
//
//  Member:     PBInPlace::~PBInPlace
//
//  Synopsis:   Destroy this sub-object
//
//---------------------------------------------------------------

PBInPlace::~PBInPlace(void)
{
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::Init
//
//  Synopsis:   Initialize sub-object
//
//  Arguments:  [pPBCtrl] --  pointer to control we are a part of
//              [pClass]  -- pointer to an initialized class descriptor
//
//  Returns:    NOERROR if sucessful
//
//---------------------------------------------------------------

HRESULT
PBInPlace::Init(LPPBCTRL pPBCtrl, LPCLASSDESCRIPTOR pClass)
{
    HRESULT hr = SrvrInPlace::Init(pClass, pPBCtrl);
    return hr;
}

IMPLEMENT_DELEGATING_IUNKNOWN(PBInPlace)

IMPLEMENT_PRIVATE_IUNKNOWN(PBInPlace)

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::QueryInterface, public
//
//     OLE:     IUnkown
//
//  Synopsis:   Expose our IFaces
//
//---------------------------------------------------------------
STDMETHODIMP
PBInPlace::PrivateUnknown::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
#ifdef DUMP_QI
#if DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"PBInPlace::PrivateUnknown::QueryInterface (%lx)\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif
#endif // DUMP_QI

    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IOleInPlaceObject)
          || IsEqualIID(riid,IID_IOleWindow))
    {
        *ppv = (LPVOID)(LPOLEINPLACEOBJECT)_pPBInPlace;
    }
    else if (IsEqualIID(riid,IID_IOleInPlaceActiveObject))
    {
        *ppv = (LPVOID)(LPOLEINPLACEACTIVEOBJECT)_pPBInPlace;
    }
    else if (IsEqualIID(riid,IID_IDropTarget))
    {
        *ppv = (LPVOID)(LPDROPTARGET)_pPBInPlace;
    }
    else
    {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    //
    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    //
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::OnFrameWindowActivate, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method changes the color of our border shading
//              depending on whether our frame window is activating
//              or deactivating.
//
//---------------------------------------------------------------


STDMETHODIMP
PBInPlace::OnFrameWindowActivate(BOOL fActivate)
{
    DOUT(L"PBInPlace::OnFrameWindowActivate\r\n");

    if(fActivate && _hwnd && !gfInDialog && (_pCtrl->State() != OS_OPEN))
        SetFocus(_hwnd);

    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     PBInPlace::SetObjectRects, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method puts is used by containers
//              to set the position and size of InPlace objects (us)
//
//---------------------------------------------------------------

STDMETHODIMP
PBInPlace::SetObjectRects(LPCRECT lprcPos, LPCRECT lprcVisRect)
{
    DOUT(L"PBInPlace::SetObjectRects\r\n");

    RECT rc = *lprcPos;
    _rcVis = *lprcVisRect;
    InflateRect(&rc, UIBORDER_WIDTH, UIBORDER_HEIGHT);
    return SrvrInPlace::SetObjectRects(&rc, lprcVisRect);
}


//+---------------------------------------------------------------
//
//  Member:     PBInPlace::DragEnter
//
//     OLE:     IDropTarget
//
//  Synopsis:   Answer whether the target could accept a drop
//
//---------------------------------------------------------------
STDMETHODIMP
PBInPlace::DragEnter( LPDATAOBJECT pDataObj,
                    DWORD grfKeyState,
                    POINTL ptl,
                    LPDWORD pdwEffect)
{
    //
    // check the formats available via this transfer data-object
    // if we find one we understand, set _fCanDrop = TRUE
    //
    CLIPFORMAT cf;
    _fCanDrop = ObjectOffersAcceptableFormats(pDataObj, &cf);

    _ptLast.x = (int)ptl.x;
    _ptLast.y = (int)ptl.y;
    ScreenToClient(_hwndDropTarget, &_ptLast); //convert to client coordinates

    //
    // get the object descriptor from the object to determine its size
    // and position relative to the cursor
    //
    SetRectEmpty(&_rcLastFeedback);
    OBJECTDESCRIPTOR objdesc;
    if (OK(GetObjectDescriptor(pDataObj, &objdesc)))
    {
        _sizeObj.cx = HPixFromHimetric(objdesc.sizel.cx);
        _sizeObj.cy = VPixFromHimetric(objdesc.sizel.cy);
        _ptOffset.x = HPixFromHimetric(objdesc.pointl.x);
        _ptOffset.y = VPixFromHimetric(objdesc.pointl.y);
    }
    else
    {
        _sizeObj.cx = _sizeObj.cy = _ptOffset.x = _ptOffset.y = 0;
    }

    //
    // determine the drop-effect for user feedback...
    return DragOver(grfKeyState, ptl, pdwEffect);
}

//+---------------------------------------------------------------
//
//  Member:     OPInPlace::DragOver
//
//     OLE:     IDropTarget
//
//  Synopsis:   Manage user feedback
//
//---------------------------------------------------------------
STDMETHODIMP
PBInPlace::DragOver(DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    Assert(pdwEffect != NULL);

    *pdwEffect = DROPEFFECT_NONE;
    if(!_fCanDrop)
        return NOERROR;

    POINT pt; pt.x = (int)ptl.x; pt.y = (int)ptl.y;
    ScreenToClient(_hwndDropTarget, &pt);

    //
    // update the feedback rectangle
    //
    _ptLast = pt;
    HDC hdc = GetDC(_hwndDropTarget);
    DrawFocusRect(hdc, &_rcLastFeedback);
    _rcLastFeedback.left = pt.x - _ptOffset.x;
    _rcLastFeedback.top = pt.y - _ptOffset.y;
    _rcLastFeedback.right = _rcLastFeedback.left + _sizeObj.cx;
    _rcLastFeedback.bottom = _rcLastFeedback.top + _sizeObj.cy;
    DrawFocusRect(hdc, &_rcLastFeedback);
    ReleaseDC(_hwndDropTarget, hdc);

    //
    // Determine the drag effect...
    //
    // look at the key combinations:
    //
    // NONE      => move operation
    // SHFT      => move operation
    // CTRL      => copy operation
    // SHFT+CTRL => link operation (which we don't support)
    //
    if (grfKeyState & MK_CONTROL)
    {
        if (!(grfKeyState & MK_SHIFT))
            *pdwEffect = DROPEFFECT_COPY;
    }
    else
        *pdwEffect = DROPEFFECT_MOVE;

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::DragLeave
//
//     OLE:     IDropTarget
//
//  Synopsis:   Remove any user feedback
//
//---------------------------------------------------------------
STDMETHODIMP
PBInPlace::DragLeave(void)
{
    //
    // erase the feedback rectangle
    //
    HDC hdc = GetDC(_hwndDropTarget);
    DrawFocusRect(hdc, &_rcLastFeedback);
    ReleaseDC(_hwndDropTarget, hdc);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::Drop
//
//     OLE:     IDropTarget
//
//  Synopsis:   Handle the drop operation
//
//---------------------------------------------------------------

STDMETHODIMP
PBInPlace::Drop( LPDATAOBJECT pDataObj,
                DWORD grfKeyState,
                POINTL ptl,
                LPDWORD pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;
    HRESULT hr = NOERROR;
    if(_fCanDrop)
    {
        CLIPFORMAT cf;
        HGLOBAL hGlobal;
        if(GetTypedHGlobalFromObject(pDataObj, &cf, &hGlobal) == NOERROR)
        {
            SetupForDrop(cf, ptl);
            PasteTypedHGlobal(cf, hGlobal);
        }
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:    PBInPlace::AttachWin
//
//  Synopsis:  Create our InPlace window
//
//  Arguments: [hwndParent] -- our container's hwnd
//
//  Returns:   hwnd of InPlace window, or NULL
//
//---------------------------------------------------------------
HWND
PBInPlace::AttachWin(HWND hwndParent)
{
    DOUT(L"PBInPlace::AttachWin\r\n");

    //
    // remember current PBrush frame window state
    //
    _rcPBrush = gprcApp[iFrame];
    _hwndPBrush = gpahwndApp[iFrame];

    DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    return CreateWindow(szIPWndClass,
                            NULL, dwStyle,
                            0, 0, 0, 0,     // will get resized later
                            hwndParent,
                            0,              // no child ID
                            _pClass->_hinst,
                            this);
}

//+---------------------------------------------------------------
//
//  Member:    PBInPlace::SwitchIPContext
//
//  Synopsis:  Manage reparenting context-switch of PBrush windows
//
//---------------------------------------------------------------
void
PBInPlace::SwitchIPContext( BOOL fBeInPlace )
{
    DOUT(L"PBInPlace::SwitchIPContext\r\n");

    if(gfInPlace = fBeInPlace)
    {
        _hmenuPBrush = ghMenuFrame; //save the PBrush menu handle
        gafMenuPresent[0] = FALSE;
        gafMenuPresent[2] = FALSE;
        gpahwndApp[iFrame] = _hwnd;
        SetParent(gpahwndApp[iPaint], _hwnd);
        if((_trayTool = ToolTray::Create(_pClass->_hinst,
                                        NULL,
                                        _hwnd,
                                        iTool)) != NULL)
        {
            SetParent(gpahwndApp[iTool], _trayTool->WindowHandle());
            SetParent(gpahwndApp[iSize], _trayTool->WindowHandle());
        }
        if((_trayColor = ColorTray::Create(_pClass->_hinst,
                                        NULL,
                                        _hwnd,
                                        iColor)) != NULL)
        {
            SetParent(gpahwndApp[iColor], _trayColor->WindowHandle());
        }
        SetPaintWindowPos();
#ifdef SUPORT_DROP_TARGETTING
        RegisterAsDropTarget(gpahwndApp[iPaint]);
#endif
        return;
    }

#ifdef SUPORT_DROP_TARGETTING
    RevokeOurDropTarget();
#endif
    gprcApp[iFrame] = _rcPBrush;
    ghMenuFrame = _hmenuPBrush;

    gpahwndApp[iFrame] = _hwndPBrush;
    SetParent(gpahwndApp[iPaint], _hwndPBrush);
    SetParent(gpahwndApp[iTool], _hwndPBrush);
    SetParent(gpahwndApp[iSize], _hwndPBrush);
    SetParent(gpahwndApp[iColor], _hwndPBrush);

    _hwndPBrush = NULL;
    delete _trayTool;
    delete _trayColor;

    CalcWnds(NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW);
    for(int j = iPaint; j <= iColor; j++)
    {
        MoveWindow(gpahwndApp[j],
                gprcApp[j].left,
                gprcApp[j].top,
                gprcApp[j].right - gprcApp[j].left,
                gprcApp[j].bottom - gprcApp[j].top,
                TRUE);
        ShowWindow(gpahwndApp[j], SW_SHOWNA);
    }
    for(int i = 0; i < MAXmenus; i++)
        gafMenuPresent[i] = TRUE;
}


//+---------------------------------------------------------------
//
//  Member:    PBInPlace::LoadIPServerMenu
//
//  Synopsis:  Answer a modified copy of PBrush menus for use InPlace
//
//---------------------------------------------------------------
HMENU
PBInPlace::LoadIPServerMenu(void)
{
    HMENU hmenu = NULL;
    if((hmenu = ::LoadMenu(_pClass->_hinst, L"pbrush2")) != NULL)
    {
        HMENU hmenuOptions = ::GetSubMenu(hmenu, 5);
        EnableMenuItem(hmenuOptions, FILEclear, MF_GRAYED | MF_BYCOMMAND);

        HMENU hmenuFile = ::GetSubMenu(hmenu, 0);
        HMENU hmenuView = ::GetSubMenu(hmenu, 2);
        if(hmenuView != NULL)
            ::RemoveMenu(hmenu, 2, MF_BYPOSITION);
        if(hmenuFile != NULL)
            ::RemoveMenu(hmenu, 0, MF_BYPOSITION);
        if(hmenuFile != NULL)
            ::DestroyMenu(hmenuView);
        if(hmenuView != NULL)
            ::DestroyMenu(hmenuFile);
    }
    return hmenu;
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::CreateUI
//
//  Synopsis:   Create menus and toolbar
//
//---------------------------------------------------------------

void
PBInPlace::CreateUI(void)
{
    DOUT(L"PBInPlace::CreateUI\r\n");

    //
    // Load an instance of our server menus
    // (this menu will be destroyed when we deactivate)
    //
    _hmenu = LoadIPServerMenu();

    //
    // Grab the child windows for use by our InPlace window
    //
    SwitchIPContext(TRUE);

    //
    // let the base do the default OLE-menu settup
    //
    SrvrInPlace::CreateUI();
    //
    // BUGBUG: the following belongs in SwitchIPContext, but since
    //         the shared menu dosn't exist until after the base
    //         CreateUI call, we have to do this here...
    ghMenuFrame = _hmenuShared;
    EnablePickMenu(FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::CalcMenuPos
//
//  Synopsis:   Given the index of one of our menus, return
//              it's index in the composit menu bar
//
//---------------------------------------------------------------

int
PBInPlace::CalcMenuPos(int iMenu)
{
    int iActual = _mgw.width[0];
    for(int iGroup = 1; iMenu > 0 && iGroup < 6; )
    {
        if(iMenu <= _mgw.width[iGroup])
        {
            iActual += iMenu;
            break;
        }
        int iWidth = _mgw.width[iGroup++];
        iActual += iWidth;
        iMenu -= iWidth;
        //
        // The container owns the even groups
        //
        iActual += _mgw.width[iGroup++];
    }

    return iActual;
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::DestroyUI
//
//  Synopsis:   Cleanup allocations done in CreateUI
//
//---------------------------------------------------------------

void
PBInPlace::DestroyUI(void)
{
    DOUT(L"PBInPlace::DestroyUI\r\n");

    //
    // Release the child windows from use by our InPlace window
    //
    SwitchIPContext(FALSE);

    //
    // let the base do the default menu destruction
    //
    SrvrInPlace::DestroyUI();

    //
    // Free the menu we loaded earlier
    //
    if(_hmenu != NULL)
    {
        DestroyMenu(_hmenu);
        _hmenu = NULL;
    }
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::InstallFrameUI
//
//  Synopsis:   Integrate our UI with host UI
//
//---------------------------------------------------------------

void
PBInPlace::InstallFrameUI(void)
{
    DOUT(L"PBInPlace::InstallFrameUI\r\n");

    if(_trayTool)
    {
        int y = GetSystemMetrics(SM_CYCAPTION);
        RECT rcPos;
        GetWindowRect(_hwnd, &rcPos);
        RECT rcVis = _rcVis;
        HWND hwnd;
        _pInPlaceSite->GetWindow(&hwnd);
        RectToScreen(hwnd, &rcVis);
        RECT rc;
        IntersectRect(&rc, &rcPos, &rcVis);
        _trayTool->Position(rc.left, rc.top);
        _trayColor->Position(rc.left, rc.bottom - y);
        ShowWindow(_trayTool->WindowHandle(), SW_SHOW);
        ShowWindow(_trayColor->WindowHandle(), SW_SHOW);
    }

    // let the base class do the default menu installation
    SrvrInPlace::InstallFrameUI();
    //
    // BUGBUG: get the pick menu status updated corectly...
    //
    DrawMenuBar(GetFrameWindow());
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::RemoveFrameUI
//
//  Synopsis:   Take down our UI
//
//---------------------------------------------------------------
void
PBInPlace::RemoveFrameUI(void)
{
    DOUT(L"PBInPlace::RemoveFrameUI\r\n");

    if(_trayTool)
        ShowWindow(_trayTool->WindowHandle(), SW_HIDE);
    if(_trayColor)
        ShowWindow(_trayColor->WindowHandle(), SW_HIDE);

    // let the base do the default menu removal
    SrvrInPlace::RemoveFrameUI();
}


void
PBInPlace::SetFocus(HWND hwnd)
{
    if(_hwndPBrush && (gpahwndApp[iPaint] != GetFocus()))
    {
        DOUT(L"PBInPlace::SetFocus to Paint\r\n");

        ::SetFocus(gpahwndApp[iPaint]);
    }
    else
    {
        DOUT(L"PBInPlace::SetFocus to Self\r\n");

        ::SetFocus(hwnd);
    }
}

//+---------------------------------------------------------------
//
//  Function:   IPWndProc
//
//  Synopsis:   Window procedure for our InPlace window
//
//  Notes:      Uses windowsx.h message crackers to dispatch
//              messages to private members of PBInPlace
//
//---------------------------------------------------------------
extern "C" LRESULT CALLBACK
IPWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //
    // get a pointer to our object associated with this window
    // if our pointer is not set up yet then forward all messages
    // to the default window procedure until we get an WM_NCCREATE message
    // with the pointer so we can set up the desired association
    //
    LPPBINPLACE pWndObj = (LPPBINPLACE)GetWindowLong(hwnd, 0);
    if (pWndObj == NULL)
    {
        if (msg == WM_NCCREATE)
        {
            pWndObj = (LPPBINPLACE)((LPCREATESTRUCT)lParam)->lpCreateParams;
            pWndObj->_hwnd = hwnd;
            SetWindowLong(hwnd, 0, (LONG)pWndObj);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    if (msg == WM_NCDESTROY)
    {
        //
        // when we get the WM_NCDESTROY message then we need to break the
        // association between the window and the object
        //
        SetWindowLong(hwnd, 0, 0L);
        pWndObj->_hwnd = NULL;
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    //
    // convert window messages to method calls on the associated object
    //
    switch(msg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, pWndObj->OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, pWndObj->OnDestroy);
        HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, pWndObj->OnWindowPosChanged);
        HANDLE_MSG(hwnd, WM_PAINT, pWndObj->OnPaint);
        HANDLE_MSG(hwnd, WM_ERASEBKGND, pWndObj->OnEraseBkgnd);
    }

    if(pWndObj->_hwndPBrush)
    {
        if((msg >= WM_KEYFIRST && msg <= WM_KEYLAST)
            || (msg >= WM_INITMENU && msg <= WM_ENTERIDLE))
        {
            return SendMessage(pWndObj->_hwndPBrush, msg, wParam, lParam);
        }

        switch(msg)
        {
        case WM_COMMAND:
        case WM_MOUSEMOVE:
            return SendMessage(gpahwndApp[iPaint], msg, wParam, lParam);
            return 0;

        case WM_ACTIVATE:
        case WM_ERRORMSG:
        case WM_WININICHANGE:
        case WM_DROPFILES:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            return SendMessage(pWndObj->_hwndPBrush, msg, wParam, lParam);
        }
    }


    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------
//
//  Member:    PBInPlace::OnCreate
//
//  Synopsis:  establish default window state
//
//---------------------------------------------------------------
BOOL
PBInPlace::OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
    DOUT(L"PBInPlace::OnCreate\r\n");

    return TRUE;
}


//+---------------------------------------------------------------
//
//  Member:    PBInPlace::OnDestroy
//
//  Synopsis:  clean up window allocations
//
//---------------------------------------------------------------
void
PBInPlace::OnDestroy(HWND hwnd)
{
    DOUT(L"PBInPlace::OnDestroy\r\n");
}

void
PBInPlace::OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos)
{
    if (_fClientResize || lpwpos->flags & SWP_HIDEWINDOW)
       return;

    if(_pInPlaceSite != NULL)
    {
        RECT rc = { lpwpos->x,
                    lpwpos->y,
                    lpwpos->x + lpwpos->cx,
                    lpwpos->y + lpwpos->cy };

        //
        // tell the client we are the size of the region inside
        // our UIActive border...
        //
        InflateRect(&rc, -UIBORDER_WIDTH, -UIBORDER_HEIGHT);
        _pInPlaceSite->OnPosRectChange(&rc);

        DOUT(L"PBInPlace::OnWindowPosChanged (moving Paint)\r\n");

        SetPaintWindowPos();
    }
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::OnPaint
//
//---------------------------------------------------------------

#define ROP_DPa 0x00A000C9L

void
PBInPlace::OnPaint(HWND hwnd)
{
    static WORD wHatchBmp[] = {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};

    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    if(_pCtrl->State() == OS_UIACTIVE)
    {
        HBITMAP hbm = CreateBitmap(8, 8, 1, 1, wHatchBmp);
        HBRUSH hbr = CreatePatternBrush(hbm);
        HDC hdc = ps.hdc;
        HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
        RECT rc = ps.rcPaint;
        COLORREF cvText = SetTextColor(hdc, RGB(255, 255, 255));
        COLORREF cvBk = SetBkColor(hdc, RGB(0, 0, 0));

#ifdef SHADE_WHOLE_BACKGROUND
        PatBlt(hdc,
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                ROP_DPa );
#else //just shade the border
        PatBlt(hdc,
        rc.left, rc.top, rc.right - rc.left, UIBORDER_WIDTH,
        PATCOPY ); //ROP_DPa );
        PatBlt(hdc,
        rc.left, rc.top, UIBORDER_WIDTH, rc.bottom - rc.top,
        PATCOPY ); //ROP_DPa );
        PatBlt(hdc,
        rc.right - UIBORDER_WIDTH, rc.top, UIBORDER_WIDTH,rc.bottom - rc.top,
        PATCOPY ); //ROP_DPa );
        PatBlt(hdc,
        rc.left, rc.bottom - UIBORDER_WIDTH, rc.right-rc.left, UIBORDER_WIDTH,
        PATCOPY ); //ROP_DPa );
#endif SHADE_WHOLE_BACKGROUND

        SetTextColor(hdc, cvText);
        SetBkColor(hdc, cvBk);
        SelectObject(hdc, hbrOld);
        DeleteObject(hbr);
        DeleteObject(hbm);
    }
    if(ps.fErase)
        OnEraseBkgnd(hwnd, ps.hdc);

    EndPaint(hwnd, &ps);
}

//+---------------------------------------------------------------
//
//  Member:     PBInPlace::OnEraseBkgnd
//
//  Synopsis:   repaint our window background
//
//---------------------------------------------------------------
BOOL
PBInPlace::OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    InflateRect(&rc, -UIBORDER_WIDTH, -UIBORDER_HEIGHT);
    HBRUSH hbr = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    FillRect(hdc, &rc, hbr);
    return TRUE;
}
