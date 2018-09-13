#include "headers.hxx"


#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif


extern int CFETC;                   // global in dataxfrobj

extern FORMATETC g_rgFETC[];

MtDefine(CDropTargetInfo, Utilities, "CDropTargetInfo ")
MtDefine(CSelDragDropSrcInfo, Utilities, "CSelDragDropSrcInfo")

//
// Constants for comparison of results of IMarkupPointer::Compare method.
//
const int LEFT = 1;   
const int RIGHT = -1 ;
const int SAME = 0;

CDropTargetInfo::CDropTargetInfo(CLayout * pLayout, CDoc* pDoc , POINT pt)
{
    _pDoc = pDoc;
    Init(  );

    UpdateDragFeedback( pLayout, pt, NULL, TRUE );
}

CDropTargetInfo::~CDropTargetInfo()
{
    ReleaseInterface( _pCaret );
    ReleaseInterface( _pPointer );
    ReleaseInterface( _pTestPointer );
}

void 
CDropTargetInfo::Init()
{
    IMarkupServices * pMarkup  = NULL;
#if !(SPVER >= 0x1)
    IGNORE_HR( _pDoc->QueryInterface( IID_IMarkupServices, (void**) & pMarkup ));
    IGNORE_HR( pMarkup->CreateMarkupPointer( & _pPointer ));
    IGNORE_HR( pMarkup->CreateMarkupPointer( & _pTestPointer ));
    IGNORE_HR( _pDoc->GetCaret( & _pCaret ));
    ReleaseInterface( pMarkup );
#else
    HRESULT hr = S_OK;
    
    hr = THR( _pDoc->QueryInterface( IID_IMarkupServices, (void**) & pMarkup ));
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->CreateMarkupPointer( & _pPointer ));
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->CreateMarkupPointer( & _pTestPointer ));
    if (hr)
        goto Cleanup;

    hr = THR( _pDoc->GetCaret( & _pCaret ));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pMarkup)
        ReleaseInterface( pMarkup );
#endif
    _fAtLogicalBOL = FALSE;
    _fFurtherInStory = FALSE;
    _fPointerPositioned = FALSE;
}
    
void 
CDropTargetInfo::UpdateDragFeedback(CLayout*                pLayout, 
                                    POINT                   pt,
                                    CSelDragDropSrcInfo *   pInfo,
                                    BOOL                    fPositionLastToTest )
{
    HRESULT hr = S_OK;
    BOOL fInside = FALSE;
    BOOL fSamePointer = FALSE;
    int iWherePointer = SAME;
    CPoint ptContent(pt);
    CMarkupPointer* pPointerInternal = NULL;

    pLayout->TransformPoint( &ptContent, COORDSYS_GLOBAL, COORDSYS_CONTENT, NULL );
    hr = THR( _pTestPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointerInternal));
    if( hr )
        goto Cleanup;
        
    hr = THR( _pDoc->MovePointerToPointInternal( ptContent,
                                                 pLayout->GetFirstBranch(),
                                                 pPointerInternal,
                                                 & _fNotAtBOL,
                                                 & _fAtLogicalBOL,
                                                 NULL,
                                                 FALSE,
                                                 pLayout,
                                                 FALSE ) ); // don't hit test EOL 

    // BUGBUG (MohanB): Should release pPOinterInternal here? Not done in IsPointInSelection.
    if ( hr )
        goto Cleanup;

    if ( _fPointerPositioned )
    {
        hr = THR( OldCompare( _pPointer, _pTestPointer, & iWherePointer ));
        if ( hr )
            goto Cleanup;
        _fFurtherInStory = ( iWherePointer == RIGHT );            
    }
    
    if ( fPositionLastToTest )
    {
        hr = THR( _pPointer->MoveToPointer( _pTestPointer ));        
        if ( hr )
           goto Cleanup;
        _fPointerPositioned = TRUE;           
    }
    
    if ( pInfo )
        fInside = pInfo->IsInsideSelection(_pTestPointer );
        
    if ( fInside )
    {
        _pDoc->_fSlowClick = TRUE;
        DrawDragFeedback();
    }
    else
    {
        // If feedback is currently displayed and the feedback location
        // is changing, then erase the current feedback.

        if ( ! fPositionLastToTest )
        {
            hr = THR ( _pPointer->IsEqualTo( _pTestPointer, & fSamePointer ));
            if ( hr )
                goto Cleanup;
        }
        else 
            fSamePointer = TRUE;

        if ( ! fSamePointer  )
        {
            DrawDragFeedback();            
            hr = THR ( _pPointer->MoveToPointer( _pTestPointer ));
            if ( hr )
                goto Cleanup;
            _fPointerPositioned = TRUE;                
        }

        // Draw new feedback.
        if (! _pDoc->_fDragFeedbackVis)
        {
            DrawDragFeedback();
        }

        _pDoc->_fSlowClick = FALSE;
    }
Cleanup:
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     DrawDragFeedback
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CDropTargetInfo::DrawDragFeedback()
{

    if ( _pDoc->_fDragFeedbackVis)
    {
        _pCaret->Hide();
        _pDoc->_fDragFeedbackVis = 0;
    }
    else
    {
        _pCaret->MoveCaretToPointerEx( _pPointer, _fNotAtBOL, TRUE, FALSE, CARET_DIRECTION_INDETERMINATE );
        _pDoc->_fDragFeedbackVis = 1;
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     PasteDataObject
//
//  Synopsis: Paste a Data Object at a MarkupPointer
//
//----------------------------------------------------------------------------


HRESULT
CDropTargetInfo::PasteDataObject( 
                    IDataObject * pdo,
                    IMarkupPointer* pInsertionPoint,
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd )
{
    HRESULT hr = S_OK;
    
    IHTMLEditor* pHTMLEditor = _pDoc->GetHTMLEditor();
    IHTMLEditingServices * pEditingServices = NULL;

    Assert( pHTMLEditor );
    if ( ! pHTMLEditor )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = THR( pHTMLEditor->QueryInterface( IID_IHTMLEditingServices, ( void** ) & pEditingServices ));
    if ( hr )
        goto Cleanup;

    //
    // We'll use Markup Services "magic" to move the pointers around where we insert
    //

    hr = THR( pStart->MoveToPointer( pInsertionPoint ));
    if ( hr )
        goto Cleanup;
    hr = THR( pStart->SetGravity ( POINTER_GRAVITY_Left ));
    if ( hr )   
        goto Cleanup;
    hr = THR( pEnd->MoveToPointer( pInsertionPoint ));
    if ( hr )
        goto Cleanup;
    hr = THR( pEnd->SetGravity ( POINTER_GRAVITY_Right ));
    if ( hr )   
        goto Cleanup;    

    hr = THR( pEditingServices->PasteFromClipboard( pInsertionPoint, NULL, pdo ));
    if ( hr )
       goto Cleanup;

Cleanup:

    ReleaseInterface( pEditingServices );
    RRETURN ( hr );    
}
            
//+====================================================================================
//
// Method: EquivalentElements
//
// Synopsis: Test elements for 'equivalency' - ie if they are the same element type,
//           and have the same class, id , and style.
//
//------------------------------------------------------------------------------------

BOOL 
CDropTargetInfo::EquivalentElements( IHTMLElement* pIElement1, IHTMLElement* pIElement2 )
{
    CElement* pElement1 = NULL;
    CElement* pElement2 = NULL;
    BOOL fEquivalent = FALSE;
    HRESULT hr = S_OK;
    IHTMLStyle * pIStyle1 = NULL;
    IHTMLStyle * pIStyle2 = NULL;
    BSTR id1 = NULL;
    BSTR id2 = NULL;
    BSTR class1 = NULL;
    BSTR class2 = NULL;
    BSTR style1 = NULL;
    BSTR style2 = NULL;
    
    hr = THR( pIElement1->QueryInterface( CLSID_CElement, (void**) & pElement1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->QueryInterface( CLSID_CElement, (void**) & pElement2 ));
    if ( hr )
        goto Cleanup;

    //
    // Compare Tag Id's
    //
    if ( pElement1->_etag != pElement2->_etag )
        goto Cleanup;

    //
    // Compare Id's
    //
    hr = THR( pIElement1->get_id( & id1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->get_id( & id2 ));
    if ( hr )   
        goto Cleanup;  
    if ((( id1 != NULL ) || ( id2 != NULL )) && 
        ( StrCmpIW( id1, id2) != 0))
        goto Cleanup;

    //
    // Compare Class
    //
    hr = THR( pIElement1->get_className( & class1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->get_className( & class2 ));
    if ( hr )   
        goto Cleanup;   
        
    if ((( class1 != NULL ) || ( class2 != NULL )) &&
        ( StrCmpIW( class1, class2) != 0 ) )
        goto Cleanup;

    //
    // Compare Style's
    //        
    hr = THR( pIElement1->get_style( & pIStyle1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIElement2->get_style( & pIStyle2 ));
    if ( hr )   
        goto Cleanup;                
    hr = THR( pIStyle1->toString( & style1 ));
    if ( hr )
        goto Cleanup;
    hr = THR( pIStyle2->toString( & style2 ));
    if ( hr )
        goto Cleanup;        
    if ((( style1 != NULL ) || ( style2 != NULL )) &&
        ( StrCmpIW( style1, style2) != 0 ))
        goto Cleanup;

    fEquivalent = TRUE;        
Cleanup:
    SysFreeString( id1 );
    SysFreeString( id2 );
    SysFreeString( class1 );
    SysFreeString( class2 );
    SysFreeString( style1 );
    SysFreeString( style2 );
    ReleaseInterface( pIStyle1 );
    ReleaseInterface( pIStyle2 );
    
    AssertSz(!FAILED(hr), "Unexpected failure in Equivalent Elements");

    return ( fEquivalent );
}


//+---------------------------------------------------------------------------
//
//  Member:     Drop
//
//  Synopsis: Do everything you need to do as a result of a drop operation
//
//----------------------------------------------------------------------------

HRESULT 
CDropTargetInfo::Drop (  
                    CLayout* pLayout, 
                    IDataObject *   pDataObj,
                    DWORD           grfKeyState,
                    POINT          ptScreen,
                    DWORD *         pdwEffect)
{

    HRESULT hr = S_OK;

    ISegmentList* pSegmentList = NULL;

    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    SELECTION_TYPE eType = SELECTION_TYPE_None;
    IHTMLElement* pIDragElement = NULL;
    IHTMLElement* pICurElement = NULL;
    IHTMLEditingServices * pEdServices = NULL;
    IMarkupPointer* pBoundaryStart = NULL;
    IMarkupPointer* pBoundaryEnd = NULL;
    CElement* pDropElement = NULL;
    CSelDragDropSrcInfo* pSelDragSrc = NULL;
    HRESULT hrDrop = S_OK;
    
    pSelDragSrc = _pDoc->GetSelectionDragDropSource();
    if ( pSelDragSrc )
    {
        hrDrop = THR( pSelDragSrc->IsValidDrop( _pTestPointer ));
    }
    
    if ( hrDrop == S_OK && 
        (  *pdwEffect == DROPEFFECT_COPY 
        || *pdwEffect == DROPEFFECT_MOVE 
        || *pdwEffect == DROPEFFECT_NONE ))
    {

        //
        // BUGBUG todo - move this into a separate routine on Formkrnl.
        //
        hr = THR( _pDoc->GetEditingServices(& pEdServices ));
        if ( hr )
            goto Cleanup;

        hr = THR( _pDoc->CreateMarkupPointer( & pBoundaryStart));
        if ( hr )
            goto Cleanup;
            
        hr = THR( _pDoc->CreateMarkupPointer( & pBoundaryEnd));
        if ( hr )
            goto Cleanup;

        pDropElement = pLayout->ElementOwner();
        Assert( pDropElement);
        if ( ! pDropElement )
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        pDropElement = pDropElement->HasSlaveMarkupPtr() ? pDropElement->GetSlaveMarkupPtr()->FirstElement() : pDropElement;

        //
        // We call GetEditContext  JUST to get 2 pointers for AdjustPointerForInsert Boundaries
        //
        hr = THR( _pDoc->GetEditContext( 
                                            pDropElement , 
                                            NULL , 
                                            pBoundaryStart,
                                            pBoundaryEnd ,
                                            TRUE ) ); /* We are drilling in */
        if ( hr )
            goto Cleanup;
            
        hr = THR( pEdServices->AdjustPointerForInsert( _pPointer, _fAtLogicalBOL , !_fAtLogicalBOL, pBoundaryStart, pBoundaryEnd ) );
        if ( hr )
            goto Cleanup;

        pSelDragSrc = _pDoc->GetSelectionDragDropSource();
        if ( pSelDragSrc )
        {
            //
            // Test if the point after we do the adjust is ok.
            //
            hrDrop = THR( pSelDragSrc->IsValidDrop( _pPointer ));
        }           

        if ( hrDrop == S_OK )
        {
            hr = THR( _pDoc->EnsureEditContext( pLayout->ElementContent(), TRUE ));
            if (hr)
                goto Cleanup;

            if ( *pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE )
            {
                hr = THR( _pDoc->CreateMarkupPointer( & pStart ));
                if ( hr )
                    goto Cleanup;

                hr = THR( _pDoc->CreateMarkupPointer( & pEnd ));
                if ( hr )
                    goto Cleanup;           
                            
                hr = THR( PasteDataObject( pDataObj, _pPointer, pStart, pEnd )) ;
                if ( hr )
                {
                    AssertSz(0, "Paste Failed !");
                    goto Cleanup;            
                }
                
                hr = THR( pEdServices->FindSiteSelectableElement( pStart, pEnd, & pIDragElement));
                if ( hr == S_OK )                
                {
                    eType = SELECTION_TYPE_Control ;
                    
                    hr = THR( pStart->MoveAdjacentToElement( pIDragElement, ELEM_ADJ_BeforeBegin ));
                    if ( hr )
                        goto Cleanup;
                    hr = THR( pEnd->MoveAdjacentToElement( pIDragElement, ELEM_ADJ_AfterEnd ));
                    if ( hr )
                        goto Cleanup;
                }
                else
                    eType = SELECTION_TYPE_Selection;

               
                hr = THR( _pDoc->Select( pStart, pEnd, eType ));
                if ( hr )
                    goto Cleanup;

                if ( eType != SELECTION_TYPE_Control )
                {
                    hr = THR( _pDoc->ScrollPointersIntoView( pStart, pEnd ));
                    if ( hr )
                        goto Cleanup; 
                }
                //
                // else
                //   do nothing - as the Adorner scrolls itself on creation.
            }
            else
            {
                *pdwEffect = DROPEFFECT_NONE;
                //
                // For None we put the caret here 
                //
                hr = THR( _pDoc->Select(_pPointer, _pPointer, SELECTION_TYPE_Caret ));
                _pDoc->_fDragFeedbackVis = FALSE;

            }
        }
        else
        {
            if ( hrDrop == S_FALSE &&  pSelDragSrc &&       
                (  *pdwEffect == DROPEFFECT_COPY 
                || *pdwEffect == DROPEFFECT_MOVE ))
            {
                hr = S_FALSE;
            }
        
            //
            // BUGBUG - what should we do ?
            //
            *pdwEffect = DROPEFFECT_NONE ;
        }
    }
    else
    {    
        if ( hrDrop == S_FALSE &&  pSelDragSrc &&       
            (  *pdwEffect == DROPEFFECT_COPY 
            || *pdwEffect == DROPEFFECT_MOVE ))
        {
            hr = S_FALSE;
        }
    
        //
        // BUGBUG - what should we do ?
        //
        *pdwEffect = DROPEFFECT_NONE ;
    }


Cleanup:
    ReleaseInterface( pBoundaryStart );
    ReleaseInterface( pBoundaryEnd );
    ReleaseInterface( pICurElement );
    ReleaseInterface( pIDragElement );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pEdServices );

    RRETURN1 ( hr, S_FALSE );
}


CSelDragDropSrcInfo::CSelDragDropSrcInfo( CDoc* pDoc, CElement* pElement ) : CDragDropSrcInfo()
{
    _ulRefs = 1;
    _srcType = DRAGDROPSRCTYPE_SELECTION;
    _pDoc = pDoc;
    Assert( ! _pBag );
    Init( pElement);
}

CSelDragDropSrcInfo::~CSelDragDropSrcInfo()
{
    ReleaseInterface( _pStart );
    ReleaseInterface( _pEnd );
    delete _pBag;   // To break circularity
}


//+====================================================================================
//
// Method:GetSegmentList
//
// Synopsis: Hide the getting of the segment list on the doc - so we can easily be moved
//
//------------------------------------------------------------------------------------

HRESULT
CSelDragDropSrcInfo::GetSegmentList( ISegmentList** ppSegmentList )
{
    return ( _pDoc->GetCurrentSelectionSegmentList( ppSegmentList ) ) ; 
}

HRESULT
CSelDragDropSrcInfo::GetMarkup( IMarkupServices** ppMarkup)
{
    return ( _pDoc->PrimaryMarkup()->QueryInterface( IID_IMarkupServices, (void**) ppMarkup )) ; 
}

//+====================================================================================
//
// Method: Init
//
// Synopsis: Inits the SelDragDrop Info. Basically positions two TreePointers around 
// the selection, creates a range, and invokes the saver on that to create a Bag.
// 
//
//------------------------------------------------------------------------------------

VOID 
CSelDragDropSrcInfo::Init(CElement* pElement)
{
    HRESULT hr = S_OK;
    ISegmentList * pSegmentList = NULL;
    IMarkupServices * pMarkup = NULL;
    CBaseBag *              pBag = NULL;;
    int ctSegment = 0;
    CMarkup * pMarkupInternal;
    BOOL    fSupportsHtml;
    
    Assert( pElement );
    //
    // Position the Pointers
    //
    hr = GetSegmentList( & pSegmentList );
    if ( hr )
        goto Cleanup;

    hr = THR( pSegmentList->GetSegmentCount( & ctSegment, & _eSelType ));           
    if ( hr )
        goto Cleanup; 

    Assert( ctSegment == 1 ); // What does it mean to drag-drop multiple selections ? I don't know.
    Assert(    ( _eSelType == SELECTION_TYPE_Selection ) 
            || ( _eSelType == SELECTION_TYPE_Control ) );


    hr = THR( GetMarkup( & pMarkup ));
    if ( hr )
        goto Cleanup;

    hr = THR( pMarkup->CreateMarkupPointer( & _pStart ));
    if ( hr )
        goto Cleanup;

    hr = THR( pMarkup->CreateMarkupPointer( & _pEnd ));
    if ( hr )
        goto Cleanup;
   
#if DBG == 1
        _pDoc->SetDebugName(_pStart, _T("[ DragDrop Source Start Drag]"));
        _pDoc->SetDebugName( _pEnd, _T("[ DragDrop Source End Drag]"));            
#endif        
    hr = THR ( pSegmentList->MovePointersToSegment( 0, _pStart, _pEnd ) );

    pMarkupInternal = pElement->GetMarkup();

    fSupportsHtml = pElement->GetFirstBranch()->SupportsHtml();

    hr = THR(CTextXBag::Create( pMarkupInternal, 
                                fSupportsHtml, 
                                pSegmentList, 
                                TRUE, 
                                (CTextXBag **)&pBag, 
                                this ));
    if (hr)
        goto Cleanup;

    _pBag = pBag;

    hr = THR( SetInSameFlow());
    
Cleanup:
    ReleaseInterface( pMarkup );
    ReleaseInterface( pSegmentList );
}


//+====================================================================================
//
// Method:SetInSameFlow
//
// Synopsis: Set the _fInSameFlow Boolean for the Pointers
//
//------------------------------------------------------------------------------------

HRESULT
CSelDragDropSrcInfo::SetInSameFlow()
{
    HRESULT hr = S_OK;
    
    CElement* pElement1 = NULL;
    CElement* pElement2 = NULL;
    CFlowLayout* pFlow1 = NULL;
    CFlowLayout* pFlow2 = NULL;
    IHTMLElement* pIHTMLElement1 = NULL;
    IHTMLElement* pIHTMLElement2 = NULL;
    
    hr = THR( _pDoc->CurrentScopeOrSlave( _pStart, & pIHTMLElement1));
    if ( hr )
        goto Cleanup;

    if ( ! pIHTMLElement1 )
    {
        AssertSz(0, "Didn't get an element");
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement1->QueryInterface( CLSID_CElement, (void**) & pElement1 ));
    if ( hr )
        goto Cleanup;

    hr = THR( _pDoc->CurrentScopeOrSlave(_pEnd, & pIHTMLElement2));
    if ( hr )
        goto Cleanup;

    if ( ! pIHTMLElement2 )
    {
        AssertSz(0, "Didn't get an element");
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement2->QueryInterface( CLSID_CElement, (void**) & pElement2 ));
    if ( hr )
        goto Cleanup;

    pFlow1 = _pDoc->GetFlowLayoutForSelection( pElement1->GetFirstBranch());

    pFlow2 = _pDoc->GetFlowLayoutForSelection( pElement2->GetFirstBranch());

    _fInSameFlow =  ( pFlow1 == pFlow2 );
    
Cleanup:
    ReleaseInterface( pIHTMLElement1 );
    ReleaseInterface( pIHTMLElement2 );

    RRETURN ( hr );
    
}

BOOL
CSelDragDropSrcInfo::IsInsideSelection(IMarkupPointer* pTestPointer)
{
    BOOL                fInsideSelection    = FALSE;
    CMarkupPointer *    pTest               = NULL;
    CMarkupPointer *    pStart              = NULL;
    CMarkupPointer *    pEnd                = NULL;
    CMarkup *           pMarkupSel;
    CMarkup *           pMarkupTest;
    CElement *          pElemMaster;
    BOOL                fRefOnTest          = FALSE;


    // Handle the case where pTestPointer is not in the same markup as
    // _pStart and _pEnd.
    Verify(
        S_OK == pTestPointer->QueryInterface(CLSID_CMarkupPointer, (void**)&pTest)
     && S_OK == _pStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pStart)
     && S_OK == _pEnd->QueryInterface(CLSID_CMarkupPointer, (void**)&pEnd));

    pMarkupSel = pStart->Markup();
    Assert(pMarkupSel && pMarkupSel == pEnd->Markup());

    pMarkupTest = pTest->Markup();
    Assert(pMarkupTest);

    if (pMarkupTest != pMarkupSel)
    {
        // Walk up the chain of master markups, if necessary
        for (;;)
        {
            pElemMaster = pMarkupTest->Master();
            if (!pElemMaster)
                goto Cleanup;
            pMarkupTest = pElemMaster->GetMarkup();
            if (!pMarkupTest)
                goto Cleanup;
            if (pMarkupTest == pMarkupSel)
            {
                if (S_OK != _pDoc->CreateMarkupPointer(&pTest))
                    goto Cleanup;
                fRefOnTest = TRUE;
                if (S_OK != pTest->MoveAdjacentToElement(pElemMaster, ELEM_ADJ_BeforeBegin))
                    goto Cleanup;
                break;
            }
        }
    }

    //
    // Why are the rules for AreWeInside A selection different ? 
    // We want to not compare on equality for character selection
    // otherwise we can't drag characters adjacent to each other ( bug 11490 ).
    // BUT we want to not allow controls to be dragged onto each other - for example dragging a Text box
    // onto iteself with nothing else in the tree ( bug 54388 )
    //
    if ( _eSelType == SELECTION_TYPE_Control )
    {
        if ( pTest->IsRightOfOrEqualTo(pStart ) )
        {        
            if ( pTest->IsLeftOfOrEqualTo( pEnd ) )
            {
                fInsideSelection = TRUE;
            }
        }
    }
    else
    {

        if ( pTest->IsRightOf(pStart ) )
        {        
            if ( pTest->IsLeftOf( pEnd ) )
            {
                fInsideSelection = TRUE;
            }
        }
    }

Cleanup:
    if (fRefOnTest)
        ReleaseInterface(pTest);
    return ( fInsideSelection );
}

//+====================================================================================
//
// Method: IsValidPaste
//
// Synopsis: Ask if the drop is valid. If the pointer is within the selection it isn't valid
//
//------------------------------------------------------------------------------------


HRESULT
CSelDragDropSrcInfo::IsValidDrop(IMarkupPointer* pTestPointer)
{
    BOOL                fInsideSelection    = FALSE;
    CMarkupPointer *    pTest               = NULL;
    CMarkupPointer *    pStart              = NULL;
    CMarkupPointer *    pEnd                = NULL;
    HRESULT hr;
    
    hr = THR( pTestPointer->QueryInterface(CLSID_CMarkupPointer, (void**)&pTest));
    if ( hr )
        goto Cleanup;
        
    hr = THR( _pStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pStart));
    if ( hr )
        goto Cleanup;
        
    hr = THR(_pEnd->QueryInterface(CLSID_CMarkupPointer, (void**)&pEnd));
    if ( hr )
        goto Cleanup;

    if ( pTest->Markup() != pStart->Markup() ||
         pTest->Markup() != pEnd->Markup() )
    {
        goto Cleanup;
    }
    
    if ( pTest->IsRightOfOrEqualTo(pStart ) )
    {        
        if ( pTest->IsLeftOfOrEqualTo( pEnd ) )
        {
            fInsideSelection = TRUE;
        }
    }


Cleanup:
    if ( fInsideSelection )
        hr = S_FALSE;
    else if ( ! hr )
        hr = S_OK;
        
    RRETURN1( hr, S_FALSE );        
}

    
HRESULT 
CSelDragDropSrcInfo::GetDataObjectAndDropSource(    IDataObject **  ppDO,
                                                    IDropSource **  ppDS )
{
    HRESULT hr = S_OK;

    hr = THR( QueryInterface(IID_IDataObject, (void **)ppDO));
    if (hr)
        goto Cleanup;

    hr = THR( QueryInterface(IID_IDropSource, (void **)ppDS));
    if (hr)
    {
        (*ppDO)->Release();
        goto Cleanup;
    }
    
Cleanup:
    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     PostDragDelete
//
//  Synopsis: The drag is over and has successfully finished ( on a move)
//            We want to now delete the contents of the selection
//
//----------------------------------------------------------------------------

 
HRESULT 
CSelDragDropSrcInfo::PostDragDelete()
{
    IHTMLEditingServices * pEdServices = NULL;
    IHTMLEditor * pEditor = NULL ;
    
    HRESULT hr = S_OK;

    if ( _fInSameFlow )
    {
        pEditor = _pDoc->GetHTMLEditor();

        if ( pEditor )
        {
            hr = THR( pEditor->QueryInterface( IID_IHTMLEditingServices, ( void**) & pEdServices ));
            if ( hr )
                goto Cleanup;
            
            hr = THR( pEdServices->Delete( _pStart, _pEnd, TRUE ));
        }
        else
            hr = E_FAIL;
        
    }
    
Cleanup:

    ReleaseInterface( pEdServices );
    
    RRETURN ( hr );
}

HRESULT 
CSelDragDropSrcInfo::PostDragSelect()
{
    HRESULT hr = S_OK;

    
    hr = THR( _pDoc->Select( _pStart, _pEnd, _eSelType ));
    
    RRETURN ( hr );
}

    // ISegmentList methods
HRESULT
CSelDragDropSrcInfo::MovePointersToSegment (
        int iSegmentIndex, 
        IMarkupPointer * pStart, 
        IMarkupPointer * pEnd ) 
{
    HRESULT hr = S_OK;

    Assert( iSegmentIndex == 0 );
    hr = THR( pStart->MoveToPointer( _pStart ));
    if ( hr )
        goto Cleanup;

    hr = THR( pEnd->MoveToPointer( _pEnd ));
    if ( hr)    
        goto Cleanup;
        
Cleanup:        
    RRETURN ( hr );    
}

HRESULT
CSelDragDropSrcInfo::GetSegmentCount (
        int* piSegmentCount,
        SELECTION_TYPE * peType )
{
    HRESULT hr = S_OK;
    if ( piSegmentCount )
        *piSegmentCount = 1;
    if ( peType )
        *peType = _eSelType;
        
    RRETURN( hr );
}


STDMETHODIMP
CSelDragDropSrcInfo::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, ISegmentList )
        QI_INHERITS( this , ISegmentList )
        QI_INHERITS( this, IDataObject )
        QI_INHERITS( this, IDropSource )
    default:
        if (_pBag)
            return _pBag->QueryInterface(iid, ppv);
        break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

//
// Methods that we delegate to our _pBag
//

HRESULT CSelDragDropSrcInfo::DAdvise( FORMATETC FAR* pFormatetc,
        DWORD advf,
        LPADVISESINK pAdvSink,
        DWORD FAR* pdwConnection) 
{
    if ( _pBag )
    {
        RRETURN ( _pBag->DAdvise( pFormatetc, advf, pAdvSink, pdwConnection ) ); 
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::DUnadvise( DWORD dwConnection)
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->DUnadvise( dwConnection ) );
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise)
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->EnumDAdvise( ppenumAdvise ));
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::EnumFormatEtc(
            DWORD dwDirection,
            LPENUMFORMATETC FAR* ppenumFormatEtc)
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->EnumFormatEtc( dwDirection, ppenumFormatEtc ) );
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::GetCanonicalFormatEtc(
            LPFORMATETC pformatetc,
            LPFORMATETC pformatetcOut)
{ 
    if ( _pBag )
    {
        RRETURN( _pBag->GetCanonicalFormatEtc( pformatetc, pformatetcOut) );
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{ 
    if ( _pBag )
    {
        RRETURN ( _pBag->GetData( pformatetcIn, pmedium ));
    }
    else
        return E_FAIL;
}

HRESULT 
CSelDragDropSrcInfo::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{ 
    if ( _pBag )
    {
        RRETURN( _pBag->GetDataHere( pformatetc, pmedium ));
    }
    else
        return E_FAIL;
}


HRESULT 
CSelDragDropSrcInfo::QueryGetData(LPFORMATETC pformatetc )
{ 
    if ( _pBag )
    {
        RRETURN( _pBag->QueryGetData( pformatetc ));
    }
    else
        return E_FAIL;
}

HRESULT
CSelDragDropSrcInfo::SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease)
{
    if ( _pBag )
    {
        RRETURN( _pBag->SetData( pformatetc, pmedium , fRelease));  
    }
    else
        return E_FAIL;
}


HRESULT
CSelDragDropSrcInfo::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if ( _pBag )
    {
        return( _pBag->QueryContinueDrag( fEscapePressed, grfKeyState ));
    }
    else
        return E_FAIL;
}

HRESULT 
CSelDragDropSrcInfo::GiveFeedback (DWORD dwEffect)
{
    if ( _pBag )
    {
        return( _pBag->GiveFeedback (dwEffect));
    }
    else
        return E_FAIL;
}




CDragDropTargetInfo::CDragDropTargetInfo(CDoc* pDoc )
{
    _pDoc = pDoc;
    if ( _pDoc->_fDesignMode )
    {
        StoreSelection();
        //
        // Tell the Selection to tear itself down for the duration of the drag    
        //
        _pDoc->NotifySelection( SELECT_NOTIFY_DESTROY_SELECTION, NULL );    
    }
}

CDragDropTargetInfo::~CDragDropTargetInfo()
{
    ReleaseInterface( _pStart );
    ReleaseInterface( _pEnd );
}


HRESULT 
CDragDropTargetInfo::StoreSelection()
{
    HRESULT hr = S_OK;

    ISegmentList * pSegmentList = NULL;
    IMarkupServices * pMarkup = NULL;

    int ctSegment = 0;

    hr =  _pDoc->GetCurrentSelectionSegmentList( & pSegmentList ) ;
    if ( hr )
        goto Cleanup;


    hr = THR( pSegmentList->GetSegmentCount( & ctSegment, & _eType ));           
    if ( hr )
        goto Cleanup; 

    if ( ctSegment > 0 ) 
    {
        hr = THR( _pDoc->QueryInterface( IID_IMarkupServices, (void**) & pMarkup ) );
        if ( hr )
            goto Cleanup;

        hr = THR( pMarkup->CreateMarkupPointer( & _pStart ));
        if ( hr )
            goto Cleanup;

        hr = THR( pMarkup->CreateMarkupPointer( & _pEnd ));
        if ( hr )
            goto Cleanup;
       
#if DBG == 1
        _pDoc->SetDebugName(_pStart, _T("[ DragDrop Target Start Saved Selection ]"));
        _pDoc->SetDebugName( _pEnd, _T("[ DragDrop Target End Saved Selection ]"));            
#endif

        hr = THR ( pSegmentList->MovePointersToSegment( 0, _pStart, _pEnd ));

        _pElemCurrentAtStoreSel = _pDoc->_pElemCurrent;
        
    }

Cleanup:
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pMarkup );
    RRETURN( hr );
}

HRESULT 
CDragDropTargetInfo::RestoreSelection()
{
    HRESULT hr = S_OK;

    //
    // Ensure that the current element we began a drag & drop with is the same
    // as the currency now. This makes us not whack selection back - if our host
    // has changed currency on us in some way ( as access does over ole controls)
    //
    if ( _pDoc->_pElemCurrent == _pElemCurrentAtStoreSel &&
        _pStart && 
        _pEnd && 
        _eType != SELECTION_TYPE_None )
    {
        hr = THR( _pDoc->Select( _pStart, _pEnd, _eType ) );
    }

    _pElemCurrentAtStoreSel = NULL;
    RRETURN( hr );
}
