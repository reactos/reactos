//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccTab.Cxx
//
//  Contents:   Accessible object for a (generic) element that has a tabstop, but
//              would otherwise be unsupported 
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCTAB_HXX_
#define X_ACCTAB_HXX_
#include "acctab.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif


//-----------------------------------------------------------------------
//  CAccTabStopped::CAccTabStopped()
//
//  DESCRIPTION:
//      Contructor. 
//
//  PARAMETERS:
//      pElementParent  :   Address of the CElement that hosts this 
//                          object.
//----------------------------------------------------------------------
CAccTabStopped::CAccTabStopped( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );

    //initialize the instance variables, since this is sort of 
    // a generic element, we would like to set a role that aprximates
    // the role that this element is playing on the page. Note that 
    // none of the tags listed below are specified in the supported
    // element list.
    switch (pElementParent->Tag())
    {
    case ETAG_LI:
    case ETAG_OL:
    case ETAG_UL:
    case ETAG_DD:
    case ETAG_DL:
    case ETAG_DT:
        SetRole( ROLE_SYSTEM_LISTITEM );
        break;

    case ETAG_HR:
        SetRole( ROLE_SYSTEM_SEPARATOR );
        break;

    case ETAG_DIV:
    case ETAG_SPAN:
        SetRole( ROLE_SYSTEM_GROUPING );
        break;

    default:
        SetRole( ROLE_SYSTEM_TEXT );
        break;
    }
}

//----------------------------------------------------------------------------
//  helper : GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty return it. Otherwise return the innerText value 
//  
//----------------------------------------------------------------------------
STDMETHODIMP
CAccTabStopped::GetAccName( BSTR* pbstrName)
{
    HRESULT hr = S_OK;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    hr = GetTitle(pbstrName);
    if (hr || !*pbstrName)
    {
        hr = THR( _pElement->get_innerText( pbstrName ) );
    }
    

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  helper : GetAccDescription
//  
//  DESCRIPTION:
//      If the title is not empty return the innerText, else return nothing 
//  
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccTabStopped::GetAccDescription( BSTR* pbstrDescription)
{
    HRESULT hr = S_OK;
    BOOL    fNeedText;

    // validate out parameter
    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    // check for the title attribute, then toss the string
    hr = GetTitle( pbstrDescription );
    fNeedText = (hr == S_OK && *pbstrDescription);

    SysFreeString(*pbstrDescription);
    *pbstrDescription = NULL;
    hr = S_OK;

    // if we had a title, return the innerText, else nothing
    if (fNeedText)
    {
        hr = THR( _pElement->get_innerText( pbstrDescription ) );
    }


Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  member : GetAccState
//  
//  DESCRIPTION:
//      always STATE_SYSTEM_Focusable 
//      if not visible, then STATE_SYSTEM_INVISIBLE
//      if this is the active element. then STATE_SYSTEM_FOCUSED
//  
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccTabStopped::GetAccState(VARIANT *pvarState)
{
    HRESULT hr = S_OK;
    CAccBase * pParentA =NULL;
    CDoc *  pDoc = _pElement->Doc();

    // validate out parameter
     if ( !pvarState )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

     // first, if we have an anchor parent set our state to its state
    pParentA = GetParentAnchor();

    //if we have  an anchor, assume its state
    if (pParentA)
    {
        hr = THR( DYNCAST(CAccElement, pParentA)->GetAccState( pvarState ) );
    }
    else 
    {
        V_VT( pvarState ) = VT_I4;
        V_I4( pvarState ) = 0;        
    
        if ( !_pElement->IsEnabled() )
            V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;
        else
        {
            if ( IsFocusable(_pElement) )
                V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;
    
            if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus()) 
                V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;

            if (_pElement->IsDisplayNone() || 
            	_pElement->IsVisibilityHidden())
                V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;    
        }
    }

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  helper : GetAccValue
//  
//  DESCRIPTION:
//      If the parent is an anchor, delegate to the anchor. If not, Nothing.
//  
//----------------------------------------------------------------------------
STDMETHODIMP
CAccTabStopped::GetAccValue( BSTR* pbstrValue)
{
    HRESULT     hr = S_OK;
    CAccBase *  pParentA = NULL;

    // validate out parameter
    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrValue = NULL;

    //get the parent and see if it is an anchor.
    pParentA = GetParentAnchor();

    //if parent is an anchor, delegate the call. otherwise return S_OK and
    // a NULL string
    if (pParentA )
    {
        hr = THR( DYNCAST(CAccElement, pParentA)->GetAccValue( pbstrValue ));
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  member : get_accKeyboardShortCut
//  
//  DESCRIPTION :   
//          Returns the keyboard shortcut if there is one.
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccTabStopped::get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;
    CStr            accessString;
    CStr            sString;    

    // validate out parameter
     if ( !pbstrKeyboardShortcut )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrKeyboardShortcut = NULL;
    
    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        // get the actual key combination value
        hr = THR (accessString.Set( _pElement->GetAAaccessKey() ) );
        if ( hr )
            goto Cleanup;

        // if there is an access key string
        if ( accessString.Length() > 0 )
        {
            // we want all keyboard shortcut values to contain 'Alt+' 
            hr = THR( sString.Set( _T("Alt+") ) );
            if ( hr )
                goto Cleanup;
            
            hr = THR( sString.Append( accessString ) );
            if ( hr )
                goto Cleanup;
                
            hr = THR( sString.AllocBSTR( pbstrKeyboardShortcut ) );
        }                
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            //no keyboard shortcuts for plain text, unless a parent is an anchor
            CAccBase * pParentA = GetParentAnchor();
            CVariant   varChildSelf;

            V_VT(&varChildSelf) = VT_I4;
            V_I4(&varChildSelf) = CHILDID_SELF;

            //delegate this call 
            hr = (!pParentA) ? S_OK :
                THR( pParentA->get_accKeyboardShortcut(varChildSelf, pbstrKeyboardShortcut) );
        }
        else 
        {
            // call child's implementation of this method. 
            V_I4( &varChild ) = CHILDID_SELF;
            hr = THR( pAccChild->get_accKeyboardShortcut(varChild, pbstrKeyboardShortcut) );
        }
    }

Cleanup:
    RRETURN( hr );
}

