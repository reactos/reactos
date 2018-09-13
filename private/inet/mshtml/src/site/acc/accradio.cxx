//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccRadio.Cxx
//
//  Contents:   Accessible radio button object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCRADIO_HXX_
#define X_ACCRADIO_HXX_
#include "accradio.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif


//----------------------------------------------------------------------------
//  CAccRadio
//  
//  DESCRIPTION:    
//      The radio button accessible object constructor
//
//  PARAMETERS:
//      Pointer to the radio button element 
//----------------------------------------------------------------------------
CAccRadio::CAccRadio( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_RADIOBUTTON );
}


//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      Returns the label, if not the title.
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccRadio::GetAccName( BSTR* pbstrName )
{
    HRESULT hr;
 
    // validate out parameter
    if ( !pbstrName )
    {
        hr= E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    hr = THR( GetLabelorTitle( pbstrName ) );

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      Returns the label, if not, the title
//  
//  PARAMETERS:
//      pbstrDescription   :   BSTR pointer to receive the Description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccRadio::GetAccDescription( BSTR* pbstrDescription)
{
    HRESULT hr=S_OK;

    // validate out parameter
    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    if (HasLabel())
        hr = THR( GetTitle( pbstrDescription ) );

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//----------------------------------------------------------------------------
//  GetAccState
//  
//  DESCRIPTION:
//      if not visible, then STATE_SYSTEM_INVISIBLE
//      if document has the focus, then STATE_SYSTEM_FOCUSABLE
//      if this is the active element. then STATE_SYSTEM_FOCUSED
//      if it is not enabled then STATE_SYSTEM_UNAVAILABLE
//      if checked then STATE_SYSTEM_CHECKED
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccRadio::GetAccState( VARIANT *pvarState )
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bChecked = FALSE;
    CDoc *          pDoc = _pElement->Doc();

    // validate out parameter
    if ( !pvarState )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    
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
    
        if ( !_pElement->IsVisible(FALSE) )
            V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;
    
        hr = THR( (DYNCAST(CInput, _pElement))->GetChecked(&bChecked) ) ;
        if ( hr )
            goto Cleanup;
    
        if ( bChecked != VB_FALSE )
            V_I4( pvarState ) |= STATE_SYSTEM_CHECKED;
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccDefaultAction
//  
//  DESCRIPTION:
//      Returns the default action for a radio button, "select"
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccRadio::GetAccDefaultAction( BSTR* pbstrDefaultAction)
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = SysAllocString( _T("Select") );

    if (!(*pbstrDefaultAction) )
        hr = E_OUTOFMEMORY;
   
Cleanup:
   RRETURN( hr );
}
