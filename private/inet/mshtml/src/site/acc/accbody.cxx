//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBody.Cxx
//
//  Contents:   Accessible object for the Body Element 
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCBODY_HXX_
#define X_ACCBODY_HXX_
#include "accbody.hxx"
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

ExternTag(tagAcc);

//-----------------------------------------------------------------------
//  CAccBody::CAccBody()
//
//  DESCRIPTION:
//      Contructor. 
//
//  PARAMETERS:
//      pElementParent  :   Address of the CElement that hosts this 
//                          object.
//----------------------------------------------------------------------
CAccBody::CAccBody( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );

    //initialize the instance variables
    SetRole(ROLE_SYSTEM_PANE);
}

//-----------------------------------------------------------------------
//  get_accParent
//
//  DESCRIPTION    :
//          Return the window object related to the document that contains
//          the body tag.
//          The implementation of this method is fairly simple and different
//          from the CAccBase::get_accParent implementation. The reason is 
//          that we know for a that the body can be only parented by the
//          document itself.
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBody::get_accParent( IDispatch ** ppdispParent )
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = _pElement->Doc();
    
    if ( !ppdispParent )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispParent = NULL;

    // If the document has not been questioned for its accessible object
    // yet. This could happen if the hit test started at a windowed child.
    if ( pDoc )
    {
        if ( !pDoc->_pAccWindow )
        {
            pDoc->_pAccWindow = new CAccWindow( pDoc );
            if (!pDoc->_pAccWindow )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        Assert( pDoc->_pAccWindow );
    
        hr = THR( pDoc->_pAccWindow->QueryInterface( IID_IDispatch, 
                                                     (void**)ppdispParent) );
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  get_accSelection
//  
//  DESCRIPTION:
//      The CAccBody is a special case for this method, since it also can
//      represent a frameset. 
//      If the object represents a frameset, then the call is delegated to the 
//      frame that has the focus.
//      Otherwise, the call is delegated to the CAccElement implementation of this
//      method, since CAccElement is the base class for the CAccBOdy, and the body
//      element is treated as any other.
//      
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBody::get_accSelection(VARIANT * pvarSelectedChildren)
{
    HRESULT     hr = S_OK;
    CDoc *      pActiveFrameDoc = NULL;
    CElement *  pClient = NULL;
    CAccBase *  pAccChild = NULL;
    CDoc *      pDoc = _pElement->Doc();

    if ( _pElement->Tag() == ETAG_BODY )
    {
        hr = THR( CAccElement::get_accSelection( pvarSelectedChildren ) );
        goto Cleanup;
    }

    // the element is a frameset.

    // get the active frame and delegate the call. This function handles
    // recursive frames too.
    if ( pDoc )
    {
        hr = THR(pDoc->GetActiveFrame(NULL, 0, &pActiveFrameDoc, NULL));

        // does the frameset document have focus?
        if (pActiveFrameDoc == pDoc )
        {
            hr = THR( CAccElement::get_accSelection( pvarSelectedChildren ) );
            goto Cleanup;
        }

        // Delegate the call to the active frame's document's element client.
        pClient = pActiveFrameDoc->GetPrimaryElementClient();

        Assert( pClient );
    
        pAccChild = GetAccObjOfElement( pClient );
        if ( !pAccChild )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( pAccChild->get_accSelection( pvarSelectedChildren ) );
    }
        
Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  DESCRIPTION:
//      Returns the document title
//      
//
//  PARAMETERS:
//      pbstrName   :   address of the pointer to receive the URL BSTR
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBody::GetAccName( BSTR* pbstrName)
{
    HRESULT hr = S_OK;
    
    CDoc *  pDoc = _pElement->Doc();

    // validate out parameter
    if ( !pbstrName )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrName = NULL;

    
    if ( pDoc )
        hr = THR( pDoc->get_title( pbstrName ) );

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  DESCRIPTION :   
//      Return the value for the document object, this is the URL of the 
//      document if the child id is CHILDID_SELF. 
//
//  PARAMETERS:
//      pbstrValue  :   pointer to the BSTR to receive the value.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBody::GetAccValue( BSTR* pbstrValue )
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = _pElement->Doc();

    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pbstrValue = NULL;

    if ( pDoc )
        hr = THR( pDoc->get_URL( pbstrValue ) );
       
Cleanup:
    RRETURN( hr );
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccBody::GetAccState( VARIANT *pvarState)
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
    
    if ( _pElement->GetReadyState() != READYSTATE_COMPLETE )
        V_I4( pvarState ) = STATE_SYSTEM_UNAVAILABLE;
    else 
        V_I4( pvarState ) = STATE_SYSTEM_READONLY;
    
    if ( IsFocusable(_pElement) )
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSABLE;

    if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus()) 
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;    

Cleanup:
    RRETURN( hr );
}
