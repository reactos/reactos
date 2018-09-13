//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       CHARCMD.CXX
//
//  Contents:   Implementation of character command classes.
//
//  History:    07-14-98 - ashrafm - created
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_ANCHOR_H_
#define _X_ANCHOR_H_
#include "anchor.h"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

using namespace EdUtil;

MtDefine(CCharCommand, EditCommand, "CCharCommand");
MtDefine(CFontCommand, EditCommand, "CFontCommand");
MtDefine(CForeColorCommand, EditCommand, "CForeColorCommand");
MtDefine(CBackColorCommand, EditCommand, "CBackColorCommand");
MtDefine(CFontNameCommand, EditCommand, "CFontNameCommand");
MtDefine(CFontSizeCommand, EditCommand, "CFontSizeCommand");
MtDefine(CAnchorCommand, EditCommand, "CAnchorCommand");
MtDefine(CRemoveFormatBaseCommand, EditCommand, "CRemoveFormatBaseCommand");
MtDefine(CRemoveFormatCommand, EditCommand, "CRemoveFormatCommand");
MtDefine(CUnlinkCommand, EditCommand, "CUnlinkCommand");

DefineSmartPointer(IHTMLAnchorElement);

//=========================================================================
//
// CBaseCharCommand: constructor
//
//-------------------------------------------------------------------------
CBaseCharCommand::CBaseCharCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
: CCommand(cmdId, pEd)
{
    _tagId = tagId;
}

//=========================================================================
//
// CBaseCharCommand: Apply
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::Apply( 
    CMshtmlEd       *pCmdTarget,
    IMarkupPointer  *pStart, 
    IMarkupPointer  *pEnd,
    VARIANT         *pvarargIn,
    BOOL            fGenerateEmptyTags)
{
    HRESULT            hr;
    SP_IMarkupPointer  spStartCopy;
    SP_IMarkupPointer  spEndCopy;

    IFR( CopyMarkupPointer(GetMarkupServices(), pStart, &spStartCopy) );
    IFR( CopyMarkupPointer(GetMarkupServices(), pEnd, &spEndCopy) );
    
    _pcmdtgt = pCmdTarget;
        //BUGBUG: there are some markup services issues when the two pointers are equal
    IGNORE_HR( PrivateApply(spStartCopy, spEndCopy, pvarargIn, fGenerateEmptyTags) );
    _pcmdtgt = NULL;
    
    RRETURN(hr);
}

//=========================================================================
//
// CBaseCharCommand: GetNormalizedTagId
//
// Synopsis: Converts any synomyms to a normal form
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::GetNormalizedTagId(IHTMLElement *pElement, ELEMENT_TAG_ID *ptagId)
{
    HRESULT hr;

    IFR( GetMarkupServices()->GetElementTagId(pElement, ptagId) );

    switch (*ptagId)
    {
        case TAGID_B:
            *ptagId = TAGID_STRONG;
            break;

        case TAGID_I:
            *ptagId = TAGID_EM;
            break;
    }

    return S_OK;    
}

//=========================================================================
//
// CBaseCharCommand: RemoveSimilarTags
//
// Synopsis: Removes all similar tags contained within
//
//-------------------------------------------------------------------------
HRESULT
CBaseCharCommand::RemoveSimilarTags(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spCurrent;
    SP_IMarkupPointer   spLimit;
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagId;
    MARKUP_CONTEXT_TYPE context;
    INT                 iPosition;
        
    IFR( CopyMarkupPointer(GetMarkupServices(), pStart, &spCurrent) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spLimit) );

    do
    {
        IFR( Move(spCurrent, RIGHT, TRUE, &context, &spElement) );
        
        switch (context)
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
                IFR( GetNormalizedTagId(spElement, &tagId) );
                if (tagId == _tagId)
                {   
                    if (context == CONTEXT_TYPE_EnterScope)
                    {
                        IFR( spLimit->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeEnd) );
                        IFR( OldCompare( spLimit, pEnd, &iPosition) );                    
                    }
                    else
                    {
                        IFR( spLimit->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterBegin) );
                        IFR( OldCompare( pStart, spLimit, &iPosition) );
                    }

                    if (iPosition == RIGHT)
                    {
                        IFR( RemoveTag(spElement, pvarargIn) );
                    }
                }
        }
        IFR( OldCompare( spCurrent, pEnd, &iPosition) );
    } 
    while (iPosition == RIGHT);

    RRETURN(hr);
}

BOOL
CBaseCharCommand::IsBlockOrLayout(IHTMLElement *pElement)
{
    HRESULT hr;
    BOOL    bResult = FALSE;

    Assert(pElement);
    
    IFC( GetViewServices()->IsBlockElement(pElement, &bResult) );
    
    if (!bResult)
        IFC( GetViewServices()->IsLayoutElement(pElement, &bResult) );

    if (!bResult)
    {
        ELEMENT_TAG_ID  tagId;

        IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
        return (tagId == TAGID_RT);
    }

Cleanup:    
    return bResult;
}

//=========================================================================
//
// CBaseCharCommand: ExpandCommandSegment
//
// Synopsis: Expands the segment to contain the maximum bolded segment
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::ExpandCommandSegment(IMarkupPointer *pStart, Direction direction, VARIANT *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spCurrent;
    SP_IHTMLElement     spElement;
    MARKUP_CONTEXT_TYPE context;
    ELEMENT_ADJACENCY   elemAdj;
    ELEMENT_TAG_ID      tagId;
    BOOL                fMustCheckEachPosition = FALSE;

    Assert(direction == LEFT || direction == RIGHT);

    // TODO: maybe we can be smarter with character influence that is not from
    // formatting tags [ashrafm]

    IFR( CopyMarkupPointer(GetMarkupServices(), pStart, &spCurrent) );

    for (;;)
    {
        IFR( Move(spCurrent, direction, TRUE, &context, &spElement) );
        
        switch (context)
        {
            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_NoScope:
                if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                    goto Cleanup;

                break;

            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
                if (context == CONTEXT_TYPE_ExitScope && spElement != NULL)
                {                    
                    // Make sure we can't expand out of an instrinsic control
                    if (IsIntrinsic(GetMarkupServices(), spElement))
                    {
                        // We just exited an instrinsic, we don't continue
                        goto Cleanup;
                    }                    
                }
                
                if (fMustCheckEachPosition)
                {   
                    IFC( pStart->MoveToPointer(spCurrent) );
                    
                    if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                        goto Cleanup;                
                }
                else
                {
                    IFC(GetNormalizedTagId(spElement, &tagId));
                    if (tagId == _tagId)
                    {
                        if (context == CONTEXT_TYPE_EnterScope)
                        {
                            if (tagId == TAGID_FONT)
                            {
                                if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                                    goto Cleanup;
                            }
                            
                            elemAdj = (direction == LEFT) ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd;
                            IFC( spCurrent->MoveAdjacentToElement(spElement, elemAdj) );
                        }
                        IFC( pStart->MoveToPointer(spCurrent) );
                    }
                    else if (spElement == NULL)
                    {
                        goto Cleanup;
                    }                
                    else if (IsBlockOrLayout(spElement))
                    {
                        fMustCheckEachPosition = TRUE;
                    }
                }
                break;
                
            case CONTEXT_TYPE_None:
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);    
}

//=========================================================================
//
// CBaseCharCommand: CreateAndInsert
//
// Synopsis: Creates and inserts the specified element
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::CreateAndInsert(ELEMENT_TAG_ID tagId, IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;

    IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
    IFR( GetEditor()->InsertElement(spElement, pStart, pEnd) );

    if (ppElement)
    {
        *ppElement = spElement;
        (*ppElement)->AddRef();
    }

    RRETURN(hr);    
}

//=========================================================================
//
// CBaseCharCommand: InsertTags
//
// Synopsis: Inserts the commands tags
//
//-------------------------------------------------------------------------

HRESULT
CBaseCharCommand::InsertTags(IMarkupPointer *pStart, IMarkupPointer *pEnd, IMarkupPointer *pLimit, VARIANT *pvarargIn, BOOL fGenerateEmptyTags)
{
    HRESULT             hr;
    SP_IMarkupPointer   spCurrentStart;
    SP_IMarkupPointer   spCurrentEnd;
    INT                 iPosition;
    BOOL                bEqual;

    //
    // Optimize for springloader case
    //
    
    IFC( pStart->IsEqualTo(pEnd, &bEqual) );
    if (bEqual)
    {
        if (fGenerateEmptyTags)
            IFC( InsertTag(pStart, pStart, pvarargIn) );

        return S_OK; // done
    }

    //
    // General case
    //
    
    IFC( CopyMarkupPointer(GetMarkupServices(), pStart, &spCurrentStart) );
    IFC( spCurrentStart->SetGravity(POINTER_GRAVITY_Right) );
    
    IFC( CopyMarkupPointer(GetMarkupServices(), pStart, &spCurrentEnd) );
    IFC( spCurrentEnd->SetGravity(POINTER_GRAVITY_Left) );
    
    for (;;)
    {
        //
        // Find start of command
        //

        IFC( FindTagStart(spCurrentStart, pvarargIn, pEnd) );
        if (hr == S_FALSE)  
        {
            hr = S_OK;
            goto Cleanup; // we're done
        }
 
        //
        // Find end of command
        //

        IFC( spCurrentEnd->MoveToPointer(spCurrentStart) );
        IFC( FindTagEnd(spCurrentStart, spCurrentEnd, pLimit) );
        
        //
        // Check for empty tags
        //

        IFC( OldCompare( spCurrentStart, spCurrentEnd, &iPosition) );
        if (iPosition != RIGHT)
        {
            if (iPosition == SAME && fGenerateEmptyTags)
            {
                IFC( InsertTag(spCurrentStart, spCurrentEnd, pvarargIn) );
            }
            continue; // done
        }

        //
        // Insert the tag
        //
        IFC( InsertTag(spCurrentStart, spCurrentEnd, pvarargIn) );
        IFC( spCurrentStart->MoveToPointer(spCurrentEnd) );
    }

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CBaseCharCommand: FindTagStart
//
// Synopsis: finds the start of the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::FindTagStart(IMarkupPointer *pCurrent, VARIANT *pvarargIn, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    CEditPointer        ep(GetEditor());
    DWORD               dwFound;
    DWORD               dwSearch;
    
    IFR( ep->MoveToPointer(pCurrent) );
    IFR( ep.SetBoundary(NULL, pEnd) );

    dwSearch = BREAK_CONDITION_Text | BREAK_CONDITION_NoScopeSite;

    for (;;)
    {
        IFR( ep.Scan(RIGHT, dwSearch, &dwFound, NULL, NULL, NULL, SCAN_OPTION_SkipControls) );
        if (ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            return S_FALSE; // done

        if (!IsCmdInFormatCache(ep, pvarargIn))
        {
            IFR( ep.Scan(LEFT, dwSearch, &dwFound, NULL, NULL, NULL, SCAN_OPTION_SkipControls) );
            IFR( pCurrent->MoveToPointer(ep) );
            break;
        }        
    }

    return S_OK;
}
    
//=========================================================================
//
// CBaseCharCommand: FindTagEnd
//
// Synopsis: finds the end of the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::FindTagEnd(IMarkupPointer *pStart, IMarkupPointer *pCurrent, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spPointer;
    SP_IMarkupPointer   spTemp;
    SP_IObjectIdentity  spIdent;
    INT                 iPosition;
    MARKUP_CONTEXT_TYPE context;
    CEditPointer        epTest(GetEditor());
    DWORD               dwFound;

    IFR( GetMarkupServices()->CreateMarkupPointer(&spPointer) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spTemp) );
    for (;;)
    {
        //
        // Check for termination
        //
        
        IFR( OldCompare( pCurrent, pEnd, &iPosition) );
        if (iPosition != RIGHT)
        {
            if (iPosition == LEFT)
            {
                // If we didn't skip text, don't worry about it - otherwise, fix it up
                IFR( GetViewServices()->LeftOrSlave(pCurrent, FALSE, &context, NULL, NULL, NULL) );
                
                if (context == CONTEXT_TYPE_Text)
                    IFR( pCurrent->MoveToPointer(pEnd) );
            }
                
            return S_OK; // done;
        }

        //
        // Try extending the tag some more
        //
        IFR( Move(pCurrent, RIGHT, TRUE, &context, &spElement) );
        switch (context)
        {
            case CONTEXT_TYPE_EnterScope:
                if (IsBlockOrLayout(spElement))
                {
                    IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                    return S_OK; // must end tag here
                }
                else
                {
                    ELEMENT_TAG_ID tagId;

                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                    if (tagId == TAGID_A)
                    {
                        IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                        return S_OK; // must end tag here
                    }
                }

                //
                // For font tags, try pushing the tag end back
                //

                if (CanPushFontTagBack(spElement))
                {
                    IFR( spTemp->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                    IFR( GetMarkupServices()->RemoveElement(spElement) );
                    IFR( GetEditor()->InsertElement(spElement, pEnd, spTemp) );
                    continue;
                }

                IFR( spPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                IFR( ClingToText(spPointer, LEFT, NULL) );
                IFR( OldCompare( spPointer, pEnd, &iPosition) );
                if (iPosition == LEFT)
                {
                    IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                    return S_OK; // must end here
                }

                // 
                // Before we jump over an element, make sure than it contains no block/layout
                //

                IFR( epTest->MoveToPointer(pCurrent) );
                IFR( epTest.SetBoundary(pCurrent, spPointer) );
                IFR( epTest.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_TEXT, &dwFound, 
                                 NULL, NULL, NULL, SCAN_OPTION_SkipControls) );

                if (!epTest.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
                    return S_OK;

                IFR( pCurrent->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                break;
                
            case CONTEXT_TYPE_ExitScope:
                if (IsBlockOrLayout(spElement))
                {
                    IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                    return S_OK; // must end tag here
                }
                else
                {
                    ELEMENT_TAG_ID tagId;

                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
                    if (tagId == TAGID_A)
                    {
                        IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                        return S_OK; // must end tag here
                    }
                }

                //
                // For font tags, try pushing the tag end back
                //

                if (CanPushFontTagBack(spElement))
                {
                    IFR( spTemp->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                    IFR( GetMarkupServices()->RemoveElement(spElement) );
                    IFR( GetEditor()->InsertElement(spElement, spTemp, pStart) );
                    continue;
                }
                
                //
                // Try to extend the left of the command segment to avoid overlap
                //
                
                IFR( spPointer->MoveToPointer(pStart) );
                IFR( GetViewServices()->LeftOrSlave(spPointer, TRUE, &context, NULL, NULL, NULL) );
                
                if (context == CONTEXT_TYPE_ExitScope)
                {
                    IFR( GetViewServices()->CurrentScopeOrSlave(pCurrent, &spElement) );

                    if (spElement != NULL)
                    {
                        IFR( GetViewServices()->CurrentScopeOrSlave(spPointer, &spElement) );
                        
                        IFR( spElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
                        IFR( spIdent->IsEqualObject(spElement) );
                        if (hr == S_OK)
                        {
                            IFR( pStart->MoveToPointer(spPointer) );
                            continue;
                        }
                    }                        
                        
                }
                IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                return S_OK; // by default we end the tag
                                
            case CONTEXT_TYPE_NoScope:
            case CONTEXT_TYPE_Text:
                // do nothing - skipping over text is ok
                continue; 
        }
        
    }

    return S_OK;
}

BOOL 
CBaseCharCommand::CanPushFontTagBack(IHTMLElement *pElement)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;
    UINT            iCount;
    CVariant        var;
    
    if (_tagId != TAGID_FONT)
        goto Cleanup;

    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    if (tagId != TAGID_FONT)
        goto Cleanup;

    IFC( GetViewServices()->GetElementAttributeCount(pElement, &iCount) );
    if (iCount > 1)
        goto Cleanup;

    switch (_cmdId)
    {
        case IDM_FORECOLOR:
            IFR( pElement->getAttribute(_T("color"), 0, &var) )
            break;

        case IDM_BACKCOLOR:
            {
                SP_IHTMLStyle spStyle;

                IFR( pElement->get_style(&spStyle) );
                IFR( spStyle->getAttribute(_T("backgroundColor"), 0, &var) )
            }
            break;
            
        case IDM_FONTSIZE:
            IFR( pElement->getAttribute(_T("size"), 0, &var) )
            break;
            
        case IDM_FONTNAME:
            IFR( pElement->getAttribute(_T("face"), 0, &var) )
            break;
    }
    if (!var.IsEmpty() && !(V_VT(&var) == VT_BSTR && V_BSTR(&var) == NULL))
        return TRUE;
    

Cleanup:
    return FALSE;
}


HRESULT
CBaseCharCommand::ApplyCommandToWord(VARIANT      * pvarargIn,
                                     VARIANT      * pvarargOut,
                                     ISegmentList * pSegmentList,
                                     BOOL           fApply)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IMarkupPointer    * pmpStart = NULL;
    IMarkupPointer    * pmpEnd = NULL;
    CSegmentListIter    iter;
    int                 iSegmentCount;
    BOOL                fInWord = FALSE;
    HRESULT             hr = S_FALSE;

    Assert(pSegmentList);

    if (pvarargOut)
        goto Cleanup;

    IFC( pSegmentList->GetSegmentCount(&iSegmentCount, NULL ) );
    if (iSegmentCount != 1)
        goto Cleanup;

    IFC( iter.Init(pMarkupServices, GetViewServices(), pSegmentList) );

    IFC( iter.Next(&pmpStart, &pmpEnd) );
    Assert(pmpStart && pmpEnd);

    // Check to see if we are inside a word, and if so expand markup pointers.
    hr = THR(EdUtil::ExpandToWord(pMarkupServices, GetViewServices(), pmpStart, pmpEnd));
    if (hr)
        goto Cleanup;

    // We now know we are inside a word.
    if (fApply)
        hr = THR(Apply(_pcmdtgt, pmpStart, pmpEnd, pvarargIn));
    else
    {
        // BUGBUG: kill this hack [ashrafm]
        Assert(_tagId != TAGID_FONT);
        hr = THR(((CCharCommand *)this)->Remove(_pcmdtgt, pmpStart, pmpEnd));
    }

    fInWord = TRUE;

Cleanup:

    if (!hr && !fInWord)
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}


//=========================================================================
//
// CBaseCharCommand: PrivateApply
//
// Synopsis: Applies the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::PrivateApply( 
    IMarkupPointer    *pStart, 
    IMarkupPointer    *pEnd,
    VARIANT           *pvarargIn,
    BOOL              fGenerateEmptyTags)
{
    HRESULT             hr;
    BOOL                bEqual;
    SP_IHTMLElement     spElement;
    MARKUP_CONTEXT_TYPE context;
    SP_IMarkupPointer   spLimit;
    
    //
    // For site selected controls that are color commands, apply the attribute directly.
    //
    if ( ( _cmdId == IDM_FORECOLOR ) && 
         ( GetEditor()->GetSelectionManager()->GetSelectionType() == SELECTION_TYPE_Control ) )
    {
        IFC( GetViewServices()->RightOrSlave(pStart, FALSE, & context, & spElement, NULL, NULL ));
        Assert( (context == CONTEXT_TYPE_EnterScope) || (context == CONTEXT_TYPE_NoScope));
        IFC( DYNCAST( CForeColorCommand, this )->InsertTagAttribute(spElement, pvarargIn));
    }
    else
    {
        if (fGenerateEmptyTags)
        {
            IFR( pStart->IsEqualTo(pEnd, &bEqual) );
            if (bEqual)
            {
                IFR( InsertTag(pStart, pStart, pvarargIn) );

                return S_OK; // done
            }
        }

        //
        // First expand to the find the maximum segment that will be under the influence of
        // this type of command.  
        //

        IFR( ExpandCommandSegment(pStart, LEFT, pvarargIn) );
        IFR( ExpandCommandSegment(pEnd, RIGHT, pvarargIn) );

        IFR( CopyMarkupPointer(GetMarkupServices(), pEnd, &spLimit) );

        //
        // Remove all similar tags contained within the segment
        //

        IFR( RemoveSimilarTags(pStart, pEnd, pvarargIn) );

        //
        // Next, cling both sides of the expanded segment to text.  
        //
     
        IFR( ClingToText(pStart, RIGHT, pEnd, TRUE) );
        IFR( ClingToText(pEnd, LEFT, pStart, TRUE) );
        
        //
        // Insert tags in current segment
        //

        IFR( InsertTags(pStart, pEnd, (_tagId == TAGID_FONT) ? static_cast<IMarkupPointer *>(spLimit) : pEnd, pvarargIn, fGenerateEmptyTags) );
    }
Cleanup:    
    return S_OK;
}
                          

//=========================================================================
//
// CCharCommand: constructor
//
//-------------------------------------------------------------------------
CCharCommand::CCharCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
: CBaseCharCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
//
// CCharCommand: PrivateExec
//
//-------------------------------------------------------------------------

HRESULT
CCharCommand::PrivateExec( 
        DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT             hr;
    INT                 iSegmentCount;
    OLECMD              cmd;
    SP_ISegmentList     spSegmentList;
    CSegmentListIter    iter;
    IMarkupPointer      *pStart;
    IMarkupPointer      *pEnd;
    CSpringLoader       *psl = GetSpringLoader();
    CUndoUnit           undoUnit(GetEditor());

    IFR( GetSegmentList(&spSegmentList) );
    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, NULL) );

    if (iSegmentCount <= 0) /// nothing to do
    {
        hr = S_OK;
        goto Cleanup;
    }


    IFR( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
                           
    IFR( PrivateQueryStatus(&cmd, NULL ));
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    // Give the current word a chance to intercept the command.
    hr = THR(ApplyCommandToWord(pvarargIn, pvarargOut, spSegmentList, cmd.cmdf == MSOCMDSTATE_UP));
    if (hr != S_FALSE)
        goto Cleanup;

    // Give the spring loader a chance to intercept the command.
    hr = THR(psl->PrivateExec(_cmdId, pvarargIn, pvarargOut, spSegmentList));
    if (hr != S_FALSE)
        goto Cleanup;

    hr = S_OK;
    
    IFC( iter.Init(GetMarkupServices(), GetViewServices(), spSegmentList) );

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );

        if (hr == S_FALSE)
        {
            hr = S_OK; // proper termination
            break;
        }

        if (cmd.cmdf == MSOCMDSTATE_UP)
            IFC( PrivateApply(pStart, pEnd, NULL, FALSE) )
        else
            IFC( PrivateRemove(pStart, pEnd) );
    }
    

Cleanup:

    RRETURN(hr);
}

//=========================================================================
// CCharCommand: PrivateQueryStatus
//
//  Synopsis: Returns the format data for the first character in the segment
//            list.  NOTE: this is the same behavior as Word97 and IE401.
//
//-------------------------------------------------------------------------
HRESULT
CCharCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )

{
    HRESULT             hr;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_ISegmentList     spSegmentList;
    CSpringLoader       *psl = GetSpringLoader();
    SELECTION_TYPE      eSelectionType;
    INT                 iSegmentCount;

    IFR( GetSegmentList( &spSegmentList ));

    // Give the spring loader a chance to intercept the query.
    hr = THR(psl->PrivateQueryStatus(_cmdId, pCmd));
    if (hr != S_FALSE)
        RRETURN(hr);

    IFR( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        RRETURN(hr);
        
    //
    // Return state of first character
    //
    IFR( GetSegmentPointers(spSegmentList, 0, &spStart, &spEnd) );

    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );
    if (eSelectionType == SELECTION_TYPE_Selection ||
        eSelectionType == SELECTION_TYPE_Auto )
    {
        IFR(ClingToText(spStart, RIGHT, NULL));
    }

    if (IsCmdInFormatCache(spStart, NULL))
        pCmd->cmdf = MSOCMDSTATE_DOWN;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;

    RRETURN(hr);
}

//=========================================================================
//
// CCharCommand: Remove
//
//-------------------------------------------------------------------------
HRESULT 
CCharCommand::Remove( 
    CMshtmlEd       *pCmdTarget,
    IMarkupPointer  *pStart, 
    IMarkupPointer  *pEnd)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStartCopy;
    SP_IMarkupPointer   spEndCopy;

    IFR( CopyMarkupPointer(GetMarkupServices(), pStart, &spStartCopy) );
    IFR( spStartCopy->SetGravity(POINTER_GRAVITY_Right) );

    IFR( CopyMarkupPointer(GetMarkupServices(), pEnd, &spEndCopy) );     
    IFR( spEndCopy->SetGravity(POINTER_GRAVITY_Left) );

    _pcmdtgt = pCmdTarget;
    hr = THR( PrivateRemove(spStartCopy, spEndCopy) );
    _pcmdtgt = NULL;
    
    RRETURN(hr);
}


//=========================================================================
//
// CBaseCharCommand: PrivateRemove
//
// Synopsis: Removes the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::PrivateRemove( 
    IMarkupPointer    *pStart, 
    IMarkupPointer    *pEnd,
    VARIANT           *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStartCopy;
    SP_IMarkupPointer   spEndCopy;
    BOOL                bEqual;

    IFR( CopyMarkupPointer(GetMarkupServices(), pStart, &spStartCopy) );
    IFR( CopyMarkupPointer(GetMarkupServices(), pEnd, &spEndCopy) );
    
    //
    // First expand to the find the maximum segment that will be under the influence of
    // this type of command.  
    //
    
    IFR( ExpandCommandSegment(spStartCopy, LEFT, pvarargIn) );
    IFR( ExpandCommandSegment(spEndCopy, RIGHT, pvarargIn) );

    //
    // Remove all similar tags contained within the segment
    //

    IFR( RemoveSimilarTags(spStartCopy, spEndCopy, pvarargIn) );
    
    //
    // Next, re-insert segments outside selection   
    //

    IFR(pStart->IsEqualTo(spStartCopy, &bEqual) );
    if (!bEqual)
        IFR( PrivateApply(spStartCopy, pStart, pvarargIn, FALSE) );
 
    IFR(pEnd->IsEqualTo(spEndCopy, &bEqual) );
    if (!bEqual)
        IFR( PrivateApply(pEnd, spEndCopy, pvarargIn, FALSE) );
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       AreAttributesEqual
//
//  Synopsis:     Returns true if attributes of both tags are equal
//
//----------------------------------------------------------------------------
BOOL 
CCharCommand::AreAttributesEqual(IHTMLElement *pElementLeft, IHTMLElement *pElementRight)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagIdLeft, tagIdRight;

    // TODO: check attributes as well
    IFC( GetMarkupServices()->GetElementTagId(pElementLeft, &tagIdLeft) );
    IFC( GetMarkupServices()->GetElementTagId(pElementRight, &tagIdRight) );

    return (tagIdLeft == tagIdRight && tagIdLeft != TAGID_FONT);
    
Cleanup:
    return FALSE;
}

//=========================================================================
//
// CBaseCharCommand: TryTagMerge
//
//-------------------------------------------------------------------------

HRESULT 
CBaseCharCommand::TryTagMerge(
    IMarkupPointer *pCurrent)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElementLeft;
    SP_IHTMLElement         spElementRight;
    MARKUP_CONTEXT_TYPE     context;
    ELEMENT_TAG_ID          tagId;
    
    IFR( GetViewServices()->RightOrSlave(pCurrent, FALSE, &context, &spElementRight, NULL, NULL) );
    if (context == CONTEXT_TYPE_EnterScope)
    {
        IFR( GetViewServices()->LeftOrSlave(pCurrent, FALSE, &context, &spElementLeft, NULL, NULL) );
        if (context == CONTEXT_TYPE_EnterScope) 
        {
            // TODO: check attributes as well. [ashrafm]            
            
            if (AreAttributesEqual(spElementLeft, spElementRight))
            {
                SP_IMarkupPointer spStart, spEnd;

                // Merge tags
    
                IFR( GetMarkupServices()->CreateMarkupPointer(&spStart) );
                IFR( GetMarkupServices()->CreateMarkupPointer(&spEnd) );

                IFR( spStart->MoveAdjacentToElement(spElementLeft, ELEM_ADJ_BeforeBegin) );
                IFR( spEnd->MoveAdjacentToElement(spElementRight, ELEM_ADJ_AfterEnd) );

                // TODO: this call needs to change when font tag merging is added
                IFR( GetMarkupServices()->GetElementTagId(spElementLeft, &tagId) );
                IFR( CreateAndInsert(tagId, spStart, spEnd, NULL) );

                IFR( GetMarkupServices()->RemoveElement(spElementLeft) );
                IFR( GetMarkupServices()->RemoveElement(spElementRight) );
            }
        }
        
    }
    

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       IsCmdInFormatChache
//
//  Synopsis:     Returns S_OK if the command is in the format cache
//                data.  Otherwise, S_FALSE is returned.
//
//----------------------------------------------------------------------------

BOOL
CCharCommand::IsCmdInFormatCache(IMarkupPointer  *pMarkupPointer,
                                 VARIANT         *pvarargIn)
{
    HRESULT             hr;
    HTMLCharFormatData  chFmtData;
    BOOL                bResult = FALSE;

    IFC(GetViewServices()->GetCharFormatInfo(pMarkupPointer, CHAR_FORMAT_FontStyle, &chFmtData));
    switch (_cmdId)
    {
        case IDM_BOLD:
            bResult = chFmtData.fBold;
            break;

        case IDM_ITALIC:
            bResult = chFmtData.fItalic;
            break;

        case IDM_UNDERLINE:
            bResult = chFmtData.fUnderline;
            break;

        case IDM_STRIKETHROUGH:
            bResult = chFmtData.fStrike;
            break;

        case IDM_SUBSCRIPT:
            bResult = chFmtData.fSubScript;
            break;

        case IDM_SUPERSCRIPT:
            bResult = chFmtData.fSuperScript;
            break;

        default:
            Assert(0); // unsupported cmdId
    }


Cleanup:
    return bResult;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: ConvertFormatDataToVariant
//
//----------------------------------------------------------------------------
HRESULT 
CCharCommand::ConvertFormatDataToVariant(
    HTMLCharFormatData      &chFmtData,
    VARIANT                 *pvarargOut )
{
    AssertSz(0, "CCharCommand::ConvertFormatDataToVariant not implemented");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: IsFormatDataEqual
//
//----------------------------------------------------------------------------

BOOL
CCharCommand::IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b)
{
    AssertSz(0, "CCharCommand::IsFormatDataEqual not implemented");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: IsVariantEqual
//
//----------------------------------------------------------------------------
BOOL 
CCharCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    AssertSz(0, "CCharCommand::IsVariantEqual not implemented");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: InsertTag
//
//----------------------------------------------------------------------------
HRESULT 
CCharCommand::InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    RRETURN( CreateAndInsert(_tagId, pStart, pEnd, NULL) );
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: RemoveTag
//
//----------------------------------------------------------------------------

HRESULT 
CCharCommand::RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT           hr;
    SP_IMarkupPointer spLeft, spRight;

    // TODO: check if it has attributes and convert to span if it does not

    IFR( GetMarkupServices()->CreateMarkupPointer(&spLeft) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spRight) );

    IFR( spLeft->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
    IFR( spRight->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFR( GetMarkupServices()->RemoveElement(pElement) );

    IFR( TryTagMerge(spLeft) );
    IFR( TryTagMerge(spRight) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: InsertTag
//
//----------------------------------------------------------------------------
HRESULT 
CCharCommand::InsertStyleAttribute(IHTMLElement *pElement)
{
    HRESULT         hr;
    SP_IHTMLStyle   spStyle;

    IFR( pElement->get_style(&spStyle) );
    switch (_cmdId)
    {
        case IDM_BOLD:
            IFR( spStyle->put_fontWeight(_T("bold")) );
            break;
            
        case IDM_ITALIC:
            VARIANT var;

            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = _T("italic");
            
            IFR( spStyle->setAttribute(_T("fontStyle"), var, 0) );
            break;

        case IDM_UNDERLINE:
            IFR( spStyle->put_textDecorationUnderline(VB_TRUE) );
            break;
    }

    return S_OK;
}

HRESULT 
CCharCommand::RemoveStyleAttribute(IHTMLElement *pElement)
{
    HRESULT         hr;
    SP_IHTMLStyle   spStyle;

    IFR( pElement->get_style(&spStyle) );
    switch (_cmdId)
    {
        case IDM_BOLD:
            IFR( spStyle->put_fontWeight(_T("normal")) );
            break;
            
        case IDM_ITALIC:
            IFR( spStyle->removeAttribute(_T("fontStyle"), 0, NULL) );
            break;

        case IDM_UNDERLINE:
            IFR( spStyle->put_textDecorationUnderline(VB_FALSE) );
            break;
    }

    return S_OK;
}

//=========================================================================
//
// CFontCommand: constructor
//
//-------------------------------------------------------------------------
CFontCommand::CFontCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor *pEd )
: CBaseCharCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
//
// CFontCommand: GetCharFormatFamily
//
//-------------------------------------------------------------------------
CHAR_FORMAT_FAMILY
CFontCommand::GetCharFormatFamily()
{
    switch (_cmdId)
    {
    case IDM_FONTNAME:
        return CHAR_FORMAT_FontName;
        
    case IDM_FONTSIZE:
        return CHAR_FORMAT_FontInfo;

    case IDM_FORECOLOR:
        return CHAR_FORMAT_ColorInfo;

    case IDM_BACKCOLOR:
        return CHAR_FORMAT_ColorInfo;
    }

    AssertSz(0, "Unhandled font command");
    return CHAR_FORMAT_None;
}

//+---------------------------------------------------------------------------
//
//  Method:       IsCmdInFormatChache
//
//  Synopsis:     Returns S_OK if the command is in the format cache
//                data.  Otherwise, S_FALSE is returned.
//
//----------------------------------------------------------------------------

BOOL 
CFontCommand::IsCmdInFormatCache(IMarkupPointer  *pCurrent,
                                 VARIANT         *pvarargIn)
{
    HRESULT             hr;
    HTMLCharFormatData  chFmtData;
    BOOL                bResult = FALSE;
    VARIANT             var;

    IFC( GetViewServices()->GetCharFormatInfo(pCurrent, GetCharFormatFamily(), &chFmtData) );
    IFC( ConvertFormatDataToVariant(chFmtData, &var) );

    bResult = IsVariantEqual(&var, pvarargIn);
    VariantClear(&var);

Cleanup:
    return bResult;
}

//=========================================================================
// CFontCommand: GetCommandRange
//
// Synopsis: Get the range of the font command (usually for dropdowns)
//
//-------------------------------------------------------------------------
HRESULT 
CFontCommand::GetCommandRange(VARIANTARG *pvarargOut)
{
    HRESULT hr = S_OK;

    Assert(pvarargOut && V_VT(pvarargOut) == VT_ARRAY);

    switch (_cmdId)
    {
    case IDM_FONTSIZE:
    {
        // IDM_FONTSIZE command is from form toolbar (font size combobox)
        // * V_VT(pvarargIn)  = VT_I4/VT_BSTR   : set font size
        // * V_VT(pvarargOut) = VT_I4           : request current font size setting
        // * V_VT(pvarargOut) = VT_ARRAY        : request all possible font sizes

        SAFEARRAYBOUND sabound;
        SAFEARRAY * psa;
        long l, lZoom;

        sabound.cElements = FONTMAX - FONTMIN + 1;
        sabound.lLbound = 0;
        psa = SafeArrayCreate(VT_I4, 1, &sabound);

        for (l = 0, hr = S_OK, lZoom = FONTMIN;
             l < (FONTMAX - FONTMIN + 1) && SUCCEEDED(hr);
             l++, lZoom++)
        {
            hr = THR_NOTRACE(SafeArrayPutElement(psa, &l, &lZoom));
        }

        V_ARRAY(pvarargOut) = psa;
    }
        break;

    default:
        Assert(!"CFontCommand::Exec VT_ARRAY-out mode only supported for IDM_FONTSIZE");
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}

//=========================================================================
// CFontCommand: GetSegmentListFontValue
//
// Synopsis: Returns the property if everywhere in the range.
//           Otherwise, VT_EMPTY is returned;
//-------------------------------------------------------------------------
HRESULT
CFontCommand::GetSegmentListFontValue(
    ISegmentList    *pSegmentList,
    VARIANT         *pvarargOut)
{
    HRESULT             hr;
    INT                 iSegmentCount;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    IMarkupPointer      *pStart = NULL;
    IMarkupPointer      *pEnd = NULL;
    HTMLCharFormatData  chFmtData;
    HTMLCharFormatData  chFmtDataCurrent;
    CHAR_FORMAT_FAMILY  chFmtFamily;
    CSegmentListIter    iter;
    INT                 iPosition;
    MARKUP_CONTEXT_TYPE context;
    SELECTION_TYPE      eSelectionType;
    
    Assert(pvarargOut); 

    // Default to VT_EMPTY with 
    if (pvarargOut)
    {
        V_VT(pvarargOut) = VT_NULL;
        V_BSTR(pvarargOut) = NULL; // some hosts assume we are returning a BSTR and do not check type
    }

    //
    // Return VT_EMPTY on an empty segment list
    //
    IFC( pSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );
    if (iSegmentCount <= 0)
    {
        return S_OK;       // empty segment not an error
    }

    //
    // Check first segment
    //

    IFC( GetSegmentPointers(pSegmentList, 0, &spStart, &spEnd) );
    if (eSelectionType == SELECTION_TYPE_Selection ||
        eSelectionType == SELECTION_TYPE_Auto )
    {
        IFC( ClingToText(spStart, RIGHT, NULL) );
    }

    chFmtFamily = GetCharFormatFamily();
    IFC( GetViewServices()->GetCharFormatInfo(spStart, chFmtFamily, &chFmtData) );

    //
    // Check that all other segments have the same format info
    //

    iter.Init(GetMarkupServices(), GetViewServices(), pSegmentList);

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );
        if (hr ==  S_FALSE)
        {
            // done - all format data is equal
            hr = THR(ConvertFormatDataToVariant(chFmtData, pvarargOut));
            goto Cleanup; 
        }
        if (eSelectionType == SELECTION_TYPE_Selection ||
            eSelectionType == SELECTION_TYPE_Auto )
        {
            IFC( ClingToText(pStart, RIGHT, NULL, FALSE, TRUE) );
            IFC( ClingToText(pEnd, LEFT, NULL, FALSE, TRUE) );
        }

        for (;;)
        {
            IFC( OldCompare( pStart, pEnd, &iPosition) );
            if (iPosition != RIGHT)
                break; // continue iterating through segments
                
            IFC( Move(pStart, RIGHT, TRUE, &context, NULL) );
            if (context == CONTEXT_TYPE_Text)
            {
                IFC( GetViewServices()->GetCharFormatInfo(pStart, chFmtFamily, &chFmtDataCurrent) );
                if (!IsFormatDataEqual(chFmtData, chFmtDataCurrent))
                {
                    V_VT(pvarargOut) = VT_NULL;
                    V_BSTR(pvarargOut) = NULL; // some hosts assume we are returning a BSTR and do not check type
                    goto Cleanup; // done - found format data that is not equal
                }
            }    
        }
    }

Cleanup:
    RRETURN(hr);
}


//=========================================================================
// CFontCommand: PrivateExec
//
// Synopsis: Exec for font commands
//
//-------------------------------------------------------------------------

HRESULT
CFontCommand::PrivateExec( 
        DWORD             nCmdexecopt,
    VARIANTARG *      pvarargIn,
    VARIANTARG *      pvarargOut )
{
    HRESULT             hr;
    IMarkupPointer      *pStart = NULL;
    IMarkupPointer      *pEnd = NULL;
    CSegmentListIter    iter;
    INT                 iSegmentCount;
    SP_IMarkupPointer   spSegmentLimit;
    CSpringLoader       * psl = GetSpringLoader();
    SP_ISegmentList     spSegmentList;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spPointer;
    HTMLCharFormatData  chFmtData;
    CUndoUnit           undoUnit(GetEditor());

    // Handle VT_ARRAY range requests first.
    if (pvarargOut && V_VT(pvarargOut) == VT_ARRAY)
    {
        hr = THR(GetCommandRange(pvarargOut));
        if (!hr)
        {
            // Out part successfully handled.
            pvarargOut = NULL;

            // Done?
            if (!pvarargIn)
                goto Cleanup;
        }
    }

    IFC( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, NULL) );

    if (iSegmentCount <= 0) /// nothing to do
    {
        hr = S_OK;
        if (pvarargOut)
        {
            VariantInit(pvarargOut);
            
            if (SUCCEEDED(GetActiveElemSegment(GetMarkupServices(), &spPointer, NULL)))
            {
                if (SUCCEEDED(GetViewServices()->GetCharFormatInfo(spPointer, GetCharFormatFamily(), &chFmtData)))
                {
                    IGNORE_HR(ConvertFormatDataToVariant(chFmtData, pvarargOut));
                 }
            }
        }

        goto Cleanup;
    }

    if (pvarargIn)
    {
        IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );

        // Give the current word a chance to intercept the command.
        hr = THR(ApplyCommandToWord(pvarargIn, pvarargOut, spSegmentList, TRUE));
        if (hr != S_FALSE)
            goto Cleanup;
    }

    // Give the spring loader a chance to intercept the command.
    hr = THR(psl->PrivateExec(_cmdId, pvarargIn, pvarargOut, spSegmentList));
    if (hr != S_FALSE)
        goto Cleanup;
        
    hr = S_OK;
    
    if (pvarargIn)
    {
        // Set the font property
        IFC( iter.Init(GetMarkupServices(), GetViewServices(), spSegmentList) );

        for (;;)
        {
            IFC( iter.Next(&pStart, &pEnd) );
            if (hr ==  S_FALSE)
                break;

            IFC( PrivateApply(pStart, pEnd, pvarargIn, FALSE) );
        }
        hr = S_OK;
    }

    // Get the font property
    if (pvarargOut)
    {
        IFC( GetSegmentListFontValue(spSegmentList, pvarargOut) );
    }

Cleanup:        
    RRETURN(hr);
}
//=========================================================================
// CFontCommand: PrivateQueryStatus
//
// Synopsis: Does nothing for now
//-------------------------------------------------------------------------

HRESULT
CFontCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )

{
    HRESULT             hr;
    CSpringLoader       *psl = GetSpringLoader();
    SP_ISegmentList     spSegmentList;
    CVariant            var;

    //
    // This is where the spring loader gets to intercept a command.
    //

    IFC( GetSegmentList(&spSegmentList) );
    
    IFR( psl->PrivateQueryStatus(_cmdId, pCmd) );
    if (hr != S_FALSE)
        goto Cleanup;

    //
    // Return the character format
    //

    IFR( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( GetSegmentListFontValue(spSegmentList, &var) );
    if (var.IsEmpty())
        pCmd->cmdf = MSOCMDSTATE_NINCHED;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;

Cleanup:
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: InsertTag
//
//----------------------------------------------------------------------------
HRESULT CFontCommand::InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT         hr;
    SP_IHTMLElement spElement;

    IFR( FindReuseableTag(pStart, pEnd, &spElement) );
    if (!spElement)
    {
        IFR( GetMarkupServices()->CreateElement(_tagId, NULL, &spElement) );            
        IFR( InsertTagAttribute(spElement, pvarargIn) );
        IFR( GetEditor()->InsertElement(spElement, pStart, pEnd) );
    }
    else
    {
        IFR( InsertTagAttribute(spElement, pvarargIn) );
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: FindReuseableTag
//
//----------------------------------------------------------------------------
HRESULT 
CFontCommand::FindReuseableTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart, spEnd, spTemp;
    SP_IHTMLElement     spScope, spParentScope;
    MARKUP_CONTEXT_TYPE context;
    BOOL                bEqual;
    ELEMENT_TAG_ID      tagId;

    *ppElement = NULL;

    IFR( GetMarkupServices()->CreateMarkupPointer(&spStart) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spEnd) );
    IFR( GetMarkupServices()->CreateMarkupPointer(&spTemp) );
    
    IFR( spStart->MoveToPointer(pStart) );
    IFR( ClingToText(spStart, RIGHT, pEnd) );    
    IFR( spEnd->MoveToPointer(pEnd) );
    IFR( ClingToText(spEnd, LEFT, pStart) );    
    
    IFR( GetViewServices()->CurrentScopeOrSlave(spStart, &spScope) );

    while (spScope != NULL)
    {
        IFR( spTemp->MoveAdjacentToElement(spScope, ELEM_ADJ_AfterBegin) );
        IFR( spStart->IsEqualTo(spTemp, &bEqual) );
        if (!bEqual)
            return S_OK;
        
        IFR( spTemp->MoveAdjacentToElement(spScope, ELEM_ADJ_BeforeEnd) );
        IFR( spEnd->IsEqualTo(spTemp, &bEqual) );
        if (!bEqual)
            return S_OK;

        IFR( GetMarkupServices()->GetElementTagId(spScope, &tagId) );
        if (tagId == TAGID_FONT)
        {
            *ppElement = spScope;
            (*ppElement)->AddRef();
            return S_OK;
        }
        
        IFR( Move(spStart, LEFT, TRUE, &context, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            return S_OK;
            
        IFR( Move(spEnd, RIGHT, TRUE, &context, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            return S_OK;

        IFR( spScope->get_parentElement(&spParentScope) );
        spScope = spParentScope;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: RemoveTag
//
//----------------------------------------------------------------------------
HRESULT CFontCommand::RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spLeft, spRight;
    CVariant            var;
    UINT                iCount;

    if (!pvarargIn)
        return S_OK;

    switch (_cmdId)
    {
        case IDM_FORECOLOR:
            IFR( pElement->removeAttribute(_T("color"), 0, NULL) )
            break;

        case IDM_BACKCOLOR:
            {
                SP_IHTMLStyle spStyle;

                IFR( pElement->get_style(&spStyle) );
                IFR( spStyle->removeAttribute(_T("backgroundColor"), 0, NULL) )
            }
            break;
            
        case IDM_FONTSIZE:
            IFR( pElement->removeAttribute(_T("size"), 0, NULL) )
            break;
            
        case IDM_FONTNAME:
            IFR( pElement->removeAttribute(_T("face"), 0, NULL) )
            break;
    }

    IFR( GetViewServices()->GetElementAttributeCount(pElement, &iCount) );
    if (iCount > 0)
        return S_OK; 

    // TODO: check if it has attributes and convert to span if it does not

    //
    // Remove the tag
    //

    IFR( GetMarkupServices()->CreateMarkupPointer(&spLeft) );
        IFR( GetMarkupServices()->CreateMarkupPointer(&spRight) );

    IFR( spLeft->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
    IFR( spRight->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFR( GetMarkupServices()->RemoveElement(pElement) );

    IFR( TryTagMerge(spLeft) );
    IFR( TryTagMerge(spRight) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: IsValidOnControl
//
//----------------------------------------------------------------------------
BOOL 
CFontCommand::IsValidOnControl()
{
    HRESULT         hr;
    SP_ISegmentList spSegmentList;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID  tagId;
    INT             iSegmentCount;
    
    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, NULL) );
    if (iSegmentCount <= 0)
        return FALSE;

    IFC( GetSegmentElement(spSegmentList, 0, &spElement) );
    IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

    return (tagId == TAGID_HR && _cmdId == IDM_FORECOLOR);
    
Cleanup:    
    return FALSE;
}
    

//=========================================================================
//
// CFontNameCommand: constructor
//
//-------------------------------------------------------------------------
CFontNameCommand::CFontNameCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CFontNameCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CFontNameCommand::ConvertFormatDataToVariant(
    HTMLCharFormatData &chFmtData,
    VARIANT            *pvar)
{
    HRESULT hr = S_OK;

    V_VT(pvar) = VT_BSTR;
    V_BSTR(pvar) = SysAllocString(chFmtData.szFont);
    if (V_BSTR(pvar) == NULL)
        hr = E_OUTOFMEMORY;

    RRETURN(hr);
}

//=========================================================================
// CFontNameCommand: IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CFontNameCommand::IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData 
&b)
{
    return (StrCmp(a.szFont, b.szFont) == 0);
}

//=========================================================================
// CFontNameCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CFontNameCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareFontName(pvarA, pvarB);
}

//=========================================================================
// CFontNameCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CFontNameCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT hr = S_OK;

    // Set the property
    if (pvarargIn)
        hr = THR(pElement->setAttribute(_T("FACE"), *pvarargIn, 0));

    RRETURN(hr);
}

//=========================================================================
//
// CFontSizeCommand: constructor
//
//-------------------------------------------------------------------------
CFontSizeCommand::CFontSizeCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CFontSizeCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CFontSizeCommand::ConvertFormatDataToVariant(
    HTMLCharFormatData &chFmtData,
    VARIANT            *pvar)
{
    V_VT(pvar) = VT_I4;
    V_I4(pvar) = chFmtData.wFontSize;
    RRETURN(THR(GetViewServices()->ConvertVariantFromTwipsToHTML(pvar)));
}

//=========================================================================
// CFontSizeCommand::IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CFontSizeCommand::IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData 
&b)
{
    return (a.wFontSize == b.wFontSize);
}

//=========================================================================
// CFontSizeCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CFontSizeCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareFontSize(pvarA, pvarB);
}

//=========================================================================
// CFontSizeCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CFontSizeCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT         hr;
    CVariant        var;
    CVariant        var2;
 
    if (V_VT(pvarargIn) == VT_EMPTY)
    {
        // remove all attributes
        IFC( pElement->setAttribute(_T("SIZE"), *pvarargIn, 0) );
    }
    else
    {
        IFC( var.Copy(pvarargIn) );
        IFC( GetViewServices()->ConvertVariantFromHTMLToTwips(&var) );
        IFC( var2.Copy(&var) );

        IFC( GetViewServices()->ConvertVariantFromTwipsToHTML(&var2) );
        if (V_VT(&var2) != VT_I4)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            // use attribute on the element
            hr = THR(pElement->setAttribute(_T("SIZE"), var2, 0));
        }
    }

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CBackColorCommand: constructor
//
//-------------------------------------------------------------------------
CBackColorCommand::CBackColorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CBackColorCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CBackColorCommand::ConvertFormatDataToVariant(
    HTMLCharFormatData &chFmtData,
    VARIANT            *pvar)
{
    V_VT(pvar) = VT_I4;
    V_I4(pvar) = chFmtData.dwBackColor;

    return ConvertRGBToOLEColor(pvar);
}

//=========================================================================
// CBackColorCommand: IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CBackColorCommand::IsFormatDataEqual(HTMLCharFormatData &a, 
HTMLCharFormatData &b)
{
    BOOL bResult;

    if (!a.fHasBgColor)
        bResult = !b.fHasBgColor;
    else
        bResult = (a.dwBackColor == b.dwBackColor);

    return bResult;
}

//=========================================================================
// CBackColorCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CBackColorCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareColor(pvarA, pvarB);
}

//=========================================================================
// CBackColorCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CBackColorCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT             hr;
    VARIANT             var;
    SP_IHTMLStyle       spStyle;

    IFC( pElement->get_style(&spStyle) );

    VariantInit(&var);
    if (V_VT(pvarargIn) != VT_EMPTY)
    {
        IFC( VariantCopy(&var, pvarargIn) );
        IFC( ConvertOLEColorToRGB(&var) );
        IFC( spStyle->put_backgroundColor(var) );
    }
    else
    {
        IFC( spStyle->removeAttribute(_T("backgroundColor"), 0, NULL) );
    }

Cleanup:
    VariantClear(&var);
    RRETURN(hr);
}

//=========================================================================
//
// CForeColorCommand: constructor
//
//-------------------------------------------------------------------------
CForeColorCommand::CForeColorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CForeColorCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CForeColorCommand::ConvertFormatDataToVariant(
    HTMLCharFormatData &chFmtData,
    VARIANT            *pvar)
{
    DWORD dwColor = chFmtData.dwTextColor;

    V_VT(pvar) = VT_I4;
    V_I4(pvar) = dwColor;

    return ConvertRGBToOLEColor(pvar);
}

//=========================================================================
// CForeColorCommand: IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CForeColorCommand::IsFormatDataEqual(HTMLCharFormatData &a, 
HTMLCharFormatData &b)
{
    return (a.dwTextColor == b.dwTextColor);
}


//=========================================================================
// CForeColorCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CForeColorCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareColor(pvarA, pvarB);
}

//=========================================================================
// CForeColorCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CForeColorCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT             hr;
    VARIANT             var;

    VariantInit(&var);
    if (V_VT(pvarargIn) != VT_EMPTY)
    {
        IFC( VariantCopy(&var, pvarargIn) );
        IFC( ConvertOLEColorToRGB(&var) );
    }

    // use attribute on the element
    IFC( pElement->setAttribute(_T("COLOR"), var, 0) );

Cleanup:
    VariantClear(&var);
    RRETURN(hr);
}


//=========================================================================
//
// CAnchorCommand: constructor
//
//-------------------------------------------------------------------------
CAnchorCommand::CAnchorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CBaseCharCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: ConvertFormatDataToVariant
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::ConvertFormatDataToVariant(
    HTMLCharFormatData      &chFmtData,
    VARIANT                 *pvarargOut )
{
    AssertSz(0, "CCharCommand::ConvertFormatDataToVariant not implemented");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: IsVariantEqual
//
//----------------------------------------------------------------------------
BOOL 
CAnchorCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    AssertSz(0, "CCharCommand::IsVariantEqual not implemented");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: IsVariantEqual
//
//----------------------------------------------------------------------------
BOOL
CAnchorCommand::IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b)
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: RemoveTag
//
//----------------------------------------------------------------------------
HRESULT
CAnchorCommand::RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    RRETURN(GetMarkupServices()->RemoveElement(pElement));
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: RemoveTag
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElement;
    SP_IHTMLAnchorElement   spAnchor;
    
    if (!pvarargIn || V_VT(pvarargIn) != VT_BSTR)
        return E_INVALIDARG;

    switch (_cmdId)
    {
        case IDM_BOOKMARK:
            IFR( InsertNamedAnchor(V_BSTR(pvarargIn), pStart, pEnd) ); 
            break;

        case IDM_HYPERLINK:
            IFR( CreateAndInsert(_tagId, pStart, pEnd, &spElement) );
            IFR( spElement->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&spAnchor) );
            IFR( spAnchor->put_href(V_BSTR(pvarargIn)) );
            break;

        default:
            AssertSz(0, "Unsupported anchor command");
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::IsCmdInFormatCache
//
//----------------------------------------------------------------------------
BOOL
CAnchorCommand::IsCmdInFormatCache(IMarkupPointer  *pMarkupPointer,
                                 VARIANT         *pvarargIn)
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::InsertNamedAnchor
//
//----------------------------------------------------------------------------
HRESULT
CAnchorCommand::InsertNamedAnchor(BSTR bstrName, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    CStr                strAttr;
    SP_IHTMLElement     spElement;

    //
    // HACKHACK: If we try to set the name of an element in browse mode, the OM ignores the
    // request and sets the submit name instead.  So, we need to create another element with
    // the name set.
    //

    // Build the attribute string
    if (bstrName)
    {
        IFR( strAttr.Set(_T("name=\"")) );
        IFR( strAttr.Append(bstrName) );
        IFR( strAttr.Append(_T("\"")) );
    }

    // Insert the new anchor
    IFR( GetMarkupServices()->CreateElement(TAGID_A, strAttr, &spElement) );
    IFR( GetMarkupServices()->InsertElement(spElement, pStart, pEnd) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::UpdateContainedAnchors
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::UpdateContainedAnchors(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT                 hr;
    CEditPointer            epTest(GetEditor());
    DWORD                   dwSearch = BREAK_CONDITION_EnterAnchor;
    DWORD                   dwFound;
    BOOL                    fFoundAnchor = FALSE;
    MARKUP_CONTEXT_TYPE     context;
    SP_IHTMLElement         spElement;

    //
    // Scan for anchors and change attributes to pvarargIn
    //

    IFR( epTest->MoveToPointer(pStart) );
    IFR( epTest.SetBoundary(NULL, pEnd) );

    for (;;)
    {
        IFR( epTest.Scan(RIGHT, dwSearch, &dwFound) );
        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            break;

        Assert(epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor));

        //
        // Update anchor
        //
        IFR( epTest->Left(FALSE, &context, &spElement, NULL, NULL) );

        Assert(context == CONTEXT_TYPE_ExitScope);
        if (context == CONTEXT_TYPE_ExitScope && spElement != NULL)
        {
            IFR( UpdateAnchor(spElement, pvarargIn) );            
            fFoundAnchor = TRUE;
        }
            
    }

    return (fFoundAnchor ? S_OK : S_FALSE);
}

HRESULT
CAnchorCommand::UpdateAnchor(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT                 hr;
    SP_IHTMLAnchorElement   spAnchor;

    if (!pvarargIn || V_VT(pvarargIn) != VT_BSTR)
        return E_INVALIDARG;
    
    switch (_cmdId)
    {
        case IDM_BOOKMARK:
            {
                SP_IMarkupPointer spLeft, spRight;

                IFR( GetMarkupServices()->CreateMarkupPointer(&spLeft) )
                IFR( spLeft->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
                
                IFR( GetMarkupServices()->CreateMarkupPointer(&spRight) );
                IFR( spRight->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );
                
                IFR( InsertNamedAnchor(V_BSTR(pvarargIn), spLeft, spRight) );

                IFR( GetMarkupServices()->RemoveElement(pElement) );
            }
            break;

        case IDM_HYPERLINK:
            IFR( pElement->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&spAnchor) );
            IFR( spAnchor->put_href(V_BSTR(pvarargIn)) );
            break;

        default:
            AssertSz(0, "Unsupported anchor command");
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::PrivateExec
//
//----------------------------------------------------------------------------
HRESULT
CAnchorCommand::PrivateExec( 
        DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_IHTMLElement         spElement;
    CTagBitField            tagBitField;
    SP_ISegmentList         spSegmentList;
    BOOL                    fEqual;
    INT                     iSegmentCount;
    SELECTION_TYPE          eSelectionType;
    CUndoUnit               undoUnit(GetEditor());
        
    IFR( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
                            
    IFC( GetSegmentList( &spSegmentList ));
    IFC( GetSegmentPointers(spSegmentList, 0, &spStart, &spEnd) );

    if (pvarargIn == NULL || V_VT(pvarargIn) != VT_BSTR)
        return E_INVALIDARG;

    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );
        
    tagBitField.Set(TAGID_A);
    if (IsCmdAbove(GetMarkupServices(), spStart, spEnd, &spElement, NULL, &tagBitField))
    {
        IFC( UpdateAnchor(spElement, pvarargIn) );

        if (eSelectionType != SELECTION_TYPE_Caret)
        {
            IGNORE_HR( UpdateContainedAnchors(spStart, spEnd, pvarargIn) );
        }
    }
    else
    {
        if (eSelectionType == SELECTION_TYPE_Caret)
            return E_FAIL;

        // First try to update any anchors contained in the selection.  If there are no anchors,
        // we create one below
        IFC( UpdateContainedAnchors(spStart, spEnd, pvarargIn) );
        if (hr != S_FALSE)
            goto Cleanup;

        // Create the anchor
        IFC( spStart->IsEqualTo(spEnd, &fEqual) );        

        if (fEqual)
        {
            CSpringLoader * psl = GetSpringLoader();

            IGNORE_HR(psl->SpringLoadComposeSettings(spStart));
            IGNORE_HR(psl->Fire(spStart, spEnd));
        }

        IFC( PrivateApply(spStart, spEnd, pvarargIn, fEqual) );
    }
        
Cleanup:
   RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::PrivateQueryStatus
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::PrivateQueryStatus( 
    OLECMD *pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    SP_IMarkupPointer   spStart, spEnd;
    CTagBitField        tagBitField;
    INT                 iSegmentCount;
    SELECTION_TYPE      eSelectionType;
    ELEMENT_TAG_ID      tagId;
    CSelectionManager   *pSelMan;

    IFR( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        RRETURN(hr);

    // Make sure the edit context isn't a button
    if (GetEditor())
    {
        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            IFR( GetMarkupServices()->GetElementTagId(pSelMan->GetEditableElement(), &tagId) );
            if (tagId == TAGID_INPUT || tagId == TAGID_BUTTON)
            {
                pCmd->cmdf = MSOCMDSTATE_DISABLED;                
                return S_OK;
            }
        }
    }

    // Check if we are under an anchor
    pCmd->cmdf = MSOCMDSTATE_UP; // up by default

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount( &iSegmentCount, &eSelectionType) );
    if (eSelectionType == SELECTION_TYPE_Caret)
    {
        IFC( GetSegmentList( &spSegmentList ));
        IFC( GetSegmentPointers(spSegmentList, 0, &spStart, &spEnd) );
            
        tagBitField.Set(TAGID_A);
        if (!IsCmdAbove(GetMarkupServices(), spStart, spEnd, NULL, NULL, &tagBitField))
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;    
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:       CRemoveFormatBaseCommand::CRemoveFormatBaseCommand
//
//----------------------------------------------------------------------------
CRemoveFormatBaseCommand::CRemoveFormatBaseCommand(DWORD cmdId, CHTMLEditor *
ped)
: CCommand(cmdId, ped)
{
}

//+---------------------------------------------------------------------------
//
//  Method:       CRemoveFormatBaseCommand::Exec
//
//----------------------------------------------------------------------------
HRESULT 
CRemoveFormatBaseCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT             hr = S_OK;
    IMarkupPointer      *pStart = NULL;
    IMarkupPointer      *pEnd = NULL;
    SP_ISegmentList     spSegmentList;
    CSegmentListIter    iter;
    CSpringLoader       *psl = GetSpringLoader();
    int                 iSegmentCount;
    CUndoUnit           undoUnit(GetEditor());

    IFC( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
    
    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, NULL ) );
    IFC( iter.Init(GetMarkupServices(), GetViewServices(), spSegmentList) );

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );

        if (hr == S_FALSE)
        {
            hr = S_OK; // proper termination
            break;
        }

        if (iSegmentCount == 1 && _cmdId == IDM_REMOVEFORMAT)
            IGNORE_HR(EdUtil::ExpandToWord(GetMarkupServices(), GetViewServices(), pStart, pEnd));

        IFC( Apply(pStart, pEnd) );

        // Apply compose font.
        if (psl && _cmdId == IDM_REMOVEFORMAT)
        {
            IFC( psl->SpringLoadComposeSettings(NULL, TRUE) );
            IFC( psl->Fire(pStart, pEnd) );
        }
    }

Cleanup:

    RRETURN(hr);
}


HRESULT
CRemoveFormatBaseCommand::Apply(
    IMarkupPointer  *pStart,
    IMarkupPointer  *pEnd,
    BOOL            fQueryMode)
{
    HRESULT             hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    INT                 iPosition;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spNewElement;
    SP_IMarkupPointer   spCurrent;
    ELEMENT_TAG_ID      tagId;
            
    //
    // Walk pStart/pEnd out so we can avoid overlapping tags
    //

    // TODO: can make this faster but be careful about pointer placement [ashrafm]
    for(;;)
    {
        IFR( GetViewServices()->LeftOrSlave(pStart, FALSE, &context, NULL, NULL, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            break;
        IFR( GetViewServices()->LeftOrSlave(pStart, TRUE, NULL, NULL, NULL, NULL) );
    }

    for(;;)
    {
        IFR( GetViewServices()->RightOrSlave(pEnd, FALSE, &context, NULL, NULL, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            break;
        IFR( GetViewServices()->RightOrSlave(pEnd, TRUE, NULL, NULL, NULL, NULL) );
    }

    //
    // Set gravity
    //
    IFR( pStart->SetGravity(POINTER_GRAVITY_Right) );
    IFR( pEnd->SetGravity(POINTER_GRAVITY_Left) );

    //
    // Walk from left to right removing any tags we see
    //

    IFR( GetMarkupServices()->CreateMarkupPointer(&spCurrent) );
    IFR( spCurrent->MoveToPointer(pStart) );
    for (;;)
    {
        // Move right
        IFR( GetViewServices()->RightOrSlave(spCurrent, TRUE, &context, &spElement, NULL, NULL) );        
        IFR( OldCompare( spCurrent, pEnd, &iPosition) );
        if (iPosition != RIGHT)
            break;

        // Check tagId
        if (context == CONTEXT_TYPE_EnterScope)
        {
            IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
            if (_tagsRemove.Test(tagId))
            {
                if (fQueryMode)
                    return S_OK; // found tag
                    
                IFR( RemoveElement(spElement, pStart, pEnd) );
            }
        }        
    }

    //
    // Walk up remove tags we see
    //

    IFR( GetViewServices()->CurrentScopeOrSlave(pStart, &spElement) );
    for (;;)
    {
        IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
        if (_tagsRemove.Test(tagId))
        {
            if (fQueryMode)
                return S_OK; // found tag

            IFR( RemoveElement(spElement, pStart, pEnd) );
        }

        // Move up the tree
        IFR( spElement->get_parentElement(&spNewElement) );
        if (!spNewElement)
            break;            
        spElement = spNewElement;
    }

    if (fQueryMode)
        hr = S_FALSE; // not found
        
    RRETURN1(hr, S_FALSE);
}
    

//+---------------------------------------------------------------------------
//
//  CRemoveFormatCommand Class
//
//----------------------------------------------------------------------------

CRemoveFormatCommand::CRemoveFormatCommand(DWORD cmdId, CHTMLEditor *ped)
: CRemoveFormatBaseCommand(cmdId, ped)
{
    _tagsRemove.Set(TAGID_FONT);
    _tagsRemove.Set(TAGID_B);
    _tagsRemove.Set(TAGID_U);
    _tagsRemove.Set(TAGID_I);
    _tagsRemove.Set(TAGID_STRONG);
    _tagsRemove.Set(TAGID_EM);
    _tagsRemove.Set(TAGID_SUB);
    _tagsRemove.Set(TAGID_SUP);
    _tagsRemove.Set(TAGID_STRIKE);
}

HRESULT 
CRemoveFormatCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT         hr;

    IFR( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        RRETURN(hr);

    pCmd->cmdf = MSOCMDSTATE_UP;
    
    return S_OK;
}

HRESULT
CRemoveFormatCommand::RemoveElement(IHTMLElement *pElement, IMarkupPointer  *
pStart, IMarkupPointer  *pEnd)
{
    HRESULT         hr;
    elemInfluence   theInfluence;
   
    // Check influence and split        
    theInfluence = GetElementInfluenceOverPointers(GetMarkupServices(), pStart
, pEnd, pElement);
    hr = THR( SplitInfluenceElement(GetMarkupServices(), pStart, pEnd, 
pElement, theInfluence, NULL) ); 

    RRETURN(hr);
}   

//+---------------------------------------------------------------------------
//
//  CUnlinkCommand Class
//
//----------------------------------------------------------------------------

CUnlinkCommand::CUnlinkCommand(DWORD cmdId, CHTMLEditor *ped)
: CRemoveFormatBaseCommand(cmdId, ped)
{
    _tagsRemove.Set(TAGID_A);
}

HRESULT
CUnlinkCommand::RemoveElement(IHTMLElement *pElement, IMarkupPointer  *pStart
, IMarkupPointer  *pEnd)
{
    RRETURN( GetMarkupServices()->RemoveElement(pElement) );
}

HRESULT 
CUnlinkCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT                 hr;
    SP_ISegmentList         spSegmentList;
    CSegmentListIter        iter;
    IMarkupPointer          *pStart;
    IMarkupPointer          *pEnd;

    IFR( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        RRETURN(hr);
             
    IFC( GetSegmentList(&spSegmentList) );

    IFC( iter.Init(GetMarkupServices(), GetViewServices(), spSegmentList) );
    pCmd->cmdf = MSOCMDSTATE_DISABLED;
    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );
        if (hr ==  S_FALSE)
            break; // end of list
   
        IFC(Apply(pStart, pEnd, TRUE)); // query mode
        if (hr == S_OK)
        {
            pCmd->cmdf = MSOCMDSTATE_UP; // found link
            goto Cleanup;
        }
    }
    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

BOOL 
CUnlinkCommand::IsValidOnControl()
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

BOOL
CAnchorCommand::IsCmdAbove(   IMarkupServices *pMarkupServices ,
                                IMarkupPointer* pStart,
                                IMarkupPointer* pEnd,
                                IHTMLElement**  ppFirstMatchElement,
                                elemInfluence * pInfluence ,
                                CTagBitField *  inSynonyms )
{
    BOOL match = FALSE;
    IHTMLElement *pCurrentElement = NULL ;
    IHTMLElement *pNextElement = NULL ;
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID currentTag = TAGID_NULL ;

    Assert ( pStart && pEnd );

    //
    // First look to the left of the Start pointer - to see if that "leads up to" the tag
    //
    hr = GetViewServices()->CurrentScopeOrSlave(pStart, & pCurrentElement );
    if (  hr ) goto CleanUp;
    Assert( pCurrentElement );
     
    while ( ! match && pCurrentElement )
    {
        hr = pMarkupServices->GetElementTagId( pCurrentElement, &currentTag);
        if ( hr ) goto CleanUp;

        match = inSynonyms->Test( (USHORT) currentTag );
        if ( match ) break;

        pCurrentElement->get_parentElement( & pNextElement );
        ReplaceInterface( &pCurrentElement, pNextElement );
        ClearInterface( & pNextElement );
    }

    if (match )
    {
        if( ppFirstMatchElement )
            *ppFirstMatchElement = pCurrentElement;
        if ( pInfluence )
            *pInfluence = GetElementInfluenceOverPointers( pMarkupServices, pStart, pEnd, pCurrentElement );
    }

CleanUp:

    if ( ( ! ppFirstMatchElement )  || ( ! match ) )
        ReleaseInterface( pCurrentElement );

    return match;
}


BOOL 
CAnchorCommand::IsValidOnControl()
{
    HRESULT         hr;
    BOOL            bResult = FALSE;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;

    IFC( GetSegmentList(&spSegmentList) );
#if DBG==1
    SELECTION_TYPE  eSelectionType;
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );
    Assert(eSelectionType == SELECTION_TYPE_Control);
#else
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, NULL) );    
#endif

    bResult = (iSegmentCount == 1);

    if (bResult && _cmdId == IDM_HYPERLINK)
    {
        SP_IHTMLElement spElement;
        ELEMENT_TAG_ID  tagId;
        
        bResult = FALSE; // just in case we fail below
        
        // Check that the element in the control range is an image
        IFC( GetSegmentElement(spSegmentList, 0, &spElement) );
        IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

        bResult = (tagId == TAGID_IMG);
    }

Cleanup:
    return bResult;
}

