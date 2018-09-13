//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccArea.Cxx
//
//  Contents:   Accessible Area object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCAREA_HXX_
#define X_ACCAREA_HXX_
#include "accarea.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

ExternTag(tagAcc);

//----------------------------------------------------------------------------
//  CAccArea
//  
//  DESCRIPTION:    
//      The area accessible object constructor
//
//  PARAMETERS:
//      Pointer to the area element 
//----------------------------------------------------------------------------
CAccArea::CAccArea( CElement* pElementParent )
:CAccElement(pElementParent)
{
    Assert( pElementParent );
    
    //initialize the instance variables
    SetRole( ROLE_SYSTEM_LINK );
}

//----------------------------------------------------------------------------
//  get_accParent
//  
//  DESCRIPTION:
//      Area elements have map elements as their parents in the tree. The map
//  elements have the body as their parent. In the accessibility code, the parent
//  for an area must be the image that contains that area, since the map element
//  is not supported and the area is really associated with the image and not the
//  body.
//  
//  Since there is no way for us to learn which area is associated with which 
//
//  BUGBUG: [FerhanE] 
//      The implementation here is a workaround. As a better solution we should 
//      somehow cache the relation between areas and images without increasing the
//      reference counts on either of these elements. The caching can be done in either
//      side. ( as an area list on an image, or an image list on an area. )
//      
//----------------------------------------------------------------------------
STDMETHODIMP    
CAccArea::get_accParent(IDispatch ** ppdispParent)
{
    HRESULT             hr;
    CTreeNode *         pNode = NULL;
    CTreeNode *         pParentNode = NULL;
    CCollectionCache *  pCollectionCache;
    long                lSize, l;
    CAccBase *          pAccParent = NULL;
    CMarkup *           pMarkup = NULL;


    TraceTag((tagAcc, "CAccArea::get_accParent"));      

    if ( !ppdispParent )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppdispParent = NULL;

    //
    // get to the element's node
    //
    pNode = _pElement->GetFirstBranch();

    // We have to have a tree node that is in the tree when we reach here
    if ( !pNode || pNode->IsDead())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // get the parent node.
    pParentNode = pNode->Parent();

    AssertSz( ( pParentNode->Tag()== ETAG_MAP ), 
                    "How can we not have a MAP Parent for an AREA?? !?!" );

    pMarkup = _pElement->GetMarkupPtr();

    // there has to be a MAP parent for an area.
    if ( !pParentNode || 
        ( pParentNode->Tag()!= ETAG_MAP ) || 
        !pMarkup)       // we can not count on the fact that we are in a markup at anytime.
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // We have the MAP element that contains the area, now find an image that 
    // is associated with this map.
    hr = THR( pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION) );
    if ( hr )
        goto Cleanup;
        
    pCollectionCache = pMarkup->CollectionCache();

    // Get the size of the doc's elements collection (0).
    lSize = pCollectionCache->SizeAry(CMarkup::ELEMENT_COLLECTION);

    for ( l=0; l< lSize; l++ )
    {
        CElement * pElemCandidate;

        WHEN_DBG( pElemCandidate = NULL; )

        hr = THR( pCollectionCache->GetIntoAry( CMarkup::ELEMENT_COLLECTION, l, &pElemCandidate ));
        if (hr)
            goto Cleanup;

        Assert( pElemCandidate );
        
        // if this element is an image and 
        // the image is associated with the map that is this area's parent, then we return
        // the element as the parent.
        if ( pElemCandidate->Tag() == ETAG_IMAGE) 
        {
            CMapElement * pMap;

            DYNCAST( CImgElement, pElemCandidate)->EnsureMap();

            pMap = DYNCAST( CImgElement, pElemCandidate)->GetMap();

            if ( (CElement *)pMap == pParentNode->Element() )
            {
                // get the accessible object of the image element 
                pAccParent = GetAccObjOfElement( pElemCandidate );

                if ( pAccParent )
                {
                    hr = THR(pAccParent->QueryInterface(IID_IDispatch, (void**)ppdispParent));
                }
                else
                    hr = E_OUTOFMEMORY;

                // return with the success code and the value for the parent
                goto Cleanup;
            }
        }
    }

    // if we did not return from the loop, it means that we could not find 
    // an image that contains this area. Return S_FALSE and NULL pointer for 
    // the parent value.
    hr = S_FALSE;

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccName
//  
//  DESCRIPTION:
//      Returns the title
//  
//  PARAMETERS:
//      pbstrName   :   BSTR pointer to receive the name
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccArea::GetAccName( BSTR* pbstrName)
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
    
    //get the title for the area.
    pchString = (LPTSTR) _pElement->GetAAtitle();
    if (!pchString)
    {
        // if there is no title return the alt text for the area.
        pchString = (LPTSTR) (DYNCAST(CAreaElement, _pElement))->GetAAalt();
    }

    if ( pchString )
    {
        *pbstrName = SysAllocString( pchString );
        if ( !(*pbstrName) )
            hr = E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetAccValue
//  
//  DESCRIPTION:
//      Returns the href of the area, which is stored in the CHyperLink
//  
//  PARAMETERS:
//      pbstrValue   :   BSTR pointer to receive the value
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP
CAccArea::GetAccValue( BSTR* pbstrValue)
{
    HRESULT hr;

    // validate out parameter
    if ( !pbstrValue )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrValue = NULL;

    hr = THR( (DYNCAST(CAreaElement,_pElement))->get_href( pbstrValue ) );

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDescription
//  
//  DESCRIPTION:
//      If the area has a shape, then appends the shape string to the string:
//      "link region type:"
//      The shape string can be "RECT", "CIRCLE" or "POLY"
//  
//  PARAMETERS:
//      pbstrDescription    :   BSTR pointer to receive the description
//  
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------    
STDMETHODIMP
CAccArea::GetAccDescription( BSTR* pbstrDescription)
{
    HRESULT hr;
    CStr    strShape;
    CStr    strBase;

    // validate out parameter
    if ( !pbstrDescription )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pbstrDescription = NULL;

    //get the shape string
    hr = THR( ((CAreaElement *)_pElement)->GetshapeHelper(&strShape));
    if ( hr )
        goto Cleanup;

    //if there was a shape string, then prepare the return string
    if ( strShape.Length() > 0 )
    {
        hr = THR( strBase.Set(_T("link region type: ")));
        if ( hr )
            goto Cleanup;

        hr = THR (strBase.Append( strShape ) );
        if ( hr )
            goto Cleanup;

        hr = THR(strBase.AllocBSTR(pbstrDescription));
    }

Cleanup:
    RRETURN( hr );

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
CAccArea::GetAccState( VARIANT *pvarState)
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
    
    if ( pDoc && (pDoc->_pElemCurrent == _pElement) && pDoc->HasFocus())
        V_I4( pvarState ) |= STATE_SYSTEM_FOCUSED;
    
    if ( !_pElement->IsVisible(FALSE) )
        V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;

Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  GetAccDefaultAction
//  
//  DESCRIPTION:
//  Returns the default action for  an area element, in a string. The default
//  action string is "jump"
//
//  PARAMETERS:
//      pbstrDefaultAction  :   BSTR pointer to receive the default action str.
//  
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccArea::GetAccDefaultAction( BSTR* pbstrDefaultAction)
{
    HRESULT hr = S_OK;

    if ( !pbstrDefaultAction )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDefaultAction = SysAllocString( _T("Jump") );

    if (!(*pbstrDefaultAction) )
        hr = E_OUTOFMEMORY;
   
Cleanup:
   RRETURN( hr );
}


//+------------------------------------------------------------------
//
//  Member : accLocation
//
//  Synopsis : virtual override. this method deals wit hteh specific 
//      positioning informatio nof areas. beause they are not in the 
//      tree/runs the same way as other elements all we really need to 
//      do here is get the offset properties and return that.  In addition
//      this is all the proxy did, so we will be consistent.
//-------------------------------------------------------------------
STDMETHODIMP 
CAccArea::accLocation(  long* pxLeft, long* pyTop, 
                        long* pcxWidth, long* pcyHeight, 
                        VARIANT varChild)
{
    HRESULT         hr;
    RECT            rectBound;
    CDoc *          pDoc = _pElement->Doc();
    IAccessible *   pAccImg = NULL;
   
    TraceTag((tagAcc, "CAccArea::accLocation, childid=%d", V_I4(&varChild)));  
    WHEN_DBG(rectBound.left = rectBound.top = rectBound.bottom = rectBound.right = 0);

    // validate out parameter
    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //reset out parameters
    *pxLeft = *pyTop =  *pcxWidth = *pcyHeight = 0;
    
    // unpack varChild, and validate the child id against child array limits.
    hr = THR(ValidateChildID(&varChild));
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) != CHILDID_SELF )
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    // we need to be in the active tree.
    if ( _pElement->IsInMarkup() && pDoc && pDoc->GetView()->IsActive() )
    {
        RECT rectImg = g_Zero.rc;       // left, top, bottom, right will map to
                                        // left, top, width, height for location of image

        // get the location relative to the top of the image the area is associated with
        DYNCAST(CAreaElement, _pElement)->GetBoundingRect(&rectBound);

        // get the image's coordinates to calculate the screen location.
        hr = THR(get_accParent((IDispatch **)&pAccImg));
        if (hr)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        Assert( pAccImg );

        // varChild has to contain CHILDID_SELF, otherwise we would not be here.
        hr = THR(pAccImg->accLocation( &rectImg.left, &rectImg.top, 
                                       &rectImg.bottom, &rectImg.right, varChild));
        if (hr)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // add the image coordinates to the area coordinates to get the screen coordinates
        *pyTop = rectBound.top + rectImg.top;
        *pxLeft = rectBound.left + rectImg.left;
        *pcxWidth = rectBound.right - rectBound.left;
        *pcyHeight = rectBound.bottom - rectBound.top;
    }

Cleanup:
    TraceTag((tagAcc, "CAccArea::accLocation, Location reported as left=%d top=%d width=%d height=%d, hr=%d", 
                rectBound.left,
                rectBound.top,
                rectBound.right - rectBound.left,
                rectBound.bottom - rectBound.top,
                hr));
    ReleaseInterface(pAccImg);
    RRETURN( hr ); 
}

