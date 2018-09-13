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

#ifndef X_ACCANCH_HXX_
#define X_ACCANCH_HXX_
#include "accanch.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif


//----------------------------------------------------------------------------
//  CAccAnchor
//  
//  DESCRIPTION:    
//      The anchor accessible object constructor
//
//  PARAMETERS:
//      Pointer to the anchor element 
//----------------------------------------------------------------------------
CAccAnchor::CAccAnchor( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_LINK );
}


//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title
//      else, returns the inner text of the anchor, which is the name seen for the 
//      anchor. 
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccAnchor::GetAccName( BSTR* pbstrName )
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

    //get the title for the anchor.
    pchString = (LPTSTR) _pElement->GetAAtitle();
    if ( pchString )
    {
        *pbstrName = SysAllocString( pchString );
        if ( !(*pbstrName) )
            hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR( _pElement->get_innerText( pbstrName ) );
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccValue
//  
//  DESCRIPTION:
//      Returns the href of the anchor, which is stored in the CHyperLink
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccAnchor::GetAccValue( BSTR* pbstrValue )
{
    HRESULT hr;

    // validate out parameter
     if ( !pbstrValue )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrValue = NULL;

    hr = THR( (DYNCAST(CAnchorElement,_pElement))->get_href( pbstrValue ) );

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      Returns the inner text of the anchor, which is the name seen for the 
//      anchor. Actually, this is the same function with the GetAccName for the
//      anchor element, so it calls GetAccName()
//  
//  PARAMETERS:
//      pbstrDescription    :   BSTR pointer to receive the description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccAnchor::GetAccDescription( BSTR* pbstrDescription )
{
    RRETURN( GetAccName(pbstrDescription) );
}

//----------------------------------------------------------------------------
//  GetAccState
//  
//  DESCRIPTION:
//      always STATE_SYSTEM_LINKED
//      if not visible, then STATE_SYSTEM_INVISIBLE
//      if document has the focus, then STATE_SYSTEM_FOCUSABLE
//      if this is the active element. then STATE_SYSTEM_FOCUSED
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccAnchor::GetAccState( VARIANT *pvarState)
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
    V_I4( pvarState ) = STATE_SYSTEM_LINKED;
    
    if ( IsFocusable(_pElement) )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;
    
    if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus() )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
    
    if (_pElement->IsDisplayNone() || 
    	_pElement->IsVisibilityHidden())
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;

    if ( (DYNCAST( CAnchorElement, _pElement))->IsVisited() )
        V_I4( pvarState ) |= STATE_SYSTEM_TRAVERSED;
        
Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDefaultAction
//  
//  DESCRIPTION:
//  Returns the default action for  an anchor element, in a string. The default
//  action string is "jump"
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccAnchor::GetAccDefaultAction( BSTR* pbstrDefaultAction )
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = SysAllocString( _T("Jump") );

    if ( !(*pbstrDefaultAction) )
        hr = E_OUTOFMEMORY;
   
Cleanup:
   RRETURN( hr );
}
