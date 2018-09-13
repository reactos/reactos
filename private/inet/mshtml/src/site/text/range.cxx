/*
 *  @doc INTERNAL
 *
 *  @module RANGE.C - Implement the CTxtRange Class |
 *
 *      This module implements the internal CTxtRange methods.  See range2.c
 *      for the ITextRange methods
 *
 *  Authors: <nl>
 *      Original RichEdit code: David R. Fulmer <nl>
 *      Christian Fortini <nl>
 *      Murray Sargent <nl>
 *
 *  Revisions: <nl>
 *      AlexGo: update to runptr text ptr; floating ranges
 */

#include "headers.hxx"

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__RANGE_H_
#define X__RANGE_H_
#include "_range.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_RANGE_H_
#define X_RANGE_H_
#include "range.h"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_DOBJ_HXX_
#define X_DOBJ_HXX_
#include "dobj.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_TOMCONST_H_
#define X_TOMCONST_H_
#include "tomconst.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

MtDefine(CTxtRange, Tree, "CTxtRange")
MtDefine(CTxtRangeNukePhraseElementsUnderBlockElement_aryElements_pv, Locals, "CTxtRange::NukePhraseElementsUnderBlockElement aryElements::_pv")

// ===========================  Invariant stuff  ======================================================

#define DEBUG_CLASSNAME CTxtRange
#include "_invar.h"

#if DBG==1

void CTxtRange::DumpTree()
{
    GetMarkup()->DumpTree();
}

// BUGBUG: MERGEFUN (raminh)
//         Invariant is called by TSR, it should go away once TSR is completely gone
BOOL
CTxtRange::Invariant( void ) const
{
    return FALSE;
}

#endif

CTxtRange::CTxtRange ( CMarkup *  pMarkup,
                       CElement * pElemContainer ) 
{
#ifdef MERGEFUN //rtp
    long cpLast  = GetLastCp();
    long cpOther = cp - cch;            // Calculate cpOther with entry cp

    ValidateCp(cpOther);                // Validate requested other end
    cp = GetCp();                       // Validated cp
    if(cp == cpOther && cp > cpLast)    // IP cannot follow undeletable
        cp = cpOther = SetCp(cpLast);   //  EOP at end of story

    _cch = cp - cpOther;                // Store valid length
#endif

    Assert(!pElemContainer || pElemContainer->GetMarkup() == pMarkup);
    CElement::SetPtr( & _pElemContainer, pElemContainer );
    _pMarkup = pMarkup;

    _pNodeHint = NULL;
    ClearFlags();
    _pLeft = _pRight = NULL;
    InitPointers();
}

CTxtRange::CTxtRange ( const CTxtRange & rg ) 
{
    CElement::SetPtr( & _pElemContainer, rg._pElemContainer );
    CTreeNode::SetPtr( & _pNodeHint, rg._pNodeHint );
    ClearFlags();
    _pLeft = _pRight = NULL;
    InitPointers();
}

CTxtRange::~CTxtRange()
{
    ReleaseInterface( _pLeft );
    ReleaseInterface( _pRight );

    CTreeNode::ClearPtr(&_pNodeHint);
    CElement::ClearPtr( & _pElemContainer );
}
 

HRESULT
CTxtRange::AdjustForInsert()
{
    RRETURN( ValidatePointers() );
}


BOOL
IsIntrinsicTag( ELEMENT_TAG eTag )
{
    switch (eTag)
    {
    case ETAG_BUTTON:
    case ETAG_TEXTAREA:
    case ETAG_HTMLAREA:
    case ETAG_FIELDSET:
    case ETAG_LEGEND:
    case ETAG_MARQUEE:
    case ETAG_SELECT:
        return TRUE;

    default:
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CTextSegment::ClingToText()
//
//  Synopsis:   Move pOrigin towards pTarget while the context type is not text. 
//              The direction of the move is denoted by fLeftBound.
//              If the move causes pOrigin to pass pTarget, the pointers are
//              adjusted to move next to each other.
//
//----------------------------------------------------------------------------

CLING_RESULT
CTxtRange::ClingToText( 
    IMarkupPointer *        pInPointer, 
    IMarkupPointer *        pBoundary, 
    MV_DIR                  eDir, 
    PTR_ADJ                 ePtrAdj )
{
    CLING_RESULT        cr = CR_Failed;
    HRESULT             hr = S_OK;
    BOOL                fDone = FALSE;
    INT                 iResult = 0;
    BOOL                fLeft = eDir == MV_DIR_Left;
    BOOL                fAdjustingOut = ePtrAdj == PTR_ADJ_Out;
    CElement *          pElement = NULL;
    MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;

    IMarkupPointer *    pPointer = NULL;
    IHTMLElement *      pHTMLElement = NULL;

    hr = THR( _pMarkup->Doc()->CreateMarkupPointer( & pPointer ) );
    if (! hr)   hr = THR( pPointer->MoveToPointer( pInPointer ) );
    if (hr)
        goto Cleanup;

    //
    // Quick Out: Check if I'm adjacent to text. If so, go nowhere.
    //

    if( ! fAdjustingOut )
    {
        if( fLeft )
            hr = THR( pPointer->Right( FALSE, & eCtxt, NULL, NULL, NULL ) );
        else
            hr = THR( pPointer->Left( FALSE, & eCtxt, NULL , NULL, NULL ) );
        
        if( eCtxt == CONTEXT_TYPE_Text )
        {
            cr = CR_Text;
            goto Cleanup;   // Nothing to do!
        }
    }

    while( ! fDone )
    {
        ClearInterface( & pHTMLElement );

        //
        // If the pointer is beyond the boundary, fall out
        // boundary left of pointer =  -1
        // boundary equals pointer =    0
        // boundary right of pointer =  1
        //
        
        IGNORE_HR( pBoundary->Compare( pPointer , &iResult ));

        if(( fLeft && iResult > -1 ) || ( ! fLeft && iResult < 1 ))
        {
            cr = CR_Boundary;
            goto Done;
        }
            
        if( fLeft )
        {
            hr = THR( pPointer->Left( TRUE, &eCtxt, &pHTMLElement, NULL, NULL ));
        }
        else
        {
            hr = THR( pPointer->Right( TRUE, &eCtxt, &pHTMLElement, NULL, NULL ));
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
                }
                break;

            case CONTEXT_TYPE_ExitScope:
                if( pHTMLElement )
                {
                    pElement = NULL;
                    IGNORE_HR( pHTMLElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement));

                    if( ( ! fAdjustingOut ) && pElement 
                        && (pElement->HasLayout() || pElement->IsBlockElement() || pElement->Tag()==ETAG_BR))
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

    if( fAdjustingOut && cr == CR_Boundary )
    {
        //
        // Our goal is to hit either text or our boundary
        //

        hr = THR( pInPointer->MoveToPointer( pPointer ));
    }
    else
    {
        //
        // If we found text, move our pointer
        //
        
        switch( cr )
        {
            case CR_Text:
            case CR_NoScope:
            case CR_Intrinsic:

                //
                // We have inevitably gone one move too far, back up one move
                //

                if( fLeft )
                {
                    hr = THR( pPointer->Right( TRUE, &eCtxt, NULL, NULL, NULL ));
                }
                else
                {
                    hr = THR( pPointer->Left( TRUE, &eCtxt, NULL, NULL, NULL ));
                }

                if( FAILED( hr ))
                    goto Cleanup;

                //
                // Now we position the pointer
                //
                
                hr = THR( pInPointer->MoveToPointer( pPointer ));
                break;
        }
    }

Cleanup:
    ReleaseInterface( pPointer );
    ReleaseInterface( pHTMLElement );
    
    return( cr );
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
CTxtRange::FlipRangePointers()
{
    IMarkupPointer *  pTemp = NULL;
    HRESULT         hr = S_OK;
    POINTER_GRAVITY eGravityLeft;
    POINTER_GRAVITY eGravityRight;
    
    _pMarkup->Doc()->CreateMarkupPointer( & pTemp );
      
    if (! pTemp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR( pTemp->MoveToPointer( _pRight ) );
    if (! hr)   hr = THR( _pRight->MoveToPointer( _pLeft ) );
    if (! hr)   hr = THR( _pLeft->MoveToPointer( pTemp ) );

    // Swap gravity as well
    if (! hr)   hr = THR(_pLeft->Gravity(&eGravityLeft));
    if (! hr)   hr = THR(_pRight->Gravity(&eGravityRight));
    if ( hr )
        goto Cleanup;
    
    if (eGravityLeft != eGravityRight)
    {
        THR(_pLeft->SetGravity(eGravityRight));
        THR(_pRight->SetGravity(eGravityLeft));
    }

Cleanup:
    ReleaseInterface( pTemp );
    RRETURN( hr );
}

HRESULT
CTxtRange::InitPointers()
{
    HRESULT     hr = S_OK;

    if ( ! _pMarkup )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if ( _pLeft )
        ClearInterface( & _pLeft );

    _pMarkup->Doc()->CreateMarkupPointer( & _pLeft );
    if (! _pLeft )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = _pLeft->SetGravity(POINTER_GRAVITY_Right);
    if ( hr )
        goto Cleanup;

    if ( _pRight )
        ClearInterface( & _pRight );

    _pMarkup->Doc()->CreateMarkupPointer( & _pRight );
    if (! _pRight )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = _pRight->SetGravity(POINTER_GRAVITY_Left);
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}


HRESULT
CTxtRange::KeepRangeLeftToRight()
{
    HRESULT     hr;
    int         iResult;

    hr = THR( _pLeft->Compare( _pRight, & iResult ) );
    if (hr)
        goto Cleanup;

    if ( iResult == 1 )
    { 
        hr = THR( FlipRangePointers() );
        if (hr)
            goto Cleanup;
    }
Cleanup:
    RRETURN( hr );
}


HRESULT
CTxtRange::ValidatePointers()
{
    HRESULT         hr = S_OK;
    CLING_RESULT    cr = CR_Failed;
    
    if (! (_pLeft && _pRight) )
        goto Cleanup;

    hr = THR( KeepRangeLeftToRight() );
    if (hr)
        goto Cleanup;

    cr = ClingToText( _pLeft, _pRight, MV_DIR_Right );
    cr = ClingToText( _pRight, _pLeft, MV_DIR_Left );

Cleanup:   
    RRETURN( hr );

}


/*
 *  CTxtRange::CheckChange(cpSave, cchSave)
 *
 *  @mfunc
 *      Set _cch according to _fExtend and set selection-changed flag if
 *      this range is a CTextSelectionRecord and the new _cp or _cch differ from
 *      cp and cch, respectively.
 *
 *  @devnote
 *      We can count on GetCp() and cpSave both being <= GetTextLength(),
 *      but we can't leave GetCp() equal to GetTextLength() unless _cch ends
 *      up > 0.
 */
void CTxtRange::CheckChange(
    long cpSave,        // Original _cp  for this range
    long cchSave)       // Original _cch for this range
{
#ifdef MERGEFUN // rtp 
    _cch = 0;                                   // Default insertion point
    if(_fExtend)                                // Wants to be nondegenerate
        _cch = GetCp() - (cpSave - cchSave);    //  and maybe it is

    if (!_cch && long(GetCp()) > GetLastCp())   // If still IP and active end
        SetCp(GetLastCp());                     //  follows nondeletable EOP,
                                                //  backspace over that EOP
    if(cpSave != (long)GetCp() || cchSave != _cch)
    {
#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
        if(_fSel)
            GetPed()->GetCallMgr()->SetSelectionChanged();
#endif

        _TEST_INVARIANT_
    }
#endif
}

/*
 *  CTxtRange::GetRange
 *
 *  @mfunc
 *      set cpMin  = this range cpMin
 *      set cpMost = this range cpMost
 *      return cpMost - cpMin, i.e. abs(_cch)
 *
 *  @rdesc
 *      abs(_cch)
 */

long
CTxtRange::GetRange (
    long & cpMin, long & cpMost
#if DBG==1
    , BOOL fIsValid
#endif
        ) const
{
#ifdef MERGEFUN // rtp
    long cch = _cch;

    if(cch >= 0)
    {
        cpMost  = GetCp();
        cpMin   = cpMost - cch;
    }
    else
    {
        cch     = -cch;
        cpMin   = GetCp();
        cpMost  = cpMin + cch;
    }

#if DBG==1
    if (fIsValid)
    {
        Assert(cpMin  >= GetFirstCp());
        Assert(cpMost <= GetLastCp());
    }
#endif
    Assert(cpMin <= cpMost);

    return cch;
#else
    return 0;
#endif
}

/*
 *  CTxtRange::GetRange()
 *
 *  @mfunc
 *      return the passed in range's cpMin and cpMost
 *
 */
void
CTxtRange::GetRange (
    long cp, long cch,
    long & cpMin, long & cpMost)
{
    if(cch >= 0)
    {
        cpMost  = cp;
        cpMin   = cpMost - cch;
    }
    else
    {
        cch     = -cch;
        cpMin   = cp;
        cpMost  = cpMin + cch;
    }
    Assert(cpMin <= cpMost);
    Assert(cpMost - cpMin == cch);
}


/*
 *  CTxtRange::GetCpMin()
 *
 *  @mfunc
 *      return this range's cpMin
 *
 *  @rdesc
 *      cpMin
 *
 *  @devnote
 *      If you need cpMost and/or cpMost - cpMin, GetRange() is faster
 */
long CTxtRange::GetCpMin() const
{
#ifdef MERGEFUN // rtp
    long cp = GetCp();
    Assert(cp >= GetFirstCp() && (cp - _cch) >= GetFirstCp());
    return min(cp, cp - _cch);
#else
    return 0;
#endif
}

/*
 *  CTxtRange::GetCpMost()
 *
 *  @mfunc
 *      return this range's cpMost
 *
 *  @rdesc
 *      cpMost
 *
 *  @devnote
 *      If you need cpMin and/or cpMost - cpMin, GetRange() is faster
 */
long CTxtRange::GetCpMost() const
{
    AssertSz( 0, "This sucker no longer works!" );

    return 0;
}

/*
 *  CTxtRange::Update(fScrollIntoView)
 *
 *  @mfunc
 *      Virtual stub routine overruled by CTextSelectionRecord::Update() when this
 *      text range is a text selection.  The purpose is to update the screen
 *      display of the caret or selection to correspond to changed cp's.
 *
 *  @rdesc
 *      TRUE
 */
BOOL CTxtRange::Update (
    BOOL fScrollIntoView,   //@parm TRUE if should scroll caret into view
    BOOL fIsSelAnchor,
    CLayout *pCurrentLayout)
{
    return TRUE;                // Simple range has no selection colors or
}                               //  caret, so just return TRUE

BOOL
CTxtRange::Set ( long cp, long cch )
{
    AssertSz( 0, "This no longer works!" );
    return FALSE;
}

/*
 *  CTxtRange::Advance(cch)
 *
 *  @mfunc
 *      Advance active end of range by cch.
 *      Other end stays put iff _fExtend
 *
 *  @rdesc
 *      cch actually moved
 */
long CTxtRange::Advance (
    long cch)
{
    AssertSz( 0, "This no longer works!" );
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     ValidateReplace
//
//  Synopsis:   This member determines if it is okay to remove/replace a
//              sequence of characters.  Bad sequences would involve, for
//              example, those which partially overlap tables.
//
//  Return:     Returns S_FALSE if the chars can not be replaced, or a
//              correction cannot be performed.
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::ValidateReplace ( BOOL fNonEmptyReplace, BOOL fPerformCorrection )
{
    AssertSz( 0, "This sucker no longer works!" );
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AdjustCRLF
//
//  Synopsis:   Moves backward or forward a character (treating CRLF stuff
//              as a single char).
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::AdjustCRLF ( long dDir, long * pcch )
{
    AssertSz( 0, "This sucker no longer works!" );
    return E_FAIL;
}

/*
 *  CTxtRange::FindWordBreak(action)
 *
 *  @mfunc
 *      Move active end as determined by plain-text FindWordBreak().
 *      Other end stays put iff _fExtend
 *
 *  @rdesc
 *      cch actually moved
 */
long CTxtRange::FindWordBreak (
    INT action)         // @parm action defined by CTxtPtr::FindWordBreak()
{
    AssertSz( 0, "This sucker no longer works!" );
    return E_FAIL;
}

/*
 *  CTxtRange::FlipRange()
 *
 *  @mfunc
 *      Flip active and non active ends
 */
void CTxtRange::FlipRange()
{
    AssertSz( 0, "This sucker no longer works!" );
}

//+----------------------------------------------------------------------------
//
//  Member:     GetFirstLine
//
//  Synopsis:   Helper function.
//              Skips any leading CRs and LFs. Points *ppchIn to the start of
//              the next character if any (otherwise to NULL). Returns the
//              number of characters in that line (upto but excluding hte next
//              CR or LF).
//
//-----------------------------------------------------------------------------

long GetFirstLine(long cch, LPCTSTR * ppchIn)
{
    LPCTSTR pch;
    long    ichStart, ichEnd;

    Assert(cch > 0 && ppchIn && *ppchIn);

    // Jump to the first character after any leading CRs and LFs
    ichStart = 0;
    pch = *ppchIn;
    while (ichStart < cch && (*pch == _T('\n') || *pch == _T('\r')))
    {
        pch++;
        ichStart++;
    }

    if (ichStart == cch)
    {
        *ppchIn = NULL;
        return 0;
    }

    // Go on until the next CR or LF
    ichEnd = ichStart + 1;
    pch++;
    while (ichEnd < cch && *pch != _T('\n') && *pch != _T('\r'))
    {
        pch++;
        ichEnd++;
    }

    Assert (ichEnd > ichStart);
    *ppchIn = *ppchIn + ichStart;
    return (ichEnd - ichStart);
}

//+----------------------------------------------------------------------------
//
//  Member:     SplitLine
//
//  Synopsis:   This handles a shift-enter which usually ends up in inserting
//              a <br> tag.
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::SplitLine ( )
{
#ifdef MERGEFUN // rtp
    CElement *     pElementBr = NULL;
    HRESULT        hr;

    //
    // If the parent of the new break is empty, it must not break, so set
    // the break on empty tag.
    //

    SearchBranchForBlockElement()->Element()->_fBreakOnEmpty = TRUE;

    //
    // Place a new <br> at this point in the stream, marking it such that
    // it will break a line even is it "contains" no text.
    //

    hr = InsertBreak( TRUE );

    if (hr)
        goto Cleanup;

    //
    // Make sure we are in the <br> run, after any chars in it (especially any
    // line breaking characters which were just inserted by fixing up the line
    // breaks).
    //

    Assert( CurrBranch()->Tag() == ETAG_BR );

    AdvanceToEndOfRun();

Cleanup:

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     SplitForm
//
//  Synopsis:   This does not really split the form, bu introduces a new block
//              element under the form to cause the contents of the form to
//              split.
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::SplitForm ( )
{
#ifdef MERGEFUN // rtp
    CTreePosList &  eruns = GetList();
    CRangeRestorer  rr ( this );
    HRESULT         hr = S_OK;
    CElement *      pElementForm;
    long            iRunFormLast;
    CElement *      pElementNew = NULL;

    pElementForm = SearchBranchForBlockElement()->Element();

    Assert( pElementForm->Tag() == ETAG_FORM );

    //
    // Prepare the runs to accept the new element
    //

    hr = THR( SplitRun() );

    if (hr)
        goto Cleanup;

    Verify( AdjustForward() );

    //
    // Make sure we hold our place after line breaking
    //

    hr = THR( rr.Set() );

    if (hr)
        goto Cleanup;


    //
    // Locate the right extent of the form tag and insert the new element over
    // to that run.
    //

    iRunFormLast = GetIRun();

    while (
        iRunFormLast + 1 <= GetLastRun() &&
        SameScope(
            SearchBranchForBlockElement( iRunFormLast + 1 ), pElementForm ) )
    {
        iRunFormLast++;
    }

    hr = THR(
        eruns.CreateAndInsertElement(
            NULL, GetIRun(), iRunFormLast, GetDefaultBlockTag(), & pElementNew ) );

    if (hr)
        goto Cleanup;

    //
    // Must mark form as break on empty if hitting return at very beginning
    // of form.  Also mark the new element as break on empty.
    //

    pElementNew->_fBreakOnEmpty = TRUE;
    pElementForm->_fBreakOnEmpty = TRUE;

    hr = THR( rr.Restore() );

    if (hr)
        goto Cleanup;

Cleanup:

    CElement::ClearPtr( & pElementNew );

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     ReplaceRange
//
//  Synopsis:   Replaces the currently "selected" characters with the given
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::ReplaceRange ( long cchNew, TCHAR const * pch, BOOL fCanMerge )
{
    AssertSz( 0, "This sucker no longer works!" );
    return E_FAIL;
#if 0
    //WHEN_DBG( long cchOld = GetTextLength(); )
    HRESULT hr;
    CDoc *  pDoc;
    BOOL    fEqual; 

    _TEST_INVARIANT_;

    // Make sure we are not pasting into read-only-through-OM controls like InputFile
    Assert(GetCommonContainer() && !GetCommonContainer()->TestClassFlag(CElement::ELEMENTDESC_OMREADONLY) || !GetMarkup()->Doc()->IsInScript());

    // If we are inserting text into a single-line edit control, filter out
    // the CRs and LFs and add only the first line of text.
    if (cchNew > 0 && !GetCommonContainer()->HasFlowLayout()->GetMultiLine())
        // BUGBUG: what do we do if we don't hav a layout yet?
        cchNew = GetFirstLine(cchNew, &pch);

    pDoc = _pMarkup->Doc();
    Assert( pDoc );

    hr = THR( _pLeft->Equal ( _pRight, & fEqual ) );
    if (hr)
        goto Cleanup;

    if (! fEqual)
    {
        hr = THR( pDoc->Remove( _pLeft, _pRight ) );
        if (hr)
            goto Cleanup;
    }

    if (pch)
    {
        hr = THR( pDoc->InsertText( const_cast<OLECHAR *> (pch), -1, _pRight ) );
        if (hr)
            goto Cleanup;
    }

#ifdef MERGEFUN // BUGBUG raminh

    Assert( hr || GetTextLength() + cchForReplace - cchNew == cchOld );

    if (cchNew > 0)
    {
        long cpNew = GetCp();

        Set(cpNew, cchNew);
        FlipRange();
        AdvanceToNonEmpty();
        Set(cpNew, 0);
    }
#endif

Cleanup:
    RRETURN( hr );
#endif
}



//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetCharFormatForRange
//
//  Synopsis:   Returns the text format for current selection
//
//  Arguments:  pCF - On return contains the format at the start of selection
//
//-----------------------------------------------------------------------------

void CTxtRange::GetCharFormatForRange(const CCharFormat *  * ppCF) const
{
#ifdef MERGEFUN // rtp
    long    iRunStart;
    CTxtRange range(*this);

    Assert(ppCF != NULL);

    if (range.GetCch() > 0)
        range.RetreatToNonEmpty();
    else if (range.GetCch() < 0)
        range.AdvanceToNonEmpty();

    // Get the first run in the selected range
    IGNORE_HR( range.GetEndRuns( & iRunStart, NULL ) );

    *ppCF = GetList().GetBranchAbs(iRunStart)->GetCharFormat();

    // We don't return NINCHed now. If selection contains different formats
    // only the format at the selection start is returned
    // NB (cthrash) since we don't support NINCH, remove mask altogether
    // pCF->dwMask = CFM_ALL2;
#endif
}

static BOOL IsPara ( CElement * pElement )
    { return pElement->Tag() == ETAG_P; }

static BOOL IsList ( CElement * pElement )
{
    return pElement->HasFlag(TAGDESC_LIST);
}

static BOOL IsListItem ( CElement * pElement )
{
    return pElement->HasFlag(TAGDESC_LISTITEM);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::IsRangeCollpased
//
//  Synopsis:   returns true if left and right pointers are equal
//
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
CTxtRange::IsRangeCollapsed()
{
    BOOL    fEqual = FALSE;
    HRESULT hr;

    hr = THR( _pLeft->Equal( _pRight, & fEqual ) );
    if (hr)
        goto Cleanup;

Cleanup:
    return fEqual;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::IsRangeEmpty
//
//  Synopsis:   returns true if the given set of runs are empty
//
//  Arguments:  iRunStart:  start run of the given range
//              iRunEnd:    End run of the given range
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------
BOOL
CTxtRange::IsRangeEmpty(LONG iRunStart, LONG iRunEnd)
{
#ifdef MERGEFUN // iRuns
    while (iRunStart <= iRunEnd)
    {
        if((GetRunAbs(iRunStart++))->Cch())
        {
            return FALSE;
        }
    }
#endif 
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::ApplyList
//
//  Synopsis:   Incorporates given element into the tree over the
//              current selection
//
//----------------------------------------------------------------------------

#ifdef MERGEFUN // iRuns
HRESULT
CTxtRange::ApplyList ( ELEMENT_TAG etagList, long iRunStart, long iRunFinish)
{
    HRESULT         hr = S_OK;
    ELEMENT_TAG     etagListItem;
    BOOL            fIsDL;
    long            iRun;
    CTreePosList &  eruns    = GetList();
    CFlowLayout *   pFlowLayout = eruns.GetFlowLayout( iRunStart );
    CTreeNode *     pNode;
    CElement *      pElementList = NULL;
    CElement *      pElementCommonListParent;
    CTreeNode *     pNodeCommonBlock = FindCommonBlockElement(
                                                    iRunStart, iRunFinish);

    Assert( iRunStart >= GetFirstRun() && iRunFinish <= GetLastRun() );
    Assert( eruns.GetFlowLayout( iRunStart ) == eruns.GetFlowLayout( iRunFinish ) );

    // ApplyList should only be called from ApplyBlockFormat.
    // if the common block element is a list or a list item
    // it should be handled by ApplyBlockFomat.
    // Here it is assumed that scope is extended to include direct
    // scope of the end blocks.
    Assert(!(pNodeCommonBlock->Element()->HasFlag(TAGDESC_LIST) ||
            pNodeCommonBlock->Element()->HasFlag(TAGDESC_LIST)));

    fIsDL = (etagList == ETAG_DD || etagList == ETAG_DT) ? TRUE : FALSE;
    etagListItem = fIsDL ? etagList : ETAG_LI;

    // IsolateRuns before inserting the list
    hr = eruns.IsolateRuns(pNodeCommonBlock->Element(), iRunStart, iRunFinish);
    if(hr)
        goto Cleanup;

    // Find the insertion point
    if(pNodeCommonBlock->Element() == pFlowLayout->Element() ||
       pNodeCommonBlock->Element()->HasFlag(TAGDESC_SPLITBLOCKINLIST) &&
       pNodeCommonBlock->Tag() != ETAG_DIV)
    {
        pNode =
            SearchBranchForChildOfScope( iRunStart, pNodeCommonBlock->Element() );
        pElementCommonListParent = pNodeCommonBlock->Element();
    }
    else
    {
        pNode = pNodeCommonBlock;
        pElementCommonListParent =
            eruns.SearchBranchForBlockElement( pNode->Parent(), pFlowLayout )->Element();
    }

    hr = eruns.CreateAndInsertElement(pNode->SafeElement(), iRunStart, iRunFinish,
                                        fIsDL ? ETAG_DL : etagList,
                                        & pElementList);
    // Release pElementList at the end
    if(hr)
        goto Cleanup;

    // Turn off "break on empty" to fix bug 51655 (nested elements shouldn't
    // have extra breaks between them.
    pElementCommonListParent->_fBreakOnEmpty = FALSE;

    // Now run through all the child block elements and change them
    // appropriately.
    iRun = iRunStart;
    while(iRun <= iRunFinish)
    {
        CElement *  pElementChildBlock = NULL;
        ELEMENT_TAG etagChildBlock;
        long iRunChildEnd;

        pNode = SearchBranchForBlockElement( iRun, pFlowLayout );

        while(DifferentScope(pNode, pElementList))
        {
            pElementChildBlock = pNode->Element();
            pNode = eruns.SearchBranchForBlockElement( pNode->Parent(), pFlowLayout );
        }

        if(pElementChildBlock)
        {
            etagChildBlock = pElementChildBlock->Tag();
            hr = eruns.GetElementScope(iRun, pElementChildBlock, NULL,
                                    &iRunChildEnd);
            if(hr)
                goto Cleanup;

            // Isolate the runs before insertion, replace or delete
            hr = eruns.IsolateRuns(pElementChildBlock,
                                iRun, iRunChildEnd);
            if(hr)
                goto Cleanup;

            if(pElementChildBlock->HasFlag(TAGDESC_LIST))
            {
                // change the list's item to the appropriate list item
                // and delete the list
                hr = eruns.ChangeListItemsType(iRun, pElementChildBlock,
                                                etagListItem);
                if(hr)
                    goto Cleanup;

                hr = eruns.RemoveElement(iRun, pElementChildBlock,
                                            iRunChildEnd, NULL, NULL);
            }
            else if(etagChildBlock == ETAG_P || etagChildBlock == ETAG_PRE ||
                    etagChildBlock == ETAG_DIV ||
                        (etagChildBlock != etagListItem &&
                            pElementChildBlock->HasFlag(TAGDESC_LISTITEM)))
            {
                // replace the child block element with a new list item
                hr = eruns.CreateAndReplaceElement(iRun, pElementChildBlock,
                                                    etagListItem);
            }
            else if(etagChildBlock != etagListItem)
            {
                // headers, address, blockquote and other block elements
                // insert a list item above them
                hr = eruns.CreateAndInsertElement(pElementChildBlock,
                                            iRun, iRunChildEnd,
                                            etagListItem);
            }
            if(hr)
                goto Cleanup;

            iRun = iRunChildEnd + 1;
        }
        else
        {
            hr = eruns.GetDirectScopeOfBlockElement(iRun, pElementList,
                                NULL, &iRunChildEnd);

            // Make sure pElementList is the direct block element for
            // run iRun
            Assert(iRunChildEnd != -1);

            // IsolateRuns before insertion

            hr = eruns.IsolateRuns(pNode->Element(), iRun, iRunChildEnd);
            if(hr)
                goto Cleanup;

            // now check to see if we have anything interesting
            // in the current set of runs (any characters in the runs)
            // then only insert the list item
            if (!IsRangeEmpty(iRun, iRunChildEnd) || iRunChildEnd == GetLastRun())
            {
                // insertion position
                pNode = SearchBranchForChildOfScope( iRun, pElementList );

                // insert the appropriate list item
                hr = eruns.CreateAndInsertElement(pNode->SafeElement(), iRun, iRunChildEnd,
                                                    etagListItem);
                if(hr)
                    goto Cleanup;
            }

            iRun = iRunChildEnd + 1;
        }
    }

    hr = eruns.MergeChildLists(pElementCommonListParent, iRunStart);
    if(hr)
        goto Cleanup;

Cleanup:

    if(pElementList)
        pElementList->Release();

    RRETURN(hr);
}
#endif

#ifdef MERGEFUN // iRuns
HRESULT
CTxtRange::GetEndRuns ( long * piRunStart, long * piRunFinish ) const
{
    CTxtRange range ( * this );

    if (range._cch > 0)
        range.FlipRange();

    if (piRunStart)
        * piRunStart = range.GetIRun();

    if (piRunFinish)
    {
        range.AdvanceCp( - range._cch );

        // Adjust the pointer point to the end of previous run and
        // eat all runs that a empty at the end backwards

        while(*piRunStart < long( range.GetIRun() ) && range.GetIch() == 0)
            range.AdjustBackward();

        * piRunFinish = range.GetIRun();
    }


    Assert( !(piRunStart && piRunFinish) || *piRunStart <= *piRunFinish );
    Assert(!piRunStart  || (*piRunStart  >= GetFirstRun() && *piRunStart  <= GetLastRun()));
    Assert(!piRunFinish || (*piRunFinish >= GetFirstRun() && *piRunFinish <= GetLastRun()));

    return S_OK;
}
#endif

// This function returns the starting and ending run numbers that are affected by
// current selection. Leading and trailing space charactes are ignored if the range
// conatins at least one non space character
// if the starting run pointer is at the end of a run it is moved to the beginning of the
// next run before the run number is returned. If the end pointer is at the beginning of a
// run it is moved to the end of the previous run before the run number is returned.

#ifdef MERGEFUN // iRuns
HRESULT
CTxtRange::GetEndRunsIgnoringSpaces ( long * piRunStart, long * piRunFinish)
{
    long        lStart, lEnd;
    CRchTxtPtr  SelStart(*this);
    CRchTxtPtr  SelEnd(*this);

    Assert(piRunStart != NULL || piRunFinish != NULL);

    // If there is no selection return current run
    if(_cch == 0)
    {
        lStart = lEnd = GetIRun();
        goto Cleanup;
    }

    // Move SelEnd to the end of the selection (or SelStart to the beginning, dependin
    // on the direction of the selection)
    if(_cch < 0)
        SelEnd.Advance(-_cch);
    else
        SelStart.Advance(-_cch);

    // If the selection start and end are in the same run, return it
    if(SelStart.GetIRun() == SelEnd.GetIRun())
    {
        lStart = lEnd = SelStart.GetIRun();
        goto Cleanup;
    }

    // advance the rich text pointer until the first non space character
    // (or end of selection)
    while(SelStart < SelEnd && IsWhiteSpace(SelStart.GetChar()))
    {
        SelStart.Advance(1);
    }


    if(SelStart == SelEnd)  {
        // The range consists of spaces, return the original start run
        CRchTxtPtr  SelOrigStart(*this);
        if(_cch > 0)
            SelOrigStart.Advance(-_cch);
        while( SelOrigStart.GetIch() ==
            long ( GetList().GetRunAbs( SelOrigStart.GetIRun() ).Cch() ) &&
                SelOrigStart.GetIRun() < SelEnd.GetIRun())
        {
            SelOrigStart.AdjustForward();
        }
        lStart = SelOrigStart.GetIRun();
        // Skip over empty runs
        while(SelEnd.GetIch() == 0 && lStart < (long)SelEnd.GetIRun())
            SelEnd.AdjustBackward();
        lEnd = SelEnd.GetIRun();
        goto Cleanup;
    }


    // Skip over empty runs
    while(SelStart.GetIch() == long( GetList().GetRunAbs(SelStart.GetIRun()).Cch() ) && SelStart.GetIRun() < SelEnd.GetIRun())
        SelStart.AdjustForward();

    // Save the adjusted starting run number
    lStart = SelStart.GetIRun();

    // move back the rich text pointer until the first non space character
    while((long)SelEnd.GetIRun() > lStart)
    {
        SelEnd.Advance(-1);
        if(!IsWhiteSpace(SelEnd.GetChar()))
        {
            SelEnd.Advance(1);
            break;
        }
    }


    // If at the beginning of a new run or in an empty run move back
    while(SelEnd.GetIch() == 0 && SelStart.GetIRun() < SelEnd.GetIRun())
        SelEnd.AdjustBackward();

    // Save the last run number
    lEnd = SelEnd.GetIRun();

Cleanup:
    if(piRunStart != NULL) *piRunStart = lStart;
    if(piRunFinish != NULL) *piRunFinish = lEnd;

    Assert(!piRunStart  || (*piRunStart  >= GetFirstRun() && *piRunStart  <= GetLastRun()));
    Assert(!piRunFinish || (*piRunFinish >= GetFirstRun() && *piRunFinish <= GetLastRun()));

    return S_OK;
}
#endif

//+---------------------------------------------------------------
//
//  Member:     CTxtRange::FormatRange
//
//  Synopsis:   Incorporates given element into the tree over the
//              current selection
//
//---------------------------------------------------------------

HRESULT
CTxtRange::FormatRange (
    CElement * pElement,
    BOOL (* pfnUnFormatCriteria) ( CTreeNode * ) )
{
#ifdef MERGEFUN // rtp
    // CWordLikeTxtRange is our magical class that may modify CTxtRange in
    // its constructor and destructor to mimic Microsoft Word's behavior

    CWordLikeTxtRange wltr(this);
    HRESULT hr;

    if(_cch > 0)
        FlipRange();

    Assert( _cch <= 0 );

    //
    // First delete the targe character attribute elements to prevent
    // redundant nested tags
    //

    if (pfnUnFormatCriteria && _cch)
    {
        hr = THR( CRchTxtPtr::UnFormatRange( - _cch,
                                             pfnUnFormatCriteria ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Then go and apply the tags to the range
    //
    hr = CRchTxtPtr::FormatRange( - _cch, pElement );
    if (hr)
        goto Cleanup;

Cleanup:

    return hr;
#else
    RRETURN(E_FAIL);
#endif
}

//+---------------------------------------------------------------
//
//  Member:     CTxtRange::UnFormatRange
//
//  Synopsis:   Incorporates given element into the tree over the
//              current selection
//
//---------------------------------------------------------------

HRESULT
CTxtRange::UnFormatRange (
    BOOL (* pfnUnFormatCriteria) ( CTreeNode * ) )
{
#ifdef MERGEFUN // rtp
    // CWordLikeRange is our magical class that may modify CTxtRange in
    // its constructor and destructor to mimic Microsoft Word's behavior

    CWordLikeTxtRange wltr(this);

    if(_cch > 0)
        FlipRange();

    Assert( _cch <= 0 );

    return CRchTxtPtr::UnFormatRange( - _cch,
                                      pfnUnFormatCriteria );
#else
    RRETURN(E_FAIL);
#endif
}

/*
 *  CTxtRange::CheckTextLength(cch)
 *
 *  @mfunc
 *      Check to see if can add more characters. If not, notify parent
 *
 *  @rdesc
 *      TRUE if OK to add chars
 */
BOOL CTxtRange::CheckTextLength (
    long cch)
{
#ifdef MERGEFUN // rtf
    _TEST_INVARIANT_

    if (GetTextLength() + cch > GetCommonContainer()->HasFlowLayout()->GetMaxLength())
    {
        return FALSE;
    }
#endif
    return TRUE;
}

#ifdef WIN16
#pragma code_seg( "RANGE_2_TEXT" )
#endif // WIN16

/*
 *  CTxtRange::FindParagraph(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = closest paragraph cpMin  <lt>= range cpMin (see comment)
 *      Set *pcpMost = closest paragraph cpMost <gt>= range cpMost
 *
 *  @devnote
 *      If this range's cpMost follows an EOP, use it for bounding-paragraph
 *      cpMost unless 1) the range is an insertion point, and 2) pcpMin and
 *      pcpMost are both nonzero, in which case use the next EOP.  Both out
 *      parameters are nonzero if FindParagraph() is used to expand to full
 *      paragraphs (else StartOf or EndOf is all that's requested).  This
 *      behavior is consistent with the selection/IP UI.  Note that FindEOP
 *      treats the beginning/end of document (BOD/EOD) as a BOP/EOP,
 *      respectively, but IsAfterEOP() does not.
 */
void CTxtRange::FindParagraph (
    long *pcpMin,           // @parm Out parm for bounding-paragraph cpMin
    long *pcpMost) const    // @parm Out parm for bounding-paragraph cpMost
{
#ifdef MERGEFUN // rtp
    long    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    _TEST_INVARIANT_

    GetRange(cpMin, cpMost);
    if(pcpMin)
    {
        tp.SetCp(cpMin);                    // tp points at this range's cpMin
        if(!tp.IsAfterEOP())                // Unless tp directly follows an
            tp.FindEOP(tomBackward);        //  EOP, search backward for EOP
        *pcpMin = cpMin = tp.GetCp();
    }

    if(pcpMost)
    {
        tp.SetCp(cpMost);                   // If range cpMost doesn't follow
        if (!tp.IsAfterEOP() ||             //  an EOP or else if expanding
            (!cpMost || pcpMin) &&
             cpMin == cpMost)               //  IP at paragraph beginning,
        {
            tp.FindEOP(tomForward);         //  search for next EOP
        }
        *pcpMost = tp.GetCp();
    }
#endif
}

/*
 *  CTxtRange::FindSentence(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = closest sentence cpMin  <lt>= range cpMin
 *      Set *pcpMost = closest sentence cpMost <gt>= range cpMost
 *
 *  @devnote
 *      If this range's cpMost follows a sentence end, use it for bounding-
 *      sentence cpMost unless the range is an insertion point, in which case
 *      use the next sentence end.  The routine takes care of aligning on
 *      sentence beginnings in the case of range ends that fall on whitespace
 *      in between sentences.
 */
void CTxtRange::FindSentence (
    long *pcpMin,           // @parm Out parm for bounding-sentence cpMin
    long *pcpMost) const    // @parm Out parm for bounding-sentence cpMost
{
#ifdef MERGEFUN // rtp
    long    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    _TEST_INVARIANT_

    GetRange(cpMin, cpMost);
    if(pcpMin)                              // Find sentence beginning
    {
        tp.SetCp(cpMin);                    // tp points at this range's cpMin
        if(!tp.IsAtBOSentence())            // If not at beginning of sentence
            tp.FindBOSentence(tomBackward); //  search backward for one
        *pcpMin = cpMin = tp.GetCp();
    }

    if(pcpMost)                             // Find sentence end
    {                                       // Point tp at this range's cpLim
        tp.SetCp(cpMost);                   // If cpMost isn't at sentence
        if (!tp.IsAtBOSentence() ||         //  beginning or if at story
            (!cpMost || pcpMin) &&          //  beginning or expanding
             cpMin == cpMost)               //  IP at sentence beginning,
        {                                   //  find next sentence beginning
            if(!tp.FindBOSentence(tomForward))
                tp.SetCp(GetTextLength());  // End of story counts as
        }
        *pcpMost = tp.GetCp();
    }
#endif
}

/*
 *  CTxtRange::FindWord(pcpMin, pcpMost, type)
 *
 *  @mfunc
 *      Set *pcpMin  = closest word cpMin  <lt>= range cpMin
 *      Set *pcpMost = closest word cpMost <gt>= range cpMost
 *
 *  @comm
 *      There are two interesting cases for finding a word.  The first,
 *      (FW_EXACT) finds the exact word, with no extraneous characters.
 *      This is useful for situations like applying formatting to a
 *      word.  The second case, FW_INCLUDE_TRAILING_WHITESPACE does the
 *      obvious thing, namely includes the whitespace up to the next word.
 *      This is useful for the selection double-click semantics and TOM.
 */
void CTxtRange::FindWord(
    long *pcpMin,           //@parm Out parm to receive word's cpMin; NULL OK
    long *pcpMost,          //@parm Out parm to receive word's cpMost; NULL OK
    FINDWORD_TYPE type) const //@parm Type of word to find
{
#ifdef MERGEFUN // rtp
    long    cch;
    long    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    _TEST_INVARIANT_

    Assert(type == FW_EXACT || type == FW_INCLUDE_TRAILING_WHITESPACE );

    GetRange(cpMin, cpMost);
    if(pcpMin)
    {
        tp.SetCp(cpMin);
        if(!tp.IsAtBOWord())                            // cpMin not at BOW:
            cpMin += tp.FindWordBreak(WB_MOVEWORDLEFT); //  go there

        *pcpMin = cpMin;

        Assert(cpMin >= GetFirstCp() && cpMin <= GetLastCp());
    }

    if(pcpMost)
    {
        tp.SetCp(cpMost);
        if (!tp.IsAtBOWord() ||                         // If not at word strt
            (!cpMost || pcpMin) && cpMin == cpMost)     //  or there but need
        {                                               //  to expand IP,
            cch = tp.FindWordBreak(WB_MOVEWORDRIGHT);   //  move to next word

            if(cch && type == FW_EXACT)                 // If moved and want
            {                                           //  word proper, move
                cch += tp.FindWordBreak(WB_LEFTBREAK);  //  back to end of
            }                                           //  preceding word
            if(cch > 0)
                cpMost += cch;
        }
        *pcpMost = cpMost;

        Assert(cpMost >= GetFirstCp() && cpMost <= GetLastCp());
        Assert(cpMin <= cpMost);
    }
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     Expander
//
//  Synopsis:   Helper function to expand a range
//
//  Notes:      Snarfed from richedit code base
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::Expander (
    long Unit, BOOL fExtend,
    long * pDelta, long * pcpMin, long * pcpMost )
{
#ifdef MERGEFUN // rtp
    long    cch = 0;                        // Default no chars added
    long    cchRange;
    long    cpLast = GetLastCp();
    long    cp;
    long    cpMin, cpMost;
    BOOL    fUnitFound = TRUE;              // Most Units can be found
    long    cchCollapse;

    GetRange(cpMin, cpMost);                // Save starting cp's
    if(pcpMin)                              // Default no change
    {
        *pcpMin = cpMin;
        AssertSz(!pcpMost || fExtend,
            "CTxtRange::Expander should extend if both pcpMin and pcpMost != 0");
    }
    if(pcpMost)
        *pcpMost = cpMost;
    if(pDelta)
        *pDelta = 0;

    switch(Unit)                            // Calculate new cp's
    {
#if 0 // BUGBUG: What is the equiv for HTML?
    case tomObject:
        fUnitFound = FindObject(pcpMin, pcpMost);
        break;
#endif

    case htmlUnitCharacter:
        if (pcpMost && cpMin == cpMost &&   // EndOf/Expand insertion point
            cpMost < cpLast &&              //  with at least 1 more char
            (!cpMost || pcpMin))            //  at beginning of story or
        {                                   //  Expand(), then
            (*pcpMost)++;                   //  expand by one char
        }
        break;

    case htmlUnitWord:
        FindWord (pcpMin, pcpMost, FW_INCLUDE_TRAILING_WHITESPACE);
        break;

    case htmlUnitSentence:
        FindSentence (pcpMin, pcpMost);
        break;

#if 0 // BUGBUG: Can/should we do these
    case tomLine:
    {
        CDisplay *pdp;                          //  but tomObject maybe not
        pdp = &GetTxtSite()->_dp;
        if(pdp)                             // If this story has a display
        {                                   //  use line array
            CLinePtr rp(pdp);
            cp = GetCp();
            pdp->WaitForRecalc(cp, -1);
            rp.RpSetCp(cp, FALSE);
            rp.FindRun (pcpMin, pcpMost, cpMin, _cch, cchText);
            break;
        }                                   // Else fall thru to tomPara
    }

    case tomParagraph:
        FindParagraph(pcpMin, pcpMost);
        break;

    case tomWindow:
        fUnitFound = FindVisibleRange(pcpMin, pcpMost);
        break;
#endif

    case htmlUnitTextEdit:
        if(pcpMin)
            *pcpMin = GetFirstCp();
        if(pcpMost)
            *pcpMost = cpLast;
        break;

    default:
        return E_INVALIDARG;
    }

    if(!fUnitFound)
        goto Cleanup;

    cchCollapse = !fExtend && _cch;         // Collapse counts as a char
                                            // Note: Expand() has fExtend = 0
    if(pcpMin)
    {
        cch = cpMin - *pcpMin;              // Default positive cch for Expand
        cpMin = *pcpMin;
    }

    if(pcpMost)                             // EndOf() and Expand()
    {
        if(!fExtend)                        // Will be IP if not already
        {
            if(cpMost > cpLast)             // If we collapse (EndOf only),
                cchCollapse = -cchCollapse; //  it'll be before the final CR
            else
                *pcpMost = min(*pcpMost, cpLast);
        }
        cch += *pcpMost - cpMost;
        cp = cpMost = *pcpMost;
    }
    else                                    // StartOf()
    {
        cch = -cch;                         // Invert count
        cp = cpMin;                         // Active end at cpMin
        cchCollapse = -cchCollapse;         // Backward collapses count as -1
    }

    cch += cchCollapse;                     // Collapse counts as a char
    if(cch)                                 // One or both ends changed
    {
        cchRange = cpMost - cpMin;          // cch for EndOf() and Expand()
        if(!pcpMost)                        // Make negative for StartOf()
            cchRange = -cchRange;
        if(!fExtend)                        // We're not expanding (EndOf()
            cchRange = 0;                   //  or StartOf() call)
        if(Set(cp, cchRange))               // Set active end and signed cch
        {                                   // Something changed
            if(pDelta)                      // Report cch if caller cares
                *pDelta = cch;
            return NOERROR;
        }
    }

Cleanup:
    return S_FALSE;                         // Report Unit found but no change
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     Mover
//
//  Synopsis:   Helper function to move a range
//
//  Notes:      Snarfed from richedit code base
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::Mover (
    long Unit, long Count, long * pDelta, MOVES Mode )
{
#ifdef MERGEFUN // rtp
    long      cch;
    long      cpLast = GetLastCp();
    long      cchMax = 0;                       // Default full story limits
    long      cp;
    long      cpMost = GetCpMost();
    long      cUnitCollapse = 0;
    HRESULT   hr = NOERROR;
    CTxtRange rg(*this);                        // Use a copy to look around

    if(pDelta)
        *pDelta = 0;                            // Default no units moved

    if(_cch && Count)                           // Nondegenerate range
    {
        if(Mode == MOVE_IP)                     // Insertion point: will
        {                                       //  collapse range if Unit is
            if((Count ^ rg._cch) < 0)           //  defined. Go to correct end
                rg.FlipRange();
            if(Count > 0)
            {
                if(cpMost > cpLast)
                {
                    cUnitCollapse = -1;         // Collapse before final CR
                    Count = 0;                  // No more motion
                }
                else
                {   //               Extend pDelta pcpMin pcpMost
                    hr = rg.Expander(Unit, FALSE, NULL, NULL, &cp);
                    cUnitCollapse = 1;          // Collapse counts as a Unit
                    Count--;                    // One less Unit to count
                }
            }
            else
            {
                hr = rg.Expander(Unit, FALSE, NULL, &cp, NULL);
                cUnitCollapse = -1;
                Count++;
            }
            if(FAILED(hr))
                return hr;
        }
        else if((Mode ^ rg._cch) < 0)           // MOVE_START or MOVE_END
            rg.FlipRange();                     // Go to Start or End
    }

    if(Count > 0 && Mode != MOVE_END)           // Moving IP or Start forward
    {
        cchMax = cpLast - rg.GetCp();           // Can't pass final CR
        if(cchMax <= 0)                         // Already at or past it
        {                                       // Only count comes from
            Count = cUnitCollapse;              //  possible collapse
            cp = cpLast;                        // Put active end at cchText
            cch = (Mode == MOVE_START && cpMost > cpLast)
                ? cp - cpMost : 0;
            goto set;
        }
    }

    cch = rg.UnitCounter(Unit, Count, cchMax);  // Count off Count Units

    if(cch == tomForward)                       // Unit not implemented
        return E_NOTIMPL;

    if(cch == tomBackward)                      // Unit not available, e.g.,
        return S_FALSE;                         //  tomObject and no objects

    Count += cUnitCollapse;                     // Add a Unit if collapse
    if(!Count)                                  // Nothing changed, so quit
        return S_FALSE;

    if (Mode == MOVE_IP ||                      // MOVE_IP or
        !_cch && (Mode ^ Count) < 0)            //  initial IP end cross
    {
        cch = 0;                                // New range is degenerate
    }
    else if(_cch)                               // MOVE_START or MOVE_END
    {                                           //  with nondegenerate range
        if((_cch ^ Mode) < 0)                   // Make _cch correspond to end
            _cch = -_cch;                       //  that moved
        cch += _cch;                            // Possible new range length
        if((cch ^ _cch) < 0)                    // Nondegenerate end cross
            cch = 0;                            // Use IP
    }
    cp = rg.GetCp();

set:
    if(Set(cp, cch))                            // Attempt to set new range
    {                                           // Something changed
        if(pDelta)                              // Report count of units
            *pDelta = Count;                    //  advanced
        Update(TRUE, FALSE);                           // Update selection
        return NOERROR;
    }
    return S_FALSE;
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     Finder
//
//  Synopsis:   Helper function to find text
//
//  Notes:      Snarfed from richedit code base
//
//-----------------------------------------------------------------------------

HRESULT CTxtRange::Finder (
    BSTR bstr, long Count, long Flags, long * pDelta, MOVES Mode )
{
#ifdef MERGEFUN // rtp
    if(!bstr)
        return S_FALSE;

#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr    callmgr(GetPed());
#endif

    long        cpMin, cpMost;
    long        cch = GetRange(cpMin, cpMost);  // Get this range's cp's
    long        cchBstr = SysStringLen(bstr);
    long        cchSave = _cch;
    long        cp, cpMatch, cpSave;
    long        cpStart = cpMost;               // Default Start cp to range
    CRchTxtPtr  rtp(*this);                     //  End

    if(Mode == MOVE_IP)                         // FindText(): Count = 0 is
    {                                           //  treated specially: if IP,
        if(!Count)                              //  compare string at IP; else
            Count = cch ? cch : cchBstr;        //  confine search to range
        if(Count > 0)                           // Forward searches start from
            cpStart = cpMin;                    //  beginning of range
    }
    else                                        // FindTextStart() or
    {                                           //   FindTextEnd()
        if(!Count)                              // Compare string at IP; else
            Count = cch ? -Mode*cch : cchBstr;  //  confine search to range
        if(Count < 0)                           // Find from Start
            cpStart = cpMin;
    }

    cpSave = cpStart;                           // Save starting cp
    cp = cpStart + Count;                       // cp = limiting cp. Can be on
    cp = max(cp, 0L);                           //  either side of cpStart

    if(Count >= 0)                              // It's forward, so make sure
        Flags &= ~FINDTEXT_BACKWARDS;           // backwards is off.
    else
        Flags |= FINDTEXT_BACKWARDS;

    rtp.SetCp(cpStart);                         // Move to start of search

#ifdef MACPORT
    CStrInW  strinw(bstr);
    if( tp.FindText( cp, Flags, strinw, cchBstr ) >= 0 )
    {
        cpMatch = tp._cp;
    }
    WideCharToMultiByte(CP_ACP, 0, strinw, wcslen(strinw), bstr,
                        wcslen(strinw)+1, NULL, NULL);
#else
    if( tp.FindText( cp, Flags, bstr, cchBstr ) >= 0)
    {
        cpMatch = tp._cp;
    }
#endif

    if(cpMatch < 0)                             // Match failed
    {
        if(pDelta)                              // Return match string length
            *pDelta = 0;                        //  = 0
        return S_FALSE;                         // Signal no match
    }

    // Match succeeded: set new cp and cch for range, update selection (if
    // this range is a selection), send notifications, and return NOERROR

    // BUGBUG (t-johnh): This needs to be re-written to modified FindText,
    // which now returns the cp of the match end and sets the pointer to
    // the match beginning.  Also note that cchBstr is not necessarily
    // the length of the match in the document (complex text stuff).

    cp = rtp.GetCp();                           // cp = cpMost of match string
    if(pDelta)                                  // Return match string length
        *pDelta = cchBstr;                      //  if caller wants to know

    cch = cp - cpMatch;                         // Default to select matched
                                                //  string (for MOVE_IP)
    if(Mode != MOVE_IP)                         // MOVE_START or MOVE_END
    {
        if(Mode == MOVE_START)                  // MOVE_START moves to start
            cp = cpMatch;                       //  of matched string
        cch = cp - cpSave;                      // Distance end moved
        if(!cchSave && (Mode ^ cch) < 0)        // If crossed ends of initial
            cch = 0;                            //  IP, use an IP
        else if(cchSave)                        // Initially nondegenerate
        {                                       //  range
            if((cchSave ^ Mode) < 0)            // If wrong end is active,
                cchSave = -cchSave;             //  fake a FlipRange to get
            cch += cchSave;                     //  new length
            if((cch ^ cchSave) < 0)             // If ends would cross,
                cch = 0;                        //  convert to insertion point
        }
    }
    if ((cp != long(GetCp()) || cch != _cch)    // Active end and/or length of
        && Set(cp, cch))                        //  range changed
    {                                           // Use the new values
        Update(TRUE, FALSE);                           // Update selection
    }
    return NOERROR;
#else
    RRETURN(E_FAIL);
#endif
}

/*
 *  CTxtRange::GetExtendedSelectionInfo ( long * piRunStart,
 *                                        long * piRunFinish )
 *
 *  Returns information about the extended selection.  The extended selection
 *  is the current selection plus the runs under the influence of the lowest
 *  block elements.  Certain operations (apply style, remove character format-
 *  ting) are performed on the extended selection.
 *
 */
#ifdef NEVER
HRESULT
CTxtRange::GetExtendedSelectionInfo (
    long * piRunStart,
    long * piRunFinish,
    CElement * (CElement::*pfnSearchCriteria) ( CTxtSite * ) )
{
    CElement *  pElementBlock;
    int         iDirection;
    long        iRunStart = GetFirstRun(), iRunFinish = GetFirstRun(), iRunMax;

    Assert( piRunStart && piRunFinish );

    AdvanceToNonEmpty();

    iDirection = (GetCch() > 0) ? 1 : -1;

    // loop twice; once to go left, once to go right.

    for ( int i=0 ; i < 2; i++ )
    {
        pElementBlock = (GetElementAbs( GetIRun() ).*pfnSearchCriteria)( GetPed() );
        Assert(pElementBlock);

        if (iDirection < 0)
        {
            // traverse to the left of the selection

            long    iRunFirst = GetFirstRun();
            for ( iRunStart = GetIRun(); iRunStart > iRunFirst; iRunStart-- )
            {
                // BUGBUG: FLATRUN - This should be restricted to a single CTxtSite (brendand)
                if (pElementBlock !=
                        SearchBranchForBlockElement( iRunStart - 1 ))
                {
                    break;
                }
            }
        }
        else
        {
            // traverse to the right of the selection

            iRunMax = GetLastRun();
            for ( iRunFinish = GetIRun(); iRunFinish < iRunMax; iRunFinish++ )
            {
                // BUGBUG: FLATRUN - This should be restricted to a single CTxtSite (brendand)
                if (pElementBlock !=
                        SearchBranchForBlockElement( iRunFinish + 1 ))
                {
                    break;
                }
            }
        }

        // setup variables to work in opposite direction;

        if (!i)
        {
            FlipRange();
            iDirection = -iDirection;
        }
    }

    *piRunStart = iRunStart;
    *piRunFinish = iRunFinish;

    Assert( *piRunStart  >= GetFirstRun() && *piRunStart  <= GetLastRun() );
    Assert( *piRunFinish >= GetFirstRun() && *piRunFinish <= GetLastRun() );

    return S_OK;
}
#endif


//
// BUGBUG : (johnbed) RaminH will move this logic up to the CAutoRange 
//

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::SaveHTMLToStream
//
//  Synopsis:   Saves the range text to the specified stream.
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::SaveHTMLToStream(CStreamWriteBuff * pswb, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    CLING_RESULT cr = CR_Failed;

    CMarkupPointer mpLeft( _pMarkup->Doc() );
    CMarkupPointer mpRight( _pMarkup->Doc() );
    
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
        CRangeSaver rs( &mpLeft, &mpRight, dwFlags, pswb, _pMarkup );
        hr = THR( rs.Save());
    }
    
Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetBlockFormat
//
//  Synopsis:   Get the block format of a given selection
//
//  Arguments:  BOOL        -   fCheckForList, to check for a list up in the
//                              tree for a selection
//
//  Returns:    ELEMENT_TAG -   tag of block format common for the given
//                              selection
//
//----------------------------------------------------------------------------

ELEMENT_TAG
CTxtRange::GetBlockFormat(BOOL fCheckForList)
{
#ifdef MERGEFUN // iRuns
    CTreeNode * pNodeCommonBlock;
    ELEMENT_TAG etagCBlock;
    long        iFirstRangeRun;
    long        iLastRangeRun;

    GetEndRuns(&iFirstRangeRun, &iLastRangeRun);

    Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
    Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );

    // Find the common block element and check the to see if it is in the
    // drop down combo
    pNodeCommonBlock = FindCommonBlockElement(iFirstRangeRun, iLastRangeRun);
    Assert(pNodeCommonBlock);
    etagCBlock = pNodeCommonBlock->Tag();

    if(pNodeCommonBlock->Element()->HasFlag(TAGDESC_LIST))
    {
        // if it is a list then return the list tag(DL is also returned but
        // ignored by CTextSelectionRecordRecord::Exec
        return etagCBlock;
    }
    else if (pNodeCommonBlock->Element()->HasFlag(TAGDESC_BLKSTYLEDD) ||
                etagCBlock == ETAG_LI || etagCBlock == ETAG_DIV ||
                etagCBlock == ETAG_FIELDSET)
    {
        // if it is in block format style drop down combo or if it is a
        // list item
        if(fCheckForList || etagCBlock == ETAG_LI || etagCBlock == ETAG_DIV ||
                etagCBlock == ETAG_FIELDSET)
        {
            // if we are checkin for lists, search up the branch for a
            // list container and return it's tag if found

            CElement * pElementListContainer =
                GetList().FindMyListContainer( pNodeCommonBlock )->SafeElement();

            if(pElementListContainer)
                return pElementListContainer->Tag();
        }
        return etagCBlock;
    }
    else
    {
        // if the common block element is body or an item not in the
        // block style drop down combo
        long        iRun;
        CTreeNode * pNode;
        BOOL        fFirstBranch = TRUE;
        ELEMENT_TAG etagCurr;
        ELEMENT_TAG etagRange = (ELEMENT_TAG)0;

        // run through all the runs find the lowest common block in the
        // block style drop down combo and compare it with the previous
        // lowest block style, if same continue till the last run otherwise
        // return ETAG_UNKNOWN
        for (iRun = iFirstRangeRun; iRun <= iLastRangeRun; iRun++)
        {
            etagCurr = ETAG_UNKNOWN;
            // BUGBUG: FLATRUN - This should be restricted to a single CTxtSite (brendand)
            pNode = SearchBranchForBlockElement( iRun );

            while(pNode)
            {
                if(pNode->HasFlowLayout())
                {
                    // plaintext pointing to body is considered normal.
                    etagCurr = GetDefaultBlockTag();
                    pNode = NULL;
                }
                else if(pNode->Element()->HasFlag(TAGDESC_BLKSTYLEDD))
                {
                    // if the block element style is in the drop down
                    etagCurr = pNode->Tag();
                    pNode = NULL;
                }
                else
                {
                // BUGBUG: FLATRUN - This should be restricted to a single CTxtSite (brendand)
                    pNode =
                        GetList().SearchBranchForBlockElement(
                            pNode->Parent() );
                }
            }
            if(!fFirstBranch)
            {
                // if the current block is different fron the range
                // test till now
                if(etagRange != etagCurr)
                    return ETAG_UNKNOWN;
            }
            else
            {
                fFirstBranch = FALSE;
                etagRange = etagCurr;
            }
        }
        return etagRange;
    }
#else
    return ETAG_NULL;
#endif
}

//+---------------------------------------------------------------------------
//
// Member:      CTxtRange::ApplyBlockFormat
//
// Synopsis:    Applies a given block format style over the current range
//
// Arguments:   ELEMENT_TAG -   tag to be applied
//
// Returns:     S_OK, if successful
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::ApplyBlockFormat(ELEMENT_TAG etag)
{
#ifdef MERGEFUN // rtp
    HRESULT         hr = S_OK;
    long            iRun,
                    iRunStart,
                    iRunFinish,
                    iLastRangeRun,
                    iFirstRangeRun;
    CTreePosList &  eruns = GetList();
    CFlowLayout *   pFlowLayout;
    CRangeRestorer  rr( this );
    CPedUndo        pu( GetPed() );
    CElement *      pContainer = GetCommonContainer();

    pu.Start( IDS_UNDOGENERICTEXT, this, pContainer->IsEditable() );

    hr = THR( rr.Set() );

    if (hr)
        goto Cleanup;

    hr = THR(GetEndRuns(&iFirstRangeRun, &iLastRangeRun));
    if(hr)
        goto Cleanup;

    Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
    Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );
    pFlowLayout = eruns.GetFlowLayout( iFirstRangeRun );

    Assert( pFlowLayout == eruns.GetFlowLayout( iLastRangeRun ) );

    // BUGBUG (cthrash) This is overkill.

    if ( pFlowLayout->GetDisplay()->Instantiated() &&
         ( etag == ETAG_OL   || etag == ETAG_UL ||
           etag == ETAG_MENU || etag == ETAG_DIR ) )
    {
        pFlowLayout->GetDisplay()->ClearListIndexCache();
    }

    // if the current selection, selects the first and the last outermost
    // block partially, then update the scope extents
    hr = THR(eruns.ExtendScopeToEndBlocksDirectScope(NULL,
                                    &iFirstRangeRun, &iLastRangeRun));
    if(hr)
        goto Cleanup;

    iRun = iFirstRangeRun;

    // address is treated as headers
    if( etag == ETAG_ADDRESS ||                 // apply address
        etag == ETAG_P || etag == ETAG_DIV ||
        etag == ETAG_PRE ||   // apply paragraph(normal) or PRE
        (etag >= ETAG_H1 && etag <= ETAG_H6))   // apply header
    {
        CElement * pElementBlockNukeBelowMe;
        while(iRun <= iLastRangeRun)
        {
            CTreeNode * pNodeBlock = SearchBranchForBlockElement( iRun, pFlowLayout );
            pElementBlockNukeBelowMe = NULL;

            hr = THR(eruns.GetDirectScopeOfBlockElement(iRun, pNodeBlock->Element(),
                                                    &iRunStart, &iRunFinish));

            Assert( iRunStart != -1 && iRunFinish != -1 );
            Assert( iRunStart  >= GetFirstRun() && iRunStart  <= GetLastRun() );
            Assert( iRunFinish >= GetFirstRun() && iRunFinish <= GetLastRun() );

            if(hr)
                goto Cleanup;

            if(pNodeBlock->Element()->HasFlag(TAGDESC_LIST) ||
               pNodeBlock->Element()->HasFlag(TAGDESC_LISTITEM) ||
               pNodeBlock->Tag() == ETAG_BLOCKQUOTE)
            {
                if(etag == GetDefaultBlockTag())  // do nothing
                {
                    iRun = iRunFinish + 1;
                    continue;
                }

                // it is a list/listitem/blockquote
                // so insert a new element of the given tag below it
                hr = THR(eruns.IsolateRuns(pNodeBlock->Element(),
                                    iRunStart, iRunFinish));
                if(hr)
                    goto Cleanup;

                CElement * pElementInsert =
                    SearchBranchForChildOfScope( iRun, pNodeBlock->Element() )->SafeElement();

                hr = THR(eruns.CreateAndInsertElement(pElementInsert,
                                            iRunStart, iRunFinish, etag,
                                            &pElementBlockNukeBelowMe));
                if(hr)
                    goto Cleanup;

                // DT cannot contain any block element so turn it into a DD
                if(pNodeBlock->Tag() == ETAG_DT)
                {
                    hr = THR(eruns.CreateAndReplaceElement(iRunStart,
                                    pNodeBlock->Element(), ETAG_DD));
                    if(hr)
                        goto Cleanup;
                }
            }
            else if (pNodeBlock->HasFlowLayout())
            {
                if (!IsRangeEmpty(iRunStart, iRunFinish) ||
                        iRunFinish == GetLastRun())
                {

                    // plaintext pointing to body.
                    // Insert the selected style
                    CElement * pElementInsert =
                        SearchBranchForChildOfScope( iRun, pNodeBlock->Element() )->SafeElement();

                    hr = THR(eruns.CreateAndInsertElement(pElementInsert,
                                                iRunStart, iRunFinish, etag,
                                                &pElementBlockNukeBelowMe));
                    if(hr)
                        goto Cleanup;
                }
            }
            else
            {
                // it is a P/PRE/H1 - H6/ADDRESS/XMP/CENTER
                // so replace the block element with a new one of the
                // given tag
                if(etag != pNodeBlock->Tag())
                {
                    // BUGBUG (srinib) - cleanup the tree here, remove any
                    // nested header's, paragraph's or address blocks
                    hr = THR(eruns.IsolateRuns(pNodeBlock->Element(),
                                        iRunStart, iRunFinish));
                    if(hr)
                        goto Cleanup;

                    if(etag == GetDefaultBlockTag())
                    {
                        // check to see if the current block is nested in
                        // another block with the same scope. If so delete
                        // the current block, otherwise replace it with
                        // the default block tag.

                        CElement * pElementParentBlock =
                            eruns.SearchBranchForBlockElement(
                                pNodeBlock->Parent(), pFlowLayout )->Element();

                        if(!pElementParentBlock->HasFlowLayout() &&
                            eruns.IsCompleteScopeOfElement(pElementParentBlock,
                                    iRunStart, iRunFinish))
                        {
                            hr = THR(eruns.RemoveElement(iRunStart,
                                                pNodeBlock->Element(), iRunFinish,
                                                NULL, NULL ));
                            pElementBlockNukeBelowMe = pElementParentBlock;
                            pElementBlockNukeBelowMe->AddRef();
                        }
                        else
                        {
                            hr = THR(eruns.CreateAndReplaceElement(iRun,
                                                pNodeBlock->Element(), etag,
                                                &pElementBlockNukeBelowMe));
                        }
                    }
                    else
                    {
                        hr = THR(eruns.CreateAndReplaceElement(iRun,
                                        pNodeBlock->Element(), etag,
                                        &pElementBlockNukeBelowMe));
                    }
                    if(hr)
                        goto Cleanup;
                }
                // This will clear formatting from a
                // block element even if you haven't changed
                // the block element.
                else
                {
                    pElementBlockNukeBelowMe = pNodeBlock->Element();
                    pElementBlockNukeBelowMe->AddRef();
                }
            }
            if (pElementBlockNukeBelowMe)
            {
                hr = THR(NukePhraseElementsUnderBlockElement(iRun, iRunFinish,
                                    pElementBlockNukeBelowMe));
                if (hr)
                    goto Cleanup;
                pElementBlockNukeBelowMe->Release();
            }
            iRun = iRunFinish + 1;
        }
    }
    //else if (etag == ETAG_BLOCKQUOTE)   // apply blockquote
    //{
    //}
    else    // apply lists
    {
        CElement * pElementCBParentBlock;
        CTreeNode *pNodeCommonBlock =
                       FindCommonBlockElement(iFirstRangeRun, iLastRangeRun);

        Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
        Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );
        pFlowLayout = eruns.GetFlowLayout( iFirstRangeRun );

        Assert( pFlowLayout == eruns.GetFlowLayout( iLastRangeRun ) );

        // srinib - Fix for bug #231
        // Run up the tree and look for a list container, if there is
        // a list container then it is the common block element.
        if (!(pNodeCommonBlock->Element()->HasFlag(TAGDESC_LIST) ||
              pNodeCommonBlock->Element()->HasFlag(TAGDESC_LISTITEM)))
        {
            CTreeNode * pNodeListContainer =
                GetList().FindMyListContainer( pNodeCommonBlock );

            // found a list containing common block
            if(pNodeListContainer)
            {
                pNodeCommonBlock = pNodeListContainer;
#ifdef NEVER
                while(pElementCommonBlock->Scope() !=
                                            pElementListContainer->Scope())
                {
                    // BUGBUG: FLATRUN - This should be restricted to a single CTxtSite (brendand)
                    CElement *  pElementTemp =
                                    pElementCommonBlock->EParent()->
                                                SearchBranchForBlockElement(pTxtSite);
                    if(eruns.IsCompleteScopeOfElement(pElementTemp,
                                            iFirstRangeRun, iLastRangeRun))
                    {
                        pElementCommonBlock = pElementTemp;
                    }
                    else
                        break;
                }
#endif
            }
        }

        if (pNodeCommonBlock->Element()->HasFlag(TAGDESC_LIST) ||
             pNodeCommonBlock->Element()->HasFlag(TAGDESC_LISTITEM))
        {
            // if the common block selected is a listitem or list then
            // convert the list type to the new list type
            if(pNodeCommonBlock->Element()->HasFlag(TAGDESC_LISTITEM))
            {
                // find the list container, if a list container is not
                // found, then insert the appropriate list and convert
                // the list item to the new list style.
                CTreeNode * pNodeListItemContainer =
                    GetList().FindMyListContainer( pNodeCommonBlock );

                if(pNodeListItemContainer)
                {
                    pNodeCommonBlock = pNodeListItemContainer;
                }
                else
                {
                    CElement* pElementTemp;

                    hr = THR(eruns.GetElementScope(iRun, pNodeCommonBlock->Element(),
                                   &iFirstRangeRun, &iLastRangeRun));
                    if(hr)
                        goto Cleanup;

                    Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
                    Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );

                    hr = THR(eruns.IsolateRuns(pNodeCommonBlock->Element(),
                                           iFirstRangeRun, iLastRangeRun));
                    if(hr)
                        goto Cleanup;
                     // Insert a new list container
                    hr = THR(eruns.CreateAndInsertElement(pNodeCommonBlock->Element(),
                                   iFirstRangeRun, iLastRangeRun,
                                   (etag == ETAG_DD || etag == ETAG_DT) ?
                                       ETAG_DL : etag, &pElementTemp));
                    if(hr)
                        goto Cleanup;
                    if(pElementTemp)
                        pElementTemp->Release();
                    pNodeCommonBlock = eruns.SearchBranchForScopeInStory(iFirstRangeRun, pElementTemp);
                }
            }
            if(pNodeCommonBlock->Tag() != ETAG_DL ||
                (!(etag == ETAG_DT || etag == ETAG_DD) &&
                    pNodeCommonBlock->Tag() == ETAG_DL))
            {
                // extend the range to the scope of the list.
                hr = THR(eruns.GetElementScope(iRun, pNodeCommonBlock->Element(),
                                    &iFirstRangeRun, &iLastRangeRun));
                if(hr)
                    goto Cleanup;

                Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
                Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );
            }

            hr = THR(eruns.IsolateRuns(pNodeCommonBlock->Element(),
                                    iFirstRangeRun, iLastRangeRun));
            if(hr)
                 goto Cleanup;

            // Run through all the children and change the
            // listitem's type to the new list style
            hr = THR(eruns.ChangeListItemsType(iFirstRangeRun, pNodeCommonBlock->Element(),
                        (etag == ETAG_DT || etag == ETAG_DD) ? etag : ETAG_LI,
                        iLastRangeRun));
            if(hr)
                goto Cleanup;

            pElementCBParentBlock =
                eruns.SearchBranchForBlockElement(
                    pNodeCommonBlock->Parent(), pFlowLayout )->Element();

            if(pNodeCommonBlock->Tag() !=
                    ((etag == ETAG_DT || etag == ETAG_DD) ? ETAG_DL : etag))
            {
                // replace the list
                hr = THR(eruns.CreateAndReplaceElement(iFirstRangeRun,
                            pNodeCommonBlock->Element(),
                            (etag == ETAG_DT || etag == ETAG_DD) ?
                            ETAG_DL : etag));
                if(hr)
                    goto Cleanup;
            }
            hr = THR(eruns.MergeChildLists(pElementCBParentBlock,
                                    iFirstRangeRun));
            if(hr)
                goto Cleanup;
        }
        else
        {
             ApplyList(etag, iRun, iLastRangeRun);
        }
    }

    rr.Restore();

Cleanup:
    pu.Finish( hr );

    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::OutdentSelection
//
//  Synopsis:   outdent's a selected range of text by removing a BLOCKQUOTE
//              or a list appropriately
//
//  Arguments:  BOOL - fRemoveListOnly, this flag is true when the user is
//              is trying to remove the selection from the influence of a list
//              using the bullet/numbered button on the toolbar
//
//  Returns:    HRESULT             S_OK if succesfull or an error
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::OutdentSelection(BOOL fRemoveListsOnly)
{
#ifdef MERGEFUN // rtp
    HRESULT         hr = S_OK;
    long            iRun,
                    iRunStart,
                    iRunFinish,
                    iLastRangeRun,
                    iFirstRangeRun;
    CTreePosList &  eruns = GetList();
    CFlowLayout *   pFlowLayout;
    CNotification   nf;
    CPedUndo        pu( GetPed() );
    ELEMENT_TAG     etagReplace = GetDefaultBlockTag();
    CElement *      pContainer = GetCommonContainer();

    pu.Start( IDS_UNDOALIGN, this, pContainer->IsEditable() );

    pu.MarkForClearRunCaches();

    // Find the scope of the selection in terms of the runs
    GetEndRuns(&iFirstRangeRun, &iLastRangeRun);

    Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
    Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );

    pFlowLayout = eruns.GetFlowLayout( iFirstRangeRun );

    Assert( pFlowLayout == eruns.GetFlowLayout( iLastRangeRun ) );

    // if the current selection, selects the first and the last outermost
    // block partially, then update the scope extents
    hr = THR(eruns.ExtendScopeToEndBlocksDirectScope(NULL,
                                    &iFirstRangeRun, &iLastRangeRun));
    if(hr)
        goto Cleanup;

    iRun = iRunStart = iRunFinish = iFirstRangeRun;

    // Following loop try's to find the top most list or blockquote that
    // indents and removes it's influence. Ex. If the given range spans from
    // run 1 - 10, and a ul spans from 1-5, and a blockquote spans from 8-9.
    // The scope of ul and then blockquote are removed on each iteration

    while(iRun <= iLastRangeRun)
    {
        CTreeNode * pNodeRemoveAlternate = NULL;
        CTreeNode * pNodeRemove = NULL;
        CElement *  pElementAvoidNesting = NULL;
        CElement *  pElementCBParentBlock;

        // Search the current branch for a block element
        CTreeNode * pNode = SearchBranchForBlockElement( iRun, pFlowLayout );
        Assert(pNode);

        // Find the topmost block  whose scope is completely
        // in the given range that causes indent.
        while(!pNode->HasFlowLayout())
        {
            // if the current block is completely in scope or we didn't find
            // an element to remove yet, search up in the tree for an element
            // that causes the indent.
            if(!eruns.IsCompleteScopeOfElement(pNode->Element(), iRun, iLastRangeRun)
                        && pNodeRemove)
            {
                break;
            }

            // If the current block element is a list item or a blocquote
            // and we are not trying to remove just lists, save this element as
            // the element to be removed.
            if(pNode->Element()->HasFlag(TAGDESC_LIST) ||
                (!fRemoveListsOnly && pNode->Tag() == ETAG_BLOCKQUOTE))
            {
                pNodeRemove = pNode;
            }

            if(pNode->Element()->HasFlag(TAGDESC_LISTITEM))
            {
                pNodeRemoveAlternate = pNode;
            }

            // We may find another element that indents up in the branch,
            // that is completely selected.
            pNode = eruns.SearchBranchForBlockElement( pNode->Parent(), pFlowLayout );
        }

        if(!pNodeRemove)
            pNodeRemove = pNodeRemoveAlternate;

        // If we didn't find any element to remove, skip to the next branch.
        if(!pNodeRemove)
        {
            iRun++;
            continue;
        }

        // now check to see if other block children of parent are also fully
        // in the selected range.
        long    iLastRemoveRun;

        // For merging lists, cache the parent of the element being removed.
        pElementCBParentBlock =
            eruns.SearchBranchForBlockElement( pNodeRemove->Parent(), pFlowLayout )->Element();

        // Might want to replace with the same type of element as the
        // parent of the list. This will avoid nesting of block elements,
        // something to be avoided for the sake of simplicity.
        if (!pElementCBParentBlock->HasLayout() &&
            !IsList(pElementCBParentBlock) &&
            !IsListItem(pElementCBParentBlock) &&
            pElementCBParentBlock->Tag() != ETAG_BLOCKQUOTE)
        {
            pElementAvoidNesting = pElementCBParentBlock;
            etagReplace = pElementAvoidNesting->Tag();
        }

        // Find the scope of the element being removed
        hr = THR(eruns.GetElementScope(iRun, pNodeRemove->Element(),
                                        NULL, &iLastRemoveRun));
        if(hr)
            goto Cleanup;

        iRunStart = iRun;

        if(iLastRemoveRun <= iLastRangeRun)
        {
            // we can remove all children after the current child
            iRunFinish = iLastRemoveRun;
        }
        else
        {
            // parent is partialy selected, so search for a continuous scope
            // of children with their scope in the selected range.
            hr = THR(eruns.FindEndRunOfCompletelySelectedChildren(pNodeRemove->Element(),
                                                iRun, iLastRangeRun, &iRunFinish));
        }

        // If we are trying to remove a list, then we should convert all the
        // list items in the current range to paragraph's or different list items
        // appropriately based on the parent
        if (pNodeRemove->Element()->HasFlag(TAGDESC_LIST))
        {
            long iRunTemp = iRun;
            CElement * pElementCListItem = NULL;
            CElement * pElementListContainer =
                GetList().FindMyListContainer( pNodeRemove )->SafeElement();

            // check to see if there is a BLOCKQUOTE found before encountering
            // the list's container or it's list item, if so delete it
            CTreeNode * pNodeTemp = pNodeRemove;

            if(!fRemoveListsOnly)
            {
                do
                {
                    pNodeTemp =
                        eruns.SearchBranchForBlockElement(
                            pNodeTemp->Parent(), pFlowLayout );

                    if(pNodeTemp->Tag() == ETAG_BLOCKQUOTE)
                    {
                        hr = THR(eruns.IsolateRuns(pNodeTemp->Element(),
                                                    iRunStart, iRunFinish));
                        if(hr)
                            goto Cleanup;

                        pElementCBParentBlock =
                            eruns.SearchBranchForBlockElement(pNodeTemp->Parent(), pFlowLayout )->SafeElement();

                        hr = THR(eruns.RemoveElement(iRunStart, pNodeTemp->Element(),
                                            iRunFinish, NULL, NULL));
                        if(hr)
                            goto Cleanup;
                        goto NextIteration;
                    }
                } while (!pNodeTemp->HasFlowLayout() &&
                        !(pNodeTemp->Element()->HasFlag(TAGDESC_LIST) ||
                          pNodeTemp->Element()->HasFlag(TAGDESC_LISTITEM)));
            }

            if(pElementListContainer)
            {
                // found list's container, change all the list's child
                // listitem type to that of the list's container.
                // Before that find if there is any listitem between the
                // list and the list's container, if found remove the
                // influence of the list container's list item on the list
                CTreeNode * pNodeTemp = pNodeRemove->Parent();

                while(DifferentScope(pNodeTemp, pElementListContainer))
                {
                    if(pNodeTemp->Element()->HasFlag(TAGDESC_LISTITEM))
                    {
                        pElementCListItem = pNodeTemp->Element();
                        break;
                    }
                    else
                        pNodeTemp = pNodeTemp->Parent();
                }

                if(pElementListContainer->Tag() == ETAG_DL)
                {
                    // Srinib - Bug fix for #222 (some how instead of checking
                    // pElementRemove i was checking for pElementTemp)
                    if(pNodeRemove->Tag() != ETAG_DL)
                    {
                        // Run through all the children and change the
                        // listitem's type to <DD>
                        hr = THR(eruns.ChangeListItemsType(iRun,
                                            pNodeRemove->Element(), ETAG_DD));
                        if(hr)
                            goto Cleanup;
                    }
                }
                else
                {
                    if(pNodeRemove->Tag() == ETAG_DL)
                    {
                        // Run through all the children and change the
                        // listitem's type to <LI>
                        hr = THR(eruns.ChangeListItemsType(iRun,
                                            pNodeRemove->Element(), ETAG_LI));
                        if(hr)
                            goto Cleanup;
                    }
                }
                pElementCBParentBlock = pElementListContainer;
            }
            else
            {
                // we didn't find list's container, so we need to convert all the
                // list items into paragraph's appropriately
                long iRunChildStart = iRunStart;
                long iRunChildEnd;

                while(iRunChildStart <= iRunFinish)
                {
                    CElement *  pElementChildBlock = NULL;
                    CTreeNode * pNodeTemp =
                        SearchBranchForBlockElement( iRunChildStart, pFlowLayout );

                    while(DifferentScope(pNodeTemp, pNodeRemove))
                    {
                        pElementChildBlock = pNodeTemp->Element();

                        pNodeTemp =
                            eruns.SearchBranchForBlockElement(pNodeTemp->Parent(), pFlowLayout );
                    }
                    if(!pElementChildBlock)
                    {
                        // we didn't find a block element so we need to insert
                        // a paragraph
                        CTreeNode * pNodeInsertPoint =
                            SearchBranchForChildOfScope(iRunChildStart, pNodeRemove->Element());

                        hr = THR(
                            eruns.CreateAndInsertElement(
                                pNodeInsertPoint->SafeElement(),
                                iRunChildStart, iRunChildStart, etagReplace ) );

                        iRunChildStart++;
                    }
                    else
                    {
                        // Get the scope of the element being removed
                        hr = THR(eruns.GetElementScope(iRunChildStart,
                                                pElementChildBlock,
                                                NULL, &iRunChildEnd));

                        // (srinib) Bug4303 - Child scope may extend beyond the
                        // selection, if so limit it.
                        if(iRunChildEnd > iRunFinish)
                            iRunChildEnd = iRunFinish;

                        if(hr)
                            goto Cleanup;
                        if(pElementChildBlock->HasFlag(TAGDESC_LISTITEM))
                        {
                            // IsolateRuns
                            hr = THR(eruns.IsolateRuns(pElementChildBlock,
                                        iRunChildStart, iRunChildEnd));
                            if(hr)
                                goto Cleanup;
                            // srinib - Bug fix for #218 (replaced
                            // CreateAndReplaceElement with the following)

                            // New default argument fParentBeingRemoved inserts
                            // a P tag if the parent is being removed.
                            // fixes #22742
                            hr = THR(eruns.RemoveBlockElement(pElementChildBlock,
                                            iRunChildStart, iRunChildEnd,
                                            etagReplace, TRUE));
                            if(hr)
                                goto Cleanup;
                        }
                        iRunChildStart = iRunChildEnd + 1;
                    }
                }
            }

            // Remove list's influence on its children

            hr = THR(eruns.IsolateRuns(pNodeRemove->Element(), iRunStart, iRunFinish));
            if(hr)
                goto Cleanup;
            hr = THR(eruns.RemoveElement(iRunStart, pNodeRemove->Element(),
                                        iRunFinish, NULL, NULL));
            if(hr)
                goto Cleanup;

            if(pElementCListItem)
            {

                // Remove the influence of the list item's container on
                // the list item
                hr = THR(eruns.IsolateRuns(pElementCListItem,
                                        iRun, iRunFinish));
                if(hr)
                    goto Cleanup;
                hr = THR(eruns.RemoveElement(iRun, pElementCListItem,
                                            iRunFinish, NULL, NULL));
                if(hr)
                    goto Cleanup;

                // BUGBUG we have to consider merging lists here

                // now run through all the elements and insert a list item
                // if necessary

                long iRunTemp = iRun;
                while(iRunTemp <= iRunFinish)
                {
                    CElement *  pElementChildBlock = NULL;
                    CTreeNode * pNodeParentBlock;
                    long iRunChildEnd;

                    pNodeParentBlock =
                        SearchBranchForBlockElement( iRunTemp, pFlowLayout );

                    while(DifferentScope(pNodeParentBlock, pElementListContainer))
                    {
                        pElementChildBlock = pNodeParentBlock->Element();

                        pNodeParentBlock =
                            eruns.SearchBranchForBlockElement(pNodeParentBlock->Parent(), pFlowLayout );
                    }

                    if(pElementChildBlock)
                    {
                        hr = THR(eruns.GetElementScope(iRunTemp,
                                            pElementChildBlock,
                                                NULL, &iRunChildEnd));
                        if(hr)
                            goto Cleanup;

                        if(pElementChildBlock->HasFlag(TAGDESC_LIST))
                        {
                            // Isolate the runs before insertion
                            hr = THR(eruns.IsolateRuns(pElementChildBlock,
                                                    iRunTemp, iRunChildEnd));
                            if(hr)
                                goto Cleanup;
                            hr = THR(eruns.CreateAndInsertElement(
                                        pElementChildBlock,
                                        iRunTemp, iRunChildEnd,
                                        (pElementListContainer->Tag() ==
                                        ETAG_DL) ? ETAG_DD : ETAG_LI));
                            if(hr)
                                goto Cleanup;
                        }
                        iRunTemp = iRunChildEnd + 1;
                    }
                    else
                    {
                        iRunTemp++;
                    }
                }
            }
        }
        else
        {
            // Remove <BLOCKQUOTE>
            hr = THR(eruns.RemoveBlockElement(pNodeRemove->Element(),
                            iRunStart, iRunFinish, etagReplace));
            if(hr)
                goto Cleanup;
        }
NextIteration:
        hr = THR(eruns.MergeChildLists(pElementCBParentBlock, iRunStart));

        if(hr)
            goto Cleanup;

        // Get rid of simply nested block elements.
        if (pElementAvoidNesting)
        {
            hr = THR(eruns.RemoveElement(iRunStart, pElementAvoidNesting,
                                         iRunFinish, NULL, NULL));
            if(hr)
                goto Cleanup;
        }

        iRun = iRunFinish + 1;
    }

    // BUGBUG (cthrash) HACK ALERT!: LIs under OLs need to be repainted
    // as a result of this operation.  However, the renderer is too smart,
    // and stops painting when it realizes the text hasn't changed.
    // For the moment cause a brute force repaint.

    if (eruns.SearchBranchForTag(iLastRangeRun, ETAG_LI))
    {
        CTreeNode *   pNodeFlowLayout;
        CFlowLayout * pFlowLayout;

        pNodeFlowLayout = eruns.GetFlowLayoutNodeOnBranch( iRunStart );

        if ( pNodeFlowLayout->IsPed() && DifferentScope(pNodeFlowLayout, GetPed()) )
        {
            pNodeFlowLayout = pNodeFlowLayout->Parent()->GetFlowLayoutNode();
            Assert( pNodeFlowLayout->GetPed() == GetPed() );
        }

        Assert(pNodeFlowLayout->HasFlowLayout());

        pFlowLayout = DYNCAST(CFlowLayout, pNodeFlowLayout->GetLayout());

        // Add one extra (non-null) run to ensure that
        // we reset the start of list bit.
        if (iLastRangeRun < (long)eruns.Count() - 1)
        {

            while (++iLastRangeRun < (long)eruns.Count() - 1 &&
                   eruns.GetRunAbs(iLastRangeRun).Cch() == 0);

            Assert(pNodeFlowLayout->HasFlowLayout());

            nf.CharsResize(
                eruns.CpAtStartOfRun( iLastRangeRun ),
                eruns.GetRunAbs(iLastRangeRun).Cch(), iLastRangeRun, 1,
                pNodeFlowLayout);
            _snLast = nf.SerialNumber();
            eruns.Notify( nf );
        }

        pFlowLayout->Invalidate();

#if DBG==1
        CTreeNode * pNodeFlowLayoutEnd = eruns.GetFlowLayoutNodeOnBranch( iRunFinish );

        if ( pNodeFlowLayoutEnd->IsPed() && DifferentScope(pNodeFlowLayoutEnd, GetPed()) )
        {
            pNodeFlowLayoutEnd = pNodeFlowLayoutEnd->Parent()->GetFlowLayoutNode();
            Assert( pNodeFlowLayoutEnd->GetPed() == GetPed() );
        }

        Assert( SameScope(pFlowLayout->Element(), pNodeFlowLayoutEnd) );
#endif
    }

Cleanup:
    pu.ClearRunCaches( iFirstRangeRun, iLastRangeRun );

    pu.Finish(hr);

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::NukePhraseElementsUnderBlockElement
//
//  Synopsis:   Nukes all phrase elements which reside inside of a block element.
//
//  Arguments:  iRunBegin:    Beginning run of the block element
//              iRunEnd:      Ending run of the block element
//              pElementBlockNukeBeforeMe:
//                            Block element below which we nuke the phrase elements
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CTxtRange::NukePhraseElementsUnderBlockElement(
    LONG iRunBegin,
    LONG iRunEnd,
    CElement *pElementBlockNukeBelowMe)
{
#ifdef MERGEFUN // rtp
    HRESULT hr = S_OK;
    CRchTxtPtr rtp(GetPed());
    CTreePosList& eRuns = GetList();
    CTreeNode * pNode;
    CStackPtrAry < CElement *, 10 > aryElements(Mt(CTxtRangeNukePhraseElementsUnderBlockElement_aryElements_pv));
    LONG cpStart;
    LONG cpStop;
    LONG iRunFront;
    LONG iRunBack;
    LONG iRunBackButOne;
    LONG i;

    Assert(pElementBlockNukeBelowMe && iRunBegin >= 0 && iRunEnd >= 0);
    Assert(eRuns.SearchBranchForScopeInStory(iRunBegin, pElementBlockNukeBelowMe));
    Assert(eRuns.SearchBranchForScopeInStory(iRunEnd, pElementBlockNukeBelowMe));
    Assert(    iRunBegin == 0
           || !eRuns.SearchBranchForScopeInStory(iRunBegin - 1, pElementBlockNukeBelowMe)
          );
    Assert(    iRunEnd == eRuns.NumRuns() - 1
           || !eRuns.SearchBranchForScopeInStory(iRunEnd + 1, pElementBlockNukeBelowMe)
          );

    // The start and end cp's of the block element
    cpStart = pElementBlockNukeBelowMe->GetFirstCp();
    Assert(cpStart >= 0);
    cpStop  = pElementBlockNukeBelowMe->GetLastCp();
    Assert(cpStop  >= 0);

    // Find the run where the block element starts
    rtp.SetCp(cpStart);
    // This AdvanceToNonEmpty() will put us in the same run as the
    // one which contains the first character within the block element.
    // This is iRunFront.
    //
    // Assume that there were empty runs before this run under the block
    // element and if those runs had phrase elements above them. If those
    // phrase elements spanned the entire block element then we would find
    // then when we walked up from iRunFront. If we did not find them when
    // we walked up from iRunFront then they do not span the entire block
    // element and hence we are not interested in them.
    rtp.AdvanceToNonEmpty();
    iRunFront = rtp.GetIRun();

    // Setup pElement so that we can use it to walk up the tree, looking
    // for phrase elements
    pNode = rtp.CurrBranch();

    // Check to see if the block element is still above us. It might
    // not be if we're at the end of the doc, or just an empty block
    // element. Bail out.
    if (!pNode->SearchBranchToRootForScope(pElementBlockNukeBelowMe))
        goto Cleanup;

    // And the run in which it ends
    rtp.SetCp(cpStop);
    // The reason for RetreatToNonEmpty() call here is similar to the reason
    // for the AdvanceToNonEmpty() call above
    rtp.RetreatToNonEmpty();
    iRunBack = rtp.GetIRun();


    // If the last character was a block break character, then we
    // need to find nested phrase elements which cover the entire
    // block element or which cover the entire block element less
    // the block break character.
    if (IsSyntheticBlockBreakChar(rtp._rpTX.GetPrevChar()))
    {
        rtp.Advance(-1);
        rtp.RetreatToNonEmpty();
        iRunBackButOne = rtp.GetIRun();
    }
    else
    {
        iRunBackButOne = -1;
    }

    // Walk up the tree till the block element, looking for phrase elements
    // NOTE: pElement has been initialized above
    while (DifferentScope(pNode, pElementBlockNukeBelowMe))
    {
        CElement *pElementScope = pNode->Element();

        // We will remove all phrase elements (which are not span elements)
        // inside block elements
        if (    pElementScope->Tag() != ETAG_SPAN
            &&  !GetPed()->Layout()->IsElementBlockInContext(pElementScope)
            &&  !GetPed()->Layout()->IsElementNoScopeInContext(pElementScope)
           )
        {
            // We add in the element (or a proxy) to the element into
            // our array. We will search for elements in this array
            // from the other end of the block element.
            hr = THR(aryElements.Append(pNode->Element()));
            if (hr)
                goto Cleanup;
        }
        pNode = pNode->Parent();
    }

    for (i = 0; i < aryElements.Size(); i++)
    {
        BOOL fFoundInLast;
        BOOL fFoundInLastButOne;
        LONG iRunRemoveStart;
        LONG iRunRemoveStop;
        CElement* pElement;

        pElement = aryElements[i];

        // At the end of the block element, can we find the phrase element
        // we recorded at the beginning of the block element? If so then
        // this is the element we want to blow away.
        fFoundInLast = !!eRuns.SearchBranchForScopeInStory(iRunBack, pElement);
        fFoundInLastButOne = !fFoundInLast && iRunBackButOne != -1
                && eRuns.SearchBranchForScopeInStory(iRunBackButOne, pElement);

        if (fFoundInLast || fFoundInLastButOne)
        {
            // Found one. Now get the runs the phrase element influences
            // under the block element.
            iRunRemoveStart = iRunFront;

            // Should be able to find both the block element and the phrase
            // element from iRunFront
            Assert(   eRuns.SearchBranchForScopeInStory(iRunRemoveStart, pElement)
                   && eRuns.SearchBranchForScopeInStory(iRunRemoveStart, pElementBlockNukeBelowMe)
                  );

            // Keep going back till we can find both the block
            // and the phrase elements.
            while (iRunRemoveStart > 0)
            {
               if (!(   eRuns.SearchBranchForScopeInStory(iRunRemoveStart - 1, pElement)
                     && eRuns.SearchBranchForScopeInStory(iRunRemoveStart - 1, pElementBlockNukeBelowMe)
                    )
                  )
                   break;
               iRunRemoveStart--;
            }

            // Now at the other end, get the runs influenced by
            // the block and phrase element.
            if (fFoundInLast)
                iRunRemoveStop = iRunBack;
            else
                iRunRemoveStop = iRunBackButOne;
            Assert(iRunRemoveStop >= 0);
            Assert(   eRuns.SearchBranchForScopeInStory(iRunRemoveStop, pElement)
                   && eRuns.SearchBranchForScopeInStory(iRunRemoveStop, pElementBlockNukeBelowMe)
                  );

            // Keep going till we find runs influenced by the phrase and
            // the block element.
            while (iRunRemoveStop < NumRuns() - 1)
            {
                if (! (   eRuns.SearchBranchForScopeInStory(iRunRemoveStop + 1, pElement)
                       && eRuns.SearchBranchForScopeInStory(iRunRemoveStop + 1, pElementBlockNukeBelowMe)
                      )
                   )
                    break;
                iRunRemoveStop++;
            }

            // Found the end runs ... now remove the influence of
            // the element over these set of runs.
            hr = THR(eRuns.RemoveElement(iRunRemoveStart, pElement,
                                         iRunRemoveStop, NULL, NULL));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
}

//+---------------------------------------------------------------
//
//  Member:     FindCommonElement
//
//  Synopsis:   Returns the lowest element that completly covers
//              the current selection.  Guaranteed to return an
//              element unless a specific etag is specified or
//              more then one site is selected.
//
//---------------------------------------------------------------

CTreeNode *
CTxtRange::FindCommonElement ( CTreeNode * pNodeSiteSelected, ELEMENT_TAG etag )
{
    CTreeNode * pNode;

    if (pNodeSiteSelected)
    {
        pNode = pNodeSiteSelected;
    }
    else
    {
#ifdef MERGEFUN // iRuns
        long lRunFirst, lRunLast;

        GetEndRunsIgnoringSpaces( & lRunFirst, & lRunLast );

        pNode = FindCommonBlockElement( lRunFirst, lRunLast, TRUE );
#else
        pNode = NULL;
#endif
    }

    if (etag != ETAG_UNKNOWN)
    {
        while (pNode && pNode->Tag() != etag)
            pNode = pNode->Parent();
    }

    return pNode;
}


//+---------------------------------------------------------------
//
//  Member:     GetAnchorElements
//
//  Synopsis:   Returns IElement ptrs to any anchors in the selection.
//
//---------------------------------------------------------------

// BUGBUG: Must have a site version of this in CSelectionRecord

HRESULT
CTxtRange::GetAnchorElements (
    CLayout * pLayoutSelected, CPtrAry < CElement * > * paryElement)
{
    CTreeNode * pNode;
    HRESULT     hr = S_OK;

    paryElement->SetSize(0);

    if (pLayoutSelected)
    {
        pNode = FindCommonElement(pLayoutSelected->GetFirstBranch(), ETAG_A);

        if (!pNode)
            goto Cleanup;

        hr = THR( paryElement->Append( pNode->SafeElement()) );

        if (hr)
            goto Cleanup;
    }
    else
    {
#ifdef MERGEFUN // iRuns
        long lRunFirst, lRunLast, lRun;

        GetEndRunsIgnoringSpaces( & lRunFirst, & lRunLast );

        for ( lRun = lRunFirst ; lRun <= lRunLast ; ++lRun )
        {
            pNode = GetBranchAbs( lRun );

            while (pNode && pNode->Tag() != ETAG_A)
                pNode = pNode->Parent();

            if (pNode)
            {
                CElement *pElement = pNode->Element();

                if (paryElement->Find( pElement ) < 0)
                {
                    hr = paryElement->Append( pElement );

                    if (hr)
                        goto Cleanup;
                }
            }
        }
#endif
    }

Cleanup:

    RRETURN( hr );
}
