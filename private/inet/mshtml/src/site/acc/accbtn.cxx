//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBtn.Cxx
//
//  Contents:   Accessible object for the INPUT buttons( BUTTON, RESET, SUBMIT)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCBTN_HXX_
#define X_ACCBTN_HXX_
#include "accbtn.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif


//-----------------------------------------------------------------------
//  CAccBtn::CAccBtn()
//
//  DESCRIPTION:
//      Contructor. 
//
//  PARAMETERS:
//      pElementParent  :   Address of the CElement that hosts this 
//                          object.
//----------------------------------------------------------------------
CAccButton::CAccButton( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );

    //initialize the instance variables
    SetRole( ROLE_SYSTEM_PUSHBUTTON );
}

//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty return it. Otherwise return the object.value 
//      for tag==INPUT and object.innertext for tag==BUTTON
//      
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccButton::GetAccName( BSTR* pbstrName)
{
    HRESULT hr = S_OK;
    CStr    strValue;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    if ( _pElement->Tag() == ETAG_INPUT )
    {
        hr = THR( DYNCAST(CInput, _pElement)->GetValueHelper( &strValue ) );
        if ( hr )
            goto Cleanup;

        hr = THR( strValue.AllocBSTR( pbstrName ) );
    }
    else
    {
        hr = THR( _pElement->get_innerText( pbstrName ) );
    }

    //get the title 
    if ( !( *pbstrName ) )
        hr = GetTitle(pbstrName);


Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      If the title is not empty return it. Otherwise return the object.value 
//      for tag==INPUT and object.innertext for tag==BUTTON
//  
//  PARAMETERS:
//      pbstrDescription    :   BSTR pointer to receive the description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccButton::GetAccDescription( BSTR* pbstrDescription)
{
    HRESULT hr = S_OK;
    CStr    strValue;

    // validate out parameter
    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    // the description is the Title if there is a value for:
    //     NAME:{if <input type=button > return Value or "" .
    //           for <BUTTON> tag return the inner text }
    // otherwise, it is ""
    if ( _pElement->Tag() == ETAG_INPUT )
    {
        hr = THR( DYNCAST(CInput, _pElement)->GetValueHelper( &strValue ) );
    }
    else
    {
        hr = THR( _pElement->get_innerText( pbstrDescription ) );
    }

    // we'll have one or the other, but never both
    if ( *pbstrDescription)
    {
        SysFreeString(*pbstrDescription);
        *pbstrDescription = NULL;
        hr = GetTitle( pbstrDescription );
    }
    else if (!strValue.IsNull())
    {
        hr = GetTitle( pbstrDescription );
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
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
CAccButton::GetAccState(  VARIANT *pvarState)
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
    
        if (( _pElement->Tag()==ETAG_INPUT ) && 
            ( (DYNCAST(CInput, _pElement))->GetAAtype() == htmlInputSubmit) )
        {
            V_I4( pvarState ) |= STATE_SYSTEM_DEFAULT;
        }
    }

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDefaultAction
//  
//  DESCRIPTION:
//  Returns the default action, which is "Press"
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccButton::GetAccDefaultAction( BSTR* pbstrDefaultAction)
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = SysAllocString( _T("Press") );

    if (!(*pbstrDefaultAction) )
        hr = E_OUTOFMEMORY;
   
Cleanup:
   RRETURN( hr );
}

