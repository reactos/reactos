#include "headers.hxx"

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_SELMAN_HXX_
#define _X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

using namespace EdUtil;

MtDefine(EditCommand, Edit, "Commands")
MtDefine(CCommand, EditCommand, "CCommand")
MtDefine(CCommandTable, EditCommand, "CCommandTable")

//
// CCommandTable
//

CCommandTable::CCommandTable(unsigned short iInitSize)
{
    _rootNode = NULL;
}

CCommandTable::~CCommandTable()
{
    // TODO - Make this non-recursive
    // We want to delete all the pointers in the command table
    _rootNode->Passivate();

}


void CCommand::Passivate()
{
    if( _leftNode )
        _leftNode->Passivate();

    if( _rightNode )
        _rightNode->Passivate();

    delete this;
}    


CCommand::CCommand( 
    DWORD                     cmdId, 
    CHTMLEditor *             pEd )
{
    _pEd = pEd;
    _cmdId = cmdId;
    _leftNode = NULL;
    _rightNode = NULL;

}


CHTMLEditor * 
CCommand::GetEditor()
{ 
    return _pEd;
}

IHTMLDocument2* 
CCommand::GetDoc() 
{ 
    return _pEd->GetDoc(); 
}

IMarkupServices* 
CCommand::GetMarkupServices() 
{ 
    return _pEd->GetMarkupServices();
}

IHTMLViewServices* 
CCommand::GetViewServices() 
{ 
    return _pEd->GetViewServices(); 
}

//=========================================================================
// CCommand: GetViewServices
//
// Synopsis: Gets an IHTMLViewServices ptr from an IMarkupServices ptr
//-------------------------------------------------------------------------
HRESULT
CCommand::GetViewServices(IMarkupServices *    pMarkupServices,
                          IHTMLViewServices ** ppViewServices)
{
    HRESULT         hr;
    IHTMLDocument * pDoc;

    // TODO: do I need to go through IHTMLDocument2? [ashrafm]
    hr = THR(pMarkupServices->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc));
    if (FAILED(hr)) {
        *ppViewServices = NULL;
        return hr;
    }

    hr = THR(pDoc->QueryInterface(IID_IHTMLViewServices, (LPVOID *)ppViewServices));

    pDoc->Release();

    RRETURN(hr);
}

BOOL
CCommand::IsSelectionActive()
{
    HRESULT           hr;
    SP_ISegmentList   spSegmentList;
    INT               iSegmentCount;
    SELECTION_TYPE    eSelectionType;
    
    // If the selection is still active, do nothing for the command
    //

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );

    if (eSelectionType == SELECTION_TYPE_Selection && iSegmentCount > 0)
    {
        CSelectionManager *pSelMan = GetEditor()->GetSelectionManager();        
        if (pSelMan->GetActiveTracker() && pSelMan->GetSelectionType() == SELECTION_TYPE_Selection)
        {
            CSelectTracker *pSelectTracker = DYNCAST(CSelectTracker, pSelMan->GetActiveTracker());

            if (!pSelectTracker->IsPassive())
                return TRUE; // done
        }
    }

Cleanup:
    return FALSE;
}    


HRESULT 
CCommand::Exec( 
    DWORD                    nCmdexecopt,
    VARIANTARG *             pvarargIn,
    VARIANTARG *             pvarargOut,
    CMshtmlEd *              pTarget  )
{
    HRESULT           hr;
    
    
    _pcmdtgt = pTarget;
    hr = THR( PrivateExec( nCmdexecopt, pvarargIn, pvarargOut ));
    _pcmdtgt = NULL;
    
    RRETURN( hr );
}    

HRESULT 
CCommand::QueryStatus( 
    OLECMD                     rgCmds[],
    OLECMDTEXT *             pcmdtext,
    CMshtmlEd *              pTarget  )
{
    HRESULT hr;
    _pcmdtgt = pTarget;
    hr = THR( PrivateQueryStatus( rgCmds, pcmdtext ));
    _pcmdtgt = NULL;
    RRETURN( hr );
}    
 

HRESULT
CCommand::GetSegmentList( ISegmentList ** ppSegmentList ) 
{ 
    HRESULT hr = E_UNEXPECTED;
    AssertSz( _pcmdtgt != NULL , "Attempt to get the segment list without a valid command target." );
    if( _pcmdtgt == NULL )
        goto Cleanup;
        
    hr = THR( _pcmdtgt->GetSegmentList( ppSegmentList ));

Cleanup:
    RRETURN( hr );
}

CSpringLoader*
CCommand::GetSpringLoader() 
{ 
    AssertSz( _pcmdtgt != NULL , "Attempt to get the spring loader without a valid command target." );
    if( _pcmdtgt == NULL )
        return NULL;

    return _pcmdtgt->GetSpringLoader();
}


// TODO: move this to EdUtil
HRESULT CCommand::FindCommonElement( 
    IMarkupServices *        pMarkupServices,
    IHTMLViewServices *      pViewServices,
    IMarkupPointer  *        pStart, 
    IMarkupPointer  *        pEnd,
    IHTMLElement    **       ppElement,
    BOOL                     fIgnorePhrase /* = FALSE */)
{
    HRESULT         hr;
    IMarkupPointer  *pLeft;
    IMarkupPointer  *pRight;
    INT             iPosition;
    IMarkupPointer  *pCurrent = NULL;
    IHTMLElement    *pOldElement = NULL;

    // init for error case
    *ppElement = NULL;
    
    //
    // Find right/left pointers
    //
    
    hr = THR(OldCompare( pStart, pEnd, &iPosition));
    if (FAILED(hr))
        goto Cleanup;

    if (iPosition == SAME)
    {   
        hr = THR(pViewServices->CurrentScopeOrSlave(pStart, ppElement));
        goto Cleanup;
    }
    else if (iPosition == RIGHT)
    {
        pLeft = pStart;     // weak ref
        pRight = pEnd;      // weak ref
    }
    else
    {
        pLeft = pEnd;       // weak ref
        pRight = pStart;    // weak ref
    }

    //
    // Walk the left pointer up until the right end of the element
    // is to the right of pRight
    //

    hr = THR(pViewServices->CurrentScopeOrSlave(pLeft, ppElement));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pMarkupServices->CreateMarkupPointer(&pCurrent));
    if (FAILED(hr))
        goto Cleanup;
    
    while (*ppElement)
    {
        hr = THR(pCurrent->MoveAdjacentToElement(*ppElement, ELEM_ADJ_AfterEnd));
        if (FAILED(hr))
            goto Cleanup;
        
        if (fIgnorePhrase)
        {
            if (!GetEditor()->IsPhraseElement(*ppElement))
            {
                IFC( OldCompare( pCurrent, pRight, &iPosition) );

                if (iPosition != RIGHT)
                    break; // found common element
            }
        }
        else
        {
            IFC( OldCompare( pCurrent, pRight, &iPosition) );

            if (iPosition != RIGHT)
                break; // found common element
        }

        pOldElement = *ppElement;                
        hr = THR(pOldElement->get_parentElement(ppElement));
        pOldElement->Release();
        if (FAILED(hr))
            goto Cleanup;        
        Assert(*ppElement); // we should never walk up past the root of the tree
    }
    
Cleanup:
    ReleaseInterface(pCurrent);
    RRETURN(hr);    
}

// TODO: move this to EdUtil
HRESULT CCommand::FindBlockElement( 
    IMarkupServices *        pMarkupServices,
    IHTMLElement     *        pElement, 
    IHTMLElement     **        ppBlockElement )
{
    HRESULT             hr = S_OK;
    IHTMLElement        *pOldElement = NULL;
    IHTMLViewServices   *pViewServices = NULL;    
    BOOL                bBlockElement;

    *ppBlockElement = pElement;
    pElement->AddRef();

    hr = THR(pMarkupServices->QueryInterface(IID_IHTMLViewServices, (LPVOID *)&pViewServices));
    if (FAILED(hr))
        goto Cleanup;

    do
    {
        hr = pViewServices->IsBlockElement(*ppBlockElement, &bBlockElement); 
        if (FAILED(hr) || bBlockElement)
            goto Cleanup;
            
        pOldElement = *ppBlockElement;                
        hr = THR(pOldElement->get_parentElement(ppBlockElement));
        pOldElement->Release();
        if (FAILED(hr))
            goto Cleanup;        
    }
    while (*ppBlockElement);

Cleanup:    
    ReleaseInterface(pViewServices);
    RRETURN(hr);        
}

BOOL
CCommand::CanSplitBlock( IMarkupServices *pMarkupServices , IHTMLElement* pElement )
{
    ELEMENT_TAG_ID curTag = TAGID_NULL;

    THR( pMarkupServices->GetElementTagId( pElement, &curTag  ));

    //
    // TODO: make sure this is a complete list [ashrafm]
    //
    
    switch( curTag )
    {
        case TAGID_P:
        case TAGID_DIV:
        case TAGID_LI:
        case TAGID_BLOCKQUOTE:
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
        case TAGID_HR:
        case TAGID_CENTER:
        case TAGID_PRE:
        case TAGID_ADDRESS:
            return true;

        default:
            return false;
    }
}

BOOL 
CCommand::IsValidEditContext(IHTMLElement *pElement)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;

    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );

    return (tagId != TAGID_BUTTON && tagId != TAGID_INPUT);

Cleanup:
    return FALSE;
}

CCommand::~CCommand()
{
}


//+==========================================================================
//  CCommandTable::Add
//
//  Add an entry to the command table.
//
//---------------------------------------------------------------------------

VOID
CCommandTable::Add( CCommand* pCommandEntry )
{
    CCommand* pInsertNode = NULL;

    if ( _rootNode == NULL )
        _rootNode = pCommandEntry;
    else
    {
        Verify(!FindEntry( pCommandEntry->GetCommandId() , &pInsertNode));
        Assert( pInsertNode );

        if ( pInsertNode->GetCommandId() > pCommandEntry->GetCommandId() )
            pInsertNode->SetLeft( pCommandEntry );
        else
            pInsertNode->SetRight( pCommandEntry );
    }

}

//+==========================================================================
//  CCommandTable::Get
//
//  Get the Contents of a Node with the given key entry - or null if none exists
//
//---------------------------------------------------------------------------

CCommand*
CCommandTable::Get(DWORD entryKey )
{
    CCommand* pFoundNode = NULL;

    if (  FindEntry( entryKey, &pFoundNode ) )
    {
        Assert( pFoundNode );
        return pFoundNode ;
    }
    else
    {
        return NULL;
    }
}

//+==========================================================================
//  CCommandTable::FindEntry
//
//  Find a given key entry.
//
//  RESULT:
//      1  - we found an entry with the given key. pFoundNode points to it
//      0  - didn't find an entry. pFoundNode points to the last node in the tree
//           where we were. You can test pFoundNode for where to insert the next node.
//
//---------------------------------------------------------------------------

short
CCommandTable::FindEntry(DWORD entryKey, CCommand** ppFoundNode )
{
    CCommand *pCommandEntry = _rootNode;
    short result = 0;

    while ( pCommandEntry != NULL)
    {
        if ( pCommandEntry->GetCommandId() == entryKey)
        {
            result = 1;
            *ppFoundNode = pCommandEntry;
            break;
        }
        else
        {
            *ppFoundNode = pCommandEntry;

            if ( pCommandEntry->GetCommandId() > entryKey )
            {
                pCommandEntry = pCommandEntry->GetLeft();
            }
            else
                pCommandEntry = pCommandEntry->GetRight();
        }
    }

    return result;
}

#if DBG == 1
VOID
CCommand::DumpTree( IUnknown* pUnknown)
{
    IOleCommandTarget  *  pHost = NULL;
    Assert( pUnknown );
    GUID theGUID = CGID_MSHTML;
    IGNORE_HR( pUnknown->QueryInterface( IID_IOleCommandTarget,  (void**)& pHost ) ) ;
    IGNORE_HR( pHost->Exec( &theGUID , IDM_DEBUG_DUMPTREE, 0, NULL, NULL  ));

    ReleaseInterface( pHost );

}

#endif

//+---------------------------------------------------------------------------
//
//  CCommand::GetSegmentElement
//
//----------------------------------------------------------------------------
HRESULT CCommand::GetSegmentElement(IMarkupServices *pMarkupServices, 
                                    IHTMLViewServices *pViewServices,
                                    IMarkupPointer  *pStart, 
                                    IMarkupPointer  *pEnd, 
                                    IHTMLElement    **ppElement,
                                    BOOL            fOuter)
{
    HRESULT             hr = E_FAIL;
    MARKUP_CONTEXT_TYPE context, contextGoal;
    IHTMLElement        *pElementRHS = NULL;
    IObjectIdentity     *pObjectIdent = NULL;    

    *ppElement = NULL;
    
    //
    // Is there an element right at this position?  If so,
    // return it.  Otherwise, fail.
    //
    
    //
    // Find the left side element
    //

    if (fOuter)
    {
        if (FAILED(THR(pViewServices->LeftOrSlave(pStart, FALSE, &context, ppElement, NULL, NULL))))
            goto Cleanup;
            
        contextGoal = CONTEXT_TYPE_ExitScope;
    }
    else
    {
        if (FAILED(THR(pViewServices->RightOrSlave(pStart, FALSE, &context, ppElement, NULL, NULL))))
            goto Cleanup;
            
        contextGoal = CONTEXT_TYPE_EnterScope;
    }
        
    if (context != contextGoal && (context != CONTEXT_TYPE_NoScope || !(*ppElement)))
        goto Cleanup; // fail

    //
    // Check to see if the right side is a div element
    //

    if (fOuter)
    {
        if (FAILED(THR(pViewServices->RightOrSlave(pEnd, FALSE, &context, &pElementRHS, NULL, NULL))))
            goto Cleanup;
    }
    else
    {
        if (FAILED(THR(pViewServices->LeftOrSlave(pEnd, FALSE, &context, &pElementRHS, NULL, NULL))))
            goto Cleanup;
    }

    if (context != contextGoal && (context != CONTEXT_TYPE_NoScope || !(*ppElement)))
        goto Cleanup; // fail

    //
    // Check if the elements are the same
    //

    if (FAILED(THR(pElementRHS->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pObjectIdent))))
        goto Cleanup; // fail

    hr = THR(pObjectIdent->IsEqualObject(*ppElement));

Cleanup:
    if (FAILED(hr))
        ClearInterface(ppElement);

    ReleaseInterface(pElementRHS);
    ReleaseInterface(pObjectIdent);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:       SplitInfluenceElement
//
//  Synopsis:     Given an IHTMLElement* that influences a pair of pointers - adjust the element's
//                influence - so it no longer influences the range
//
//                Various Remove and Insert Operations will be involved here.
//
//  ppElementNew - if passed in a Pointer to a New Element, and a New Element is created
//                  this returns the new element that's been created.
//----------------------------------------------------------------------------
HRESULT
CCommand::SplitInfluenceElement(
                    IMarkupServices * pMarkupServices,
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd,
                    IHTMLElement* pElement,
                    elemInfluence inElemInfluence,
                    IHTMLElement** ppElementNew )
{
    IMarkupPointer *pStartPointer = NULL ;
    IMarkupPointer *pEndPointer = NULL ;
    IHTMLElement *pNewElement = NULL;
    HRESULT hr = S_OK;
    BOOL    bEqual = FALSE;

    switch ( inElemInfluence )
    {
        case elemInfluenceWithin:
        {
            hr = pMarkupServices->RemoveElement( pElement );
        }
        break;

        case elemInfluenceCompleteContain:
        {
            hr = pMarkupServices->CreateMarkupPointer( & pStartPointer   );
            if (!hr) hr = pStartPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeBegin );
            if (!hr) hr = pStartPointer->SetGravity(POINTER_GRAVITY_Right);
            if (!hr) hr = pMarkupServices->CreateMarkupPointer( & pEndPointer );
            if (!hr) hr = pEndPointer->SetGravity(POINTER_GRAVITY_Left);
            if (!hr) hr = pEndPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd );
            if (hr) goto Cleanup;

            hr = pMarkupServices->RemoveElement( pElement );
            if (!hr) hr = THR(pStartPointer->IsEqualTo(pStart, &bEqual));
            if (hr) goto Cleanup;
            
            if (!bEqual)
            {
                hr = InsertElement(pMarkupServices, pElement, pStartPointer, pStart );
                if (hr) goto Cleanup;
            }

            hr = THR(pEndPointer->IsEqualTo(pEnd, &bEqual));
            if (hr) goto Cleanup;
            
            if (!bEqual)
            {
                hr = pMarkupServices->CloneElement( pElement, &pNewElement );
                if (!hr) hr = InsertElement(pMarkupServices, pNewElement, pEnd , pEndPointer );
            }

            if ( ppElementNew )
            {
                *ppElementNew = pNewElement;
            }
        }
        break;

        case elemInfluenceOverlapWithin:
        {
            hr = pMarkupServices->CreateMarkupPointer( & pEndPointer   );
            if (!hr) hr = pEndPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd );
            if (hr) goto Cleanup;

            if (!hr) hr = pMarkupServices->RemoveElement( pElement );
            if (!hr) hr = THR(pEndPointer->IsEqualTo(pEnd, &bEqual));
            if (hr) goto Cleanup;

            if (!bEqual)
            {
                hr = InsertElement(pMarkupServices, pElement, pEnd, pEndPointer );
            }

        }
        break;

        case elemInfluenceOverlapOutside:
        {
            hr = pMarkupServices->CreateMarkupPointer( & pStartPointer   );
            if (!hr) hr = pStartPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeBegin );
            if (hr) goto Cleanup;

            hr = pMarkupServices->RemoveElement( pElement );
            if (!hr) hr = THR(pStart->IsEqualTo(pStartPointer, &bEqual));
            if (hr) goto Cleanup;

            if (!bEqual)
            {
                if ( ! hr ) hr = InsertElement(pMarkupServices, pElement, pStartPointer, pStart );
                if (hr) goto Cleanup;
            }
        }
        break;
    }

Cleanup:
    ReleaseInterface( pStartPointer );
    ReleaseInterface( pEndPointer );
    if ( ! ppElementNew ) ReleaseInterface( pNewElement );

    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Method:       GetElementInfluenceOverPointers
//
//  Synopsis:     Given an IHTMLElement* determine how that tag Influences
//                the pair of tree pointers.
//
//                See header file EdTree.hxx for description of different
//                values of tagInfluence
//
//----------------------------------------------------------------------------

// TODO: be careful about contextually equal pointers here [ashrafm]

elemInfluence
CCommand::GetElementInfluenceOverPointers( IMarkupServices* pMarkupServices, IMarkupPointer* pStart, IMarkupPointer * pEnd, IHTMLElement* pInfluenceElement )
{
    elemInfluence theInfluence = elemInfluenceNone ;
    int iStartStart, iStartEnd, iEndStart, iEndEnd;
    iStartStart = iStartEnd = iEndStart = iEndEnd = 0;
    IMarkupPointer *pStartInfluence = NULL ;
    IMarkupPointer *pEndInfluence = NULL ;

    pMarkupServices->CreateMarkupPointer( & pStartInfluence );
    pMarkupServices->CreateMarkupPointer( & pEndInfluence );
    Assert( pStartInfluence && pEndInfluence );
    pStartInfluence->MoveAdjacentToElement( pInfluenceElement, ELEM_ADJ_BeforeBegin );
    pEndInfluence->MoveAdjacentToElement( pInfluenceElement, ELEM_ADJ_AfterEnd );

    OldCompare( pStart, pStartInfluence, &iStartStart);
    OldCompare( pEnd, pEndInfluence, & iEndEnd);

    if ( iStartStart == RIGHT ) // Start is to Right of Start of Range
    {
        if ( iEndEnd == LEFT ) // End is to Left of End of Range
            theInfluence = elemInfluenceWithin;
        else
        {
            // End is Inside Range - where is the Start ?

            OldCompare( pEnd, pStartInfluence, & iStartEnd );
            if ( iStartEnd == LEFT ) // Start is Inside Range, End is Outside
            {
                theInfluence = elemInfluenceOverlapWithin;
            }
            else
                theInfluence = elemInfluenceNone; // completely outside range
        }
    }
    else // Start is to Left of Range.
    {
        if ( iEndEnd == RIGHT ) // End is Outside
            theInfluence = elemInfluenceCompleteContain;
        else
        {
            // Start is Outside Range - where does End Start
            OldCompare( pStart, pEndInfluence, &iEndStart );
            if ( iEndStart == RIGHT )
            {
                // End is to Right of Start
                theInfluence = elemInfluenceOverlapOutside;
            }
            else
                theInfluence = elemInfluenceNone;
        }
    }

    ReleaseInterface( pStartInfluence );
    ReleaseInterface( pEndInfluence );

    return theInfluence;
}


//=========================================================================
// CCommand: GetSegmentPointers
//
// Synopsis: Get start/end pointers for a specified segment
//-------------------------------------------------------------------------

HRESULT CCommand::GetSegmentPointers( ISegmentList    *pSegmentList,
                                      INT             iSegment,
                                      IMarkupPointer  **ppStart,
                                      IMarkupPointer  **ppEnd)
{
    HRESULT        hr;
    IMarkupPointer *pStart = NULL;
    IMarkupPointer *pEnd = NULL;

    // We need these guys so either use the users pointers or local ones
    if (ppStart == NULL)
        ppStart = &pStart;

    if (ppEnd == NULL)
        ppEnd = &pEnd;

    // Move pointers to segment
    IFC(GetMarkupServices()->CreateMarkupPointer(ppStart));
    IFC(GetMarkupServices()->CreateMarkupPointer(ppEnd));

    IFC(MovePointersToSegmentHelper(GetViewServices(), pSegmentList, iSegment, ppStart, ppEnd));

Cleanup:
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    RRETURN(hr);
}

//=========================================================================
//
// CCommand: ClingToText
//
//-------------------------------------------------------------------------
HRESULT
CCommand::ClingToText(IMarkupPointer *pMarkupPointer, Direction direction, IMarkupPointer *pLimit, 
                      BOOL fSkipExitScopes /* = FALSE */, BOOL fIgnoreWhiteSpace /* = FALSE */)
{
    HRESULT             hr;
    CEditPointer        epPosition(GetEditor(), pMarkupPointer);
    DWORD               dwSearch, dwFound;
    DWORD               dwSearchReverse;
    DWORD               dwScanOptions = SCAN_OPTION_SkipControls;

    if (fIgnoreWhiteSpace)
        dwScanOptions |= SCAN_OPTION_SkipWhitespace;

    Assert(direction == LEFT || direction == RIGHT);

    // Set boundary on the edit pointer
    if (pLimit)
    {
        if (direction == LEFT)
            IFR( epPosition.SetBoundary(pLimit, NULL) )        
        else
            IFR( epPosition.SetBoundary(NULL, pLimit) );            
    }

    // Do cling to text
    dwSearch = BREAK_CONDITION_Text           |
               BREAK_CONDITION_NoScopeSite    |
               BREAK_CONDITION_NoScopeBlock   |
               BREAK_CONDITION_ExitSite       | 
               (fSkipExitScopes ? 0 : BREAK_CONDITION_ExitBlock)     |
               BREAK_CONDITION_Control;

    IFR( epPosition.Scan(direction, dwSearch, &dwFound, NULL, NULL, NULL, dwScanOptions) );
    
    if (!epPosition.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
    {
        dwSearchReverse = BREAK_CONDITION_Text           |
                          BREAK_CONDITION_NoScopeSite    |
                          BREAK_CONDITION_NoScopeBlock   |
                          BREAK_CONDITION_EnterSite      | 
                          (fSkipExitScopes ? 0: BREAK_CONDITION_EnterBlock)     |
                          BREAK_CONDITION_Control;

        // move back before break condition
        IFR( epPosition.Scan(Reverse(direction), dwSearchReverse, &dwFound, NULL, NULL, NULL, dwScanOptions) );     }

    // Return pMarkupPointer
    IFR( pMarkupPointer->MoveToPointer(epPosition) );

    return S_OK;
}

//=========================================================================
//
// CCommand: Move
//
//-------------------------------------------------------------------------

HRESULT 
CCommand::Move(
    IMarkupPointer          *pMarkupPointer, 
    Direction               direction, 
    BOOL                    fMove,
    MARKUP_CONTEXT_TYPE *   pContext,
    IHTMLElement * *        ppElement)
{
    HRESULT                 hr;
    MARKUP_CONTEXT_TYPE     context;
    SP_IHTMLElement         spElement;
    SP_IMarkupPointer       spPointer;
    ELEMENT_TAG_ID          tagId;
    BOOL                    fSite;

    Assert(direction == LEFT || direction == RIGHT);

    if (!fMove)
    {
        IFR( GetMarkupServices()->CreateMarkupPointer(&spPointer) );
        IFR( spPointer->MoveToPointer(pMarkupPointer) );
        
        pMarkupPointer = spPointer; // weak ref 
    }
    
    for (;;)
    {
        if (direction == LEFT)
            IFC( GetViewServices()->LeftOrSlave(pMarkupPointer, TRUE, &context, &spElement, NULL, NULL) )
        else
            IFC( GetViewServices()->RightOrSlave(pMarkupPointer, TRUE, &context, &spElement, NULL, NULL) );

        switch (context)
        {
            case CONTEXT_TYPE_EnterScope:
                IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

                if (IsIntrinsic(GetMarkupServices(), spElement))
                {
                    if (direction == LEFT)
                        IFC( pMarkupPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin ) )
                    else
                        IFC( pMarkupPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd ) ); 
                }
                // fall through
                           
            case CONTEXT_TYPE_ExitScope:
            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_None:
                goto Cleanup; // done;
                break;  

            case CONTEXT_TYPE_NoScope:                
                IFC( GetViewServices()->IsSite(spElement, &fSite, NULL, NULL, NULL) );
                if (!fSite)
                    continue;
                    
                goto Cleanup; // done;
                break;  
                            
            default:
                AssertSz(0, "CBaseCharCommand: Unsupported context");
                hr = E_FAIL; // CONTEXT_TYPE_None
                goto Cleanup;
        }
    }
    
Cleanup:
    if (ppElement)
    {
        if (SUCCEEDED(hr))
        {
            *ppElement = spElement;
            if (*ppElement)
                (*ppElement)->AddRef();
        }
        else
        {
            *ppElement = NULL;
        }
    }
        
    if (pContext)
    {
        *pContext = (SUCCEEDED(hr)) ? context : CONTEXT_TYPE_None;
    }    

    RRETURN(hr);
}

//=========================================================================
//
// CCommand: MoveBack
//
//-------------------------------------------------------------------------

HRESULT 
CCommand::MoveBack(
    IMarkupPointer          *pMarkupPointer, 
    Direction               direction, 
    BOOL                    fMove,
    MARKUP_CONTEXT_TYPE *   pContext,
    IHTMLElement * *        ppElement)
{
    if (direction == RIGHT)
    {
        RRETURN(Move(pMarkupPointer, LEFT, fMove, pContext, ppElement));
    }
    else
    {
        Assert(direction == LEFT);
        RRETURN(Move(pMarkupPointer, RIGHT, fMove, pContext, ppElement));
    }
}

//=========================================================================
// CCommand: GetActiveElemSegment
//
// Synopsis: Gets the segment for the active element
//-------------------------------------------------------------------------
HRESULT 
CCommand::GetActiveElemSegment( IMarkupServices *pMarkupServices,
                                IMarkupPointer  **ppStart,
                                IMarkupPointer  **ppEnd)
{
    HRESULT        hr;
    IHTMLElement   *pElement = NULL;
    IMarkupPointer *pStart = NULL;
    IMarkupPointer *pEnd = NULL;
    BOOL fNoScope = FALSE;    
    IHTMLDocument2* pDoc = GetDoc();
    IHTMLViewServices* pVS = GetViewServices();
    
#if 0
    hr = THR(pMarkupServices->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pMarkupServices->QueryInterface(IID_IHTMLViewServices, (LPVOID *)&pVS));
    if (FAILED(hr))
        goto Cleanup;    
#endif //0
    
    hr = THR( pDoc->get_activeElement(&pElement));
    if (FAILED(hr))
        goto Cleanup;

    //
    // If a No-Scope is Active - don't position pointers inside.
    //
    IFC( pVS->IsNoScopeElement( pElement, & fNoScope ));
    if ( fNoScope )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = pMarkupServices->CreateMarkupPointer(&pStart);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupServices->CreateMarkupPointer(&pEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    if (FAILED(hr))
        goto Cleanup;
        
    hr = THR(pEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeEnd ));

Cleanup:
    if (SUCCEEDED(hr))
    {
        if (ppStart)
        {
            *ppStart = pStart;
            pStart->AddRef();
        }
        if (ppEnd)
        {
            *ppEnd = pEnd;
            pEnd->AddRef();
        }
    }
#if 0
    ReleaseInterface(pDoc);
    ReleaseInterface(pVS);
#endif // 0    
    ReleaseInterface(pElement);
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    
    RRETURN(hr);

}

//=========================================================================
// CCommand: GetLeftAdjacentTagId
//
// Synopsis: Moves the markup pointer to an element tag with the specified
//           TAGID.  However, the pointer is not advanced past any text.
//
// Returns:  S_OK if found
//           S_FALSE if not found
//
//-------------------------------------------------------------------------
HRESULT
CCommand::GetLeftAdjacentTagId(  IMarkupServices *pMarkupServices,
                                 IHTMLViewServices *pViewServices,
                                 IMarkupPointer  *pMarkupPointer,
                                 ELEMENT_TAG_ID  tagIdTarget,
                                 IMarkupPointer  **ppLeft,
                                 IHTMLElement    **ppElement,
                                 MARKUP_CONTEXT_TYPE *pContext)
{
    HRESULT             hr;
    ELEMENT_TAG_ID      tagIdCurrent;
    IMarkupPointer      *pCurrent = NULL;
    IHTMLElement        *pElement = NULL;
    MARKUP_CONTEXT_TYPE context;
    //
    // Check to the left
    //

    hr = THR(CopyMarkupPointer(pMarkupServices, pMarkupPointer, &pCurrent));
    if (FAILED(hr))
        goto Cleanup;

    for (;;) {
        hr = THR(pViewServices->LeftOrSlave(pCurrent, TRUE, &context, &pElement, NULL, NULL));
        if (FAILED(hr))
            break; // not found

        // TODO: maybe we can be more agressive and handle EnterScope as well [ashrafm]
        if (context != CONTEXT_TYPE_ExitScope)
            break; // not found

        hr = THR(pMarkupServices->GetElementTagId(pElement, &tagIdCurrent));
        if (FAILED(hr))
            goto Cleanup;

        if (tagIdCurrent == tagIdTarget)
        {
            // found tagid
            if (ppElement)
            {
                *ppElement = pElement;
                pElement->AddRef();
            }
            if (ppLeft)
            {
                 *ppLeft = pCurrent;
                 pCurrent->AddRef();
            }
            if (pContext)
                *pContext = context;

            goto Cleanup;
        }
        ClearInterface(&pElement);
    }

    hr = S_FALSE; // not found
    if (ppElement)
        *ppElement = NULL;

    if (ppLeft)
        *ppLeft = NULL;

Cleanup:
    ReleaseInterface(pCurrent);
    ReleaseInterface(pElement);

    RRETURN1(hr, S_FALSE);
}

//=========================================================================
// CCommand: GetRightAdjacentTagId
//
// Synopsis: Moves the markup pointer to an element tag with the specified
//           TAGID.  However, the pointer is not advanced past any text.
//
// Returns:  S_OK if found
//           S_FALSE if not found
//
//-------------------------------------------------------------------------
HRESULT
CCommand::GetRightAdjacentTagId( 
    IMarkupServices *pMarkupServices,
    IHTMLViewServices *pViewServices,
    IMarkupPointer  *pMarkupPointer,
    ELEMENT_TAG_ID  tagIdTarget,
    IMarkupPointer  **ppLeft,
    IHTMLElement    **ppElement,
    MARKUP_CONTEXT_TYPE *pContext )
{
    HRESULT             hr;
    ELEMENT_TAG_ID      tagIdCurrent;
    IMarkupPointer      *pCurrent = NULL;
    IHTMLElement        *pElement = NULL;
    MARKUP_CONTEXT_TYPE context;
    //
    // Check to the left
    //

    hr = THR(CopyMarkupPointer(pMarkupServices, pMarkupPointer, &pCurrent));
    if (FAILED(hr))
        goto Cleanup;

    for (;;) {
        hr = THR(pViewServices->RightOrSlave(pCurrent, TRUE, &context, &pElement, NULL, NULL));
        if (FAILED(hr))
            break; // not found

        // TODO: maybe we can be more agressive and handle EnterScope as well [ashrafm]
        if (context != CONTEXT_TYPE_ExitScope)
            break; // not found

        hr = THR(pMarkupServices->GetElementTagId(pElement, &tagIdCurrent));
        if (FAILED(hr))
            goto Cleanup;

        if (tagIdCurrent == tagIdTarget)
        {
            // found tagid
            if (ppElement)
            {
                *ppElement = pElement;
                pElement->AddRef();
            }
            if (ppLeft)
            {
                 *ppLeft = pCurrent;
                 pCurrent->AddRef();
            }
            if (pContext)
                *pContext = context;

            goto Cleanup;
        }
        ClearInterface(&pElement);
    }

    hr = S_FALSE; // not found
    if (ppElement)
        *ppElement = NULL;

    if (ppLeft)
        *ppLeft = NULL;

Cleanup:
    ReleaseInterface(pCurrent);
    ReleaseInterface(pElement);

    RRETURN1(hr, S_FALSE);
}

HRESULT CCommand::SplitElement(  IMarkupServices *pMarkupServices,
                                 IHTMLElement    *pElement,
                                 IMarkupPointer  *pTagStart,
                                 IMarkupPointer  *pSegmentEnd,
                                 IMarkupPointer  *pTagEnd,
                                 IHTMLElement    **ppNewElement )
{
    HRESULT      hr;
    IHTMLElement *pNewElement = NULL;
    
#if DBG==1  // make sure we don't split the body
    ELEMENT_TAG_ID tagId;
    INT            iPosition;
    
    hr = pMarkupServices->GetElementTagId(pElement, &tagId);
    Assert(hr == S_OK && tagId != TAGID_BODY);

    // Make sure the order of pTagStart, pSegmentEnd, and pTagEnd is right
    hr = OldCompare( pTagStart, pSegmentEnd, &iPosition);
    Assert(hr == S_OK && iPosition != LEFT);
    
    hr = OldCompare( pSegmentEnd, pTagEnd, &iPosition);
    Assert(hr == S_OK && iPosition != LEFT);       
#endif    

    hr = THR( pSegmentEnd->SetGravity( POINTER_GRAVITY_Right ));
    if ( FAILED(hr))
        goto Cleanup;
    hr = THR( pTagEnd->SetGravity( POINTER_GRAVITY_Right ));
    if ( FAILED(hr))
        goto Cleanup;      
    //
    // Move element to first part of range
    //
    hr = THR(pMarkupServices->RemoveElement(pElement));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(InsertElement(pMarkupServices, pElement, pTagStart, pSegmentEnd));
    if (FAILED(hr))
        goto Cleanup;

    //
    // Clone element for the rest of the range
    //

    IFC( pMarkupServices->CloneElement( pElement, &pNewElement ) );

    IFC( pSegmentEnd->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFC( InsertElement(pMarkupServices, pNewElement, pSegmentEnd, pTagEnd) );
    

Cleanup:
    if (ppNewElement)
    {
        if (SUCCEEDED(hr))
        {
            *ppNewElement = pNewElement;
            pNewElement->AddRef();
        }
        else
        {
            *ppNewElement = NULL;
        }
    }

    ReleaseInterface(pNewElement);
    RRETURN(hr);
}

HRESULT 
CCommand::InsertBlockElement(IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    INT                 iSegmentCount;
    SELECTION_TYPE      selectionType;
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spPointer;
    BOOL                fNotAtBOL;
    
    //
    // Get the selection type
    //
    
    IFR( GetSegmentList(&spSegmentList) ); 
    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, &selectionType) );
    Assert(iSegmentCount);

    //
    // If we are a caret selection, make sure any new block elements inserted
    // at the caret position leave the caret inside
    //

    if (selectionType == SELECTION_TYPE_Caret)
    {        
        IFR( GetMarkupServices()->CreateMarkupPointer(&spPointer) );
        IFR( GetViewServices()->GetCaret(&spCaret) );
        IFR( spCaret->MovePointerToCaret(spPointer) );
        IFR( spCaret->GetNotAtBOL( &fNotAtBOL ));
        IFR( EdUtil::InsertBlockElement(GetMarkupServices(), pElement, pStart, pEnd, spPointer) );
        IFR( spCaret->MoveCaretToPointer(spPointer, fNotAtBOL, TRUE, CARET_DIRECTION_INDETERMINATE ));
    }
    else
    {
        IFR( GetEditor()->InsertElement(pElement, pStart, pEnd) );
    }

    RRETURN(hr);
}

HRESULT 
CCommand::CreateAndInsert(ELEMENT_TAG_ID tagId, IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement)
{
    HRESULT         hr;
    SP_IHTMLElement  spElement;

    IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
    IFR( InsertBlockElement(spElement, pStart, pEnd) );

    if (ppElement)
    {
        *ppElement = spElement;
        (*ppElement)->AddRef();
    }

    RRETURN(hr);    
}

HRESULT 
CCommand::CommonQueryStatus( 
        OLECMD *       pCmd,
        OLECMDTEXT *   pcmdtext )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    SELECTION_TYPE      eSelectionType;
    INT                 iSegmentCount;
    
    pCmd->cmdf = MSOCMDSTATE_UP; // up by default
    
    IFR( GetSegmentList( &spSegmentList ));

    //
    // If there is no segment count, return up
    //
    
    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );
    if (iSegmentCount <= 0) /// nothing to do
    {
        // enable by default
        if (eSelectionType == SELECTION_TYPE_None)        
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        
        return S_OK;
    }

    if (!CanAcceptHTML(spSegmentList))
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        return S_OK;
    }

     // Make sure the edit context is valid
    if (GetEditor())
    {
        CSelectionManager *pSelMan;
        
        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            if (!IsValidEditContext(pSelMan->GetEditableElement()))
            {
                pCmd->cmdf = MSOCMDSTATE_DISABLED;                
                return S_OK;
            }
        }
    }
    
    return S_FALSE; // not done
}

HRESULT 
CCommand::CommonPrivateExec( 
        DWORD                    nCmdexecopt,
        VARIANTARG *             pvarargIn,
        VARIANTARG *             pvarargOut )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    INT                 iSegmentCount;

    IFR( GetSegmentList(&spSegmentList) );
    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, NULL) );

    if (iSegmentCount <= 0) /// nothing to do
        return S_OK;
    
    if (!CanAcceptHTML(spSegmentList))
    {
        if (pvarargOut)
        {
            return S_FALSE; // not handled
        }
        return E_FAIL;
    }

    // Make sure the edit context is valid
    if (GetEditor())
    {
        CSelectionManager *pSelMan;
        
        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            if (!IsValidEditContext(pSelMan->GetEditableElement()))
                return E_FAIL;
        }
    }
    
    return S_FALSE; // not done
}


BOOL 
CCommand::CanAcceptHTML(ISegmentList *pSegmentList)
{
    HRESULT             hr;
    BOOL                bResult = FALSE;
    SP_IHTMLElement     spFlowElement;
    SP_IMarkupPointer   spStart;
    SELECTION_TYPE      eSelectionType;
    INT                 iSegmentCount;

    IFC( pSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );

    switch (eSelectionType)
    {
        case SELECTION_TYPE_Caret:
        case SELECTION_TYPE_Selection:
            bResult = GetEditor()->GetSelectionManager()->CanContextAcceptHTML();    
            break;

        case SELECTION_TYPE_Control:
            bResult = IsValidOnControl();
            break;
            
        default:
            IFC( GetSegmentPointers(pSegmentList, 0, &spStart, NULL) );
            IFC( GetViewServices()->GetFlowElement(spStart, &spFlowElement) );
            if ( spFlowElement )
            {
                IFC( GetViewServices()->IsContainerElement(spFlowElement, NULL, &bResult) );
            }                
    }

Cleanup:
    return bResult;
}

HRESULT 
CCommand::GetSegmentElement(ISegmentList *pSegmentList, INT i, IHTMLElement **ppElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart;
    
    *ppElement = NULL;    
    
    IFR( GetSegmentPointers(pSegmentList, i, &spStart, NULL) );

    // Get the element
    IFR( GetViewServices()->RightOrSlave(spStart, FALSE, NULL, ppElement, NULL, NULL) );
    Assert(*ppElement);

    return S_OK;
}

HRESULT
CCommand::CopyAttributes( IHTMLElement * pSrcElement, IHTMLElement * pDestElement, BOOL fCopyId)
{
    return EdUtil::CopyAttributes(GetViewServices(), pSrcElement, pDestElement, fCopyId);
}

HRESULT 
CCommand::AdjustSegment(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT         hr;    
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;
    SELECTION_TYPE  eSelectionType;

    IFR( GetSegmentList(&spSegmentList) );
    if (spSegmentList == NULL)
        return S_FALSE;
        
    IFR( spSegmentList->GetSegmentCount(&iSegmentCount, &eSelectionType) );

    if (iSegmentCount > 0 && eSelectionType == SELECTION_TYPE_Selection)
    {    
        CEditPointer epTest(GetEditor());
        DWORD        dwFound;

        //
        // If we have:              {selection start}...</p><p>{selection end} ...
        // we want to adjust to:    {selection start}...</p>{selection end}<p> ...
        // so that the edit commands don't apply to unselected lines
        //

        IFR( epTest->MoveToPointer(pEnd) );
        IFR( epTest.SetBoundary(pStart, NULL) );
        
        IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );        
        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            return S_OK;

        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_ExitBlock))
            IFR( pEnd->MoveToPointer(epTest) );
    }

    return S_OK;
}

