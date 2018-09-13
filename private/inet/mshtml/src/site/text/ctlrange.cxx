//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       txtsrang.cxx
//
//  Contents:   Implementation of Control Range
//
//  Class:      CAutoTxtSiteRange
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "siterang.hdl"

MtDefine(CAutoTxtSiteRange, ObjectModel, "CAutoTxtSiteRange")
MtDefine(CAutoTxtSiteRange_aryElements_pv, CAutoTxtSiteRange, "CAutoTxtSiteRange::_aryElements::_pv")

// IOleCommandTarget methods

BEGIN_TEAROFF_TABLE(CAutoTxtSiteRange, IOleCommandTarget)
    TEAROFF_METHOD(CAutoTxtSiteRange, QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext))
    TEAROFF_METHOD(CAutoTxtSiteRange, Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut))
END_TEAROFF_TABLE()

//+------------------------------------------------------------------------
//
//  Member:     s_classdesc
//
//  Synopsis:   class descriptor
//
//-------------------------------------------------------------------------

const CBase::CLASSDESC CAutoTxtSiteRange::s_classdesc =
{
    0,                              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLControlRange,     // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+------------------------------------------------------------------------
//
//  Member:     CAutoTxtSiteRange constructor
//
//-------------------------------------------------------------------------

CAutoTxtSiteRange::CAutoTxtSiteRange(CElement * pElementOwner)
    : _aryElements(Mt(CAutoTxtSiteRange_aryElements_pv))
{
    _pElementOwner = pElementOwner;
}


CAutoTxtSiteRange::~CAutoTxtSiteRange()
{
    _EditRouter.Passivate();
}


//+------------------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   vanilla implementation
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLControlRange)
        QI_TEAROFF(this, IOleCommandTarget, (IHTMLControlRange *)this)
        QI_INHERITS(this, IHTMLControlRange)
        QI_INHERITS(this, ISegmentList)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    } 

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CloseErrorInfo
//
//  Synopsis:   defer to base object
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::CloseErrorInfo(HRESULT hr)
{
    return _pElementOwner->CloseErrorInfo(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     add
//
//  Synopsis:   add a site to the range
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::add ( IHTMLControlElement * pDisp )
{
    HRESULT     hr = E_INVALIDARG;
    CElement *  pElement = NULL;
    ELEMENT_TAG eTag;
    CTreeNode * pNode;

    if (! pDisp)
        goto Cleanup;

    hr = THR( pDisp->QueryInterface( CLSID_CElement, (void**) & pElement ) );
    if (hr)
        goto Cleanup;
    //
    // Check to see whether this element is a "site"
    //
    pNode = pElement->GetFirstBranch();
    if (! (pNode && pNode->NeedsLayout()) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
        
    //
    // Do not allow table content to be added per bug 44154
    //
    eTag = pElement->Tag();
    switch (eTag)
    {
    case ETAG_BODY:
    case ETAG_TD:
    case ETAG_TR:
    case ETAG_TH:
    case ETAG_TC:
    case ETAG_CAPTION:
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    //
    // Verify that the element being added is within the hierarchy
    // of the owner
    //
    pNode = pElement->GetFirstBranch();
    if ( (pNode == NULL) || (pNode->SearchBranchToRootForScope( _pElementOwner ) == NULL) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // It's OK to add the element
    //
    hr = THR( AddElement( pElement) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT 
CAutoTxtSiteRange::AddElement( CElement* pElement )
{
    RRETURN ( _aryElements.Append( pElement ) );
}


//+------------------------------------------------------------------------
//
//  Member:     delete
//
//  Synopsis:   remove a site from the range
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::remove(long lIndex)
{
    HRESULT hr = S_OK;

    if ( lIndex < 0 || lIndex >= _aryElements.Size() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    _aryElements.Delete(lIndex);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     Select
//
//  Synopsis:   turn this range into the selection, but only if there are
//      sites selected
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::select ( void )
{

    HRESULT     hr = CTL_E_METHODNOTAPPLICABLE;
    CElement* pElement;
    CTreeNode* pMyNode;
    CDoc* pDoc = _pElementOwner->Doc();
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    IHTMLElement* pIElement = NULL;
    
    if (! _aryElements.Size() )
    {
        //
        // We now place the caret in the edit context
        //
        hr = pDoc->NotifySelection (
                                    SELECT_NOTIFY_CARET_IN_CONTEXT , 
                                    (IUnknown*) (IHTMLControlRange*) this );   
    }
    else
    {
        pElement = _aryElements.Item( 0 );
        pMyNode = pElement->GetFirstBranch();
        Assert( pMyNode );

        // Make the owner current
        if ( pDoc->_state >= OS_INPLACE )
        {
            //
            // BUGBUG this was an access fix - with OLE Sites not going UI Active
            // on selecting away from them. Changing currencty should do this "for free".
            //
            if ( ( pDoc->_pElemCurrent != _pElementOwner ) && ( pDoc->_pElemCurrent->_etag == ETAG_OBJECT ))
                pDoc->_pElemCurrent->YieldUI( _pElementOwner );              
        }

        hr = THR( pDoc->CreateMarkupPointer( & pStart ));
        if ( hr )
            goto Cleanup;
        hr = THR( pDoc->CreateMarkupPointer( & pEnd ));
        if ( hr )
            goto Cleanup;

        hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void**) & pIElement ));
        if ( hr )
            goto Cleanup;
            
        hr = THR( pStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ));
        if ( hr )
            goto Cleanup;

        hr = THR( pEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
        if ( hr )   
            goto Cleanup;

        hr = THR( pDoc->EnsureEditContext( pStart ));
        if ( hr )
            goto Cleanup;
            
        hr = THR( pDoc->Select( pStart, pEnd, SELECTION_TYPE_Control ));
        if ( hr )
            goto Cleanup;
                                    
    }                                
Cleanup:
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pIElement );
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     Getlength
//
//  Synopsis:   SiteRange object model
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::get_length(long * plSize)
{
    *plSize = _aryElements.Size();

    RRETURN(SetErrorInfo(S_OK));
}


//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   SiteRange object model Method
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::item ( long lIndex, IHTMLElement ** ppElem )
{
    HRESULT     hr = E_INVALIDARG;
    CElement *  pElement;

    if (! ppElem)
        goto Cleanup;

    *ppElem = NULL;

    // Check Index validity, too low
    if (lIndex < 0)
        goto Cleanup;

    // ... too high
    if (lIndex >=_aryElements.Size())
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    // ... just right    
    pElement = _aryElements[ lIndex ];

    if (! pElement)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = THR( pElement->QueryInterface(IID_IHTMLElement,
          (void **)ppElem));
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     scrollIntoView
//
//  Synopsis:   scroll the first control into view
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::scrollIntoView (VARIANTARG varargStart)
{
    // BUGBUG: The old code worked only if a site was already
    //         selected.

    HRESULT     hr = CTL_E_METHODNOTAPPLICABLE;
    CElement *  pElement;

    if (! _aryElements.Size() )
        goto Cleanup;

    //
    // Multiple selection not supported, only the first item
    // is scrolled into view
    //
    pElement = _aryElements[ 0 ]; 
    if (! pElement)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = pElement->scrollIntoView(varargStart);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     commonParentElement()
//
//  Synopsis:   Return the common parent for elements in the control range
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoTxtSiteRange::commonParentElement ( IHTMLElement ** ppParent )
{
    HRESULT     hr = S_OK;
    LONG        nSites;
    CTreeNode * pNodeCommon;
    int         i;

    //
    // Check incoming pointer
    //
    if (!ppParent)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppParent = NULL;

    // BUGBUG: The old code worked only if sites were already
    //         selected.
    // nSites = GetSelectedSites(&ppLayout);

    nSites = _aryElements.Size();

    if (nSites <= 0)
        goto Cleanup;

    //
    // Loop through the elements to find their common parent
    //
    pNodeCommon = _aryElements[ 0 ]->GetFirstBranch();
    for(i = 1; i < nSites; i++)
    {
        pNodeCommon = _aryElements[ i ]->GetFirstBranch()->GetFirstCommonAncestor(pNodeCommon, NULL);
    }

    if (! pNodeCommon)
        goto Cleanup;

    hr = THR( pNodeCommon->Element()->QueryInterface( IID_IHTMLElement, (void **) ppParent ) );
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo(hr) );

}


//+--------------------------------------------------------------------------
//
// Member : CTxtSiteRange::Exec
//
// Sysnopsis : deal with commands related to sites in text
//
//+--------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT         hr = S_OK;
    int             iElement;
    CDoc         *  pDoc;
    CElement     *  pElement = NULL;
    AAINDEX         aaindex;
    IUnknown *      pUnk = NULL;
    
    Assert( _pElementOwner );
    pDoc = _pElementOwner->Doc();
    Assert( pDoc );

    aaindex = FindAAIndex(DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
    if (aaindex != AA_IDX_UNKNOWN)
    {
        hr = THR(GetUnknownObjectAt(aaindex, &pUnk));
        if (hr)
            goto Cleanup;
    }
    
    //
    // Allow the elements a chance to handle the command
    //
    for ( iElement = 0; iElement < _aryElements.Size(); iElement++ )
    {
        pElement = _aryElements.Item( iElement );

        if (! pElement)
            break;       

        if (pUnk)
        {
            pElement->AddUnknownObject(
                DISPID_INTERNAL_INVOKECONTEXT, pUnk, CAttrValue::AA_Internal);
        }
        
        hr = THR( pElement->Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));
        if (pUnk)
        {
            pElement->FindAAIndexAndDelete(
                DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        }
        
        if (hr)
            break;
    }

    //
    // Route the command using the edit router, unless all 
    // elements handled it already
    //
    if (hr || !pElement)
    {
        hr = THR( _EditRouter.ExecEditCommand(pguidCmdGroup,
                                        nCmdID, nCmdexecopt,
                                        pvarargIn, pvarargOut,
                                        (IUnknown *) (IHTMLControlRange *)this, 
                                        pDoc ) );                
    }

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN(hr);
}


VOID 
CAutoTxtSiteRange::QueryStatusSitesNeeded(MSOCMD *pCmd, INT cSitesNeeded)
{
    pCmd->cmdf = (cSitesNeeded <= _aryElements.Size()) ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED; 
}
//+--------------------------------------------------------------------------
//
// Member : CAutoTxtSiteRange::QueryStatus
//
// Sysnopsis : deal with commands related to sites in text
//
//+--------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::QueryStatus(
    GUID * pguidCmdGroup,
    ULONG cCmds,
    MSOCMD rgCmds[],
    MSOCMDTEXT * pcmdtext)
{
    MSOCMD *    pCmd = &rgCmds[0];
    HRESULT     hr = S_OK;
    CDoc  *     pDoc;
    DWORD       cmdID;

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert( _pElementOwner );
    pDoc = _pElementOwner->Doc();
    Assert( pDoc );
    
    cmdID = CBase::IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
    switch (cmdID)
    {        
    case IDM_DYNSRCPLAY:
    case IDM_DYNSRCSTOP:

    case IDM_BROWSEMODE:
    case IDM_EDITMODE:
    case IDM_REFRESH:
    case IDM_REDO:
    case IDM_UNDO:
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        break;
        
    case IDM_SIZETOCONTROLWIDTH:
    case IDM_SIZETOCONTROLHEIGHT:
    case IDM_SIZETOCONTROL:
        QueryStatusSitesNeeded(pCmd, 2);
        break;

    case IDM_SIZETOFIT:
        QueryStatusSitesNeeded(pCmd, 1);
        break;
        
    case IDM_CODE:
    break;

    case IDM_OVERWRITE:
    case IDM_SELECTALL:
    case IDM_CLEARSELECTION:
        // Delegate this command to document
        if(_pElementOwner != NULL && _pElementOwner->Doc()!= NULL)
        {
            hr = _pElementOwner->Doc()->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
        }
        break;
    break;

    default:
        // Make sure we have at least one contol
        QueryStatusSitesNeeded(pCmd, 1);

        if (pCmd->cmdf == MSOCMDSTATE_DISABLED)
        {
            break;
        }

        // Delegate to the edit router
        hr = _EditRouter.QueryStatusEditCommand(
                    pguidCmdGroup,
                    1,
                    pCmd,
                    pcmdtext,
                    (IUnknown *) (IHTMLControlRange *)this,
                    pDoc );

    }

    RRETURN_NOTRACE(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandSupported
//
//  Synopsis:
//
//  Returns: returns true if given command (like bold) is supported
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandSupported(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandSupported(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandEnabled
//
//  Synopsis:
//
//  Returns: returns true if given command is currently enabled. For toolbar
//          buttons not being enabled means being grayed.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandEnabled(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandEnabled(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandState
//
//  Synopsis:
//
//  Returns: returns true if given command is on. For toolbar buttons this
//          means being down. Note that a command button can be disabled
//          and also be down.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandState(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandState(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandIndeterm
//
//  Synopsis:
//
//  Returns: returns true if given command is in indetermined state.
//          If this value is TRUE the value returnd by queryCommandState
//          should be ignored.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandIndeterm(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandIndeterm(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandText
//
//  Synopsis:
//
//  Returns: Returns the text that describes the command (eg bold)
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandText(BSTR bstrCmdId, BSTR *pcmdText)
{
    RRETURN(CBase::queryCommandText(bstrCmdId, pcmdText));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandValue
//
//  Synopsis:
//
//  Returns: Returns the  command value like font name or size.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandValue(BSTR bstrCmdId, VARIANT *pvarRet)
{
    RRETURN(CBase::queryCommandValue(bstrCmdId, pvarRet));
}


//+------------------------------------------------------------------------
//
//  Member:     CAutoTxtSiteRange::execCommand
//
//  Synopsis:
//
//-------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRange::execCommand(BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT varValue,
                                        VARIANT_BOOL *pfRet)
{
    HRESULT hr = S_OK;
    BOOL fAllow;

    Assert(_pElementOwner);
    Assert(_pElementOwner->Doc());

    hr = THR(_pElementOwner->Doc()->AllowClipboardAccess(bstrCmdId, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;           // Fail silently

    hr = CBase::execCommand(bstrCmdId, showUI, varValue);

    if (pfRet)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     execCommandShowHelp
//
//  Synopsis:
//
//  Returns:
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::execCommandShowHelp(BSTR cmdId, VARIANT_BOOL *pfRet)
{
    HRESULT   hr;
    DWORD     dwCmdId;

    // Convert the command ID from string to number
    hr = CmdIDFromCmdName(cmdId, &dwCmdId);
    if(hr)
        goto Cleanup;

    hr = THR(CBase::execCommandShowHelp(cmdId));

Cleanup:
    if(pfRet != NULL)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     MovePointersToSegment()
//
//  Synopsis:   ISegmentList method implementation that moves two pointers
//              to a given segment index. The segment index corresponds to
//              an element in the control range. Pointers are moved before
//              and after the element.
//
//-------------------------------------------------------------------------

HRESULT 
CAutoTxtSiteRange::MovePointersToSegment ( 
    int iSegmentIndex, 
    IMarkupPointer * pStart, 
    IMarkupPointer * pEnd ) 
{
    HRESULT         hr;
    CElement     *  pElement;
    IHTMLElement *  pHTMLElement = NULL;

    if ( iSegmentIndex < 0 || iSegmentIndex >= _aryElements.Size() )    
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (! (pStart && pEnd) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pElement = _aryElements.Item( iSegmentIndex );

    Assert( pElement );

    hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void **) & pHTMLElement ) );
    if ( hr )
        goto Cleanup;

    // 
    // Move the pointers around the element
    //

    hr = THR( pStart->MoveAdjacentToElement( pHTMLElement , ELEM_ADJ_BeforeBegin ) ); 
    if ( !hr )    
        hr = THR( pEnd->MoveAdjacentToElement( pHTMLElement , ELEM_ADJ_AfterEnd ) ); 

    //
    // set gravity
    //
     
    if ( !hr )  hr = pStart->SetGravity( POINTER_GRAVITY_Right );            
    if ( !hr )  hr = pEnd->SetGravity( POINTER_GRAVITY_Left );                

    if (hr) 
        goto Cleanup;
    
Cleanup:        
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}    


//+------------------------------------------------------------------------
//
//  Member:     GetSegmentCount()
//
//  Synopsis:   ISegmentList method implementation that retruns the number
//              of elements in the control range.
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::GetSegmentCount(
    int* piSegmentCount,
    SELECTION_TYPE *peSelectionType )
{
    HRESULT hr = S_FALSE;

    if ( piSegmentCount )
    {
        *piSegmentCount = _aryElements.Size();
        hr = S_OK;
    }
    if ( peSelectionType )
        *peSelectionType = SELECTION_TYPE_Control;
        
    RRETURN( hr );
}
