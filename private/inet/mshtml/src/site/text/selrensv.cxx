//+---------------------------------------------------------------------
//
//   File:      Selrensv.cpp
//
//  Contents:   Implementation of ISelectionRenderServices on CTxtEdit.
//
//  Classes:    CTxtEdit
//
//------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_SELRENSV_HXX_
#define X_SELRENSV_HXX_
#include "selrensv.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif



const int LEFT = 1;   
const int RIGHT = -1 ;
const int SAME = 0;
const int INVALIDATE_VERSION = - 1000000;

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

MtDefine(CSelectionRenderingServiceProvider, Tree , "CSelectionRenderingServiceProvider" )    
MtDefine(CSelRenSvcProvider_parySegment_pv, CSelectionRenderingServiceProvider , "CSelRenSvcProvider::_parySegment::_pv")
MtDefine(CSelRenSvcProvider_paryElementSegment_pv, CSelectionRenderingServiceProvider, "CSelRenSvcProvider::_paryElementSegment::_pv")
MtDefine(CSelRenSvcProvider_aryInvalLayouts_pv, Locals, "CSelRenSvcProvider::InvalidateSegment aryInvalLayouts::_pv");

DeclareTag(tagValidateSelRenSv, "Selection", "Validate Selection Rendering Changes")

#if DBG == 1
static const LPCTSTR strSelectionStartName = _T(" Start Selection Segment");
static const LPCTSTR strSelectionEndName = _T(" End Selection Segment");
#endif

#if DBG == 1
CSelectionRenderingServiceProvider::CSelectionRenderingServiceProvider(CDoc* pDoc, CMarkup* pMarkup )
#else
CSelectionRenderingServiceProvider::CSelectionRenderingServiceProvider(CDoc* pDoc )
#endif
{
    _lContentsVersion = INVALIDATE_VERSION;
    _pDoc = pDoc;
#if DBG == 1    
    _pMarkup = pMarkup ;
#endif    
}

CSelectionRenderingServiceProvider::~CSelectionRenderingServiceProvider()
{
    ClearSegments(FALSE);
    ClearElementSegments();
}

//+============================================================================================
//
//  Method: InvalidateSegment
//
//  Synopsis:: The Area from pStart to pEnd's "selectioness" has changed. 
//             Tell the renderer about it.
//
//---------------------------------------------------------------------------------------------

DeclareTag(tagDisplayShowInvalSegment, "Selection", "Selection show InvalidateSegment")

#define INVAL_LAYOUT 4

HRESULT 
CSelectionRenderingServiceProvider::InvalidateSegment( 
                                CMarkupPointer* pStart, 
                                CMarkupPointer *pEnd,
                                CMarkupPointer* pNewStart,
                                CMarkupPointer* pNewEnd ,
                                BOOL fSelected,
                                BOOL fFireOM /* = false*/ )
{
    CTreePos * pCurPos;
    CTreePos * pEndPos;
    CTreeNode * pNode;
    CLayout *pCurLayout = NULL;
    CTreePos* pPrevPos = NULL;
    HRESULT hr = S_OK;
    
    IGNORE_HR( pStart->Markup()->EmbedPointers() );

    pEndPos = pEnd->GetEmbeddedTreePos();
    
    BOOL fLayoutEnclosed = FALSE;
#if DBG == 1
    //
    // marka - we shouldn't be invalidating if End < Start or End == SAME 
    //
    int layoutStartCp , layoutEndCp;
    Assert( pStart->IsLeftOf( pEnd ) );
#endif

    CStackPtrAry < CLayout*, INVAL_LAYOUT > 
            aryInvalLayouts(Mt(CSelRenSvcProvider_aryInvalLayouts_pv));

    CTreeNode* pFirstNode = pStart->Branch();
    CLayout *pPrevLayout = pFirstNode->GetUpdatedNearestLayout();
    CTreePos* pLayoutStart =  pStart->GetEmbeddedTreePos() ;

    // If the selection is in a slave markup, GetNearestLayout() will return NULL
    // on the top-level nodes. In such a case, we use the layout of the markup's
    // master element.
    if (!pPrevLayout)
    {
        CMarkup * pMarkup = pFirstNode->GetMarkup();

        if (pMarkup->Master())
        {
            pPrevLayout = pMarkup->Master()->GetUpdatedLayout();
        }
    }

#if DBG == 1
    layoutEndCp = pEndPos->GetCp();
    layoutStartCp = pLayoutStart->GetCp( FALSE );
    Assert( layoutStartCp <= layoutEndCp );
#endif

    for ( pCurPos = pStart->GetEmbeddedTreePos() ; 
          (pCurPos ) &&  (pCurPos != pEndPos); 
          pCurPos = pCurPos->NextTreePos() )
    {
        //
        // Code to handle rendering glyphs in between the two pos's
        //
        if ( pCurPos->IsNode() && pCurPos->ShowTreePos(NULL) )
        {
            CTreeNode* pGlyphNode = pCurPos->Branch();
            CLayout* pParentLayout = pGlyphNode->GetUpdatedParentLayout();

            pParentLayout->ShowSelected( pGlyphNode->GetBeginPos() ,
                                         pGlyphNode->GetEndPos(), 
                                         fSelected, 
                                         fLayoutEnclosed, 
                                         fFireOM);
        }            

        if (  ( pCurPos && pCurPos->IsBeginElementScope() )
           || ( pPrevPos && pPrevPos->IsEndElementScope()) )
               
        {
            Assert( pCurPos ); // assume that if pPrevPos - we have a pCurPos
            pNode = pCurPos->GetBranch();
            pCurLayout = pNode->GetUpdatedNearestLayout();


            //
            // If we have a new layout - then we draw the previous layout.
            //
            if ( pPrevLayout !=  pCurLayout )
            {

                //
                // If this is the first time we are looping, then
                // pLayoutRunOwner will be NULL, so we need to check
                // that it is not
                //
                if ( pPrevLayout )
                {
                    TraceTag((tagDisplayShowInvalSegment, "InvalidteSegment: Inval prevLayout from tp:%d to %d:",pLayoutStart->_nSerialNumber, pCurPos->_nSerialNumber ));
#if DBG == 1
                    layoutEndCp = pCurPos->GetCp( FALSE );
                    layoutStartCp = pLayoutStart->GetCp( FALSE );
                    Assert( layoutStartCp <= layoutEndCp );
#endif  
                    fLayoutEnclosed = IsLayoutCompletelyEnclosed( pPrevLayout, pNewStart, pNewEnd);
                    pPrevLayout->ShowSelected( pLayoutStart, pCurPos, fSelected, fLayoutEnclosed, fFireOM  );
                    if ( fFireOM) 
                        aryInvalLayouts.Append( pPrevLayout );

                }

                pPrevLayout = pCurLayout ;
                pLayoutStart = pCurPos;
            }
        }
        pPrevPos = pCurPos;
    }

    //
    // Add final layout
    //
    if ( pPrevLayout )
    {
        TraceTag((tagDisplayShowInvalSegment, "InvalidateSegment: Inval Final Layout from tp:%d to %d:",pLayoutStart->_nSerialNumber, pEndPos->_nSerialNumber ));

#if DBG == 1
        layoutEndCp = pEndPos->GetCp();
        layoutStartCp = pLayoutStart->GetCp( FALSE );
        Assert( layoutStartCp <= layoutEndCp );
#endif        
        fLayoutEnclosed = IsLayoutCompletelyEnclosed( pPrevLayout, pNewStart, pNewEnd);
        pPrevLayout->ShowSelected( pLayoutStart, pEndPos, fSelected, fLayoutEnclosed, fFireOM );
        if ( fFireOM )
            aryInvalLayouts.Append( pPrevLayout );
    }     

    if ( fFireOM )
    {
        int i;
#if DBG == 1        
        CPtrAry<PointerSegment*> * parySegment = _parySegment;
#endif        
        CLayout* pLayout;
        CElement* pElement = NULL;
        
        for( i = aryInvalLayouts.Size(); i -- ; i > 0 )
        {
            Assert( parySegment == _parySegment );
            pLayout = aryInvalLayouts.Item(i);
            pElement = pLayout->ElementOwner();
            pElement->AddRef();
            
            if ( pLayout && pLayout->ElementOwner()->GetFirstBranch() )
            {                    
                aryInvalLayouts.Item(i)->OnSelectionChange();
            }
            BOOL fInTree = ( pElement->GetFirstBranch() != 0 );
            pElement->Release();
            
            if ( ! fInTree )
            {
                //
                // Something evil has transpired. The doc has been changed from under us
                // due to firing the OM. We now return E_FAIL.
                //
                hr = E_FAIL;
                break;
            }                
        }                
    }

    return hr;
    
}

//+============================================================================================
//
// Method: UpdateSegment
//
// Synopsis: The Given Segment is about to be changed. Work out what needs to be invalidated
//           and tell the renderer about it.
//
//  Passing pIStart, and pIEnd == NULL, makes the given segment hilite ( used on AddSegment )
//
//---------------------------------------------------------------------------------------------

DeclareTag(tagDisplaySelectedSegment , "Selection", "Selection Show Update Segement")


//+====================================================================================
//
// Method: IsLayoutCompletelySelected
//
// Synopsis: Check to see if a layout is completely enclosed by a pair of treepos's
//
//------------------------------------------------------------------------------------


BOOL 
CSelectionRenderingServiceProvider::IsLayoutCompletelyEnclosed( 
                                                                CLayout* pLayout, 
                                                                CMarkupPointer* pStart, 
                                                                CMarkupPointer* pEnd )
{
    BOOL fCompletelyEnclosed = FALSE;
    int iWhereStart = 0;
    int iWhereEnd = 0;
    CMarkupPointer* pLayoutStart = NULL;
    CMarkupPointer* pLayoutEnd = NULL;
    HRESULT hr = S_OK;

    if ( !pStart || ! pEnd )
        return FALSE;

    if ( ! pLayout->ElementOwner()->GetFirstBranch() )
        return FALSE;
        
    hr = THR( pStart->Doc()->CreateMarkupPointer( & pLayoutStart ));
    if ( hr )
        goto Cleanup;
        
    hr = THR( pEnd->Doc()->CreateMarkupPointer( & pLayoutEnd ));
    if ( hr )
        goto Cleanup;
        
    hr = THR ( pLayoutStart->MoveAdjacentToElement( pLayout->ElementOwner(), ELEM_ADJ_BeforeBegin));
    if ( hr )
        goto Cleanup;
        
    hr = THR ( pLayoutEnd->MoveAdjacentToElement( pLayout->ElementOwner(), ELEM_ADJ_AfterEnd ));
    if ( hr )
        goto Cleanup;

    if ( pLayoutStart->Markup() != pStart->Markup() )
        goto Cleanup;
        
    iWhereStart = OldCompare( pStart, pLayoutStart );

    if ( pLayoutEnd->Markup() != pEnd->Markup() )
        goto Cleanup;
        
    iWhereEnd = OldCompare( pEnd, pLayoutStart );

    fCompletelyEnclosed = ( ( iWhereStart != LEFT ) && ( iWhereEnd != RIGHT ) ) ;
    if ( fCompletelyEnclosed )
    {
        iWhereStart = OldCompare( pStart, pLayoutEnd);
        iWhereEnd = OldCompare( pEnd, pLayoutEnd );
        fCompletelyEnclosed = ( ( iWhereStart != LEFT ) && ( iWhereEnd != RIGHT ) ) ;        
    }

Cleanup:
    ReleaseInterface( pLayoutStart );
    ReleaseInterface( pLayoutEnd );
    return fCompletelyEnclosed ;
}


HRESULT
CSelectionRenderingServiceProvider::UpdateSegment( CMarkupPointer* pOldStart, CMarkupPointer* pOldEnd, CMarkupPointer* pNewStart, CMarkupPointer* pNewEnd )
{
    int ssStart;
    int ssStop;
    BOOL fSame = FALSE;
    int compareStartEnd = SAME;
    HRESULT hr = S_OK;
    
    _fInUpdateSegment = TRUE;
    
    compareStartEnd = OldCompare( pNewStart, pNewEnd ) ;
    switch( compareStartEnd )
    {
        case LEFT:
        {
            CMarkupPointer* pTemp = pNewStart;
            pNewStart = pNewEnd;
            pNewEnd = pTemp;
        }
        break;

        case SAME:
        fSame = TRUE;
        break;
    }

    
    if ( !fSame && ( pOldStart != NULL ) && ( pOldEnd != NULL ) )
    {
        //
        // Initialize pOldStart, pOldEnd, pNewStart, pNewEnd, handling Start > End.
        //
        if ( OldCompare( pOldStart, pOldEnd ) == LEFT )
        {
            CMarkupPointer* pTemp2 = pOldStart;
            pOldStart = pOldEnd;
            pOldEnd = pTemp2;
        }

        
        if ( ( OldCompare( pOldStart, pNewEnd ) == LEFT ) ||  // New End is to the left
             ( OldCompare( pOldEnd, pNewStart) == RIGHT ) )   // New Start is to right
        {
            //
            // Segments do not overlap
            //   
            TraceTag( ( tagDisplaySelectedSegment, "Non Overlap") );
            if (OldCompare( pOldStart, pOldEnd ) != SAME)
            {
                IFC( InvalidateSegment( pOldStart, pOldEnd, pNewStart, pNewEnd , FALSE )); // hide old selection
            }
            if (OldCompare( pNewStart, pNewEnd ) != SAME)
            {
                IFC( InvalidateSegment( pNewStart, pNewEnd,  pNewStart, pNewEnd , TRUE ));
            }
        }
        else
        {
            ssStart = OldCompare( pOldStart, pNewStart) ;
            ssStop  = OldCompare( pOldEnd, pNewEnd );

            //
            // Note that what we invalidate is the Selection 'delta', ie that portion of the selection
            //  whose "highlightness" is now different. We don't turn on/off parts that havent' changed
            //
            switch( ssStart )
            {
                case RIGHT:
                    switch( ssStop )
                    {
                        //
                        //      S    NS     NE   E
                        //
                        case LEFT:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 1. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pOldStart, pNewStart,  pNewStart, pNewEnd ,FALSE )); // hide old selection
                            IFC( InvalidateSegment( pNewEnd, pOldEnd, pNewStart, pNewEnd , FALSE ));
                        }
                        break;
                        //
                        //      S    NS      E    NE
                        //
                        case RIGHT: 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 2. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pOldStart, pNewStart,pNewStart, pNewEnd ,FALSE ));
                            IFC( InvalidateSegment( pOldEnd, pNewEnd, pNewStart, pNewEnd, TRUE ));
                        }
                        break;
                        //
                        //      S    NS      E   
                        //                   NE
                        case SAME:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 3. ssStart:%d, ssEnd:%d", ssStart, ssStop ));                        
                            IFC( InvalidateSegment( pOldStart, pNewStart, pNewStart, pNewEnd , FALSE  ));
                        }
                        break;
                    }
                    break;
                    
                case LEFT : 
                    switch( ssStop )
                    {
                        //
                        //      NS  S   NE E
                        //
                        case LEFT:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 4. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pNewEnd , pOldEnd ,  pNewStart, pNewEnd , FALSE )); // hide old selection
                            IFC( InvalidateSegment( pNewStart, pOldStart, pNewStart, pNewEnd, TRUE  ));
                        }
                        break;
                        //
                        //     NS    S      E    NE
                        //
                        case RIGHT : 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 5. ssStart:%d, ssEnd:%d", ssStart, ssStop ));                            
                            IFC( InvalidateSegment( pNewStart, pOldStart, pNewStart, pNewEnd  , TRUE ));
                            IFC( InvalidateSegment( pOldEnd, pNewEnd, pNewStart, pNewEnd , TRUE  ));                                 
                        }
                        break;
                        
                       //
                       //      NS    S      E
                       //                   NE
                        case SAME:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 6. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pNewStart, pOldStart, pNewStart, pNewEnd , TRUE ));
                        }
                        break;
                    }
                    break;
                    
                default: // or same - which gives us less invalidation
                    switch( ssStop )
                    {
                        //
                        //      NS     NE E
                        //      S    
                        case LEFT:
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 7. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pNewEnd , pOldEnd , pNewStart, pNewEnd , FALSE )); // hide old selection
                        }
                        break;
                        //
                        //      S         E    NE
                        //      NS
                        case RIGHT : 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 8. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                            IFC( InvalidateSegment( pOldEnd, pNewEnd,  pNewStart, pNewEnd, TRUE  ));
                        }
                        break;

                        //
                        // For SAME case - we do nothing.
                        //
                        case SAME : 
                        {
                            TraceTag(( tagDisplaySelectedSegment, "Overlap 9. ssStart:%d, ssEnd:%d", ssStart, ssStop ));
                        }
                        break;
                    }
                    break;                    
            } 
        }
    }
    else
    {
        TraceTag(( tagDisplaySelectedSegment, "New Segment Invalidation"));

        if ( !fSame )
        {
            IFC( InvalidateSegment( pNewStart , pNewEnd ,  pNewStart, pNewEnd , TRUE ));        
        }            
        else if ( ( pOldStart != NULL ) && ( pOldEnd != NULL ) )
        {
            int iCompareOld = OldCompare( pOldStart, pOldEnd );
            if ( iCompareOld == LEFT )
            {
                CMarkupPointer* pTemp2 = pOldStart;
                pOldStart = pOldEnd;
                pOldEnd = pTemp2;
            }        
            if ( iCompareOld != SAME )
            {
                IFC( InvalidateSegment( pOldStart , pOldEnd ,  pNewStart, pNewEnd , FALSE ));   
            }                
        }
    }

    _fInUpdateSegment = FALSE;
    
Cleanup:
    RRETURN( hr );
}


//+====================================================================================
//
// Method: NotifyBeginSelection
//
// Synopsis: Post a WM_BEGINSELECTION message, to every Trident HWND.
//           This allows a recipient of this message to clear its own selection 
//
//           This is used for clearing selections between multiple nested tridents 
//              (eg. nested Iframes).
//              
//
//------------------------------------------------------------------------------------

HRESULT
CSelectionRenderingServiceProvider::NotifyBeginSelection(WPARAM wParam)
{
    HRESULT hr = S_OK;
    HWND hwndCur, hwndParent;
    HWND hwndTopTrident = NULL;

    if ( ! _pDoc->_pInPlace )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    hwndCur = _pDoc->_pInPlace->_hwnd;

    
    _pDoc->_fNotifyBeginSelection = TRUE;

    //
    // Find the topmost Instance of Trident
    //
    while(hwndCur)
    {
        if ( IsTridentHwnd( hwndCur ))
        {
            hwndTopTrident = hwndCur;
        }    
        hwndParent = hwndCur;
        hwndCur = GetParent(hwndCur);        
    }
    
    ::SendMessage( hwndTopTrident,
                   WM_BEGINSELECTION, 
                   wParam ,
                   0);
    
    _pDoc->_fNotifyBeginSelection = FALSE;    
Cleanup:
    RRETURN( hr );
}

//+==================================================================================
//
// Method: AddSegment
//
// Synopsis: Add a Selection Segment, at the position given by two MarkupPointers.
//           Expected that Segment Type is either SELECTION_RENDER_Selected or None
//
//-----------------------------------------------------------------------------------

HRESULT 
CSelectionRenderingServiceProvider::AddSegment( 
    IMarkupPointer* pIStart, 
    IMarkupPointer* pIEnd,
    HIGHLIGHT_TYPE HighlightType,
    int * piSegmentIndex )  
{
    HRESULT hr = S_OK;
    CMarkupPointer* pInternalStart = NULL;
    CMarkupPointer* pInternalEnd = NULL;
    PointerSegment * pNewPair = NULL;

    BOOL fEqual;
    BOOL fFireNotify = FALSE;

    POINTER_GRAVITY eGravity = POINTER_GRAVITY_Left;    // need to initialize

    Assert( pIStart && pIEnd );
    if ( ! _fSelectionVisible )
        _fSelectionVisible = TRUE;


    hr = THR( pIStart->IsEqualTo( pIEnd, & fEqual ));
    AssertSz( hr != CTL_E_INCOMPATIBLEPOINTERS ,"Pointers in different markups");
    if ( hr )
        goto Cleanup;

    if ( ! fEqual )
    {
        if ( NotifyBeginSelection(START_TEXT_SELECTION) == S_OK )      
            fFireNotify = TRUE;
    }
    
    if ( ! _parySegment )
    {
        _parySegment = new  (Mt(CSelRenSvcProvider_parySegment_pv)) CPtrAry<PointerSegment*> (Mt(CSelRenSvcProvider_parySegment_pv));
        if ( ! _parySegment )
            goto Error;
    }    
    pInternalStart = new CMarkupPointer( _pDoc );
    if ( ! pInternalStart ) goto Error;
    pInternalStart->SetAlwaysEmbed( TRUE );
    
    pInternalEnd = new CMarkupPointer( _pDoc );
    if ( ! pInternalEnd ) goto Error;
    pInternalEnd->SetAlwaysEmbed( TRUE);
    
    pNewPair = new PointerSegment;
    if ( ! pNewPair )
        goto Error;

    pNewPair->_HighlightType = HighlightType;
    pNewPair->_pStart = pInternalStart;
    pNewPair->_pEnd = pInternalEnd;
    pNewPair->_fFiredSelectionNotify = fFireNotify;
    
    hr = pInternalStart->MoveToPointer( pIStart );
    if ( !hr ) hr = pIStart->Gravity( &eGravity );            // need to maintain gravity
    if ( !hr ) hr = pInternalStart->SetGravity( eGravity );

    if ( !hr ) hr = pInternalEnd->MoveToPointer( pIEnd );
    if ( !hr ) hr = pIEnd->Gravity( &eGravity );
    if ( !hr ) hr = pInternalEnd->SetGravity( eGravity );     // need to maintain gravity

    if ( hr )
    {
        delete pNewPair;
        goto Cleanup;
    }

    //
    // Check that we are in the same tree.
    //
#if DBG == 1
    Assert( pInternalStart->Markup() == _pMarkup );
    Assert( pInternalEnd->Markup() == _pMarkup );
    pInternalStart->SetDebugName( strSelectionStartName );
    pInternalEnd->SetDebugName( strSelectionEndName );
#endif
    _parySegment->Append( pNewPair );
    if ( piSegmentIndex )
        *piSegmentIndex = _parySegment->Size() - 1 ;

    _lContentsVersion = INVALIDATE_VERSION; // Invalidate CP cache.
    
    hr = THR( UpdateSegment( NULL, NULL, pInternalStart, pInternalEnd ));    


    
Cleanup:
    if ( FAILED(hr) )
    {
        ReleaseInterface( pInternalStart );
        ReleaseInterface( pInternalEnd );
    }

    RRETURN( hr );

Error:
    return E_OUTOFMEMORY;
}
        
//+===============================================================================
//
// Method: AddElementSegment
//
// Synopsis: Add's an Element Segment of the given type
//
//--------------------------------------------------------------------------------

HRESULT 
CSelectionRenderingServiceProvider::AddElementSegment( 
    IHTMLElement* pIElement ,
    int * piSegmentIndex )  
{
    CElement* pElement = NULL;
    HRESULT hr = S_OK;

    //
    // Post a notify to clear any selection
    //

    NotifyBeginSelection(START_CONTROL_SELECTION);
    
    if ( ! _fSelectionVisible )
        _fSelectionVisible = TRUE;

    if ( ! _paryElementSegment )
    {
        _paryElementSegment = new (Mt(CSelRenSvcProvider_paryElementSegment_pv ))  CPtrAry<CElement*> ( Mt(CSelRenSvcProvider_paryElementSegment_pv) );
        if ( ! _paryElementSegment ) goto Error;        
    }
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    if ( hr ) goto Cleanup;

    //
    // Put a slave inside the array of things we're storing this on.
    //
    if ( pElement->_etag == ETAG_TXTSLAVE )
    {
        pElement = pElement->MarkupMaster();
    }
    
    _paryElementSegment->Append( pElement );
    if ( piSegmentIndex )
        *piSegmentIndex = _paryElementSegment->Size() - 1;
    
Cleanup:

    RRETURN( hr );
    
Error:
    return E_OUTOFMEMORY;    
}

CElement* 
CSelectionRenderingServiceProvider::GetSelectedElement( 
    int iElement  )
{
    if ( _paryElementSegment )
    {
          return  _paryElementSegment->Item( iElement );
    }
    else
    {
        return NULL;
    }    
}

BOOL
CSelectionRenderingServiceProvider::IsElementSelected( CElement* pElement )
{
    BOOL fSelected = FALSE;
    CElement** ppElement = NULL;
    int i;
    
    if ( _paryElementSegment )
    {
         for (i = _paryElementSegment->Size(), ppElement =  * _paryElementSegment ;
                     i > 0;
                     i--, ppElement++)
        {
            if (( *ppElement ) == pElement )
            {
                fSelected = TRUE;
                break;
            }                
        }    
    }

    return ( fSelected );
}


//+====================================================================================
//
// Method: GetFlattenedSelection
//
// Synopsis: Get cp counts for a "flattened" selection ( for undo )
//
//------------------------------------------------------------------------------------

HRESULT 
CSelectionRenderingServiceProvider::GetFlattenedSelection( 
                                                            int iSegmentIndex, 
                                                            int & cpStart, 
                                                            int & cpEnd, 
                                                            SELECTION_TYPE&  eType )
{
    HRESULT hr = S_OK;
    PointerSegment* pPair = NULL;
    IMarkupPointer* pICaretPointer = NULL;
    CMarkupPointer* pPointerInternal = NULL;
    
    IHTMLCaret* pCaret = NULL;

    if ( ( _parySegment ) && ( _parySegment->Size() != 0 ) )
    {
        pPair = _parySegment->Item( iSegmentIndex );        
        if ( _pDoc->GetDocContentsVersion() != _lContentsVersion  )
        {
            cpStart = pPair->_pStart->GetCp();
            cpEnd = pPair->_pEnd->GetCp(); 
            if ( cpStart > cpEnd )
            {
                int cpTemp = cpStart;
                cpStart = cpEnd;
                cpEnd = cpTemp;
            }
        }
        else
        {
            cpStart = pPair->_cpStart;
            cpEnd = pPair->_cpEnd;
        }
        eType = SELECTION_TYPE_Selection;
    }
    else if ( _pDoc->IsCaretVisible() )
    {
        hr = THR( _pDoc->GetCaret( & pCaret ));
        if ( hr )
            goto Cleanup;
            
        hr = THR( _pDoc->CreateMarkupPointer( & pICaretPointer ));
        if ( hr )
            goto Cleanup;

        hr = THR( pCaret->MovePointerToCaret( pICaretPointer ));
        if ( hr )
            goto Cleanup;
            
        hr = THR( pICaretPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointerInternal));
        if ( hr )
            goto Cleanup;

        cpStart = pPointerInternal->GetCp();
        cpEnd = cpStart;
        eType = SELECTION_TYPE_Caret;              
    }            
    else if ( ( _paryElementSegment ) && ( _paryElementSegment->Size() != 0))
    {
        CElement * pElement = _paryElementSegment->Item( iSegmentIndex );
        Assert( pElement );
        //
        // marka - we want the characters just before, and just after the element
        //
        cpStart = pElement->GetFirstCp() - 1;
        cpEnd= pElement->GetLastCp() + 1;
        eType = SELECTION_TYPE_Control;
    } 
    else
        AssertSz( 0, "No Such Segment Exists!");

Cleanup:
    ReleaseInterface( pCaret );
    ReleaseInterface( pICaretPointer);
    RRETURN( hr );
}

HRESULT 
CSelectionRenderingServiceProvider::MovePointersToSegment( 
    int iSegmentIndex, 
    IMarkupPointer* pIStart, 
    IMarkupPointer* pIEnd ) 
{
    HRESULT hr = S_OK;
    PointerSegment* pPair = NULL;
    IHTMLElement* pIElement = NULL;
    IHTMLCaret* pCaret = NULL;
    int wherePointer = SAME;
    Assert( pIStart && pIEnd );

    
    if ( ( _parySegment ) && ( _parySegment->Size() != 0 ) )
    {
        pPair = _parySegment->Item( iSegmentIndex );
        
        hr = THR( OldCompare( pPair->_pStart, pPair->_pEnd, & wherePointer ) );
        if ( hr )
            goto Cleanup;
        //
        // Swap pointers if they're out of order.
        //
        if ( wherePointer != LEFT )             
        {
            hr = pIStart->MoveToPointer( pPair->_pStart );
            if (!hr) hr = pIEnd->MoveToPointer( pPair->_pEnd );
        }
        else
        {
            hr = pIStart->MoveToPointer( pPair->_pEnd );
            if (!hr) hr = pIEnd->MoveToPointer( pPair->_pStart );
        }
        
        //
        // copy gravity - important for commands
        //

        //if ( !hr ) hr = pPair->_pStart->Gravity( &eGravity );            // need to maintain gravity
        if ( !hr ) hr = pIStart->SetGravity( POINTER_GRAVITY_Right );
        //if ( !hr ) hr = pPair->_pEnd->Gravity( &eGravity );            // need to maintain gravity
        if ( !hr ) hr = pIEnd->SetGravity( POINTER_GRAVITY_Left );
    }
    else
    {
        BOOL fVisible = FALSE;
        BOOL fPositioned = FALSE;

        fVisible = _pDoc->IsCaretVisible( & fPositioned );
        
        if ( fVisible && fPositioned )
        {
            Assert( iSegmentIndex == 0 );

            hr = THR( _pDoc->GetCaret( & pCaret ));
            if ( hr )
                goto Cleanup;
                
            hr = THR( pCaret->MovePointerToCaret( pIStart ));
            if ( hr )
                goto Cleanup;
                
            hr = THR( pIStart->SetGravity(POINTER_GRAVITY_Right ));
            if ( hr )
                goto Cleanup;

            hr = THR( pCaret->MovePointerToCaret( pIEnd ));
            if ( hr )
                goto Cleanup;            

            hr = THR( pIEnd->SetGravity(POINTER_GRAVITY_Left ));
            if ( hr )
                goto Cleanup;
              
        }            
        else
        {
            if ( ( _paryElementSegment ) && ( _paryElementSegment->Size() != 0))
            {
                CElement * pElement = _paryElementSegment->Item( iSegmentIndex );
                Assert( pElement );
                hr = THR( pElement->QueryInterface( IID_IHTMLElement, ( void**) & pIElement ));
                if ( hr )
                    goto Cleanup;
                    
                hr = THR( pIStart->MoveAdjacentToElement( pIElement , ELEM_ADJ_BeforeBegin ));
                if ( hr )
                {
                    if ( pElement->GetFirstBranch() == NULL)
                    {
                        AssertSz(0, "Selected element - is no longer in the tree !");
                        //
                        // This Element is no longer in the tree. 
                        // This is bad - but we try to clean up.
                        //
                        // BUGBUG - this is the general problem of the tree being deleted
                        // out from under a selected/adorned element. We need to fix this.
                        //
                        _paryElementSegment->Delete(iSegmentIndex );
                    }
                    
                    goto Cleanup;
                }                    
                hr = THR( pIStart->SetGravity(POINTER_GRAVITY_Right ));
                if ( hr )
                    goto Cleanup;
                    
                hr = THR( pIEnd->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd ));
                if ( hr )
                    goto Cleanup;
                    
                hr = THR( pIEnd->SetGravity(POINTER_GRAVITY_Left ));
                if ( hr )
                    goto Cleanup;                    
            } 
            else
                AssertSz( 0, "No Such Segment Exists!");
        }

    }
Cleanup:

#if DBG == 1
    BOOL fPositionedStart = FALSE;
    BOOL fPositionedEnd = FALSE;

    IGNORE_HR( pIStart->IsPositioned( & fPositionedStart ));
    IGNORE_HR( pIEnd->IsPositioned( & fPositionedEnd ));

    Assert( fPositionedStart && fPositionedEnd );
#endif
    ReleaseInterface( pCaret );
    ReleaseInterface( pIElement );
    RRETURN( hr );
}    

                
HRESULT
CSelectionRenderingServiceProvider::GetSegmentCount(
    int* piSegmentCount,
    SELECTION_TYPE * peType )
{
    HRESULT hr = S_OK;
    Assert( piSegmentCount );
    int iSegmentCount = 0;
    SELECTION_TYPE myType  = SELECTION_TYPE_None;
   
    //
    // Check for a segment in the SegmentList, the Caret, or the Element Segment List
    // all are mutually exclusive, or we assert
    //
    if (( _parySegment) && ( _parySegment->Size() != 0 ))
    {
        iSegmentCount = _parySegment->Size();
        myType = SELECTION_TYPE_Selection;
    }        
    else
    {
        BOOL fVisible = FALSE;
        BOOL fPositioned = FALSE;

        fVisible = _pDoc->IsCaretVisible( & fPositioned );
        if ( fVisible && fPositioned )
        {
            iSegmentCount = 1;
            myType = SELECTION_TYPE_Caret;
        }            
        else
        {
            if ( _paryElementSegment )
            {
                iSegmentCount = _paryElementSegment->Size();
                if ( iSegmentCount > 0 )
                    myType = SELECTION_TYPE_Control;
            }                
        }

    }
    if ( iSegmentCount == 0 )
       myType = SELECTION_TYPE_None;
       
#if NEVER
    //
    // marka TODO - make these assert work !
    //
    { 
        SELECTION_TYPE editType = _pDoc->GetSelectionType();
        Assert( editType == myType );    
    }
#endif

    *piSegmentCount = iSegmentCount;
    
    if ( peType )
        *peType =  myType;
    
    RRETURN ( hr ) ;
}


HRESULT 
CSelectionRenderingServiceProvider::GetElementSegment( 
    int iSegmentIndex, 
    IHTMLElement** ppIElement) 
{
    HRESULT hr = S_OK;
    CElement* pElement = NULL;

    Assert( ppIElement );
    Assert( iSegmentIndex <= _paryElementSegment->Size());

    pElement = _paryElementSegment->Item( iSegmentIndex );
    if ( pElement )
    {
        hr = pElement->QueryInterface(IID_IHTMLElement, (void**) ppIElement );
    }
    else
    {
        hr = E_FAIL;
    }
    RRETURN( hr );
}    
    
HRESULT 
CSelectionRenderingServiceProvider::MoveSegmentToPointers( 
    int iSegmentIndex,
    IMarkupPointer* pIStart, 
    IMarkupPointer* pIEnd,
    HIGHLIGHT_TYPE HighlightType )  
{
    HRESULT hr = S_OK;
    CMarkupPointer * pNewStart = NULL ;
    CMarkupPointer * pNewEnd = NULL ;
    CMarkupPointer * pOldStart = NULL;
    CMarkupPointer * pOldEnd = NULL;
    
    POINTER_GRAVITY eGravity = POINTER_GRAVITY_Left;
    Assert( pIStart && pIEnd );
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( pIStart->IsPositioned( & fPositioned ));
    Assert ( fPositioned );
    IGNORE_HR( pIEnd->IsPositioned( & fPositioned ));
    Assert ( fPositioned );
    int cpStartNew =0;
    int cpEndNew = 0;
    int cchNewSel = 0;
    int cpStart = 0;
    int cpEnd = 0;
    int cchSel = 0;
#endif
    PointerSegment *pSegment = NULL;

    
    if ( ! _fSelectionVisible )
        _fSelectionVisible = TRUE;

    if ( !_parySegment || iSegmentIndex >= _parySegment->Size() )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pSegment = _parySegment->Item( iSegmentIndex );
    if ( ! pSegment )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if ( ! pSegment->_fFiredSelectionNotify )
    {
        BOOL fEqual;
        hr = THR( pIStart->IsEqualTo( pIEnd , & fEqual ));
        AssertSz( hr != CTL_E_INCOMPATIBLEPOINTERS, "Pointers in Different markups !");
        if ( hr )
            goto Cleanup;

        if ( ! fEqual )
        {
            if ( NotifyBeginSelection(START_TEXT_SELECTION) == S_OK )
                pSegment->_fFiredSelectionNotify = TRUE;
        }
    }
    
    pSegment->_HighlightType = HighlightType;

    hr = THR( pIStart->QueryInterface( CLSID_CMarkupPointer, (void**) & pNewStart));
    if (!hr)
        hr = THR( pIEnd->QueryInterface( CLSID_CMarkupPointer, (void**) & pNewEnd ));
    if ( hr)
        goto Cleanup;   

#if DBG == 1
    if ( IsTagEnabled( tagValidateSelRenSv))
    {
        cpStartNew = pNewStart->GetCp();
        cpEndNew= pNewEnd->GetCp();
        cchNewSel = abs( cpStartNew - cpEndNew );
        cpStart = pSegment->_pStart->GetCp();
        cpEnd = pSegment->_pEnd->GetCp();
        cchSel = abs( cpStart - cpEnd );
        TraceTag(( tagValidateSelRenSv, "Selection - Old Start:%d End:%d Size:%d New Start:%d End:%d Size:%d\n", 
                cpStart, cpEnd, cchSel, cpStartNew, cpEndNew, cchNewSel ));
        _cchLast = cchSel;
    }
#endif

    //
    // Move Old Pointers to start and end.
    //
    IFC( _pDoc->CreateMarkupPointer( & pOldStart ));
    IFC( _pDoc->CreateMarkupPointer( & pOldEnd ));
    IFC( pOldStart->MoveToPointer( pSegment->_pStart ));
    IFC( pOldEnd->MoveToPointer( pSegment->_pEnd ));

    
    IFC( pSegment->_pStart->MoveToPointer( pIStart ));
    IFC( pSegment->_pEnd->MoveToPointer( pIEnd )) ;

    //
    // BUGBUG - rewrite Update Segment sometime - so it doesn't need to take
    // CMarkupPointers as parameters for the old selection boundarys (perf).
    //
    IFC( UpdateSegment( pOldStart, pOldEnd,  pSegment->_pStart , pSegment->_pEnd ) );

    //
    // Copy Gravity on pointers.
    //
    IFC( pIStart->Gravity ( & eGravity));
    IFC( pSegment->_pStart->SetGravity( eGravity ));
    IFC( pIEnd->Gravity ( & eGravity));
    IFC( pSegment->_pEnd->SetGravity( eGravity ));
    
    _lContentsVersion = INVALIDATE_VERSION; // Invalidate CP cache.

    //
    // While processing this - we got an Invalidate call, which is invalid (as we had'nt 
    // updated our selection as yet.
    //
    if ( _fPendingInvalidate )
    {
        InvalidateSelection( TRUE, _fPendingFireOM );
        _fPendingInvalidate = FALSE;
    }

Cleanup:
    if ( pOldStart ) pOldStart->Release();
    if ( pOldEnd ) pOldEnd->Release();
    
    RRETURN( hr );
}    
    
HRESULT 
CSelectionRenderingServiceProvider::SetElementSegment( 
    int iSegmentIndex,
    IHTMLElement* pIElement)  
{
    HRESULT hr = S_OK;
    CElement* pElement = NULL;

    Assert( pIElement );

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr) goto Cleanup;

    _paryElementSegment->Insert( iSegmentIndex, pElement );
    _paryElementSegment->Delete( iSegmentIndex + 1 );
    
Cleanup:    
    RRETURN( hr );
}    


//+===================================================================================
//
// Method: ClearSegments
//
// Synopsis: Empty ( remove all ) selection segments. ( ie nothing is selected )
//
//-----------------------------------------------------------------------------------

HRESULT
CSelectionRenderingServiceProvider::ClearSegments(BOOL fInvalidate)
{
    int i;
    
    if ( _parySegment )
    {
        for (i = _parySegment->Size(); i--; )
        {
            if (_parySegment->Item(i))
            {
                ClearSegment( i, fInvalidate );
            }
        }     

        delete _parySegment;
        _parySegment = NULL;
    }   
    return S_OK;
}


HRESULT
CSelectionRenderingServiceProvider::ClearSegment(int iSegmentIndex, BOOL fInvalidate)
{
    HRESULT hr;
    
    if (iSegmentIndex >= _parySegment->Size())
    {
        hr = E_INVALIDARG;
    }
    else
    {
        PointerSegment* pPointerPair = _parySegment->Item(iSegmentIndex);

        if ( fInvalidate )
        {
            int wherePointer = OldCompare( pPointerPair->_pStart, pPointerPair->_pEnd);

            switch( wherePointer )
            {
                case RIGHT:
                    InvalidateSegment( pPointerPair->_pStart, pPointerPair->_pEnd,NULL, NULL , FALSE, FALSE );
                    break;

                case LEFT:
                    InvalidateSegment( pPointerPair->_pEnd, pPointerPair->_pStart, NULL, NULL, FALSE, FALSE  );
                    break;
            }
        }

        pPointerPair->_pStart->Unposition();
        pPointerPair->_pEnd->Unposition();
        delete( pPointerPair->_pStart );
        delete( pPointerPair->_pEnd );
        delete( pPointerPair );
        _parySegment->Item(iSegmentIndex) = NULL;

        hr = S_OK;
    }

    RRETURN(hr);
}

HRESULT
CSelectionRenderingServiceProvider::ClearElementSegments()
{
    if ( _paryElementSegment )
    {
        delete _paryElementSegment;
        _paryElementSegment = NULL;
    }
    return S_OK;
    
}

        
//+===============================================================================
//
// Method: ConstructSelectionRenderCache
//
// Synopsis: This method is called by the renderer at the start of a paint,
//
//             As it is expected that the work involved in "chunkifying" the selection
//             will be expensive. So this work is done once per render.
//
//             As each flow layout is rendered, the Renderer calls GetSelectionChunksForLayout,
//             which essentially returns part of the array built on this call.
// 
// Build an array of FlowLayoutChunks. A "chunk" is a FlowLayout, and pair of 
// cps within that FlowLayout that are selected. Hence Selections that span multiple
// layouts are broken up by this routine into separate chunks for the renderer.
// 
//--------------------------------------------------------------------------------

VOID 
CSelectionRenderingServiceProvider::ConstructSelectionRenderCache()
{
    PointerSegment** ppPointerSegment;    
    int i;
    int start, end;

    //
    // We only need to recompute CP's if our version nos' are different.
    //
    Assert( ( _parySegment ) && ( _pDoc->GetDocContentsVersion() != _lContentsVersion ) );

    for ( i = _parySegment->Size(), ppPointerSegment = *_parySegment;
        i > 0 ;
        i --, ppPointerSegment++)
    {
        if (!*ppPointerSegment)
            continue;
        
        start = (*ppPointerSegment)->_pStart->GetCp();
        end = (*ppPointerSegment)->_pEnd->GetCp(); 

        //
        // Make sure we are embedded.
        //
        Assert( (*ppPointerSegment)->_pStart->GetEmbeddedTreePos() != NULL );
        Assert( (*ppPointerSegment)->_pEnd->GetEmbeddedTreePos() != NULL );

        if ( start > end )
        {
            (*ppPointerSegment)->_cpStart = end; 
            (*ppPointerSegment)->_cpEnd = start;
        }
        else
        {
            (*ppPointerSegment)->_cpStart =  start;
            (*ppPointerSegment)->_cpEnd = end;
        }
    }

    //
    // marka - take this call out for now - we will try to use _lContentsVersion
    // to figure out when we need to reconstruct the cache.
    //
#ifdef NEVER
    WHEN_DBG( _fInPaint = TRUE; )    
#endif    
}
//+====================================================================================
//
// Method: HideSelection
//
// Synopsis: Selection is to be hidden. Anticipated use is switching focus to another window
//
//------------------------------------------------------------------------------------

VOID
CSelectionRenderingServiceProvider::HideSelection()
{
    _fSelectionVisible = FALSE; 
    if ( _pDoc->GetView()->IsActive() )
    {
        InvalidateSelection( FALSE, FALSE );
    }        
}

//+====================================================================================
//
// Method: ShowSelection
//
// Synopsis: Selection is to be shown. Anticipated use is gaining focus from another window
//
//------------------------------------------------------------------------------------
VOID
CSelectionRenderingServiceProvider::ShowSelection()
{
    _fSelectionVisible = TRUE;

    InvalidateSelection( TRUE, FALSE );            
}

//+====================================================================================
//
// Method: InvalidateSelection
//
// Synopsis: Invalidate the Selection
//
//------------------------------------------------------------------------------------

VOID
CSelectionRenderingServiceProvider::InvalidateSelection( BOOL fSelectionOn, BOOL fFireOM )
{
    PointerSegment* pPointerPair ;
    int i;
    int wherePointer = SAME;
    int iSegmentIndex = 0;

    if ( _fInUpdateSegment )
    {
        _fPendingInvalidate = TRUE;
        _fPendingFireOM = fFireOM;
        return;
    }        
    if ( _parySegment )
    {
        for (i = _parySegment->Size(); i--; )
        {
            if (_parySegment->Item(i))
            {
                pPointerPair = _parySegment->Item(iSegmentIndex);
                wherePointer = OldCompare( pPointerPair->_pStart, pPointerPair->_pEnd);

                switch( wherePointer )
                {
                    case RIGHT:
                        InvalidateSegment( pPointerPair->_pStart, pPointerPair->_pEnd,
                                           pPointerPair->_pStart, pPointerPair->_pEnd, fSelectionOn, fFireOM );
                        break;

                    case LEFT:
                        InvalidateSegment( pPointerPair->_pEnd, pPointerPair->_pStart, 
                                           pPointerPair->_pEnd, pPointerPair->_pStart, fSelectionOn, fFireOM );
                        break;
                }                            
            }
        } 
    }
}          


//+==============================================================================
// 
// Method: GetSelectionChunksForLayout
//
// Synopsis: Get the 'chunks" for a given Flow Layout, as well as the Max and Min Cp's of the chunks
//              
//            A 'chunk' is a part of a SelectionSegment, broken by FlowLayout
//
//-------------------------------------------------------------------------------

VOID
CSelectionRenderingServiceProvider::GetSelectionChunksForLayout( 
    CFlowLayout* pFlowLayout, CPtrAry<HighlightSegment*> *paryHighlight, 
    int* piCpMin, 
    int * piCpMax )
{
    int cpMin = LONG_MAX;  // using min() macro on this will always give a smaller number.
    int cpMax = - 1;
    int i,j , insertAt, segmentStart, segmentEnd ;    
    int flowStart = pFlowLayout->GetContentFirstCp();
    int flowEnd = pFlowLayout->GetContentLastCp();
    int chunkStart = -1;
    int chunkEnd = -1;
    HighlightSegment **ppHighlight;
    enum SELEDGE
    {
        SE_INSIDE, SE_OUTSIDE
    };
    SELEDGE ssSelStart;
    SELEDGE ssSelEnd;   
    
    Assert( paryHighlight->Size() == 0 );
    
    PointerSegment** ppPointerSegment;    
    if ( _parySegment && _fSelectionVisible && ! pFlowLayout->_fTextSelected )
    {
        if ( _pDoc->GetDocContentsVersion() != _lContentsVersion  )
        {
            ConstructSelectionRenderCache();
            _lContentsVersion = _pDoc->GetDocContentsVersion();
        }
        
        for ( i = _parySegment->Size(), ppPointerSegment = * _parySegment;
            i > 0 ;
            i --, ppPointerSegment++)
        {
            if (!*ppPointerSegment)
                continue;
            
            segmentStart = ( *ppPointerSegment )->_cpStart;
            segmentEnd = ( *ppPointerSegment )->_cpEnd  ; 
            if (! (   (segmentEnd <= flowStart ) 
                   || ( flowEnd <= segmentStart ))  )
            {                
                ssSelStart = segmentStart  <= flowStart  ? SE_OUTSIDE : SE_INSIDE;
                ssSelEnd  = segmentEnd >= flowEnd ? SE_OUTSIDE : SE_INSIDE;

                switch (ssSelStart)
                {
                case SE_OUTSIDE:
                {
                    switch (ssSelEnd)
                    {
                    case SE_OUTSIDE:
                        //
                        // Selection completely encloses layout
                        //
                        chunkStart = flowStart;
                        chunkEnd = flowEnd;
                        break;
                    
                    case SE_INSIDE:
                        //
                        // Overlap with selection ending inside
                        //
                        chunkStart = flowStart;
                        chunkEnd = segmentEnd ;                 
                        break;
                    }
                    break;
                }
                case SE_INSIDE:
                {
                    switch( ssSelEnd )
                    {
                    case SE_OUTSIDE:
                        //
                        // Overlap with selection ending outside
                        //
                        chunkStart = segmentStart;
                        chunkEnd = flowEnd;
                        break;
                    
                    case SE_INSIDE:
                        //
                        // Layout completely encloses selection
                        //
                        chunkStart = segmentStart;
                        chunkEnd = segmentEnd  ;                 
                        break;
                    }
                    break;
                }
                }
                if (( chunkStart != -1 ) && ( chunkEnd != -1))
                {
                    //
                    // Insertion Sort on The Array we've been given.
                    //
                    insertAt = -1;
                    for (  j = paryHighlight->Size(), ppHighlight = * paryHighlight ;
                        j > 0;
                        j-- , ppHighlight++ )
                    {
                        if ( (*ppHighlight)->_cpStart > chunkStart )
                        {
                            insertAt = j;
                            break;
                        }                
                    }
                    HighlightSegment* pNewSegment = new HighlightSegment;
                    pNewSegment->_cpStart = chunkStart;
                    pNewSegment->_cpEnd = chunkEnd;
                    pNewSegment->_dwHighlightType = ( *ppPointerSegment )->_HighlightType;
                    if ( insertAt == -1  )
                    {
                           paryHighlight->Append( pNewSegment);
                    }
                    else
                    {
                        paryHighlight->Insert( insertAt, pNewSegment );
                    }
                    
                    cpMin = min ( chunkStart, cpMin );
                    cpMax = max ( chunkEnd, cpMax );
                    
                    chunkStart = -1;
                    chunkEnd = -1;
                }
            }
        }
    }
    if ( piCpMin )
        *piCpMin = cpMin;
    if ( piCpMax )
        *piCpMax = cpMax;
}

#if DBG == 1 

MtDefine(CTxtEdit_aryChunkTest_pv, Utilities , "TestSelectionRenderServices::_aryChunkTest::_pv")

void TestSelectionRenderServices( CMarkup* pMarkup , CElement* pTestElement)
{
    pMarkup->DumpTree();
   //
    // BUGBUG - this is just a test. Note that this WILL LEAK MEMORY.
    //
#ifdef NEVER
    CDoc* pDoc = ped->Doc();
    CMarkupPointer* pStart = new CMarkupPointer( pDoc );
    CMarkupPointer* pEnd = new CMarkupPointer( pDoc );
    int firstSelCp, lastSelCp, firstCp, firstFlow, lastFlow;
    IMarkupPointer* pIStart = NULL ;
    IMarkupPointer* pIEnd = NULL ;
    IHTMLElement* pElement = NULL;
    IHTMLElement* pNewElement = NULL;
    IHTMLElement* pNewElement2 = NULL;
    IMarkupServices* pTreeServices = NULL;
    LONG pch = 0;
    
    ISelectionRenderingServices* pISelRenderSvcs = NULL;
    int iSelectionIndex = -1;

    THR(pTestElement->QueryInterface( IID_IHTMLElement, ( void**) & pElement ));
    THR(pStart->QueryInterface( IID_IMarkupPointer, (void * *) & pIStart ) );
    THR( pIStart->SetGravity( POINTER_GRAVITY_Left ));
    THR(pEnd->QueryInterface( IID_IMarkupPointer, (void * *) & pIEnd ) );
//
// Test Select All
//
    /*THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeEnd ));
    THR( pMarkup->QueryInterface( IID_ISelectionRenderingServices, ( void**) & pISelRenderSvcs ));
    THR( pISelRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));
    */

    //
    // Test Multiple selection.
    //
    //
    // Segment 1 from 0 to 30.

    THR( pMarkup->QueryInterface( IID_ISelectionRenderingServices, ( void**) & pISelRenderSvcs ));

    //
    // Test Case 1 Non-Overlapped Segments.
    //
    THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    pch = 9;
    firstCp = pStart->TreePos()->GetCp(FALSE);
    BSTR testString = SysAllocString(_T("                "));
    THR( pIStart->Right(TRUE, NULL, NULL, &pch, testString ));
   
    firstSelCp = pStart->TreePos()->GetCp(FALSE);
    
    pch = 19;
    THR( pIEnd->Right(TRUE, NULL, NULL, &pch, testString ));
    lastSelCp = pEnd->TreePos()->GetCp( FALSE);
    THR( pISelRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));

    CFlowLayout *pFLayout = pTestElement->GetFlowLayout();
    firstFlow = pFLayout->GetFirstCp();
    lastFlow = pFLayout->GetLastCp();
/*
    THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    pch = 32;
    THR( pIStart->Right(TRUE, NULL, NULL, &pch, NULL ));
    pch = 41;
    THR( pIEnd->Right(TRUE, NULL, NULL, &pch, NULL ));
    THR( pISelRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));
*/
    //
    // TestCaseII Overlapped Segments.
    //
  /*  THR( pIStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    THR( pIEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    pch = 29;
    THR( pIStart->Right(TRUE, NULL, NULL, &pch, NULL ));
    pch = 31;
    THR( pIEnd->Right(TRUE, NULL, NULL, &pch, NULL ));
    THR( pMarkup->QueryInterface( IID_ISelectionRenderingServices, ( void**) & pISelRenderSvcs ));
    THR( pISelRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));*/

    //Segment 2 10 to 20
/*     pch = 10;
    THR( pIStart->Right( TRUE, NULL, NULL, &pch, NULL ));
    THR( pIEnd->Left( TRUE, NULL, NULL, &pch, NULL ));
    THR( pISelRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex ));

    // Segment 3 110 to 120
    pch = 100;
    THR( pIStart->Right( TRUE, NULL, NULL, &pch, NULL ));
    THR( pIEnd->Right( TRUE, NULL, NULL, &pch, NULL ));
    THR( pISelRenderSvcs->AddSegment( pIStart, pIEnd, & iSelectionIndex )); */
/*    THR( pDoc->QueryInterface( IID_IHTMLTreeServices, ( void**) & pTreeServices ));
    THR( pTreeServices->CreateElement( TAGID_INPUT, NULL, & pNewElement ));
    THR( pTreeServices->InsertElement( pNewElement, pIStart, pIStart ));
    THR( pTreeServices->CreateElement( TAGID_INPUT, NULL, &pNewElement2 ));
    THR( pTreeServices->InsertElement( pNewElement2, pIEnd, pIEnd ));*/


    //THR( pISelRenderSvcs->AddElementSegment( pElement, &iSelectionIndex  ));

 /*   pMarkup->ConstructSelectionRenderCache();
    CStackDataAry<int, 10> aryChunkCp(Mt( CTxtEdit_aryChunkTest_pv ) );
    CStackDataAry<int, 10> aryChunkCCh(Mt( CTxtEdit_aryChunkTest_pv ) );
    int cpMax = 0;
    int cpMin = 0;

    pMarkup->GetSelectionChunksForLayout( pMarkup->Root()->GetFlowLayout(), &aryChunkCp, &aryChunkCCh, &cpMin, &cpMax );
    aryChunkCp.DeleteAll();
    aryChunkCCh.DeleteAll();
    CElement* pNewElementClass = NULL;
    THR( pNewElement->QueryInterface( CLSID_CElement,  ( void**) & pNewElementClass ));

    CFlowLayout* pNewLayout = pNewElementClass->GetFlowLayout();
    pMarkup->GetSelectionChunksForLayout( pNewLayout, &aryChunkCp, &aryChunkCCh, &cpMin,  &cpMax );
    pMarkup->InvalidateSelectionRenderCache();

    pMarkup->InvalidateSelectionRenderCache();*/

    ReleaseInterface( pIStart );
    ReleaseInterface( pIEnd );
    ReleaseInterface( pElement );
    ReleaseInterface( pNewElement );
    ReleaseInterface( pNewElement2 );
    ReleaseInterface( pISelRenderSvcs );

    pStart->Release();
    pEnd->Release();
    
#endif // NEVER
}

void
CSelectionRenderingServiceProvider::DumpSegments()
{
    if (_parySegment)
    {
        int i;

        for (i=0;i<_parySegment->Size();i++)
        {
            PointerSegment * p = _parySegment->Item( i );
            char buf[256];

            if (p)
            {

                wsprintfA(buf, "%3d : cp=(%d,%d) sn=(%d,%d) type=%d\r\n",
                          i,
                          p->_cpStart,
                          p->_cpEnd,
                          p->_pStart->SN(),
                          p->_pEnd->SN(),
                          p->_HighlightType );
            }
            else
            {
                wsprintfA(buf, "%3d : deleted\r\n", i);
            }

            OutputDebugStringA( buf );
        }
    }
}

#endif
