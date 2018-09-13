//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  File:       DELCMD.CXX
//
//  Contents:   Implementation of Delete command.
//
//  History:    07-14-98 - raminh - created
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef X_BLOCKCMD_HXX_
#define X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include <wchdefs.h>
#endif
using namespace EdUtil;

//
// Externs
//

MtDefine(CDeleteCommand, EditCommand, "CDeleteCommand");


//+---------------------------------------------------------------------------
//
//  CDeleteCommand private method: IsLaunderChar( ch )
//  Is the ch a character we launder when we launder spaces?
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsLaunderChar ( TCHAR ch )
{
    return    _T(' ')  == ch
           || _T('\t') == ch
           || WCH_NBSP == ch;
}

BOOL
CDeleteCommand::IsInPre( IMarkupPointer  * pStart, IHTMLElement ** ppPreElement )
{
    BOOL                fMatch = FALSE;
    IHTMLElement    *   pNextElement = NULL ;
    HRESULT             hr   = S_OK;
    ELEMENT_TAG_ID      eTag = TAGID_NULL ;
    IMarkupServices   * pMarkupServices = GetMarkupServices();

    IFC( pStart->CurrentScope( ppPreElement ) );
    if (! *ppPreElement)
    {
        // Scope is NULL, must be a TXTSLAVE
        return TRUE;
    }

    while ( ! fMatch && *ppPreElement )
    {
        IFC( pMarkupServices->GetElementTagId( *ppPreElement, & eTag) );

        switch ( eTag )
        {
        case TAGID_PRE :
        case TAGID_PLAINTEXT :
        case TAGID_LISTING :
        case TAGID_XMP :
        case TAGID_TEXTAREA:
        case TAGID_INPUT: 
            fMatch = TRUE;
            break;
        }

        (*ppPreElement)->get_parentElement( & pNextElement );
        ReplaceInterface( ppPreElement, pNextElement );
        ClearInterface( & pNextElement );
    }

Cleanup:
    if (! fMatch)
        ClearInterface( ppPreElement );
    return fMatch;
}


//+---------------------------------------------------------------------------
//
// CDeleteCommand::HasLayoutOrIsBlock()
//
// Synopsis:    Determine whether pIElement is a block element or has a layout
//
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::HasLayoutOrIsBlock( IHTMLElement * pIElement )
{
    BOOL    fIsBlockElement = FALSE;
    BOOL    fHasLayout = FALSE;

    IGNORE_HR( GetViewServices()->IsBlockElement( pIElement, & fIsBlockElement ) );
    if (! fIsBlockElement)
    {
        IGNORE_HR( GetViewServices()->IsSite( pIElement, & fHasLayout, NULL, NULL, NULL ) );
    }

    return( fIsBlockElement || fHasLayout );
}


//+---------------------------------------------------------------------------
//
// CDeleteCommand::IsIntrinsicControl()
//
// Synopsis:    Determine whether pIElement is an intrinsic control (including INPUT)
//
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsIntrinsicControl( IHTMLElement * pHTMLElement )
{
    ELEMENT_TAG_ID  eTag;

    IGNORE_HR( GetMarkupServices()->GetElementTagId( pHTMLElement, & eTag) );

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
    case ETAG_INPUT:
        return TRUE;

    default:
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
// CDeleteCommand::SkipBlanks()
//
// Synopsis: Walks pPointerToMove in the appropriate direction until a block/layout
//           tag or a non-blank character is encountered. Returns the number of
//           characters crossed.
//
//----------------------------------------------------------------------------

HRESULT
CDeleteCommand::SkipBlanks( IMarkupPointer * pPointerToMove,
                            Direction        eDir,
                            long           * pcch )
{
    HRESULT             hr = S_OK;
    IHTMLElement      * pHTMLElement = NULL;
    long                cch;
    TCHAR               pch[ TEXTCHUNK_SIZE + 1 ]; 
    MARKUP_CONTEXT_TYPE context;
    SP_IMarkupPointer   spPointer;

    Assert( pcch );
    *pcch = 0;

    IFC( GetMarkupServices()->CreateMarkupPointer( & spPointer ) );
    IFC( spPointer->MoveToPointer( pPointerToMove ) );

    for ( ; ; )
    {
        //
        // Move in the appropriate direction
        //
        cch = TEXTCHUNK_SIZE;
        ClearInterface( & pHTMLElement );
        if (eDir == LEFT)
        {
            IFC( spPointer->Left( TRUE, & context, & pHTMLElement, & cch, pch ) );
        }
        else
        {
            IFC( spPointer->Right( TRUE, & context, & pHTMLElement, & cch, pch ) );
        }

        switch( context )
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
            case CONTEXT_TYPE_NoScope:
                if ( HasLayoutOrIsBlock( pHTMLElement ) || IsIntrinsicControl( pHTMLElement ) )
                {
                    // we're done
                    goto Cleanup;
                }
                break;

            case CONTEXT_TYPE_Text:
            {
                //
                // Got some fresh text, look for white space pch using
                // the appropriate direction.
                //
                TCHAR * pchStart;
                long    iOffset;

                if ( eDir == RIGHT )
                {
                    // Left to right we go from beginning of pch to the end
                    for( pchStart = pch; pchStart < ( pch + cch ); pchStart++ )
                    {
                        if (! IsLaunderChar( * pchStart ) )
                        {
                            break;
                        }
                    }
                    iOffset = (pch + cch) - pchStart;
                }
                else
                {
                    // Right to left we go the other way around
                    for( pchStart = pch + (cch - 1); pchStart >= pch; pchStart-- )
                    {
                        if (! IsLaunderChar( * pchStart ) )
                        {
                            break;
                        }
                    }
                    iOffset = (pchStart + 1) - pch;
                }

                //
                // Check the offset of blanks
                //
                if (iOffset == cch)
                {
                    // First character was non-blank, we're done
                    goto Cleanup;
                }
                else if (iOffset == 0)
                {
                    // We got text which was all white space, update pPointerToMove
                    // and continue on...
                    IFC( pPointerToMove->MoveToPointer( spPointer ) );
                    *pcch += cch;
                }
                else
                {
                    // Position the pointer back to where the white space ends
                    if (eDir == LEFT)
                    {
                        IFC( spPointer->Right( TRUE, NULL, NULL, & iOffset, NULL ) );
                    }
                    else
                    {
                        IFC( spPointer->Left( TRUE, NULL, NULL, & iOffset, NULL ) );
                    }
                    //
                    // Update pPointerToMove  and we're done
                    //
                    IFC( pPointerToMove->MoveToPointer( spPointer ) );
                    *pcch += (cch - iOffset);
                    goto Cleanup;
                }
                break;
            }

            case CONTEXT_TYPE_None:
                goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeleteCommand::LaunderSpaces()
//
//  Synopsis:   Replace consecutive blanks resulted from delete or paste
//              operations into a blank and &NBSP's such that the blank 
//              sequence is properly preserved.
//
//----------------------------------------------------------------------------
HRESULT
CDeleteCommand::LaunderSpaces ( IMarkupPointer    * pStart,
                                IMarkupPointer    * pEnd )
{
    const TCHAR chNBSP  = WCH_NBSP;

    HRESULT     hr = S_OK;
    long        cch;
    long        cchCurrent;
    long        cchBlanks = 0;
    DWORD       dwBreak;
    TCHAR       pch[ TEXTCHUNK_SIZE + 1];
    BOOL        fFirstInBlock;
    BOOL        fResult;

    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IMarkupPointer    * pLeft = NULL;
    IMarkupPointer    * pRight = NULL;
    IMarkupPointer    * pDeletionPoint = NULL;
    MARKUP_CONTEXT_TYPE context;
    CEditPointer        ePointer( GetEditor() );
    IHTMLElement      * pPreElement = NULL;

    if (! (pStart && pEnd) ) 
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( pMarkupServices->CreateMarkupPointer( & pLeft ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pRight ) );

    IFC ( pStart->IsRightOf( pEnd, & fResult ) );
    if ( fResult )
    {
        IFC( pLeft->MoveToPointer(  pEnd ) );
        IFC( pRight->MoveToPointer( pStart ) );
    }
    else
    {
        IFC( pLeft->MoveToPointer(  pStart ) );
        IFC( pRight->MoveToPointer( pEnd ) );
    }

    IFC( pLeft->SetGravity( POINTER_GRAVITY_Right ) );

    //
    // If either end of the segment is in a pre formatted element, we're done
    //
    if ( IsInPre( pLeft, & pPreElement ) )
    {       
        goto Cleanup;
    }
    ClearInterface( & pPreElement );
    if ( IsInPre( pRight, & pPreElement ) )
    {       
        goto Cleanup;
    }

    //
    // Move pLeft left and pRight right while next to blanks
    //
    IFC( SkipBlanks( pLeft, LEFT, & cch ) );
    cchBlanks += cch;

    IFC( SkipBlanks( pRight, RIGHT, & cch ) );
    cchBlanks += cch;

    if (! cchBlanks)
    {
        // No blanks found, we're done
        goto Cleanup;
    }    

    //
    // Check whether pLeft is at the first blank of a block or layout element
    //
    IFC( ePointer.MoveToPointer( pLeft ) );
    IGNORE_HR( ePointer.Scan(  LEFT,
                    BREAK_CONDITION_Block |
                    BREAK_CONDITION_Site |
                    BREAK_CONDITION_Control |
                    BREAK_CONDITION_NoScopeBlock |
                    BREAK_CONDITION_Text ,
                    &dwBreak ) );

    fFirstInBlock = ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock  ) ||
                    ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitSite )   ||
                    ePointer.CheckFlag( dwBreak, BREAK_CONDITION_NoScopeBlock );
       
    if ( cchBlanks == 1 && !fFirstInBlock )
    {
        // We only have one blank and it's not at the beginning of a block
        goto Cleanup;
    }

    //
    // pLeft and pRight span consecutive blanks at this point, 
    // let's preserve the blanks now
    //
    
    IFC( pMarkupServices->CreateMarkupPointer( & pDeletionPoint ) );
    IFC( pDeletionPoint->MoveToPointer( pLeft ) );    
  
    cchCurrent = 0;

    while ( cchBlanks )
    {       
        cch = 1;
        IFC( pDeletionPoint->Right( TRUE, & context, NULL, & cch, pch ) );
        switch ( context )
        {
        case CONTEXT_TYPE_Text:
            Assert ( cch == 1 );
            Assert ( IsLaunderChar( *pch ) );
            cchBlanks -= cch;
            cchCurrent++;
            
            if (*pch != chNBSP)
            {                
                if ( cchBlanks == 0                             // We're on the last white character
                     && !(cchCurrent == 1 && fFirstInBlock) )   // And it is not the first char in a block
                {
                    // Leave it alone, we're done
                    break;
                }

                *pch = chNBSP;
                IFC( GetViewServices()->InsertMaximumText( pch, 1, pLeft ) );
                IFC( pMarkupServices->Remove( pLeft, pDeletionPoint ) );
            }
            break;

        case CONTEXT_TYPE_None:
            goto Cleanup;

        }

        // Catch up to the moving pointer
        IFC( pLeft->MoveToPointer( pDeletionPoint ) );
        
    }

Cleanup:
    ReleaseInterface( pLeft );
    ReleaseInterface( pRight );
    ReleaseInterface( pDeletionPoint );
    ReleaseInterface( pPreElement );
    RRETURN ( hr );
}

BOOL    
CDeleteCommand::IsMergeNeeded( IMarkupPointer * pStart, IMarkupPointer * pEnd )
{
    SP_IMarkupPointer   spPointer;
    BOOL                fEqual = FALSE;
    BOOL                fMerger = FALSE;
    int                 iWherePointer;
    HRESULT             hr;
    DWORD               dwBreaks;

    IFC( GetMarkupServices()->CreateMarkupPointer( & spPointer ) );
    IFC( spPointer->MoveToPointer( pStart ) );
    IFC( OldCompare( spPointer, pEnd, & iWherePointer ) );

    switch (iWherePointer)
    {
    case LEFT:
        //
        // We should not have pEnd to the left of pStart
        // otherwise we'll go to a loopus infinitus later
        //
        Assert( iWherePointer == RIGHT );
        goto Cleanup;

    case SAME:
        goto Cleanup;

    }

    //
    // Walk pStart towards pEnd looking for block breaks. If a block break is 
    // crossed, merge is necessary.
    //

    for ( ; ; )
    {
        IFC( GetViewServices()->QueryBreaks( spPointer, & dwBreaks, TRUE ) ); 
        if ( dwBreaks & BREAK_BLOCK_BREAK )
        {
            // We have a merger
            fMerger = TRUE;
            break;
        }        

        IFC( GetViewServices()->RightOrSlave(spPointer, TRUE , NULL, NULL, NULL, NULL ) );

        IFC( spPointer->IsEqualTo( pEnd, & fEqual ) );
        if ( fEqual )
        {
            // We're done, no merger here
            break;            
        }       
    }

Cleanup:
    return fMerger;
}


HRESULT 
CDeleteCommand::MergeDeletion( IMarkupPointer * pStart, IMarkupPointer * pEnd, BOOL fAdjustPointers )
{
    HRESULT             hr = S_OK;
    CEditPointer        ePointerStart( GetEditor() );
    CEditPointer        ePointerEnd  ( GetEditor() );
    IObjectIdentity * pIObj1 = NULL;
    IObjectIdentity * pIObj2 = NULL;
    IHTMLElement    * pHTMLElement1 = NULL;
    IHTMLElement    * pHTMLElement2 = NULL;
    DWORD             dwBreak;
    
    if (! IsMergeNeeded( pStart, pEnd ) )
    {
        goto Cleanup;
    }

    // 
    // Before merging check whether we should delete an empty block,
    // rather than doing the full blown merge.
    //

    if ( fAdjustPointers )
    {
        IFC( ePointerStart.MoveToPointer( pStart ) );
        IGNORE_HR( ePointerStart.Scan(  LEFT,
                        BREAK_CONDITION_Block |
                        BREAK_CONDITION_Site |
                        BREAK_CONDITION_Control |
                        BREAK_CONDITION_Text |
                        BREAK_CONDITION_NoScopeSite |
                        BREAK_CONDITION_NoScopeBlock |
                        BREAK_CONDITION_BlockPhrase,
                        &dwBreak, & pHTMLElement1 ) );
    
        if ( ePointerStart.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock ) )
        {
            IFC( ePointerEnd.MoveToPointer( pStart ) );
            IGNORE_HR( ePointerEnd.Scan(  RIGHT,
                            BREAK_CONDITION_Block |
                            BREAK_CONDITION_Site |
                            BREAK_CONDITION_Control |
                            BREAK_CONDITION_Text |
                            BREAK_CONDITION_NoScopeSite |
                            BREAK_CONDITION_NoScopeBlock |
                            BREAK_CONDITION_BlockPhrase,
                            &dwBreak, & pHTMLElement2 ) );

            if ( ePointerEnd.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock ) )
            {
                IFC( pHTMLElement1->QueryInterface( IID_IObjectIdentity, (void **) & pIObj1 ));
                IFC( pHTMLElement2->QueryInterface(IID_IObjectIdentity,  (void **) & pIObj2 ));
                if ( pIObj1->IsEqualObject( pIObj2 ) == S_OK )
                {
                    // Remove the block
                    IFC( 
                        GetMarkupServices()->Remove( (IMarkupPointer *) ePointerStart, 
                                                     (IMarkupPointer *) ePointerEnd ) );
                    goto Cleanup;
                }   
            }
        }
    }

    //
    // Do the merge
    //
    IFC( GetViewServices()->MergeDeletion( pStart ) );

Cleanup:
    ReleaseInterface( pHTMLElement1 );
    ReleaseInterface( pHTMLElement2 );
    ReleaseInterface( pIObj1 );
    ReleaseInterface( pIObj2 );
    return ( hr );
}


//+====================================================================================
//
// Method: AdjustOutOfBlock
//
// Synopsis: Move the given pointer out of a block while skipping any empty phrase elements.
//           This helper function is called by Delete when it is determined that a entire
//           block was selected for deletion. 
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::AdjustOutOfBlock ( IMarkupPointer * pStart, BOOL * pfDidAdjust )
{
    HRESULT             hr = S_OK;
    DWORD               dwBreakCondition;
    long                cch;
    CEditPointer        pointerStart ( GetEditor() );
    DWORD               dwBreakFor = BREAK_CONDITION_Block       | BREAK_CONDITION_EnterAnchor  | 
                                     BREAK_CONDITION_Text        | BREAK_CONDITION_Control      | 
                                     BREAK_CONDITION_NoScopeSite | BREAK_CONDITION_NoScopeBlock |
                                     BREAK_CONDITION_Site ;

    IFC( pointerStart->MoveToPointer( pStart ) );
    
    *pfDidAdjust = FALSE;

    IGNORE_HR( pointerStart.Scan( LEFT, dwBreakFor, & dwBreakCondition  ) );

    if ( pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_ExitBlock ) )
    {
        *pfDidAdjust = TRUE;
        //
        // Here we also skip any phrase elements around the block we just exited
        //
        IGNORE_HR( pointerStart.Scan( LEFT, dwBreakFor, & dwBreakCondition  ) );
        if (! pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ) )
        {
            cch = 1;
            IFC( pointerStart.Move( RIGHT, TRUE, NULL, NULL, &cch, NULL ) );
        }
        //
        // Set pStart to pointerStart and we're done
        //
        IFC( pStart->MoveToPointer( pointerStart ) );
    }

Cleanup:
    RRETURN( hr );
}


//+====================================================================================
//
// Method: AdjustPointersForDeletion
//
// Synopsis: Move the given pointers out for a delete operation. This adjust stops at
//           blocks, and layouts, but will skip over "other" tags (ie character formatting).
//           This function also detects when an entire block is selected and adjusts the
//           left edge out of the block element to fully delete the block. 
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::AdjustPointersForDeletion ( IMarkupPointer* pStart, 
                                            IMarkupPointer* pEnd )
{
    HRESULT             hr = S_OK;
    DWORD               dwBreakCondition;
    long                cch;
    CEditPointer        pointerStart ( GetEditor(), pStart );
    CEditPointer        pointerEnd   ( GetEditor(), pEnd );
    DWORD               dwBreakFor = BREAK_CONDITION_Block       | BREAK_CONDITION_EnterAnchor  | 
                                     BREAK_CONDITION_Text        | BREAK_CONDITION_Control      | 
                                     BREAK_CONDITION_NoScopeSite | BREAK_CONDITION_NoScopeBlock |
                                     BREAK_CONDITION_NoScope     | BREAK_CONDITION_Site ;

    //
    // Scan left to skip phrase elements
    //
    IGNORE_HR( pointerStart.Scan( LEFT, dwBreakFor, & dwBreakCondition ) );
    
    if (! pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ) )
    {
        // We went too far, move back once
        cch = 1;
        IFC( pointerStart.Move( RIGHT, TRUE, NULL, NULL, &cch, NULL ) );
    }

    //
    // Scan right to skip phrase elements
    //
    IGNORE_HR( pointerEnd.Scan( RIGHT, dwBreakFor, & dwBreakCondition ) );
    if (! pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ) )
    {
        // We went too far, move back once
        cch = 1;
        IFC( pointerEnd.Move( LEFT, TRUE, NULL, NULL, &cch, NULL ) );
    }

Cleanup:
    RRETURN( hr );
}


//+====================================================================================
//
// Method: RemoveBlockIfNecessary
//
// When the left pointer is positioned at the beginning of a block and the right pointer 
// is after the end of the block, the entire block is to be deleted. Here we are detecting
// this case by walking left to right: if we enter a block before hitting the right
// pointer we bail, since this routine is being called after a call to Remove. 
// Otherwise we remove the block if we exit the scope of a block and hit the righthand pointer.
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::RemoveBlockIfNecessary( IMarkupPointer * pStart, IMarkupPointer * pEnd )
{
    IMarkupServices   * pMarkupServices  = GetMarkupServices();
    IHTMLViewServices * pViewServices    = GetViewServices();
    SP_IMarkupPointer   spPointer;
    MARKUP_CONTEXT_TYPE context;
    IHTMLElement *      pHTMLElement = NULL;            
    BOOL                fResult;
    HRESULT             hr = S_OK;
    BOOL                fExitedBlock = FALSE;
    BOOL                fDone = FALSE;

    IFC( pMarkupServices->CreateMarkupPointer( & spPointer ) );    
    IFC( spPointer->MoveToPointer( pStart ) );

    while (! fDone)
    {
        ClearInterface( & pHTMLElement );
        IFC( spPointer->Right( TRUE, & context, & pHTMLElement, NULL, NULL ) );
        
        switch( context )        
        {
        case CONTEXT_TYPE_EnterScope:
            // If we entered a block, we're done
            IGNORE_HR( pViewServices->IsBlockElement( pHTMLElement, & fDone ) );
            break;

        case  CONTEXT_TYPE_ExitScope:
            // Track whether we have exited a block 
            if (! fExitedBlock)
            {
                IGNORE_HR( pViewServices->IsBlockElement( pHTMLElement, & fExitedBlock ) );
            }
            break;

        case CONTEXT_TYPE_None:
            fDone = TRUE;
            break;

        }

        if ( fDone )
            break;

        IFC( spPointer->IsRightOfOrEqualTo( pEnd, & fResult ) );
        if ( fResult )
        {
            if ( fExitedBlock )
            {
                //
                // We've reached the right end and have already exited a block
                // so move pStart out of the block
                //
                IFC( AdjustOutOfBlock( pStart, & fResult ) );
                if (! fResult)
                {
                    // Done
                    break;
                }
            
                //
                // Ask MarkupServices to remove the segment
                //
                IFC( pMarkupServices->Remove( pStart, pEnd ) );      
            }
            // We're done
            break;
        }       
    }

    //
    // If pStart or pEnd are within empty list containers, remove the list containers as well
    //

    IFC( RemoveEmptyListContainers( pStart ) );
    IFC( RemoveEmptyListContainers( pEnd ) );

Cleanup:
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


HRESULT 
CDeleteCommand::RemoveEmptyListContainers(IMarkupPointer * pPointer)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    CBlockPointer       bpCurrent(GetEditor());

    IFR( pPointer->CurrentScope(&spElement) );
    IFR( bpCurrent.MoveTo(spElement) );

    if (bpCurrent.GetType() == NT_ListContainer)
    {
        SP_IMarkupPointer   spStart;
        SP_IMarkupPointer   spEnd;
        BOOL                fDone = FALSE;
        BOOL                fEqual;
            
        IFR( GetMarkupServices()->CreateMarkupPointer(&spStart) );
        IFR( GetMarkupServices()->CreateMarkupPointer(&spEnd) );

        do
        {
            // If the list container is not empty, we're done
            IFR( bpCurrent.MovePointerTo(spStart, ELEM_ADJ_AfterBegin) );    
            IFR( bpCurrent.MovePointerTo(spEnd, ELEM_ADJ_BeforeEnd ) );    

            IFR( spStart->IsEqualTo(spEnd, &fEqual) );
            if (!fEqual)
                break;
           
            // Remove list container and move to parent
            IFR( bpCurrent.GetElement(&spElement) );
            
            IFR( bpCurrent.MoveToParent() );
            fDone = (hr == S_FALSE); // done if no parent

            IFR( GetMarkupServices()->RemoveElement(spElement) );        
        }
        while (bpCurrent.GetType() == NT_ListContainer && !fDone);
    }
    
    return S_OK;
}


//+====================================================================================
//
// Method: InflateBlock
//
// Synopsis: Sets the break on empty flag for a block element
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::InflateBlock( IMarkupPointer * pPointer )
{
    HRESULT         hr = S_OK;
    BOOL            fLayout;
    ELEMENT_TAG_ID  etagId;
    SP_IHTMLElement spElement;
    SP_IHTMLElement spBlockElement;
    IMarkupServices * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();

    IFR( pViewServices->CurrentScopeOrSlave( pPointer, &spElement) );

    IFR( EdUtil::FindBlockElement( pMarkupServices, spElement, & spBlockElement ) );

    if ( spElement )
    {
        IFR( pViewServices->IsLayoutElement( spElement, &fLayout ) );
        if ( !fLayout )
        {
            IFR( pMarkupServices->GetElementTagId( spElement, &etagId ) );
            if ( !IsListContainer( etagId ) && etagId != TAGID_LI )
            {
                IFR( pViewServices->InflateBlockElement( spElement ) );
            }
        }
    }

    RRETURN( hr );
}


//+====================================================================================
//
// Method: Delete
//
// Synopsis: Given two MarkupPointers - delete everything between them
//
//------------------------------------------------------------------------------------

HRESULT 
CDeleteCommand::Delete ( IMarkupPointer* pStart, 
                         IMarkupPointer* pEnd,
                         BOOL fAdjustPointers /* = FALSE */ )
{
    HRESULT             hr = S_OK;
    int                 wherePointer = SAME;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    BOOL                fResult;
    CUndoUnit           undoUnit(GetEditor());
    
    //
    // First adjust the pointers out to skip phrase elements
    //
    IFC( OldCompare( pStart, pEnd , & wherePointer ) );

    if ( wherePointer == SAME )
        goto Cleanup;

    if ( fAdjustPointers )
    {
        if ( wherePointer == RIGHT )
        {
            IFC( AdjustPointersForDeletion( pStart, pEnd ) );
        }
        else
        {
            IFC( AdjustPointersForDeletion( pEnd, pStart ) );
        }
    }

    IFC( OldCompare( pStart, pEnd , & wherePointer ) );
    
    if ( wherePointer == SAME )
        goto Cleanup;

    //
    // Begin the undo unit
    //
    IFC( undoUnit.Begin(IDS_EDUNDOTEXTDELETE) );
    
    //
    // Ask MarkupServices to remove the segment
    //
    if ( wherePointer == RIGHT )
    {
        IFC( pMarkupServices->Remove( pStart, pEnd ) );      
    }
    else
    {
        IFC( pMarkupServices->Remove( pEnd, pStart ) );
    }

    if (fAdjustPointers)
    {
        //
        // Detect if the entire block should be deleted, and if so delete the block
        //
        IFC( pStart->IsEqualTo( pEnd, & fResult ) ); // If equal we have nothing to do...
        if (! fResult)
        {
            if ( wherePointer == RIGHT )
            {
                IFC( RemoveBlockIfNecessary( pStart, pEnd ) );
            }
            else
            {
                IFC( RemoveBlockIfNecessary( pEnd, pStart ) );
            }
        }
    }

    //
    // Find the block elements for pStart and pEnd and set the
    // break on empty flag on them. 
    //
    IFC( InflateBlock( pStart ) );
    IFC( InflateBlock( pEnd ) );

    IFC( OldCompare( pStart, pEnd , & wherePointer ) );
    switch ( wherePointer )
    {
        case RIGHT:
            IFC( MergeDeletion( pStart, pEnd, fAdjustPointers ));  
            break;
        case LEFT:
            IFC( MergeDeletion( pEnd, pStart, fAdjustPointers ));
            break;
    }

    IFC( LaunderSpaces( pStart, pEnd ) );

Cleanup:
    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  CDeleteCommand::IsValidOnControl
//
//----------------------------------------------------------------------------

BOOL CDeleteCommand::IsValidOnControl()
{
    HRESULT         hr;
    BOOL            bResult = FALSE;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, NULL) );

    bResult = (iSegmentCount == 1);

Cleanup:
    return bResult;
}

//+---------------------------------------------------------------------------
//
//  CDeleteCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CDeleteCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{

    HRESULT        hr = S_OK;
    int            iSegmentCount, i;
    IMarkupPointer * pStart = NULL;
    IMarkupPointer * pEnd = NULL;
    IHTMLCaret * pCaret = NULL;
    SELECTION_TYPE eSelectionType;

    IHTMLViewServices * pViewServices = GetViewServices();
    IMarkupServices * pMarkupServices = GetMarkupServices();
    ISegmentList * pSegmentList = NULL;

    //
    // Do the prep work
    //
    IFC( GetSegmentList( &pSegmentList ));   
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, & eSelectionType ) );           
    IFC( pMarkupServices->CreateMarkupPointer( & pStart ) );
    IFC( pMarkupServices->CreateMarkupPointer( & pEnd ) );
            
    if ( eSelectionType != SELECTION_TYPE_Caret )
    {
        //
        // Delete the segments
        //
        if ( iSegmentCount != 0 )
        {       
            for ( i = 0; i < iSegmentCount; i++ )
            {
                IFC(MovePointersToSegmentHelper(GetViewServices(), pSegmentList, i, &pStart, &pEnd));

                //
                // BUGBUG - this is an icky place to handle control-delete. I can't really think of
                // another - short of having the delete command talk to the tracker.
                //
                if ( _cmdId == IDM_DELETEWORD && 
                     eSelectionType == SELECTION_TYPE_Selection  )
                {
                    // they're performing a control delete.
                    // Instead of using the start and end - the end is the word end from the start.
                    //
                    DWORD dwBreak = 0;
                    
                    CEditPointer blockScan( GetEditor());
                    blockScan.MoveToPointer( pEnd );
                    blockScan.Scan( RIGHT,
                                    BREAK_CONDITION_Block |
                                    BREAK_CONDITION_Site |
                                    BREAK_CONDITION_Control |
                                    BREAK_CONDITION_Text ,
                                    &dwBreak );

                    if ( blockScan.CheckFlag( dwBreak, BREAK_CONDITION_Text ))
                    {
                        //
                        // We use moveUnit instead of MoveWord - as MoveWord is only good to move
                        // to word beginnings. We want to do just like Word does - and really move
                        // to the word end.
                        //
                        IFC( pEnd->MoveToPointer( pStart ));
                        IFC( pEnd->MoveUnit(MOVEUNIT_NEXTWORDEND ));
                    }
                }
                
                //
                // Cannot delete or cut unless the range is in the same flow layout
                //
                if ( PointersInSameFlowLayout( pStart, pEnd, NULL , GetViewServices() ) )
                {                 
                    IFC( Delete( pStart, pEnd, TRUE ) );
                }
            }

            if ( ( eSelectionType != SELECTION_TYPE_Auto) && 
                 ( eSelectionType != SELECTION_TYPE_Control ) ) // Control is handled in ExitTree
            {
                GetEditor()->GetSelectionManager()->EmptySelection();
            }

        }
    }
    else
    {
        CSpringLoader     * psl = GetSpringLoader();
        MARKUP_CONTEXT_TYPE mctContext;
        long                cch = 2;
        BOOL                fCacheFontForLastChar = TRUE;

        Assert(eSelectionType == SELECTION_TYPE_Caret);

        //
        // Handle delete at caret
        //

        IFC( pViewServices->GetCaret( & pCaret ));
        IFC( pCaret->MovePointerToCaret( pStart));

        // Reset springloader.
        Assert(psl);
        psl->Reset();

        // Decide if springloader needs to preserve formatting of last character in block.
        fCacheFontForLastChar = _cmdId != IDM_DELETE
                             || S_OK != THR(pStart->Right(FALSE, &mctContext, NULL, &cch, NULL))
                             || mctContext != CONTEXT_TYPE_Text
                             || cch != 2;
        if (fCacheFontForLastChar)
        {
            IFC( psl->SpringLoad(pStart, SL_TRY_COMPOSE_SETTINGS) );
        }

        IFC( DeleteCharacter( pStart, FALSE, _cmdId == IDM_DELETEWORD,
                              GetEditor()->GetSelectionManager()->GetStartEditContext() ) );

        //
        // Set the caret to pStart
        // Note that this is the code path for forward delete only, backspace
        // is handled in HandleKeyDown()       
        CCaretTracker * pCaretTracker = (CCaretTracker *)GetEditor()->GetSelectionManager()->GetActiveTracker();
        Assert( pCaretTracker );       
        
        IFC( pCaretTracker->PositionCaretAt( pStart, 
                                             pCaretTracker->GetNotAtBOL(), 
                                             pCaretTracker->GetAtLogicalBOL(), CARET_DIRECTION_BACKWARD, POSCARETOPT_None, ADJPTROPT_None ) );

        // If we didn't delete last character, we need to forget formatting by resetting springloader.
        if (   fCacheFontForLastChar
            && S_OK != psl->CanSpringLoadComposeSettings(pStart, NULL, FALSE, TRUE))
        {
            psl->Reset();
        }
    }

Cleanup:   
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pCaret );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );

    RRETURN ( hr );
}


//+---------------------------------------------------------------------------
//
//  CDeleteCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT
CDeleteCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK;
    INT                 iSegmentCount;
    IMarkupPointer *    pStart = NULL;
    IMarkupPointer *    pEnd = NULL;
    IHTMLElement   *    pElement = NULL;
    BOOL                fEditable;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    ISegmentList      * pSegmentList = NULL;
    SELECTION_TYPE      eSelectionType;
    
    // 
    // Status is disabled by default
    //
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    //
    // Get Segment list and selection type
    //
    IFC( GetSegmentList( &pSegmentList ));
    IFC( pSegmentList->GetSegmentCount( & iSegmentCount, &eSelectionType ) );

    //
    // If no segments found we're done
    //
    if (iSegmentCount == 0)
    {
        goto Cleanup;
    }

    //
    // Get the segments and check to see if we're editable
    //
    IFC(GetSegmentPointers(pSegmentList, 0, &pStart, &pEnd));

    if (eSelectionType != SELECTION_TYPE_Auto && eSelectionType != SELECTION_TYPE_Control)
    {

        IFC(FindCommonElement(pMarkupServices, GetViewServices(), pStart, pEnd, &pElement));
        if (! pElement)
            goto Cleanup;

        IFC(pViewServices->IsEditableElement(pElement, &fEditable));
        if (! fEditable)
            goto Cleanup;
    } 
    else if (eSelectionType == SELECTION_TYPE_Control)
    {
        if (!IsValidOnControl())
            goto Cleanup; // disable
    }   

    //
    // Cannot delete or cut unless the range is in the same flow layout
    //
    if ( PointersInSameFlowLayout( pStart, pEnd, NULL, pViewServices ) )
    {
        pCmd->cmdf = MSOCMDSTATE_UP; 
    }

Cleanup:
    ReleaseInterface( pSegmentList );
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    ReleaseInterface(pElement);
    RRETURN(hr);
}





//+------------------------------------------------------------------------------------
//
// Method: DeleteCharacter
//
// Synopsis: Delete one character or a NOSCOPE element in fLeftBound direction
//           used by backspace and delete.
//
// General Algorithm:
//      Peek fLeftBound, if context is:
//      - TEXT: call MoveUnit() if we have not crossed a block element
//      - NOSCOPE: move passed it and we're done
//      - NONE: we're done
//      - else move and continue to Peek fLeftBound
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::DeleteCharacter( 
                 IMarkupPointer* pPointer, 
                 BOOL fLeftBound, 
                 BOOL fWordMode,
                 IMarkupPointer* pBoundary )
{
    CHTMLEditor     * pEditor = GetEditor();
    HRESULT           hr = S_OK;
    IMarkupPointer  * pStart = NULL ;
    IMarkupPointer  * pEnd = NULL ;
    IMarkupServices * pMarkup = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    IHTMLElement    * pCurElement = NULL ;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    long            charCount;
    BOOL            fMoved = FALSE;
    BOOL            fCrossedBlock = FALSE;
    BOOL            fHasLayout = FALSE;
    DWORD           dwBreaks;
    OLECHAR         ch;

    IFC( pMarkup->CreateMarkupPointer( & pStart ));
    IFC( pMarkup->CreateMarkupPointer( & pEnd ));
    IFC( pEnd->MoveToPointer( pPointer ));
    IFC( pStart->MoveToPointer( pEnd ));

    while (! fMoved)
    {
        ClearInterface( & pCurElement );
        charCount = 1;

        //
        // Peek fLeftBound:
        //        

        if ( fLeftBound )
        {
            IFC( pViewServices->LeftOrSlave(pStart, FALSE, & context, &pCurElement , & charCount, &ch ));
        }
        else
        {
            IFC( pViewServices->RightOrSlave(pStart, FALSE, & context, &pCurElement , & charCount, NULL ));
        }

        switch (context)
        {
            case CONTEXT_TYPE_Text:

                //
                // Call MoveUnit() according to fLeftBound, only if we
                // have not crossed block elements already.
                //

                if (! fCrossedBlock && !fWordMode)
                {
#ifndef NO_UTF16
                    //
                    // NOTE (cthrash) For IE5.1, we've elected to leave the symantic of
                    // MOVEUNIT_PREVCHAR/MOVEUNIT_NEXTCHAR unchanged, i.e. the method will
                    // continue to move the markup pointer by one TCHAR.  This means that we
                    // have the possibility of splitting a high/low surrogate pair.  It is up
                    // to the consumer of these methods to prevent an improper splitting of
                    // surrogates.
                    //
                            
                    if (fLeftBound)
                    {
                        IFC( pStart->MoveUnit( MOVEUNIT_PREVCHAR )); // Use PREVCHAR for backspace

                        if (IsLowSurrogateChar(ch))
                        {
                            IFC( pViewServices->LeftOrSlave(pStart, FALSE, & context, &pCurElement , & charCount, &ch ));

                            if (context == CONTEXT_TYPE_Text && IsHighSurrogateChar(ch))
                            {
                                IFC( pStart->MoveUnit( MOVEUNIT_PREVCHAR )); // Use PREVCHAR for backspace
                            }                                
                        }
                    }
                    else
                    {
                        IFC( pStart->MoveUnit( MOVEUNIT_NEXTCLUSTEREND ));   // Use NEXTCLUSTEREND for delete
                    }
#else
                    IFC( pStart->MoveUnit( fLeftBound  ? MOVEUNIT_PREVCHAR             // Use PREVCHAR for backspace
                                                       : MOVEUNIT_NEXTCLUSTEREND ));   // Use NEXTCLUSTEREND for delete                
#endif // !NO_UTF16
                }
                
                //
                // We're done
                //
                
                fMoved = TRUE;
                break;

            case CONTEXT_TYPE_None:
                //
                // Can't delete, we're done
                //
                goto Cleanup;

            case CONTEXT_TYPE_NoScope:
                fWordMode = FALSE;

                if ( fCrossedBlock )
                {
                    //
                    // We've passed a block, don't eat any noscopes. We're done
                    //
                    fMoved = TRUE;
                    break;
                }

                //
                // Do the actual move to pass the NOSCOPE element, we're done
                //
                if ( fLeftBound )
                {
                    IFC( GetViewServices()->LeftOrSlave(pStart, TRUE, NULL, NULL , NULL, NULL ));
                }
                else
                {
                    IFC( GetViewServices()->RightOrSlave(pStart, TRUE, NULL, NULL , NULL, NULL ));
                }

                fMoved = TRUE;
                break;

            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
                //
                // Handle Tables, Positioned elements etc.
                //
                if ( pCurElement )
                {
                    //
                    // In this case we remove all block phrase elements (RT, RP,...).
                    //
                    CEditPointer tStart(pEditor, pStart);
                    DWORD dwBreakCondition = static_cast<DWORD>(BREAK_CONDITION_ANYTHING);
                    tStart.Scan(fLeftBound ? LEFT : RIGHT, dwBreakCondition, &dwBreakCondition, NULL, NULL, NULL);
                    if ( dwBreakCondition & BREAK_CONDITION_BlockPhrase)
                    {

                        CUndoUnit   undoUnit( pEditor );
                        IFC( undoUnit.Begin(IDS_EDUNDOTEXTDELETE) );

                        IFC( pMarkup->RemoveElement( pCurElement ) );
                        // Element removed, we're done
                        goto Cleanup;
                    }
                    else
                    {
                        // This isn't a block phrase element, move pointer to previous position
                        tStart.Move(fLeftBound ? RIGHT : LEFT, TRUE, NULL, NULL, NULL, NULL);
                    }

                    IGNORE_HR( pViewServices->IsLayoutElement( pCurElement, & fHasLayout ) );
                    if ( fHasLayout )
                    {
                        fWordMode = FALSE;
                        if (context == CONTEXT_TYPE_ExitScope)
                        {
                            //
                            // If we are getting out of an element which has layout
                            // then stop, we're done
                            //
                            goto Cleanup;
                        }
                        else
                        {
                            //
                            // If we are entering an element which has layout
                            // then delete the element. If we've passed a block
                            // already, we do not want to delete though, we're done
                            //
                            if ( ! fCrossedBlock )
                            {
                                IFC( pStart->MoveAdjacentToElement( pCurElement,
                                    fLeftBound ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
                            }
                            //
                            // We're outta here
                            //
                            fMoved = TRUE;
                            break;
                        }
                    }
                    else if (fWordMode)
                    {
                        BOOL fIsBlock = FALSE;
                    
                        IGNORE_HR( pViewServices->IsBlockElement( pCurElement, & fIsBlock ) );
                        fWordMode = !fIsBlock;
                    }
                }
                else
                {
                    fWordMode = FALSE;
                }

                //
                // Do the actual move now
                //
                if ( fLeftBound )
                {
                    IFC( GetViewServices()->LeftOrSlave(pStart, TRUE, NULL, NULL , NULL, NULL ));
                }
                else
                {
                    IFC( GetViewServices()->RightOrSlave(pStart, TRUE, NULL, NULL , NULL, NULL ));
                }

                //
                // Remember whether we have crossed a block element because
                // in such case, we don't want to eat a character
                //
                if (! fCrossedBlock )
                {
                    IFC( pViewServices->IsBlockElement( pCurElement, & fCrossedBlock ) );
                }            
                else
                {
                    IFC( pViewServices->QueryBreaks( pStart, & dwBreaks, TRUE ) ); 
                    if ( dwBreaks & BREAK_BLOCK_BREAK )
                    {
                        fMoved = TRUE;
                    }
                }            
                break;
            }
    }

    if ( fMoved )
    {
        if ( fLeftBound )
        {
            if (fWordMode)
                IFC( pStart->MoveUnit(MOVEUNIT_PREVWORDBEGIN) );
            
            if ( pBoundary )
            {
                IFC( Delete( pStart, pEnd, TRUE ));
            }
            else
            {
                IFC( Delete( pStart, pEnd ));
            }
        }
        else
        {
            if (fWordMode)
                IFC( pStart->MoveUnit(MOVEUNIT_NEXTWORDEND) );
            
            if ( pBoundary )
            {
                IFC( Delete( pEnd, pStart, TRUE ));
            }
            else
            {
                IFC( Delete( pEnd, pStart ));
            }
        }
    }

    //
    // Update the passed in pointer
    //
    IFC( pPointer->MoveToPointer( pStart ));

Cleanup:
    ReleaseInterface( pCurElement );
    ReleaseInterface( pStart ) ;
    ReleaseInterface( pEnd );

    return ( hr );
}

