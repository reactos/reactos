
#include "headers.hxx"

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_COMMENT_HXX_
#define X_COMMENT_HXX_
#include "comment.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

//+----------------------------------------------------------------------------
//
//  Functions:  Equal & Compare
//
//  Synopsis:   Helpers for comparing IMarkupPointers
//
//-----------------------------------------------------------------------------

static inline BOOL
IsEqualTo ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    BOOL fEqual;
    IGNORE_HR( p1->IsEqualTo( p2, & fEqual ) );
    return fEqual;
}

static inline int
Compare ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    int result;
    IGNORE_HR( OldCompare( p1, p2, & result ) );
    return result;
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSourceFragComments
//
//  Synopsis:   Remove the fragment begin and end comments which occur
//              in CF_HTML.
//
//-----------------------------------------------------------------------------

static HRESULT
FixupPasteSourceFragComments(
    CDoc * pDoc,
    IMarkupPointer * pSourceStart,
    IMarkupPointer * pSourceFinish )
{
    HRESULT        hr = S_OK;
    CMarkupPointer pmp ( pDoc );

    //
    // Remove the start frag comment
    //

    hr = THR( pmp.MoveToPointer( pSourceStart ) );

    if (hr)
        goto Cleanup;

    for ( ; ; )
    {
        CTreeNode *         pNode;
        MARKUP_CONTEXT_TYPE ct;
        
        hr = THR( pmp.Left( TRUE, & ct, & pNode, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct == CONTEXT_TYPE_None)
            break;

        if (ct == CONTEXT_TYPE_NoScope && pNode->Tag() == ETAG_RAW_COMMENT)
        {
            CCommentElement * pElemComment;
            
            pElemComment = DYNCAST( CCommentElement, pNode->Element() );

            if (pElemComment->_fAtomic &&
                !StrCmpIC( pElemComment->_cstrText, _T( "<!--StartFragment-->" ) ))
            {
                hr = THR( pDoc->RemoveElement( pElemComment ) );

                if (hr)
                    goto Cleanup;

                break;
            }
        }
    }

    //
    // Remove the end frag comment
    //

    hr = THR( pmp.MoveToPointer( pSourceFinish ) );

    if (hr)
        goto Cleanup;

    for ( ; ; )
    {
        CTreeNode *         pNode;
        MARKUP_CONTEXT_TYPE ct;
        
        hr = THR( pmp.Right( TRUE, & ct, & pNode, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct == CONTEXT_TYPE_None)
            break;

        if (ct == CONTEXT_TYPE_NoScope && pNode->Tag() == ETAG_RAW_COMMENT)
        {
            CCommentElement * pElemComment;
            
            pElemComment = DYNCAST( CCommentElement, pNode->Element() );

            if (pElemComment->_fAtomic &&
                !StrCmpIC( pElemComment->_cstrText, _T( "<!--EndFragment-->" ) ))
            {
                hr = THR( pDoc->RemoveElement( pElemComment ) );

                if (hr)
                    goto Cleanup;

                break;
            }
        }
    }

Cleanup:

    RRETURN( hr );
}
                             
//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSourceTables
//
//  Synopsis:   Makes sure that whole (not parts of) tables are included in
//              the source of a paste.
//
//-----------------------------------------------------------------------------

static HRESULT
FixupPasteSourceTables (
    CDoc * pDoc,
    IMarkupPointer * pSourceStart,
    IMarkupPointer * pSourceFinish )
{
    HRESULT hr = S_OK;
    CElement * pElementTableFirst = NULL;
    CElement * pElementTableLast = NULL;
    CMarkupPointer pointer ( pDoc );
    
    //
    // Here we make sure there are no stranded tables in the source
    //

    Assert( Compare( pSourceStart, pSourceFinish ) <= 0 );

    for ( pointer.MoveToPointer( pSourceStart ) ;
          Compare( & pointer, pSourceFinish ) < 0 ; )
    {
        MARKUP_CONTEXT_TYPE ct;
        CTreeNode * pNode;
        
        hr = THR( pointer.Right( TRUE, & ct, & pNode, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct == CONTEXT_TYPE_EnterScope)
        {
            switch ( pNode->Element()->Tag() )
            {
            case ETAG_TC : case ETAG_TD : case ETAG_TR : case ETAG_TBODY :
            case ETAG_THEAD : case ETAG_TFOOT : case ETAG_TH : case ETAG_TABLE :
            {
                CTreeNode * pNodeTable =
                    pNode->SearchBranchToRootForTag( ETAG_TABLE );

                if (pNodeTable)
                {
                    if (!pElementTableFirst)
                        pElementTableFirst = pNodeTable->Element();
                    else
                        pElementTableLast = pNodeTable->Element();
                }
            }
            }
        }
    }

    if (pElementTableFirst)
    {
        CMarkupPointer pointerTable ( pDoc );
        BOOL fResult;

        hr = THR(
            pointerTable.MoveAdjacentToElement(
                pElementTableFirst, ELEM_ADJ_BeforeBegin ) );

        if (hr)
            goto Cleanup;

        hr = THR( pSourceStart->IsRightOf( & pointerTable, & fResult ) );

        if (hr)
            goto Cleanup;
        
        if (fResult)
        {
            hr = THR( pSourceStart->MoveToPointer( & pointerTable ) );

            if (hr)
                goto Cleanup;
        }
        
        hr = THR(
            pointerTable.MoveAdjacentToElement(
                pElementTableFirst, ELEM_ADJ_AfterEnd ) );

        if (hr)
            goto Cleanup;

        hr = THR( pSourceFinish->IsLeftOf( & pointerTable, & fResult ) );

        if (hr)
            goto Cleanup;
        
        if (fResult)
        {
            hr = THR( pSourceFinish->MoveToPointer( & pointerTable ) );

            if (hr)
                goto Cleanup;
        }
    }

    if (pElementTableLast && pElementTableLast != pElementTableFirst)
    {
        CMarkupPointer pointerTable ( pDoc );
        BOOL fResult;

        hr = THR(
            pointerTable.MoveAdjacentToElement(
                pElementTableLast, ELEM_ADJ_BeforeBegin ) );

        if (hr)
            goto Cleanup;

        hr = THR( pSourceStart->IsRightOf( & pointerTable, & fResult ) );

        if (hr)
            goto Cleanup;

        if (fResult)
        {
            hr = THR( pSourceStart->MoveToPointer( & pointerTable ) );

            if (hr)
                goto Cleanup;
        }
        
        hr = THR(
            pointerTable.MoveAdjacentToElement(
                pElementTableLast, ELEM_ADJ_AfterEnd ) );

        if (hr)
            goto Cleanup;

        hr = THR( pSourceFinish->IsLeftOf( & pointerTable, & fResult ) );

        if (hr)
            goto Cleanup;

        if (fResult)
        {
            hr = THR( pSourceFinish->MoveToPointer( & pointerTable ) );

            if (hr)
                goto Cleanup;
        }
    }
    
Cleanup:

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSourceBody
//
//  Synopsis:   Makes sure the <body> is NOT included in the source of the
//              paste.
//
//-----------------------------------------------------------------------------

static HRESULT
Contain (
    CMarkupPointer * pointer,
    CMarkupPointer * pointerLeft,
    CMarkupPointer * pointerRight )
{
    HRESULT hr = S_OK;
    
    if (Compare( pointerLeft, pointer ) > 0)
    {
        hr = THR( pointer->MoveToPointer( pointerLeft ) );

        if (hr)
            goto Cleanup;
    }

    if (Compare( pointerRight, pointer ) < 0)
    {
        hr = THR( pointer->MoveToPointer( pointerRight ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

static HRESULT
FixupPasteSourceBody (
    CDoc *           pDoc,
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish )
{
    HRESULT        hr = S_OK;
    CElement *     pElementClient;
    CMarkup *      pMarkup;
    CMarkupPointer pointerBodyStart( pDoc );
    CMarkupPointer pointerBodyFinish( pDoc );

    //
    // Get the markup container associated with the sel
    //

    pMarkup = pPointerSourceStart->Markup();

    Assert( pMarkup );

    //
    // Get the client element from the markup and check to make sure
    // it's there and is a body element.
    //

    pElementClient = pMarkup->GetElementClient();

    if (!pElementClient || pElementClient->Tag() != ETAG_BODY)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Move temp pointers to the inside edges of the body
    //

    hr = THR(
        pointerBodyStart.MoveAdjacentToElement(
            pElementClient, ELEM_ADJ_AfterBegin ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        pointerBodyFinish.MoveAdjacentToElement(
            pElementClient, ELEM_ADJ_BeforeEnd ) );

    if (hr)
        goto Cleanup;

    //
    // Make sure the source start and finish are within the body
    //

    hr = THR( Contain( pPointerSourceStart, & pointerBodyStart, & pointerBodyFinish ) );

    if (hr)
        goto Cleanup;
    
    hr = THR( Contain( pPointerSourceFinish, & pointerBodyStart, & pointerBodyFinish ) );
    
    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSource
//
//  Synopsis:   Makes sure the source of a paste is valid.  This means, for
//              the most part, that sub-parts of tables must not be pasted
//              without their corresponding table.
//
//-----------------------------------------------------------------------------

static HRESULT
FixupPasteSource (
    CDoc *           pDoc,
    BOOL             fFixupFragComments,
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish )
{
    HRESULT hr = S_OK;

    if (fFixupFragComments)
    {
        hr = THR(
            FixupPasteSourceFragComments(
                pDoc, pPointerSourceStart, pPointerSourceFinish ) );

        if (hr)
            goto Cleanup;
    }

    hr = THR( FixupPasteSourceBody( pDoc, pPointerSourceStart, pPointerSourceFinish ) );

    if (hr)
        goto Cleanup;

    hr = THR( FixupPasteSourceTables( pDoc, pPointerSourceStart, pPointerSourceFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

#if 0 // Pushed this feature to later 'cuase I can't get it to work properly now

//+----------------------------------------------------------------------------
//
//  Function:   IncorporateContextFormattingElements
//
//  Synopsis:   Helper which sniffs the edges of the source to include
//              elements in the context which affect rendering.  This makes
//              paste more word like.
//
//-----------------------------------------------------------------------------

static
BOOL IsFormattingElement ( CElement * pElement )
{
    Assert( pElement );

    switch ( pElement->Tag() )
    {
        case ETAG_A :
        case ETAG_ACRONYM : case ETAG_B :     case ETAG_BDO :    case ETAG_BIG :         
        case ETAG_BLINK :   case ETAG_CITE :  case ETAG_CODE :   case ETAG_DEL :         
        case ETAG_DFN :     case ETAG_EM :    case ETAG_FONT :   case ETAG_I :           
        case ETAG_INS :     case ETAG_KBD :   case ETAG_NOBR :   case ETAG_Q :           
        case ETAG_RP :      case ETAG_RT :    case ETAG_RUBY :   case ETAG_S :           
        case ETAG_SAMP :    case ETAG_SMALL : case ETAG_STRIKE : case ETAG_STRONG :      
        case ETAG_SUB :     case ETAG_SUP :   case ETAG_TT :     case ETAG_U :           
        case ETAG_VAR :
            return TRUE;
            
        default:
            return FALSE;
    }
}
   
static HRESULT
IncorporateContextFormattingElements(
    CMarkupPointer * pmpStart, CMarkupPointer * pmpFinish )
{
    HRESULT hr = S_OK;

    Assert( pmpStart->IsLeftOfOrEqualTo( pmpFinish ) );

    for ( ; ; )
    {
        MARKUP_CONTEXT_TYPE ct;
        CTreeNode * pNodeLeft, * pNodeRight;

        //
        // Here see if an entire element is just outside the range.  Note that
        // I will skip over any text (deleting it to make sure it does not get
        // pasted).  This needs to be done because the saver, when writing out
        // CF_HTML, will sometimes put new lines in which get turned into spaces.
        //

        for ( ; ; )
        {
            hr = THR( pmpStart->Left( FALSE, & ct, & pNodeLeft, NULL, NULL, NULL ) );

            if (hr)
                goto Cleanup;

            if (ct == CONTEXT_TYPE_Text)
            {
                CMarkupPointer pmp ( pmpStart->Doc() );

                hr = THR( pmp.MoveToPointer( pmpStart ) );

                if (hr)
                    goto Cleanup;

                hr = THR( pmp.Left( TRUE, NULL, NULL, NULL, NULL, NULL ) );

                if (hr)
                    goto Cleanup;

                hr = THR( pmp.Doc()->Remove( & pmp, pmpStart ) );

                if (hr)
                    goto Cleanup;

                continue;
            }

            break;
        }

        if (ct != CONTEXT_TYPE_ExitScope)
            break;

        if (!IsFormattingElement( pNodeLeft->Element() ))
            break;

        for ( ; ; )
        {
            hr = THR( pmpFinish->Right( FALSE, & ct, & pNodeRight, NULL, NULL, NULL ) );

            if (hr)
                goto Cleanup;
            
            if (ct == CONTEXT_TYPE_Text)
            {
                CMarkupPointer pmp ( pmpFinish->Doc() );

                hr = THR( pmp.MoveToPointer( pmpFinish ) );

                if (hr)
                    goto Cleanup;

                hr = THR( pmp.Right( TRUE, NULL, NULL, NULL, NULL ) );

                if (hr)
                    goto Cleanup;

                hr = THR( pmp.Doc()->Remove( pmpFinish, & pmp ) );

                if (hr)
                    goto Cleanup;

                continue;
            }

            break;
        }
        
        if (ct != CONTEXT_TYPE_ExitScope)
            break;

        if (pNodeLeft->Element() != pNodeRight->Element())
            break;

        //
        // Now, include that element
        //
        
        hr = THR( pmpStart->Left( TRUE, NULL, NULL, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;
        
        hr = THR( pmpFinish->Right( TRUE, NULL, NULL, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

#endif

//+----------------------------------------------------------------------------
//
//  Function:   RemoveWithBreakOnEmpty
//
//  Synopsis:   Helper fcn which removes stuff between two pointers, and
//              sets the break on empty bit of the block element above
//              the left hand side of the removal.  This is needed for
//              compatibility with IE4 RemoveChars which was used for most
//              text removal operations.
//
//-----------------------------------------------------------------------------

HRESULT
RemoveWithBreakOnEmpty (
    CMarkupPointer * pPointerStart, CMarkupPointer * pPointerFinish )
{
    CMarkup *   pMarkup = pPointerStart->Markup();
    CDoc *      pDoc = pPointerStart->Doc();
    CTreeNode * pNodeBlock;
    HRESULT     hr = S_OK;

    //
    // First, locate the block element above here and set the break on empty bit
    //

    pNodeBlock = pMarkup->SearchBranchForBlockElement( pPointerStart->Branch() );

    if (pNodeBlock)
        pNodeBlock->Element()->_fBreakOnEmpty = TRUE;

    //
    // Now, do the remove
    //

    hr = THR( pDoc->Remove( pPointerStart, pPointerFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   UnoverlapPartials
//
//  Synopsis:   Assuming the contents of an element have been gutted by a
//              Remove operation, this function will take any elements which
//              partially overlapp it, and remove that partial overalpping.
//
//              For example: calling this on y where we have y overlapped
//              from both sides: "<x><y></x><z></y></z>" will produce
//              "<x><y></y></x><z></z>".
//
//-----------------------------------------------------------------------------

#if DBG == 1

static void
IsBefore (
    CElement * pElement1, ELEMENT_ADJACENCY eAdj1,
    CElement * pElement2, ELEMENT_ADJACENCY eAdj2 )
{
    CMarkupPointer pmp1 ( pElement1->Doc() );
    CMarkupPointer pmp2 ( pElement2->Doc() );

    IGNORE_HR( pmp1.MoveAdjacentToElement( pElement1, eAdj1 ) );
    IGNORE_HR( pmp2.MoveAdjacentToElement( pElement2, eAdj2 ) );

    Assert( pmp1.IsLeftOf( & pmp2 ) );
}

#endif

HRESULT
UnoverlapPartials ( CElement * pElement )
{
    HRESULT        hr = S_OK;
    CDoc *         pDoc = pElement->Doc();
    CMarkupPointer pmp ( pDoc );
    CElement *     pElementOverlap = NULL;
    CMarkupPointer pmpStart ( pDoc ), pmpFinish ( pDoc );

    Assert( pElement->IsInMarkup() );

    pElement->GetMarkup()->AddRef();

    hr = THR( pmp.MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeEnd ) );

    if (hr)
        goto Cleanup;

    for ( ; ; )
    {
        MARKUP_CONTEXT_TYPE ct;
        CTreeNode *         pNodeOverlap;

        hr = THR( pmp.Left( TRUE, & ct, & pNodeOverlap, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        //
        // All text and no-scopes better have been removed.
        //

        if (ct != CONTEXT_TYPE_EnterScope && ct != CONTEXT_TYPE_ExitScope)
            break;

        if (ct == CONTEXT_TYPE_ExitScope && pNodeOverlap->Element() == pElement)
            break;

        pElementOverlap = pNodeOverlap->Element();
        pElementOverlap->AddRef();

        if (ct == CONTEXT_TYPE_EnterScope)
        {
            WHEN_DBG( IsBefore( pElementOverlap, ELEM_ADJ_BeforeBegin, pElement, ELEM_ADJ_BeforeBegin ) );

            hr = THR( pmpStart.MoveAdjacentToElement( pElementOverlap, ELEM_ADJ_BeforeBegin ) );

            if (hr)
                goto Cleanup;
            
            hr = THR( pmpFinish.MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd ) );
            
            if (hr)
                goto Cleanup;
        }
        else
        {
            WHEN_DBG( IsBefore( pElement, ELEM_ADJ_AfterEnd, pElementOverlap, ELEM_ADJ_AfterEnd ) );

            hr = THR( pmpFinish.MoveAdjacentToElement( pElementOverlap, ELEM_ADJ_AfterEnd ) );

            if (hr)
                goto Cleanup;
            
            hr = THR( pmpStart.MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd ) );
            
            if (hr)
                goto Cleanup;
        }
        
        hr = THR( pDoc->RemoveElement( pElementOverlap ) );

        if (hr)
            goto Cleanup;

        hr = THR( pDoc->InsertElement( pElementOverlap, & pmpStart, & pmpFinish ) );

        if (hr)
            goto Cleanup;
        
        pElementOverlap->Release();
        
        pElementOverlap = NULL;
    }

Cleanup:

    pElement->GetMarkup()->Release();
    
    if (pElementOverlap)
        pElementOverlap->Release();

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleHTMLInjection
//
//  Synopsis:   Workhorse for innerHTML, outerHTML and insertAdjacentHTML and
//              data binding
//
//-----------------------------------------------------------------------------

HRESULT
HandleHTMLInjection (
    CMarkupPointer * pPointerTargetStart,
    CMarkupPointer * pPointerTargetFinish,
    HGLOBAL          hglobal,
    CElement *       pElementInside )
{
    HRESULT            hr = S_OK;
    CDoc *             pDoc = pPointerTargetStart->Doc();
    CMarkup *          pMarkupContainer = NULL;
    CMarkup *          pMarkupTarget = NULL;
    CMarkupPointer     pointerSourceStart( pDoc );
    CMarkupPointer     pointerSourceFinish( pDoc );
    CMarkupPointer *   pPointerStatus = NULL;
    
    //
    // Make sure the pointers are in the same markup and are ordered
    // correctly.
    //

    Assert( pPointerTargetStart->IsPositioned() );
    Assert( pPointerTargetFinish->IsPositioned() );
    Assert( pPointerTargetStart->Markup() == pPointerTargetFinish->Markup() );

    //
    // Make sure the target start and end are properly oriented (the
    // start must be before the end.
    //

    EnsureLogicalOrder ( pPointerTargetStart, pPointerTargetFinish );

    Assert( Compare( pPointerTargetStart, pPointerTargetFinish ) <= 0 );

    //
    // Now, parse (or attempt to) the HTML given to us
    //

    hr = THR(
        pDoc->ParseGlobal(
            hglobal, PARSE_ABSOLUTIFYIE40URLS, & pMarkupContainer,
            & pointerSourceStart, & pointerSourceFinish ) );

    if (hr)
        goto Cleanup;

    pMarkupTarget = pPointerTargetStart->Markup();
    pMarkupTarget->AddRef();

    //
    // If there was nothing really there to parse, just do a remove
    //
    // BUGBUG: Try to merge this with the removal after inserting the
    // new stuff.
    //
    
    if (!pMarkupContainer)
    {
        hr = THR(
            RemoveWithBreakOnEmpty(
                pPointerTargetStart, pPointerTargetFinish ) );

        if (hr)
            goto Cleanup;

        if (pElementInside)
        {
            hr = THR( UnoverlapPartials( pElementInside ) );

            if (hr)
                goto Cleanup;
        }

        //
        // We're done, git outta here
        //

        goto Cleanup;
    }
    
    //
    // Cleanup the source to paste, possibly adjusting the sel range.
    // Also, the fixup of the source is dependant on the location where
    // it is to be placed.
    //

    hr = THR(
        FixupPasteSource(
            pDoc, FALSE, & pointerSourceStart, & pointerSourceFinish ) );

    if (hr)
        goto Cleanup;

    //
    // See if the source is valid under the context
    //

    hr = THR(
        pDoc->ValidateElements(
            & pointerSourceStart, & pointerSourceFinish, pPointerTargetStart,
            0, & pPointerStatus, NULL, NULL ) );

    if (hr == S_FALSE)
    {
        hr = CTL_E_INVALIDPASTESOURCE;
        goto Cleanup;
    }

    if (hr)
        goto Cleanup;

    //
    // Now, remove the old contents, and put the new in.
    //

    hr = THR( RemoveWithBreakOnEmpty( pPointerTargetStart, pPointerTargetFinish ) );

    if (hr)
        goto Cleanup;

    if (pElementInside)
    {
        hr = THR( UnoverlapPartials( pElementInside ) );

        if (hr)
            goto Cleanup;
    }

    hr = THR(
        pDoc->Move(
            & pointerSourceStart, & pointerSourceFinish, pPointerTargetStart ) );

    if (hr)
        goto Cleanup;

    //
    // BUGBUG (EricVas) - Ramin, fixup adjacent spaces here
    //

    //
    // Evidently, we gotta do this ...
    //
    // BUGBUG (EricVas)  I'm not sure CommitScripts should be used anymore.
    //
    // N.B. (dbau) CommitScripts now commits deferred scripts also (which used to be done explicitly here)
    //

    ULONG cDie;

    cDie = pDoc->_cDie;
    
    hr = THR(pDoc->CommitScripts());
    
    if (hr)
        goto Cleanup;

    if (cDie != pDoc->_cDie)
    {
        hr = E_ABORT;
    }
    
    
Cleanup:

    ReleaseInterface( pPointerStatus );
    
    if (pMarkupContainer)
        pMarkupContainer->Release();
    
    if (pMarkupTarget)
        pMarkupTarget->Release();

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleHTMLInjection
//
//  Synopsis:   Helper function which taks string, coverts it to a global
//              and calls real function.
//
//-----------------------------------------------------------------------------

HRESULT
HandleHTMLInjection (
    CMarkupPointer * pPointerTargetStart,
    CMarkupPointer * pPointerTargetFinish,
    const TCHAR *    pStr,
    long             cch,
    CElement *       pElementInside )
{
    HRESULT hr = S_OK;
    HGLOBAL hHtmlText = NULL;

    if (pStr && *pStr)
    {
        extern HRESULT HtmlStringToSignaturedHGlobal (
            HGLOBAL * phglobal, const TCHAR * pStr, long cch );

        hr = THR(
            HtmlStringToSignaturedHGlobal(
                & hHtmlText, pStr, _tcslen( pStr ) ) );

        if (hr)
            goto Cleanup;

        Assert( hHtmlText );
    }

    hr = THR(
        HandleHTMLInjection(
            pPointerTargetStart, pPointerTargetFinish,
            hHtmlText, pElementInside ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    if (hHtmlText)
        GlobalFree( hHtmlText );
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRightPartialBlockElement
//
//  Synopsis:   Get the partial block element on the right
//
//-----------------------------------------------------------------------------

static void
GetRightPartialBlockElement (
    CMarkupPointer * pPointerLeft,
    CMarkupPointer * pPointerRight,
    CElement * *     ppElementPartialRight )
{
    CTreeNode * pNode;

    Assert( pPointerLeft && pPointerRight );
    Assert( ppElementPartialRight );
    
    *ppElementPartialRight = NULL;
    
    //
    // We want to only examine elements which partially overlapp either side.
    //
    // Set the mark bits on the left and right branches such that _fMark1
    // on the left branch being 1 means that that elements does not totally
    // overalpp.  Also _fMark2 on the right.
    //

    for ( pNode = pPointerLeft->Branch() ; pNode ; pNode = pNode->Parent() )
        pNode->Element()->_fMark1 = 1;

    for ( pNode = pPointerRight->Branch() ; pNode ; pNode = pNode->Parent() )
        pNode->Element()->_fMark2 = 1;
    
    for ( pNode = pPointerLeft->Branch() ; pNode ; pNode = pNode->Parent() )
        pNode->Element()->_fMark2 = 0;

    for ( pNode = pPointerRight->Branch() ; pNode ; pNode = pNode->Parent() )
        pNode->Element()->_fMark1 = 0;

    //
    // Run up the right for a partial overlapping block element
    //
    
    for ( pNode = pPointerRight->Branch() ; pNode ; pNode = pNode->Parent() )
    {
        CElement * pElement = pNode->Element();
        
        if (!pElement->_fMark2)
            continue;

        if (!*ppElementPartialRight && pElement->IsBlockElement())
            *ppElementPartialRight = pElement;
                    
        if (pElement->IsRunOwner())
            *ppElementPartialRight = NULL;
    }
}
    
//+----------------------------------------------------------------------------
//
//  Function:   ResolveConflict
//
//  Synopsis:   Resolves an HTML DTD conflict between two elements by removing
//              the top element over a limited range (defined by the bottom
//              element).
//
//-----------------------------------------------------------------------------

static HRESULT
ResolveConflict(
    CDoc *     pDoc,
    CElement * pElementBottom,
    CElement * pElementTop )
{
    HRESULT          hr = S_OK;
    BOOL             fFoundContent;
    CElement *       pElementClone = NULL;
    CMarkupPointer   pointerTopStart ( pDoc );
    CMarkupPointer   pointerTopFinish ( pDoc );
    CMarkupPointer   pointerBottomStart ( pDoc );
    CMarkupPointer   pointerBottomFinish ( pDoc );
    CMarkupPointer   pointerTemp ( pDoc );

    Assert( pElementBottom->IsInMarkup() );
    Assert( pElementTop->IsInMarkup() );

    //
    // Addref the top element to make sure it does not go away while it
    // is out of the tree.
    //

    pElementTop->AddRef();

    //
    // In IE4, we would never remove a ped (IsContainer is nearly equivalent).
    // So, if a conflict arises where the top element is a ccontainer, remove
    // the bottom element instead.  Also, if there is no container above the
    // top element, we should not remove it.  THis takes care to not remove
    // elements like HTML.
    //

    if (pElementTop->IsContainer() || ! pElementTop->GetFirstBranch()->GetContainer())
    {
        hr = THR( pDoc->RemoveElement( pElementBottom ) );

        if (hr)
            goto Cleanup;

        goto Cleanup;
    }

#if DBG == 1
    pointerTopStart.SetDebugName( _T( "Top Start" ) );
    pointerTopFinish.SetDebugName( _T( "Top Finish" ) );
    pointerBottomStart.SetDebugName( _T( "Bottom Start" ) );
    pointerBottomFinish.SetDebugName( _T( "Bottom Finish" ) );
#endif
    
    //
    // First, more pointer to the locations of the elements in question.
    //
    
    hr = THR(
        pointerTopStart.MoveAdjacentToElement(
            pElementTop, ELEM_ADJ_BeforeBegin ) );

    if (hr)
        goto Cleanup;
    
    hr = THR(
        pointerTopFinish.MoveAdjacentToElement(
            pElementTop, ELEM_ADJ_AfterEnd ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        pointerBottomStart.MoveAdjacentToElement(
            pElementBottom, ELEM_ADJ_BeforeBegin ) );

    if (hr)
        goto Cleanup;
    
    hr = THR(
        pointerBottomFinish.MoveAdjacentToElement(
            pElementBottom, ELEM_ADJ_AfterEnd ) );

    if (hr)
        goto Cleanup;

    //
    // Make sure the input elements are where we think they 'aught to be
    //

    Assert( pointerTopStart.IsLeftOf( & pointerBottomStart ) );
    Assert( pointerTopFinish.IsRightOf( & pointerBottomStart ) );

    //
    // Now, remove the top element and reinsert it so that it is no
    // over the bottom element.
    //

    hr = THR( pDoc->RemoveElement( pElementTop ) );

    if (hr)
        goto Cleanup;

    //
    // Look left.  If we can get to the beginning of the top element,
    // without seeing any text (including break chars) or any elements
    // terminating then we must not put the top element back in before
    // the bottom.
    //

    hr = THR( pointerTemp.MoveToPointer( & pointerBottomStart ) );

    if (hr)
        goto Cleanup;

    fFoundContent = TRUE;

    for ( ; ; )
    {
        MARKUP_CONTEXT_TYPE ct;
        DWORD               dwBreaks;

        //
        // Where a break is relative to a pointer is ambiguous.  Assume they
        // are to the left of the pointer.  Thus, before moving left, check
        // for breaks.
        //

        hr = THR( pointerTemp.QueryBreaks( & dwBreaks ) );

        if (hr)
            goto Cleanup;

        if (dwBreaks)
            break;

        //
        // See if we have reached the beginning of the top element
        //

        if (pointerTemp.IsEqualTo( & pointerTopStart ))
        {
            fFoundContent = FALSE;
            break;
        }

        //
        // Now, move the pointer left
        //

        hr = THR( pointerTemp.Left( TRUE, & ct, NULL, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct != CONTEXT_TYPE_ExitScope)
            break;
    }

    if (fFoundContent)
    {
        hr = THR(
            pDoc->InsertElement(
                pElementTop, & pointerTopStart, & pointerBottomStart ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Look right.
    //

    hr = THR( pointerTemp.MoveToPointer( & pointerBottomFinish ) );

    if (hr)
        goto Cleanup;

    fFoundContent = TRUE;

    for ( ; ; )
    {
        MARKUP_CONTEXT_TYPE ct;
        DWORD               dwBreaks;
        
        hr = THR( pointerTemp.QueryBreaks( & dwBreaks ) );

        if (hr)
            goto Cleanup;

        if (dwBreaks)
            break;

        if (pointerTemp.IsEqualTo( & pointerTopFinish ))
        {
            fFoundContent = FALSE;
            break;
        }
        
        hr = THR( pointerTemp.Right( TRUE, & ct, NULL, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct != CONTEXT_TYPE_ExitScope)
            break;
    }

    if (fFoundContent)
    {
        hr = THR( pElementTop->Clone( & pElementClone, pDoc ) );

        if (hr)
            goto Cleanup;

        hr = THR(
            pDoc->InsertElement(
                pElementClone, & pointerBottomFinish, & pointerTopFinish ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    if (pElementClone)
        pElementClone->Release();

    pElementTop->Release();
    
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   IsEmbeddedElement
//
//  Synopsis:   Intrinsic control and tables are special elements that we must
//              "jump over" while doing a block merge.
//
//-----------------------------------------------------------------------------

BOOL
IsEmbeddedElement ( CTreeNode * pNode )
{
    switch ( pNode->Tag() )
    {
    case ETAG_BUTTON:
    case ETAG_TEXTAREA:
    case ETAG_FIELDSET:
    case ETAG_LEGEND:
    case ETAG_MARQUEE:
    case ETAG_SELECT:
    case ETAG_TABLE:
        
        return TRUE;

    default:

        return pNode->HasLayout();
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   SanitizeRange
//
//  Synopsis:   ...
//
//-----------------------------------------------------------------------------

static HRESULT
ConvertShouldCrLf ( CMarkupPointer * pmp, BOOL & fShouldConvert )
{
    HRESULT     hr = S_OK;
    CTreeNode * pNode;

    fShouldConvert = FALSE;

    for ( pNode = pmp->Branch() ; pNode ; pNode = pNode->Parent() )
    {
        CElement * pElement = pNode->Element();

        if (pElement->IsContainer())
        {
            fShouldConvert = pElement->HasFlag( TAGDESC_ACCEPTHTML );

            goto Cleanup;
        }

        if (pElement->HasFlag( TAGDESC_LITERALTAG ))
        {
            fShouldConvert = FALSE;

            goto Cleanup;
        }

        //
        // Special case for PRE because it is not marked as literal
        //

        if (pElement->Tag() == ETAG_PRE)
        {
            fShouldConvert = FALSE;

            goto Cleanup;
        }
    }
    
Cleanup:

    RRETURN( hr );
}

static HRESULT
LaunderEdge ( CMarkupPointer * pmp )
{
    HRESULT                hr = S_OK;
    IHTMLEditingServices * pedserv = NULL;
    CMarkupPointer         pmpOther ( pmp->Doc() );

    if (!pedserv)
    {
        IHTMLEditor * phtmed = pmp->Doc()->GetHTMLEditor();

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
    }

    hr = THR( pmpOther.MoveToPointer( pmp ) );

    if (hr)
        goto Cleanup;

    hr = THR( pedserv->LaunderSpaces( pmp, & pmpOther ) );

    if (hr)
        goto Cleanup;
    
Cleanup:

    ReleaseInterface( pedserv );
    
    RRETURN( hr );
}

static HRESULT
SanitizeCrLf ( CMarkupPointer * pmp, long & cchAfter )
{
    HRESULT             hr = S_OK;
    CDoc *              pDoc = pmp->Doc();
    CMarkupPointer      mp2 ( pDoc );
    CElement *          pElementNew = NULL;
    TCHAR               ch1, ch2;
    MARKUP_CONTEXT_TYPE ct;
    BOOL                fShouldConvert;
    long                cch;

    //
    // cchAfter is the numner of characters this member deals with after the
    // passed in pointer.
    //

    cchAfter = 0;

    //
    // First, determine the combination of CR/LF chars here
    //

    hr = THR( mp2.MoveToPointer( pmp ) );

    if (hr)
        goto Cleanup;

    hr = THR( mp2.Left( TRUE, & ct, NULL, & (cch = 1), & ch1, NULL ) );

    if (hr)
        goto Cleanup;

    Assert( ct == CONTEXT_TYPE_Text && cch == 1 );
    Assert( ch1 == _T('\r') || ch1 == _T('\n') );

    hr = THR( pmp->Right( FALSE, & ct, NULL, & (cch = 1), & ch2, NULL ) );

    if (hr)
        goto Cleanup;

    if (ct != CONTEXT_TYPE_Text || cch != 1)
        ch2 = 0;

    if ((ch2 == _T('\r') || ch2 == _T('\n')) && ch1 != ch2)
    {
        hr = THR( pmp->Right( TRUE, NULL, NULL, & (cch = 1), NULL, NULL ) );

        if (hr)
            goto Cleanup;
        
        cchAfter++;
    }

    //
    // Now, the text between mp2 and pmp comprises a single line break.
    // Replace it with some marup if needed.
    //

    hr = THR( ConvertShouldCrLf( pmp, fShouldConvert ) );

    if (hr)
        goto Cleanup;

    if (fShouldConvert)
    {
        //
        // Remove the Cr/LF and insert a BR
        //

        hr = THR( pDoc->Remove( & mp2, pmp ) );

        if (hr)
            goto Cleanup;

        hr = THR( pDoc->PrimaryMarkup()->CreateElement( ETAG_BR, & pElementNew ) );

        if (hr)
            goto Cleanup;

        hr = THR( pDoc->InsertElement( pElementNew, pmp, NULL ) );

        if (hr)
            goto Cleanup;
    }
    
Cleanup:

    if (pElementNew)
        pElementNew->Release();

    RRETURN( hr );
}

static HRESULT
SanitizeRange ( CMarkupPointer * pmpStart, CMarkupPointer * pmpFinish )
{
    HRESULT        hr = S_OK;
    CDoc *         pDoc = pmpStart->Doc();
    CMarkupPointer mp ( pDoc );
    TCHAR *        pchBuff = NULL;
    long           cchBuff = 0;

    hr = THR( mp.MoveToPointer( pmpStart ) );

    if (hr)
        goto Cleanup;

    IGNORE_HR( mp.SetGravity( POINTER_GRAVITY_Right ) );

// move the start and finish out to catch adjacent space...
// --> instead, use launder spaces to deal with spaces at the edges of block element

    while ( mp.IsLeftOf( pmpFinish ) )
    {
        MARKUP_CONTEXT_TYPE ct;
        long                cch = cchBuff;
        long                ich;
        TCHAR *             pch;

        //
        // BUGBUG: 
        //
        // It is quite possible to process text AFTER pmpFinish.  This
        // should not be a problem, but if it is, I should add a feature
        // to the There member to stop at a give pointer.  THis may be difficult
        // in that unembedded pointers will have to be searched!
        //

        hr = THR( mp.Right( TRUE, & ct, NULL, & cch, pchBuff, NULL ) );

        if (hr)
            goto Cleanup;
        
        if (ct != CONTEXT_TYPE_Text)
            continue;

        //
        // See if we were not able to get the entire run of text into the buffer
        //

        if (cch == cchBuff)
        {
            long cchMore = -1;
            
            hr = THR( mp.Right( TRUE, & ct, NULL, & cchMore, NULL, NULL ) );

            if (hr)
                goto Cleanup;

            //
            // In order to know if we got all the text, we try to get one more
            // char than we know is there.
            //

            Assert( cchBuff <= cch + cchMore );

            delete pchBuff;

            pchBuff = new TCHAR [ cch + cchMore + 1 ];

            if (!pchBuff)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            cchBuff = cch + cchMore + 1;

            //
            // Now, move the pointer back and attempt to get the text again.
            //

            cchMore += cch;

            hr = THR( mp.Left( TRUE, NULL, NULL, & cchMore, NULL, NULL ) );

            if (hr)
                goto Cleanup;

            continue;
        }

        //
        // Now, examine the buffer for CR/LF or adjacent spaces
        //

        for ( ich = 0, pch = pchBuff ; ich < cch ; ich++, pch++ )
        {
            TCHAR ch = *pch;
            
            if (ch == _T('\r') || ch == _T('\n'))
            {
                CMarkupPointer mpCRLF ( pDoc );
                long           cchMoveBack, cchAfter;

                hr = THR( mpCRLF.MoveToPointer( & mp ) );

                if (hr)
                    goto Cleanup;

                cchMoveBack = cch - ich - 1;

                hr = THR( mpCRLF.Left( TRUE, NULL, NULL, & cchMoveBack, NULL ) );

                if (hr)
                    goto Cleanup;
                
                hr = THR( SanitizeCrLf( & mpCRLF, cchAfter ) );

                if (hr)
                    goto Cleanup;

                ich += cchAfter;
                pch += cchAfter;
            }
            else if (ch == _T(' ') && ich + 1 < cch && *(pch + 1) == _T(' '))
            {
                BOOL fShouldConvert;
                
                // BUGBUG: Is it OK to reuse ConvertShouldCrLf here?
                
                hr = THR( ConvertShouldCrLf( & mp, fShouldConvert ) );

                if (hr)
                    goto Cleanup;

                if (fShouldConvert)
                {
                    CMarkupPointer mpBeforeSpace ( pDoc );
                    CMarkupPointer mpAfterSpace ( pDoc );
                    long           cchMoveBack;
                    TCHAR          cpSpace;

                    hr = THR( mpAfterSpace.MoveToPointer( & mp ) );

                    if (hr)
                        goto Cleanup;

                    cchMoveBack = cch - ich - 1;

                    hr = THR( mpAfterSpace.Left( TRUE, NULL, NULL, & cchMoveBack, NULL ) );

                    if (hr)
                        goto Cleanup;
                    
                    hr = THR( mpBeforeSpace.MoveToPointer( & mpAfterSpace ) );

                    if (hr)
                        goto Cleanup;

                    cchMoveBack = 1;

                    hr = THR( mpBeforeSpace.Left( TRUE, NULL, NULL, & cchMoveBack, NULL ) );

                    if (hr)
                        goto Cleanup;

                    hr = THR( pDoc->Remove( & mpBeforeSpace, & mpAfterSpace ) );

                    if (hr)
                        goto Cleanup;

                    cpSpace = WCH_NBSP;

                    hr = THR( pDoc->InsertText( & mpBeforeSpace, & cpSpace, 1 ) );

                    if (hr)
                        goto Cleanup;
                }
            }
        }
    }

    hr = THR( LaunderEdge( pmpStart ) );

    if (hr)
        goto Cleanup;

    hr = THR( LaunderEdge( pmpFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    delete pchBuff;

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   MergeBlock
//
//  Synopsis:   Does a block merge.  The content after the give block element
//              is merged with that block elements content.
//
//-----------------------------------------------------------------------------

static BOOL
IsAccessDivHack( CTreeNode * pNode )
{
    CElement * pElement;
    LPCTSTR    strClass;
    
    if (!pNode)
        return FALSE;
    
    if (pNode->Tag() != ETAG_DIV)
        return FALSE;

    pElement = pNode->Element();

    if (!pElement)
        return FALSE;

    strClass = pElement->GetAAclassName();

    if (!strClass)
        return FALSE;

    if (!StrCmpC( strClass, _T( "MicrosoftAccessBanner" ) ))
        return TRUE;
    
    if (!StrCmpC( strClass, _T( "MSOShowDesignGrid" ) ))
        return TRUE;

    return FALSE;
}

HRESULT
MergeBlock ( CMarkupPointer * pPointerMerge )
{
    HRESULT          hr = S_OK;
    CDoc *           pDoc = pPointerMerge->Doc();
    CMarkup *        pMarkup = pPointerMerge->Markup();
    CElement *       pElementContainer;
    CFlowLayout *    pFlowContainer;
    CMarkupPointer   pointer ( pDoc );
    CMarkupPointer   pointerEnd ( pDoc );
    CElement *       pElementBlockMerge;
    CTreeNode *      pNodeBlockMerge;
    CTreeNode *      pNode;
    CLineBreakCompat breaker;
    BOOL             fFoundContent;
    CElement *       pElementBlockContent = NULL;
    int              i;
    CStackPtrAry < CElement *, 4 > aryMergeLeftElems ( Mt( Mem ) );
    CStackPtrAry < CElement *, 4 > aryMergeRightElems ( Mt( Mem ) );
    CStackPtrAry < INT_PTR, 4 > aryMergeRightElemsRemove ( Mt( Mem ) );

    breaker.SetWantPendingBreak( TRUE );

    Assert( pPointerMerge->IsPositioned() );

    //
    // The merge must be contained to certain elements.  For example, a TD
    // cannot be merged with stuff after it.
    //
    // Text sites are the limiting factor here.  Locate the element which
    // will contain the merge.  
    //

    if (!pPointerMerge->Branch())
        goto Cleanup;
    
    pElementContainer = pPointerMerge->Branch()->GetFlowLayoutElement();

    if (!pElementContainer)
        goto Cleanup;

    pFlowContainer = pElementContainer->HasFlowLayout();

    Assert( pFlowContainer );

    //
    // Locate the block element the merge pointer is currently in.  This
    // is the block element which will subsume the content to its right.
    //

    pNodeBlockMerge =
        pMarkup->SearchBranchForBlockElement(
            pPointerMerge->Branch(), pFlowContainer );

    if (!pNodeBlockMerge)
        goto Cleanup;

    pElementBlockMerge = pNodeBlockMerge->Element();

    //
    // Search right looking for real content.  The result of this will
    // be the element under which this content exists.
    //

    hr = THR( pointer.MoveToPointer( pPointerMerge ) );

    if (hr)
        goto Cleanup;

    fFoundContent = FALSE;
    
    for ( ; ; )
    {
        DWORD dwBreaks;
        CTreeNode * pNode;
        CElement * pElementFlow;
        MARKUP_CONTEXT_TYPE ct;

        //
        // Make sure we are still under the influence of the container
        //

        pNode = pointer.Branch();

        if (!pNode)
            break;

        pElementFlow = pNode->GetFlowLayoutElement();

        if (!pElementFlow || pElementFlow != pElementContainer)
            break;

        //
        // Get the current block element
        //
        
        pNode =
            pMarkup->SearchBranchForBlockElement( pNode, pFlowContainer );

        if (!pNode)
            break;
        
        pElementBlockContent = pNode->Element();

        //
        // Get the current break
        //

        hr = THR( breaker.QueryBreaks( & pointer, & dwBreaks ) );

        if (hr)
            goto Cleanup;

        if (dwBreaks && pElementBlockContent != pElementBlockMerge)
        {
            fFoundContent = TRUE;
            break;
        }
        //
        // See if there is content to the right
        //

        hr = THR( pointer.Right( TRUE, & ct, & pNode, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct == CONTEXT_TYPE_None)
            break;

        //
        //
        //

        if (ct == CONTEXT_TYPE_NoScope || ct == CONTEXT_TYPE_Text)
        {
            fFoundContent = TRUE;
            break;
        }
        else if (ct == CONTEXT_TYPE_EnterScope)
        {
            if (IsAccessDivHack( pNode ))
                break;
            
            if (IsEmbeddedElement( pNode ))
            {
                fFoundContent = TRUE;
            
                hr = THR( pointer.MoveAdjacentToElement( pNode->Element(), ELEM_ADJ_AfterEnd ) );

                if (hr)
                    goto Cleanup;

                break;
            }
        }
    }

    if (!fFoundContent)
        goto Cleanup;

    //
    // Now, locate the extent of this content un this element
    //

    hr = THR( pointerEnd.MoveToPointer( & pointer ) );

    if (hr)
        goto Cleanup;

    for ( ; ; )
    {
        DWORD dwBreaks;
        CTreeNode * pNode;
        CElement * pElementFlow;
        MARKUP_CONTEXT_TYPE ct;

        //
        // Make sure we are still under the influence of the container
        //

        pNode = pointer.Branch();

        if (!pNode)
            break;

        pElementFlow = pNode->GetFlowLayoutElement();

        if (!pElementFlow || pElementFlow != pElementContainer)
            break;

        //
        // Get the current block element
        //
        
        pNode =
            pMarkup->SearchBranchForBlockElement( pNode, pFlowContainer );

        if (!pNode || pNode->Element() != pElementBlockContent)
            break;

        //
        // Get the current break
        //

        hr = THR( breaker.QueryBreaks( & pointer, & dwBreaks ) );

        if (hr)
            goto Cleanup;

        if (dwBreaks)
        {
            hr = THR( pointerEnd.MoveToPointer( & pointer ) );

            if (hr)
                goto Cleanup;
        }

        //
        // See if there is content to the right
        //

        hr = THR( pointer.Right( TRUE, & ct, & pNode, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        if (ct == CONTEXT_TYPE_None)
            break;

        //
        //
        //

        if (ct == CONTEXT_TYPE_NoScope || ct == CONTEXT_TYPE_Text)
        {
            hr = THR( pointerEnd.MoveToPointer( & pointer ) );

            if (hr)
                goto Cleanup;
        }
        else if (ct == CONTEXT_TYPE_EnterScope)
        {
            if (IsAccessDivHack( pNode ))
                break;

            if (IsEmbeddedElement( pNode ))
            {
                hr = THR( pointerEnd.MoveAdjacentToElement( pNode->Element(), ELEM_ADJ_AfterEnd ) );

                if (hr)
                    goto Cleanup;

                hr = THR( pointer.MoveToPointer( & pointerEnd ) );

                if (hr)
                    goto Cleanup;
            }
        }
    }

    //
    // Locate all the elements which will subsume content
    //

    for ( pNode = pPointerMerge->Branch() ;
          pNode->Element() != pElementContainer ;
          pNode = pNode->Parent() )
    {
        if (pFlowContainer->IsElementBlockInContext( pNode->Element() ))
        {
            hr = THR( aryMergeLeftElems.Append( pNode->Element() ) );

            if (hr)
                goto Cleanup;
        }
    }

    //
    // Locate all the elements which will loose content
    //

    for ( pNode = pointerEnd.Branch() ;
          pNode->Element() != pElementBlockMerge && pNode->Element() != pElementContainer ;
          pNode = pNode->Parent() )
    {
        BOOL fHasContentLeftover;
        
        if (!pFlowContainer->IsElementBlockInContext( pNode->Element() ))
            continue;

        {
            CMarkupPointer p ( pDoc );

            fHasContentLeftover = FALSE;
            
            hr = THR( p.MoveToPointer( & pointerEnd ) );

            if (hr)
                goto Cleanup;

            for ( ; ; )
            {
                MARKUP_CONTEXT_TYPE ct;
                CTreeNode * pNode2;
                
                hr = THR( p.Right( TRUE, & ct, & pNode2, NULL, NULL, NULL ) );

                if (hr)
                    goto Cleanup;

                if (ct == CONTEXT_TYPE_ExitScope && pNode2->Element() == pNode->Element())
                    break;

                if (ct == CONTEXT_TYPE_Text)
                {
                    fHasContentLeftover = TRUE;
                    break;
                }
            }
        }
        
        hr = THR( aryMergeRightElemsRemove.Append( ! fHasContentLeftover ) );

        if (hr)
            goto Cleanup;

        hr = THR( aryMergeRightElems.Append( pNode->Element() ) );

        if (hr)
            goto Cleanup;
    }
    
    //
    // Now, move the end of the left elements to subsume the content
    //

    hr = THR( pointerEnd.SetGravity( POINTER_GRAVITY_Right ) );

    if (hr)
        goto Cleanup;

    for ( i = 0 ; i < aryMergeLeftElems.Size() ; i++ )
    {
        CMarkupPointer pointerStart ( pDoc );
        CElement *     pElement = aryMergeLeftElems[i];

        //
        // Make sure we don't move an end tag to the left.
        //

        {
            CMarkupPointer p2 ( pDoc );

            hr = THR( p2.MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeEnd ) );

            if (hr)
                goto Cleanup;

            if (p2.IsRightOfOrEqualTo( & pointerEnd ))
                continue;
        }
        
        hr = THR(
            pointerStart.MoveAdjacentToElement(
                pElement, ELEM_ADJ_BeforeBegin ) );

        if (hr)
            goto Cleanup;

        pElement->AddRef();
        
        hr = THR( pDoc->RemoveElement( pElement ) );

        if (hr)
            goto Cleanup;

        hr = THR(
            pDoc->InsertElement(
                pElement, & pointerStart, & pointerEnd ) );
        
        if (hr)
            goto Cleanup;

        pElement->Release();
    }

    //
    // Move the begin of the right elements to loose the content
    //

    hr = THR( pointerEnd.SetGravity( POINTER_GRAVITY_Left ) );

    if (hr)
        goto Cleanup;

    for ( i = 0 ; i < aryMergeRightElems.Size() ; i++ )
    {
        CMarkupPointer pointerFinish ( pDoc );
        CElement *     pElement = aryMergeRightElems[i];

        {
            CMarkupPointer p2 ( pDoc );

            hr = THR( p2.MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ) );

            if (hr)
                goto Cleanup;

            if (p2.IsLeftOfOrEqualTo( pPointerMerge ))
                continue;
        }
        
        hr = THR(
            pointerFinish.MoveAdjacentToElement(
                pElement, ELEM_ADJ_AfterEnd ) );

        if (hr)
            goto Cleanup;

        pElement->AddRef();
        
        hr = THR( pDoc->RemoveElement( pElement ) );

        if (hr)
            goto Cleanup;

        if (!aryMergeRightElemsRemove[i])
        {
            CMarkupPointer * p1 = & pointerEnd, * p2 = & pointerFinish;

            EnsureLogicalOrder( p1, p2 );

            hr = THR( pDoc->InsertElement( pElement, p1, p2 ) );

            if (hr)
                goto Cleanup;
        }

        pElement->Release();
    }

    //
    // THe content which was "moved" may have \r or multiple spaces which are
    // not legal under the new context.  Sanitize this range.
    //
    
    hr = THR( SanitizeRange( pPointerMerge, & pointerEnd ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

static HRESULT
UiDeleteContent ( CMarkupPointer * pmpStart, CMarkupPointer * pmpFinish )
{
    HRESULT                hr = S_OK;
    CDoc *                 pDoc = pmpStart->Doc();
    IHTMLEditingServices * pedserv = NULL;

    if (!pDoc->GetHTMLEditor())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(
        pDoc->GetHTMLEditor()->QueryInterface(
            IID_IHTMLEditingServices, (void **) & pedserv ) );

    if (hr)
        goto Cleanup;

    hr = THR( pedserv->Delete( pmpStart, pmpFinish, TRUE ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    ReleaseInterface( pedserv );

    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   HandleUIPasteHTML
//
//  Synopsis:   Performs the actual work of pasting (like, ctrl-V)
//
//-----------------------------------------------------------------------------

HRESULT
HandleUIPasteHTML (
    CMarkupPointer * pPointerTargetStart,
    CMarkupPointer * pPointerTargetFinish,
    HGLOBAL          hglobal,
    BOOL             fAutomation )
{
    HRESULT                hr = S_OK;
    CDoc *                 pDoc = pPointerTargetStart->Doc();
    CMarkup *              pMarkup = NULL;
    CMarkupPointer         pointerSourceStart ( pDoc );
    CMarkupPointer         pointerSourceFinish ( pDoc );
    CMarkupPointer *       pPointerStatus = NULL;
    CMarkupPointer         pointerNewContentLeft ( pDoc );
    CMarkupPointer         pointerNewContentRight ( pDoc );
    CMarkupPointer         pointerMergeRight ( pDoc );
    CMarkupPointer         pointerMergeLeft ( pDoc );
    CMarkupPointer         pointerRemoveLeft ( pDoc );
    CMarkupPointer         pointerRemoveRight ( pDoc );
    CElement *             pElementMergeRight;

    //
    // Make sure the pointers are in the same markup and are ordered
    // correctly.
    //

    Assert( pPointerTargetStart->IsPositioned() );
    Assert( pPointerTargetFinish->IsPositioned() );
    Assert( pPointerTargetStart->Markup() == pPointerTargetFinish->Markup() );

    WHEN_DBG( pPointerTargetStart->SetDebugName( _T( "Target Start" ) ) );
    WHEN_DBG( pPointerTargetFinish->SetDebugName( _T( "Target Finish" ) ) );

    //
    // Make sure the target start and end are properly oriented (the
    // start must be before the end.
    //

    EnsureLogicalOrder ( pPointerTargetStart, pPointerTargetFinish );

    Assert( Compare( pPointerTargetStart, pPointerTargetFinish ) <= 0 );

    //
    // Now, parse (or attempt to) the HTML given to us
    //

    hr = THR(
        pDoc->ParseGlobal(
            hglobal, PARSE_ABSOLUTIFYIE40URLS, & pMarkup,
            & pointerSourceStart, & pointerSourceFinish ) );

    if (hr)
        goto Cleanup;

    WHEN_DBG( pointerSourceStart.SetDebugName( _T( "Source Start" ) ) );
    WHEN_DBG( pointerSourceFinish.SetDebugName( _T( "Source Finish" ) ) );

    //
    // If there was nothing really there to parse, just do a remove
    //
    
    if (!pMarkup)
    {
        hr = THR( UiDeleteContent( pPointerTargetStart, pPointerTargetFinish ) );

        if (hr)
            goto Cleanup;
        
        //
        // BUGBUG - Launder spaces here on the edges
        // BUGBUG - Launder spaces here on the edges
        // BUGBUG - Launder spaces here on the edges
        // BUGBUG - Launder spaces here on the edges
        // BUGBUG - Launder spaces here on the edges
        //

        goto Cleanup;
    }

    //
    // Make sure this current range is one which is valid to delete and/or
    // replace with something else.  In the old IE4 code, the function
    // ValidateRplace was used to determine this.
    //
    // Also, the ole code used to call ValidateInsert which would decide if
    // the target location was a validate place to insert stuff.
    //
    // BUGBUG: Ramin - do this here.
    //

// ValidateInsert code goes here (or similar)

    //
    // Cleanup the source to paste, possibly adjusting the sel range.
    // We paste NULL for the last parameter because I do not want the
    // fixup code to conditionally not do the table check.
    //

    hr = THR(
        FixupPasteSource(
            pDoc, !fAutomation, & pointerSourceStart, & pointerSourceFinish ) );

    if (hr)
        goto Cleanup;
    
// I took this out because there were too many bugs/issues with it in
//    //
//    // If this is not being performed from the automation range, then
//    // include formatiing elements from the context
//    //
//
//    if (!fAutomation)
//    {
//        hr = THR(
//            IncorporateContextFormattingElements(
//                & pointerSourceStart, & pointerSourceFinish ) );
//
//        if (hr)
//            goto Cleanup;
//    }
    
    //
    // Compute right block element to merge
    //

    GetRightPartialBlockElement (
        & pointerSourceStart, & pointerSourceFinish, & pElementMergeRight );

    if (hr)
        goto Cleanup;

    //
    // In IE4 because of the way the splice operation was written, any
    // elements which partially overlapped the left side of the stuff to
    // move would not be move to the target.  This implicit behaviour
    // was utilized to effectively get left handed block merging (along
    // with not moving any elements which partially overlapped).
    //
    // Here, I remove all elements which partially overlap the left
    // hand side so that the move operation does not move a clone of them
    // to the target.
    //

    {
        CTreeNode * pNode;
        int i;
        CStackPtrAry < CElement *, 4 > aryRemoveElems ( Mt( Mem ) );
        
        for ( pNode = pointerSourceStart.Branch() ; pNode ; pNode = pNode->Parent() )
            pNode->Element()->_fMark1 = 1;

        for ( pNode = pointerSourceFinish.Branch() ; pNode ; pNode = pNode->Parent() )
            pNode->Element()->_fMark2 = 1;

        for ( pNode = pointerSourceStart.Branch() ; pNode ; pNode = pNode->Parent() )
            pNode->Element()->_fMark2 = 0;

        for ( pNode = pointerSourceFinish.Branch() ; pNode ; pNode = pNode->Parent() )
            pNode->Element()->_fMark1 = 0;

        for ( pNode = pointerSourceStart.Branch() ; pNode ; pNode = pNode->Parent() )
        {
            if (pNode->Element()->_fMark1)
            {
                hr = THR( aryRemoveElems.Append( pNode->Element() ) );

                if (hr)
                    goto Cleanup;
            }
        }

        for ( i = 0 ; i < aryRemoveElems.Size() ; i++ )
        {
            hr = THR( pDoc->RemoveElement( aryRemoveElems[i] ) );

            if (hr)
                goto Cleanup;
        }
    }

    //
    // Before actually performing the move, insert two pointers into the
    // target such that they will surround the moved source.
    //
    
    IGNORE_HR( pointerNewContentLeft.SetGravity( POINTER_GRAVITY_Left ) );
    IGNORE_HR( pointerNewContentRight.SetGravity( POINTER_GRAVITY_Right ) );

    hr = THR( pointerNewContentLeft.MoveToPointer( pPointerTargetStart ) );

    if (hr)
        goto Cleanup;

    hr = THR( pointerNewContentRight.MoveToPointer( pPointerTargetStart ) );

    if (hr)
        goto Cleanup;

    //
    // Locate the target with pointers which stay to the right of
    // the newly inserted stuff.  This is especially needed when
    // the two are equal.
    //

    hr = THR( pointerRemoveLeft.MoveToPointer( pPointerTargetStart ) );

    if (hr)
        goto Cleanup;
    
    hr = THR( pointerRemoveRight.MoveToPointer( pPointerTargetFinish ) );

    if (hr)
        goto Cleanup;

    IGNORE_HR( pointerRemoveLeft.SetGravity( POINTER_GRAVITY_Right ) );
    IGNORE_HR( pointerRemoveRight.SetGravity( POINTER_GRAVITY_Right ) );

    //
    // Before performing the move, we insert pointers with cling next
    // to elements which we will later perform a merge.  We need to do
    // this because (potentially) clones of the merge elements in the
    // source will be moved to the target because those elements are
    // only partially selected int the source.
    //

    if (pElementMergeRight)
    {
        hr = THR(
            pointerMergeRight.MoveAdjacentToElement(
                pElementMergeRight, ELEM_ADJ_BeforeBegin ) );

        if (hr)
            goto Cleanup;

        IGNORE_HR( pointerMergeRight.SetGravity( POINTER_GRAVITY_Right ) );
        
        IGNORE_HR( pointerMergeRight.SetCling( TRUE ) );
    }

    //
    // Now, move the source to the target. Here I insert two pointers
    // to record the location of the source after it has moved to the
    // target.
    //

    hr = THR(
        pDoc->Move(
            & pointerSourceStart, & pointerSourceFinish,
            pPointerTargetStart ) );

    if (hr)
        goto Cleanup;

    //
    // Now that the new stuff is in, make the pointers which indicate
    // it point inward so that the isolating char does not get in between
    // them.
    //

    IGNORE_HR( pointerNewContentLeft.SetGravity( POINTER_GRAVITY_Right ) );
    IGNORE_HR( pointerNewContentRight.SetGravity( POINTER_GRAVITY_Left ) );

    //
    // Now, remove the old stuff.  Because we use a rather high level
    // operation to do this, we have to make sure the newly inserted
    // stuff does not get mangled.  We do this by inserting an insulating
    // character.
    //

    {
        long  cch;
        TCHAR ch = _T('~');

        hr = THR( pDoc->InsertText( & pointerNewContentRight, & ch, 1 ) );

        if (hr)
            goto Cleanup;

        hr = THR(
            UiDeleteContent(
                & pointerRemoveLeft, & pointerRemoveRight ) );

        if (hr)
            goto Cleanup;

#if DBG == 1
        {
            MARKUP_CONTEXT_TYPE ct;
        
            IGNORE_HR(
                pointerNewContentRight.Right(
                    FALSE, & ct, NULL, & (cch = 1), & ch ) );

            Assert( ct == CONTEXT_TYPE_Text && ch == _T('~') );
        }
#endif

        hr = THR( pointerRemoveLeft.MoveToPointer( & pointerNewContentRight ) );

        if (hr)
            goto Cleanup;

        hr = THR(
            pointerRemoveLeft.Right(
                TRUE, NULL, NULL, & (cch = 1), NULL ) );

        hr = THR( pDoc->Remove( & pointerNewContentRight, & pointerRemoveLeft ) );

        if (hr)
            goto Cleanup;

        hr = THR( pointerRemoveLeft.Unposition() );

        if (hr)
            goto Cleanup;
        
        hr = THR( pointerRemoveRight.Unposition() );

        if (hr)
            goto Cleanup;
    }

    //
    // Now, recover the elements to merge in the target tree
    //

    if (pElementMergeRight)
    {
        MARKUP_CONTEXT_TYPE ct;
        CTreeNode * pNode;
        
        Assert( pointerMergeRight.IsPositioned() );
        
        pointerMergeRight.Right( FALSE, & ct, & pNode, NULL, NULL, NULL );

        Assert( ct == CONTEXT_TYPE_EnterScope );

        pElementMergeRight = pNode->Element();

        hr = THR( pointerMergeRight.Unposition() );

        if (hr)
            goto Cleanup;
    }

    //
    // Now, look for conflicts and remove them
    //

    //
    // Make sure changes to the document get inside the new content pointers
    //
    
    IGNORE_HR( pointerNewContentLeft.SetGravity( POINTER_GRAVITY_Left ) );
    IGNORE_HR( pointerNewContentRight.SetGravity( POINTER_GRAVITY_Right ) );
    
    for ( ; ; )
    {
        CTreeNode * pNodeFailBottom;
        CTreeNode * pNodeFailTop;

        hr = THR(
            pDoc->ValidateElements(
                & pointerNewContentLeft, & pointerNewContentRight, NULL,
                VALIDATE_ELEMENTS_REQUIREDCONTAINERS,
                & pPointerStatus, & pNodeFailBottom, & pNodeFailTop ) );

        if (hr && hr != S_FALSE)
            goto Cleanup;

        if (hr == S_OK)
            break;

        hr = THR(
            ResolveConflict(
                pDoc, pNodeFailBottom->Element(), pNodeFailTop->Element() ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Perform any merging
    //

    if (pElementMergeRight && pElementMergeRight->GetMarkup())
    {
        CMarkupPointer pointer ( pDoc );

        hr = THR(
            pointer.MoveAdjacentToElement(
                pElementMergeRight, ELEM_ADJ_BeforeEnd ) );

        if (hr)
            goto Cleanup;

        hr = THR( MergeBlock( & pointer ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Look to see if only site-like elements were pasted in and return them
    // so that htey may be selected after this operation.
    //

//
// BUGBUG( EricVas)
//          
// Ramin - do this here ...  You will need to add an array to the parameter
// list whith wich you can fill and return.  Alternatevely, you may want
// to simply return pointers which indicate the result of the paste, and have
// the calling code figure this out for itself (this is probably the better
// solution).
//

    //
    // Because the source context for the paste may have allowed \r's and
    // the like in the text which we just pasted to the target, and the
    // recieving target element may not allow these kind of chars, we must
    // sanitize here.
    //

    hr = THR( SanitizeRange( & pointerNewContentLeft, & pointerNewContentRight ) );

    //
    //
    //

    hr = THR( pointerNewContentLeft.Unposition() );

    if (hr)
        goto Cleanup;
    
    hr = THR( pointerNewContentRight.Unposition() );

    if (hr)
        goto Cleanup;

    //
    // Evidently, we gotta do this ...
    //
    // BUGBUG (EricVas)  I'm not sure CommitScripts should be used anymore.
    //
    // BUGBUG BUGBUG - LOOK AT THIS AGAIN!
    //
    // N.B. CommitScripts now also calls CommitDeferredScripts

    ULONG cDie;

    cDie = pDoc->_cDie;

    hr = THR(pDoc->CommitScripts());
    
    if (hr)
        goto Cleanup;

    if (cDie != pDoc->_cDie)
    {
        hr = E_ABORT;
    }
    
Cleanup:

    ReleaseInterface( pPointerStatus );
    
    if (pMarkup)
        pMarkup->Release();
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleUIPasteHTML
//
//  Synopsis:   Helper function which taks string, coverts it to a global
//              and calls real function.
//
//-----------------------------------------------------------------------------

HRESULT
HandleUIPasteHTML (
    CMarkupPointer * pPointerTargetStart,
    CMarkupPointer * pPointerTargetFinish,
    const TCHAR *    pStr,
    long             cch,
    BOOL             fAutomation )
{
    HRESULT hr = S_OK;
    HGLOBAL hHtmlText = NULL;

    if (pStr && *pStr)
    {
        extern HRESULT HtmlStringToSignaturedHGlobal (
            HGLOBAL * phglobal, const TCHAR * pStr, long cch );

        hr = THR(
            HtmlStringToSignaturedHGlobal(
                & hHtmlText, pStr, _tcslen( pStr ) ) );

        if (hr)
            goto Cleanup;

        Assert( hHtmlText );
    }

    hr = THR(
        HandleUIPasteHTML(
            pPointerTargetStart, pPointerTargetFinish, hHtmlText, fAutomation ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    if (hHtmlText)
        GlobalFree( hHtmlText );
    
    RRETURN( hr );
}

