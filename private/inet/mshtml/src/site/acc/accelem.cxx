//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccElem.Cxx
//
//  Contents:   Accessible element implementation
//
//----------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

MtDefine(CAccElementaccLocation_aryRects_pv, Locals, "CAccElement::accLocation aryRects::_pv")
MtDefine(CAccElementget_accSelection_aryVariants_pv, Locals, "CAccElement::get_accSelection aryVariants::_pv")
DeclareTag(tagAcc, "Accessibility", "IAccessible Methods");

//
//we have some test code in the accLocation that needs this.
//
#if DBG==1
#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif
MtExtern(CFlowLayoutGetChildElementTopLeft_aryRects_pv);
#endif


//-----------------------------------------------------------------------
//  CAccElem::CAccElem()
//
//  DESCRIPTION:
//      Contructor. 
//
//  PARAMETERS:
//      pElementParent  CElement pointer to the element which this
//                      accessible class hangs from. 
//                      If this parameter is NOT provided.
// ----------------------------------------------------------------------
CAccElement::CAccElement( CElement* pElementParent ) 
: CAccBase(), _lastChild( pElementParent->Doc() )
{
    Assert( pElementParent );
    
    _pElement = pElementParent;         //cache the pointer

    //only if the element pointer is valid
    if ( _pElement )
    {
        //if this element already has an accessibility object.
        if ( _pElement->HasAccObjPtr() )
        {
            //this should never happen.
            AssertSz( FALSE, 
                      "The newly created acc obj was already listed in lookaside list" );
        
            _pElement->DelAccObjPtr();
        }

        _pElement->SetAccObjPtr(this);
    }
}

//----------------------------------------------------------------------------
//  CAccElement::~CAccElement()
//  
//  DESCRIPTION:
//      Destructor
//----------------------------------------------------------------------------
CAccElement::~CAccElement()
{
    //Since this destructor only gets called after the element deletes the 
    //lookaside pointer, we don't have to delete the lookaside pointer here.
    _pElement = NULL;    
}

//+---------------------------------------------------------------------------
//  PrivateAddRef
//  
//  DESCRIPTION
//      We overwrite the CBase implementation to be able to delegate the call 
//      to the element that we are connected to.
//----------------------------------------------------------------------------
ULONG
CAccElement::PrivateAddRef()
{
    Assert( _pElement );
    return _pElement->AddRef();
}

//+---------------------------------------------------------------------------
//  PrivateRelease

//  DESCRIPTION
//      We overwrite the CBase implementation to be able to delegate the call 
//      to the element that we are connected to.
//----------------------------------------------------------------------------
ULONG
CAccElement::PrivateRelease()
{
    Assert( _pElement );
    return _pElement->Release();
}


//----------------------------------------------------------------------------
//
//  IAccessible Interface Implementation.
//
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------
//  get_accParent()
//
//  DESCRIPTION:
//
//      Element implementation of the IAccessible::get_accParent method.
//      If the accessible object already has an accessible parent, that 
//      parent is returned. Otherwise the accessible element tree is 
//      walked and the parent is found.
//
//  PARAMETERS:
//
//      ppdispParent    :   address of the variable that will receive the
//                          parent pointer.
//
//  RETURNS:
//
//      E_POINTER | S_OK
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accParent(IDispatch ** ppdispParent)
{
    HRESULT     hr;
    CAccBase  * pBase = NULL;

    hr = GetAccParent( _pElement, &pBase);
    if (hr)
        goto Cleanup;

    hr = THR(pBase->QueryInterface(IID_IDispatch, (void**)ppdispParent));
  
Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  CAccElement::get_accChildCount()
//
//  DESCRIPTION:
//          Returns the number of accessible children that the object has.
//
//  PARAMETERS:
//
//      pChildCount :   Address of the variable to receive the child count
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accChildCount(long* pChildCount)
{
    HRESULT             hr = S_OK;
    
    if ( !pChildCount )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pChildCount = 0;

    if ( CanHaveChildren() )
    {
        // cache check.
        // since the IsCacheValid checks for the Doc() too, we don't have to
        // check for Doc being there here... 
        if ( !IsCacheValid() )
        {

            CMarkupPointer *    pBegin = &( GetAccDoc()->_pAccWindow->_elemBegin );
            CMarkupPointer *    pEnd = &( GetAccDoc()->_pAccWindow->_elemEnd );
        
            _lChildCount = 0;       // reset cached value

            // get markup limits for this element
            hr = THR( GetMarkupLimits( _pElement, pBegin, pEnd) );
            if ( hr )
                goto Cleanup;
            
            hr = THR( GetChildCount( pBegin, pEnd, &_lChildCount));
        }

        *pChildCount = _lChildCount;
    } 

    TraceTag((tagAcc, "CAccElement::get_accChildCount, role=%d, childcnt=%d hr=%d", GetRole(), _lChildCount, hr));

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  CAccElement::get_accChild()
//
//  DESCRIPTION:
//          Returns the number of accessible children that the object has.
//          The implementation here is called by all derived implementations
//          since it checks the parameters and performs initial work.
//
//  PARAMETERS:
//      varChild    :   Child information
//      ppdispChild :   Address of the variable to receive the child 
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK | S_FALSE
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
    HRESULT          hr;
    CAccBase *       pAccChild = NULL;

    TraceTag((tagAcc, "CAccElement::get_accChild, role=%d, childid=%d requested", 
                        GetRole(), 
                        V_I4(&varChild)));  

    // validate out parameter
    if ( !ppdispChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppdispChild = NULL;        //reset the return value.

    // unpack varChild, and validate the child id 
    hr = THR(ValidateChildID(&varChild));
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        //the object itself is not its own child
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    else 
    {
        //if there are no children, this function returns S_FALSE.
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            //if the child is text, then the client has to talk to 
            //the parent object to get to that child. So we have to
            //return an invalid return value to tell the client
            //that this acc obj. is the object to talk to for this 
            //child.
            hr = E_NOINTERFACE;

            TraceTag((tagAcc, 
                        "CAccElement::get_accChild, role=%d, childid=%d, text child indication returned", 
                        GetRole(), 
                        V_I4(&varChild)));  
        }
        else 
        {
            //assign return value.
            *ppdispChild = (IDispatch *)pAccChild;            

            //AddRef since this is going outside 
            pAccChild->AddRef();

            TraceTag((tagAcc, 
                        "CAccElement::get_accChild, role=%d, childid=%d, accChild=0x%x", 
                        GetRole(), 
                        V_I4(&varChild),
                        pAccChild));
        }
    }

Cleanup:

    RRETURN1( hr, S_FALSE );    //S_FALSE is valid when there is no children
}

//----------------------------------------------------------------------------
//  get_accName()
//
//  DESCRIPTION:
//  CAccElement class implementation of the get_accName. 
//  This method is not implemented in any of the derived classes. 
//  The derived classes implement the GetAccName() instead, to make 
//  the code simpler.
//  This implementation handles all child related checks, to let the 
//  GetAccName to only worry about the value to be returned, for the 
//  object that it is implemented on.
//
//  PARAMETERS:
//      varChild    :   Child information
//      pbstrName   :   pointer to the bstr to receive the name
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP
CAccElement::get_accName(VARIANT varChild, BSTR* pbstrName)
{
    HRESULT                 hr;
    CAccBase *              pAccChild = NULL;
    CMarkupPointer *        pMarkupText = NULL;
    MARKUP_CONTEXT_TYPE     context;
    long                    lchCnt =-1;

    TraceTag((tagAcc, "CAccElement::get_accName, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
     if ( !pbstrName )
     {
        hr= E_POINTER;
        goto Cleanup;
     }

    *pbstrName = NULL;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        //call this instance's GetAccName implementation
        hr = THR( GetAccName(pbstrName) );
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        pMarkupText = &( GetAccDoc()->_pAccWindow->_elemBegin );
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, pMarkupText) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            // the markup pointer is actually pointing after the text now. 
            // get the number of characters available for reading.
            hr = THR( pMarkupText->Left( TRUE, &context, NULL, &lchCnt, NULL, NULL) );
            if ( hr )
                goto Cleanup;

            Assert( context == CONTEXT_TYPE_Text );   //just checking.. !

            //allocate buffer
            hr = FormsAllocStringLen ( NULL, lchCnt, pbstrName );
            if ( hr )
                goto Cleanup;

            //read text
            hr = THR( pMarkupText->Right( TRUE, &context, NULL, &lchCnt, *pbstrName, NULL));
            if ( hr )
            {
                // release the BSTR and reset the pointer if we have failed to get
                // the text from tree services.
                FormsFreeString( *pbstrName );
                *pbstrName = NULL;
            }    
        }
        else 
        {
            V_I4( &varChild ) = CHILDID_SELF;
            
            //call child's get_accName implementation
            hr = THR( pAccChild->get_accName( varChild, pbstrName) );
        }
    }

Cleanup:

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  DESCRIPTION:
//  CAccElement class implementation of the get_accValue. This method is not 
//  implemented in any of the derived classes. The derived classes implement
//  the GetAccValue() instead, to make the code simpler.
//
//  PARAMETERS:
//      varChild    :   Child information
//      pbstrValue  :   pointer to the bstr to receive the value
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accValue(VARIANT varChild, BSTR* pbstrValue)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;

    TraceTag((tagAcc, "CAccElement::get_accValue, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
     if ( !pbstrValue )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrValue = NULL;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        //call this instance's implementation
        hr = THR( GetAccValue(pbstrValue) );
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            //if the parent is anchor, then anchor's value, otherwise
            //not implemented
            if ( _pElement->Tag() == ETAG_A )
            {
                //call instance's implementation
                hr = THR( GetAccValue(pbstrValue) );
            }
            else
            {
                hr = E_NOTIMPL;
            }
        }
        else 
        {
            V_I4( &varChild ) = CHILDID_SELF;
            
            //call child's GetAccName implementation
            hr = THR( pAccChild->get_accValue( varChild, pbstrValue) );
        }
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  DESCRIPTION:
//  CAccElement class implementation of the get_accDescription. This method is not 
//  implemented in any of the derived classes. The derived classes implement
//  the GetAccDescription() instead, to make the code simpler.
//
//  PARAMETERS:
//      varChild    :   Child information
//      pbstrDescription  :   pointer to the bstr to receive the description
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accDescription(VARIANT varChild, BSTR* pbstrDescription)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;

    TraceTag((tagAcc, "CAccElement::get_accDescription, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
     if ( !pbstrDescription )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrDescription = NULL;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        //call this instance's implementation
        hr = THR( GetAccDescription(pbstrDescription) );
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            // child is text.
            hr = E_NOTIMPL;
        }
        else 
        {
            V_I4( &varChild ) = CHILDID_SELF;

            //call child's implementation
            hr = THR( pAccChild->get_accDescription( varChild, pbstrDescription) );
        }
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//-----------------------------------------------------------------------
//  CAccElement::get_accRole()
//
//  DESCRIPTION:
//          Returns the accessibility role of the object.
//
//  PARAMETERS:
//
//      varChild    :   Variant that contains the child information
//      pvarRole    :   Address of the variant to receive the role information
//
//  RETURNS:
//
//      E_INVALIDARG | S_OK
//
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT         hr ;      
    long            lRetRole = 0;
    CAccBase *      pAccChild = NULL;

    TraceTag((tagAcc, "CAccElement::get_accRole, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate the out parameter
    if ( !pvarRole )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // clear the out parameter
    V_VT( pvarRole ) = VT_EMPTY;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        lRetRole = GetRole();   //this acc. objects role
    }
    else 
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            lRetRole = ROLE_SYSTEM_TEXT;
        }
        else 
        {
            //call child's implementation
            lRetRole = pAccChild->GetRole();
        }
    }
       
    if ( hr == S_OK )
    {
        // pack role into out parameter
        V_VT( pvarRole ) = VT_I4;
        V_I4( pvarRole ) = lRetRole;
    }
    
Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accState()
//
//  DESCRIPTION:
//  CAccElement class implementation of the get_accState. This method is not 
//  implemented in any of the derived classes. The derived classes implement
//  the GetAccState() instead, to make the code simpler.
//
//  PARAMETERS:
//      varChild    :   Child information
//      pvarState   :   pointer to the VARIANT to receive the state information
//
//  RETURNS:    
//      E_INVALIDARG | S_OK | 
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT             hr;
    CAccBase *          pAccChild = NULL;
    CMarkupPointer *    pTextBegin = NULL;
    CMarkupPointer *    pTextEnd = NULL;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;

    CSelectionObject    selectionObject(_pElement->Doc());
    htmlSelection       selectionType;
    IDispatch *         pDispRange = NULL;

    TraceTag((tagAcc, "CAccElement::get_accState, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
     if ( !pvarState )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    // reset the out parameter
    V_VT( pvarState ) = VT_EMPTY;
    
    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {   

        hr = THR( GetAccState(pvarState) );

        // a pane is always visible.
        if (ROLE_SYSTEM_PANE != GetRole())
        {
            CLayout *           pLayout;
            CDataAry <RECT>     aryRects( Mt(CAccElementaccLocation_aryRects_pv) );
            CRect               rectElement;
            CDoc *              pDoc = _pElement->Doc();

            // bodies and framesets should be treated like any other element
            // get the closest layout.
            pLayout = _pElement->GetUpdatedNearestLayout();
            if ( !pLayout )
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            // get the region 
            if (ETAG_AREA != _pElement->Tag())
            {
                pLayout->RegionFromElement( _pElement, &aryRects, &rectElement, RFE_SCREENCOORD);
            }
            else if (_pElement->IsInMarkup() && pDoc && pDoc->GetView()->IsActive())
            {
                DYNCAST(CAreaElement, _pElement)->GetBoundingRect(&rectElement);
            }
            else
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            // if the element is out of the client area, it is not visible.
            if (!IsVisibleRect(&rectElement))
                V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE; 
        }
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        pTextBegin = &(GetAccDoc()->_pAccWindow->_elemBegin);

        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, pTextBegin) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            CAccBase *  pParentA = (_pElement->Tag() == ETAG_A) ? this : GetParentAnchor();

            //if we are an anchor, assume its state
            if (pParentA)
            {
                hr = THR( DYNCAST(CAccElement, pParentA)->GetAccState( pvarState ) );
                if ( hr )
                    goto Cleanup;
            }
            else 
            {
                V_VT( pvarState ) = VT_I4;
                V_I4( pvarState ) = 0;
            }
            
            //always READONLY
            V_I4( pvarState ) |= STATE_SYSTEM_READONLY;

            if ( fBrowserWindowHasFocus() )
                V_I4( pvarState ) |= STATE_SYSTEM_SELECTABLE;

            CTreeNode * pParentNode = pTextBegin->CurrentScope();
            
            if (pParentNode->IsDisplayNone() || 
            	pParentNode->IsVisibilityHidden())
                V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE;

            //pTextBegin is already pointing to the end of the text. 
            pTextEnd = &(GetAccDoc()->_pAccWindow->_elemEnd);
            hr = THR( pTextEnd->MoveToPointer( pTextBegin ) );
            if ( hr )
                goto Cleanup;

            //move the pointer to mark the beginning of the child.
            hr = THR( pTextBegin->Left( TRUE, &context, NULL, NULL, NULL, NULL) );
            if ( hr )
                goto Cleanup;

            // Is text selected ? If any text that is inside this text child is selected, then
            // we consider this text child as selected.
            hr = THR( selectionObject.GetType( &selectionType ) );

            if ( (selectionType != htmlSelectionNone) && _pElement->Doc() ) 
            {
                CMarkupPointer      selBegin( _pElement->Doc() );
                CMarkupPointer      selEnd( _pElement->Doc() );
                CAutoRange *        pRange = NULL;

                // Get the range that is selected from the selection object
                hr = THR( selectionObject.createRange(&pDispRange) );
                if ( hr )
                    goto Cleanup;

                // The selection range is always there, even if there is no selection
                Assert( pDispRange );

                hr = THR( pDispRange->QueryInterface( CLSID_CRange, (void **)&pRange ) );
                if ( hr )
                    goto Cleanup;

                Assert( pRange );

                // place the left and right markups to the txtrange left and right
                hr  = THR( pRange->GetLeftAndRight( &selBegin, &selEnd ) );
                if ( hr )
                    goto Cleanup;

                Assert( context == CONTEXT_TYPE_Text );

                // if the selection is NOT totally out of this element, then the state
                // has to be STATE_SYSTEM_SELECTED.
       
                // if there is a selection, it should somehow be inside or overlapping with the
                // text area. 
                // 
                if ( selEnd.IsRightOf( pTextBegin ) && selBegin.IsLeftOf( pTextEnd ) )
                    V_I4( pvarState ) |= STATE_SYSTEM_SELECTED; //hr is S_OK here.
            }

            //visibility of the text depends on the following two things to be TRUE
            //1- Is the closest element parent of the text visible?
            //2- Is the text within the client window coordinates ?
            //
            if (!IsTextVisible(pTextBegin, pTextEnd))
            {
                V_I4( pvarState ) |= STATE_SYSTEM_INVISIBLE; 
            }
        }
        else 
        {
            V_I4( &varChild ) = CHILDID_SELF;

            //call child's implementation
            hr = THR( pAccChild->get_accState(varChild, pvarState) );
        }
    }

Cleanup:
    ReleaseInterface( pDispRange );

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accKeyboardShortCut
//  
//  DESCRIPTION :   
//          Returns the keyboard shortcut if there is one.
//
//  PARAMETERS:
//      varChild                :   VARIANT containing the child ID
//      pbstrKeyboardShortcut   :   address of the bstr to receive data
//
//  RETURNS:
//      E_POINTER | S_OK | E_NOTIMPL | E_OUTOFMEMORY
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;
    CStr            accessString;
    CStr            sString;    

    TraceTag((tagAcc, "CAccElement::get_accKeyboardShortcut, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
     if ( !pbstrKeyboardShortcut )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrKeyboardShortcut = NULL;
    
    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        switch (_pElement->Tag())
        {
            case ETAG_INPUT:
            case ETAG_BUTTON:
            case ETAG_A:
            case ETAG_SELECT:
                // get the actual key combination value
                hr = THR (accessString.Set( _pElement->GetAAaccessKey() ) );
                if ( hr )
                    goto Cleanup;

                // if there is an access key string
                if ( accessString.Length() > 0 )
                {
                    // we want all keyboard shortcut values to contain 'Alt+' 
                    hr = THR( sString.Set( _T("Alt+") ) );
                    if ( hr )
                        goto Cleanup;
                    
                    hr = THR( sString.Append( accessString ) );
                    if ( hr )
                        goto Cleanup;
                        
                    hr = THR( sString.AllocBSTR( pbstrKeyboardShortcut ) );
                }
                
                break;
                
            default:
                hr = S_OK;
                break;
        }
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            //no keyboard shortcuts for plain text, unless we (or a parent is an
            // anchor)
            CAccBase * pParentA = (_pElement->Tag() == ETAG_A) ? this : GetParentAnchor();
            CVariant   varChildSelf;

            V_VT(&varChildSelf) = VT_I4;
            V_I4(&varChildSelf) = CHILDID_SELF;

            //if we are an anchor, assume its state
            hr = (!pParentA) ? S_OK :
                THR( pParentA->get_accKeyboardShortcut(varChildSelf, pbstrKeyboardShortcut) );
        }
        else 
        {
            // call child's implementation of this method. We don't have a helper here,
            // since the CHILDID_SELF case above has all the smarts and code.
            // varChild already is of type VT_I4, so only set the value.
            V_I4( &varChild ) = CHILDID_SELF;
            hr = THR( pAccChild->get_accKeyboardShortcut(varChild, pbstrKeyboardShortcut) );
        }
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  get_accFocus
//
//  DESCRIPTION :   Gets the active element on the current document and 
//                  checks if this acc object's element is the same one.
//
//  PARAMETERS:
//      pvarFocusChild  :   address of VARIANT to receive the focused child info
//
//  RETURNS:
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accFocus(VARIANT * pvarFocusChild)
{
    HRESULT         hr = S_OK;
    CElement *      pElemFocus = NULL;
    CAccBase *      pAccFocus = NULL;
    IDispatch *     pAccParent = NULL;
   
    TraceTag((tagAcc, "CAccElement::get_accFocus, role=%d, childid=%d", GetRole()));  

    if ( !pvarFocusChild )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    V_VT( pvarFocusChild ) = VT_EMPTY;

    //using GetAccDoc saves us from reimplementing this on window.
    pElemFocus = GetAccDoc()->_pElemCurrent;

    //is the active element this one?
    if ( _pElement == pElemFocus )
    {
        V_VT( pvarFocusChild ) = VT_I4;
        V_I4( pvarFocusChild ) = CHILDID_SELF;
        goto Cleanup;
    }
    else
    {
        // if this object is the accessible parent of the element that
        // has the focus, then return the acc object for that element
        
// ???? - look into area's & images when the area has the focus
        pAccFocus = GetAccObjOfElement(pElemFocus);
        if ( !pAccFocus )
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // Get the parent accessible object for the element that has the 
        // focus. We have to call through interface, since the pAccFocus element
        // can be a body or a frame set.
        hr = THR( pAccFocus->get_accParent( &pAccParent ) );
        if ( hr )
            goto Cleanup;

        Assert( pAccParent );

        // if these two objects are the same, then this acc object instance
        // is the parent for the acc object that is attached to the element
        // that currently has the focus. We return the acc object for the 
        // element that has the focus
        if ( pAccParent == DYNCAST( IAccessible, this) )  
        {
            V_VT( pvarFocusChild ) = VT_DISPATCH;
            V_DISPATCH( pvarFocusChild ) = pAccFocus;
            pAccFocus->AddRef();
        }
        else
        {
            V_VT(pvarFocusChild) = VT_EMPTY;
        }
    }
    
Cleanup:    
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//  get_accSelection
//  
//  DESCRIPTION:
//      This is the method implementation for all HTML elements except for the
//      body/frameset tags. The body/frameset implementation delegates the call
//      to the frame that has the focus.
//      This method returns an acc object, or a pointer to the IEnumVariant
//      that represents an array of acc. objects, that is/are intersecting with
//      the selection on the page.
//      
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accSelection(VARIANT * pvarSelectedChildren)
{
    HRESULT             hr;
    CSelectionObject    selectionObject(_pElement->Doc());
    htmlSelection       selectionType;
    IDispatch *         pDispRange = NULL;    

    TraceTag((tagAcc, "CAccElement::get_accSelection, role=%d, childid=%d", GetRole()));  

    if ( !pvarSelectedChildren )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // default the return value to no selection.
    V_VT(pvarSelectedChildren) = VT_EMPTY;  

    // we can not return selected children , if there are no children.
    if ( !CanHaveChildren() )
    {
        hr = S_OK;
        goto Cleanup;
    }


    // Is text selected ? If any text that is inside this text child is selected, then
    // we consider this text child as selected.
    hr = THR( selectionObject.GetType( &selectionType ) );
    if (hr)
        goto Cleanup;

    if ( (selectionType != htmlSelectionNone) && _pElement->Doc() ) 
    {
        CAutoRange *        pRange = NULL;
        MARKUP_CONTEXT_TYPE context;
        CMarkupPointer      selStart( _pElement->Doc() );
        CMarkupPointer      selEnd( _pElement->Doc() );
        CMarkupPointer      elemStart( _pElement->Doc() );
        CMarkupPointer      elemEnd( _pElement->Doc() );
        long                cChildBase = 0;              

//BUGBUG: [FerhanE]
//          Using the range object is very expensive for us, since it 
//          is created using markups and we have to ask for them again.
//          However, since the editing and selection is possibly moving out
//          of the native code base, we have to use interface calls.
        // Get the range that is selected from the selection object
        hr = THR( selectionObject.createRange(&pDispRange) );
        if ( hr )
            goto Cleanup;

        // if there is no selection, there is nothing to do, we have already 
        // set the return value to VT_EMPTY by clearing the out parameter.
        if ( !pDispRange )
            goto Cleanup;
   
        hr = THR( pDispRange->QueryInterface( CLSID_CRange, (void **)&pRange ) );
        if ( hr )
            goto Cleanup;

        Assert( pRange );

        // place the left and right markups to the txtrange left and right
        hr  = THR( pRange->GetLeftAndRight( &selStart, &selEnd ) );
        if ( hr )
            goto Cleanup;

        // move the pSelStart pointer to the closest tag to its left, 
        // to ease our lives for child counting. Any child that is partially
        // selected is assumed to be selected.
        hr = THR( selStart.Left( FALSE, &context, NULL, NULL, NULL, NULL ));
        if (hr) 
            goto Cleanup;

        if ( context == CONTEXT_TYPE_Text )
        {   
            hr = THR( selStart.Left( TRUE, &context, NULL, NULL, NULL, NULL ) );
            if ( hr ) 
                goto Cleanup;
        }
    
        // place markups to beginning and end of the element too.
        hr = THR( GetMarkupLimits(_pElement, &elemStart, &elemEnd) );
        if ( hr ) 
            goto Cleanup;

        // Check if the selection is completely out of this element's scope.
        if ( selStart.IsRightOf( &elemEnd ) || selEnd.IsLeftOf( &elemStart ) )
            goto Cleanup;       //hr is S_OK here.
        
        // If the selection starts before the element, we should make it start
        // with the element, to ease our calculations later.
        if ( selStart.IsLeftOf( &elemStart ) )
        {
            // Move the selection pointer to the start of element
            hr = THR( selStart.MoveToPointer( &elemStart ) );
            if ( hr )
                goto Cleanup;
        }

        // If the selection ends after the element, make it end with the element
        // to ease calculations.
        if ( elemEnd.IsLeftOf( &selEnd ) )
        {
            // Move the selection pointer to the end of element
            hr = THR( selEnd.MoveToPointer( &elemEnd ) );
            if ( hr )
                goto Cleanup;
        }

        // Count the number of children from the beginning of the element's scope,
        // until the selection starts.
        hr = THR( GetChildCount( &elemStart, &selStart, &cChildBase) );
        if ( hr ) 
            goto Cleanup;

        // GetChildCount moves the pElemStart in the markup stream as it counts children.
        // If the pElemStart is further than the selStart in the markup stream, then it means
        // that the selection starts in a supported element. We should move the selection begin
        // point to the beginning of that element and decrement the child count.
        if ( selStart.IsLeftOf( &elemStart)  )
        {   
            CTreeNode * pNode = NULL;

            pNode = selStart.CurrentScope();

            while ( !IsSupportedElement(pNode->Element()) 
    #if DBG ==1 
                    && ( _pElement != pNode->Element() )   //security check
    #endif                
                   )
            {
                pNode = pNode->Parent();
            }
        
            //security check continues
            Assert( _pElement != pNode->Element() );

            // move the selection start to the beginning of the supported element
            hr = THR( selStart.MoveAdjacentToElement( pNode->Element(), ELEM_ADJ_BeforeBegin));
            if ( hr )
                goto Cleanup;
        
            //decrement the child base, since we took a step backwards.
            cChildBase--;
        }

        // count and retrieve the children starting from the selStart position, 
        // upto and incuding the element that contains the selEnd position
        hr = THR( GetSelectedChildren( cChildBase, 
                                        &selStart, &selEnd, 
                                        pvarSelectedChildren ));
    }

Cleanup:
    ReleaseInterface( pDispRange );
    
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  get_accDefaultAction
//
//  DESCRIPTION:
//      CAccElement implementation for the get_accDefaultAction method. Each
//      object that is derived from this class overwrites this method's helper
//      function GetAccDefaultAction. This implementation is only responsible for
//      manipulating the child id and processing the call in case the child
//      id that is passed refers to text.
//      
//  PARAMETERS:
//      varChild            :   VARIANT that contains the child information
//      pbstrDefaultAction  :   Address of the BSTR to receive the default 
//                              action string.
//      
//  RETURNS:
//          S_OK | E_INVALIDARG | E_POINTER    
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;

    TraceTag((tagAcc, "CAccElement::get_accDefaultAction, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
     if ( !pbstrDefaultAction )
     {
        hr = E_POINTER;
        goto Cleanup;
     }

    *pbstrDefaultAction = NULL;

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        //call this instance's GetAccDefaultAction implementation
        hr = THR( GetAccDefaultAction(pbstrDefaultAction) );
    }
    else
    {
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            //if the text is inside an anchor, then it reflects
            //anchors default action
            if ( _pElement->Tag() ==ETAG_A )
            {
                hr = THR( GetAccDefaultAction(pbstrDefaultAction) );
            }
            else
            {
                //there is no default action for text.
                hr = E_NOTIMPL;
            }
        }
        else 
        {
            V_I4( &varChild ) = CHILDID_SELF;

            //call child's GetAccDefaultAction implementation
            hr = THR( pAccChild->get_accDefaultAction( varChild, pbstrDefaultAction) );
        }
    }

Cleanup:
    RRETURN( hr );

}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::accSelect( long flagsSel, VARIANT varChild)
{
    HRESULT             hr;
    CAccBase *          pAccChild = NULL;
    CAccBase *          pAccParent = NULL;
    CElement *          pElemFocus = _pElement;

    TraceTag((tagAcc, "CAccElement::accSelect, role=%d, childid=%d, flagsSel=0x%x", 
                GetRole(), 
                V_I4(&varChild),
                flagsSel));  

    // validate the child,
    hr = THR( ValidateChildID( &varChild ));
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {   
        // if the element is an IMG 
        if ( _pElement->Tag() == ETAG_IMG )
        {
            // we can call the helper, since we know that the 
            // element is not a body/frameset
            hr = GetAccParent( _pElement, &pAccParent);
            if ( hr )
                goto Cleanup;

            Assert( pAccParent );

            //  If the parent acc. obj is an anchor, call
            //  the standard implementation. otherwise E_NOTIMPL
            if ( pAccParent->GetElement()->Tag() != ETAG_A )
            {
                hr =E_NOTIMPL;
                goto Cleanup;
            }

            pElemFocus = pAccParent->GetElement();
            
            //fall through if the parent is anchor.
        }

        // For all elements, including the body/frameset, the behavior is to
        // set the focus to itself
        if ( flagsSel & SELFLAG_TAKEFOCUS )
        {
            hr = ScrollIn_Focus( pElemFocus );
        }
        else
        {
            hr = E_INVALIDARG;
        }
                
    }
    else
    {
        CMarkupPointer *    ptrBegin = &( GetAccDoc()->_pAccWindow->_elemBegin );

        // Find the child indicated with the ID.
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, ptrBegin) );
        if ( hr ) 
            goto Cleanup;

        if ( pAccChild )
        {
            // Go through the interface method, to support frames
            V_I4( &varChild ) = CHILDID_SELF;
            
            hr = pAccChild->accSelect(flagsSel, varChild);
        }
        else
        {
            //text 
            
            // the child is a text block.
            switch ( flagsSel )
            {
                // set focus to the parent element.
                case SELFLAG_TAKEFOCUS:
                    hr = ScrollIn_Focus( _pElement );
                    break;

                case SELFLAG_TAKESELECTION:
                    IMarkupPointer  * pIMarkup;
                    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;

                    hr = THR(ptrBegin->Left(TRUE, &context, NULL, NULL, NULL, NULL));
                    if (hr)
                        goto Cleanup;

                    hr = THR( ptrBegin->QueryInterface( IID_IMarkupPointer, 
                                                                (void**)&pIMarkup ));
                    if ( hr )
                        goto Cleanup;
                        
                    hr = THR( SelectText( _pElement, pIMarkup ) );

                    // Release the instance we got from the QI call.
                    ReleaseInterface( pIMarkup );
                    
                    break;
            }
        }
    }
    
Cleanup:
    RRETURN( hr );
}


//----------------------------------------------------------------------------
//  accLocation()
//  
//  DESCRIPTION:
//      Returns the coordinates of the element relative to the top left corner 
//      of the client window.
//      To do that, we are getting the CLayout pointer from the element
//      and calling the GetRect() method on that class, using the global coordinate
//      system. This returns the coordinates relative to the top left corner of
//      the screen. 
//      We then convert these screen coordinates to client window coordinates.
//  
//  PARAMETERS:
//        pxLeft    :   Pointers to long integers to receive coordinates of
//        pyTop     :   the rectangle.
//        pcxWidth  :
//        pcyHeight :
//        varChild  :   VARIANT containing child information. 
//
//  RETURNS
//      S_OK | E_INVALIDARG
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::accLocation(   long* pxLeft, long* pyTop, 
                            long* pcxWidth, long* pcyHeight, 
                            VARIANT varChild)
{
    HRESULT             hr;
    CLayout *           pLayout = NULL;
    CRect               rectElement;
    CRect               rectWnd;
    HWND                hWnd;
    CAccBase *          pAccChild = NULL;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    CMarkupPointer *    pBegin = NULL;
    CMarkupPointer *    pEnd = NULL;
    CDataAry <RECT>     aryRects( Mt(CAccElementaccLocation_aryRects_pv) );


    TraceTag((tagAcc, "CAccElement::accLocation, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // validate out parameter
    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // reset out parameters
    *pxLeft = *pyTop =  *pcxWidth = *pcyHeight = 0;
    
    // unpack varChild, and validate the child id 
    hr = THR(ValidateChildID(&varChild));
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        // bodies and framesets should be treated like any other element
        // get the closest layout.
        pLayout = _pElement->GetUpdatedNearestLayout();
        if ( !pLayout )
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // get the region 
        pLayout->RegionFromElement( _pElement, &aryRects, &rectElement, RFE_SCREENCOORD);

        // we fall into the offset calculation code placed after the 'if'.
    }
    else 
    {

        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        pBegin = &(GetAccDoc()->_pAccWindow->_elemBegin );
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, pBegin) );
        if ( hr ) 
            goto Cleanup;

        // If there is an accessible child, delegate the call, otherwise get the 
        // location for the text child.
        if ( pAccChild )
        {
            // call child's implementation of this method. We don't have a helper here,
            // since the CHILDID_SELF case above has all the smarts and code.
            V_I4( &varChild ) = CHILDID_SELF;

            // Delegate to the accessible child.
            hr = THR( pAccChild->accLocation( pxLeft, pyTop, 
                                              pcxWidth, pcyHeight, 
                                              varChild ) );

            // We are done here, skipping over the offset calculation
            goto Cleanup;
        }
        else        
        {
            pEnd = &(GetAccDoc()->_pAccWindow->_elemEnd );
            hr = THR( pEnd->MoveToPointer( pBegin ) );
            if ( hr ) 
                goto Cleanup;

            // move the begin pointer to where it should be
            hr = THR( pBegin->Left( TRUE, &context, (CTreeNode **) NULL, NULL, NULL, NULL ) );
            if ( hr )
                goto Cleanup;

            Assert( context == CONTEXT_TYPE_Text );

            hr = THR( _pElement->Doc()->RegionFromMarkupPointers(   pBegin, 
                                                                    pEnd,
                                                                    &aryRects, 
                                                                    &rectElement ));
            if ( hr ) 
                goto Cleanup;

        }
    }

    // Offset calculation...

    // the window handle may come back NULL, if the window is not in place activated
    hWnd = _pElement->Doc()->GetHWND();
    if ( !hWnd )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if ( !GetWindowRect( hWnd, &rectWnd) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pxLeft = rectElement.left + rectWnd.left;
    *pyTop = rectElement.top + rectWnd.top;
    *pcxWidth = rectElement.right - rectElement.left;
    *pcyHeight = rectElement.bottom - rectElement.top;

    TraceTag((tagAcc, "CAccElement::accLocation, Location reported as left=%d top=%d width=%d height=%d", 
                rectElement.left + rectWnd.left,
                rectElement.top + rectWnd.top,
                rectElement.right - rectElement.left,
                rectElement.bottom - rectElement.top));

Cleanup:
    RRETURN( hr );    
}


//----------------------------------------------------------------------------
//  accNavigate
//
//  DESCRIPTION:
//      Provides navigation for the accessible children of the accessible object
//      it is called on.
//      The navigation is provided according to the schema below:
//      
//                          start=self      start = id
//      NAVDIR_NEXT     :   err                 OK
//      NAVDIR_PREV     :   err                 OK
//      NAVDIR_FIRST    :   OK                  err
//      NAVDIR_LAST     :   OK                  err
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT                 hr;
    CAccBase *              pAccChild = NULL;
    long                    lIndex = 0;
    long                    lChildIdx = 0;
    CAccElement *           pAccObj = this;


    TraceTag((tagAcc, "CAccElement::accNavigate, role=%d, start=%d, direction=%d", 
                GetRole(), 
                V_I4(&varStart),
                navDir));  

    if ( !pvarEndUpAt )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // unpack varChild, and validate the child id against child array limits.
    hr = THR(ValidateChildID(&varStart));
    if ( hr )
        goto Error;

    switch ( navDir )
    {
        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
            // An acc object can only return its own first and last children.
            if ( V_I4(&varStart) != CHILDID_SELF )
            {
                hr = E_INVALIDARG;
                goto Error;
            }

            if ( navDir == NAVDIR_FIRSTCHILD )
                lIndex = 1;                 //get the first child
            else        
                lIndex = -1;                //get the last child
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_NEXT:
            // check the varStart. 
            // If the varStart is pointing to the self, then we have to delegate
            // the call to this element's accessible parent.
            // Otherwise, we have to return the child that is being asked from us.
            if ( V_I4(&varStart) == CHILDID_SELF )
            {
                 // If we are in a pane, since a window can have only one pane child
                // the navigation should fail, regardless of the direction.
                if (ROLE_SYSTEM_PANE == GetRole())
                {
                    Assert( (ETAG_BODY == _pElement->Tag())||
                            (ETAG_FRAMESET == _pElement->Tag()) );

                    hr = S_FALSE;       // no error, just no more children
                    goto Error;
                }

                // get the accessible parent element, we know for sure that the parent
                // is an element here, since we are not a pane. So casting is OK.
                hr = THR(GetAccParent(_pElement, (CAccBase **)&pAccObj));
                if (hr)
                     goto Error;

                // get the index of the element.
                hr = THR(pAccObj->GetNavChildId(navDir, _pElement, &lIndex));
                if (hr)
                    goto Error;
            }
            else
            {
                // the upper limit checks for the validity of indexes for these navdirs
                // are left to the GetChildFromID call below.
                if ( navDir == NAVDIR_PREVIOUS )
                {
                    //we can not go left from the first child
                    if ( V_I4(&varStart) == 1 )
                    {   
                        hr = S_FALSE;
                        goto Error;
                    }

                    // Get the index of the previous child
                    lIndex = V_I4(&varStart) - 1;
                }
                else    //NAVDIR_NEXT
                {
                    lIndex = V_I4(&varStart) + 1;
                }
            }
            break;

        // we don't support any other navigation commands
        default:
            hr = E_INVALIDARG;
            goto Error;
    }

    // get the child
    hr = THR( pAccObj->GetChildFromID( lIndex, &pAccChild, NULL, &lChildIdx) );
    if ( hr )
    {
        // FIRST and LAST can only return S_FALSE, if there are no children.
        // NEXT and PREVIOUS can return E_INVALIDARG, which indicates that the
        // index we passed to the function was out of limits. In that case, 
        // spec asks us to return S_FALSE, and an empty variant.
        if ( hr == E_INVALIDARG )
            hr = S_FALSE;       
            
        goto Error;
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
        Assert ((lIndex == -1) || (lIndex == lChildIdx));

        //return the child id
        V_VT( pvarEndUpAt ) = VT_I4;
        V_I4( pvarEndUpAt ) = lChildIdx;
    }

Cleanup:
    RRETURN1( hr, S_FALSE );
Error:
    V_VT( pvarEndUpAt ) = VT_EMPTY;
    RRETURN1( hr, S_FALSE );
}

//-----------------------------------------------------------------------
//  accHitTest()
//  
//  DESCRIPTION :   Since the window already have checked the coordinates
//                  and decided that the document contains the point, this
//                  function does not do any point checking. 
//                  
//  PARAMETERS  :
//      xLeft, yTop         :   (x,y) coordinates 
//      pvarChildAtPoint    :   VARIANT pointer to receive the acc. obj.
//
//  RETURNS:    
//      S_OK | E_INVALIDARG | 
//-----------------------------------------------------------------------
STDMETHODIMP 
CAccElement::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    HRESULT         hr = E_FAIL;
    CTreeNode *     pElemNode = NULL;
    CTreeNode *     pHitNode = NULL;
    CTreeNode *     pTreeWalker = NULL;
    CTreeNode *     pLastSupported = NULL;
    CAccBase  *     pAccObj = NULL;
    ELEMENT_TAG     elementTag;

    POINT       pt = {xLeft, yTop};
    HTC         htc;
    CMessage    msg;

    TraceTag((tagAcc, "CAccElement::accHitTest, role=%d", GetRole()));  

    if ( !pvarChildAtPoint )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    V_VT( pvarChildAtPoint ) = VT_EMPTY;
    V_I4( pvarChildAtPoint ) = 0;
    
    //get this element's node
    pElemNode = _pElement->GetFirstBranch();
    if ( !pElemNode )
        goto Cleanup;       //the node was not in the tree.

    // make sure that the window that contains the document is in-place
    // activated.
    if ( !_pElement->Doc()->GetHWND() )
        goto Cleanup;   //the document is not inplace activated

    //convert the (x,y) coordinates that we receive to client coordinates
    // BUGBUG (carled) we need to test on a multi monitor system
    if ( !ScreenToClient( _pElement->Doc()->GetHWND(), &pt) )
        goto Cleanup;
    
    msg.pt = pt;

    // Get the node that got the hit
    htc = _pElement->Doc()->HitTestPoint( &msg, &pHitNode, HT_VIRTUALHITTEST);

    // if the hit node is in a slave tree, we have to get to the master tree
    // and the master tree element,
    if ( pHitNode->Tag() == ETAG_TXTSLAVE )
    {
        pHitNode = pHitNode->GetMarkup()->Master()->GetFirstBranch();
    }

    // Is there a tree built and ready?
    if ( pHitNode->Tag() == ETAG_ROOT )
        goto Cleanup;

    if (_pElement->Tag() ==ETAG_IMG )
    {
        pHitNode->Element()->SubDivisionFromPt(msg.ptContent, 
                                               &msg.lSubDivision);

        // We are an area was hit so just return it.  if there is an error
        // bail and fall through to return the img
        hr = HitTestArea(&msg, pHitNode, pvarChildAtPoint);
        if (SUCCEEDED( hr ))
            goto Cleanup;
    }

    hr = S_OK;                              //Reset the return value

    // If the point was not inside of this document, return VT_EMPTY
    if ( htc == HTC_NO )
    {
        V_VT( pvarChildAtPoint ) = VT_EMPTY;       
        goto Cleanup;
    }

    

//BUGBUG: [Ferhane]  When we implement the positioning and visual parenting, instead of
//                      moving up the tree, we will walk the tree walker on the offset
//                      parent nodes, towards the tree top.
//
    // if we have already on the node that this object's element is connected, then
    // move on to child calculation part
    //
    // Compare elements instaad of nodes, to avoid problems with overlapping
    //
    // stert from the node that got hit, move up until you find a supported element
    // that is an immediage acc. child of this acc. object.
    pTreeWalker = pHitNode;

    if ( pHitNode->Element() != _pElement)
    {
        do
        {
            // if the parent node is a suported element's node, move the 
            // hit node to that node
            if ( IsSupportedElement( pTreeWalker->Element()) )
            {
                pLastSupported = pTreeWalker;
            }
        
            // Walk up the tree
            pTreeWalker = pTreeWalker->Parent();
            
#if DBG == 1 
            long l1 = pTreeWalker->Element()->GetSourceIndex();
            long l2 = _pElement->GetSourceIndex();
            Assert(!g_Zero.ab[0] || l1 || l2);
#endif

        }
        while ( pTreeWalker->Tag() != ETAG_ROOT &&                     // just to be safe
                (pTreeWalker->Element() != _pElement) &&               // hit is on us
                (pTreeWalker->Element()->GetSourceIndex() >= 
                                        _pElement->GetSourceIndex())); // hit outside our scope
    }
    // else they are equal, drop to the last else case below

    if ( pTreeWalker->Tag() == ETAG_ROOT ||
         (pTreeWalker->Element()->GetSourceIndex() < _pElement->GetSourceIndex()) ) 
    {
        // this case means the hit was OUTSIDE of our scope 
        V_VT( pvarChildAtPoint ) = VT_EMPTY;       
    }
    else if (pLastSupported && (pLastSupported->Element() != _pElement) )
    {
        // hit in our scope, and the hit is on a supported child
        // return this child 

        pAccObj = GetAccObjOfElement( pLastSupported->Element() );
        if ( !pAccObj )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        V_VT( pvarChildAtPoint )  = VT_DISPATCH;
        V_DISPATCH( pvarChildAtPoint ) = pAccObj;

        pAccObj->AddRef();

    }
    else
    {
        // hit in our scope, and the hit is on a text node
        // or the hit was on us directly
        //
        // If there were no supported elements below us, we have to find the child id
        // from the current element
        if ( !pLastSupported )
            pLastSupported = pElemNode;

        // We have to return ourselves as the acc object if we are
        // connected to a noscope tag, since it can not have any
        // text content.
        // buttons need to be handled here, even though they can contain
        //   html.  This is primarily due to proxy compat.
        // Also if the hit was not on any content, the cp comes back -1
        // and that requires us to return CHILDID_SELF for the acc object.
        elementTag = pLastSupported->Tag();
    
        if ( !CanHaveChildren()              ||  // can not have children
            (msg.resultsHitTest._cpHit < 0 ) ||  // ? FerhanE: BUGBUG ! ! !
            msg.resultsHitTest._fWantArrow   )   // arrow means we are not on text
        {
            V_VT( pvarChildAtPoint )  = VT_I4;
            V_I4 ( pvarChildAtPoint ) = CHILDID_SELF;
            this->AddRef();
            goto Cleanup;
        }

        // We know for a fact now, that the element that is represented
        // by this acc object was hit, and the hit was on content.
        // We should now find the child that was hit.

        // since this element can have children, we have to find out
        // which one of its children the pointer was on.
        // we can return with the information we get from this 
        // function, since it will fill the VARIANT for us.
        //
        hr = THR( GetHitChildID( _pElement, &msg, pvarChildAtPoint) );
    }


Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  accDoDefaultAction
//  
//  DESCRIPTION:
//  Does different things per tag:
//      The implementation for the tags OBJECT/EMBED and PLUGIN is in the 
//      AccObject class implementation.
//
//  PARAMETERS:
//      varChild            :   VARIANT child information
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::accDoDefaultAction(VARIANT varChild)
{   
    HRESULT     hr;
    CAccBase *  pParent = NULL;
    CAccBase *  pAccChild = NULL;

    TraceTag((tagAcc, "CAccElement::accDoDefaultAction, role=%d, childid=%d", 
                GetRole(), 
                V_I4(&varChild)));  

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        switch ( _pElement->Tag() )
        {
            case ETAG_INPUT:
                switch( DYNCAST(CInput, _pElement)->GetAAtype())
                {
                    case htmlInputButton:
                    case htmlInputReset:
                    case htmlInputCheckbox:
                    case htmlInputImage:
                    case htmlInputRadio:
                    case htmlInputSubmit:
                        hr = THR( ScrollIn_Focus_Click( _pElement ) );
                        break;

                    case htmlInputPassword: //E_NOTIMPL.
                    case htmlInputText:
                    case htmlInputTextarea:
                        hr = E_NOTIMPL;
                        break;
                }
                break;

            case ETAG_SELECT:
            case ETAG_BUTTON:
            case ETAG_A:
            case ETAG_AREA:
                hr = THR( ScrollIn_Focus_Click( _pElement ) );
                break;
          
            case ETAG_FRAMESET:
            case ETAG_BODY:
            case ETAG_MARQUEE:
            case ETAG_TABLE:
            case ETAG_TD:
            case ETAG_TH:
                hr = E_NOTIMPL;
                break;

            case ETAG_IMG:  //if your acc.parent is anchor, ask your parent
            default:        //text nodes and IMG are the same
//
// BUGBUG - what if anchor around table around image???  is sniffing 1 level enough
//
                hr = THR( GetAccParent( _pElement, &pParent ));
                if ( hr )
                    break;

                Assert( pParent );

                //there is no need to change varChild, since it
                //contains the CHILDID_SELF anyway...
                hr = THR( pParent->accDoDefaultAction(varChild) );
                break;
        }
    }
    else 
    {
        // find that object and call its API implementation, passing 
        // CHILDID_SELF as the child information.
        //
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        //
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( !pAccChild )
        {
            //if the text is inside an anchor, then the 
            //anchor behavior is used.
            
            // BUGBUG (carled) or inside an anchor
            if ( _pElement->Tag() == ETAG_A )
            {
                hr = THR( ScrollIn_Focus_Click( _pElement ) );
            }
            else
            {
                //no default action for plain text.
                hr = E_NOTIMPL;
            }
        }
        else 
        {
            // call child's implementation of this method. We don't have a helper for this
            // method, since the CHILDID_SELF case above has all the smarts and code.
            V_VT( &varChild ) = VT_I4;
            V_I4( &varChild ) = CHILDID_SELF;

            hr = THR( pAccChild->accDoDefaultAction( varChild ) );
        }
    }

Cleanup:
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  put_accValue
//  
//  DESCRIPTION :   
//          Sets the value property of the accessible object. Only the edit box
//          and password accessible objects support this method, all others 
//          return an error.
//          
//  PARAMETERS:
//      varChild     :   VARIANT containing the child ID
//      pbstrValue   :   value bstr
//
//  RETURNS:
//      DISP_E_MEMBERNOTFOUND
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccElement::put_accValue(VARIANT varChild, BSTR bstrValue)
{
    HRESULT         hr;
    CAccBase *      pAccChild = NULL;

    // validate parameter
     if ( !bstrValue )
     {
        hr = E_INVALIDARG;
        goto Cleanup;
     }

    // unpack varChild 
    hr = THR( ValidateChildID(&varChild) );
    if ( hr )
        goto Cleanup;

    if ( V_I4(&varChild) == CHILDID_SELF )
    {
        //call this instance's implementation
        hr = THR( PutAccValue(bstrValue) );
    }
    else
    {
        // get the child CElement/CMarkupPointer. If the child id 
        // is invalid, the GetChildFromID will return with an err.
        hr = THR( GetChildFromID( V_I4(&varChild), &pAccChild, NULL) );
        if ( hr ) 
            goto Cleanup;

        if ( pAccChild )
        {
            V_I4( &varChild ) = CHILDID_SELF;
            
            //call child's implementation
            hr = THR( pAccChild->put_accValue( varChild, bstrValue) );
        }
        else 
            hr = E_NOTIMPL;
    }

Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//  GetChildFromID()
//
//  DESCRIPTION:
//      Return the child object that is indicated with the child id that is 
//      passed.
//      The object returned is either a CElement pointer or a CMarkupPointer 
//      pointer.
//      If the child refers to a text block, a CMarkupPointer * is placed
//      after the text block and returned.
//      If the child refers to a tag that is supported ( determined by calling
//      the IsSupportedElement() helper from inside AccStepRight() ) then a
//      pointer to the CElement is returned. 
//      The accessible object decides how to use the information.
//      
//  
//  PARAMETERS:
//      lChildId    :   ID of the child that is being asked for. If this parameter
//                      is (-1) then the last accessible child of this element is 
//                      returned.
//      plType      :   Address of the long to receive the child type.
//                      Valid child types are the members of the childType
//                      enumeration in the CAccElement class
//      ppAccChild  :   Address of the pointer to receive the accessible child
//                      object. The parameter can be NULL, indicating that the caller
//                      does not want it.
//      ppMarkupPtr :   Address (CMarkupPointer *) variable. The parameter
//                      can be NULL, indicating that the caller does not want it
//      plChildCnt  :   The address of the variable to receive the ID of this child.
//                      Using this parameter, the caller can both determine the total
//                      number of children and get the last one at the same time. The
//                      parameter is optional and defaults to NULL.
//----------------------------------------------------------------------------
HRESULT
CAccElement::GetChildFromID(    long                lChildId, 
                                CAccBase **         ppAccChild, 
                                CMarkupPointer *    pMarkupPtrBegin,
                                long *              plChildCnt /*= NULL */)
{
    HRESULT             hr= S_OK;
    CMarkupPointer *    pelemBegin = NULL;
    CMarkupPointer *    pelemEnd = NULL;
    CTreeNode *         pElemNode = NULL;
    long                lChildCnt =0;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;

    //if the lChildId is <0 then go till the end of the scope
    BOOL                bGotoTheEnd = (lChildId < 0) ? TRUE : FALSE; 

    // we should never pass in CHILDID_SELF to this method.
    Assert( lChildId != 0 );

    //BUGBUG: This function uses the markup pointer that is passed in as a parameter
    //          to walk on the HTML stream using the tree services core functions.
    //          There are two cached CMarkupPointer instances on the CAccWindow to 
    //          avoid the constant create/destroy of these pointers. This function uses
    //          the _elemEnd to mark the end of the element, so it can determine when
    //          to stop. 
    //          The pointer that is passed in MUST NOT BE the end pointer, since that
    //          would cause immediate satisfaction of condition in the <while> below
    //          and return the wrong child.
    //          Hence the assert. (ferhane)
    AssertSz( pMarkupPtrBegin != &( GetAccDoc()->_pAccWindow->_elemEnd ), 
                            "The markup pointer this function relies on being static is passed in as volatile. BUG!");


    if ( ppAccChild )
        *ppAccChild = NULL;

    if (_pElement->Tag() == ETAG_IMAGE)
    {
        CImgElement *pImg = DYNCAST(CImgElement, _pElement);

        // handle areas under an image. in this case the childID is the 
        // subdivision of the image as a 1 based index.
        if (pImg && pImg->GetMap())
        {
            CAreaElement *  pArea = NULL;

            IGNORE_HR(pImg->GetMap()->GetAreaContaining(lChildId-1, 
                                                        &pArea));
            if (pArea)
            {
                *ppAccChild = GetAccObjOfElement( pArea );
                lChildCnt = lChildId;
                hr = S_OK;
                goto Cleanup;
            }
        }
    }

    if ( !CanHaveChildren() )
        return S_FALSE;             // No Children

    // Either use my local markup pointer, or the one I'm handed.
    // This is the pointer to be moved around in the markup stream
    if ( pMarkupPtrBegin )
        pelemBegin = pMarkupPtrBegin;
    else
        pelemBegin = &( GetAccDoc()->_pAccWindow->_elemBegin );

    // did we have a cached child, and is the cache still valid? 
    if ( IsCacheValid() && ( lChildId >= _lLastChild ) )
    {
        // move the beginning pointer to where the cached pointer is
        pelemBegin->MoveToPointer( &_lastChild );

        // set the current child count to the current index
        lChildCnt = _lLastChild;

        // if we are accessing the same child, just get the context information
        // and return.
        if ( lChildId == _lLastChild )
        {
            // Check left to see what the last child was
            hr = THR( pelemBegin->Left( FALSE, &context, &pElemNode, NULL, NULL, NULL) );
            if ( hr ) 
                goto Cleanup;

            // we have the context and pElemNode values, go to return routine...
            goto Return;
        }
    }
    else
    {
        // the cache was not valid, reset variables.

        _lLastChild = 0;   // invalidate the cache.
        _lTreeVersion = 0;
    }

    // if the last child is zero, either we reset it above, or there was no cache.
    // either way, we have to start from the very beginning.
    if ( _lLastChild == 0 )
    {
        // mark the beginning of the element
        hr = THR( pelemBegin->MoveAdjacentToElement( _pElement, ELEM_ADJ_AfterBegin));
        if (hr)
            goto Cleanup;
    }

    // set the end markup pointer to the end of the element, so
    // that we can stop.
    pelemEnd = &(GetAccDoc()->_pAccWindow->_elemEnd);
    hr = THR( pelemEnd->MoveAdjacentToElement( _pElement, ELEM_ADJ_BeforeEnd));
    if ( hr )
        goto Cleanup;

    //now, start walking to the right, until you hit child that is being 
    //asked, or the end of the scope for this element
    while ( bGotoTheEnd || ( lChildCnt < lChildId ) )
    {
        // we have reached the end of the scope of this element. If we have
        // the bGotoTheEnd set to TRUE, then we actually want the last child
        // which we just jumped over. 
        // Otherwise we have reached here without finding the child id that 
        // was passed and the child id is invalid
        if ( pelemBegin->IsEqualTo( pelemEnd ) )
        {
            // If the last child was being asked, there is nothing wrong,
            // break out and continue.
            if ( bGotoTheEnd )
                break;
            else
            {
                //[FerhanE]
                // Different from the proxy, we return E_INVALIDARG for all child
                // needing calls if we don't have a child. That is the same return
                // code we return if we have an invalid child id.
                if (lChildCnt)
                {
                    // there were children counted, so a bad value must have been passed in
                    hr = E_INVALIDARG;
                }
                else
                {
                    // return ourselves. since no children were counted, and we left
                    // our scope. This can happen hittesting and Iframe.
                    *ppAccChild= this;
                    hr = S_OK;
                    goto Cleanup;
                }
            }
        }

        //Take us out if the index was invalid or there were no children.
        if ( hr )               
            goto Cleanup;


        //get what is to our right
        hr = THR( pelemBegin->Right( TRUE, &context, &pElemNode, NULL, NULL, NULL));
        if ( hr )
            goto Cleanup;

        switch ( context )
        {
#if DBG==1            
            case CONTEXT_TYPE_None:     
                //if there was nothing on this side at all,
                AssertSz(FALSE, "We have hit the root..");
                break;

            case CONTEXT_TYPE_ExitScope://don't care ...
                break;
#endif

            case CONTEXT_TYPE_EnterScope:
                //check the element to see if it is a supported element
                if ( IsSupportedElement( pElemNode->Element() ) )
                {
                    //go to the end of this element, since we will handle it as a container
                    hr = THR( pelemBegin->MoveAdjacentToElement( pElemNode->Element(), ELEM_ADJ_AfterEnd));
                    if ( hr )
                        goto Cleanup;

                    lChildCnt++;      
                }
                //don't do anything if it is not a supported element.
                break;  
            
            //we jumped over text or noscope
            case CONTEXT_TYPE_NoScope:
                if ( !IsSupportedElement( pElemNode->Element() ))
                    break;
            
            case CONTEXT_TYPE_Text:
                lChildCnt++;
                break;
        }
    }

    // if we don't have any children
    // we better leave before reaching here, 
    Assert( lChildCnt );

    // place the cache pointer to where the walker is
    hr = THR( _lastChild.MoveToPointer( pelemBegin ) );
    if ( hr )
        goto Cleanup;                

Return:
    //if the block was a text block, we return the markup pointer
    //otherwise return the acc object for the element that we passed over.
    if ((( context == CONTEXT_TYPE_EnterScope ) || 
              ( context == CONTEXT_TYPE_NoScope    ) ||
              ( context == CONTEXT_TYPE_ExitScope )) && 
             ( ppAccChild ))
    {
        
        // if the element is a supported element, return the element,
        // however, if the call was for a last child, the last tag
        // may belong to a non-supported element.
        if (bGotoTheEnd && !IsSupportedElement(pElemNode->Element()))
        {
            context = CONTEXT_TYPE_None;

            // until we find an element 
            while ((context != CONTEXT_TYPE_Text) && 
                    !IsSupportedElement(pElemNode->Element()))
            {
                // we use the _lastChild, since we also want to be able to preserve the
                // location in case we return text.
                hr = THR( _lastChild.Left( TRUE, &context, &pElemNode, NULL, NULL, NULL));
                if (hr)
                    goto Cleanup;
            }

            // reverse the last move, so that we are pointing to the end of the last child
            hr = THR( _lastChild.Right( TRUE, &context, &pElemNode, NULL, NULL, NULL));
            if (hr)
                goto Cleanup;

            if (context == CONTEXT_TYPE_Text)
                goto Cleanup;
        }
 
        //get the accessible object for that element.
        *ppAccChild = GetAccObjOfElement( pElemNode->Element() );
        if ( !(*ppAccChild))
            hr = E_OUTOFMEMORY;
    }
    
Cleanup:
    //set the return child count.
    if ( plChildCnt && !hr )
        *plChildCnt = lChildCnt;

    // invalidate the cache if there was an error.
    if ( hr )
    {
        _lLastChild = 0;
        _lTreeVersion = 0;
    }
    else
    {
        _lTreeVersion = _pElement->Doc()->GetDocTreeVersion();
        _lLastChild = lChildCnt;                       // last child we accessed.
    }
   
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  GetLabelOrTitle
//-----------------------------------------------------------------------
HRESULT 
CAccElement::GetLabelorTitle( BSTR* pbstrOutput )
{
    HRESULT         hr=S_OK;
    CLabelElement * pLabel;

    if (!pbstrOutput)
        return S_FALSE;

    *pbstrOutput = NULL;

    //get the associated label element if there is one
    pLabel = GetLabel();

    if( pLabel)
    {
        hr = THR( pLabel->get_innerText( pbstrOutput ) );

        //BUGBUG we might want to revisit this.  If the user
        // explicitly wanted to have a label with no text then
        // we should honor it. but for proxy compatability we
        // need to test the length
        if (!SysStringLen(*pbstrOutput))
        {
            SysFreeString(*pbstrOutput);
            *pbstrOutput = NULL;
        }
    }

    if (!*pbstrOutput)
    {
        //get the title for the checkbox
        hr = GetTitle(pbstrOutput);
        if (FAILED(hr) || !*pbstrOutput)
            hr = E_NOTIMPL;
    }

    RRETURN1( hr, S_FALSE );
}

//-----------------------------------------------------------------------
//  GetTitleorLabel
//-----------------------------------------------------------------------
HRESULT 
CAccElement::GetTitleorLabel( BSTR* pbstrOutput )
{
    HRESULT         hr=S_OK;
    CLabelElement * pLabel = NULL;

    Assert(pbstrOutput);

    //get the title for the checkbox
    hr = GetTitle(pbstrOutput);
    if (hr || !*pbstrOutput)
    {
        //get the associated label element if there is one
        pLabel = GetLabel();

        if (pLabel)
            hr = THR( pLabel->get_innerText( pbstrOutput ) );
    }

    RRETURN1( hr, S_FALSE );
}

//-----------------------------------------------------------------------
//  GetTitle
//-----------------------------------------------------------------------
HRESULT
CAccElement::GetTitle( BSTR* pbstrOutput )
{
    HRESULT hr = S_OK;
    TCHAR* pchString=NULL;

    Assert(pbstrOutput);
    Assert(!*pbstrOutput);

    //get the title 
    pchString = (LPTSTR) _pElement->GetAAtitle();
    if ( pchString )
    {
        *pbstrOutput = SysAllocString( pchString );
        if ( !(*pbstrOutput) )
            hr = E_OUTOFMEMORY;
    }
    else
        hr = S_FALSE;

    RRETURN1( hr, S_FALSE );
}

//-----------------------------------------------------------------------
//  HasLabel - this helper function is necessaryt to support the accDescription
//      methods.  The way that description works is that it returns what
//      the name doesn't.  So if there is a label, accName returns it and 
//      accDesc shoule return the title.  If there is no label, then accName 
//      returns the title and accDescription is left returning nothing.
//          the wrinkle in this is that there not only has to be a label but
//      the label needs to actually have text . i.e. <label for=idFoo></label>
//      doesn't count.
//-----------------------------------------------------------------------
BOOL
CAccElement::HasLabel()
{
    BSTR            bstrOutput = NULL;
    CLabelElement * pLabel;
    BOOL            fRet = FALSE;

    pLabel = GetLabel();

    if (!pLabel)
        goto Cleanup;

    if (FAILED( pLabel->get_innerText( &bstrOutput ) ))
        goto Cleanup;

    if (bstrOutput)
    {
        fRet = !!SysStringLen(bstrOutput);

        SysFreeString(bstrOutput);
    }

Cleanup:
    return fRet;
}

//+-------------------------------------------------------------------------
//
// HitTestArea - helper function for accHitTest.  This is called when an
//  image with a map is hittested over an area (subdivision).  its job is 
//  to return the accElemetn of hte area that was hit.
//
//---------------------------------------------------------------------------
HRESULT
CAccElement::HitTestArea(CMessage *pMsg,    
                         CTreeNode *pHitNode,
                         VARIANT * pvarChildAtPoint)
{
    HRESULT      hr = E_FAIL;
    CImgElement *pImg = DYNCAST(CImgElement, pHitNode->Element());

    Assert(pvarChildAtPoint && V_VT(pvarChildAtPoint)==VT_EMPTY);

    if (pImg && pImg->GetMap())
    {
        CAreaElement *  pArea = NULL;

        IGNORE_HR(pImg->GetMap()->GetAreaContaining(pMsg->lSubDivision, 
                                                    &pArea));
        if (pArea)
        {
            CAccBase  *pAccObj = GetAccObjOfElement( pArea );
            if ( pAccObj )
            {
                V_VT( pvarChildAtPoint )  = VT_DISPATCH;
                V_DISPATCH( pvarChildAtPoint ) = pAccObj;
    
                pAccObj->AddRef();
    
                hr = S_OK;
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//+----------------------------------------------------
//
//  GetAnchorParent() - helper function that walks up the 
//      accesssible parent chain looking to see if there is
//      an anchor in scope above us.  If there is return it
//      so that it's properties can be used.  If there isn't
//      return NULL
//
//-----------------------------------------------------
CAccBase *
CAccElement::GetParentAnchor()
{
    CAccBase * pParent = NULL;
    CAccBase * pCurrent = this;
    BOOL       fDone = FALSE;

    while (!fDone && pCurrent)
    {
        if (S_OK != GetAccParent( pCurrent->GetElement(), &pParent ) )
        {
            fDone = TRUE;
            pParent = NULL;
        }
        else
        {
            Assert( pParent && pParent->GetElement());
    
            switch ( pParent->GetElement()->Tag())
            {
            case ETAG_A:
                // bingo, retun this puppy
                fDone = TRUE;
                break;

            case ETAG_TABLE:
            case ETAG_TD:
            case ETAG_TH:
            case ETAG_BODY:
            case ETAG_IFRAME:
            case ETAG_FRAMESET:
                // anchors don't propogate through tables.
                // ..and other good stopping criteria
                pParent = NULL;
                fDone = TRUE;
                break;

            }
            pCurrent = pParent;
        }
    }

    return pParent;
}



//+------------------------------------------------------
//
//   BOOL fBrowserWindowHasFocus() - 
//  Another helper function for determining when we can
//  specify the STATE_SYSTEM_SELECTABLE for text.  For
//  proxy compatablity this is when the forground focus
//  window is ours.
//-------------------------------------------------------
BOOL
CAccElement::fBrowserWindowHasFocus()
{
    HWND hwndCurrent = GetElement()->Doc()->GetHWND();
    HWND hwndTarget = GetForegroundWindow();

    while(hwndCurrent)
    {
        if(hwndTarget == hwndCurrent)
            break;
    
        hwndCurrent = ::GetParent(hwndCurrent);
    }

    //--------------------------------------------------
    // no focused window means that neither this 
    // window or any of its parent chain has the focus.
    // **NOTE** this handles uninitialized frames also.
    //--------------------------------------------------
    return !! hwndCurrent;
}

//+---------------------------------------------------------------------------
//  GetSelectedChildren
//  
//  DESCRIPTION: Returns accessible children that reside between two markup 
//          pointers. 
//          bForceEnumerator flag is used to force creation of an IEnumVariant 
//          implementation. If the flag is FALSE, and there is only one child
//          then the only child is returned in the return variant. Otherwise
//          the return variant contains the IEnumVariant pointer.
//          cChildBase contains the offset child id, for the child that starts
//          at the location marked with pStart. This is needed for text children.
//  
//  PARAMETERS:
//      cChildBase          :   Child id offset
//      pStart              :   Starting point within the markup
//      pEnd                :   Ending point within the markup
//      pvarSelectedChildren:   Return variant that receives a child or enumeration
//      bForceEnumerator    :   If TRUE, an enumerator is created even if there 
//                              is only one child in the markup region.
//
//----------------------------------------------------------------------------
HRESULT
CAccElement::GetSelectedChildren(   long cChildBase, 
                                    CMarkupPointer *pStart, 
                                    CMarkupPointer * pEnd, 
                                    VARIANT * pvarSelectedChildren,
                                    BOOL    bForceEnumerator )
{
    HRESULT                     hr = S_OK;
    long                        lCounter = 0;
    long                        lOldCounter = 0;
    CTreeNode *                 pElemNode = NULL;
    VARIANT                     varCurrent;
    MARKUP_CONTEXT_TYPE         context;
    CAccBase *                  pAccChild = NULL;
    CDataAry <VARIANT> *        pary = NULL;

    Assert( pStart );
    Assert( pEnd );
    Assert( pvarSelectedChildren );
    
    varCurrent.vt = VT_EMPTY;
    varCurrent.lVal = 0;

    pary = new(Mt(CAccElementget_accSelection_aryVariants_pv)) 
                        CDataAry<VARIANT>(Mt(CAccElementget_accSelection_aryVariants_pv));
    if ( !pary )
        RRETURN( E_OUTOFMEMORY );

    // Until the start pointer reaches or passes the end location
    while ( pStart->IsLeftOf( pEnd )  )
    {
        hr = THR( pStart->Right( TRUE, &context, &pElemNode, 
                                            NULL, NULL, NULL));
        if ( hr )
            goto Cleanup;

        switch ( context )
        {
#if DBG==1            
            case CONTEXT_TYPE_None:     
                //if there was nothing on this side at all,
                AssertSz(FALSE, "We have hit the root..");
                break;
            case CONTEXT_TYPE_ExitScope://don't care ...
                break;
#endif
            case CONTEXT_TYPE_EnterScope:
                Assert( pElemNode );
                
                // check the element to see if it is a supported element
                if ( IsSupportedElement( pElemNode->Element() ) )
                {
                    // go to the end of this element, 
                    // since we will handle it as a container
                    hr = THR( pStart->MoveAdjacentToElement( pElemNode->Element(), 
                                                                ELEM_ADJ_AfterEnd));
                    if ( hr )
                        goto Cleanup;

                    lCounter++;
                }
                //don't do anything if it is not a supported element.
                break;  
                
            //we jumped over text or noscope
            case CONTEXT_TYPE_NoScope:
                Assert( pElemNode );
                if ( !IsSupportedElement( pElemNode->Element() ))
                    break;
                //else 
                //  fall through to increment the counter.
                
            case CONTEXT_TYPE_Text:
                lCounter++;      
                break;
        }

        // if we passed a valid accessible child, then the counter must have been incremented
        if ( lOldCounter != lCounter )
        {
            VariantInit( &varCurrent );
            
            if ( context == CONTEXT_TYPE_Text )
            {   
                V_VT( &varCurrent ) = VT_I4;
                V_I4( &varCurrent ) = lCounter + cChildBase;
            }
            else
            {
                IDispatch * pDispTmp;

                V_VT( &varCurrent ) = VT_DISPATCH;

                pAccChild = GetAccObjOfElement( pElemNode->Element() );
                if ( !pAccChild )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                // increment the ref count and do the casting at the same time.
                hr = THR( pAccChild->QueryInterface( IID_IDispatch, (void**)&pDispTmp));
                if ( hr )
                    goto Cleanup;
                    
                Assert( pDispTmp );

                V_DISPATCH( &varCurrent ) = pDispTmp;
            }

            //append the record to the list, and reset the variant
            hr = THR(pary->EnsureSize(lCounter));
            if (hr)
                goto Cleanup;

            // no need to addref the variant's IDispatch content, since we will dispose 
            // of the varCurrent's contents. AddRef is done above with the QI call.
            hr = THR( pary->AppendIndirect( &varCurrent, NULL ) );
            if ( hr )
                goto Cleanup;

            // reset the check condition.
            lOldCounter = lCounter;
        }
    }
    if ( bForceEnumerator  || (lCounter > 1 ) )   
    {
        // create the enumerator using the CDataAry contents and return the 
        // IEnumVariant * in the return VARIANT.
        hr = THR(pary->EnumVARIANT(VT_VARIANT, 
                                    (IEnumVARIANT **) &V_DISPATCH(pvarSelectedChildren), 
                                    FALSE,  // don't copy the array being enumerated use the one we gave
                                    TRUE)); // delete enumeration when no one is left to use .
        if (hr)
            goto Cleanup;

        V_VT( pvarSelectedChildren ) = VT_DISPATCH;
        
        //we don't delete the pary, since its contents are being used by the enumerator...!
        pary = NULL;
    }
    else if ( lCounter == 1 )
    {
        // we copy ourselves, and not use VariantCopy, since the value is already
        // addref ed if it is a dispatch ptr.
        memcpy( pvarSelectedChildren, &varCurrent, sizeof(VARIANT));
    }
    else    //no children, and bForceEnumerator is FALSE, return VT_EMPTY.
    {
        V_VT( pvarSelectedChildren ) = VT_EMPTY;
    }

Cleanup:
    delete  pary;
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  GetEnumerator
//
//  DESCRIPTION:
//      Create and return a pointer to the enumerator object for this 
//----------------------------------------------------------------------------
HRESULT 
CAccElement::GetEnumerator( IEnumVARIANT ** ppEnum)
{
    HRESULT             hr;
    CMarkupPointer      markupStart( _pElement->Doc() );
    CMarkupPointer      markupEnd( _pElement->Doc() );
    VARIANT             varResult;

    // only called internally.
    Assert( ppEnum );

    *ppEnum = NULL;

    // get the limits for the element 
    hr = THR( GetMarkupLimits( _pElement, &markupStart, &markupEnd ) );
    if ( hr )
        goto Cleanup;    

    // get an array of children of this element.
    // Even if there are no children, varResult contains the enumerator, 
    // since we pass TRUE for the last parameter( bForceEnumerator )
    hr = THR( GetSelectedChildren( 0, &markupStart, &markupEnd, &varResult, TRUE ) );
    if ( hr )
        goto Cleanup;

    // We've got to receive an enumerator here.
    Assert( V_VT(&varResult) == VT_DISPATCH );

    // return the enumerator object.
    *ppEnum = (IEnumVARIANT *)V_DISPATCH( &varResult );
    
Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  GetLabelParent
//  
//  DESCRIPTION:
//      REturns the CLabelElement parent for the element this accessible object
//      is associated with. 
//
//      Return value can be NULL.
//----------------------------------------------------------------------------
CLabelElement *
CAccElement::GetLabel()
{
    CTreeNode * pNode;
    CLabelElement * pLabel = _pElement->GetLabel();

    // does this element have a label associated with it.
    // if so, return it.
    if (pLabel)
        goto Cleanup;

    // may be the element does not have a label associated
    // but is wrapped with one.
    pNode = _pElement->GetFirstBranch();

    if ( !pNode || pNode->IsDead() )
        goto Cleanup;

    pNode = pNode->Parent();

    // if we have a parent, then check if it is actually 
    // a label element,
    if (pNode && (pNode->Tag() == ETAG_LABEL) )
        pLabel = DYNCAST( CLabelElement, pNode->Element());

Cleanup:
    return pLabel;
}


BOOL 
CAccElement::IsVisibleRect( CRect * pRectRegion )
{
    HRESULT     hr;
    CElement *  pElemClient;
    long        clientWidth;
    long        clientHeight;

    // get the document's primary element client and ask it where its scroll
    // positions are.
    pElemClient = _pElement->Doc()->GetPrimaryElementClient();

    // if there is no primary element client, NOT VISIBLE
    if (!pElemClient)
        goto Invisible;

    hr = THR(pElemClient->get_clientWidth(&clientWidth));
    if (hr)
        goto Visible;

    hr = THR(pElemClient->get_clientHeight(&clientHeight));
    if (hr)
        goto Visible;

    // if the bounding rectangle of the text is containing the scroll area, 
    // or it has one of its corners inside it, the text is visible.
    // it is easier to check the invisible condition
    if ((pRectRegion->left > clientWidth) || 
        (pRectRegion->top > clientHeight) ||
        (pRectRegion->bottom < 0) ||
        (pRectRegion->right < 0) )
        goto Invisible;

Visible:
    return TRUE;
Invisible:
    return FALSE;
}

//+---------------------------------------------------------------------------
//  IsTextVisible
//
//----------------------------------------------------------------------------
BOOL
CAccElement::IsTextVisible(CMarkupPointer * pTextBegin, CMarkupPointer * pTextEnd)
{
    HRESULT             hr;
    CTreeNode *         pParentNode;
    CDataAry <RECT>     aryRects( Mt(CAccElementaccLocation_aryRects_pv) );
    CRect               rectRegion;
    CDoc *              pDoc = _pElement->Doc();

    //get the closest HTML element parent 
    pParentNode = pTextEnd->CurrentScope(MPTR_SHOWSLAVE);

    if (pParentNode)
    {
        // if the parent element is not visible, then it does not matter if we 
        // are in the view or not.
        if (!pParentNode->Element()->IsVisible(TRUE))
            goto Cleanup;

        // is the text inside the client window coordinates
        hr = THR( pDoc->RegionFromMarkupPointers( pTextBegin, 
                                                    pTextEnd,
                                                    &aryRects, 
                                                    &rectRegion ));

        if ( hr || IsVisibleRect(&rectRegion))
            return TRUE;
    }

Cleanup:
    return FALSE;
}

//----------------------------------------------------------------------------
//  Function    :   GetNavChildId
//  Description :   Given a child element of this element, returns the index
//                  of its sibling in the direction indicated by the navigation
//                  direction parameter.
//----------------------------------------------------------------------------
HRESULT
CAccElement::GetNavChildId( long navDir, CElement * pChildElement, long * pChildId)
{
    HRESULT             hr;
    CMarkupPointer *    pBegin = &( GetAccDoc()->_pAccWindow->_elemBegin );
    CMarkupPointer *    pEnd = &( GetAccDoc()->_pAccWindow->_elemEnd );
    long                lChildId = 0;
    long                lCachedChildId = 0;
    long                lIndex = 0;
    
    // place a markup pointer before the beginning of the child element
    hr = THR( pEnd->MoveAdjacentToElement(pChildElement, ELEM_ADJ_BeforeBegin) );
    if (hr)
        goto Cleanup;
 
    // if there is a cached location pointer and 
    // if this element is on the right or on top of the cached location
    if ( IsCacheValid() && 
            !pEnd->IsLeftOf(&_lastChild))
    {
        // move the beginning pointer to where the cached pointer is
        pBegin->MoveToPointer( &_lastChild );

        // The cached child id will change on the parent side when we walk to 
        // find this element in the stream. We must backup.
        lCachedChildId = _lLastChild;
    }
    else
    {
        //place a pointer to the beginning of this element
        hr = THR( pBegin->MoveAdjacentToElement( _pElement,ELEM_ADJ_AfterBegin ) );
        if (hr) 
            goto Cleanup;

        // invalidate the cache of the parent, 
        // we will have to walk from the start anyway.
        _lLastChild = 0;
        _lTreeVersion = 0;
    }
    
    // find child element's child id
    hr = GetChildCount( pBegin, pEnd, &lChildId);
    if(hr)
        goto Cleanup;

    // since we have counted the children up to the child element, it is +1
    // to the total number of children that are before it.
    lChildId = lCachedChildId + lChildId + 1;
    
    // If we need to go to the next element, we could actually benefit from the 
    // cache on the parent element
    if ( navDir == NAVDIR_NEXT )
    {
        // place the cached markup pointer to the end of this child.
        hr = THR( _lastChild.MoveAdjacentToElement( pChildElement, ELEM_ADJ_AfterEnd ));
        if (hr)
            goto Cleanup;

        _lLastChild = lChildId;

        if ( _pElement->Doc() )
            _lTreeVersion = _pElement->Doc()->GetDocTreeVersion();

        // the child id we will ask for from the parent
        lIndex = lChildId + 1;
    }
    else if (lChildId == 1)
    {
        // if NAVDIR_PREVIOUS, then we MUST be at least the second child.
        hr = S_FALSE;
    }
    else
    {
        // the child id we will ask for from the parent
        lIndex = lChildId - 1;
    }

Cleanup:
    *pChildId = lIndex;

    // convert all other error codes to S_FALSE too.
    if (hr)
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}
