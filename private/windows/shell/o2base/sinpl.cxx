//+---------------------------------------------------------------------
//
//   File:      sinpl.cxx
//
//   Contents:  Implementation of the SrvrInPlace class
//
//------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::SrvrInPlace, public
//
//  Synopsis:   Constructor for SrvrInPlace object
//
//  Notes:      To create a properly initialized object you must
//              call the Init method immediately after construction.
//
//---------------------------------------------------------------

SrvrInPlace::SrvrInPlace(void)
{
    DOUT(L"SrvrInPlace: Constructing\r\n");

    _hwnd = NULL;

    _pInPlaceSite = NULL;
    _frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    _pFrame = NULL;
    _pDoc = NULL;

    _hmenu = NULL;
    _hOleMenu = NULL;
    _hmenuShared = NULL;

    _fCSHelpMode = FALSE;
    _fChildActivating = FALSE;
    _fDeactivating = FALSE;
    _rcFrame.top = 0; _rcFrame.left = 0;
    _rcFrame.bottom = 0; _rcFrame.right = 0;

    _fClientResize = FALSE;
    _fUIDown = TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::Init, public
//
//  Synopsis:   Fully initializes a SrvrInPlace object
//
//  Arguments:  [pClass] -- The initialized class descriptor for the server
//              [pCtrl] -- The control subobject of the server we are a part of.
//
//  Returns:    NOERROR if successful
//
//  Notes:      The class descriptor pointer is saved in the protected _pClass
//              member variable where it is accessible during the lifetime
//              of the object.
//
//---------------------------------------------------------------

HRESULT
SrvrInPlace::Init(LPCLASSDESCRIPTOR pClass, LPSRVRCTRL pCtrl)
{
    _pClass = pClass;
    _pCtrl = pCtrl;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::~SrvrInPlace, protected
//
//  Synopsis:   Destructor for the SrvrCtrl object
//
//  Notes:      The destructor is called as a result of the servers
//              reference count going to 0.  It releases all held
//              resources.
//
//---------------------------------------------------------------

SrvrInPlace::~SrvrInPlace(void)
{
    DOUT(L"SrvrInPlace: Destructed\r\n");
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ActivateInPlace, public
//
//  Synopsis:   In-place activates the object
//
//  Arguments:  [pClientSite] -- The site on our container
//
//  Returns:    Success if we in-place activated properly
//
//  Notes:      This method implements the standard in-place activation
//              protocol and creates the in-place window.  It creates
//              all U.I. elements using CreateUI but does not
//              activate the U.I., for that is reserved for
//              the ActivateUI and InstallUI methods.
//
//---------------------------------------------------------------

HRESULT
SrvrInPlace::ActivateInPlace(LPOLECLIENTSITE pClientSite)
{
    DOUT(L"SrvrInPlace::ActivateInPlace\r\n");

    if (pClientSite == NULL)
    {
        DOUT(L"SrvrInPlace::ActivateInPlace E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    HWND hwndSite;
    RECT rect, rectVis;
    HRESULT hr;

    //
    // see if the client site supports in-place, and is willing to do so...
    //
    if (OK(hr = pClientSite->QueryInterface(IID_IOleInPlaceSite,
                (LPVOID FAR *)&_pInPlaceSite)))
    {
        if (OK(hr = _pInPlaceSite->CanInPlaceActivate()))
        {
            if (OK(hr = _pInPlaceSite->GetWindow(&hwndSite))
                    &&  OK(hr = _pInPlaceSite->GetWindowContext(&_pFrame,
                                &_pDoc,
                                &rect,
                                &rectVis,
                                &_frameInfo)))
            {
                if ((_hwnd = AttachWin(hwndSite)) == NULL)
                {
                    DOUT(L"SrvrInPlace::ActivateInPlace failed at AttachWin\r\n");
                    hr = E_UNEXPECTED;
                }
                else
                {
                    if (OK(hr = _pInPlaceSite->OnInPlaceActivate()))
                    {
                        if(_pCtrl->IsIPBEnabled())
                        {
                            _IPB.Bind(_pInPlaceSite, _hwnd, FALSE);
                        }

                        // create any U.I. elements
                        CreateUI();

                        // position and show the window
                        SetObjectRects(&rect, &rectVis);
                        ShowWindow(_hwnd, SW_SHOW);

                        return NOERROR;

                        //
                        //The rest of this code cleans up after errors...
                        //
                    }
                    DetachWin();
                }
                if (_pFrame != NULL)
                    _pFrame->Release();
                if (_pDoc != NULL)
                    _pDoc->Release();
            }
        }
        _pInPlaceSite->Release();
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::DeactivateInPlace, public
//
//  Synopsis:   In-place deactivates the object
//
//  Returns:    Success except for catastophic circumstances
//
//  Notes:      This method "undoes" everything done in ActivateInPlace
//              including destroying U.I. elements via DestroyUI, and
//              destroying the inplace active window.
//
//---------------------------------------------------------------

HRESULT
SrvrInPlace::DeactivateInPlace(void)
{
    DOUT(L"SrvrInPlace::DeactivateInPlace\r\n");

    //
    // The following prevents some nasty recursion cases, in which the call
    // bellow to OnInPlaceDeactivate get's us back in to the same transition we
    // are in now...
    //
    _pCtrl->SetState(OS_RUNNING);

    // undo everything we did in InPlaceActivate
    if(_pCtrl->IsIPBEnabled())
    {
        _IPB.Detach();
    }

    DestroyUI();
    DetachWin();

    _pInPlaceSite->OnInPlaceDeactivate();

    if (_pFrame != NULL)
        _pFrame->Release();
    if (_pDoc != NULL)
        _pDoc->Release();

    // release the in-place site we were holding on to.
    _pInPlaceSite->Release();
    _pInPlaceSite = NULL;

    DOUT(L"SrvrInPlace::DeactivateInPlace (returning)\r\n");
    return NOERROR; // we never fail this function
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ActivateUI, public
//
//  Synopsis:   Notifies container of U.I. activation and installs our
//              U.I. elements.
//
//  Returns:    Success if our container granted permission to U.I. activate.
//
//  Notes:      Installing our U.I. (border toolbars, floating palettes,
//              adornments) is accomplished via a virtual call to InstallUI.
//
//---------------------------------------------------------------

HRESULT
SrvrInPlace::ActivateUI(void)
{
    DOUT(L"SrvrInPlace::ActivateUI\r\n");

    HRESULT hr;
    if (OK(hr = _pInPlaceSite->OnUIActivate()))
    {
        InstallUI();

        if(!GetChildActivating() && !IsDeactivating())
            ReflectState(TRUE);
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::DeactivateUI, public
//
//  Synopsis:   Removes any U.I. we have installed and notifies our container
//              that we are no longer U.I. active
//
//  Returns:    Success except for catastrophic circumstances
//
//  Notes:      This method "undoes" everything done in ActivateUI.
//              U.I. elements are removed via a virtual call to RemoveUI
//
//---------------------------------------------------------------

HRESULT
SrvrInPlace::DeactivateUI(void)
{
    DOUT(L"SrvrInPlace::DeactivateUI\r\n");

    //
    // The following prevents some nasty recursion cases, in which the call
    // bellow to OnUIDeactivate get's us back in to the same transition we
    // are in now...
    //
    _pCtrl->SetState(OS_INPLACE);

    // remove any UI that is up and notify our container that we have deactivated
    RemoveUI();

    DOUT(L"SrvrInPlace::DeactivateUI calling OnUIDeactivate(FALSE)\r\n");

    _pInPlaceSite->OnUIDeactivate(FALSE);
    ReflectState(FALSE);

    //REVIEW: we should return TRUE if we add Undo capability.
    DOUT(L"SrvrInPlace::DeactivateUI (returning)\r\n");

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::InstallUI, public
//
//  Synopsis:   Installs previously created U.I. so it is displayed to
//              the user.
//
//  Notes:      This method will call the InstallFrameUI and InstallDocUI
//              methods to install those U.I. elements, respectively.
//
//---------------------------------------------------------------

void
SrvrInPlace::InstallUI(void)
{
    DOUT(L"SrvrInPlace::InstallUI\r\n");

    if(!_fChildActivating && !_fDeactivating)
    {
        _pFrame->SetActiveObject((LPOLEINPLACEACTIVEOBJECT)this,
                            _pClass->_szUserClassType[USERCLASSTYPE_SHORT]);
        InstallFrameUI();
        InstallDocUI();
        _fUIDown = FALSE;
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::RemoveUI, public
//
//  Synopsis:   Removes previously installed U.I. so it is hidden from the
//              the user.
//
//  Notes:      This method "undoes" everything done in InstallUI.  It calls
//              the RemoveFrameUI and RemoveDocUI methods.
//
//---------------------------------------------------------------

void
SrvrInPlace::RemoveUI(void)
{
    DOUT(L"SrvrInPlace::RemoveUI\r\n");

    if(!_fUIDown)
    {
        _fUIDown = TRUE;

        ClearSelection();
        RemoveDocUI();
        RemoveFrameUI();
        _pFrame->SetActiveObject(NULL, NULL);
    }
}


#ifdef DOCGEN  // documentation for pure virtual function
//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::AttachWin, public
//
//  Synopsis:   Attaches the child in-place window
//              to the given parent.
//
//  Arguments:  [hwndParent] -- parent window for child
//
//  Returns:    HWND of attached window
//
//  Notes:      All servers must override this method.
//
//---------------------------------------------------------------

HWND SrvrInPlace::AttachWin(HWND hwndParent) {}
#endif //DOCGEN

//+--------------------------------------------------------------
//
//  Member:     SrvrInPlace::DetachWin, public
//
//  Synopsis:   Detaches the child's in-place
//              window from the current parent.
//
//  Arguments:  [hwndParent] -- parent window for child
//
//  Notes:      This destroys the _hwnd of the server.
//              If the derived class does anything
//              other than create a Window on AttachWin,
//              it must over-ride this function.
//              If the derived class destroys the window
//              on detach, it must set _hwnd = NULL
//
//---------------------------------------------------------------
void
SrvrInPlace::DetachWin()
{
    DOUT(L"SrvrInPlace::DetachWin\r\n");

    Assert(_hwnd != NULL && IsWindow(_hwnd));
    DestroyWindow(_hwnd);
    _hwnd = NULL;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::CreateUI, protected
//
//  Synopsis:   Creates all U.I. elements
//
//  Notes:      This method uses information in the class descriptor
//              to create a merged menu and OLE menu descriptor.
//              Servers that have additional U.I. should override
//              this method, but can call the base class to do the
//              standard menu processing.
//
// IMPORTANT:   The derived class is responsible for having
//              settup the _hmenu member prior to calling
//              this code.
//
//---------------------------------------------------------------

void
SrvrInPlace::CreateUI(void)
{
    DOUT(L"SrvrInPlace::CreateUI\r\n");

    Assert(_hmenuShared == NULL);

    _fUIDown = TRUE;

    if (_hmenu != NULL)
    {
        // create an empty menu and ask application to insert its menus
        if ((_hmenuShared = CreateMenu()) != NULL)
        {
            // get a copy of our menu-group widths and perform the merge
            _mgw = _pClass->_mgw;
            if (OK(_pFrame->InsertMenus(_hmenuShared, &_mgw)))
            {
                // insert our own menus and create a descriptor for
                // the whole mess
                if (OK(InsertServerMenus(_hmenuShared, _hmenu, &_mgw)))
                    _hOleMenu = OleCreateMenuDescriptor(_hmenuShared, &_mgw);
            }
        }
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::DestroyUI, protected
//
//  Synopsis:   Destroys U.I. elements
//
//  Notes:      This method "undoes" everything that was done in
//              CreateUI -- destroys the shared menu and OLE menu
//              descriptor.  If a server overrides CreateUI then it
//              should also override this method.
//
//---------------------------------------------------------------

void
SrvrInPlace::DestroyUI(void)
{
    DOUT(L"SrvrInPlace::DestroyUI\r\n");

    if (_hmenuShared != NULL)
    {
        OleDestroyMenuDescriptor(_hOleMenu);
        RemoveServerMenus(_hmenuShared, &_mgw);
        _pFrame->RemoveMenus(_hmenuShared);
        DestroyMenu(_hmenuShared);
        _hmenuShared = NULL;
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::InstallFrameUI, protected
//
//  Synopsis:   Installs the U.I. elements on the frame window
//
//  Notes:      This method uses IOleInPlaceFrame::SetMenu to install
//              the shared menu constructed in CreateUI.  It also notifies
//              the frame that we are the active object.
//              Servers that have additional frame adornments should
//              override this method.
//              This method is called by the InstallUI method and
//              on document window activation for when we are in a MDI
//              an application.
//
//---------------------------------------------------------------

void
SrvrInPlace::InstallFrameUI(void)
{
    DOUT(L"SrvrInPlace::InstallFrameUI\r\n");

    _pFrame->SetMenu(_hmenuShared, _hOleMenu, _hwnd);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::RemoveFrameUI, protected
//
//  Synopsis:   Removes the U.I. elements on the frame window
//
//  Notes:      This method "undoes" everything that was done in
//              InstallFrameUI -- it removes the shared menu from
//              the frame.
//              Servers that override the InstallFrameUI method will
//              also want to override this method.
//              This method is call by the RemoveUI method and on
//              document window deactivation for MDI-application purposes.
//
//---------------------------------------------------------------

void
SrvrInPlace::RemoveFrameUI(void)
{
    DOUT(L"SrvrInPlace::RemoveFrameUI\r\n");

    _pFrame->SetMenu(NULL, NULL, _hwnd);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::InstallDocUI, protected
//
//  Synopsis:   Installs the U.I. elements on the document window
//
//  Notes:      This method notifies the document window that we are
//              the active object.  Otherwise, there are no standard U.I. elements
//              installed on the document window.
//              Servers that have document window tools should override this
//              method.
//
//---------------------------------------------------------------

void
SrvrInPlace::InstallDocUI(void)
{
    DOUT(L"SrvrInPlace::InstallDocUI\r\n");

    if (_pDoc != NULL)
    {
        DOUT(L"SrvrInPlace::InstallDocUI (_pDoc != NULL)\r\n");
        _pDoc->SetActiveObject((LPOLEINPLACEACTIVEOBJECT)this,
                                _pClass->_szUserClassType[USERCLASSTYPE_SHORT]);
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::RemoveDocUI, protected
//
//  Synopsis:   Removes the U.I. elements from the document window.
//
//  Notes:      This method "undoes" everything done in the InstallDocUI
//              method.
//              Servers that override the InstallDocUI method should
//              also override this method.
//
//---------------------------------------------------------------

void
SrvrInPlace::RemoveDocUI(void)
{
    DOUT(L"SrvrInPlace::RemoveDocUI\r\n");

    if (_pDoc != NULL)
    {
        _pDoc->SetActiveObject(NULL, NULL);
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ClearSelection
//
//  Synopsis:   Removes any selection because we have lost ownership of the U.I.
//
//  Notes:      When our container or an embedding steals the right to put
//              up the U.I. then we should remove any selection to avoid
//              confusing the user.
//
//---------------------------------------------------------------

void
SrvrInPlace::ClearSelection(void)
{
    DOUT(L"SrvrInPlace::ClearSelection\r\n");
}


//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::SetFocus, public
//
//  Synopsis:   Overide in derived if focus-window != _hwnd
//
//---------------------------------------------------------------

void
SrvrInPlace::SetFocus(HWND hwnd)
{
    ::SetFocus(hwnd);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::GetWindow, public
//
//  Synopsis:   Method of IOleWindow interface
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::GetWindow(HWND FAR* lphwnd)
{
    if (lphwnd == NULL)
    {
        DOUT(L"SrvrInPlace::GetWindow E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *lphwnd = _hwnd;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ContextSensitiveHelp, public
//
//  Synopsis:   Method of IOleWindow interface
//
//  Notes:      This method sets or clears the _fCSHelpMode
//              member flag.  The window procedure needs to pay
//              attention to the value of this flag in implementing
//              context-sensitive help.
//
//              We never fail!
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::ContextSensitiveHelp(BOOL fEnterMode)
{
    DOUT(L"SrvrInPlace::ContextSensitiveHelp\r\n");

    _fCSHelpMode = fEnterMode;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::InPlaceDeactivate, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method transitions the object to the loaded state
//              if the object is in the InPlace or U.I. active state.
//
//              We never fail!
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::InPlaceDeactivate(void)
{
    DOUT(L"SrvrInPlace::InPlaceDeactivate\r\n");

    if (_pCtrl->State() == OS_INPLACE || _pCtrl->State() == OS_UIACTIVE)
    {
        _pCtrl->TransitionTo(OS_LOADED);
    }
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::UIDeactivate, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      The method transitions the object to the in-place state
//              if the object is in U.I. active state.
//
//              We never fail!
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::UIDeactivate(void)
{
    DOUT(L"SrvrInPlace::UIDeactivate\r\n");

    if (_pCtrl->State() == OS_UIACTIVE)
    {
        _fDeactivating = TRUE;
        _pCtrl->TransitionTo(OS_INPLACE);
        _fDeactivating = FALSE;
    }
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::SetObjectRects, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method does a Move window on the child
//              window to put it in its new position.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::SetObjectRects(LPCRECT lprcPos, LPCRECT lprcVisRect)
{
    DOUT(L"SrvrInPlace::SetObjectRects\r\n");

    if (lprcPos == NULL || lprcVisRect == NULL)
    {
        DOUT(L"SrvrInPlace::SetObjectRects E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    _fClientResize = TRUE;  //indicate that we are being resized by client

    //
    // calculate and do the new child window positioning
    //
    RECT rc = *lprcPos;
    if(_pCtrl->IsIPBEnabled())
    {
        _IPB.SetSize(_hwnd, rc);
    }
    else
    {
        SetWindowPos( _hwnd, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOZORDER);
    }

    //
    // update our border rect (child window coordinates)
    //
    _rcFrame.right = rc.right - rc.left;
    _rcFrame.bottom = rc.bottom - rc.top;

    //
    // update our "native" extent
    //
    SIZEL sizel = { HimetricFromHPix(_rcFrame.right - _rcFrame.left),
            HimetricFromVPix(_rcFrame.bottom - _rcFrame.top) };

    _pCtrl->SetExtent(DVASPECT_CONTENT, &sizel);

    _fClientResize = FALSE; // indicate that client resize is over

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ReactivateAndUndo, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method returns E_FAIL.  If the server wishes
//              to support undo it should override this method.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::ReactivateAndUndo(void)
{
    DOUT(L"SrvrInPlace::ReactivateAndUndo E_NOTIMPL\r\n");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::TranslateAccelerator, public
//
//  Synopsis:   Method of IOleInPlaceActiveObject interface
//
//  Notes:      This method translates the message according
//              to the accelerator table in the class descriptor
//              structure.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::TranslateAccelerator(LPMSG lpmsg)
{
    //
    // translate the message via the SrvrInPlace accelerator table
    //
    if (_pClass->_haccel  &&
            ::TranslateAccelerator(_hwnd, _pClass->_haccel, lpmsg))
    {
        return NOERROR;
    }

    return S_FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::OnFrameWindowActivate, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method changes the color of our border shading
//              depending on whether our frame window is activating
//              or deactivating.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::OnFrameWindowActivate(BOOL fActivate)
{
    DOUT(L"SrvrInPlace::OnFrameWindowActivate\r\n");

    if(_pCtrl->IsIPBEnabled())
        _IPB.SetParentActive(fActivate);

    if(fActivate && _hwnd && (_pCtrl->State() != OS_OPEN))
        SetFocus(_hwnd);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::OnDocWindowActivate, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method will install or remove the frame
//              U.I. elements using the InstallFrameUI or RemoveFrameUI
//              methods.  This is to properly handle the MDI application
//              case.  It also updates our shading color.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::OnDocWindowActivate(BOOL fActivate)
{
    DOUT(L"SrvrInPlace::OnDocWindowActivate\r\n");

    //if (fActivate && _fUIDown)
    if (fActivate)
    {
        InstallFrameUI();
        SetFocus(_hwnd);
    }
    else if(!fActivate)
        RemoveFrameUI();

    if(_pCtrl->IsIPBEnabled())
        _IPB.SetParentActive(fActivate);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ResizeBorder, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      There are no standard border adornments so we do
//              nothing in this method.  Servers that have additional
//              U.I. elements that are installed on the frame or
//              document windows should override this method.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::ResizeBorder(LPCRECT lprc,
                        LPOLEINPLACEUIWINDOW pUIWindow,
                        BOOL fFrameWindow)
{
    DOUT(L"SrvrInPlace::ResizeBorder\r\n");

    // we do not install any tools on our frame or document windows.
    //REVIEW:  This must be implemented if we do implement a frame or document
    //REVIEW:  toolbar.

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::EnableModeless, public
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      If we are a DLL and hence don't have a separate
//              message pump we can ignore this call and simply
//              return NOERROR.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrInPlace::EnableModeless(BOOL fEnable)
{
    DOUT(L"SrvrInPlace::EnableModeless\r\n");

    return NOERROR;
}
