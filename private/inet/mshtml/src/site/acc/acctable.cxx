//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccTable.Cxx
//
//  Contents:   Accessible table and table cell object implementations
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCTABLE_HXX_
#define X_ACCTABLE_HXX_
#include "acctable.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE__HXX_
#include "table.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE__HXX_
#include "ltable.hxx"
#endif

#ifndef X_TCELL_HXX_
#define X_TCELL_HXX_
#include "tcell.hxx"
#endif


//----------------------------------------------------------------------------
//  CAccTable
//  
//  DESCRIPTION:    
//      The table accessible object constructor
//
//  PARAMETERS:
//      Pointer to the element 
//----------------------------------------------------------------------------
CAccTable::CAccTable( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_TABLE );
}



//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title
//      else, returns the innertext of the first caption tag that is contained
//      in this object. If there is no title and not caption, the method is
//      not implemented.
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccTable::GetAccName( BSTR* pbstrName )
{
    HRESULT           hr= S_OK;
    TCHAR           * pchString = NULL;
    CTableCaption   * pCaption = NULL;
    CTableLayout    * pTableLayout = NULL;

    // validate out parameter
     if ( !pbstrName )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrName = NULL;

    //get the title 
    pchString = (LPTSTR) _pElement->GetAAtitle();
    if ( pchString )
    {
        *pbstrName = SysAllocString( pchString );
        if ( !(*pbstrName) )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // check if the table has a caption. If it does, return 
        // inner text from the caption
        pTableLayout = (DYNCAST( CTable, _pElement))->Layout();
        
        hr = pTableLayout->EnsureTableLayoutCache();
        if (hr)
            goto Cleanup;

        pCaption = pTableLayout->GetFirstCaption();

        if (pCaption)
        {
            hr = THR( pCaption->get_innerText( pbstrName ) );
        }
        else
            hr = E_NOTIMPL;
    }

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      Same as the GetAccName
//  
//  PARAMETERS:
//      pbstrDescription    :   BSTR pointer to receive the description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccTable::GetAccDescription( BSTR* pbstrDescription )
{
    HRESULT           hr;
    CTableCaption   * pCaption = NULL;
    CTableLayout    * pTableLayout = NULL;
    BSTR              bstrTemp = NULL;

    // validate out parameter
     if ( !pbstrDescription )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrDescription = NULL;

    // check if the table has a caption. If it does, return 
    // inner text from the caption
    pTableLayout = (DYNCAST( CTable, _pElement))->Layout();
    
    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    // if there is a title then it went into name and the caption 
    // should be the description.  otherwise there is no description.
    //--------------------------------------------------------------
    hr = GetTitle(&bstrTemp);
    if (hr)
        goto Cleanup;
    else
    {
        SysFreeString(bstrTemp);
        hr = S_OK;
        pCaption = pTableLayout->GetFirstCaption();

        if (pCaption)
        {
            hr = THR( pCaption->get_innerText( pbstrDescription ) );
        }
        // else just return a blank string
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}


//----------------------------------------------------------------------------
//  GetAccState
//  
//  DESCRIPTION:
//      always STATE_SYSTEM_NORMAL
//      if not visible, then STATE_SYSTEM_INVISIBLE
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccTable::GetAccState( VARIANT *pvarState)
{
    // validate out parameter
    if ( !pvarState )
        return ( E_POINTER );       
    
    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;

    //check visibility
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) = STATE_SYSTEM_INVISIBLE;
        
    return S_OK;
}

//----------------------------------------------------------------------------
//  CAccTableCell
//  
//  DESCRIPTION:    
//      The table cell accessible object constructor
//
//  PARAMETERS:
//      Pointer to the element 
//----------------------------------------------------------------------------
CAccTableCell::CAccTableCell( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_CELL );
}


//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title. Otherwise method not 
//      implemented
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccTableCell::GetAccName( BSTR* pbstrName )
{
    HRESULT           hr = S_OK;
    TCHAR           * pchString = NULL;

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    //get the title 
    pchString = (LPTSTR) _pElement->GetAAtitle();
    if ( pchString )
    {
        *pbstrName = SysAllocString( pchString );
        if ( !(*pbstrName) )
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_NOTIMPL;

Cleanup:
    RRETURN( hr );
}



//----------------------------------------------------------------------------
//  GetAccState
//  
//  DESCRIPTION:
//      always STATE_SYSTEM_NORMAL
//      if not visible, then STATE_SYSTEM_INVISIBLE
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccTableCell::GetAccState( VARIANT *pvarState)
{
    // validate out parameter
    if ( !pvarState )
        return E_POINTER;
    
    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;

    //check visibility
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) = STATE_SYSTEM_INVISIBLE;
    
    return S_OK;
}

