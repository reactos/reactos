//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccInImg.Cxx
//
//  Contents:   Accessible INPUT TYPE=IMAGE object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCINIMG_HXX_
#define X_ACCINIMG_HXX_
#include "accinimg.hxx"
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
//  CAccInputImg
//  
//  DESCRIPTION:    
//      The input image accessible object constructor
//
//  PARAMETERS:
//      Pointer to the input image element 
//----------------------------------------------------------------------------
CAccInputImg::CAccInputImg( CElement* pElementParent )
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
//      If the title is not empty, returns the title
//      else, returns alt text if exists, otherwise
//      returns E_NOTIMPL
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccInputImg::GetAccName( BSTR* pbstrName )
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

    //get the title for the input image 
    pchString = (LPTSTR) _pElement->GetAAtitle();        
    if ( !pchString )
    {
        //get the alt
        pchString = (LPTSTR) (DYNCAST( CInput, _pElement))->GetAAalt();
    }
    
    if ( !pchString )
    { 
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    *pbstrName = SysAllocString( pchString );
    if ( !(*pbstrName) )
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccValue
//  
//  DESCRIPTION:
//      if there is dynsrc return that, if not,
//      then return src
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccInputImg::GetAccValue( BSTR* pbstrValue)
{
    HRESULT     hr = S_OK;
    TCHAR *     pchString = NULL;
    TCHAR       cBuf[pdlUrlLen];
    TCHAR *     pchNewUrl = cBuf;
    CDoc *      pDoc = _pElement->Doc();

    // validate out parameter
    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrValue = NULL;

    pchString = (LPTSTR) (DYNCAST(CInput, _pElement))->GetAAdynsrc();
    if ( !pchString )
    {
        pchString = (LPTSTR) (DYNCAST(CInput, _pElement))->GetAAsrc();
    }

    //did we get something in the pchString?
    if ( !pchString )
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    // Return fully expanded URL
    if ( pDoc )
        hr = THR( pDoc->ExpandUrl(pchString, ARRAY_SIZE(cBuf), pchNewUrl, _pElement));

    if (hr || (pchNewUrl == NULL))
        goto Cleanup;

    *pbstrValue = SysAllocString( pchNewUrl );
    if ( !(*pbstrValue) )
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title
//      else, returns alt text if exists, otherwise
//      returns E_NOTIMPL
//  
//  PARAMETERS:
//      pbstrDescription   :   BSTR pointer to receive the Description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccInputImg::GetAccDescription( BSTR* pbstrDescription)
{
    HRESULT hr = S_OK;
    TCHAR * pchString = NULL;

    // validate out parameter
    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    // if there is a title then the alt goes into descption,
    // otherwise the desc is ""
    if (_pElement->GetAAtitle())
    {
        //get the alt. text
        pchString = (LPTSTR) (DYNCAST( CInput, _pElement))->GetAAalt();

        if ( pchString )
        {   
            *pbstrDescription = SysAllocString( pchString );
            if ( !(*pbstrDescription) )
                hr = E_OUTOFMEMORY;
        }
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccState
//  
//  DESCRIPTION:
//      if not complete, then STATE_SYSTEM_UNAVAILABLE
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
CAccInputImg::GetAccState( VARIANT *pvarState )
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
    
    if ( IsFocusable(_pElement) )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;
    
    if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus()) 
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
    
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;
    
    if ( ! (DYNCAST(CInput, _pElement))->GetAAcomplete() )
        V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;


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
CAccInputImg::GetAccDefaultAction( BSTR* pbstrDefaultAction)
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

