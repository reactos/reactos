//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       irange.cxx
//
//  Contents:   Implementation of the CAutoRange class
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_OMRECT_HXX_
#define X_OMRECT_HXX_
#include "omrect.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif


#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifdef UNIX
#ifndef X_QUXCOPY_H_
#define X_QUXCOPY_H_
#include "quxcopy.hxx"
#endif
#endif

#ifdef QUILL

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "..\lequill\quilglue.hxx"
#endif

#endif  // QUILL

#define _cxx_
#include "range.hdl"

MtDefine(CAutoRange, ObjectModel, "CAutoRange")
MtDefine(CAutoRangeGetRangeBoundingRect_aryRects_pv, Locals, "CAutoRange::GetRangeBoudningRect aryRects::_pv")
MtDefine(CAutoRangegetClientRects_aryRects_pv, Locals, "CAutoRange::getClientRects aryRects::_pv")
MtDefine(CAutoRange_paryAdjacentRangePointers_pv, Locals , "CAutoRange::_paryAdjacentRangePointers::_pv")


const CBase::CLASSDESC CAutoRange::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // property pages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    & IID_IHTMLTxtRange,            // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

// IOleCommandTarget methods

BEGIN_TEAROFF_TABLE(CAutoRange, IOleCommandTarget)
    TEAROFF_METHOD(CAutoRange, QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext))
    TEAROFF_METHOD(CAutoRange, Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut))
END_TEAROFF_TABLE()

//+----------------------------------------------------------------------------
//
//  Constructor / Destructor
//
//-----------------------------------------------------------------------------

CAutoRange::CAutoRange ( CMarkup * pMarkup, CElement * pElemContainer )
{
    Assert(!pElemContainer || pElemContainer->GetMarkup() == pMarkup);

    CElement::SetPtr( & _pElemContainer, pElemContainer );

    _pMarkup = pMarkup;

    _pMarkup->AddRef();

    _pLeft = _pRight = NULL;

    _pNextRange = NULL;

    InitPointers();
}


CAutoRange::~CAutoRange()
{
    RemoveLookAsideEntry();

    ClearAdjacentRangePointers();

    ReleaseInterface( _pLeft );
    
    ReleaseInterface( _pRight );

    CElement::ClearPtr( & _pElemContainer );

    _EditRouter.Passivate();
    
    _pMarkup->Release();
}


//+----------------------------------------------------------------------------
//
//  Member:     RemoveLookAsideEntry
//
//  Synopsis:   The markup keeps a lookaside pointer which is a list of 
//              active CAutoRanges on the markup. This routine clears this list.
//
//-----------------------------------------------------------------------------

void
CAutoRange::RemoveLookAsideEntry()
{
    WHEN_DBG( BOOL fFound = FALSE );

    CAutoRange * pAutoRange = _pMarkup->GetTextRangeListPtr();

    Assert( pAutoRange );

    if ( pAutoRange == this )
    {
        CAutoRange * pLookAsideRange = pAutoRange->_pNextRange;

        _pMarkup->DelTextRangeListPtr();

        if ( pLookAsideRange )
        {
            IGNORE_HR( _pMarkup->SetTextRangeListPtr( pLookAsideRange ) );
        }
        WHEN_DBG( fFound = TRUE );
    }
    else
    {
        CAutoRange * pPrevRange = pAutoRange;

        pAutoRange = pAutoRange->_pNextRange;

        Assert( pAutoRange );

        while ( pAutoRange )
        {
            if ( pAutoRange == this )
            {
                Assert ( pPrevRange );
                pPrevRange->_pNextRange = pAutoRange->_pNextRange;
                WHEN_DBG( fFound = TRUE );
                break;
            }

            pPrevRange = pAutoRange;
            pAutoRange = pAutoRange->_pNextRange;
        }     
    }

    Assert( fFound );
}


//+----------------------------------------------------------------------------
//
//  Member:     AdjustRangePointerGravity
//
//  Synopsis:   This function compares the passed in pRangePointer to _pLeft.
//              if pRangePointer has right gravity and it is equal to _pLeft
//              then its gravity is temporarily set to POINTER_GRAVITY_Left, 
//              in preparation for the upcoming put_text operation. 
//              This prevents pther range pointers around _pLeft to accidently move
//              right, when InsertText() is called.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::AdjustRangePointerGravity( IMarkupPointer * pRangePointer )
{
    BOOL        fEqual;
    HRESULT     hr;
    POINTER_GRAVITY eGravity;

    hr = THR( pRangePointer->Gravity( &eGravity ) );
    if (hr)
        goto Cleanup;

    if ( eGravity == POINTER_GRAVITY_Left )
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR( _pLeft->IsEqualTo( pRangePointer, & fEqual ) );
    if (hr)
        goto Cleanup;

    if ( fEqual )
    {
        hr = THR( pRangePointer->SetGravity( POINTER_GRAVITY_Left ) );
        if (hr)
            goto Cleanup;

        hr = THR( StoreAdjacentRangePointer( pRangePointer ) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureAdjacentRangesGravity
//
//  Synopsis:   This function calls AdjustRangePointerGravity() on all the 
//              ranges in the current markup. Please see comments under
//              AdjustRangePointerGravity for more info.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::EnsureAdjacentRangesGravity()
{
    HRESULT      hr = S_OK;
    CAutoRange * pAutoRange = _pMarkup->GetTextRangeListPtr();
    Assert( pAutoRange );

    while ( pAutoRange )
    {
        if ( pAutoRange != this )
        {
            hr = THR( AdjustRangePointerGravity( pAutoRange->_pLeft ) );
            if (hr)
                goto Cleanup;
            
            hr = THR( AdjustRangePointerGravity( pAutoRange->_pRight ) );
            if (hr)
                goto Cleanup;
        }

        pAutoRange = pAutoRange->_pNextRange;
    }

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     RestoreAdjacentRangesGravity
//
//  Synopsis:   This function restores the gravity of range pointers whose gravity
//              was temporarily set by AdjustRangePointerGravity, and clears the
//              _paryAdjacentRangePointers list.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::RestoreAdjacentRangesGravity()
{
    HRESULT      hr = S_OK;
    long         iPointer;
    IMarkupPointer * pRangePointer;

    if (! _paryAdjacentRangePointers)
    {
        // Nothing to do
        goto Cleanup;
    }

    iPointer = _paryAdjacentRangePointers->Size() - 1;

    while ( iPointer >= 0 )
    {        
        pRangePointer = _paryAdjacentRangePointers->Item( iPointer );

        Assert( pRangePointer );

        if ( pRangePointer )
        {
            hr = THR( pRangePointer->SetGravity( POINTER_GRAVITY_Right ) );
            if (hr)
                goto Cleanup;        

            pRangePointer->Release();
        }

        _paryAdjacentRangePointers->Delete( iPointer );        
        
        iPointer--;
    }

    ClearAdjacentRangePointers();

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     ClearAdjacentRangePointers
//
//  Synopsis:   Delete the list of adjacent range pointers array.
//
//-----------------------------------------------------------------------------

void
CAutoRange::ClearAdjacentRangePointers()
{
    if ( _paryAdjacentRangePointers )
    {
        delete _paryAdjacentRangePointers;
        _paryAdjacentRangePointers = NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     StoreAdjacentRangePointer
//
//  Synopsis:   Append a range pointer whose gravity was set by AdjustRangePointerGravity
//              to the list of adjacent range pointers array.
//
//-----------------------------------------------------------------------------

HRESULT 
CAutoRange::StoreAdjacentRangePointer( IMarkupPointer * pAdjacentPointer )
{
    HRESULT hr = S_OK;

    if ( ! _paryAdjacentRangePointers )
    {
        _paryAdjacentRangePointers = new ( Mt( CAutoRange_paryAdjacentRangePointers_pv ) )  
                    CPtrAry<IMarkupPointer *> ( Mt( CAutoRange_paryAdjacentRangePointers_pv ) )  ;

        if (! _paryAdjacentRangePointers) 
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;        
        }
    }

    pAdjacentPointer->AddRef();

    hr = THR( _paryAdjacentRangePointers->Append( pAdjacentPointer ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     IsCompatibleWith
//
//  Synopsis:   Makes sure two pointers are in the same region of the tree.
//              Operations involving two pointers need to be compatible.
//
//-----------------------------------------------------------------------------

BOOL
CAutoRange::IsCompatibleWith ( IHTMLTxtRange * pIRangeThat )
{
    CAutoRange * pRangeThat;

    if (pIRangeThat->QueryInterface( CLSID_CRange, (void **) & pRangeThat ) != S_OK)
        return FALSE;

    return pRangeThat->GetContainer() == GetContainer();
}

//+----------------------------------------------------------------------------
//
//  Member:     SanityCheck
//
//  Synopsis:   Do a little self diagnosis
//
//-----------------------------------------------------------------------------

#if DBG == 1

void
CAutoRange::SanityCheck ( )
{
}

#define DO_SANITY_CHECK SanityCheck();

#else

#define DO_SANITY_CHECK

#endif


//+----------------------------------------------------------------------------
//
//  Member:     QueryStatus
//
//  Synopsis:   Implements IOleCommandTarget::QueryStatus() for IHTMLTxtRange
//
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::QueryStatus (GUID * pguidCmdGroup,
                         ULONG cCmds,
                         MSOCMD rgCmds[],
                         MSOCMDTEXT * pcmdtext)
{
    HRESULT     hr = S_OK;
    MSOCMD *    pCmd;
    INT         c;
    DWORD       cmdID;

    Assert( CBase::IsCmdGroupSupported( pguidCmdGroup ) );

    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        cmdID = CBase::IDMFromCmdID( pguidCmdGroup, pCmd->cmdID );
        
        switch ( cmdID )
        {
            case IDM_SIZETOCONTROL:
            case IDM_SIZETOCONTROLHEIGHT:
            case IDM_SIZETOCONTROLWIDTH:
            case IDM_DYNSRCPLAY:
            case IDM_DYNSRCSTOP:

            case IDM_BROWSEMODE:
            case IDM_EDITMODE:
            case IDM_REFRESH:
            case IDM_REDO:
            case IDM_UNDO:
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
                break;

            default:
                hr = _EditRouter.QueryStatusEditCommand(
                            pguidCmdGroup,
                            1,
                            pCmd,
                            pcmdtext,
                            (IUnknown *) (IHTMLTxtRange *)this,
                            GetMarkup()->Doc() );
        }
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     Exec
//
//  Synopsis:   Implements IOleCommandTarget::Exec() for IHTMLTxtRange
//
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::Exec (GUID * pguidCmdGroup,
                  DWORD nCmdID,
                  DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut)
{
    HRESULT     hr;
    int         idm;

    idm = CBase::IDMFromCmdID( pguidCmdGroup, nCmdID );

    if ( CheckOwnerSiteOrSelection(idm) )
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    //
    // Route command using the edit router...
    //
    hr = THR( _EditRouter.ExecEditCommand(pguidCmdGroup,
                                     nCmdID, nCmdexecopt,
                                     pvarargIn, pvarargOut,
                                     (IUnknown *) (IHTMLTxtRange *)this,
                                     GetMarkup()->Doc() ) );

    //
    // For commands that perform an insert, collapse the range 
    // after the insertion point
    //
    if (hr)
        goto Cleanup;

    switch( idm )
    {
    case IDM_IMAGE:
    case IDM_PARAGRAPH:
    case IDM_IFRAME:
    case IDM_TEXTBOX:
    case IDM_TEXTAREA:
#ifdef  NEVER
    case IDM_HTMLAREA:
#endif
    case IDM_CHECKBOX:
    case IDM_RADIOBUTTON:
    case IDM_DROPDOWNBOX:
    case IDM_LISTBOX:
    case IDM_BUTTON:
    case IDM_MARQUEE:
    case IDM_1D:
    case IDM_LINEBREAKNORMAL:
    case IDM_LINEBREAKLEFT:
    case IDM_LINEBREAKRIGHT:
    case IDM_LINEBREAKBOTH:
    case IDM_HORIZONTALLINE:
    case IDM_INSINPUTBUTTON:
    case IDM_INSINPUTIMAGE:
    case IDM_INSINPUTRESET:
    case IDM_INSINPUTSUBMIT:
    case IDM_INSINPUTUPLOAD:
    case IDM_INSFIELDSET:
    case IDM_INSINPUTHIDDEN:
    case IDM_INSINPUTPASSWORD:
    case IDM_PASTE:
        {
            BOOL fResult;

            hr = THR( _pLeft->IsRightOf( _pRight, & fResult ) );

            if ( fResult )
                hr = THR( _pRight->MoveToPointer( _pLeft ) );
            else
                hr = THR( _pLeft->MoveToPointer( _pRight ) );
        }
    }

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     CAutoRange::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    DO_SANITY_CHECK

    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLTxtRange)
        QI_TEAROFF(this, IOleCommandTarget, (IHTMLTxtRange *)this)
        QI_INHERITS(this, IHTMLTxtRange)
        QI_INHERITS(this, ISegmentList)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IHTMLTextRangeMetrics, NULL)
        QI_TEAROFF(this, IHTMLTextRangeMetrics2, NULL)

        default:
            if (IsEqualIID(iid, CLSID_CRange))
            {
                *ppv = this;
                return S_OK;
            }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CloseErrorInfo, IHTMLTxtRange
//
//  Synopsis:   Pass the call to the form so it can return its clsid
//              instead of the object's clsid as in CBase.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::CloseErrorInfo(HRESULT hr)
{
    GetMarkup()->Doc()->CloseErrorInfo( hr );

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     GetText, IHTMLTxtRange
//
//  Synopsis:   Gets the simple text from the range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::get_text( BSTR * pText )
{
    HRESULT hr = E_POINTER;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (pText)
    {
        hr = THR( KeepRangeLeftToRight() );
        if (hr)
            goto Cleanup;

        // BUGBUG: Work around a saver bug when range is empty
        if ( IsRangeCollapsed() )
        {
            hr = FormsAllocString(_T( "" ), pText);
        }
        else
        {
            hr = THR(GetBstrHelper(pText, RSF_SELECTION, WBF_SAVE_PLAINTEXT|WBF_NO_WRAP));  // new
        }

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( SetErrorInfoPGet( hr, DISPID_CAutoRange_text ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     SetText, IHTMLTxtRange
//
//  Synopsis:   Sets the simple text in the range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::put_text( BSTR Text )
{
    HRESULT hr;
    CMarkup *   pMarkup = GetMarkup();
    CDoc     *  pDoc = pMarkup->Doc();
    CParentUndo pu( pDoc );
    CElement *  pContainer;
    IHTMLEditingServices * pedserv = NULL;
    IHTMLEditor * phtmed;
    BOOL          result;

    Assert( pDoc );
    Assert( pMarkup );

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    pContainer = GetCommonContainer();

    if (pContainer && pContainer->TestClassFlag(CElement::ELEMENTDESC_OMREADONLY))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if( pContainer->IsEditable() )
        pu.Start( IDS_UNDOGENERICTEXT );

    if (!CheckSecurity(L"paste"))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    //
    // Get the editing services interface with which I can
    // insert sanitized text
    //

    phtmed = pDoc->GetHTMLEditor();

    if (!phtmed)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(
        phtmed->QueryInterface(
            IID_IHTMLEditingServices, (void **) & pedserv ) );
    if (hr)
        goto Cleanup;

    hr = THR( EnsureAdjacentRangesGravity() );
    if (hr)
        goto Cleanup;

    hr = THR( pedserv->Paste( _pLeft, _pRight, Text ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->IsRightOf( _pRight, & result ) );
    if ( result )
        hr = THR( _pRight->MoveToPointer( _pLeft ) );
    else
        hr = THR( _pLeft->MoveToPointer( _pRight ) );
    if (hr)
        goto Cleanup;

    hr = THR( RestoreAdjacentRangesGravity() );
    if (hr)
        goto Cleanup;

    hr = THR( ValidatePointers() );
    if (hr)
        goto Cleanup;


Cleanup:
    pu.Finish( hr );
    ReleaseInterface( pedserv );
    RRETURN( SetErrorInfoPSet( hr, DISPID_CAutoRange_text ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     GetText, IHTMLTxtRange
//
//  Synopsis:   Gets the HTML (fragment) text from the range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::get_htmlText( BSTR * ppHtmlText )
{
    HRESULT hr = S_OK;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!ppHtmlText)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( GetBstrHelper(ppHtmlText, RSF_FRAGMENT, 0) );
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfoPGet( hr, DISPID_CAutoRange_htmlText ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     parentElement, IHTMLTxtRange
//
//  Synopsis:   Get the common parent element for all chars int the range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::parentElement ( IHTMLElement * * ppParent )
{
    HRESULT           hr;
    IMarkupPointer  * pCurrent = NULL;
    IMarkupPointer  * pAdjustedLeft = NULL;
    IMarkupPointer  * pPointer = NULL;
    IHTMLElement    * pOldElement = NULL;
    CMarkup         * pMarkup = GetMarkup();
    CDoc            * pDoc;
    CElement        * pElement = NULL;
    BOOL              fEqual;
    BOOL              fRangeIsCollapsed;
    BOOL              fUseRightForParent = FALSE;
    CMarkupPointer  * pointerLeft;
    CMarkupPointer  * pointerRight;
    BOOL              fBlockBreakAdjustment = FALSE;
    MARKUP_CONTEXT_TYPE context;    
    IMarkupPointer    * pLeftBoundary  = NULL;
    IMarkupPointer    * pRightBoundary = NULL;

    Assert( pMarkup );

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Check incoming pointer
    //

    if (!ppParent)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppParent = NULL;
    
    pDoc = pMarkup->Doc();
    Assert( pDoc );

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->CreateMarkupPointer( & pCurrent ) );
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->CreateMarkupPointer( & pAdjustedLeft ) );
    if (hr)
        goto Cleanup;

    //
    // SetRangeToElementText() adjusts the left end of the range just before a character.
    // Here we must undo the adjustment to achieve IE4 compat for parentElement()
    //

    hr = THR( pAdjustedLeft->MoveToPointer( _pLeft ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->IsEqualTo(_pRight, &fRangeIsCollapsed) );
    if (hr)
        goto Cleanup;
        
    if (fRangeIsCollapsed)
    {
        hr = THR( pAdjustedLeft->MoveToPointer( _pLeft ) );
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR( MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
        if (hr)
            goto Cleanup;

        hr = THR( pCurrent->MoveToPointer( _pLeft ) );
        if (hr)
            goto Cleanup;

        hr = THR( MoveCharacter( pCurrent, MOVEUNIT_NEXTCHAR, pLeftBoundary, pRightBoundary, pAdjustedLeft ) );
        if (hr)
            goto Cleanup;

        hr = THR( pAdjustedLeft->IsRightOfOrEqualTo( _pRight, & fEqual ) );
        if (hr)
            goto Cleanup;

        if ( fEqual )
        {
            hr = THR( pAdjustedLeft->MoveToPointer( _pLeft ) );
            if (hr)
                goto Cleanup;
        }
        
        hr = THR( pAdjustedLeft->IsEqualTo( _pRight, & fRangeIsCollapsed ) );
        if (hr)
            goto Cleanup;
    }


    //
    // IE4 compat: If _pLeft and _pRight are around a noscope element that has layout,
    //             we want to return the noscope element as the parentElement for the range
    //
    hr = THR( pCurrent->MoveToPointer( pAdjustedLeft ) ) ;
    if (hr)
        goto Cleanup;

    hr = THR( pCurrent->Right( TRUE, & context, ppParent , NULL, NULL ) );
    if (hr)
        goto Cleanup;

    if ( context == CONTEXT_TYPE_NoScope && *ppParent )
    {
        hr = THR( (*ppParent)->QueryInterface( CLSID_CElement, (void **) & pElement ) );
        if (hr)
            goto Cleanup;
            
        if ( pElement->HasLayout() )
        {
            hr = THR( pCurrent->IsEqualTo( _pRight, & fEqual ) );
            if (hr)
                goto Cleanup;

            if ( fEqual )
            {
                // return ppParent as the parent element 
                // of the range
                goto Cleanup;
            }
        }
    }        
    // 
    // End IE4 compat, reset ppParent back to NULL and find the parent element
    //
    ClearInterface( ppParent ); 

    //
    // If the right end of the range is placed after a block break, we would
    // like to adjust it inside the element before detecting the parent element
    //
    {
        DWORD           dwBreaks;

        hr = THR( pDoc->CreateMarkupPointer( & pPointer ) );
        if (hr)
            goto Cleanup;

        hr = THR( pPointer->MoveToPointer( _pRight ) );
        if (hr)
            goto Cleanup;

        hr = THR( pPointer->Left( TRUE, & context, NULL, NULL, NULL ) );
        if (hr)
            goto Cleanup;

        //
        // We're looking for cases where the right pointer
        // is immediately after the end scope of an element that has
        // a block break
        //
        if ( context == CONTEXT_TYPE_EnterScope )
        {
            // We've enter the scope of an element, now check for block breaks
            hr = THR( pDoc->QueryBreaks( pPointer, & dwBreaks, FALSE) );
            if (hr)
                goto Cleanup;

            // A Site End is almost the same as a Block Break, so handle that too
            if ( (dwBreaks & (BREAK_BLOCK_BREAK | BREAK_SITE_END)) != BREAK_NONE )
            {
                // Adjust _pRight to the left
                hr = THR( _pRight->Left( TRUE, NULL, NULL, NULL, NULL ) );
                if (hr)
                    goto Cleanup;
                fBlockBreakAdjustment = TRUE;
                fUseRightForParent = TRUE;
            }
        }
    }    

    if (fUseRightForParent)
        hr = THR( _pRight->CurrentScope( ppParent ) );
    else
        hr = THR( pAdjustedLeft->CurrentScope( ppParent ) );

    if (hr)
        goto Cleanup;

    if (fRangeIsCollapsed && fBlockBreakAdjustment)
    {
        fBlockBreakAdjustment = FALSE;
        hr = THR( _pRight->Right( TRUE, NULL, NULL, NULL, NULL ) );
        if (hr)
            goto Cleanup;
    }

    //
    // CurrentScope return NULL, we must be in a TXTSLAVE,
    // return the master element as the parent element of the range
    //
    if ( *ppParent == NULL )
    {
        hr = THR( pAdjustedLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pointerLeft ) );
        if( hr )
            goto Cleanup;

        hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pointerRight ) );
        if( hr )
            goto Cleanup;

        if ( pointerLeft->Markup() == pointerRight->Markup() )
        {
            // If both pointers are in the same markup, return the master
            pElement = pointerLeft->Markup()->Master();
            if ( !pElement )
                goto Cleanup;

            hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void **) ppParent ) );
            // Return ppParent as the parent element
            goto Cleanup;
        }
        else
        {
            // if the range pointers are not in the same markup
            // set ppParent to the TXTSLAVE and find the common parent
            // in the loop below...
            ClearInterface( ppParent );
            hr = THR( pDoc->CurrentScopeOrSlave( pAdjustedLeft, ppParent ) );
            if (hr)
                goto Cleanup;
        }
    }        

    //
    // If we have a current scope and the range is collapsed, return ppParent
    //
    if ( fRangeIsCollapsed )
    {
        hr = S_OK;
        goto Cleanup;
    }

    //
    // Walk the left pointer up until the right end of the element
    // is to the right of _pRight
    //
    while ( *ppParent )
    {
        BOOL fResult;
        
        hr = THR( pCurrent->MoveAdjacentToElement( *ppParent, ELEM_ADJ_AfterEnd ) );
        if (hr)
            goto Cleanup;

        hr = THR( pCurrent->IsLeftOf( _pRight, & fResult ) );

        if (!fResult)
            break; // found common element

        pOldElement = *ppParent;                
        hr = THR( pOldElement->get_parentElement( ppParent ) );
        pOldElement->Release();
        if (hr)
            goto Cleanup;        
        Assert( *ppParent ); // we should never walk up past the root of the tree
    }
    
Cleanup:
    ReleaseInterface( pAdjustedLeft );
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    ReleaseInterface(pCurrent);
    ReleaseInterface(pPointer);
    if (hr)
    {
        ClearInterface( ppParent );
    }

    if ( fBlockBreakAdjustment )
    {
        hr = THR( _pRight->Right( TRUE, NULL, NULL, NULL, NULL ) );
    }

    RRETURN( SetErrorInfo( hr ) );
}


//+----------------------------------------------------------------------------
//
//  Member:     Duplicate, IHTMLTxtRange
//
//  Synopsis:   Create a duplicate range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::duplicate ( IHTMLTxtRange * * ppTheClone )
{
    HRESULT hr = S_OK;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    //
    // Create the new range
    //

    hr = THR( GetMarkup()->createTextRange( ppTheClone, _pElemContainer, _pLeft, _pRight, FALSE ) );

    if (hr)
        goto Cleanup;

    Assert( *ppTheClone );

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}


//+----------------------------------------------------------------------------
//
//  Member:     InRange, IHTMLTxtRange
//
//  Synopsis:   Determine whether the passed in range in within my range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::inRange ( IHTMLTxtRange * pDispRhs, VARIANT_BOOL * pfInRange )
{
    BOOL         fResult;
    HRESULT      hr = S_OK;   
    CAutoRange * pRangeRhs;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!pfInRange || !pDispRhs)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // Make sure the two ranges belong to the same ped (story)
    //

    hr = THR( pDispRhs->QueryInterface( CLSID_CRange, (void**) & pRangeRhs ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert( pRangeRhs );

    if (GetMarkup() != pRangeRhs->GetMarkup() || !IsCompatibleWith( pDispRhs ))
    {
        *pfInRange = VB_FALSE;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( pRangeRhs->KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    //
    // Now, compare for containment
    // Ranges are ordered left to right, so the following two tests are sufficient
    //
    
    *pfInRange = VB_FALSE; // Assume the result is false

    hr = THR( pRangeRhs->_pLeft->IsLeftOf( _pLeft, & fResult ) );
    if (hr || fResult )
        goto Cleanup;

    hr = THR( _pRight->IsLeftOf( pRangeRhs->_pRight, & fResult ) );
    if (hr || fResult )
        goto Cleanup;

    *pfInRange = VB_TRUE; // Passed the tests so set the result to true

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     IsEqual, IHTMLTxtRange
//
//  Synopsis:   Is the passed in range equal to me?
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::isEqual ( IHTMLTxtRange * pDispRhs, VARIANT_BOOL * pfIsEqual )
{
    BOOL         fResult;
    HRESULT      hr = S_OK;
    CAutoRange * pRangeRhs;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!pfIsEqual)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // Make sure the two ranges belong to the same ped (story)
    //

    hr = THR( pDispRhs->QueryInterface( CLSID_CRange, (void**) & pRangeRhs ) );
    
    if ( hr )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
        
    Assert( pRangeRhs );

    if ( GetMarkup() != pRangeRhs->GetMarkup()  || !IsCompatibleWith( pDispRhs ) )
    {
        *pfIsEqual = VB_FALSE;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( pRangeRhs->KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    //
    // Now, compare the ends
    //

    *pfIsEqual = VB_FALSE; 

    hr = THR( _pLeft->IsEqualTo( pRangeRhs->_pLeft, & fResult ) );
    if (hr || !fResult)
        goto Cleanup;

    hr = THR( _pRight->IsEqualTo( pRangeRhs->_pRight, & fResult ) );
    if (hr || !fResult)
        goto Cleanup;

    *pfIsEqual = VB_TRUE;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     ScrollIntoView, IHTMLTxtRange
//
//  Synopsis:   Scroll range into view
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::scrollIntoView ( VARIANT_BOOL fStart )
{
    HRESULT     hr = S_OK;
    CTreeNode * pNode;
    SCROLLPIN   sp = fStart ? SP_MINIMAL : SP_BOTTOMRIGHT;
    CMarkupPointer * pRightInternal = NULL;
    CMarkupPointer * pLeftInternal = NULL;
    LONG cpMin, cpMost;
    CMarkup *   pMarkup = GetMarkup();
    CDoc     *  pDoc = pMarkup->Doc();
    CFlowLayout* pFlowLayout = NULL;
    
    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Scroll the end of the selection into view
    pNode = RightNode();
    if (! pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    pFlowLayout = pDoc->GetFlowLayoutForSelection( pNode );

    Assert( pNode->Element());
    
    if ( pFlowLayout && (
         ( pFlowLayout == pNode->GetUpdatedNearestLayout() ) || 
         ( pNode->Element()->_etag == ETAG_TXTSLAVE ) ) )
    {
        hr = THR( _pLeft->QueryInterface(CLSID_CMarkupPointer, (void **)&pLeftInternal) );
        if (hr)
            goto Cleanup;

        hr = THR( _pRight->QueryInterface(CLSID_CMarkupPointer, (void **)&pRightInternal) );
        if (hr)
            goto Cleanup;

        // We are only interested in the end of the selection, so cut off the beginning of 
        // range to avoid the ScrollRangeIntoView asserts

        cpMin  = max(pLeftInternal->GetCp(), pFlowLayout->GetContentFirstCp());
        cpMost = pRightInternal->GetCp();
        
        hr = THR( pFlowLayout->ScrollRangeIntoView( cpMin, cpMost, sp, sp ) );
    }
    else
    {
        hr = THR( pNode->Element()->ScrollIntoView() );
    }

Cleanup:
    // S_FALSE can be returned if we were asked to scroll something that didn't have a 
    // display.  this is not an error but there is nothing that needs to be done.
    RRETURN1( SetErrorInfo( hr ), S_FALSE );
}


//+----------------------------------------------------------------------------
//
//  Member:     collapse, IHTMLTxtRange
//
//  Synopsis:   Collapse the range pointers at start or end
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::collapse ( VARIANT_BOOL fStart )
{
    HRESULT hr = S_OK;

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    if  (fStart)
    {
        hr = THR( _pRight->MoveToPointer( _pLeft ) );
    }
    else
    {
        hr = THR( _pLeft->MoveToPointer( _pRight ) );
    }

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     Helper function MoveWithinBoundary
//
//  Synopsis:   Wrapper for CMarkupPointer::MoveUnit() which keeps the passed in 
//              pointer within the range boundary.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::MoveWithinBoundary( IMarkupPointer *pPointerToMove, 
                               MOVEUNIT_ACTION muAction, 
                               IMarkupPointer *pBoundary,
                               BOOL            fLeftBound )
{
    IMarkupPointer *    pPointer = NULL;
    BOOL                fSuccess = FALSE;
    HRESULT             hr;

    hr = THR( 
            GetMarkup()->Doc()->CreateMarkupPointer( & pPointer ) );
    if (hr)
        goto Cleanup;

    hr = THR( pPointer->MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR( pPointer->MoveUnit( muAction ) );
    if (hr)
        goto Cleanup;
    
    if ( fLeftBound )
    {
        BOOL fResult;
        
        hr = THR( pPointer->IsRightOfOrEqualTo( pBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        // Going left we ended up to the right of the boundry
        if ( fResult )
        {
            fSuccess = TRUE;
        }
    }
    else
    {
        BOOL fResult;
        
        hr = THR( pPointer->IsLeftOfOrEqualTo( pBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        // Going right we ended up to the left of the boundry
        if ( fResult )
        {
            fSuccess = TRUE;
        }
    }

    if ( fSuccess )
    {
        hr = THR( pPointerToMove->MoveToPointer( pPointer ) );
        if (hr)
        {
            fSuccess = FALSE;
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface( pPointer );
    return fSuccess ? S_OK : S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     Expand, IHTMLTxtRange
//
//  Synopsis:   Expands the range by the given unit
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::expand ( BSTR bstrUnit, VARIANT_BOOL * pfSuccess )
{
    HRESULT             hr;
    htmlUnit            Unit;
    IMarkupPointer    * pLeftBoundary  = NULL;
    IMarkupPointer    * pRightBoundary = NULL;
    IMarkupPointer    * pPointer = NULL;
    long                nCountLeft, nCountRight;
    BOOL                fEqual;
    BOOL                fLeftWasAlreadyExpanded = FALSE;
    BOOL                fRightWasAlreadyExpanded = FALSE;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!pfSuccess)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pfSuccess = VB_FALSE;

    //
    // Determine what action must take place based on bstrUnit
    //

    hr = ENUMFROMSTRING ( htmlUnit, bstrUnit, (long *)&Unit );
    if ( hr )
        goto Cleanup;

    hr = THR( ValidatePointers() );
    if (hr)
        goto Cleanup;

    switch (Unit)
    {
    case 2: // "Word"
    {

        // Move Left to the beginning of the previous word unless if Left is at beginning of the word already
        // Check End of prev word and compare it to the start of current word
        hr = THR( 
                GetMarkup()->Doc()->CreateMarkupPointer( & pPointer ) );
        if (hr)
            goto Cleanup;

        hr = THR( 
                MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
        if (hr)
            goto Cleanup;

        // 
        // If the left end of the range is at the end of document
        // we cannot expand by a word.
        //
        hr = THR( pPointer->MoveToPointer( _pLeft ) );
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE( MoveWithinBoundary( pPointer, MOVEUNIT_NEXTWORDEND, pRightBoundary, FALSE ) );
        if (hr)
        {
            break;
        }

        //
        // The next two MoveWord() calls will place _pLeft at the beginning of the current word,
        // note that MoveWord() unlike MoveUnit() will account for block breaks, BRs, and boundaries
        //
        hr = THR( pPointer->MoveToPointer( _pLeft ) );
        if (hr)
            goto Cleanup;

        hr = THR( MoveWord( pPointer, MOVEUNIT_NEXTWORDBEGIN, pLeftBoundary, pRightBoundary ) );        
        if (hr)
            goto Cleanup;

        hr = THR( MoveWord( pPointer, MOVEUNIT_PREVWORDBEGIN, pLeftBoundary, pRightBoundary ) );        
        if (hr)
            goto Cleanup;

        hr = THR( pPointer->IsEqualTo( _pLeft, & fEqual ) );
        if ( fEqual )
        {
            fLeftWasAlreadyExpanded = TRUE;
        }
        else
        {
            hr = THR( _pLeft->MoveToPointer( pPointer ) );
            if (hr)
                goto Cleanup;
        }

        //
        // Move _pRight to the beginning of the next word to include trailing spaces.
        // If there are no trailing spaces, move it to the end of the next word.
        // As usual, check for corner cases in the end of the document.
        //

        hr = THR( pPointer->MoveToPointer( _pRight ) );
        if (hr)
            goto Cleanup;

        hr = THR( MoveWord( pPointer, MOVEUNIT_PREVWORDBEGIN, pLeftBoundary, pRightBoundary ) );        
        if (hr)
            goto Cleanup;

        hr = THR( MoveWord( pPointer, MOVEUNIT_NEXTWORDBEGIN, pLeftBoundary, pRightBoundary ) );
        if (hr)
            goto Cleanup;

        hr = THR( pPointer->IsEqualTo( _pRight, & fEqual ) );
        if ( fEqual )
        {
            //
            // pRight is at the end or beginning of a word, if pRight is not equal to pLeft
            // we want to expand the word, otherwise, we're already expanded
            //
            hr = THR( _pLeft->IsEqualTo( _pRight, & fEqual ) );
            if (fEqual)
            {
                hr = THR( MoveWord( _pRight, MOVEUNIT_NEXTWORDBEGIN, pLeftBoundary, pRightBoundary ) );                
                if (hr)
                    goto Cleanup;
            }
            else
            {
                fRightWasAlreadyExpanded = TRUE;
            }
        }
        else
        {
            hr = THR( _pRight->MoveToPointer( pPointer ) );
            if (hr)
                goto Cleanup;
        }
        break;
    
    }

    case 3: // "Sentence"
    {
        nCountRight = 1;
        hr  = MoveUnitWithinRange( _pRight, MOVEUNIT_NEXTSENTENCE, & nCountRight );
        if (hr)
            goto Cleanup;

        if (! nCountRight)
        {
            hr = S_FALSE;
            break;
        }

        hr = THR( _pLeft->MoveToPointer( _pRight ) );
        if (hr)
            goto Cleanup;

        nCountLeft = -1;
        hr  = MoveUnitWithinRange( _pLeft, MOVEUNIT_PREVSENTENCE, & nCountLeft );
        if (hr)
            goto Cleanup;

        hr  = (nCountLeft == -1 && nCountRight == 1) ? S_OK : S_FALSE ;
        break;
    }

    case 1: // "Character"   
    {
        nCountRight = 1;
        hr  = MoveUnitWithinRange( _pRight, MOVEUNIT_NEXTCHAR, & nCountRight );
        if (hr)
            goto Cleanup;

        hr  = (nCountRight == 1) ? S_OK : S_FALSE ;
        break;
    }

    case 6: // "TextEdit"
    {
        //
        // Move range pointers to right/left borders to move by textedit units
        //
        hr = THR( 
                MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pRight->MoveToPointer( pRightBoundary ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pLeft->MoveToPointer( pLeftBoundary ) );
        if (hr)
            goto Cleanup;
            
        break;
    }

    default:
        hr = E_NOTIMPL;
        goto Cleanup;
    }
    
    if (hr == S_OK && (!fLeftWasAlreadyExpanded || !fRightWasAlreadyExpanded) )
    {
        *pfSuccess = VB_TRUE;
    }
    else
    {
        *pfSuccess = VB_FALSE;
        hr = S_OK;
    }

Cleanup:
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    ReleaseInterface( pPointer );
    hr = THR( ValidatePointers() );

    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     GetMoveUnitAndType
//
//  Synopsis:   Expands the range by the given unit
//
//-----------------------------------------------------------------------------

HRESULT 
CAutoRange::GetMoveUnitAndType( BSTR bstrUnit, long Count, 
                                MOVEUNIT_ACTION     * pmuAction,
                                htmlUnit            * phtmlUnit )
{
    HRESULT     hr = S_OK;

    hr = THR( ENUMFROMSTRING( htmlUnit, bstrUnit, (long *) phtmlUnit ) );
    if (hr)
        goto Cleanup;

    switch (*phtmlUnit)
    {
    case 2: // "Word"
        if( Count > 0 )
            *pmuAction = MOVEUNIT_NEXTWORDBEGIN;
        else
            *pmuAction = MOVEUNIT_PREVWORDBEGIN;
        *phtmlUnit = htmlUnit_Max;
        break;

    case 3: // "Sentence"
        if( Count > 0 )
            *pmuAction = MOVEUNIT_NEXTSENTENCE;
        else
            *pmuAction = MOVEUNIT_PREVSENTENCE;
        *phtmlUnit = htmlUnit_Max;
        break;

    case 1: // "Character"   
        if( Count > 0 )
            *pmuAction = MOVEUNIT_NEXTCHAR; 
        else
            *pmuAction = MOVEUNIT_PREVCHAR; 
        *phtmlUnit = htmlUnit_Max;
        break;

    case 6: // "TextEdit"
        //
        // Since MoveUnit() does not handle TextEdit, this case does not set
        // pmuType, instead phtmlUnit is set properly and will be used by the caller 
        //
        break;

    default:
        hr = E_NOTIMPL;
        *phtmlUnit = htmlUnit_Max;
        break;
    }

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     moveToPoint, IHTMLTextRange
//
//  Synopsis:   Clips the point to the client rect of the ped, finds the site
//              which was hit by the point, and within that site finds the
//              cp at which the point lies.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::moveToPoint ( long x, long y )
{
    HRESULT       hr = S_OK;        // The return value
    POINT         pt;               // The point
    CMarkup     * pMarkup;          // The markup for the range
    CElement    * pElementClient;
    CFlowLayout * pFlowLayout;
    CTreeNode    * pNode;
    CHitTestInfo   hti;
    HITTESTRESULTS htr;
    CDispNode    * pDisp=NULL;
    CLayout      * pElemLayout = NULL;
#ifdef QUILL
    IMarkupPointer * pTempPointer = NULL;
    CMarkupPointer * pPointer = NULL;

#endif

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pMarkup = GetMarkup();
    pElementClient = pMarkup->GetElementClient();
    Assert(pElementClient);
    pElemLayout = pElementClient->GetUpdatedLayout();

    if(!pElemLayout)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    Assert(pElemLayout);

    //
    // Construct the point
    //
    pt.x = x;
    pt.y = y;

    //
    // Clip/Restrict the point to the client rect of the ped
    //
    pElemLayout->RestrictPointToClientRect(&pt);

    //
    // Wait for the ped and its parents to recalc, so that we hit test the right stuff
    //
    pElementClient->GetFlowLayout()->WaitForParentToRecalc(-1, pt.y, NULL);

    //
    // Find the site within the ped which was hit. Note that we cannot call the doc's
    // HitTestPoint() because if there are sites which are hovering above this ped,
    // then we will hit test those sites. We want to hit test within our own layout

    htr._fWantArrow = FALSE;
    htr._fRightOfCp = FALSE;
    htr._cpHit = 0;
    htr._iliHit = 0;
    htr._ichHit = 0;
    htr._cchPreChars = 0;

    hti._grfFlags = HT_VIRTUALHITTEST;
    hti._htc = HTC_NO;
    hti._pNodeElement = NULL;
    hti._pDispNode = NULL;
    hti._ptContent.x = x;
    hti._ptContent.y = y;
    hti._phtr = &htr;

    pDisp = pElemLayout->GetElementDispNode(pElementClient);

    if (! pElemLayout->HitTestContent(&pt, pDisp, &hti))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    Assert(hti._htc == HTC_YES);

    //
    // Got the site which was hit. Get the txtsite containing the site (we will always find
    // the ped)> Also be sure that the ped of txtsite we get is the same as the ped of the
    // range. (We could have a text box inside the ped and the point over it -- in this
    // case we do not want the txtsite belonging to the text box. Rather we want the parent
    // text site of the text box).
    //
    // BUGBUG: (SLAVE_TREE) do we need to iterate through markups? (jbeda)
    pNode = hti._pNodeElement->GetFlowLayoutNode();
    while (pNode->GetMarkup() != pMarkup)
    {
        pNode = pNode->Parent()->GetFlowLayoutNode();
        Assert(pNode);
    }

    pFlowLayout = pNode->HasFlowLayout();
    Assert(pFlowLayout);

    //
    // From the display of the textsite we hit, get the cp
    //
#ifdef QUILL
    if (pFlowLayout->FExternalLayout())
    {
        long cp = 0;

        if (pFlowLayout->GetQuillGlue())
        {
            cp = pFlowLayout->GetQuillGlue()->CpFromPoint(pt, NULL, FALSE, FALSE, TRUE, NULL, NULL);
        }

        Assert(cp >= 0);

        if (cp < 0)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // we need to get from the cp to a markup in order to set _pRight and _pLeft
        // we CAN'T call MoveMarkupToPoint() for these for the same reason that we
        // couldn't call cdoc's HitTestPoint above.
        //-------------------------------------------------------------------------
        hr = THR(pMarkup->Doc()->CreateMarkupPointer( &pTempPointer ) );
        if (hr)
            goto Cleanup;

        hr = THR( pTempPointer->QueryInterface( CLSID_CMarkupPointer, (void **) &pPointer ));
        if( FAILED( hr ))
            goto Cleanup;

        hr = pPointer->MoveToCp( cp, pMarkup );
        if( hr )
            goto Cleanup;

        hr = THR(SetLeftAndRight(pPointer, pPointer));
    }
    else
#endif  // QUILL

    {
        CTreeNode         * pTreeNode =NULL;
        POINT               ptContent;
        CMarkupPointer      HitMarkup(pMarkup->Doc());
        BOOL                bNotAtBOL;
        BOOL                bAtLogicalBOL;
        BOOL                bRightOfCp;

        // we need to get from the cp to a markup in order to set _pRight and _pLeft
        // we CAN'T call MoveMarkupToPoint() for these for the same reason that we
        // couldn't call cdoc's HitTestPoint above.
        //-------------------------------------------------------------------------
        // position the markup pointer at the point that we are given
        pTreeNode = pMarkup->Doc()->GetNodeFromPoint( pt, TRUE, &ptContent );
        if( pTreeNode == NULL )
            goto Cleanup;

        hr = THR( pMarkup->Doc()->MovePointerToPointInternal( ptContent, 
                                                            pTreeNode, 
                                                            &HitMarkup, 
                                                            &bNotAtBOL,
                                                            &bAtLogicalBOL,
                                                            &bRightOfCp, 
                                                            FALSE,
                                                            pFlowLayout));
        if (hr)
            goto Cleanup;


        hr = THR(SetLeftAndRight(&HitMarkup, &HitMarkup));

    }
Cleanup:
#ifdef QUILL
    ReleaseInterface(pTempPointer);
#endif

    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     setEndPoint, IHTMLTextRange
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::setEndPoint ( BSTR bstrHow, IHTMLTxtRange * pHTMLRangeSource )
{
    HRESULT hr = S_OK;
    htmlEndPoints how;
    CAutoRange * pRangeSource;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!pHTMLRangeSource)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!IsCompatibleWith( pHTMLRangeSource ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( ENUMFROMSTRING( htmlEndPoints, bstrHow, (long *) & how ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        pHTMLRangeSource->QueryInterface(
            CLSID_CRange, (void **) & pRangeSource ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( pRangeSource->KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    switch ( how )
    {
    case htmlEndPointsStartToStart : 
        SetLeft( pRangeSource->_pLeft );
        break;

    case htmlEndPointsStartToEnd :
        SetLeft( pRangeSource->_pRight );
        break;

    case htmlEndPointsEndToStart :
        SetRight( pRangeSource->_pLeft );
        break;

    case htmlEndPointsEndToEnd :
        SetRight( pRangeSource->_pRight );
        break;

    default:
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( ValidatePointers() );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     CompareRangePointers() 
//              helper function for compareEndPoints()
//
//  Synopsis:   Use IE4 compatible comparison for range pointers
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::CompareRangePointers( IMarkupPointer * pPointerSource, IMarkupPointer * pPointerTarget, int * piReturn )
{
    HRESULT     hr;
    BOOL        fLeft;
    CDoc      * pDoc;
    BOOL        fResult;
    CTreeNode * pNode;
    MARKUP_CONTEXT_TYPE context;

    pDoc = GetMarkup()->Doc();
    Assert( pDoc );
    CMarkupPointer  pointer( pDoc );

    hr = THR( OldCompare( pPointerSource, pPointerTarget, piReturn ) );
    if (hr)
        goto Cleanup;

    if ( *piReturn == 0 )
    {
        // If the pointers are equal we're done
        goto Cleanup;
    }

    //
    // If Source is greater than Target, the direction is going to be left to right,
    // otherwise we'll go right to left
    //
    fLeft = ( *piReturn == 1 );

    hr = THR( pointer.MoveToPointer( pPointerSource ) );
    if (hr)
        goto Cleanup;

    //
    // We're going to move  from source to target, if we don't encounter any text, 
    // block or layout when we get to target, the range pointers are equal.
    //

    for ( ; ; )
    {
        if( fLeft )
        {
            hr = THR( pointer.Left( TRUE, & context, & pNode, NULL, NULL, NULL ) );
        }
        else
        {
            hr = THR( pointer.Right( TRUE, & context, & pNode, NULL, NULL, NULL ) );
        }

        if( hr )
            goto Cleanup;

        switch( context )
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
                if( pNode )
                {
                    if ( pNode->HasLayout() || pNode->Element()->IsBlockElement() )
                    {
                        goto Cleanup;
                    }
                }
                break;

            case CONTEXT_TYPE_Text:            
            case CONTEXT_TYPE_NoScope:
            case CONTEXT_TYPE_None:
                goto Cleanup;

        }
        
        //
        // Check our situation
        //
        hr = THR( pointer.IsEqualTo( pPointerTarget, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            // We successfully moved from source to target, so consider the 
            // pointers equal, and take us outta here
            *piReturn = 0;
            goto Cleanup;
        }

        //
        // If we've gone past the Target, we're done, otherwise keep looping...
        //
        if (fLeft)
        {
            hr = THR( pointer.IsLeftOf( pPointerTarget, & fResult ) );
            if (hr)
                goto Cleanup;

            if ( fResult )
            {
                goto Cleanup;
            }
        }
        else
        {
            hr = THR( pointer.IsRightOf( pPointerTarget, & fResult ) );
            if (hr)
                goto Cleanup;

            if ( fResult )
            {
                goto Cleanup;
            }
        }
    }

Cleanup:
    RRETURN( hr );
}
        

//+----------------------------------------------------------------------------
//
//  Member:     compareEndPoints, IHTMLTextRange
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::compareEndPoints (
    BSTR bstrHow, IHTMLTxtRange * pHTMLRangeSource, long * pReturn )
{
    HRESULT         hr = S_OK;
    htmlEndPoints   how;
    CAutoRange *    pRangeSource;
    int             iReturn;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!pHTMLRangeSource || !pReturn)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!IsCompatibleWith( pHTMLRangeSource ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pReturn = 0;

    hr = THR( ENUMFROMSTRING( htmlEndPoints, bstrHow, (long *) & how ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        pHTMLRangeSource->QueryInterface(
            CLSID_CRange, (void **) & pRangeSource ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( pRangeSource->KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    switch ( how )
    {
    case htmlEndPointsStartToStart :
        hr = THR( CompareRangePointers( _pLeft, pRangeSource->_pLeft, & iReturn ) );
        break;

    case htmlEndPointsStartToEnd :
        hr = THR( CompareRangePointers( _pLeft, pRangeSource->_pRight, & iReturn ) );
        break;

    case htmlEndPointsEndToStart :
        hr = THR( CompareRangePointers( _pRight, pRangeSource->_pLeft, & iReturn ) );
        break;

    case htmlEndPointsEndToEnd :
        hr = THR( CompareRangePointers( _pRight, pRangeSource->_pRight, & iReturn ) );
        break;

    default:
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *pReturn = iReturn;

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}


//+----------------------------------------------------------------------------
//
//  Member: CAutoRange::getBookmark
//
//  Synopsis: Passes through to its markup pointers, asking them for a
//            bookmark representing this range.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::getBookmark ( BSTR * pbstrBookmark )
{
    HRESULT hr = S_OK;
    CMarkupPointer * pLeft, * pRight;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pLeft ) );

    if (hr)
        goto Cleanup;

    hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pRight ) );

    if (hr)
        goto Cleanup;

    hr = THR( pLeft->GetBookmark( pbstrBookmark, pRight ) );

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}


//+----------------------------------------------------------------------------
//
//  Member: CAutoRange::moveToBookmark
//
//  Synopsis: Passes through to its markup pointers, asking them to position
//            themselves at this bookmark
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::moveToBookmark ( BSTR bstrBookmark, VARIANT_BOOL * pvbSuccess )
{
    HRESULT hr = S_OK;
    CMarkupPointer * pLeft, * pRight;

    DO_SANITY_CHECK

    Assert( pvbSuccess );
    *pvbSuccess = VB_FALSE;

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pLeft ) );

    if (hr)
        goto Cleanup;

    hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pRight ) );

    if (hr)
        goto Cleanup;

    
    hr = THR( pLeft->MoveToBookmark( bstrBookmark, pRight ) );
    
    if( S_OK == hr )
    {
        *pvbSuccess = TRUE;
    }
    else if( S_FALSE == hr )
    {
        // S_FALSE means we didn't find the bookmark, but that's not an error.
        hr = S_OK;
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


//+----------------------------------------------------------------------------
//
//  Member:     moveToElementText, IHTMLTxtIRange
//
//  Synopsis:   move the range to encompass an element
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::moveToElementText ( IHTMLElement * element )
{
    HRESULT hr = S_OK;
    CElement * pElement;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!element)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR( element->QueryInterface( CLSID_CElement, (void **) & pElement ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    hr = THR( SetTextRangeToElement( pElement ) );

    if (hr == S_FALSE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}


//+----------------------------------------------------------------------------
//
//  Member:     IsTextInIE4CompatiblePlace, Helper function
//
//  Synopsis:   If text found using findText() is within an OPTION tag, IE4 would
//              not have found it, used for IE4 compat.
//
//-----------------------------------------------------------------------------

BOOL
CAutoRange::IsTextInIE4CompatiblePlace( IMarkupPointer * pmpLeft, IMarkupPointer * pmpRight )
{
    ELEMENT_TAG     eTag;
    CTreeNode *     pNode;
    CMarkupPointer* pmp = NULL;

    IGNORE_HR( pmpLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    Assert( pmp );

    pNode = pmp->CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode)
        return FALSE;
    eTag = pNode->Tag();    
    if (eTag == ETAG_OPTION)
        return FALSE;

    IGNORE_HR( pmpRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    
    Assert( pmp );

    pNode = pmp->CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode)
        return FALSE;
    eTag = pNode->Tag();    
    if (eTag == ETAG_OPTION)
        return FALSE;
    
    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Member:     findText, IHTMLTxtIRange
//
//  Synopsis:   findText within range boundaries
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::findText ( BSTR String, long count, long Flags, VARIANT_BOOL * pfSuccess )
{
    HRESULT          hr = S_OK;
    IMarkupPointer * pLeftBoundary = NULL;
    IMarkupPointer * pRightBoundary = NULL;
    IMarkupPointer * pmpLeft = NULL;
    IMarkupPointer * pmpRight = NULL;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // are all the required parameters provided
    if (!String || !pfSuccess)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // did we actually get a searchstring with text in it
    if (!SysStringLen( String ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    //
    // Position two temporary pointers that will be used for finding text
    //
    hr = THR( 
            GetMarkup()->Doc()->CreateMarkupPointer( & pmpLeft ) );
    if (hr) 
        goto Cleanup;

    hr = THR( 
            GetMarkup()->Doc()->CreateMarkupPointer( & pmpRight ) );
    if (hr) 
        goto Cleanup;

    hr = THR( 
            pmpLeft->MoveToPointer( _pLeft ) );
    if (hr)
        goto Cleanup;

    hr = THR( 
            pmpRight->MoveToPointer( _pRight ) );
    if (hr)
        goto Cleanup;

    //
    // Position the range boundry pointers
    //
    hr = THR( 
            MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
    if (hr)
        goto Cleanup;

    //
    // Call FindText(), for compat reasons we do not want to find text inside an OPTION
    //

    if (count == 0)
    {
        hr = THR( pmpLeft->FindText( String, Flags, pmpRight, pmpRight ) );

        if (hr == S_FALSE)
        {
            hr = S_OK;
            *pfSuccess = VB_FALSE;
            goto Cleanup;
        }

        goto FoundIt;
    }
        
    if ( count < 0 )
        Flags = Flags | FINDTEXT_BACKWARDS;

    for ( ; ; )
    {
        if ( Flags & FINDTEXT_BACKWARDS ) 
        {
            hr = THR( pmpRight->FindText( String, Flags, pmpLeft, pLeftBoundary ) );
        }
        else
        {            
            hr = THR( pmpLeft->FindText( String, Flags, pmpRight, pRightBoundary ) );
        }

        if (hr == S_FALSE)
        {
            // not found
            hr = S_OK;
            *pfSuccess = VB_FALSE;
            goto Cleanup;
        }

        if (hr)
            goto Cleanup;

        if ( IsTextInIE4CompatiblePlace( pmpLeft, pmpRight ) )
            goto FoundIt;

        long cch = 1;
            
        if ( Flags & FINDTEXT_BACKWARDS ) 
            IGNORE_HR( pmpRight->Left( TRUE, NULL, NULL, & cch, NULL ));
        else
            IGNORE_HR( pmpLeft->Right( TRUE, NULL, NULL, & cch, NULL ));
    }

Cleanup:
    
    ReleaseInterface( pmpLeft );
    ReleaseInterface( pmpRight );
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    
    RRETURN( SetErrorInfo( hr ) );

FoundIt:
    
    *pfSuccess = VB_TRUE;
    
    IGNORE_HR( _pLeft->MoveToPointer( pmpLeft ) );
    IGNORE_HR( _pRight->MoveToPointer( pmpRight ) );
    
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Member:     MoveStart, IHTMLTxtRange
//
//  Synopsis:   move the left end of the range per parameters passed
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CAutoRange::moveStart ( BSTR bstrUnit, long Count, long * pActualCount )
{
    return THR( moveRange( bstrUnit, Count, pActualCount, MOVERANGE_Left ) );
}

//+----------------------------------------------------------------------------
//
//  Member:     MoveEnd, IHTMLTxtRange
//
//  Synopsis:   move the right end of the range per parameters passed
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::moveEnd ( BSTR bstrUnit, long Count, long * pActualCount )
{
    return THR( moveRange( bstrUnit, Count, pActualCount, MOVERANGE_Right ) );
}


//+----------------------------------------------------------------------------
//
//  Member:     Move, TxtIRange
//
//  Synopsis:   move the left end of the range per parameters passed, 
//              and collapse the range
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::move ( BSTR bstrUnit, long Count, long * pActualCount )
{
    return THR( moveRange( bstrUnit, Count, pActualCount, MOVERANGE_Both ) );
}


//+----------------------------------------------------------------------------
//
//  Member:     MovePointersToRangeBoundary 
//
//  Synopsis:   Position pointers around the boundry of _pElemContainer
//
//-----------------------------------------------------------------------------
HRESULT
CAutoRange::MovePointersToRangeBoundary ( IMarkupPointer ** ppLeftBoundary,
                                          IMarkupPointer ** ppRightBoundary )
{
    HRESULT             hr = S_OK;
    IHTMLElement      * pHTMLElement = NULL; 
    CDoc              * pDoc = NULL;
    CMarkup           * pMarkup = NULL;
    //
    // Set the left and right borders around _pElemContainer. 
    // Range pointers shall not move outside these boundries or else...
    // 
    if (! _pElemContainer)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR_NOTRACE( 
            _pElemContainer->QueryInterface( IID_IHTMLElement, (void **) & pHTMLElement ) );
    if (hr)
        goto Cleanup;
    
    pMarkup = GetMarkup();     
    pDoc = pMarkup->Doc();

    Assert( pMarkup );
    Assert( pDoc );

    hr = THR( pDoc->CreateMarkupPointer( ppLeftBoundary ) );
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->CreateMarkupPointer( ppRightBoundary ) );
    if (hr)
        goto Cleanup;

    hr = THR( (*ppLeftBoundary)->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterBegin ) );
    if (hr)
        goto Cleanup;

    hr = THR( (*ppRightBoundary)->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_BeforeEnd ) );
    if (hr)
        goto Cleanup;

    //
    // Make sure the bounderies cling to text, just like the
    // range pointers do
    //

    hr = THR( AdjustPointers( *ppLeftBoundary, *ppRightBoundary ));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     MoveCharacter
//
//  Synopsis:   Move the range by a single character according to IE4 rules
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::MoveCharacter ( IMarkupPointer * pPointerToMove,
                            MOVEUNIT_ACTION  muAction,
                            IMarkupPointer * pLeftBoundary,
                            IMarkupPointer * pRightBoundary,
                            IMarkupPointer * pJustBefore )
{
    HRESULT hr;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    CTreeNode       *   pNode;
    CDoc            *   pDoc = GetMarkup()->Doc();
    DWORD               dwBreaks;
    IHTMLElement    *   pIElement = NULL;
    CMarkupPointer      pointerSource( pDoc );
    BOOL                fResult;

    extern BOOL IsIntrinsicTag( ELEMENT_TAG eTag );

    Assert ( muAction == MOVEUNIT_PREVCHAR ||  muAction == MOVEUNIT_NEXTCHAR );

    hr = THR( pointerSource.MoveToPointer( pPointerToMove ) );
    
    if (hr)
        goto Cleanup;

    if (pJustBefore)
    {
        hr = THR( pJustBefore->MoveToPointer( & pointerSource ) );

        if (hr)
            goto Cleanup;
    }

    //
    // If we are starting out at a block break
    //
    
    if (muAction == MOVEUNIT_NEXTCHAR)
    {
        hr = THR( pointerSource.QueryBreaks( & dwBreaks ) );
        
        if (hr)
            goto Cleanup;
            
        if (dwBreaks != BREAK_NONE)
        {
            hr = THR( pointerSource.Right( TRUE, NULL, NULL, NULL, NULL ) );

            if (hr)
                goto Cleanup;

            goto MoveIt;
        }
    }
    
    //
    // Walk pointerSource in the right direction, looking
    // for IE4 compatible text
    //    

    for ( ; ; )
    {
        long cch = 1;
        
        if (pJustBefore)
        {
            hr = THR( pJustBefore->MoveToPointer( & pointerSource ) );

            if (hr)
                goto Cleanup;
        }

        if (muAction == MOVEUNIT_PREVCHAR)
        {
            hr = THR( pointerSource.Left( TRUE, & context, & pNode , & cch, NULL, NULL ));

            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR( pointerSource.Right( TRUE, & context, & pNode , & cch, NULL, NULL ));
            
            if (hr)
                goto Cleanup;
        }

        switch( context )
        {
        //
        // Nowhere to go - we bail.
        //
            
        case CONTEXT_TYPE_None:
        case CONTEXT_TYPE_Text:
            goto MoveIt;

        case CONTEXT_TYPE_NoScope:

            if ( pNode->Tag() == ETAG_BR      ||
                 pNode->Tag() == ETAG_SCRIPT  ||
                 pNode->Element()->HasLayout() )
            {
                // Per IE4, if we pass a BR, SCRIPT or noscope with a layout 
                // before hitting text, then we're done. 
                goto MoveIt;
            }
            break;

        case CONTEXT_TYPE_EnterScope:
        case CONTEXT_TYPE_ExitScope:
            
            Assert( pNode );

            if (IsIntrinsicTag( pNode->Tag() ))
            {
                // Move over intrinsics (e.g. <BUTTON>), don't go inside them like MoveUnit() does.
                // Here the passed in pointer is set before or after the intrinsic based
                // on our direction and whether or not we've traveled over text before.
                
                ClearInterface( & pIElement );
                
                hr = THR( pNode->Element()->QueryInterface( IID_IHTMLElement, (void **) & pIElement ) );
                
                if (hr)
                    goto Cleanup;

                if ( muAction == MOVEUNIT_NEXTCHAR )
                    hr = THR( pointerSource.MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ) );
                else
                    hr = THR( pointerSource.MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin ) );
                
                goto MoveIt;
            }
            
            break;            
        }

        //
        // Check whether we have hit a break before reaching text
        //
        
        hr = THR( pointerSource.QueryBreaks( & dwBreaks ) );
        
        if (hr)
            goto Cleanup;

        if (dwBreaks != BREAK_NONE)
        {
            // If I hit a break while going right, I want to be after the break
            // going left however, I want to stop right where I am.
            //
            
            if (muAction == MOVEUNIT_NEXTCHAR)
            {
                long cch = 1;
                
                hr = THR( pointerSource.Right( TRUE, & context, & pNode , &cch, NULL, NULL ));
                
                if (hr)
                    goto Cleanup;
            }

            goto MoveIt;
        }

    }

Cleanup:
    
    ReleaseInterface( pIElement );
    
    RRETURN( hr );
    
MoveIt:

    fResult = FALSE;

    if (muAction == MOVEUNIT_PREVCHAR)
    {
        hr = THR( pointerSource.IsLeftOf( pLeftBoundary, & fResult ) );

        if (hr)
            goto Cleanup;

        if (fResult)
        {
            hr = THR( pointerSource.MoveToPointer( pLeftBoundary ) );

            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = THR( pointerSource.IsRightOf( pRightBoundary, & fResult ) );

        if (hr)
            goto Cleanup;

        if (fResult)
        {
            hr = THR( pointerSource.MoveToPointer( pRightBoundary ) );

            if (hr)
                goto Cleanup;
        }
    }
    
    if (fResult && pJustBefore)
    {
        hr = THR( pJustBefore->MoveToPointer( & pointerSource ) );

        if (hr)
            goto Cleanup;
    }

    hr = THR( pPointerToMove->MoveToPointer( & pointerSource ) );
    
    if (hr)
        goto Cleanup;

    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Member:     MoveWord
//
//  Synopsis:   MoveWord() is a wrapper for MoveUnit(). It stops at IE4 word breaks
//              that MoveUnit() ignores such as:
//              <BR>, Block Break, TSB, TSE, and Intrinsics 
//              The only muActions supported are MOVEUNIT_PREVWORDBEGIN and 
//              MOVEUNIT_NEXTWORDBEGIN
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::MoveWord ( IMarkupPointer * pPointerToMove,
                       MOVEUNIT_ACTION  muAction,
                       IMarkupPointer * pLeftBoundary,
                       IMarkupPointer * pRightBoundary )
{
    HRESULT hr;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    CTreeNode       *   pNode;
    CDoc            *   pDoc;
    DWORD               dwBreaks;
    BOOL                fResult;
    BOOL                fPassedText;
    IHTMLElement    *   pIElement = NULL;

    extern BOOL IsIntrinsicTag( ELEMENT_TAG eTag );

    Assert ( muAction == MOVEUNIT_PREVWORDBEGIN || 
             muAction == MOVEUNIT_NEXTWORDBEGIN );

    pDoc = GetMarkup()->Doc();
    Assert( pDoc );

    //
    // pPointerDestination is where MoveUnit() would have positioned us, however, 
    // since MoveUnit() does not account for IE4 word breaks like intrinsics,
    // Block Breaks, text site begin/ends, and Line breaks, we use another pointer
    // called pPointerSource. This pointer walks towards pPointerDestination
    // to detect IE4 word breaking characters that MoveUnit() does not catch.
    //
    CMarkupPointer  pointerSource( pDoc );
    CMarkupPointer  pointerDestination( pDoc );

    hr = THR( 
            pointerSource.MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR( 
            pointerDestination.MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR(
            pointerDestination.MoveUnit( muAction ) );
    if (hr)
        goto Cleanup;

    //
    // MoveUnit() may place the destination outside the range boundary. 
    // First make sure that the destination is within range boundaries.
    //
    if ( muAction == MOVEUNIT_PREVWORDBEGIN )
    {
        hr = THR( pointerDestination.IsLeftOf( pLeftBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            hr = THR( pointerDestination.MoveToPointer( pLeftBoundary ) );
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = THR( pointerDestination.IsRightOf( pRightBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            hr = THR( pointerDestination.MoveToPointer( pRightBoundary ) );
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Walk pointerSource towards pointerDestination, looking
    // for word breaks that MoveUnit() might have missed.
    //
    
    fPassedText = FALSE;

    for ( ; ; )
    {
        if ( muAction == MOVEUNIT_PREVWORDBEGIN )
        {
            hr = THR( pointerSource.Left( TRUE, & context, & pNode , NULL, NULL, NULL ));
        }
        else
        {
            hr = THR( pointerSource.Right( TRUE, & context, & pNode , NULL, NULL, NULL ));
        }
        if ( hr )
            goto Cleanup;

        switch( context )
        {
        case CONTEXT_TYPE_None:
            break;
            
        case CONTEXT_TYPE_Text:
            fPassedText = TRUE;
            break;

        case CONTEXT_TYPE_NoScope:
        case CONTEXT_TYPE_EnterScope:
        case CONTEXT_TYPE_ExitScope:
            if ( !pNode )
                break;

            if ( IsIntrinsicTag( pNode->Tag() ) )
            {
                // Move over intrinsics (e.g. <BUTTON>), don't go inside them like MoveUnit() does.
                // Here the passed in pointer is set before or after the intrinsic based
                // on our direction and whether or not we've travelled over text before.
                ClearInterface( & pIElement );
                hr = THR( pNode->Element()->QueryInterface( IID_IHTMLElement, (void **) & pIElement ) );
                if (hr)
                    goto Cleanup;

                if ( muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    hr = THR( pPointerToMove->MoveAdjacentToElement( pIElement, 
                                fPassedText ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
                }
                else
                {
                    hr = THR( pPointerToMove->MoveAdjacentToElement( pIElement, 
                                fPassedText ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ) );
                }
                //
                // We're done
                //
                goto Cleanup;
            }

            
            // <BR> is a word break 
            if ( pNode->Tag() == ETAG_BR )
            {
                if ( fPassedText && muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    // If we're travelling right and have passed some text, backup
                    // before the last BR, we've gone too far.
                    hr = THR( pointerSource.Left( TRUE, NULL, NULL, NULL, NULL, NULL ));
                    goto Done;
                }
                else if ( muAction == MOVEUNIT_PREVWORDBEGIN )
                {
                    // Travelling left we are at the right place: we're at the beginning of <BR>
                    // which is a valid word break.
                    goto Done;
                }                 
                else
                {
                    fPassedText = TRUE;
                }           
            }
            else if ( pNode->Element()->IsBlockElement() ||
                      pNode->Element()->HasLayout()      ||
                      context == CONTEXT_TYPE_NoScope)
            {
                fPassedText = TRUE;
            }           
            break;
            
        }

        //
        // If we are at or beyond the destination point where MoveUnit() took us, 
        // set the passed in pointer to the destination and we're outta here
        //
        if ( muAction == MOVEUNIT_PREVWORDBEGIN )
        {            
            if ( pointerSource.IsLeftOfOrEqualTo( & pointerDestination ) )
            {                
                hr = THR( 
                        pPointerToMove->MoveToPointer( & pointerDestination ) );
                goto Cleanup;
            }
        }
        else
        {
            if ( pointerSource.IsRightOfOrEqualTo( & pointerDestination ) )
            {                
                hr = THR( 
                        pPointerToMove->MoveToPointer( & pointerDestination ) );
                goto Cleanup;
            }
        }

        //
        // Detect Block break, Text site begin or text site end
        //
        hr = THR( pointerSource.QueryBreaks( & dwBreaks ) );
        if (hr)
            goto Cleanup;

        // We hit a break before reaching our destination, time to stop...
        if ( dwBreaks != BREAK_NONE )
        {
            if ( fPassedText )
            {
                // We're done
                goto Done;
            }
            else
            {
                fPassedText = TRUE;
            }
        }
    }

Done:
    hr = THR( 
            pPointerToMove->MoveToPointer( & pointerSource ) );
    if (hr)
        goto Cleanup;
    
Cleanup:
    ReleaseInterface( pIElement );
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     MoveUnitWithinRange
//
//  Synopsis:   Helper function for moveRange, watches MoveUnit() to ensure 
//              that range pointers don't go outside the range boundaries
//
//-----------------------------------------------------------------------------

HRESULT 
CAutoRange::MoveUnitWithinRange( IMarkupPointer * pPointerToMove, 
                                 MOVEUNIT_ACTION  muAction,
                                 long * pnActualCount )
{
    HRESULT hr = S_OK;
    long    nRequestedCount;
    BOOL    fLeftBound;
    long    i;
    IMarkupPointer * pPointer = NULL;
    IMarkupPointer * pLeftBoundary = NULL;
    IMarkupPointer * pRightBoundary = NULL;
    BOOL             fEqual;
    
    fLeftBound = ( (*pnActualCount) < 0 );
    nRequestedCount = abs(*pnActualCount);
    *pnActualCount = 0;

    hr = THR( 
            GetMarkup()->Doc()->CreateMarkupPointer( & pPointer ) );
    if (hr) 
        goto Cleanup;

    hr = THR( 
            pPointer->MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR( 
            MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
    if (hr)
        goto Cleanup;

    //
    // Bail if we are at range boundaries and we're going the wrong way
    //
    if (fLeftBound)
    {
        IGNORE_HR( 
                pPointer->IsEqualTo( pLeftBoundary, & fEqual ) );

        if ( fEqual )
        {
            goto Cleanup;
        }
    }
    else
    {
        IGNORE_HR( 
                pPointer->IsEqualTo( pRightBoundary, & fEqual ) );

        if ( fEqual )
        {
            goto Cleanup;
        }
    }
  
    //
    // Do the move...
    //

    for ( i=1; i <= nRequestedCount; i++ )
    {      
        if ( muAction == MOVEUNIT_NEXTCHAR ||
             muAction == MOVEUNIT_PREVCHAR )
        {
            MARKUP_CONTEXT_TYPE context;
            
            // Look left for text.  If we find text, skip over it with
            // Left/Right instead of MoveCharacter.
            //
            if (fLeftBound)
                hr = THR(pPointer->Left(FALSE, &context, NULL, NULL, NULL));
            else
                hr = THR(pPointer->Right(FALSE, &context, NULL, NULL, NULL));
            if (FAILED(hr))
                goto Cleanup;
                
            if (context == CONTEXT_TYPE_Text)
            {
                LONG cCharactersLeft = nRequestedCount - i + 1;

                if (fLeftBound)
                    hr = THR(pPointer->Left(TRUE, NULL, NULL, &cCharactersLeft, NULL));
                else
                    hr = THR(pPointer->Right(TRUE, NULL, NULL, &cCharactersLeft, NULL));
                if (FAILED(hr))
                    goto Cleanup;

                if (cCharactersLeft > 0)
                {
                    i += (cCharactersLeft - 1);                
                }
                else
                {
                    hr = THR( MoveCharacter( pPointer, muAction, pLeftBoundary, pRightBoundary ) );
                }
            }
            else
            {           
                hr = THR( MoveCharacter( pPointer, muAction, pLeftBoundary, pRightBoundary ) );
            }
        }
        else if ( muAction == MOVEUNIT_NEXTWORDBEGIN ||
                  muAction == MOVEUNIT_PREVWORDBEGIN )
        {
            hr = THR( 
                    MoveWord( pPointer, muAction, pLeftBoundary, pRightBoundary ) );
        }
        else
        {
            hr = THR( pPointer->MoveUnit( muAction ) );
        }
        
        if (hr)
        {
            if (hr == S_FALSE)
            {
                hr = S_OK;
            }
            break;
        }

        //
        // Update the count of movements made and 
        // check to see if we are within range boundaries
        //
        if (fLeftBound)
        {            
            (*pnActualCount) = -1*i;

            IGNORE_HR( pPointer->IsLeftOfOrEqualTo( pLeftBoundary, & fEqual ) );
            
            if ( fEqual )
            {
                hr = THR( 
                       pPointerToMove->MoveToPointer( pLeftBoundary ) );
                if (hr)
                    goto Cleanup;

                // We're Done
                break;
            }            
        }
        else
        {    
            (*pnActualCount) = i;

            IGNORE_HR( pPointer->IsRightOfOrEqualTo( pRightBoundary, & fEqual ) );

            if ( fEqual )
            {
                hr = THR( 
                       pPointerToMove->MoveToPointer( pRightBoundary ) );
                if (hr)
                    goto Cleanup;

                // We're Done
                break;
            }            
        }

        hr = THR( 
               pPointerToMove->MoveToPointer( pPointer ) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pPointer );
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     moveRange
//
//  Synopsis:   private method that implements move(), moveStart() and moveEnd()
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::moveRange ( BSTR bstrUnit, long Count, long * pActualCount, int moveWhat )
{
    HRESULT hr;
    MOVEUNIT_ACTION     muAction;
    htmlUnit            Unit;
    IMarkupPointer    * pLeftBoundary  = NULL;
    IMarkupPointer    * pRightBoundary = NULL;
    BOOL                fEqual;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!pActualCount)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // Initialize actual count value to zero
    //
    *pActualCount = 0;

    if (! _pElemContainer)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Make sure the range is positioned correctly
    //
    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    //
    // Determine what action must take place based on the bstrUnit
    //
    hr = THR( GetMoveUnitAndType( bstrUnit, Count, & muAction, & Unit ) );
    if (hr)
        goto Cleanup;

    if ( Unit != htmlUnitTextEdit )
    {
        // Use MoveUnit() to move by Character, Word, or Sentence

        Assert( Unit == htmlUnit_Max );

        *pActualCount = Count;

        if (moveWhat == MOVERANGE_Right)
        {
            hr = THR( 
                    MoveUnitWithinRange( _pRight, muAction, pActualCount ) );
        }
        else
        {
            hr = THR( 
                    MoveUnitWithinRange( _pLeft, muAction, pActualCount ) );
        }
                                 
    }
    else
    {
        //
        // Move range pointers to right/left borders to move by textedit units
        //

        if (Count == 0 || Count < -1 || Count > 1)
        {
            // Can only move textedit by -1 or 1 units
            hr = S_OK;
            goto Cleanup;
        }

        //
        // Position the left/right boundry pointers around _pElemContainer
        //
        hr = THR(
                MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
        if (hr)
            goto Cleanup;

        //
        // Move the pointers according to the values passed
        // Note that if the pointer is already where
        // it is asked to go, we don't do anything. ActualCount of
        // zero is returned in such cases to be compatible with IE4
        //
        if (Count == 1)
        {
            if (moveWhat == MOVERANGE_Right)
            {
                hr = THR( _pRight->IsEqualTo( pRightBoundary, &fEqual ) );
                if (hr || fEqual)
                    goto Cleanup;

                hr = THR( _pRight->MoveToPointer( pRightBoundary ) );
                if (hr)
                    goto Cleanup;
            }
            else
            {
                hr = THR( _pLeft->IsEqualTo( pRightBoundary, &fEqual ) );
                if (hr || fEqual)
                    goto Cleanup;

                hr = THR( _pLeft->MoveToPointer( pRightBoundary ) );
                if (hr)
                    goto Cleanup;
            }
        }
        else
        {
            Assert( Count == -1 );

            if (moveWhat == MOVERANGE_Right)
            {
                hr = THR( _pRight->IsEqualTo( pLeftBoundary, &fEqual ) );
                if (hr || fEqual)
                    goto Cleanup;

                hr = THR( _pRight->MoveToPointer( pLeftBoundary ) );
                if (hr)
                    goto Cleanup;
            }
            else
            {
                hr = THR( _pLeft->IsEqualTo( pLeftBoundary, &fEqual ) );
                if (hr || fEqual)
                    goto Cleanup;

                hr = THR( _pLeft->MoveToPointer( pLeftBoundary ) );
                if (hr)
                    goto Cleanup;
            }            
        }
        //
        // Oh, yes don't forget to set pActualCount for textedit move
        //
        *pActualCount = Count;
    }
    
    //
    // Here we collapse the range according to IE4 behavior:
    // move() must always collapse at the Start (left)
    // moveStart and moveEnd collapse the range at start or end
    // respectively, only if start is after the end
    // 
    if (moveWhat == MOVERANGE_Both)
    {
        // Collapse the range for move()
        hr = THR( _pRight->MoveToPointer( _pLeft ) );
        if (hr)
            goto Cleanup;
    }
    else
    {
        int result;
        
        IGNORE_HR( _pLeft->IsRightOf( _pRight, & result ) );
        
        if (result)
        {
            if (moveWhat == MOVERANGE_Left)
            {
                // Collpase the range at the start for MOVERANGE_Left
                hr = THR( _pRight->MoveToPointer( _pLeft ) );
                if (hr)
                    goto Cleanup;
            }
            else 
            {
                // Collapse the range at the end for MOVERANGE_Right case
                hr = THR( _pLeft->MoveToPointer( _pRight ) );
                if (hr)
                    goto Cleanup;
            }
        }
    }

    hr = THR( ValidatePointers() );

    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    RRETURN( SetErrorInfo( hr ) );
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
CAutoRange::queryCommandSupported(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
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
CAutoRange::queryCommandEnabled(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
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
CAutoRange::queryCommandState(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
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
CAutoRange::queryCommandIndeterm(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
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
CAutoRange::queryCommandText(BSTR bstrCmdId, BSTR *pcmdText)
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
CAutoRange::queryCommandValue(BSTR bstrCmdId, VARIANT *pvarRet)
{
    RRETURN(CBase::queryCommandValue(bstrCmdId, pvarRet));
}


//+----------------------------------------------------------------------------
//
//  Member:     execCmd, IHTMLTxtRange
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::execCommand(BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT varValue, VARIANT_BOOL * pfRet )
{
    HRESULT hr = S_OK;
    BOOL fAllow;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Check for security violations

    hr = THR(GetMarkup()->Doc()->AllowClipboardAccess(bstrCmdId, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;           // Fail silently

    if (!CheckSecurity((LPCWSTR) bstrCmdId))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(CBase::execCommand(bstrCmdId, showUI, varValue));
    if(hr)
        goto Cleanup;

    hr = THR( ValidatePointers() );

    if (hr)
        goto Cleanup;

Cleanup:

    if(pfRet != NULL)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     CheckSecurity
//
//  Synopsis:   Check for things like paste into File Input or Copy from Password
//
//-----------------------------------------------------------------------------
BOOL
CAutoRange::CheckSecurity(LPCWSTR pszCmdId)
{
    BOOL                fPaste = FALSE;
    BOOL                fCopy = FALSE;
    BOOL                fOk = TRUE;
    HRESULT             hr = S_OK;
    CMarkupPointer    * pointerLeft;
    CMarkupPointer    * pointerRight;
    CElement          * pElement;
    htmlInput           type;

    if (StrCmpICW(pszCmdId, L"paste") == 0)
        fPaste = TRUE;
    else if (StrCmpICW(pszCmdId, L"copy") == 0)
        fCopy = TRUE;
    else
        goto Cleanup;       // Nothing to do here

    // Get the element to check for being in an Input element
    hr = THR( _pLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pointerLeft ) );
    if( hr )
        goto Cleanup;

    hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pointerRight ) );
    if( hr )
        goto Cleanup;

    if ( pointerLeft->Markup() == pointerRight->Markup() )
    {
        // If both pointers are in the same markup, return the master
        pElement = pointerLeft->Markup()->Master();
        if ( !pElement || (pElement->Tag() != ETAG_INPUT))
            goto Cleanup;

        type = DYNCAST(CInput, pElement)->GetType();

        if (type == htmlInputFile)
            fOk = FALSE;
    }

Cleanup:
    if (S_OK != hr)
        fOk = FALSE;
    return fOk;
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
CAutoRange::execCommandShowHelp(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT   hr;

    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(CBase::execCommandShowHelp(bstrCmdId));

Cleanup:

    if(pfRet != NULL)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

    RRETURN(SetErrorInfo(hr));
}




//+----------------------------------------------------------------------------
//
//  Member:     select, IAutoRange
//
//  Synopsis:   take the given range (this), goto the markups selection and set it
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::select ( )
{
    HRESULT     hr = S_OK;
    CMarkup *   pMarkup  = GetMarkup();
    CDoc *      pDoc = NULL;     
    SELECTION_TYPE eSelType  = SELECTION_TYPE_Selection;
    BOOL fEqual = FALSE;
    
    if( ! pMarkup )
    {
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
    
    pDoc = GetMarkup()->Doc();
    Assert( pDoc );
    
    IGNORE_HR( scrollIntoView ( TRUE ) );

    //
    // IE 4.01 Compat if the 
    //
    hr = THR( _pLeft->IsEqualTo(_pRight, &fEqual) );
    if ( hr )
    {
        AssertSz(0,"Compare on pointers failed - are they in the same tree ?");
        goto Cleanup;
    }
    
    if( fEqual )
    { 
        eSelType  = SELECTION_TYPE_Caret;
    }
    hr = THR( pDoc->Select( _pLeft, _pRight , eSelType ));

Cleanup:
    RRETURN ( SetErrorInfo(hr) );   
}

//+====================================================================================
//
// Method: IsRangeEquivalentToSelection
//
// Synopsis: Is the given range "equivalent" to the selection
//
//------------------------------------------------------------------------------------


HRESULT
CAutoRange::IsRangeEquivalentToSelection ( BOOL *pfEquivalent)
{
    HRESULT hr = S_OK;
    BOOL fEquivalent = FALSE;
    IMarkupPointer* pSelStart = NULL;
    IMarkupPointer* pSelEnd = NULL;
    ISegmentList * pSegmentList = NULL;
    CDoc* pDoc = GetMarkup()->Doc();

    if ( pDoc->GetSelectionType() == SELECTION_TYPE_Selection ||
         pDoc->GetSelectionType() == SELECTION_TYPE_Control )
    {    
        hr = THR( pDoc->CreateMarkupPointer( & pSelStart ));
        if ( hr )
            goto Cleanup;

        hr = THR( pDoc->CreateMarkupPointer( & pSelEnd ));
        if ( hr )
            goto Cleanup;
            
        hr = THR( pDoc->GetCurrentSelectionSegmentList( & pSegmentList ));    
        if ( hr )
            goto Cleanup;
            
        hr = THR( pSegmentList->MovePointersToSegment( 0, pSelStart, pSelEnd ));
        if ( hr )
            goto Cleanup;

        //
        // Now make the pointers cling to text like the range dows.
        //
        hr = AdjustPointers( pSelStart, pSelEnd );
        if ( hr )
            goto Cleanup;

        hr = THR ( pSelStart->IsEqualTo( _pLeft, &fEquivalent ));
        if ( hr )
            goto Cleanup;

        if ( fEquivalent )
        {
            hr = THR ( pSelEnd->IsEqualTo( _pRight, & fEquivalent ));
            if ( hr )
                goto Cleanup;  
        }
    }
Cleanup:
    if ( pfEquivalent )
        *pfEquivalent = fEquivalent;

    ReleaseInterface( pSelStart );
    ReleaseInterface( pSelEnd );
    ReleaseInterface( pSegmentList );
    
    RRETURN( hr );        
}
//+----------------------------------------------------------------------------
//
//  Member:     pasteHTML, IAutoRange
//
//  Synopsis:   Used to be put_htmltext
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoRange::pasteHTML ( BSTR htmlText )
{
    HRESULT     hr = S_OK;
    CParentUndo pu( GetMarkup()->Doc() );
    CElement *  pContainer;
    BOOL        result;
    IHTMLElement* pIFlowElement1 = NULL;
    IHTMLElement* pIFlowElement2 = NULL;
    IObjectIdentity * pIdentity = NULL;
    CMarkup *   pMarkup = GetMarkup();   
    CDoc     *  pDoc = pMarkup->Doc();
    BOOL fInSameFlow = TRUE;
    BOOL fRangeEquivalentToSelection = FALSE;
    
    DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    pContainer = GetCommonContainer();

    if (pContainer && pContainer->TestClassFlag( CElement::ELEMENTDESC_OMREADONLY ))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!FSupportsHTML())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // CHeck the pointers are both in the same flow layout. If they're not - fail.
    //
    hr = THR( pDoc->GetFlowElement( _pLeft, & pIFlowElement1));
    if ( hr )
        goto Cleanup;

    hr = THR( pDoc->GetFlowElement( _pRight, & pIFlowElement2));
    if ( hr )
        goto Cleanup;

    hr = THR( pIFlowElement1->QueryInterface( IID_IObjectIdentity, (void **) &pIdentity ) );
    if (hr)
        goto Cleanup;

    fInSameFlow = ( pIdentity->IsEqualObject( pIFlowElement2 ) == S_OK );   
    if ( ! fInSameFlow )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    //
    // See if the range and the selection are "equivalent" - ie the pointers are in the same places
    // if they are - we have to mimic the ie 4 behavior of collapsing to a caret at the end of paste.
    //

    hr = THR( IsRangeEquivalentToSelection( & fRangeEquivalentToSelection));
    if ( hr)
        goto Cleanup;
        
    if( pContainer->IsEditable() )
        pu.Start( IDS_UNDOPASTE );

    //
    //
    //
    
    {
        CMarkupPointer * pmpLeft, * pmpRight;

        extern HRESULT HandleUIPasteHTML (
            CMarkupPointer *, CMarkupPointer *, const TCHAR *, long, BOOL );

        hr = THR( _pLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpLeft ) );

        if (hr)
            goto Cleanup;

        hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpRight ) );

        if (hr)
            goto Cleanup;

        hr = THR( HandleUIPasteHTML( pmpLeft, pmpRight, htmlText, -1, TRUE ) );

        if (hr == S_FALSE)
        {
            hr = S_OK; // BUGBUG: return something interesting
            goto Cleanup;
        }

        if (hr)
            goto Cleanup;
    }
    
    //
    // Collapse the range after the paste
    // Note: Since _pLeft has right gravity and _pRight has left gravity
    //       after paste, _pLeft ends up to the right of _pRight.
    //       That's why we're moving _pRight to _pLeft to collapse the range.
    //
    
    hr = THR( _pLeft->IsRightOf( _pRight, & result ) );

    if ( result )
        hr = THR( _pRight->MoveToPointer( _pLeft ) );
    else
        hr = THR( _pLeft->MoveToPointer( _pRight ) );

    hr = THR( ValidatePointers() );

    if (hr)
        goto Cleanup;

    //
    // Place the caret where we just pasted.
    //
    if ( fRangeEquivalentToSelection )
    {
        hr = THR( GetMarkup()->Doc()->Select( _pRight,_pRight, SELECTION_TYPE_Caret ));
    }        

Cleanup:
    ReleaseInterface( pIFlowElement1 );
    ReleaseInterface( pIFlowElement2 );
    ReleaseInterface( pIdentity );
    
    pu.Finish( hr );

    RRETURN( SetErrorInfo( hr ) );
}


//+-------------------------------------------------------------------
//
//  function : GetRangeTopLeft()
//
//  Synopsis : just a local helper function for getting the offsetTop
//      and the offsetLeft.
//
//-------------------------------------------------------------------
HRESULT
CAutoRange::GetRangeTopLeft(POINT * pPt, BOOL fScreenCoord)
{
    HRESULT       hr = S_OK;
    CDataAry<RECT>  aryRects(Mt(CAutoRangeGetRangeBoundingRect_aryRects_pv));
    CDataAry<RECT> * paryRects = &aryRects;
    RECT *  prc;
    LONG    iRect;
    LONG    lSize;

    if (!pPt)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pPt->x = pPt->y = 0;

    // Get the bounding rectangles
    hr = THR(GetRangeBoundingRects(&aryRects, fScreenCoord));
    if(hr)
        goto Cleanup;

    lSize = paryRects->Size();

    // skip any zero rects in our search for the rect of the first line
    for(  prc = *paryRects, iRect =0 ;
          (   prc 
           && prc->left == 0 
           && prc->right == 0 
           && prc->top == 0 
           && prc->bottom == 0
           && iRect < lSize ); 
           prc++, iRect++ )
           
               ;

    // Set the Point to the left and top of the first non-zero rect
    if (prc)
    {
        pPt->y = prc->top;
        pPt->x = prc->left;
    }

 Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     offsetTop,  IHTMLTextRangeMetrics
//
//  Synopsis:  returns the offsetTop of the start cp of the range
//
//----------------------------------------------------------------------------

HRESULT
CAutoRange::get_offsetTop( long *pLong )
{
    HRESULT    hr;
    POINT      pt;

    DO_SANITY_CHECK

    if (!pLong)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (IsOrphaned())
    {
        *pLong = -1;
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(GetRangeTopLeft( &pt, TRUE)); 

    *pLong = (hr) ? -1 : pt.y;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     offsetLeft,  IHTMLTextRangeMetrics
//
//  Synopsis:  returns the offsetleft of the start cp of the range
//
//----------------------------------------------------------------------------

HRESULT
CAutoRange::get_offsetLeft( long *pLong )
{
    HRESULT    hr;
    POINT      pt;

    DO_SANITY_CHECK

    if (!pLong)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (IsOrphaned())
    {
        *pLong = -1;
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(GetRangeTopLeft( &pt, TRUE)); 

    *pLong = (hr) ? -1 : pt.x;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}



//+-----------------------------------------------------------------
//
//  member : GetRangeBoundingRects()
//
//  synopsis : private helper function used by the boundingBox queries.
//               returns the array that has a rectangle for each line
//               in the range
//
//------------------------------------------------------------------

HRESULT
CAutoRange::GetRangeBoundingRects(CDataAry<RECT> * pRects, BOOL fScreenCoord)
{
    HRESULT     hr = S_FALSE;
    CTreeNode * pNode = NULL;
    CTreeNode * pNodeLeft;
    CFlowLayout * pFlowLayout = NULL;

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    Assert(pRects);

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    if (! IsRangeCollapsed() )
    {
        pNodeLeft = LeftNode();

        if (! pNodeLeft)
            goto Cleanup;

        pNode = pNodeLeft->GetFirstCommonAncestor( RightNode(), NULL);
    }
    else
    {
        pNode = LeftNode();
    }

    if(!pNode)
        goto Cleanup;

    pFlowLayout = GetMarkup()->Doc()->GetFlowLayoutForSelection( pNode );

    if(pFlowLayout)
    {
        CNotification    nf;
        CMarkupPointer * pLeft;
        CMarkupPointer * pRight = NULL;
        int     cpClipStart; 
        int     cpClipFinish;

        hr = THR_NOTRACE( _pLeft->QueryInterface(CLSID_CMarkupPointer, (void **) & pLeft) );
        if (! hr)
            hr = THR_NOTRACE( _pRight->QueryInterface(CLSID_CMarkupPointer, (void **) & pRight) );
        if (hr)
            goto Cleanup;

        cpClipStart = pLeft->GetCp();
        cpClipFinish = pRight->GetCp();

        Assert( cpClipStart <= cpClipFinish);

        nf.RangeEnsurerecalc(cpClipStart, cpClipFinish - cpClipStart, pNode);
        pLeft->Markup()->Notify(&nf);
    
        // We cannot pass the bounding rect to the function because in that case
        // it will ignore the cpMin and cpMost
        pFlowLayout->GetDisplay()->RegionFromElement(
                            pNode->Element(),
                            pRects,
                            NULL,
                            NULL,  
                            fScreenCoord
                                ? RFE_SCREENCOORD | RFE_SELECTION
                                : RFE_SELECTION,
                            cpClipStart, 
                            cpClipFinish); 
    }
                        
Cleanup:
    if (hr == S_FALSE)
        hr = S_OK;
    RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  member : GetRangeBoundingRect()
//
//  synopsis : private helper function used by the boundingBox queries.
//
//------------------------------------------------------------------


HRESULT
CAutoRange::GetRangeBoundingRect(RECT * prcBound, BOOL fScreenCoord)
{
    HRESULT         hr;
    CDataAry<RECT>  aryRects(Mt(CAutoRangeGetRangeBoundingRect_aryRects_pv));

    Assert(prcBound);

    prcBound->left = prcBound->right = prcBound->top = prcBound->bottom = 0;

    hr = THR(GetRangeBoundingRects(&aryRects, fScreenCoord));
    if(hr)
        goto Cleanup;

    // Calculate and return the total bounding rect
    BoundingRectForAnArrayOfRectsWithEmptyOnes(prcBound, &aryRects);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  member : CAutoRange::getBoundingClientRect() - External method
//
//  Synopsis:   Returns Bounding rect of the range text in client
//                coordinates
//------------------------------------------------------------------

HRESULT
CAutoRange::getBoundingClientRect(IHTMLRect **ppIRect)
{
    HRESULT       hr = S_OK;
    RECT          Rect;
    COMRect     * pOMRect = NULL;

    DO_SANITY_CHECK

    if (!ppIRect)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppIRect = NULL;

    // Get the infromation
    hr = THR(GetRangeBoundingRect(&Rect));
    if(hr)
        goto Cleanup;

    // Create an instance of the rectangle object
    pOMRect = new COMRect(&Rect);
    if (!pOMRect)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

        *ppIRect = (IHTMLRect *)pOMRect;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-----------------------------------------------------------------
//
//  member : CAutoRange::getClientRects() - External method
//
//  Synopsis:   Returns the collection of rectangles for the text under
//               element's influence in client coordinates.
//              Each rectangle represents a line of text on the screen.
//------------------------------------------------------------------

HRESULT
CAutoRange::getClientRects(IHTMLRectCollection **ppIRects)
{
    HRESULT              hr;
    COMRectCollection  * pOMRectCollection;
    CDataAry<RECT>       aryRects(Mt(CAutoRangegetClientRects_aryRects_pv));

    DO_SANITY_CHECK

    if (!ppIRects)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppIRects = NULL;

    // Get the array taht contains bounding rects for each line in the range
    hr = THR(GetRangeBoundingRects(&aryRects));
    if(hr)
        goto Cleanup;

    // Create a rectangle collection class instance
    pOMRectCollection = new COMRectCollection();
    if (!pOMRectCollection)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Fill the collection with values from aryRects
    hr = THR(pOMRectCollection->SetRects(&aryRects));
    if(hr)
        goto Cleanup;

        *ppIRects = (IHTMLRectCollection *) pOMRectCollection;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:    get_bounding[Width|Height|Top|Left],  IHTMLTextRangeMetrics
//
//  Synopsis:  returns the rect values for the bounding box around the range
//
//----------------------------------------------------------------------------

HRESULT
CAutoRange::get_boundingWidth( long *pLong )
{
    HRESULT    hr;
    RECT       rect;

    DO_SANITY_CHECK

    if (!pLong)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetRangeBoundingRect(&rect));

    *pLong = FAILED(hr) ? -1 : rect.right-rect.left;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


HRESULT
CAutoRange::get_boundingHeight( long *pLong )
{
    HRESULT    hr;
    RECT       rect;

    DO_SANITY_CHECK

    if (!pLong)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetRangeBoundingRect(&rect));

    *pLong = FAILED(hr) ? -1 : rect.bottom-rect.top;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}



HRESULT
CAutoRange::get_boundingTop( long *pLong )
{
    HRESULT    hr;
    RECT       rect;
    LONG       lScroll;
    CElement * pContainer = NULL;

    DO_SANITY_CHECK

    if (!pLong)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetRangeBoundingRect(&rect));

    *pLong = (hr) ? -1 : rect.top;

    // for ie4.01sp1/2 compatability we need to subtract
    // the scrollTop in order to make this client coords
    // rather than document coords.
    pContainer = GetCommonContainer();

    if (pContainer && 
        (SUCCEEDED(pContainer->get_scrollTop( &lScroll ))))
       *pLong -= lScroll;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


HRESULT
CAutoRange::get_boundingLeft( long *pLong )
{
    HRESULT    hr;
    RECT       rect;
    LONG       lScroll;
    CElement * pContainer = NULL;

    DO_SANITY_CHECK

    if (!pLong)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetRangeBoundingRect(&rect));

    *pLong = (hr) ? -1 : rect.left;

    // for ie4.01sp1/2 compatability we need to subtract
    // the scrollLeft in order to make this clientwindow coords
    // rather than document client coords.
    pContainer = GetCommonContainer();

    if (pContainer && 
        (SUCCEEDED(pContainer->get_scrollLeft( &lScroll ))))
       *pLong -= lScroll;

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

HRESULT 
CAutoRange::MovePointersToSegment ( 
    int iSegmentIndex, 
    IMarkupPointer * pILeft, 
    IMarkupPointer * pIRight ) 
{
    HRESULT     hr;
    POINTER_GRAVITY eGravity = POINTER_GRAVITY_Left;
    
    if ( iSegmentIndex != 0 )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // check argument sanity
    if (! (pILeft && pIRight) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pILeft->MoveToPointer( _pLeft ) );
    
    if (!hr)
        hr = THR( pIRight->MoveToPointer( _pRight ) );
    

    //
    // copy gravity - important for commands
    //
    if ( !hr ) hr = _pLeft->Gravity( &eGravity );            // need to maintain gravity
    if ( !hr ) hr = pILeft->SetGravity( eGravity );

    if ( !hr ) hr = _pRight->Gravity( &eGravity );            // need to maintain gravity
    if ( !hr ) hr = pIRight->SetGravity( eGravity );
    
    if (hr) 
        goto Cleanup;
    
Cleanup:        
    RRETURN( hr );
}    

HRESULT
CAutoRange::MoveSegmentToPointers ( int iSegmentIndex,
                                    IMarkupPointer * pILeft, 
                                    IMarkupPointer * pIRight )  
{
    HRESULT     hr;
    BOOL        fPositioned = FALSE;
    
    // check argument sanity
    if (! (pILeft && pIRight) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pILeft->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    hr = THR( pIRight->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
   
    hr = THR( SetLeft( pILeft ) );
    if (! hr)   hr = THR( SetRight( pIRight ) );
    if (hr)
        goto Cleanup;
    
Cleanup:    
    RRETURN( hr );
}


HRESULT
CAutoRange::GetSegmentCount(
    int* piSegmentCount,
    SELECTION_TYPE *peSelectionType )
{
    HRESULT hr = S_FALSE;

    if ( piSegmentCount )
    {
        *piSegmentCount = 1;
        hr = S_OK;
    }
    if ( peSelectionType )
        *peSelectionType = SELECTION_TYPE_Auto;
        
    RRETURN( hr );
}


#if DBG==1
void CAutoRange::DumpTree()
{
    GetMarkup()->DumpTree();
}
#endif


BOOL
IsIntrinsicTag( ELEMENT_TAG eTag )
{
    switch (eTag)
    {
    case ETAG_BUTTON:
    case ETAG_TEXTAREA:
#ifdef  NEVER
    case ETAG_HTMLAREA:
#endif
    case ETAG_FIELDSET:
    case ETAG_LEGEND:
    case ETAG_MARQUEE:
    case ETAG_SELECT:
        return TRUE;

    default:
        return FALSE;
    }
}


HRESULT
CAutoRange::AdjustIntoTextSite(
    IMarkupPointer *    pPointerToMove,
    MV_DIR              Dir,
    IMarkupPointer *    pBoundary )
{
    HRESULT         hr;
    IHTMLElement *  pHTMLElement = NULL;    
    IHTMLElement *  pSite = NULL;
    IMarkupPointer* pPointer;
    CDoc *          pDoc;
    BOOL            fTextSite = FALSE;
    BOOL            fResult;
    MARKUP_CONTEXT_TYPE context;

    pDoc = GetMarkup()->Doc();
    Assert( pDoc );

    hr = THR( pDoc->CreateMarkupPointer( & pPointer ) );        
    if (hr)
        goto Cleanup;

    hr = THR( pPointer->MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;
    
    for ( ; ; )
    {
        ClearInterface( & pHTMLElement );

        if( Dir == MV_DIR_Left )
        {
            hr = THR( pPointer->Left( TRUE, & context, & pHTMLElement, NULL, NULL ) );
            if (hr)
                goto Cleanup;

            hr = THR( pPointer->IsLeftOf( pBoundary, & fResult ) );
            if (hr)
                goto Cleanup;

        }
        else
        {
            hr = THR( pPointer->Right( TRUE, & context, & pHTMLElement, NULL, NULL ) );
            if (hr)
                goto Cleanup;

            hr = THR( pPointer->IsRightOf( pBoundary, & fResult ));
            if (hr)
                goto Cleanup;
        }

        // Check Boundary
        if (fResult)
        {
            goto Cleanup;
        }

        switch( context )
        {
        case CONTEXT_TYPE_None:
            goto Cleanup;

        case CONTEXT_TYPE_EnterScope:
        case CONTEXT_TYPE_ExitScope:
            if (! pHTMLElement)
                break;

            ClearInterface( & pSite );
            hr = THR( GetSiteContainer( pHTMLElement, & pSite,  & fTextSite ) );
            if (hr)
                goto Cleanup;

            if ( fTextSite )
            {                
                goto Cleanup;
            }
        }
    }

Cleanup:
    if( SUCCEEDED( hr ) && fTextSite )
    {
        hr = THR( pPointerToMove->MoveToPointer( pPointer ));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = E_FAIL;
    }
    ReleaseInterface( pPointer );
    ReleaseInterface( pHTMLElement );
    ReleaseInterface( pSite );
    return( hr );
}    


//+---------------------------------------------------------------------------
//
//  Member:     CAutoRange::ClingToText()
//
//  Synopsis:   Move pOrigin towards pTarget while the context type is not text. 
//              The direction of the move is denoted by fLeftBound.
//              If the move causes pOrigin to pass pTarget, the pointers are
//              adjusted to move next to each other.
//
//----------------------------------------------------------------------------

CLING_RESULT
CAutoRange::ClingToText( 
    IMarkupPointer *        pInPointer, 
    IMarkupPointer *        pBoundary, 
    MV_DIR                  eDir )
{
    CLING_RESULT        cr = CR_Failed;
    HRESULT             hr = S_OK;
    BOOL                fDone = FALSE;
    BOOL                fLeft = eDir == MV_DIR_Left;
    CElement *          pElement = NULL;
    MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;

    IMarkupPointer *    pPointer = NULL;
    IHTMLElement *      pHTMLElement = NULL;
    CMarkupPointer  *   pointer = NULL;
    DWORD               dwBreaks;
    long                cch;

#if 0
    //
    // Quick Out: Check if I'm adjacent to text. If so, go nowhere.
    //

    if( fLeft )
        hr = THR( pInPointer->Right( FALSE, & eCtxt, NULL, NULL, NULL ) );
    else
        hr = THR( pInPointer->Left( FALSE, & eCtxt, NULL , NULL, NULL ) );
    
    if( eCtxt == CONTEXT_TYPE_Text )
    {
        cr = CR_Text;
        goto Cleanup;   // Nothing to do!
    }
#endif // if 0

    hr = THR( GetMarkup()->Doc()->CreateMarkupPointer( & pPointer ) );
    if (hr)
        goto Cleanup;

    hr = THR( pPointer->MoveToPointer( pInPointer ) );
    if (hr)
        goto Cleanup;

    hr = THR(
            pPointer->QueryInterface(CLSID_CMarkupPointer, (void**) & pointer ) );
    if (hr)
        goto Cleanup;

    while( ! fDone )
    {
        BOOL fResult;
        
        ClearInterface( & pHTMLElement );
        
        if ( fLeft ) 
        {
            hr = pPointer->IsLeftOf( pBoundary, & fResult ) ;
        }
        else
        {
            hr = pPointer->IsRightOf( pBoundary, & fResult ) ;
        }

        if ( fResult )
        {
            cr = CR_Boundary;
            goto Done;
        }
            
        cch = 1;
        if( fLeft )
        {
            hr = THR( pPointer->Left( TRUE, & eCtxt, & pHTMLElement, & cch, NULL ));
        }
        else
        {
            hr = THR( pPointer->Right( TRUE, & eCtxt, & pHTMLElement, & cch, NULL ));
        }

        if( FAILED( hr ))
            goto Done;

        switch( eCtxt )
        {
            case CONTEXT_TYPE_EnterScope:
                if( pHTMLElement )
                {
                    pElement = NULL;
                    IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                    if ( IsIntrinsicTag( pElement->Tag() ) ||
                         pElement->HasLayout() )
                    {
                        cr = CR_Intrinsic;
                        fDone = TRUE;
                    }
                    else if ( pElement->IsBlockElement() )
                    {
                        //
                        // Check for any break characters
                        //
                        hr = THR( pointer->QueryBreaks( & dwBreaks ) );
                        if (hr)
                            goto Cleanup;

                        if ( dwBreaks == BREAK_BLOCK_BREAK )
                        {
                            // Stay after the block break, we're done
                            cr = CR_BlockBreak;
                            fDone = TRUE;
                        }
                    }
                }
                break;

            case CONTEXT_TYPE_ExitScope:
                if( pHTMLElement )
                {
                    pElement = NULL;
                    IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                    if( pElement &&
                          (pElement->HasLayout() || pElement->IsBlockElement() || pElement->Tag()==ETAG_BR))
                    {
                        cr = CR_Failed;
                        // cr = CR_Text; // this could also be CR_Failed, but this simulates a block break or tsb char
                        fDone = TRUE;
                    }
                }
                break;
                
            case CONTEXT_TYPE_NoScope:
                if( pHTMLElement )
                {
                    pElement = NULL;
                    IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                    if( pElement 
                        && (pElement->HasLayout() || pElement->IsBlockElement() || pElement->Tag()==ETAG_BR ||
                           pElement->Tag()==ETAG_SCRIPT ))
                    {
                        cr = CR_NoScope;
                        fDone = TRUE;
                    }
                }
                break;
                
            case CONTEXT_TYPE_Text:
                cr = CR_Text;
                fDone = TRUE;
                break;
                
            case CONTEXT_TYPE_None:
                cr = CR_Failed;
                fDone = TRUE;
                break;
        }
    }

Done:

    //
    // If we found text, move our pointer
    //
    
    switch( cr )
    {
        case CR_Text:
        case CR_NoScope:
        case CR_Intrinsic:
        case CR_BlockBreak:

            //
            // We have inevitably gone one move too far, back up one move
            //

            cch = 1;
            if( fLeft )
            {
                hr = THR( pPointer->Right( TRUE, & eCtxt, NULL, & cch, NULL ));
            }
            else
            {
                hr = THR( pPointer->Left( TRUE, & eCtxt, NULL, & cch, NULL ));
            }

            if( FAILED( hr ))
                goto Cleanup;

            //
            // Now we position the pointer
            //
            
            hr = THR( pInPointer->MoveToPointer( pPointer ));
            break;
    }

Cleanup:
    ReleaseInterface( pPointer );
    ReleaseInterface( pHTMLElement );
    
    return( cr );
}

//+---------------------------------------------------------------------------
//
//  Member:     Private helper function GetSiteContainer()
//
//  Synopsis:   Returns the text site corresponding to the passed in 
//              markup pointer
//
//----------------------------------------------------------------------------

HRESULT     
CAutoRange::GetSiteContainer (
        IMarkupPointer *     pPointer,
        IHTMLElement **      ppSite,
        BOOL *               pfText )
{
    HRESULT        hr = E_FAIL;
    IHTMLElement * pHTMLElement = NULL;
    CDoc         * pDoc;

    Assert( pPointer != NULL && ppSite != NULL );
    if( pPointer == NULL || ppSite == NULL )
        goto Cleanup;

    pDoc = GetMarkup()->Doc();
    Assert( pDoc );

    hr = THR( pDoc->CurrentScopeOrSlave( pPointer, & pHTMLElement ));
    if (hr)
        goto Cleanup;
    
    if( pHTMLElement )
    {
        hr = THR( GetSiteContainer( pHTMLElement, ppSite, pfText ));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    ReleaseInterface( pHTMLElement ); 
    RRETURN( hr );
}


HRESULT     
CAutoRange::GetSiteContainer(
        IHTMLElement *      pElementStart,
        IHTMLElement **     ppSite,
        BOOL *              pfText )
{
    HRESULT         hr = E_FAIL;
    BOOL            fSite = FALSE;
    BOOL            fText = FALSE;
    IHTMLElement *  pHTMLElement = NULL;
    CDoc         *  pDoc;
    IHTMLElement * pElementParent = NULL;

    Assert( pElementStart != NULL && ppSite != NULL );
    Assert( *ppSite == NULL );

    if( pElementStart == NULL || ppSite == NULL )
        goto Cleanup;

    pDoc = GetMarkup()->Doc();
    Assert( pDoc );

    *ppSite = NULL;
    ReplaceInterface( & pHTMLElement, pElementStart );

    while( ! fSite && pHTMLElement != NULL )
    {
        hr = THR( pDoc->IsSite( pHTMLElement, &fSite, &fText, NULL, NULL ));
        if (hr)
            goto Cleanup;

        if( fSite )
        {       
            hr = S_OK;

            ReplaceInterface( ppSite, pHTMLElement );

            if( pfText != NULL )
                *pfText = fText;
        }
        else
        {
            ClearInterface( & pElementParent );
            hr = THR( pHTMLElement->get_parentElement( & pElementParent ));
            ReplaceInterface( & pHTMLElement, pElementParent );
        }
    }
    
Cleanup:
    ReleaseInterface( pElementParent );
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     AdjustForInsert()
//
// . Set the boundary
//
// . Find a text site 
//
// . Bail if we're next to text
//
// . Adjust the boundary to the text site
//
// . Go Left looking for Text while in the text site boundary, break for anything other than phrase elements
//
// . Go right looking for Text, Line break or Block Break while in the text site boundary
//
//----------------------------------------------------------------------------

HRESULT
CAutoRange::AdjustForInsert( IMarkupPointer * pPointerToMove )
{
    CLING_RESULT        cr = CR_Failed;
    HRESULT             hr = S_OK;
    BOOL                fDone = FALSE;
    CElement *          pElement = NULL;
    MARKUP_CONTEXT_TYPE context;
    IMarkupPointer *    pPointer = NULL;
    IHTMLElement *      pHTMLElement = NULL;
    IHTMLElement *      pSite = NULL;
    CDoc  *             pDoc;
    BOOL                fResult;
    BOOL                fTextSite = FALSE;
    long                cch;
    DWORD               dwBreaks;
    CMarkupPointer *    mpPointer;
    IMarkupPointer *    pLeftBoundary = NULL;
    IMarkupPointer *    pRightBoundary = NULL;
    
    pDoc = GetMarkup()->Doc();
    Assert( pDoc );

    if (! _pElemContainer)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( pDoc->CreateMarkupPointer( & pPointer ) );
    if (hr)
        goto Cleanup;

    //
    // Set the range boundaries
    //
    hr = THR_NOTRACE( 
            _pElemContainer->QueryInterface( IID_IHTMLElement, (void **) & pHTMLElement ) );
    if (hr)
        goto Cleanup;
    
    hr = THR( pDoc->CreateMarkupPointer( & pLeftBoundary ) );
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->CreateMarkupPointer( & pRightBoundary ) );
    if (hr)
        goto Cleanup;

    hr = THR( pLeftBoundary->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterBegin ) );
    if (hr)
        goto Cleanup;

    hr = THR( pRightBoundary->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_BeforeEnd ) );
    if (hr)
        goto Cleanup;

    ClearInterface( & pHTMLElement );

    //
    // Position pPointerToMove within a text site
    //

    hr = THR( GetSiteContainer( pPointerToMove, & pSite, & fTextSite ));
    if (hr)
        goto Cleanup;
    
    if( ! fTextSite )
    {
        hr = AdjustIntoTextSite( pPointerToMove, MV_DIR_Right, pRightBoundary );        
        if( FAILED( hr ))
        {
            hr = AdjustIntoTextSite( pPointerToMove, MV_DIR_Left, pLeftBoundary );
        }
        
        if( FAILED( hr ))
        {
            hr = E_FAIL;
            goto Cleanup;
        }        
        ClearInterface( & pSite );
        hr = THR( GetSiteContainer( pPointerToMove, & pSite, & fTextSite ));
        if (hr)
            goto Cleanup;
    }

    //
    // Adjust the boundaries to enclose the text site, if necessary
    //        
    hr = THR( pPointer->MoveAdjacentToElement( pSite, ELEM_ADJ_AfterBegin ) );
    if (hr)
        goto Cleanup;

    hr = THR( pPointer->IsRightOf( pLeftBoundary, & fResult ) );
    if (hr)
        goto Cleanup;

    if ( fResult )
    {
        hr = THR( pLeftBoundary->MoveToPointer( pPointer ) );
        if (hr)
            goto Cleanup;
    }

    hr = THR( pPointer->MoveAdjacentToElement( pSite, ELEM_ADJ_BeforeEnd ) );
    if (hr)
        goto Cleanup;
    
    hr = THR( pPointer->IsLeftOf( pRightBoundary, & fResult ) );
    if (hr)
        goto Cleanup;

    if ( fResult )
    {
        hr = THR( pRightBoundary->MoveToPointer( pPointer ) );
        if (hr)
            goto Cleanup;
    }

    ClearInterface( & pSite );

    //
    // Go Left looking for Text while in the text site boundary, 
    // break for anything other than phrase elements
    //
    hr = THR( pPointer->MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    while ( ! fDone )
    {
        ClearInterface( & pHTMLElement );                    
        cch = 1;

        hr = THR( pPointer->Left( TRUE, &context, &pHTMLElement, & cch, NULL ));
        if ( hr )
            goto Cleanup;

        hr = THR( pPointer->IsLeftOf( pLeftBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            cr = CR_Boundary;
            fDone = TRUE;
            break;
        }

        switch( context )
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
                if (! pHTMLElement)
                    break;

                pElement = NULL;
                IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement) );

                if ( IsIntrinsicTag( pElement->Tag() ) )
                {
                    cr = CR_Intrinsic; 
                    fDone = TRUE;
                }
                else if ( pElement->HasLayout() ||
                          pElement->IsBlockElement() )
                {
                    cr = CR_Intrinsic; 
                    fDone = TRUE;
                }
                break;
               
            case CONTEXT_TYPE_NoScope:
                cr = CR_NoScope;

                if (pHTMLElement)
                {
                    pElement = NULL;
                    IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement) );
                    if (pElement)
                    {
                        if (pElement->HasLayout() 
                            || pElement->Tag() == ETAG_BR)
                        {
                            goto Cleanup;
                        }
                    }
                }
                fDone = TRUE;            
                break;
                
            case CONTEXT_TYPE_Text:
                cr = CR_Text;
                fDone = TRUE;
                break;
                
            case CONTEXT_TYPE_None:
                cr = CR_Boundary;
                fDone = TRUE;
                break;
        }
    }

    if ( cr == CR_Text )
    {
        // The only significant thing to the left is Text
        // We're done
        goto Cleanup;
    }
    
    //
    // Go right looking for Text, Line breaks or Block Breaks 
    // while in the text site boundary
    //

    hr = THR( pPointer->MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & mpPointer ) );
    if (hr)
        goto Cleanup;

    fDone = FALSE;

    while ( ! fDone )
    {
        hr = THR( mpPointer->QueryBreaks( & dwBreaks ) );
        if (hr)
            goto Cleanup;

        if ( dwBreaks == BREAK_BLOCK_BREAK )
        {
            cr = CR_BlockBreak; 
            break;
        }

        ClearInterface( & pHTMLElement );                    
        cch = 1;

        hr = THR( pPointer->Right( TRUE, &context, &pHTMLElement, & cch, NULL ));
        if ( hr )
            goto Cleanup;

        hr = THR( pPointer->IsRightOf( pRightBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            cr = CR_Boundary;
            fDone = TRUE;
            break;
        }

        switch( context )
        {
            case CONTEXT_TYPE_EnterScope:
                if (! pHTMLElement)
                    break;

                pElement = NULL;
                IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                if ( IsIntrinsicTag( pElement->Tag() ) || 
                     pElement->HasLayout() )
                {
                    cr = CR_Intrinsic;
                    fDone = TRUE;
                }
                break;

            case CONTEXT_TYPE_ExitScope:
                if (! pHTMLElement)
                    break;

                pElement = NULL;
                IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                if( pElement->HasLayout() || pElement->IsBlockElement() )
                {
                    cr = CR_Failed;
                    fDone = TRUE;
                }
                break;
                
            case CONTEXT_TYPE_NoScope:
                if (! pHTMLElement)
                    break;

                pElement = NULL;
                IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                if (pElement->Tag() == ETAG_BR )
                {
                    cr = CR_LineBreak;
                    fDone = TRUE;
                }
                else if ( pElement->HasLayout() || pElement->Tag() == ETAG_SCRIPT )
                {
                    cr = CR_NoScope; 
                    fDone = TRUE;
                }
                break;
                
            case CONTEXT_TYPE_Text:
                cr = CR_Text;
                fDone = TRUE;
                break;
                
            case CONTEXT_TYPE_None:
                cr = CR_Failed;
                fDone = TRUE;
                break;
        }
    }

    switch (cr)        
    {
    case CR_LineBreak:
    case CR_Text: 
    case CR_NoScope: 
    case CR_Intrinsic: 
        // We've gone too far, need to go left once
        hr = THR( pPointer->Left( TRUE, &context, NULL, & cch, NULL ));
        if ( hr )
            goto Cleanup;
        hr = THR( pPointerToMove->MoveToPointer( pPointer ));
        goto Cleanup;


    case CR_BlockBreak:
        hr = THR( AdjustLeftIntoEmptyPhrase(pPointer) );
        if (hr)
            goto Cleanup;

        hr = THR( pPointerToMove->MoveToPointer( pPointer ));
        goto Cleanup;

    }


Cleanup:
    ReleaseInterface( pPointer );
    ReleaseInterface( pHTMLElement );
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    ReleaseInterface( pSite );
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CTextSegment::FlipRangePointers()
//
//  Synopsis:   Wiggles pointers to appropriate position
//
//  Arguments:  none
//
//  Returns:    none
//
//----------------------------------------------------------------------------
HRESULT
CAutoRange::FlipRangePointers()
{
    IMarkupPointer *  pTemp = NULL;
    HRESULT         hr = S_OK;
    POINTER_GRAVITY eGravityLeft = POINTER_GRAVITY_Left;    // need to initialize
    POINTER_GRAVITY eGravityRight = POINTER_GRAVITY_Left;  // need to initialize
    
    GetMarkup()->Doc()->CreateMarkupPointer( & pTemp );
      
    if (! pTemp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    // Remember gravity
    hr = THR(_pLeft->Gravity(&eGravityLeft));
    if ( hr )
        goto Cleanup;
        
    hr = THR(_pRight->Gravity(&eGravityRight));
    if ( hr )
        goto Cleanup;

    // Swap pointers
    hr = THR( pTemp->MoveToPointer( _pRight ) );
    if ( hr )
        goto Cleanup;
        
    hr = THR( _pRight->MoveToPointer( _pLeft ) );
    if ( hr )
        goto Cleanup;
        
    hr = THR( _pLeft->MoveToPointer( pTemp ) );
    if ( hr )
        goto Cleanup;
    
    // Swap gravity as well
    if (eGravityLeft != eGravityRight)
    {
        THR(_pLeft->SetGravity(eGravityLeft));
        THR(_pRight->SetGravity(eGravityRight));
    }

Cleanup:
    ReleaseInterface( pTemp );
    RRETURN( hr );
}

HRESULT
CAutoRange::InitPointers()
{
    HRESULT hr = S_OK;

    if ( ! GetMarkup() )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    ClearInterface( & _pLeft );

    hr = THR( _pMarkup->Doc()->CreateMarkupPointer( & _pLeft ) );

    if (hr)
        goto Cleanup;

    WHEN_DBG( SetDebugName( _pLeft, _T( "Range Left" ) ) );
    
    hr = THR( _pLeft->SetGravity( POINTER_GRAVITY_Right ) );
    
    if (hr)
        goto Cleanup;

    ClearInterface( & _pRight );

    hr = THR( _pMarkup->Doc()->CreateMarkupPointer( & _pRight ) );

    if (hr)
        goto Cleanup;
    
    WHEN_DBG( SetDebugName( _pRight, _T( "Range Right" ) ) );
    
    hr = THR( _pRight->SetGravity( POINTER_GRAVITY_Left ) );
    
    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}


HRESULT
CAutoRange::KeepRangeLeftToRight()
{
    HRESULT     hr;
    BOOL        fResult;

    hr = THR( _pLeft->IsRightOf( _pRight, & fResult ) );
    if (hr)
        goto Cleanup;

    if ( fResult )
    { 
        hr = THR( FlipRangePointers() );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}


HRESULT
CAutoRange::ValidatePointers()
{
    HRESULT         hr = S_OK;

    if (! (_pLeft && _pRight) )
        goto Cleanup;

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    hr = THR( AdjustPointers( _pLeft, _pRight ));

Cleanup:   
    RRETURN( hr );
}


//+====================================================================================
//
// Method: IsPhraseElement
//
// Synopsis: Is the element a phrase element
//
//------------------------------------------------------------------------------------
BOOL 
CAutoRange::IsPhraseElement( IHTMLElement *pElement )
{
    HRESULT         hr;
    BOOL            fNotPhrase = TRUE;

    // Make sure the element is not a site or block element
    hr = THR( _pMarkup->Doc()->IsSite(pElement, &fNotPhrase, NULL, NULL, NULL) );
    if (hr || fNotPhrase)
        return FALSE;
        
    hr = THR( _pMarkup->Doc()->IsBlockElement(pElement, &fNotPhrase) );
    if (hr || fNotPhrase)
        return FALSE;

    return TRUE;
}


HRESULT 
CAutoRange::AdjustLeftIntoEmptyPhrase( IMarkupPointer *pLeft )
{
    HRESULT               hr;
    MARKUP_CONTEXT_TYPE   context;
    IHTMLElement          *pElement = NULL;
    IMarkupPointer        *pmpTest = NULL;
    CDoc                  *pDoc = _pMarkup->Doc();

    hr = THR( pDoc->CreateMarkupPointer(&pmpTest) );
    if (hr)
        goto Cleanup;

    hr = THR( pmpTest->MoveToPointer(pLeft) );
    if (hr)
        goto Cleanup;

    do
    {
        ClearInterface(&pElement);
        hr = THR(pmpTest->Left(TRUE, &context, &pElement, NULL, NULL));
        if (hr)
            goto Cleanup;

        if (context == CONTEXT_TYPE_ExitScope && pElement && IsPhraseElement(pElement))
        {
            hr = THR(pmpTest->Right(TRUE, NULL, NULL, NULL, NULL));
            if (hr)
                goto Cleanup;

            hr = THR( pLeft->MoveToPointer(pmpTest) );
        } 
    } 
    while (context == CONTEXT_TYPE_EnterScope && pElement && IsPhraseElement(pElement));

Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pmpTest);
    RRETURN(hr);
}


//+====================================================================================
//
// Method: AdjustPointers
//
// Synopsis: Do the work of clinging for the range. Called by validatepointers
//
//------------------------------------------------------------------------------------



HRESULT
CAutoRange::AdjustPointers( IMarkupPointer *pLeft, IMarkupPointer* pRight)
{
    HRESULT hr;
    CLING_RESULT cr;
    BOOL fResult;
    
    hr = THR( AdjustForInsert( pLeft ) );
    if (hr)
        goto Cleanup;

    hr = THR( pRight->IsLeftOf( pLeft, & fResult ) );
    if (hr)
        goto Cleanup;

    if (fResult)
    {
        hr = THR( pRight->MoveToPointer( pLeft ) );
        if (hr)
            goto Cleanup;
    }

    cr = ClingToText( pRight, pLeft, MV_DIR_Left );

Cleanup:   
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CAutoRange::IsRangeCollpased
//
//  Synopsis:   returns true if left and right pointers are equal
//
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
CAutoRange::IsRangeCollapsed()
{
    BOOL    fEqual = FALSE;
    HRESULT hr;

    hr = THR( _pLeft->IsEqualTo( _pRight, & fEqual ) );
    if (hr)
        goto Cleanup;

Cleanup:
    return fEqual;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAutoRange::SaveHTMLToStream
//
//  Synopsis:   Saves the range text to the specified stream.
//
//----------------------------------------------------------------------------

HRESULT
CAutoRange::SaveHTMLToStream(CStreamWriteBuff * pswb, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    CMarkupPointer mpLeft( GetMarkup()->Doc() );
    CMarkupPointer mpRight( GetMarkup()->Doc() );
    
    hr = THR( mpLeft.MoveToPointer( _pLeft ));
    if( FAILED( hr ))
        goto Cleanup;

    hr = THR( mpRight.MoveToPointer( _pRight ));
    if( FAILED( hr ))
        goto Cleanup;

    //
    // Now we can actually do our work
    //
    {
        CRangeSaver rs( &mpLeft, &mpRight, dwFlags, pswb, GetMarkup() );
        hr = THR( rs.Save());
    }
    
Cleanup:
    RRETURN( hr );
}


HRESULT 
CAutoRange::GetLeft( IMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pLeft ) );
    RRETURN( hr );
}

HRESULT
CAutoRange::GetLeft( CMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pLeft  ) );
    RRETURN( hr );
}

HRESULT 
CAutoRange::GetRight( IMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pRight ) );
    RRETURN( hr );
}

HRESULT 
CAutoRange::GetRight( CMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pRight ) );
    RRETURN( hr );
}

HRESULT 
CAutoRange::SetLeft( IMarkupPointer * ptp )
{
    HRESULT  hr;
    
    // BUGBUG: aren't we guarranteed to have _pLeft and _pRight
    if (! _pLeft )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    
    hr = THR( _pLeft->MoveToPointer( ptp ) );
    if (hr)
        goto Cleanup;

    hr = THR( ValidatePointers() );

Cleanup:
    RRETURN( hr );
}


HRESULT 
CAutoRange::SetRight( IMarkupPointer * ptp )
{
    HRESULT  hr;
    
    if (! _pRight )
    {
        hr = E_FAIL;
        goto Cleanup;
    }


    hr = THR( _pRight->MoveToPointer( ptp ) );
    if (hr)
        goto Cleanup;

    hr = THR( ValidatePointers() );

Cleanup:
    RRETURN( hr );
}


HRESULT 
CAutoRange::SetLeftAndRight( 
                                IMarkupPointer * pLeft, 
                                IMarkupPointer * pRight, 
                                BOOL fAdjustPointers /*=TRUE*/ )
{
    HRESULT  hr;
    
    if (! _pRight || !_pLeft)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( _pRight->MoveToPointer( pRight ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->MoveToPointer( pLeft ) );
    if (hr)
        goto Cleanup;
            
    if ( fAdjustPointers )
    {
        hr = THR( ValidatePointers() );
    }
Cleanup:
    RRETURN( hr );
}

HRESULT 
CAutoRange::GetLeftAndRight( CMarkupPointer * pLeft, CMarkupPointer * pRight )
{
    HRESULT  hr;

    // internal function...
    Assert( pRight );
    Assert( pLeft );
   
    hr = THR( pRight->MoveToPointer( _pRight ) );
    if (hr)
        goto Cleanup;

    hr = THR( pLeft->MoveToPointer( _pLeft ) );

Cleanup:
    RRETURN( hr );
}

CTreeNode * 
CAutoRange::GetNode(BOOL fLeft)
{   
    CMarkupPointer * pmp = NULL;
    HRESULT          hr = S_OK;

    if (fLeft)
        hr = THR_NOTRACE( _pLeft->QueryInterface(CLSID_CMarkupPointer, (void **) & pmp) );
    else
        hr = THR_NOTRACE( _pRight->QueryInterface(CLSID_CMarkupPointer, (void **) & pmp) );
    
    if (hr)
    {
        return NULL;
    }
    
    return pmp ? pmp->CurrentScope(MPTR_SHOWSLAVE) : NULL;
}


CTreeNode * 
CAutoRange::LeftNode()
{   
    return ( GetNode( TRUE ) );
}


CTreeNode * 
CAutoRange::RightNode()
{   
    return ( GetNode( FALSE ) );
}

   
//+---------------------------------------------------------------------------
//
//  Member:     CAutoRange::SetSelectionInlineDirection
//
//  Synopsis:   Sets the text direction in an inline selection. This is similar to
//              SetSelectionBlockDirection. However we place a <SPAN DIR=atDir>
//              around the selection and then remove any DIRs from the element's
//              children.
//
//  Arguments:  atDir               new direction type
//
//  Returns:    HRESULT             S_OK if succesfull or an error
//
//----------------------------------------------------------------------------

HRESULT
CAutoRange::SetSelectionInlineDirection(htmlDir atDir)
{
    HRESULT hr = S_OK;
    // Complex Text - Plumbing for Beta 2 work. Needed for OM Testing

    return hr;
}
    
//+----------------------------------------------------------------------------
//
//  Member:     SetTextRangeToElement
//
//  Synopsis:   Have this range select all text under the influence of the
//              given element.  
//
//  Return:     S_FALSE if the element cannot be located in the tree
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::SetTextRangeToElement ( CElement * pElement )
{
    HRESULT             hr = S_OK;
    IHTMLElement *      pHTMLElement = NULL;
    CElement     *      pElementTarget = NULL;
    CMarkupPointer      mpJustBefore ( GetMarkup()->Doc() );
    CMarkupPointer      mpTemp ( GetMarkup()->Doc() );
    IMarkupPointer *    pLeftBoundary = NULL;
    IMarkupPointer *    pRightBoundary = NULL;

    if (! pElement || ! pElement->IsInMarkup() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // First check whether the element has a slave markup.
    // Note that when CInputTxt::createTextRange() calls us,
    // it passes us a textslave, which does not have a slave markup.
    // However, if the element does have a slave markup we must see 
    // if it is the same as the one that this range was created on.
    //
    if ( pElement->HasSlaveMarkupPtr() )
    {
        CElement * pElementSlave;

        pElementSlave = pElement->GetSlaveMarkupPtr()->FirstElement();
        Assert( pElementSlave );

        if ( pElementSlave == _pElemContainer )
        {
            // The range was created on an INPUT and we've been 
            // asked to move to that same INPUT's text.
            pElementTarget = pElementSlave;
        }
    }

    if ( ! pElementTarget && 
         (pElement == _pElemContainer ||
          (pElement->GetFirstBranch()->Parent() &&
           pElement->GetFirstBranch()->Parent()->GetContainer() == _pElemContainer)))
    {
        pElementTarget = pElement;
    }
    else
    {
        // If the container of pElement is not the range's container 
        // we return an error. 
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    Assert( pElementTarget );

    hr = THR( pElementTarget->QueryInterface( IID_IHTMLElement, (void **) & pHTMLElement ) );
    if (hr)
        goto Cleanup;

    if ( pElementTarget->IsNoScope() )
    {
        // Place the range around the noscope element 
        hr = THR( _pLeft->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_BeforeBegin ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pRight->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterEnd ) );
        if (hr)
            goto Cleanup;

    }
    else
    {
        DWORD               dwBreaks;
        CMarkupPointer  *   pointer;

        // Place the left edge inside the element
        hr = THR( _pLeft->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterBegin ) );
        if (hr)
            goto Cleanup;
        
        // Position the right edge inside the element, unless there is a block break there
        hr = THR( _pRight->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_BeforeEnd ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pointer ) );
        if( hr )
            goto Cleanup;

        hr = THR( pointer->QueryBreaks( & dwBreaks ) );
        if (hr)
            goto Cleanup;

        if ( dwBreaks == BREAK_BLOCK_BREAK )
        {
            hr = THR( _pRight->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterEnd ) );
            if (hr)
                goto Cleanup;
        }
    }

    //
    // To keep IE4 compatibility, shift the left side up to the next "char".
    // Note: we do not shift the right side, since there already is code up above 
    // that positions the right end of the range AFTER the element end, 
    // passed a block break character.
    //

    hr = THR( MovePointersToRangeBoundary( & pLeftBoundary, & pRightBoundary ) );
    
    if (hr)
        goto Cleanup;

    hr = THR( mpTemp.MoveToPointer( _pLeft ) );

    if (hr)
        goto Cleanup;

    hr = THR( MoveCharacter( & mpTemp, MOVEUNIT_PREVCHAR, pLeftBoundary, pRightBoundary, & mpJustBefore ) );

    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->MoveToPointer( & mpJustBefore ) );

    if (hr)
        goto Cleanup;

    //
    //
    //
    hr = THR( ValidatePointers() );
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface( pLeftBoundary );
    ReleaseInterface( pRightBoundary );
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     CAutoRange::GetBstrHelper
//
//  Synopsis:   Gets text from the range into a given bstr in a specified
//              save mode.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoRange::GetBstrHelper(BSTR * pbstr, DWORD dwSaveMode, DWORD dwStrWrBuffFlags)
{
    HRESULT  hr;
    LPSTREAM pIStream = NULL;

    *pbstr = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);
    if (hr)
        goto Cleanup;

    //
    // Use a scope to clean up the StreamWriteBuff
    //
    {
        CStreamWriteBuff StreamWriteBuff(pIStream, CP_UCS_2);

        StreamWriteBuff.SetFlags(dwStrWrBuffFlags);
       
        hr = THR( SaveHTMLToStream( &StreamWriteBuff, dwSaveMode ));
        if (hr)
            goto Cleanup;

        StreamWriteBuff.Terminate();    // appends a null character
    }

    hr = GetBStrFromStream(pIStream, pbstr, TRUE);

Cleanup:

    ReleaseInterface(pIStream);

    RRETURN(hr);
}
 

//+----------------------------------------------------------------------------
//
//  Member:     CAutoRange::GetCommonElement
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

CTreeNode *
CAutoRange::GetCommonNode()
{
    CTreeNode * ptnLeft  = LeftNode();
    CTreeNode * ptnRight = RightNode();
    
    if (ptnLeft && ptnRight)
    {
        return ptnLeft->GetFirstCommonAncestor( ptnRight, NULL);
    }
    else
    {
        return NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CAutoRange::GetCommonContainer
//
//  Synopsis:   Find the common container of the range, if the range is inside
//              the same textsite, we return the textsite, otherwise we return
//              the ped.
//
//-----------------------------------------------------------------------------

CElement *CAutoRange::GetCommonContainer()
{
    return GetCommonNode()->GetContainer();
}

//+----------------------------------------------------------------------------
//
//  Member:     CAutoRange::OwnedBySingleTxtSite
//
//  Synopsis:   Function to determine if a single txt site owns the complete
//              range.
//
//  Arguments   None
//
//-----------------------------------------------------------------------------

BOOL
CAutoRange::OwnedBySingleFlowLayout()
{
    HRESULT     hr;
    BOOL        bResult = FALSE;
    CTreeNode   *ptnLeft  = LeftNode();
    CTreeNode   *ptnRight = RightNode();
    CElement    *pElemFlowLeft;
    CElement    *pElemFlowRight;
    IObjectIdentity *pIdentLeft = NULL;
    IObjectIdentity *pUnkRight = NULL;
    
    if (ptnLeft && ptnRight)
    {
        CFlowLayout * pFlowLeft  = ptnLeft->GetFlowLayout();
        CFlowLayout * pFlowRight = ptnRight->GetFlowLayout();

        if (pFlowLeft && pFlowRight)
        {
            pElemFlowLeft = pFlowLeft->ElementOwner();
            pElemFlowRight = pFlowRight->ElementOwner();

            hr = THR(pElemFlowLeft->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdentLeft));
            if (FAILED(hr))
                goto Cleanup;

            hr = THR(pElemFlowRight->QueryInterface(IID_IUnknown, (LPVOID *)&pUnkRight));
            if (FAILED(hr))
                goto Cleanup;

            hr = THR(pIdentLeft->IsEqualObject(pUnkRight));
            bResult = (hr == S_OK);
        }
    }

Cleanup:
    ReleaseInterface(pIdentLeft);
    ReleaseInterface(pUnkRight);
    return bResult;
}

//+----------------------------------------------------------------------------
//
//  Member:     CAutoRange::SelectionInOneTxtSite
//
//  Synopsis:   Function to determine if a single txt site owns both the
//              begin and end of a selection.
//
//  Arguments   None
//
//-----------------------------------------------------------------------------

BOOL
CAutoRange::SelectionInOneFlowLayout()
{
    BOOL fRet = TRUE;
    CFlowLayout * pFromOFL;
    CFlowLayout * pToOFL;
    CTreeNode * ptnLeft  = LeftNode();
    CTreeNode * ptnRight = RightNode();
    
    if (ptnLeft && ptnRight) 
    {
        pFromOFL = ptnLeft->GetFlowLayout();
        pToOFL   = ptnRight->GetFlowLayout();

        fRet = pFromOFL == pToOFL;

    }
    return fRet;
}

#if 0
//
// These routines have been moved to MshtmlEd
//

//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareBSTRS, local helper
//
//  Synopsis:   compares 2 btrs
//
//-----------------------------------------------------------------------------

BOOL VariantCompareBSTRS(VARIANT * pvar1, VARIANT * pvar2)
{
    BOOL    fResult;
    TCHAR  *pStr1;
    TCHAR  *pStr2;

    if (V_VT(pvar1) == VT_BSTR && V_VT(pvar2) == VT_BSTR)
    {
        pStr1 = V_BSTR(pvar1) ? V_BSTR(pvar1) : g_Zero.ach;
        pStr2 = V_BSTR(pvar2) ? V_BSTR(pvar2) : g_Zero.ach;

        fResult = StrCmpC(pStr1, pStr2) == 0;
    }
    else
    {
        fResult = FALSE;
    }

    return fResult;
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareFontSize, local helper
//
//  Synopsis:   compares font size
//
//-----------------------------------------------------------------------------

BOOL VariantCompareFontSize(VARIANT * pvarSize1, VARIANT * pvarSize2)
{
    CVariant    convVar1;
    CVariant    convVar2;
    BOOL        fResult;

    Assert(pvarSize1);
    Assert(pvarSize2);

    if (   V_VT(pvarSize1) == VT_NULL
        || V_VT(pvarSize2) == VT_NULL
       )
    {
        fResult = V_VT(pvarSize1) == V_VT(pvarSize2);
        goto Cleanup;
    }

    if (VariantChangeTypeSpecial(&convVar1, pvarSize1, VT_I4))
        goto Error;

    if (VariantChangeTypeSpecial(&convVar2, pvarSize2, VT_I4))
        goto Error;

    fResult = V_I4(&convVar1) == V_I4(&convVar2);

Cleanup:
    return fResult;

Error:
    fResult = FALSE;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareColor, local helper
//
//  Synopsis:   compares color
//
//-----------------------------------------------------------------------------

BOOL VariantCompareColor(VARIANT * pvarColor1, VARIANT * pvarColor2)
{
    BOOL        fResult;
    CVariant    var;
    COLORREF    color1;
    COLORREF    color2;

    if (   V_VT(pvarColor1) == VT_NULL
        || V_VT(pvarColor2) == VT_NULL
       )
    {
        fResult = V_VT(pvarColor1) == V_VT(pvarColor2);
        goto Cleanup;
    }

    if (VariantChangeTypeSpecial(&var, pvarColor1,  VT_I4))
        goto Error;

    color1 = (COLORREF)V_I4(&var);

    if (VariantChangeTypeSpecial(&var, pvarColor2, VT_I4))
        goto Error;

    color2 = (COLORREF)V_I4(&var);

    fResult = color1 == color2;

Cleanup:
    return fResult;

Error:
    fResult = FALSE;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareFontName, local helper
//
//  Synopsis:   compares font names
//
//-----------------------------------------------------------------------------

BOOL VariantCompareFontName(VARIANT * pvarName1, VARIANT * pvarName2)
{
    return VariantCompareBSTRS(pvarName1, pvarName2);
}

#endif // 0


//+----------------------------------------------------------------------------
//
//  Method:     CheckOwnerSiteOrSelection
//
//-----------------------------------------------------------------------------
HRESULT
CAutoRange::CheckOwnerSiteOrSelection(ULONG cmdID)
{
    HRESULT     hr = S_OK;

    switch ( cmdID )
    {
    case IDM_OVERWRITE:
        if (!OwnedBySingleFlowLayout())
        {
            hr = S_FALSE;
        }
        break;

    case IDM_IMAGE:
    case IDM_PARAGRAPH:
    case IDM_IFRAME:
    case IDM_TEXTBOX:
    case IDM_TEXTAREA:
#ifdef  NEVER
    case IDM_HTMLAREA:
#endif
    case IDM_CHECKBOX:
    case IDM_RADIOBUTTON:
    case IDM_DROPDOWNBOX:
    case IDM_LISTBOX:
    case IDM_BUTTON:
    case IDM_MARQUEE:
    case IDM_1D:
    case IDM_LINEBREAKNORMAL:
    case IDM_LINEBREAKLEFT:
    case IDM_LINEBREAKRIGHT:
    case IDM_LINEBREAKBOTH:
    case IDM_HORIZONTALLINE:
    case IDM_INSINPUTBUTTON:
    case IDM_INSINPUTIMAGE:
    case IDM_INSINPUTRESET:
    case IDM_INSINPUTSUBMIT:
    case IDM_INSINPUTUPLOAD:
    case IDM_INSFIELDSET:
    case IDM_INSINPUTHIDDEN:
    case IDM_INSINPUTPASSWORD:

    case IDM_GETBLOCKFMTS:
    case IDM_TABLE:

    case IDM_CUT:
    case IDM_PASTE:

        if (!SelectionInOneFlowLayout())
        {
            hr = S_FALSE;
        }
        break;
    }

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Method:     FSupportsHTML
//
//-----------------------------------------------------------------------------
BOOL
CAutoRange::FSupportsHTML()
{
    CTreeNode * pNode  = GetCommonNode();

    return ( pNode && pNode->SupportsHtml() );
}

//
// Helper functions so that measurer can make empty lines
// have the same height they will have after we spring
// load them and put text in them.
// 
long
GetSpringLoadedHeight(IMarkupPointer * pmpPosition, CFlowLayout * pFlowLayout, short * pyDescentOut)
{
    CDoc         * pDoc = pFlowLayout->Doc();
    CTreeNode    * pNode = pFlowLayout->GetFirstBranch();
    CCcs         * pccs;
    CBaseCcs     * pBaseCcs;
    CCharFormat    cfLocal = *(pNode->GetCharFormat());
    CCalcInfo      CI;
    int            yHeight = -1;
    CVariant       varIn, varOut;
    GUID           guidCmdGroup = CGID_MSHTML;
    HRESULT        hr;

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = pmpPosition;

    hr = THR(pDoc->Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, &varIn, &varOut));
    V_VT(&varIn) = VT_NULL;
    if (hr || V_VT(&varOut) == VT_NULL)
        goto Cleanup;

    // We now know that we have to apply compose settings on this line.

    // Get font size.
    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = IDM_FONTSIZE;
    hr = THR(pDoc->Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, &varIn, &varOut));
    if (hr)
        goto Cleanup;

    // If we got a valid font size, apply it to the local charformat.
    if (V_VT(&varOut) == VT_I4 && V_I4(&varOut) != -1)
    {
        int iFontSize = ConvertHtmlSizeToTwips(V_I4(&varOut));
        cfLocal.SetHeightInTwips(iFontSize);
    }

    // Get font name.
    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = IDM_FONTNAME;
    hr = THR(pDoc->Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, &varIn, &varOut));
    if (hr)
        goto Cleanup;

    // If we got a valid font name, apply it to the local charformat.
    if (V_VT(&varOut) == VT_BSTR)
    {
        TCHAR * pstrFontName = V_BSTR(&varOut);
        cfLocal.SetFaceName(pstrFontName);
    }

    cfLocal._bCrcFont = cfLocal.ComputeFontCrc();

    CI.Init(pFlowLayout);

    pccs = fc().GetCcs(CI._hdc, &CI, &cfLocal);

    if (pccs)
    {
        pBaseCcs = pccs->GetBaseCcs();
        yHeight = pBaseCcs->_yHeight + pBaseCcs->_yOffset;

        if (pyDescentOut)
            *pyDescentOut = (INT) pBaseCcs->_yDescent;

        pccs->Release();
    }

Cleanup:

    return yHeight;
}



long
GetSpringLoadedHeight(CCalcInfo *pci, CFlowLayout * pFlowLayout, CTreePos *ptp, long cp, short * pyDescentOut)
{
    CElement     * pElementContent = pFlowLayout->ElementContent();
    int            yHeight;

    Assert(pyDescentOut);

    if (   pElementContent
        && pElementContent->HasFlag(TAGDESC_ACCEPTHTML)
       )
    {
        CMarkup      * pMarkup = pFlowLayout->GetContentMarkup();
        CDoc         * pDoc = pMarkup->Doc();
        CMarkupPointer mpComposeFont(pDoc);
        HRESULT        hr;

        hr = THR(mpComposeFont.MoveToCp(cp, pMarkup));
        if (hr)
        {
            yHeight = -1;
            goto Cleanup;
        }

        yHeight = GetSpringLoadedHeight(&mpComposeFont, pFlowLayout, pyDescentOut);
    }
    else
    {
        WHEN_DBG(CMarkup *pMarkup = pFlowLayout->GetContentMarkup());
        WHEN_DBG(LONG junk);

        Assert(ptp);
        Assert(ptp == pMarkup->TreePosAtCp(cp, &junk));
        const CCharFormat *pCF = ptp->GetBranch()->GetCharFormat();
        CCcs *pccs = fc().GetCcs(pci->_hdc, pci, pCF);
        CBaseCcs *pBaseCcs;
        
        if (!pccs)
        {
            yHeight = -1;
            goto Cleanup;
        }
        pBaseCcs = pccs->GetBaseCcs();
        yHeight = pBaseCcs->_yHeight;
        *pyDescentOut = pBaseCcs->_yDescent;

        pccs->Release();
    }

Cleanup:

    return yHeight;
}


