//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccAnch.Cxx
//
//  Contents:   Accessible Anchor object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCOBJ_HXX_
#define X_ACCOBJ_HXX_
#include "accobj.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_PLUGINST_HXX_
#define X_PLUGINST_HXX_
#include "pluginst.hxx"
#endif

#ifndef WIN16
extern DYNLIB g_dynlibOLEACC;
#endif // !WIN16


ExternTag(tagAcc);

CAccObject::CAccObject( CElement* pElementParent )
:CAccElement( pElementParent )
{
    //reset the _pAccObject pointer.
    _pAccObject = NULL;

    //initialize the instance variables
    SetRole( ROLE_SYSTEM_CLIENT );
}

CAccObject::~CAccObject()
{
    if ( _pAccObject )
        _pAccObject->Release();
}

//-----------------------------------------------------------------------
//  CAccObject::PrivateQueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CAccBase object implements
//      IDispatch and IAccessible. CAccObject only delegates the IEnumVariant
//      request to the object that it represents.
//
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccObject::PrivateQueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = S_OK;
    
    if ( !ppv )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
  
    if (riid == IID_IEnumVARIANT)
    {
//NOTE: (ferhane)
//          If the object does not support IEnumVariant, then we should
//          return E_NOINTERFACE, since our enumerator can not really know
//          what the object's implementation would be like.

        // if there is an object that is represented, then delegate
        if ( EnsureAccObject()) 
        {
            hr = THR( _pAccObject->QueryInterface( riid, ppv ) );
        }
        else
            hr = E_NOINTERFACE;
    }
    else if (riid == IID_IOleWindow)
    {
        *ppv = (IOleWindow *)this;
        ((LPUNKNOWN) *ppv)->AddRef();
    }
    else
    {
        //Delegate the call to the super;
        hr = THR( super::PrivateQueryInterface( riid, ppv));
        goto Cleanup;
    }

Cleanup:
    RRETURN1( hr, E_NOINTERFACE);
}


//----------------------------------------------------------------------------
//  get_accChildCount
//
//  DESCRIPTION:
//      if the object that is being represented here supports IAccessible,
//      we delegate the call to that object. Otherwise we return 0.
//
//  PARAMETERS:
//      pChildCount :   address of the parameter to receive the child count
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccObject::get_accChildCount(long* pChildCount)
{
    HRESULT hr = S_OK;
    
    if ( !pChildCount )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pChildCount = 0;   //there are no children

    // if the objec that is being represented here supports IAccessible,
    // we delegate the call to that object. Otherwise we return 0.
    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accChildCount( pChildCount ) );
    }

    TraceTag((tagAcc, "CAccObject::get_accChildCount, childcnt=%d hr=%d", 
                *pChildCount, hr));

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  get_accChild()
//
//  DESCRIPTION:
//      if the object that is being represented here supports IAccessible,
//      we delegate the call to that object. Otherwise we return an error, since
//      this tag type can not have any children.
//
//  PARAMETERS:
//      varChild    :   Child information
//      ppdispChild :   Address of the variable to receive the child 
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK | S_FALSE
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccObject::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
    HRESULT      hr = S_FALSE;

    // validate out parameter
    if ( !ppdispChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispChild = NULL;        //reset the return value.

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accChild( varChild, ppdispChild) );
    }


Cleanup:
    TraceTag((tagAcc, "CAccObject::get_accChild, childid=%d requested, hr=0x%x", 
                        V_I4(&varChild),
                        hr));  

    RRETURN1( hr, S_FALSE );    //S_FALSE is valid when there is no children
}

//----------------------------------------------------------------------------
//  accLocation()
//  
//  DESCRIPTION:
//      Returns the coordinates of the element relative to the top left corner 
//      of the client window.
//      To do that, we are getting the CLayout pointer from the element
//      and calling the GetRect() method on that class, using the global coordinate
//      system. This returns the coordinates relative to the top left corner of
//      the screen. 
//      We then convert these screen coordinates to client window coordinates.
//      
//      If the childid is not CHILDID_SELF, then tries to delegate the call to the 
//      object itself, and returns E_NOINTERFACE if the object does not support 
//      IAccessible
//  
//  PARAMETERS:
//        pxLeft    :   Pointers to long integers to receive coordinates of
//        pyTop     :   the rectangle.
//        pcxWidth  :
//        pcyHeight :
//        varChild  :   VARIANT containing child information. 
//
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccObject::accLocation(  long* pxLeft, long* pyTop, 
                          long* pcxWidth, long* pcyHeight, 
                          VARIANT varChild)
{
    HRESULT     hr;
    CRect       rectElement;
   
    // validate out parameter
    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //reset out parameters
    *pxLeft = *pyTop =  *pcxWidth = *pcyHeight = 0;
    
    // unpack varChild, and validate the child id against child array limits.
    hr = THR(ValidateChildID(&varChild));
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        // call super's implementation here..... 
        hr = CAccElement::accLocation( pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
    }
    else 
    {
        if ( EnsureAccObject()) 
        {
            hr = THR( _pAccObject->accLocation( pxLeft, pyTop, pcxWidth, 
                                                pcyHeight, varChild) );
        }
        else
            hr = E_NOTIMPL;
    }

Cleanup:

    TraceTag((tagAcc, "CAccObject::accLocation, childid=%d hr=0x%x", 
                V_I4(&varChild),
                hr));  

    RRETURN1( hr, S_FALSE ); 
}


//----------------------------------------------------------------------------
//  accNavigate
//  
//  DESCRIPTION:
//      Delegate to the object if it implements the IAccessible. Otherwise
//      not implemented.
//      
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT hr = E_NOTIMPL;

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    V_VT( pvarEndUpAt ) = VT_EMPTY;
    
    if ( EnsureAccObject()) 
        hr = THR( _pAccObject->accNavigate(navDir, varStart, pvarEndUpAt) );

Cleanup:
    TraceTag((tagAcc, "CAccObject::accNavigate, start=%d, direction=%d", 
                V_I4(&varStart),
                navDir));  
    RRETURN( hr );
}


//-----------------------------------------------------------------------
//  accHitTest()
//  
//  DESCRIPTION :   Since the window already have checked the coordinates
//                  and decided that the document contains the point, this
//                  function does not do any point checking. 
//                  If the object implements IAccessible, then the call is 
//                  delegated to the object. Otherwise CHILDID_SELF is 
//                  returned.
//                  
//  PARAMETERS  :
//      xLeft, yTop         :   (x,y) coordinates 
//      pvarChildAtPoint    :   VARIANT pointer to receive the acc. obj.
//
//  RETURNS:    
//      S_OK | E_INVALIDARG | 
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccObject::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    HRESULT     hr;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->accHitTest( xLeft, yTop, pvarChildAtPoint));
    }
    else
    {
        if ( !pvarChildAtPoint )
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        V_VT( pvarChildAtPoint ) = VT_I4;
        V_I4( pvarChildAtPoint ) = CHILDID_SELF;
        
        hr = S_OK;
    }

Cleanup:
    TraceTag((tagAcc, "CAccObject::accHitTest, point(%d,%d), hr=0x%x", 
                xLeft, yTop, hr));  

    RRETURN1( hr, S_FALSE );
}    

//----------------------------------------------------------------------------
//  accDoDefaultAction
//  
//  DESCRIPTION:
//
//  PARAMETERS:
//      varChild            :   VARIANT child information
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccObject::accDoDefaultAction(VARIANT varChild)
{   
    HRESULT     hr;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( EnsureAccObject()) 
    {
        hr = THR(_pAccObject->accDoDefaultAction(varChild));
    }
    else
    {
        if ( V_I4(&varChild) == CHILDID_SELF )
        {
            hr = THR( ScrollIn_Focus( _pElement ) );                
        }
        else
            hr = E_NOTIMPL;
    }

Cleanup:
    TraceTag((tagAcc, "CAccObject::accDoDefaultAction, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accName
//  
//  DESCRIPTION:
//      If the object implements IAccessible, then calls that implementation
//      otherwise returns the title
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::get_accName(VARIANT varChild,  BSTR* pbstrName )
{
    HRESULT hr = S_OK;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accName( varChild, pbstrName) ); 
    }

    //
    // The delegation could not answer the query try to find substitution.
    //
    if (hr || !*pbstrName)
    {
        //
        // Try to get the label or the title for this element.
        //
        hr = THR( GetLabelorTitle(pbstrName) );        
    }

Cleanup:
    TraceTag((tagAcc, "CAccObject::get_accName, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accValue
//  
//  DESCRIPTION:
//      Returns the src if the element is an EMBED. Otherwise delegates to the
//      object. If there is no implementation on the object then E_NOINTERFACE
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::get_accValue(VARIANT varChild,  BSTR* pbstrValue )
{
    HRESULT hr = E_NOTIMPL;
    TCHAR * pchString = NULL;

    // validate out parameter
    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrValue = NULL;

    if ( _pElement->Tag() == ETAG_EMBED )
    {
        pchString = (TCHAR *)(DYNCAST( CPluginSite, _pElement ))->GetAAsrc();

        if ( pchString )
        {
            *pbstrValue = SysAllocString( pchString );
            
            if ( !(*pbstrValue) )
                hr = E_OUTOFMEMORY;

            hr = S_OK;
        }
    }
    else 
    {
        if ( EnsureAccObject()) 
            hr = THR( _pAccObject->get_accValue( varChild, pbstrValue) );            
    }

Cleanup:
    TraceTag((tagAcc, "CAccObject::get_accValue, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}


//----------------------------------------------------------------------------
//  get_accDefaultAction
//  
//  DESCRIPTION:
//  If the object supports the IAccessible, the object is called. Otherwise, 
//  teturns the default action for  an object, which is 
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccObject::get_accDefaultAction(VARIANT varChild,  BSTR* pbstrDefaultAction )
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = NULL;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accDefaultAction( varChild, pbstrDefaultAction) );
    }
    else
    {
        // bugbug resource string
        *pbstrDefaultAction = SysAllocString( _T("Select") );

        if ( !(*pbstrDefaultAction) )
            hr = E_OUTOFMEMORY;
    }
   
Cleanup:
    TraceTag((tagAcc, "CAccObject::get_accDefaultAction, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

   RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accState
//  
//  DESCRIPTION:
//      
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccObject::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = _pElement->Doc();

    // validate out parameter
    if ( !pvarState )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accState(varChild, pvarState));
    }
    else
    {
       
        V_I4( pvarState ) = 0;

        if ( _pElement->GetReadyState() != READYSTATE_COMPLETE )
            V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;
        
        if ( IsFocusable(_pElement) )
            V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;
        
        if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus() )
            V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
        
        if ( !_pElement->IsVisible(FALSE) )
            V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;
    }
    
Cleanup:
    TraceTag((tagAcc, "CAccObject::get_accState, childid=%d, state=0x%x, hr=0x%x", 
                V_I4(&varChild), V_I4( pvarState ), hr));  
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::get_accDescription(VARIANT varChild, BSTR * pbstrDescription )
{
    HRESULT hr;

    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accDescription( varChild, pbstrDescription));
    }
    else
    {
        hr = S_OK;
        
        if ( _pElement->Tag() == ETAG_OBJECT )
        {
            *pbstrDescription = SysAllocString( _T("PLUGIN: type=Object") );
        }
        else
        {
            *pbstrDescription = SysAllocString( _T("PLUGIN: type=Embed") );
            
            if ( !(*pbstrDescription) )
                hr = E_OUTOFMEMORY;
        }
    }

Cleanup:
    TraceTag((tagAcc, "CAccObject::get_accDescription, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    HRESULT hr  = E_NOTIMPL;

    if ( EnsureAccObject()) 
        hr = THR( _pAccObject->get_accKeyboardShortcut(varChild, pbstrKeyboardShortcut) );

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccObject::get_accFocus(VARIANT * pvarFocusChild)
{
    HRESULT hr  = E_NOTIMPL;

    if ( EnsureAccObject()) 
        hr = THR( _pAccObject->get_accFocus(pvarFocusChild) );

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::get_accSelection(VARIANT * pvarSelectedChildren)
{
    HRESULT hr  = E_NOTIMPL;

    if ( EnsureAccObject()) 
        hr = THR( _pAccObject->get_accSelection(pvarSelectedChildren) );

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccObject::accSelect( long flagsSel, VARIANT varChild)
{
    HRESULT hr  = E_NOTIMPL;

    if ( EnsureAccObject()) 
        hr = THR( _pAccObject->accSelect( flagsSel, varChild) );

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL  
CAccObject::EnsureAccObject( )
{
    HWND        hwndControl = NULL;
    HRESULT     hr;
    
    if (_pAccObject)
        return TRUE;
        
    // If the tag was an embed tag, there is no way that it
    // can support IAccessible


    // RJG -- I believe this is not True

    if ( _pElement->Tag() != ETAG_EMBED )
    {
        IDispatch * pDisp = NULL;

#if (SPVER >= 0x1)
        //HACKHACK: FerhanE: It is common for sites to contain APPLET tags that don't have 
        //                   a codebase at all. When we try to talk to these controls, the
        //                   trust dialogs are shown and this bothers the screen readers.
        //                   We check for an applet tag having a codebase attribute and if
        //                   it does not have one, we don't try to delegate to the control
        //                   or to the OLEACC at all. IEBug:36140
        //
        if ((_pElement->Tag() == ETAG_APPLET) && 
            !(DYNCAST( CObjectElement, _pElement))->GetAAcodeBase() && 
            !(DYNCAST( CObjectElement, _pElement))->GetAAcode())
        {
            return FALSE;
        }
#endif

        // get the IDispatch pointer to the COM object that is
        // inside the OBJECT/PLUGIN tag
        (DYNCAST( CObjectElement, _pElement))->get_object( &pDisp );

        if (pDisp)
        {
            hr = THR_NOTRACE(pDisp->QueryInterface( IID_IAccessible, (void **)&_pAccObject));
//BUGBUG: ferhane
//          WebView control in the active desktop returns a value in the _pAccobject, although
//          it returns an error code.
            if (hr)
                _pAccObject = NULL;

            // Release the pDisp, so that we have a single reference on the
            // object if we have the _pAccObject. 
            // In case, we don't get a valid _pAccObject, this call serves
            // as a cleanup call...
            pDisp->Release();
        }
    }
    
    // if we don't have an IAccessible after checking with the component, 
    // try to delegate to the OLEACC for windowed controls. There is nothing we can do
    // for windowless controls.
    hwndControl = _pElement->GetHwnd();

    if (!_pAccObject && hwndControl && IsWindow(hwndControl))
    {
        static DYNPROC  s_dynprocAccessibleObjectFromWindow = { NULL, &g_dynlibOLEACC, "AccessibleObjectFromWindow"};

        // Load up the LresultFromObject pointer.
        hr = THR(LoadProcedure(&s_dynprocAccessibleObjectFromWindow));
        
        if (S_OK == hr)
        {
            hr = THR((*(HRESULT (APIENTRY *)(HWND, DWORD, REFIID, void**))
                        s_dynprocAccessibleObjectFromWindow.pfn)(   hwndControl, 
                                                                    OBJID_WINDOW, 
                                                                    IID_IAccessible,
                                                                    (void **)&_pAccObject));
            if (S_OK != hr)
            {
                _pAccObject = NULL;  // double check for the correct return value.
            }
        }
    }
       
    return !!_pAccObject;
}

//----------------------------------------------------------------------------
//
//  IOleWindow implementation
//
//----------------------------------------------------------------------------
STDMETHODIMP    
CAccObject::GetWindow( HWND* phwnd )
{
    HRESULT hr = S_OK;
    
    if ( !phwnd )
    {
        hr = E_POINTER;
    }
    else
    {   
        // if there is a window handle for the control.
        *phwnd = _pElement->GetHwnd();

        // by design, for windowless objects.
        if (!(*phwnd))
            hr = E_FAIL;
    }

    RRETURN( hr );
}

STDMETHODIMP
CAccObject::ContextSensitiveHelp( BOOL fEnterMode )
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//  get_accRole
//  
//  DESCRIPTION:
//      If the object implements IAccessible, then calls that implementation
//      otherwise returns ROLE_SYSTEM_CLIENT
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccObject::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT hr = S_OK;

    // validate out parameter
    if ( !pvarRole )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarRole ) = VT_EMPTY;

    if ( EnsureAccObject()) 
    {
        hr = THR( _pAccObject->get_accRole( varChild, pvarRole) ); 
    }
    else
    {
        // return the role information we have for the element, since
        // this code is only hit when a select does not have a current selection
        V_VT(pvarRole) = VT_I4;
        V_I4(pvarRole) = GetRole();
    }
    
Cleanup:
    TraceTag((tagAcc, "CAccSelect::get_accRole, childid=%d role=%d, hr=0x%x", 
                V_I4(&varChild), V_I4( pvarRole ),  hr));  
    
    RRETURN( hr );
}
