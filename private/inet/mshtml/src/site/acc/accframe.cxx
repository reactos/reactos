//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccFrame.Cxx
//
//  Contents:   Accessible Frame object implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ACCFRAME_HXX_
#define X_ACCFRAME_HXX_
#include "accframe.hxx"
#endif

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

ExternTag(tagAcc);

//+---------------------------------------------------------------------------
//
//  CAccFrame Constructor
//
//----------------------------------------------------------------------------
CAccFrame::CAccFrame( CDoc* pDocInner, CElement * pFrameElement)
: CAccWindow( pDocInner )
{
    _pFrameElement = pFrameElement;

    SetRole( ROLE_SYSTEM_CLIENT );
}

//----------------------------------------------------------------------------
//  accLocation
//
//  DESCRIPTION: Returns the coordinates of the frame relative to the 
//              root document coordinates
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccFrame::accLocation(   long* pxLeft, long* pyTop, 
                           long* pcxWidth, long* pcyHeight, 
                           VARIANT varChild)
{
    HRESULT hr;
    RECT    rectPos = {0};
    RECT    rectRoot = {0};

    Assert( pxLeft && pyTop && pcxWidth && pcyHeight );
    TraceTag((tagAcc, "CAccFrame::accLocation, childid=%d", V_I4(&varChild)));  

    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;

    // unpack varChild
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4( &varChild ) != CHILDID_SELF )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the root document coordinates
    if ( !_pDoc->GetRootDoc()->GetHWND( ) || 
            !::GetWindowRect( _pDoc->GetRootDoc()->GetHWND( ), &rectRoot ) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // get the frame's coordinates.
    if ( !_pDoc->GetHWND() ||
            !::GetWindowRect( _pDoc->GetHWND( ), &rectPos ) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    *pxLeft = rectPos.left - rectRoot.left;
    *pyTop =  rectPos.top - rectRoot.top;
    *pcxWidth = rectPos.right - rectPos.left - rectRoot.left;
    *pcyHeight = rectPos.bottom - rectPos.top - rectRoot.top;
    
Cleanup:    
    TraceTag((tagAcc, "CAccFrame::accLocation, Location reported as left=%d top=%d width=%d height=%d, hr=%d", 
                rectPos.left - rectRoot.left,
                rectPos.top - rectRoot.top,
                rectPos.right - rectPos.left - rectRoot.left,
                rectPos.bottom - rectPos.top - rectRoot.top,
                hr));
    RRETURN( hr );
}

STDMETHODIMP 
CAccFrame::get_accParent(IDispatch ** ppdispParent)
{
    HRESULT         hr;
    CAccElement *   pAccParent;


    TraceTag((tagAcc, "CAccFrame::get_accParent"));  

    if (!ppdispParent)
        RRETURN(E_POINTER);

    hr = THR(GetParentOfFrame(&pAccParent));
    if (hr)
        goto Cleanup;

    // Reference count and cast at the same time.
    hr  = THR( pAccParent->QueryInterface( IID_IDispatch, (void **)ppdispParent ) );

Cleanup:
    RRETURN( hr );
}

STDMETHODIMP 
CAccFrame::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT         hr;


    TraceTag((tagAcc, "CAccFrame::accNavigate navdir=%d varStart=%d",
                navDir,
                V_I4(&varStart)));  

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarEndUpAt ) = VT_EMPTY;

    switch ( navDir )
    {
        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
            // delegate to the super, since this is standard window operation
            RRETURN( super::accNavigate(navDir, varStart, pvarEndUpAt) );
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_NEXT:
            // if we are a frame/iframe document, then we should try to get to other 
            // elements that are possibly next to us. 
            if (CHILDID_SELF == V_I4(&varStart) && (!_pDoc->IsRootDoc()))
            {
                CAccElement *   pAccParent; // we know that we are a frame. the parent
                                            //  MUST be an element.
                CAccBase *      pAccChild = NULL;
                long            lIndex = 0;
                long            lChildIdx = 0;

                // get the parent document's body element accessible object.
                // We know that the parent is an element, since we are a frame.
                // Casting is OK.
                hr = THR(GetParentOfFrame(&pAccParent));
                if (hr)
                    goto Cleanup;

                hr = THR( pAccParent->GetNavChildId( navDir, _pFrameElement, &lIndex));
                if (hr) 
                    goto Cleanup;

                // now, ask the parent to return the child with that index, and return
                // that child as the sibling.
                hr = THR( pAccParent->GetChildFromID( lIndex, &pAccChild, NULL, &lChildIdx) );
                if ( hr )
                {
                    // NEXT and PREVIOUS can return E_INVALIDARG, which indicates that the
                    // index we passed to the function was out of limits. In that case, 
                    // spec asks us to return S_FALSE, and an empty variant.
                    if ( hr == E_INVALIDARG )
                        hr = S_FALSE;       
            
                    goto Cleanup;
                }

                // Prepare the return value according to the type of the data received
                // Either a child id or a pointer to the accessible child to be returned.
                if ( pAccChild )
                {
                    IDispatch * pDispChild;

                    //the child did have an accessible object

                    hr = pAccChild->QueryInterface( IID_IDispatch, (void **)&pDispChild);
                    if (hr) 
                        goto Cleanup;

                    V_VT( pvarEndUpAt ) = VT_DISPATCH;
                    V_DISPATCH( pvarEndUpAt ) = pDispChild;
                }
                else
                {
                    Assert((lIndex == -1) | (lIndex == lChildIdx));

                    //return the child id
                    V_VT( pvarEndUpAt ) = VT_I4;
                    V_I4( pvarEndUpAt ) = lChildIdx;
                }
            }
            else
                hr = S_FALSE;   // There is only one child for a frame, which is its pane
            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

HRESULT
CAccFrame::GetParentOfFrame( CAccElement ** ppAccParent )
{
    HRESULT     hr;
    CDoc *      pDocParent;

    Assert( ppAccParent );

    // an IFRAME can have another element as its parent
    if ( _pFrameElement->Tag() == ETAG_IFRAME)
        hr = THR(GetAccParent(_pFrameElement, (CAccBase **)ppAccParent));
    else
    {
        Assert(_pFrameElement->Tag() == ETAG_FRAME);

        // for a FRAME tag, since there may be nested framesets and we don't want to expose them,
        // we should get the root document's primary element client and return the accessible 
        // object for that element.

        // get the parent doc.
        pDocParent = _pDoc->_pDocParent;

        Assert( pDocParent );

        // if the parent document does not have an acc. obj, create one
        if ( !pDocParent->_pAccWindow )
        {
            pDocParent->_pAccWindow = new CAccWindow( pDocParent );
            if ( !pDocParent->_pAccWindow )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        // we have to return the accessible object for parent document's element client 
        hr = THR( pDocParent->_pAccWindow->GetClientAccObj( (CAccBase **)ppAccParent ) );
        if ( hr )
            goto Cleanup;
    }

Cleanup: 
    RRETURN(hr);
}
