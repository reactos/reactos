//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccEdit.Cxx
//
//  Contents:   Accessible editbox object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCEDIT_HXX_
#define X_ACCEDIT_HXX_
#include "accedit.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

//----------------------------------------------------------------------------
//  CAccEdit
//  
//  DESCRIPTION:    
//      The editbox accessible object constructor
//
//  PARAMETERS:
//      Pointer to the editbox element 
//----------------------------------------------------------------------------
CAccEdit::CAccEdit( CElement* pElementParent, BOOL bIsPassword )
:CAccElement( pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_TEXT );

    // is this edit control of type input password.
    _bIsPassword = bIsPassword;
}


//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      Returns the label text or the title
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccEdit::GetAccName( BSTR* pbstrName )
{
    HRESULT hr = S_OK;

    // validate out parameter
     if ( !pbstrName )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrName = NULL;

    hr = THR( GetLabelorTitle(pbstrName) );    

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccValue
//  
//  DESCRIPTION:
//      Returns the value of the edit box.
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccEdit::GetAccValue( BSTR* pbstrValue)
{
    HRESULT hr = S_OK;
    CStr    str;

    // validate out parameter
    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrValue = NULL;

    // if not a password element, then return the information
    if ( !_bIsPassword )
    {
        if (_pElement->Tag() != ETAG_TEXTAREA)
        {
            hr = THR( (DYNCAST( CInput, _pElement))->GetValueHelper( &str ) );
        }
        else
        {
            hr = THR((DYNCAST(CTextArea, _pElement))->GetValueHelper( &str ));
        }
        
        if ( hr ) 
            goto Cleanup;

        //even if the value that is returned is NULL, we want to return it..
        hr = str.AllocBSTR(pbstrValue);
    }
    else
        hr = E_ACCESSDENIED;    // password's can not be read by outsiders.
    
Cleanup:
    RRETURN( hr );
}



//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      Returns the title if there is no label, otherwise ""
//  
//  PARAMETERS:
//      pbstrDescription   :   BSTR pointer to receive the Description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccEdit::GetAccDescription( BSTR* pbstrDescription)
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
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccEdit::GetAccState( VARIANT *pvarState)
{
    HRESULT hr =S_OK;
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
    }

    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;

    if ( _bIsPassword )
        V_I4( pvarState ) |= STATE_SYSTEM_PROTECTED;

Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP
CAccEdit::PutAccValue( BSTR bstrValue )
{
    HRESULT hr = S_OK;
    CStr    strValue;

    // move the element into focus.
    hr = THR( ScrollIn_Focus( _pElement ) );
    if ( hr )
        goto Cleanup;

    hr = THR( strValue.SetBSTR( bstrValue ) );
    if( hr )
        goto Cleanup;

    //set the value of the element.
    if (_pElement->Tag() != ETAG_TEXTAREA)
    {
        hr = THR( (DYNCAST( CInput, _pElement))->SetValueHelper(&strValue) );
    }
    else
    {
        hr = THR( (DYNCAST(CTextArea, _pElement))->SetValueHelper( &strValue ));
    }

Cleanup:
    RRETURN( hr );
}
