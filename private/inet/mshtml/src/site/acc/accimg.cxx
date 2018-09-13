//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccImg.Cxx
//
//  Contents:   Accessible Image object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCIMG_HXX_
#define X_ACCIMG_HXX_
#include "accimg.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

ExternTag(tagAcc);

//----------------------------------------------------------------------------
//  CAccImage
//  
//  DESCRIPTION:    
//      The image accessible object constructor
//
//  PARAMETERS:
//      Pointer to the image element 
//----------------------------------------------------------------------------
CAccImage::CAccImage( CElement* pElementParent )
:CAccElement(pElementParent)
{
    long lRole;
    
    Assert( pElementParent );
    
    //initialize the instance variables    
    if ( (DYNCAST( CImgElement, _pElement))->GetAAdynsrc() )
    {
        lRole = ROLE_SYSTEM_ANIMATION;
    }
    else
        lRole  = ROLE_SYSTEM_GRAPHIC;

    SetRole( lRole );
}

//----------------------------------------------------------------------------
//  get_accChildCount
//  
//  An image's child count is the number of areas that are connected to this 
//  image, through map objects.
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccImage::get_accChildCount(long* pChildCount)
{
    if ( !pChildCount )
        RRETURN( E_POINTER );

    TraceTag((tagAcc, "CAccImage::get_accChildCount"));  

    RRETURN( DYNCAST( CImgElement, _pElement)->GetSubDivisionCount ( pChildCount ) );
}

//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title
//      else, if there is alt text returns the alt text . 
//      else, not implemented
//
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccImage::GetAccName( BSTR* pbstrName )
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

    //get the title for the image
    pchString = (LPTSTR) _pElement->GetAAtitle();

    //if there is no title, get the alt. text
    if ( !pchString)
        pchString = (LPTSTR) (DYNCAST(CImgElement, _pElement))->GetAAalt();

    //if no title, and no alt. text.then walk up the accParent chain
    // and look for an anchor.  if there is one use its Name
    if ( !pchString )
    {
        CAccBase * pParent = GetParentAnchor();

        hr = (pParent) ? DYNCAST(CAccElement, pParent)->GetAccName(pbstrName) : E_NOTIMPL;

        goto Cleanup;
    }
    
    //we have something in the pchString, use it.
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
//      If the parent is an anchor, delegate to the anchor. If not,
//      then if there is dynsrc return that, if not,
//      then if there is src return that.
//      Since the constructor checks for the dynsrc to determine the role, we
//      check the role here.
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccImage::GetAccValue( BSTR* pbstrValue)
{
    HRESULT     hr = S_OK;
    CAccBase *  pParent = NULL;
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

    //get the parent and see if it is an anchor.
    pParent = GetParentAnchor();

    //if parent is an anchor, delegate the call.
    if (pParent )
    {
        hr = THR( DYNCAST(CAccElement, pParent)->GetAccValue( pbstrValue ));
    }
    else
    {
        //check if there is dynsrc.
        if ( _lRole == ROLE_SYSTEM_ANIMATION )
        {
            pchString = (LPTSTR) (DYNCAST( CImgElement, _pElement))->GetAAdynsrc();
        }
        else
        {
            pchString = (LPTSTR) (DYNCAST( CImgElement, _pElement))->GetAAsrc();
        }

        if (!pchString)
        {
            hr = E_NOTIMPL;
            goto Cleanup;
        }

        // Return fully expanded URL
        if ( pDoc )
            hr = THR( pDoc->ExpandUrl(pchString, ARRAY_SIZE(cBuf), pchNewUrl, _pElement));

        if (hr || (pchNewUrl == NULL))
            goto Cleanup;

        //we have something in the pchString, use it.
        *pbstrValue = SysAllocString( pchNewUrl );
        if ( !(*pbstrValue) )
            hr = E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      If the title is not empty, returns the title
//      else, if there is alt text returns the alt text . 
//      else, not implemented
//
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccImage::GetAccDescription( BSTR* pbstrDescription)
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
        pchString = (LPTSTR) (DYNCAST(CImgElement, _pElement))->GetAAalt();

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
//      if not visible, then STATE_SYSTEM_INVISIBLE
//      if object is not complete, then STATE_SYSTEM_UNAVAILABLE
//      adds the parents state to these, if the parent is an anchor.
//  
//  PARAMETERS:
//      pvarState   :   address of VARIANT to receive state information.
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccImage::GetAccState( VARIANT *pvarState )
{
    HRESULT    hr=S_OK;
    CAccBase * pParent = NULL;
    VARIANT    varChild;

    // validate out parameter
    if ( !pvarState )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
     
    V_VT( pvarState ) = VT_I4;
    V_I4( pvarState ) = 0;


    pParent = GetParentAnchor();
    
    //if parent is an anchor, get its values as well.
    if ( pParent )
    {
        V_I4(&varChild)=CHILDID_SELF; 
        V_VT(&varChild)=VT_I4;
        hr = THR( pParent->get_accState( varChild, pvarState ) );
        if ( hr )
            goto Cleanup;
    }
    
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;
    
    if ( !(DYNCAST( CImgElement, _pElement))->GetAAcomplete() )
        V_I4( pvarState ) |= STATE_SYSTEM_UNAVAILABLE;

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccDefaultAction
//  
//  DESCRIPTION:
//  Returns the default action. If the parent is anchor, returns "jump"
//  otherwise, not implemented.
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccImage::GetAccDefaultAction( BSTR* pbstrDefaultAction)
{
    HRESULT    hr = S_OK;
    CAccBase * pParent = NULL;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pParent = GetParentAnchor();
    
    //if there is an ancestor anchor, then the default action is its default action
    // BUGBUG - resourceString
    if ( pParent )
    {
        *pbstrDefaultAction = SysAllocString( _T("Jump") );

        if (!*pbstrDefaultAction )
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_NOTIMPL;
   
Cleanup:
   RRETURN( hr );
}

//----------------------------------------------------------------------------
//  accDoDefaultAction
//  
//  DESCRIPTION:
//  If the parent is anchor then calls the parent, otherwise returns E_NOTIMPL
//
//  PARAMETERS:
//      varChild            :   VARIANT child information
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccImage::accDoDefaultAction(VARIANT varChild)
{
    HRESULT      hr;
    CAccBase * pParent = NULL;

    hr = THR( ValidateChildID( &varChild ) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) != CHILDID_SELF )
        goto Cleanup;

    pParent = GetParentAnchor();

    //if ancestor anchor, then the default action is its default action
    hr = (pParent) ? THR( pParent->accDoDefaultAction( varChild ) ) : 
                     E_NOTIMPL;
   
Cleanup:
   RRETURN( hr );
}

