//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccMarq.Cxx
//
//  Contents:   Accessible Marquee object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCMARQ_HXX_
#define X_ACCMARQ_HXX_
#include "accmarq.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif


//----------------------------------------------------------------------------
//  CAccMarquee
//  
//  DESCRIPTION:    
//      The marquee accessible object constructor
//
//  PARAMETERS:
//      Pointer to the marquee element 
//----------------------------------------------------------------------------
CAccMarquee::CAccMarquee( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_ANIMATION );
}


//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title,
//      otherwise returns the innertext
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccMarquee::GetAccName( BSTR* pbstrName )
{
    HRESULT hr;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrName = NULL;

    hr = THR( GetTitle( pbstrName ) );
    if ( FAILED(hr) )
        goto Cleanup;
    
    if ( !(*pbstrName))
        hr = THR( _pElement->get_innerText( pbstrName ) );

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title,
//      otherwise returns the innertext
//  
//  PARAMETERS:
//      pbstrDescription   :   BSTR pointer to receive the Description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccMarquee::GetAccDescription( BSTR* pbstrDescription)
{
    HRESULT hr;

    // validate out parameter
    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrDescription = NULL;

    
    // if there is a title, then then inner text is reported
    // as the description.  If there isn't a title, then the 
    // description is blank.
    hr = GetTitle( pbstrDescription );
    if (hr)
        goto Cleanup;  // includeing S_FALSE

    if ( *pbstrDescription)
    {
        SysFreeString(*pbstrDescription);
        *pbstrDescription = NULL;
        hr = THR( _pElement->get_innerText( pbstrDescription ) );
    }

    
Cleanup:
    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  GetAccState
//  
//  DESCRIPTION:
//      always STATE_SYSTEM_MARQUEED
//      if not visible, then STATE_SYSTEM_INVISIBLE
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccMarquee::GetAccState( VARIANT *pvarState )
{
    HRESULT         hr = S_OK;

    // validate out parameter
     if ( !pvarState )
     {
         hr = E_POINTER;
         goto Cleanup;
     }
        
    V_VT( pvarState ) = VT_I4;    
    V_I4( pvarState ) = STATE_SYSTEM_MARQUEED;
    
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;

Cleanup:
    RRETURN( hr );
}

