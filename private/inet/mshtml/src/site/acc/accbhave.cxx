//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBhave.Cxx
//
//  Contents:   Accessible implementation for binary behaviors
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCBHAVE_HXX_
#define X_ACCBHAVE_HXX_
#include "accbhave.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

ExternTag(tagAcc);

CAccBehavior::CAccBehavior( CElement * pElementParent )
: CAccElement(pElementParent)
{
    // As the default role, we give the behavior the role that we would
    // give for an ActiveX control. However, the behavior has the right to 
    // change that, since we delegate the get_accRole call.
    SetRole(ROLE_SYSTEM_CLIENT);
}

#define IMPLEMENT_DELEGATE_TO_BEHAVIOR(meth)    \
	IAccessible * pAccPtr;						\
												\
	Assert( _pElement->GetPeerHolder() );		\
												\
    hr = THR(_pElement->GetPeerHolder()->QueryPeerInterfaceMulti		\
						(IID_IAccessible, (void **)&pAccPtr, FALSE));	\
    if (S_OK == hr)                             \
    {											\
		Assert(pAccPtr);						\
        hr = THR(pAccPtr->meth);				\
        pAccPtr->Release();						\
    }                                           \

//----------------------------------------------------------------------------
//  get_accChildCount
//
//  DESCRIPTION:
//      if the behavior that is being represented here supports IAccessible,
//      we delegate the call to that behavior. Otherwise we return 0.
//
//  PARAMETERS:
//      pChildCount :   address of the parameter to receive the child count
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accChildCount(long* pChildCount)
{
    HRESULT hr;

    if ( !pChildCount )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accChildCount(pChildCount))

    // if we get an error that indicates that the behavior does not support the 
    // IAccessible or the method has a problem running, we return S_OK and a child
    // count of zero.
    if (hr)
    {
        *pChildCount = 0;   //there are no children    
        hr = S_OK;
    }   

    TraceTag((tagAcc, "CAccBehavior::get_accChildCount, childcnt=%d hr=%d", 
                *pChildCount, hr));

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  get_accChild()
//
//  DESCRIPTION:
//      if the behavior that is being represented here supports IAccessible,
//      we delegate the call to that behavior. Otherwise we return an error, since
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
CAccBehavior::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
    HRESULT      hr = S_FALSE;

    // validate out parameter
    if ( !ppdispChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppdispChild = NULL;        //reset the return value.

    IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accChild(varChild, ppdispChild))

	// if there were any problems at all, we should assume this element
	// as an element without children.
	if (hr)
		hr = S_FALSE;

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accChild, childid=%d requested, hr=0x%x", 
                        V_I4(&varChild),
                        hr));  

    RRETURN1( hr, S_FALSE );    //S_FALSE is valid when there are no children
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
//      behavior, and returns E_NOINTERFACE if the behavior does not support 
//      IAccessible.
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
CAccBehavior::accLocation(  long* pxLeft, long* pyTop, 
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
		// we pass the error that is actually coming from the behavior, since this location is
		// only hit when the behavior actually reported that it had children.
		IMPLEMENT_DELEGATE_TO_BEHAVIOR(accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild))
    }

Cleanup:

    TraceTag((tagAcc, "CAccBehavior::accLocation, childid=%d hr=0x%x", 
                V_I4(&varChild),
                hr));  

    RRETURN1( hr, S_FALSE ); 
}

//----------------------------------------------------------------------------
//  accNavigate
//  
//  DESCRIPTION:
//      Delegate to the behavior if it implements the IAccessible. Otherwise
//      not implemented.
//      
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT hr = E_NOTIMPL;

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    V_VT( pvarEndUpAt ) = VT_EMPTY;
    
	// In the case that the behavior does not support IAccessible, 
	// cook up a return value
	IMPLEMENT_DELEGATE_TO_BEHAVIOR(accNavigate(navDir, varStart, pvarEndUpAt))

	if (E_NOINTERFACE == hr)
		hr = E_NOTIMPL;

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::accNavigate, start=%d, direction=%d", 
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
//                  If the behavior implements IAccessible, then the call is 
//                  delegated to the behavior. Otherwise CHILDID_SELF is 
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
CAccBehavior::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    HRESULT     hr;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(accHitTest(xLeft,yTop,pvarChildAtPoint))

	// if the behavior does not support IAccessible,
    if (hr)
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
    TraceTag((tagAcc, "CAccBehavior::accHitTest, point(%d,%d), hr=0x%x", 
                xLeft, yTop, hr));  

    RRETURN(hr);
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
CAccBehavior::accDoDefaultAction(VARIANT varChild)
{   
    HRESULT     hr;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(accDoDefaultAction(varChild))

	// if the behavior is not capable of handling this, try to cover for
	// its inability.
	if (hr)
    {
        if ( V_I4(&varChild) == CHILDID_SELF )
        {
            hr = THR( ScrollIn_Focus( _pElement ) );                
        }
        else
            hr = E_NOTIMPL;
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::accDoDefaultAction, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accName
//  
//  DESCRIPTION:
//      If the behavior implements IAccessible, then call that implementation
//      otherwise return the title, if there is one.
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accName(VARIANT varChild,  BSTR* pbstrName )
{
    HRESULT hr = S_OK;
    TCHAR * pchString = NULL;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accName(varChild, pbstrName))

	if (hr)
    {
        //get the title 
        pchString = (LPTSTR) _pElement->GetAAtitle();
        
        if ( pchString )
        {
            *pbstrName = SysAllocString( pchString );
            
            if ( !(*pbstrName) )
                hr = E_OUTOFMEMORY;
        }
    }
    
Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accName, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accValue
//  
//  DESCRIPTION:
//		Delegates to the behavior. If there is no implementation on the 
//		behavior then E_NOINTERFACE
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accValue(VARIANT varChild,  BSTR* pbstrValue )
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

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accValue(varChild, pbstrValue))

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accValue, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  get_accDefaultAction
//  
//  DESCRIPTION:
//  If the behavior supports the IAccessible, the behavior is called. 
//	Otherwise the default action for OBJECT tags, which is "select" is returned.
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accDefaultAction(VARIANT varChild,  BSTR* pbstrDefaultAction )
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = NULL;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accDefaultAction(varChild, pbstrDefaultAction ))

    if (hr)
    {
        // bugbug resource string
        *pbstrDefaultAction = SysAllocString( _T("Select") );

        if ( !(*pbstrDefaultAction) )
            hr = E_OUTOFMEMORY;
    }
   
Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accDefaultAction, childid=%d, hr=0x%x", 
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
CAccBehavior::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT hr = S_OK;

    // validate out parameter
    if ( !pvarState )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accState( varChild, pvarState))

	if (hr)
    {
		hr = S_OK;

		CDoc *  pDoc = _pElement->Doc();

        V_I4( pvarState ) = 0;

        if ( _pElement->GetReadyState() != READYSTATE_COMPLETE )
            V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;
        
//BUGBUG: ferhane
//			This is a problem, since with the behavior attached elements becoming 
//			supported, we have to know if it is a supported tag, or it has a behavior, 
//			and/or if it has a tabindex.
        if (IsFocusable(_pElement))
            V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;
        
        if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus() )
            V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
        
        if ( !_pElement->IsVisible(FALSE) )
            V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;
    }
    
Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accState, childid=%d, state=0x%x, hr=0x%x", 
                V_I4(&varChild), V_I4( pvarState ), hr));  

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accDescription(VARIANT varChild, BSTR * pbstrDescription )
{
    HRESULT hr;

    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accDescription(varChild, pbstrDescription))


//BUGBUG:ferhane
//			This is only needed if we decide that the behavior's implementation of IAccessible
//			takes precedence over the object's implementation in a scenario where
//          a behavior is applied to an object tag. If the object's implementation takes 
//			precedence, then this code would not get hit anyway, since the CAccObject would
//			be processing the call.
//
		
	if (hr)
    {
        hr = S_OK;

		// do something special for the object and plgins. 
        if (_pElement->Tag() == ETAG_OBJECT)
        {
            *pbstrDescription = SysAllocString( _T("PLUGIN: type=Object") );
        }
        else if (_pElement->Tag() == ETAG_EMBED)
        {
            *pbstrDescription = SysAllocString( _T("PLUGIN: type=Embed") );
            
            if ( !(*pbstrDescription) )
                hr = E_OUTOFMEMORY;
        }
		else
			hr = E_NOTIMPL;
    }

Cleanup:
    TraceTag((tagAcc, "CAccBehavior::get_accDescription, childid=%d, hr=0x%x", 
                V_I4(&varChild), hr));  
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    HRESULT hr;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accKeyboardShortcut(varChild, pbstrKeyboardShortcut))

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accFocus(VARIANT * pvarFocusChild)
{
    HRESULT hr;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accFocus(pvarFocusChild))

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccBehavior::get_accSelection(VARIANT * pvarSelectedChildren)
{
    HRESULT hr;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accSelection(pvarSelectedChildren))

    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::accSelect( long flagsSel, VARIANT varChild)
{
    HRESULT hr;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR( accSelect( flagsSel, varChild) )

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBehavior::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT hr;

    TraceTag((tagAcc, "CAccBehavior::get_accRole, childid=%d", 
                V_I4(&varChild)));  

	if (!pvarRole)
	{
		hr = E_POINTER;
		goto Cleanup;
	}

    // clear the out parameter
    V_VT( pvarRole ) = VT_EMPTY;

	IMPLEMENT_DELEGATE_TO_BEHAVIOR(get_accRole(varChild, pvarRole))

	// if the behavior is not capable of reporting a role, then return the
	// default role
	if (hr)
	{
		if ((V_VT(&varChild) == VT_I4) && 
			(V_I4(&varChild) == CHILDID_SELF))
		{
			// pack role into out parameter
			V_VT( pvarRole ) = VT_I4;
			V_I4( pvarRole ) = GetRole();
			hr = S_OK;
		}
		else
		{
			hr = E_NOTIMPL;
		}
	}

Cleanup:
	RRETURN(hr);
}