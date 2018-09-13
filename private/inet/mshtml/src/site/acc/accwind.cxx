//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccWind.Cxx
//
//  Contents:   Accessible Window object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif


extern DYNLIB g_dynlibOLEACC;

MtDefine(CAccWindowGetEnumerator_aryVariants_pv, Locals, "CAccWindow::GetEnumerator aryVariants::_pv")

//-----------------------------------------------------------------------
//  CAccWindow::CAccWindow()
//
//  DESCRIPTION:
//      Contructor. 
//
//  PARAMETERS:
//      pDocParent  :   The CAccWindow contains a pointer to the document 
//                      that it is related to. This parameter contains the
//                      address of the CDoc class.
// ----------------------------------------------------------------------
CAccWindow::CAccWindow( CDoc* pDocInner ) 
: CAccBase( ), _elemBegin( pDocInner ), _elemEnd( pDocInner )
{
    Assert( pDocInner );
    
    _pDoc = pDocInner;

    SetRole( ROLE_SYSTEM_CLIENT );
}

//----------------------------------------------------------------------------
//  CAccWindow::~CAccWindow()
//  
//  DESCRIPTION:
//      Destructor
//----------------------------------------------------------------------------
CAccWindow::~CAccWindow()
{
    Assert( _pDoc );

    //reset the document object so it can answer later requests 
    //for acc. objects. This is important if the client releases the
    //window without unloading the document, and later asks for another
    //acc object on the window. Since the CDoc has no notion of 
    //reference counting the acc. object, the object has to do that
    _pDoc->_pAccWindow = NULL;  

    _pDoc = NULL;
}


//+---------------------------------------------------------------------------
//  PrivateAddRef
//  
//  DESCRIPTION
//      We overwrite the CBase implementation to be able to delegate the call 
//      to the document that we are connected to.
//----------------------------------------------------------------------------
ULONG
CAccWindow::PrivateAddRef()
{
    Assert( _pDoc );
    return _pDoc->PrivateAddRef();
}

//+---------------------------------------------------------------------------
//  PrivateRelease

//  DESCRIPTION
//      We overwrite the CBase implementation to be able to delegate the call 
//      to the document that we are connected to.
//----------------------------------------------------------------------------
ULONG
CAccWindow::PrivateRelease()
{
    Assert( _pDoc );
    return _pDoc->PrivateRelease();
}


//----------------------------------------------------------------------------
//  get_accParent
//  
//  DESCRIPTION:
//      Returns a pointer to the parent accessible object
//      
//      If there is no parent, then set the out parameter to NULL and 
//      return S_FALSE
//
//  PARAMETERS:
//      ppdispParent    :   Address of the variable to receive the parent
//                          accessible object pointer.
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accParent(IDispatch ** ppdispParent)
{
    HRESULT hr;
    HWND    hWndParent = NULL;

    // check and reset the out parameter
    if ( !ppdispParent )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispParent = NULL;

    // Is this the object that represents the root document?
    if ( _pDoc->IsRootDoc() )
    {
        //get the window handle for this document's window.
        hWndParent = _pDoc->GetHWND();
        if ( !hWndParent )
            goto Error;
        
        //get the parent window.
        hWndParent = GetParent( hWndParent );
        if ( !hWndParent )
            goto Error;

        //get the accessible object for the parent window.
        hr = AccObjFromWindow( hWndParent, (void **)ppdispParent );
    }
    else
        goto Error;
                                                    
Cleanup:
    RRETURN(hr );

Error:
    RRETURN( E_FAIL );
}

//----------------------------------------------------------------------------
//  get_accChildCount
//  
//  DESCRIPTION:
//      Get the child count for the accessible window object. This count is 
//      always one.
//      Returns E_FAIL if there is a problem getting to the child.
//      
//  PARAMETERS:
//      pChildCount :   Address of the long integer to receive the child count
//
//  RETURNS:
//      S_OK  | E_FAIL | E_POINTER
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accChildCount(long* pChildCount)
{
    HRESULT     hr;
    CAccBase *  pAccChild = NULL;

    if ( !pChildCount )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // the following function will initialize the child's acc object,
    // in addition to making sure that we have the only child of the window
    hr = THR( GetClientAccObj( &pAccChild) );
    
    // if no error, there is always one child for a window, set and return
    *pChildCount = (hr) ? 0 : 1;

Cleanup:    
    RRETURN (hr );

}

//----------------------------------------------------------------------------
//  get_accChild
//  
//  DESCRIPTION:
//      Retrieves the one and only possible child for the acc. window object
//
//  PARAMETERS:
//      varChild    :   VARIANT that contains the child index information
//      ppdispChild :   pointer to the dispatch pointer to receive the child
//                      object interface pointer
//  RETURNS:
//      E_POINTER | S_OK | E_INVALIDARG | E_FAIL        
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
    HRESULT     hr;
    CAccBase *  pAccChild = NULL;
    
    // validate out parameter
    if ( !ppdispChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppdispChild = NULL;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;    

    // get the only child's accessible object
    hr = THR( GetClientAccObj( &pAccChild) );
    if ( hr )
        goto Cleanup;

    Assert( pAccChild );

    //increment reference count before giving out.
    pAccChild->AddRef();

    // TODO: this should eventually be a  QI (tearoffs)
    *ppdispChild = (IDispatch *) pAccChild;
    
Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  DESCRIPTION:
//        
//      Returns the URL of the document for IE3 compatibility.
//
//
//  PARAMETERS:
//      pbstrName   :   address of the pointer to receive the URL BSTR
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accName( VARIANT varChild, BSTR* pbstrName)
{
    HRESULT     hr;
    CAccBase *  pAccChild = NULL;

    // validate out parameter
     if ( !pbstrName )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrName = NULL;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        hr = THR( _pDoc->get_URL(pbstrName) );
    }
    else if ( V_I4(&varChild) == 1 )
    {   
        // get the window's only child's acc. object
        hr = THR( GetClientAccObj( &pAccChild ) );
        if ( hr )   
            goto Cleanup;

        //set the parameters and delegate the call
        //to the child
        V_I4( &varChild ) = CHILDID_SELF;
        
        hr = THR( pAccChild->get_accName( varChild, pbstrName ) );
    }
    else
    {
        // the child id passed is invalid
        hr = E_INVALIDARG;
    }
    
Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accValue
//  
//  DESCRIPTION:
//      NOT IMPLEMENTED BY THIS OBJECT
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accValue(VARIANT varChild, BSTR* pbstrValue)
{
    return ( E_NOTIMPL );
}

//----------------------------------------------------------------------------
//  get_accDescription
//
//  DESCRIPTION :   
//      Returns the description text for the Window Acc. object. 
//
//  PARAMETERS:
//      pbstrDescription    :   pointer to the BSTR to receive data.
//
//  RETURNS:
//      S_OK | E_POINTER | E_OUTOFMEMORY | E_FAIL | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accDescription( VARIANT varChild, BSTR* pbstrDescription)
{
    HRESULT     hr;
    CAccBase *  pAccChild = NULL;

    // validate out parameter
     if ( !pbstrDescription )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
//BUGBUG: [FerhanE]
//          This is here for backward compatibility. We may want to change the
//          contents of this BSTR.
// Localization ?
        *pbstrDescription = SysAllocString( _T("MSAAHTML Registered Handler") );

        if ( !*pbstrDescription )
            hr = E_OUTOFMEMORY;

    }
    else if ( V_I4(&varChild) == 1 )
    {   
        // get the acc. window's one and only child
        hr = THR( GetClientAccObj( &pAccChild ) );
        if ( hr )   
            goto Cleanup;

        //set the parameters and delegate the call
        //to the child
        V_I4( &varChild ) = CHILDID_SELF;
        
        hr = THR( pAccChild->get_accDescription( varChild, pbstrDescription ) );
    }
    else
    {
        // the child id passed is invalid
        hr = E_INVALIDARG;
    }

Cleanup:
    RRETURN( hr );    
}

//-----------------------------------------------------------------------
//  get_accRole()
//
//  DESCRIPTION:
//          Returns the accessibility role of the object.
//
//  PARAMETERS:
//
//      varChild    :   Variant that contains the child information
//      pvarRole    :   Address of the variant to receive the role information
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT         hr ;      
    long            lRetRole = 0;
    CAccBase *      pAccChild = NULL;

    // validate the out parameter
    if ( !pvarRole )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // clear the out parameter
    V_VT( pvarRole ) = VT_EMPTY;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        lRetRole = GetRole();   //this acc. objects role
    }
    else if ( V_I4(&varChild) == 1 )
    {
        // get the only acc child possible and delegate
        hr = THR( GetClientAccObj( &pAccChild ) );
        if ( hr )   
            goto Cleanup;

        //call child's implementation
        lRetRole = pAccChild->GetRole();
    }
       
    if ( hr == S_OK )
    {
        // pack role into out parameter
        V_VT( pvarRole ) = VT_I4;
        V_I4( pvarRole ) = lRetRole;
    }
    
Cleanup:
    RRETURN( hr );
}

// ----------------------------------------------------------------------
//  DESCRIPTION:
//          Returns the accessibility state of the object.
//
//  PARAMETERS:
//
//      pvarState:   Address of the variant to receive the state information
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK | E_POINTER | E_FAIL
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accState( VARIANT varChild, VARIANT *pvarState)
{
    HRESULT     hr;
    CAccBase *  pAccChild = NULL;

    // validate the out parameter
    if ( !pvarState )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //reset the out parameter.
    V_VT( pvarState ) = VT_EMPTY;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        V_VT( pvarState ) = VT_I4;

        //always focusable
        V_I4( pvarState ) = STATE_SYSTEM_FOCUSABLE;  

        //if the client site has the focus, 
        if (_pDoc->GetFocus())
            V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED; 
    }
    else if ( V_I4(&varChild) == 1 )
    {   
        // get the only acc child possible and delegate
        hr = THR( GetClientAccObj( &pAccChild ) );
        if ( hr )   
            goto Cleanup;

        //set the parameters and delegate the call
        //to the child
        V_I4( &varChild ) = CHILDID_SELF;
        
        hr = THR( pAccChild->get_accState( varChild, pvarState ) );
    }
    else
    {
        // the child id passed is invalid
        hr = E_INVALIDARG;
    }

Cleanup:
    RRETURN( hr );    
}

//----------------------------------------------------------------------------
//  get_accKeyboardShortcut
//
//  DESCRIPTION:
//      NOT IMPLEMENTED ON THIS OBJECT
//----------------------------------------------------------------------------
STDMETHODIMP
CAccWindow::get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//  get_accFocus
//
//  DESCRIPTION:
//      Returns the object that has the focus, inside the document that the 
//      window hosts.
//  
//  PARAMETERS:
//      pvarFocusChild  :   pointer to the VARIANT to receive the child that has
//                          the focus.
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accFocus(VARIANT * pvarFocusChild)
{
    HRESULT     hr = S_OK;
    CElement *  pElemFocus = NULL;
    CAccBase *  pAccChild = NULL;
    
    if ( !pvarFocusChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarFocusChild ) = VT_EMPTY;

    pElemFocus = GetAccDoc()->_pElemCurrent;

    //if no element has the focus, then return S_OK,
    //and VT_EMPTY in the out parameter
    if ( !pElemFocus )
        goto Cleanup;

    //an element has the focus, we have to return its
    //accessible object.
    pAccChild = GetAccObjOfElement( _pDoc->_pElemCurrent );

    if ( !pAccChild )
    {   
        hr = E_FAIL;
        goto Cleanup;
    }

    V_VT( pvarFocusChild ) = VT_DISPATCH;
    V_DISPATCH( pvarFocusChild ) = pAccChild;

    //increment reference count before handing the pointer out.
    pAccChild->AddRef();
       
Cleanup:
    RRETURN( hr );
}


STDMETHODIMP 
CAccWindow::get_accSelection(VARIANT * pvarSelectedChildren)
{
    return E_NOTIMPL;
}

                                                    
//----------------------------------------------------------------------------
//  get_accDefaultAction
//
//  DESCRIPTION:
//      NOT IMPLEMENTED ON THIS OBJECT
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//  accSelect
//  
//  DESCRIPTION: IAccessible::accSelect implementation for the accessible window
//                  object. Always sets the focus on the element client of the 
//                  CDoc.
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::accSelect( long flagsSel, VARIANT varChild)
{
    HRESULT     hr;
    CElement *  pClient = NULL;

    if ( flagsSel != SELFLAG_TAKEFOCUS )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // unpack varChild
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    // Window can only answer for itself and its element client.
    if ((V_I4( &varChild ) != CHILDID_SELF ) && 
        (V_I4( &varChild ) != 1) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    // always focus on the element client.
    pClient = _pDoc->GetPrimaryElementClient( );
    if ( !pClient )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( pClient->focus() );

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  accLocation
//  
//  DESCRIPTION :   
//      The window is the top level accessibility object and its location is 
//      relative to the top left corner of the screen. This method gets the
//      window that sites the document and gets its coordinates using a Win32
//      call. 
//  
//      The pane and the window objects return the same location.
//      
//  PARAMETERS:
//      pxLeft
//      pyTop       : (x,y) Coordinates of the left top window
//      pcxWidth    : Width of the window
//      pcxHeight   : Height of the window
//      varChild    : Child ID.
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::accLocation(   long* pxLeft, long* pyTop, 
                           long* pcxWidth, long* pcyHeight, 
                           VARIANT varChild)
{
    HRESULT hr;
    RECT    rectPos = { 0 };

//BUGBUG: [FerhanE]
//      I could not find any indication that a client may want to
//      get partial location information and we should be flexible 
//      in respect to that. For simplicity and perf reasons, I am
//      assuming that all four parameters will be provided.
//      This may change, if we decide to be more flexible and let 
//      an arbitrary out parameter to be ignored.
//
    Assert( pxLeft && pyTop && pcxWidth && pcyHeight );

    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;

    // unpack varChild
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    // Since the element client also calls this method, we can accept 
    // both CHILDID_SELF and 1 as valid child id values.
    if ((V_I4( &varChild ) != CHILDID_SELF ) && 
        (V_I4( &varChild ) != 1 ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // for the window, the location is relative to the screen's coordinates
    // the same location is used for the pane (body/frameset) that it contains too.
    // If the window is not inplave activated, ( frames ) then the _pDoc->GetHWND
    // will return NULL. 
    if ( !_pDoc->GetHWND() || 
         !::GetWindowRect( _pDoc->GetHWND( ), &rectPos ) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    *pxLeft = rectPos.left;
    *pyTop =  rectPos.top;
    *pcxWidth = rectPos.right - rectPos.left;
    *pcyHeight = rectPos.bottom - rectPos.top;
    
Cleanup:    
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  accNavigate
//  
//  DESCRIPTION:
//      Provides navigation for children. Since the window accessible object
//      can have only one child, NAVDIR_NEXT and NAVDIR_PREVIOUS are not valid.
//      The valid navigation directions are NAVDIR_FIRST and NAVDIR_LAST and
//      they both return the same accessible object, which is connected to the
//      element client of the document.
//
//  PARAMETERS:
//      navDir      :   Navigation direction information ( first, last, next, previous)
//      varStart    :   Starting child id.
//      pvarEndUpAt :   Destionation child ID
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarEndUpAt ) = VT_EMPTY;

    // unpack varChild, and validate the child id against child array limits.
    hr = THR(ValidateChildID(&varStart));
    if ( hr )
        goto Cleanup;    

    switch ( navDir )
    {
        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
            // An acc object can only return its own first and last children.
            if ( V_I4(&varStart) != CHILDID_SELF )
            {
                hr = E_INVALIDARG;
                break;
            }
            
            //Retrieve the accessible child object
            hr = THR( GetClientAccObj( &pAccChild ) );
            if ( hr ) 
                break;

            Assert( pAccChild );

            //set the return parameter to our only child
            V_VT( pvarEndUpAt ) = VT_DISPATCH;
            V_DISPATCH( pvarEndUpAt ) = DYNCAST( IDispatch, pAccChild);

            //increment ref count.
            pAccChild->AddRef();
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_NEXT:
            // We know that we are not a frame.
            // Since there is only one child and we can not go 
            // left or right from that child, it does not matter what
            // values we get. We will always return S_FALSE as the return
            // value, complying with the spec in this area.
            hr = S_FALSE;

            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//
//  DESCRIPTION :   
//
//      Determine if the hit was in this object's client area.
//
//  PARAMETERS:
//      xLeft
//      yTop    :   (x,y) coordinates of the point relative to the top left
//                          of the screen.
//      pvarChildAtPoint:   pointer to VARIANT to receive the information about
//                          the child that contains the point. 
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{   
    HRESULT     hr = S_OK;
    RECT        rectPos = {0};
    CAccBase *  pAccChild = NULL;
    
    if (!pvarChildAtPoint)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // reset out parameter, this is also the proper return value if the hit
    // point was not inside this window
    V_VT(pvarChildAtPoint) = VT_EMPTY;
    
    //for the window, the location is relative to the screen's coordinates
    //the same location is used for the pane (body/frameset) that it contains too.
    //_pDoc->GetHWND() may return NULL, if the window is not inplace activated.
    if ( !_pDoc->GetHWND() || 
         !::GetWindowRect( _pDoc->GetHWND( ), &rectPos ) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    //if (x, y) is inside us, 
    if ( ((rectPos.left < xLeft) && (rectPos.right > xLeft)) && 
         ((rectPos.top  < yTop) && (rectPos.bottom > yTop)) )
    {
        hr = THR( GetClientAccObj( &pAccChild ) );
        if ( hr )
            goto Cleanup;

        //set the out parameter and return
        V_VT( pvarChildAtPoint ) = VT_DISPATCH;
        V_DISPATCH( pvarChildAtPoint ) = pAccChild;

        pAccChild->AddRef();    //increment the reference count
    }

    //else, the out parameter contains VT_EMPTY and return code is S_OK
    
Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  accDoDefaultAction
//
//  DESCRIPTION:
//      NOT IMPLEMENTED ON THIS OBJECT
//----------------------------------------------------------------------------
STDMETHODIMP
CAccWindow::accDoDefaultAction( VARIANT varChild )
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//  GetClientAccObj
//
//  DESCRIPTION:
//      Gets the accessible object for the element client of the window.
//----------------------------------------------------------------------------
HRESULT 
CAccWindow::GetClientAccObj( CAccBase ** ppAccChild )
{
    HRESULT     hr = S_OK;
    ELEMENT_TAG elemTag;
    CElement *  pClient = NULL;

    Assert( ppAccChild );

    // The window object has only one child and always has that child.
    // Get the current accdocument object. Either the body or the frameset tag
    pClient = _pDoc->GetPrimaryElementClient( );
    if ( !pClient )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // We must have either a frameset or a body tag.
    elemTag = pClient->Tag();

    if ((elemTag != ETAG_FRAMESET) && 
        (elemTag != ETAG_BODY) )
    {
        // something weird happened, and the client is 
        // neither a frameset nor a body
        Assert(0 && "strange tree: window with no body or frameset");
        hr = E_FAIL;
        goto Cleanup;
    }

    // Get the accessible object for the element.
    *ppAccChild = GetAccObjOfElement( pClient );

    // make sure that we have something going back.
    Assert( *ppAccChild );

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//  put_accValue
//  
//  DESCRIPTION :   
//          Sets the value property of the accessible object. This method is not
//          supported on accessible objects that are derived from the CAccWindow
//
//  PARAMETERS:
//      varChild     :   VARIANT containing the child ID
//      bstrValue    :   value bstr
//
//  RETURNS:
//      E_NOTIMPL
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccWindow::put_accValue(VARIANT varChild, BSTR bstrValue)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//  AccObjFromWinwow
//
//  DESCRIPTION:
//      Helper function that wraps the code that is needed to call the OLEACC
//      API AccessibleObjectFromWindow
//
//  PARAMETERS:
//      hWnd    :   Window handle to get the acc. object for.
//      ppAccObj:   Address of the pointer to receive the address of the acc.obj.
//----------------------------------------------------------------------------
HRESULT
CAccWindow::AccObjFromWindow( HWND hWnd, void ** ppAccObj )
{
    HRESULT hr;
    
    static DYNPROC s_dynprocAccObjFromWindow =
            { NULL, &g_dynlibOLEACC, "AccessibleObjectFromWindow" };

    // Load up the AccessibleObjectFromWindow pointer.
    hr = THR(LoadProcedure(&s_dynprocAccObjFromWindow));
    if (hr)
        goto Cleanup;

    hr = THR ((*(LRESULT (APIENTRY *)(HWND, DWORD, REFIID, void**))
                s_dynprocAccObjFromWindow.pfn)( hWnd, OBJID_CLIENT, 
                                                IID_IAccessible, ppAccObj) );
Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  GetEnumerator
//
//  DESCRIPTION:    Fills the return parameter with the address of the 
//                  implementation for the IEnumVARIANT.
//                  Since the window object has only one child and that child
//                  is the element client of the document, the list contains
//                  only one item.
//                  The return value can not be E_NOINTERFACE, since the window
//                  always has a single child.
//  RETURNS:
//          S_OK | E_POINTER | E_OUTOFMEMORY
//----------------------------------------------------------------------------
HRESULT
CAccWindow::GetEnumerator( IEnumVARIANT** ppEnum)
{
    HRESULT                     hr;
    CDataAry <VARIANT> *        pary = NULL;
    CAccBase *                  pAccChild = NULL;
    VARIANT                     varTmp;
    IDispatch *                 pDispTmp;

    VariantInit( &varTmp );

    if ( !ppEnum )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;
    
    pary = new(Mt(CAccWindowGetEnumerator_aryVariants_pv)) 
                    CDataAry<VARIANT>(Mt(CAccWindowGetEnumerator_aryVariants_pv));
    if ( !pary )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // This is the window, so it has ony one child.
    // Get the child, and append it to the child list.
    hr = THR( GetClientAccObj( &pAccChild ));
    if ( hr )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // increment the ref count and do the casting at the same time. 
    hr = THR( pAccChild->QueryInterface( IID_IDispatch, (void**)&pDispTmp));
    if ( hr )
        goto Cleanup;

    Assert( pDispTmp );    // we must have our value

    V_DISPATCH(&varTmp) = pDispTmp;
    V_VT( &varTmp ) = VT_DISPATCH;      // set the type.

    // append the variant to the list.
    hr = THR(pary->EnsureSize(1));
    if (hr)
        goto Cleanup;

    hr = THR( pary->AppendIndirect( &varTmp, NULL ) );
    if ( hr )
        goto Cleanup;

    hr = THR(pary->EnumVARIANT(VT_VARIANT, 
                                ppEnum, 
                                FALSE,  // don't copy the array being enumerated use the one we gave
                                TRUE)); // delete enumeration when no one is left to use .

Cleanup:
    RRETURN( hr );
}

STDMETHODIMP 
CAccWindow::PrivateQueryInterface(REFIID riid, void ** ppv)
{
    if ( riid == IID_IOleWindow )
    {
        *ppv = (IOleWindow *)this;
        ((LPUNKNOWN) *ppv)->AddRef();
        return S_OK;
    }
    else
    {
        return super::PrivateQueryInterface(riid,ppv);
    }
}
