//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       selecobj.cxx
//
//  Contents:   Implementation of the CSelectionObj class.
//
//  Classes:    CSelectionObj
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#define _cxx_
#include "selecobj.hdl"

MtDefine(CSelectionObject, ObjectModel, "CSelectionObject")

const CBase::CLASSDESC CSelectionObject::s_classdesc =
{
    0,                          // _pclsid
    0,                          // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                       // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                       // _pcpi
    0,                          // _dwFlags
    &IID_IHTMLSelectionObject,  // _piidDispinterface
    &s_apHdlDescs,              // _apHdlDesc
};

//------------------------------------------------------------
//  Member   : CSelectionObject
//
//  Synopsis : Constructor
//
//------------------------------------------------------------

CSelectionObject::CSelectionObject( CDoc * pDoc)
: super(), _pDoc(pDoc)
{
    _pDoc->SubAddRef();
}

//------------------------------------------------------------
//  Member   : CSelectionObject
//
//  Synopsis : Destructor:
//       clear out the storage that knows about this Interface.
//
//------------------------------------------------------------

CSelectionObject::~CSelectionObject()
{
    _pDoc->_pCSelectionObject = NULL;
    _pDoc->SubRelease();
}

//------------------------------------------------------------
//  Member   : PrivateQUeryInterface
//
//------------------------------------------------------------

STDMETHODIMP
CSelectionObject::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IDispatch ||
        iid == IID_IHTMLSelectionObject)
    {
        *ppv = (IHTMLSelectionObject *)this;
    }
    else if (iid == IID_IDispatchEx)
    {
        *ppv = (IDispatchEx *)this;
    }
    else if (iid == IID_IObjectIdentity)
    {
        HRESULT hr = CreateTearOffThunk(this,
            (void *)s_apfnIObjectIdentity,
            NULL,
            ppv);
        if (hr)
            RRETURN(hr);
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetCreateRange
//
//  Synopsis: returns the specified item from the selection.
//              CElementCollection if structural
//              CRange             if text
//              CTable????         if table
//
//  Returns: S_OK if item found and the IDispatch * to the item
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::createRange( IDispatch ** ppDisp )
{
    HRESULT         hr = S_OK;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    IHTMLCaret*     pCaret = NULL;
    ISelectionRenderingServices * pSelRenSvc = NULL;
    int             cSegments = 0;    
    CElement *      pElement = (_pDoc->_pElemEditContext
                                    ? _pDoc->_pElemEditContext
                                    : _pDoc->_pElemCurrent);
    CMarkup*        pMarkup = NULL;
    SELECTION_TYPE  eType = SELECTION_TYPE_None;
    IHTMLElement *  pIHTMLElement = NULL;
    htmlSelection htmlSel;

    if (! pElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // BUGBUG (MohanB) Hack for INPUT's slave markup. Need to figure out
    // how to generalize this.
    if (pElement->Tag() == ETAG_INPUT && pElement->HasSlaveMarkupPtr())
    {
        pMarkup = pElement->GetSlaveMarkupPtr();
        pElement = pMarkup->FirstElement();
    }
    else
    {
        pMarkup = pElement->GetMarkup();
        pElement = pMarkup->GetElementTop();        
    }
    Assert( pMarkup );

    hr = THR( pMarkup->QueryInterface( IID_ISelectionRenderingServices, ( void**) & pSelRenSvc ));
    if ( hr )
        goto Cleanup;
        

    hr = THR( GetType( & htmlSel ));
    if ( hr )
        goto Cleanup;
        
    if ( htmlSel == htmlSelectionControl )
    {         
        //
        // Create a control range
        //
        CAutoTxtSiteRange* pControlRange =  new CAutoTxtSiteRange( _pDoc->PrimaryMarkup()->GetElementClient() );
        if (! pControlRange)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        CElement* pSelectedElement = NULL;
        pSelectedElement = pMarkup->GetSelectedElement( 0 );
        if (! pSelectedElement )
        {
#if DBG == 1        
            //
            // If we didn't have something in the array - then we must be a UI-Active adorned
            // edit context with the caret not visible. So we double-check, and then assume pSelected
            // Element is the EditContext.
            //
            IHTMLEditor* ped = _pDoc->GetHTMLEditor(FALSE);
            Assert(ped);
            AssertSz(( ped->IsEditContextUIActive() == S_OK && !_pDoc->IsCaretVisible() ),
                         "Nothing UI-Active - why is selection type control ?" );
            Assert( _pDoc->_pElemEditContext );                         
#endif            
            //
            // This handles the UI-active case.
            //
            pSelectedElement = _pDoc->_pElemEditContext;
        }
        pControlRange->AddElement( pSelectedElement );
        
        hr = THR( pControlRange->QueryInterface(IID_IDispatch, (void **) ppDisp) );
        pControlRange->Release();
        if (hr)
        {
            *ppDisp = NULL;
            goto Cleanup;
        }            
    }
    else
    {
        //
        //
        // Set the range pointers
        //
        hr = THR( _pDoc->CreateMarkupPointer( & pStart ));
        if ( hr ) 
            goto Cleanup;
        hr = THR( _pDoc->CreateMarkupPointer( & pEnd ));
        if ( hr ) 
            goto Cleanup;

        hr = THR( pSelRenSvc->GetSegmentCount(&cSegments, & eType  ));
        if ( hr )
            goto Cleanup;
        
        if (cSegments != 0)
        {
            //
            // Set pStart and pEnd using the segment list
            //
            hr = THR( pSelRenSvc->MovePointersToSegment(0, pStart, pEnd));    
            if ( hr )
                goto Cleanup;
        }
        else
        {
            //
            // BUGBUG - is this right ?
            //
            
            //
            // Set pStart and pEnd to the beginning of the current element
            //
            hr = THR_NOTRACE(
                pElement->QueryInterface( IID_IHTMLElement, (void**) & pIHTMLElement ) );
            Assert( pIHTMLElement );

            hr = THR( pStart->MoveAdjacentToElement( pIHTMLElement, ELEM_ADJ_AfterBegin ) );
            if ( hr )
                goto Cleanup;

            hr = THR( pEnd->MoveAdjacentToElement( pIHTMLElement, ELEM_ADJ_AfterBegin ) );
            if ( hr )   
                goto Cleanup;
            
        }
        // Create a Text Range
        //
        hr = THR( pMarkup->createTextRange(
                (IHTMLTxtRange **) ppDisp, pElement, pStart, pEnd, TRUE ) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pSelRenSvc );
    ReleaseInterface( pCaret );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pIHTMLElement );
    RRETURN ( SetErrorInfo(hr) );
}

//+---------------------------------------------------------------------------
//
//  Member:     Gettype
//
//  Synopsis: returns, on the parameter, the enumerated value indicating the type
//              of the selection (htmlSelectionText, htmlSelectionTable,
//              htmlSelectionStructure)
//
//  Returns: S_OK if properly executes
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::get_type( BSTR *pbstrSelType )
{
    HRESULT hr;
    htmlSelection eSelType;

    if (!pbstrSelType )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(GetType ( &eSelType ));
    if ( hr )
        goto Cleanup;

    hr = THR( STRINGFROMENUM ( htmlSelection, (long)eSelType, pbstrSelType ) );

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CSelectionObject::GetType( htmlSelection *pSelType )
{
    HRESULT hr = S_FALSE;
    int cSegments = 0;
    ISelectionRenderingServices * pSelRenSvc = NULL;
    SELECTION_TYPE eType = SELECTION_TYPE_None;
    
    CElement *  pElement    = _pDoc->_pElemCurrent;
    CMarkup *   pMarkup;

    // BUGBUG (MohanB) Hack for INPUT's slave markup. Need to figure out
    // how to generalize this.
    if (pElement->Tag() == ETAG_INPUT && pElement->HasSlaveMarkupPtr())
    {
        pMarkup = pElement->GetSlaveMarkupPtr();
        pElement = pMarkup->FirstElement();
    }
    else
    {
        pMarkup = pElement->GetMarkup();
    }

    hr = THR( pMarkup->QueryInterface(IID_ISelectionRenderingServices, ( void**) & pSelRenSvc ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSelRenSvc->GetSegmentCount(&cSegments, &eType ));

    switch( eType )
    {
        case SELECTION_TYPE_Control:
            *pSelType = htmlSelectionControl;
            break;

        case SELECTION_TYPE_Selection:
            *pSelType = htmlSelectionText;
            break;

        //
        // A Caret should return a Selection of 'None', for IE 4.01 Compat
        //
        default:  
            //
            // We return 'none' - unless we're a UI-Active ActiveX Control
            // with an invisible caret - in which case we return "control"
            // for IE 4 compat.
                        
            *pSelType = htmlSelectionNone;

            IHTMLEditor* ped = _pDoc->GetHTMLEditor(FALSE);
            if ( ped )
            {
                if ( _pDoc->_pElemEditContext && ped->IsEditContextUIActive() == S_OK )
                {
                    *pSelType = ( _pDoc->GetSelectionType() == SELECTION_TYPE_Caret &&
                                 !_pDoc->IsCaretVisible() ) || 
                                  _pDoc->_pElemEditContext->IsNoScope() ? 
                                    htmlSelectionControl : htmlSelectionNone;
                }
            }
    }
Cleanup:
    ReleaseInterface( pSelRenSvc );
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     clear
//
//  Synopsis: clears the selection and sets the selction type to NONE
//              this is spec'd to behave the same as edit.clear
//
//  Returns: S_OK if executes properly.
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::clear()
{
    RRETURN_NOTRACE(_pDoc->Exec(
            (GUID *) &CGID_MSHTML,
            IDM_DELETE,
            0,
            NULL,
            NULL));
}

//+---------------------------------------------------------------------------
//
//  Member:   empty
//
//  Synopsis: Nulls the selection and sets the selction type to NONE
//
//  Returns: S_OK if executes properly.
//
//----------------------------------------------------------------------------

HRESULT
CSelectionObject::empty()
{
    RRETURN( _pDoc->NotifySelection( SELECT_NOTIFY_EMPTY_SELECTION, NULL ) );
}
