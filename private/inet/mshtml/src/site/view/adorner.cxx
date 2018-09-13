//+----------------------------------------------------------------------------
//
// File:        Adorner.HXX
//
// Contents:    Implementation of CAdorner class
//
//  An Adorner provides the addition of  'non-content-based' nodes in the display tree
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef _X_ADORNER_HXX_
#define _X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#include <disptree.h>
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

DeclareTagOther(tagAdornerShow,    "AdornerShow",    "Fill adorners with a hatched brush")

MtDefine(CElementAdorner, Layout, "CElementAdorner")
MtDefine(CFocusAdorner,   Layout, "CFocusAdorner")

const int GRAB_INSET = 4;
const long ADORNER_LAYER = LONG_MAX; // BUGBUG - what should this be ?

//+====================================================================================
//
//  Member:     CAdorner, ~CAdorner
//
//  Synopsis:   Constructor/desctructor for CAdorner
//
//  Arguments:  pView    - Associated CView
//              pElement - Associated CElement
//
//------------------------------------------------------------------------------------

CAdorner::CAdorner( CView * pView, CElement * pElement )
{
    Assert( pView );

    _cRefs     = 1;
    _pView     = pView;
    _pDispNode = NULL;
    _pElement  = pElement && pElement->_etag == ETAG_TXTSLAVE
                    ? pElement->MarkupMaster()
                    : pElement;

    Assert( !_pElement
        ||  _pElement->IsInMarkup() );
}

CAdorner::~CAdorner()
{
    Assert( !_cRefs );
    DestroyDispNode();
}


//+====================================================================================
//
// Method:EnsureDispNode
//
// Synopsis: Creates a DispItemPlus suitable for 'Adornment'
//
//------------------------------------------------------------------------------------

VOID
CAdorner::EnsureDispNode()
{
    Assert( _pElement );
    Assert( _pView );
    Assert( _pView->IsInState(CView::VS_OPEN) );

    CTreeNode* pTreeNode = _pElement->GetFirstBranch();

    if ( !_pDispNode && pTreeNode)
    {
        _pDispNode = (CDispNode *)CDispRoot::CreateDispItemPlus(this,
                                                                TRUE,
                                                                FALSE ,
                                                                FALSE ,
                                                                DISPNODEBORDER_NONE,
                                                                FALSE);

        if ( _pDispNode )
        {
            _pDispNode->SetExtraCookie( GetDispCookie() );
            _pDispNode->SetLayerType( GetDispLayer() );
            _pDispNode->SetOwned(TRUE);
        }
    }

    if (    _pDispNode
        &&  !_pDispNode->GetParentNode() )
    {
        CNotification   nf;

        nf.ElementAddAdorner( _pElement );
        nf.SetData((void *)this);

        _pElement->SendNotification( &nf );
    }
}


//+====================================================================================
//
// Method:GetBounds
//
// Synopsis: Return the bounds of the adorner in global coordinates
//
//------------------------------------------------------------------------------------

void
CAdorner::GetBounds(
    CRect * prc) const
{
    Assert( prc );

    if ( _pDispNode )
    {
        _pDispNode->GetClientRect( prc, CLIENTRECT_CONTENT );
        _pDispNode->TransformRect( prc, COORDSYS_CONTENT, COORDSYS_GLOBAL, FALSE );
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+====================================================================================
//
// Method:  GetRange
//
// Synopsis: Retrieve the cp range associated with an adorner
//
//------------------------------------------------------------------------------------

void
CAdorner::GetRange(
    long *  pcpStart,
    long *  pcpEnd) const
{
    Assert( pcpStart );
    Assert( pcpEnd );

    if ( _pElement && _pElement->GetFirstBranch() )
    {
        long    cch;

        _pElement->GetRange( pcpStart, &cch );

        *pcpEnd = *pcpStart + cch;
    }
    else
    {
        *pcpStart =
        *pcpEnd   = 0;
    }
}


//+====================================================================================
//
// Method:  All CDispClient overrides
//
//------------------------------------------------------------------------------------

void
CAdorner::GetOwner(
    CDispNode * pDispNode,
    void ** ppv)
{
    Assert(pDispNode);
    Assert(ppv);
    *ppv = NULL;
}

void
CAdorner::DrawClient(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         cookie,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

void
CAdorner::DrawClientBackground(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

void
CAdorner::DrawClientBorder(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

void
CAdorner::DrawClientScrollbar(
    int whichScrollbar,
    const RECT* prcBounds,
    const RECT* prcRedraw,
    LONG contentSize,
    LONG containerSize,
    LONG scrollAmount,
    CDispSurface *pSurface,
    CDispNode *pDispNode,
    void *pClientData,
    DWORD dwFlags)
{
    AssertSz(0, "Unexpected/Unimplemented method called in CAdorner");
}

void
CAdorner::DrawClientScrollbarFiller(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

BOOL
CAdorner::HitTestContent(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    return FALSE;
}

BOOL
CAdorner::HitTestFuzzy(
    const POINT *pptHitInParentCoords,
    CDispNode *pDispNode,
    void *pClientData)
{
    return FALSE;
}

BOOL
CAdorner::HitTestScrollbar(
    int whichScrollbar,
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
    return FALSE;
}

BOOL
CAdorner::HitTestScrollbarFiller(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
    return FALSE;
}

BOOL
CAdorner::HitTestBorder(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
    return FALSE;
}

LONG
CAdorner::GetZOrderForSelf()
{
    return 0;
}

LONG
CAdorner::GetZOrderForChild(
    void * cookie)
{
    return 0;
}

LONG
CAdorner::CompareZOrder(
    CDispNode * pDispNode1,
    CDispNode * pDispNode2)
{
    Assert(pDispNode1);
    Assert(pDispNode2);
    Assert(pDispNode1 == _pDispNode);
    Assert(_pElement);

    CElement *  pElement = ::GetDispNodeElement(pDispNode2);

    //
    //  Compare element z-order
    //  If the same element is associated with both display nodes,
    //  then the second display node is also for an adorner
    //

    return _pElement != pElement
                ? _pElement->CompareZOrder(pElement)
                : 0;
}

CDispFilter*
CAdorner::GetFilter()
{
    AssertSz(0, "Unexpected/Unimplemented method called in CAdorner");
    return NULL;
}

void
CAdorner::HandleViewChange(
    DWORD flags,
    const RECT* prcClient,  // global coordinates
    const RECT* prcClip,    // global coordinates
    CDispNode* pDispNode)
{
    AssertSz(0, "Unexpected/Unimplemented method called in CAdorner");
}

BOOL
CAdorner::ProcessDisplayTreeTraversal(
                        void *pClientData)
{
    return TRUE;
}

void
CAdorner::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

DWORD
CAdorner::GetClientLayersInfo(CDispNode *pDispNodeFor)
{
    return 0;
}

void
CAdorner::DrawClientLayers(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    AssertSz(0, "CAdorner- unexpected and unimplemented method called");
}

#if DBG==1
void
CAdorner::DumpDebugInfo(
    HANDLE hFile,
    long level,
    long childNumber,
    CDispNode *pDispNode,
    void* cookie)
{
    WriteHelp(
            hFile,
            _T("<<br>\r\n<<font class=tag>&lt;<0s><1s>&gt;<</font><<br>\r\n"),
            _T("Adorner on "),
            GetElement()
                ? GetElement()->TagName()
                : _T("Range"));
}
#endif

//+====================================================================================
//
//  Member:     DestroyDispNode
//
//  Synopsis:   Destroy the adorner display node (if any)
//
//------------------------------------------------------------------------------------

void
CAdorner::DestroyDispNode()
{
    if ( _pDispNode )
    {
        Assert( _pView );
        Assert( _pView->IsInState(CView::VS_OPEN) );    
        _pDispNode->Destroy();
        _pDispNode = NULL;
    }
}


#if DBG==1
//+====================================================================================
//
//  Member:     ShowRectangle
//
//  Synopsis:   Fill the adorner rectangle with a hatched brush
//
//  Arguments:  pDI - Pointer to CFormDrawInfo to use
//
//------------------------------------------------------------------------------------

void
CAdorner::ShowRectangle(
    CFormDrawInfo * pDI)
{
    static COLORREF s_aclr[] =  {
                                RGB( 255,   0,   0 ),
                                RGB(   0, 255,   0 ),
                                RGB( 255, 255,   0 ),
                                RGB(   0, 255, 255 ),
                                };

    if ( IsTagEnabled( tagAdornerShow ) )
    {
        HDC     hdc    = pDI->GetDC();
        int     bkMode = ::SetBkMode( hdc, TRANSPARENT );
        HBRUSH  hbrPat = ::CreateHatchBrush( HS_DIAGCROSS, s_aclr[ (((ULONG)(ULONG_PTR)this) >> 8) & 0x00000003 ] );
        HBRUSH  hbrOld;

        hbrOld = (HBRUSH) SelectObject( hdc, hbrPat );
        ::PatBlt( hdc, pDI->_rc.left, pDI->_rc.top, pDI->_rc.Width(), pDI->_rc.Height(), PATCOPY );
        ::DeleteObject( ::SelectObject( hdc, hbrOld ) );
        ::SetBkMode( hdc, bkMode );
    }
}
#endif


//+====================================================================================
//
//  Member:     CElementAdorner, ~CElementAdorner
//
//  Synopsis:   Constructor/desctructor for CElementAdorner
//
//  Arguments:  pView    - Associated CView
//              pElement - Associated CElement
//
//------------------------------------------------------------------------------------

CElementAdorner::CElementAdorner( CView* pView, CElement* pElement )
    : CAdorner( pView, pElement )
{
    Assert( pView );
    Assert( pElement );
}

CElementAdorner::~CElementAdorner()
{
    SetSite( NULL );
}

//+====================================================================================
//
// Method: PositionChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CElementAdorner::PositionChanged( const CSize * psize )
{
    Assert( _pView->IsInState(CView::VS_OPEN) );

    if ( _pIElementAdorner )
    {
        if ( !psize )
        {
            HRESULT hr = S_OK;
            CPoint ptElemPos;
            POINT ptAdornerPos ;

            CLayout* pLayout = _pElement->GetUpdatedNearestLayout();
            Assert( pLayout );

            pLayout->GetPosition( &ptElemPos, COORDSYS_GLOBAL );

            hr = THR( _pIElementAdorner->GetPosition( (POINT*) & ptElemPos,  & ptAdornerPos ) );

            if ( ! hr )
            {
                ptElemPos.x = ptAdornerPos.x;
                ptElemPos.y = ptAdornerPos.y;

                EnsureDispNode();

                _pDispNode->TransformPoint( &ptElemPos, COORDSYS_GLOBAL, COORDSYS_PARENT );
                _pDispNode->SetPosition( ptElemPos );
            }
        }
        else
        {
            if ( ! _pDispNode )
            {
                EnsureDispNode();
                Assert( _pDispNode );
            }
            
            CPoint  pt = _pDispNode->GetPosition();

            pt += *psize;

            _pDispNode->SetPosition( pt );
        }

        IGNORE_HR( _pIElementAdorner->OnPositionSet());        
    }
}

//+====================================================================================
//
// Method: ShapeChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CElementAdorner::ShapeChanged()
{
    Assert( _pView->IsInState(CView::VS_OPEN) );

    if ( _pIElementAdorner )
    {
        HRESULT hr = S_OK;

        CLayout* pLayout =  _pElement->GetUpdatedNearestLayout();
        Assert( pLayout );
        CSize elemSize;
        SIZE szAdorner ;

        pLayout->GetSize( &elemSize );

        hr = THR( _pIElementAdorner->GetSize( (SIZE*) &elemSize, & szAdorner ));
        if ( ! hr )
        {
            elemSize.cx = szAdorner.cx;
            elemSize.cy = szAdorner.cy;

            EnsureDispNode();

            _pDispNode->SetSize( elemSize , TRUE );
        }
    }
}


//+====================================================================================
//
// Method: UpdateAdorner
//
// Synopsis: Brute-force way of updating an adorners position
//
//------------------------------------------------------------------------------------


void 
CElementAdorner::UpdateAdorner()
{
    CNotification   nf;
    long cpStart, cpEnd ;
    
    GetRange( & cpStart, & cpEnd );        
    nf.MeasuredRange(cpStart, cpEnd - cpStart);
    _pView->Notify(&nf); 
}

//+====================================================================================
//
// Method: ScrollIntoView
//
// Synopsis: Scroll the Adorner into view
//
//------------------------------------------------------------------------------------


BOOL 
CElementAdorner::ScrollIntoView()
{
    if ( _pDispNode )
        return( _pDispNode->ScrollIntoView( CRect::SP_MINIMAL, CRect::SP_MINIMAL ));
    else
        return FALSE;
}


//+====================================================================================
//
// Method: Draw
//
// Synopsis: Wraps call to IElementAdorner.Draw - to allow adorner
//           to draw.
//
//------------------------------------------------------------------------------------

void
CElementAdorner::DrawClient(
            const RECT *   prcBounds,
            const RECT *   prcRedraw,
            CDispSurface * pDispSurface,
            CDispNode *    pDispNode,
            void *         cookie,
            void *         pClientData,
            DWORD          dwFlags)
{
    if ( _pIElementAdorner )
    {
        Assert(pClientData);

        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds( pDI, prcBounds, prcRedraw, pDispSurface );

        WHEN_DBG( ShowRectangle( pDI ) );

        IGNORE_HR( _pIElementAdorner->Draw( pDI->GetDC() , (RECT*) & pDI->_rc ));
    }
}


#ifdef VSTUDIO7
HTC
#else
static HTC
#endif //VSTUDIO7
AdornerHTIToHTC ( ADORNER_HTI eAdornerHTC )
{
    HTC eHTC = HTC_NO;

    switch( eAdornerHTC )
    {

    case ADORNER_HTI_TOPBORDER :
        eHTC = HTC_TOPBORDER;
        break;

    case ADORNER_HTI_LEFTBORDER:
        eHTC = HTC_LEFTBORDER;
        break;

    case ADORNER_HTI_BOTTOMBORDER:
        eHTC = HTC_LEFTBORDER ;
        break;

    case ADORNER_HTI_RIGHTBORDER :
        eHTC = HTC_RIGHTBORDER;
        break;
        
    case ADORNER_HTI_TOPLEFTHANDLE:
        eHTC = HTC_TOPLEFTHANDLE;
        break;
            
    case ADORNER_HTI_LEFTHANDLE:
        eHTC = HTC_LEFTHANDLE       ;
        break;
    case ADORNER_HTI_TOPHANDLE:
        eHTC = HTC_TOPHANDLE ;
        break;
        
    case ADORNER_HTI_BOTTOMLEFTHANDLE:
        eHTC = HTC_BOTTOMLEFTHANDLE ;
        break;

    case ADORNER_HTI_TOPRIGHTHANDLE:
        eHTC = HTC_TOPRIGHTHANDLE   ;
        break;
        
    case ADORNER_HTI_BOTTOMHANDLE:
        eHTC = HTC_BOTTOMHANDLE ;
        break;
        
    case ADORNER_HTI_RIGHTHANDLE:
        eHTC = HTC_RIGHTHANDLE ;
        break;
        
    case ADORNER_HTI_BOTTOMRIGHTHANDLE:
        eHTC = HTC_BOTTOMRIGHTHANDLE ;
        break;
    
    }

    return eHTC;
}

//+====================================================================================
//
// Method: HitTestContent
//
// Synopsis: IDispClient - hit test point. Wraps call to IElementAdorner.HitTestPoint
//
//------------------------------------------------------------------------------------

BOOL
CElementAdorner::HitTestContent(
            const POINT *  pptHit,
            CDispNode *    pDispNode,
            void *         pClientData)
{
    BOOL fDidWeHitAdorner = FALSE;
    ADORNER_HTI eAdornerHTI = ADORNER_HTI_NONE;
    
    if (    _pIElementAdorner
        &&  _pElement->IsInMarkup() )
    {
        CRect myRect;
        HRESULT hr = S_OK;

        _pDispNode->GetClientRect( &myRect, CLIENTRECT_CONTENT );

        hr = THR( _pIElementAdorner->HitTestPoint(
                                                    const_cast<POINT*> (pptHit),
                                                    (RECT*) ( const_cast<CRect*> (&myRect)),
                                                    & fDidWeHitAdorner,
                                                    & eAdornerHTI ));
        if (  ( ! hr ) && ( fDidWeHitAdorner ))
        {
            //
            // If we hit the adorner, we fix the node of hit test info to
            // be that of the element we adorn.
            //
            CHitTestInfo *  phti = (CHitTestInfo *)pClientData;
            phti->_htc = AdornerHTIToHTC( eAdornerHTI );
            phti->_pNodeElement = _pElement->GetFirstBranch();
        }
    }

    return fDidWeHitAdorner;
}


//+====================================================================================
//
// Method: GetZOrderForSelf
//
// Synopsis: IDispClient - get z-order.
//
//------------------------------------------------------------------------------------

LONG
CElementAdorner::GetZOrderForSelf()
{
    return LONG_MAX;
}

//+====================================================================================
//
//  Member:     CFocusAdorner, ~CFocusAdorner
//
//  Synopsis:   Constructor/desctructor for CFocusAdorner
//
//  Arguments:  pView    - Associated CView
//              pElement - Associated CElement
//
//------------------------------------------------------------------------------------

CFocusAdorner::CFocusAdorner( CView* pView )
    : CAdorner( pView )
{
    Assert( pView );
}

CFocusAdorner::~CFocusAdorner()
{
    Assert( _pView );

    delete _pShape;

    if ( _pView->_pFocusAdorner == this )
    {
        _pView->_pFocusAdorner = NULL;
    }
}

//+====================================================================================
//
// Method:  EnsureDispNode
//
// Synopsis: Ensure the display node is created and properly inserted in the display tree
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::EnsureDispNode()
{
    Assert( _pElement );
    Assert( _pView->IsInState(CView::VS_OPEN) );

    if ( _pShape )
    {
        CTreeNode * pTreeNode = _pElement->GetFirstBranch();

        if ( !_pDispNode && pTreeNode)
        {
            _dnl = _pElement->GetFirstBranch()->IsPositionStatic()
                        ? DISPNODELAYER_FLOW
                        : _pElement->GetFirstBranch()->GetCascadedzIndex() >= 0
                                ? DISPNODELAYER_POSITIVEZ
                                : DISPNODELAYER_NEGATIVEZ;
            _adl = _dnl == DISPNODELAYER_FLOW
                        ? ADL_TOPOFFLOW
                        : ADL_ONELEMENT;
        }

        if (    !_pDispNode
            ||  !_pDispNode->GetParentNode() )
        {
            super::EnsureDispNode();

            if (_pDispNode)
            {
                _pDispNode->SetAffectsScrollBounds(FALSE);
            }
        }
    }

    else
    {
        DestroyDispNode();
    }
}


//+====================================================================================
//
// Method:  EnsureFocus
//
// Synopsis: Ensure focus display node exists and is properly inserted in the display tree
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::EnsureFocus()
{
    Assert( _pElement );
    if (    _pShape
        &&  (   !_pDispNode
            ||  !_pDispNode->GetParentNode() ) )
    {
        EnsureDispNode();
    }
}


//+====================================================================================
//
// Method:  SetElement
//
// Synopsis: Set the associated element
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::SetElement( CElement * pElement, long iDivision )
{
    Assert( _pView->IsInState(CView::VS_OPEN) );
    Assert( pElement );
    Assert( pElement->IsInMarkup() );

    if (pElement->_etag == ETAG_TXTSLAVE)
    {
        pElement = pElement->MarkupMaster();
    }

    // If this element is a checkbox or a radio button, use the associated
    // label element if one exists for drawing the focus shape.
    else if (   pElement->Tag() == ETAG_INPUT
             && DYNCAST(CInput, pElement)->IsOptionButton())
    {
        CLabelElement * pLabel = pElement->GetLabel();
        if (pLabel)
        {
            pElement = pLabel;
        }
    }
    else if ( pElement->Tag() == ETAG_INPUT
            && DYNCAST(CInput, pElement)->GetType() == htmlInputFile)
    {
        // force shape change
        _pElement = NULL;
    }

    if (    pElement  != _pElement
        ||  iDivision != _iDivision)
    {
        _pElement  = pElement;
        _iDivision = iDivision;

        DestroyDispNode();
        ShapeChanged();
    }

    Assert( _pElement );
}

//+====================================================================================
//
// Method: PositionChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::PositionChanged( const CSize * psize )
{
    Assert( _pElement );
    Assert( _pElement->GetFirstBranch() );
    Assert( _pView->IsInState(CView::VS_OPEN) );

    if ( _pDispNode )
    {
        CLayout *   pLayout     = _pElement->GetUpdatedNearestLayout();
        CTreeNode * pTreeNode   = _pElement->GetFirstBranch();
        BOOL        fRelative   = pTreeNode->GetCharFormat()->_fRelative;
        CDispNode * pDispParent = _pDispNode->GetParentNode();
        CDispNode * pDispNode   = NULL;

        Assert  ( _pShape );

        //
        //  Get the display node which contains the element with focus
        //  (If the focus display node is not yet anchored in the display tree, pretend the element
        //   is not correct as well. After the focus display node is anchored, this routine will
        //   get called again and can correctly associate the display nodes at that time.)
        //

        if ( pDispParent )
        {
// BUGBUG: Move this logic down into GetElementDispNode (passing a flag so that GetElementDispNode
//         can distinguish between "find nearest" and "find exact" with this call being a "find nearest"
//         and virtually all others being a "find exact" (brendand)
            CElement *  pDisplayElement = NULL;

            if (    !pTreeNode->IsPositionStatic()
                ||  _pElement->HasLayout() )
            {
                pDisplayElement = _pElement;
            }
            else if ( !fRelative )
            {
                pDisplayElement = pLayout->ElementOwner();
            }
            else
            {
                CTreeNode * pDisplayNode = pTreeNode->GetCurrentRelativeNode( pLayout->ElementOwner() );

                Assert( pDisplayNode );     // This should never be NULL, but be safe anyway
                if ( pDisplayNode )
                {
                    pDisplayElement = pDisplayNode->Element();
                }
            }

            Assert( pDisplayElement );      // This should never be NULL, but be safe anyway
            if ( pDisplayElement )
            {
                pDispNode = pLayout->GetElementDispNode( pDisplayElement );
            }
        }


        //
        //  Verify that the display node which contains the element with focus and the focus display node
        //  are both correctly anchored in the display tree (that is, have a common parent)
        //  (If they do not, this routine will get called again once both get correctly anchored
        //   after layout is completed)
        //

        if ( pDispNode )
        {
            CDispNode * pDispNodeTemp;

            for ( pDispNodeTemp = pDispNode;
                    pDispNodeTemp
                &&  pDispNodeTemp != pDispParent;
                pDispNodeTemp = pDispNodeTemp->GetParentNode() );

            if ( !pDispNodeTemp )
            {
                pDispNode = NULL;
            }

            Assert( !pDispNode
                ||  pDispNodeTemp == pDispParent );
        }

        if ( pDispNode )
        {
            if (    !psize 
                ||  _dnl != DISPNODELAYER_FLOW )
            {
                CPoint  ptFromOffset( g_Zero.pt );
                CPoint  ptToOffset( g_Zero.pt );

                if ( !_fTopLeftValid )
                {
                    CRect   rc;
                    _pShape->GetBoundingRect( &rc );
                    _pShape->OffsetShape( -rc.TopLeft().AsSize() );

                    _ptTopLeft = rc.TopLeft();

                    if (    !_pElement->HasLayout()
                        &&  fRelative )
                    {
                        CPoint  ptOffset;

                        _pElement->GetUpdatedParentLayout()->GetFlowPosition( pDispNode, &ptOffset );

                        _ptTopLeft -= ptOffset.AsSize();
                    }

                    _fTopLeftValid = TRUE;
                }

                pDispNode->TransformPoint( &ptFromOffset, COORDSYS_CONTENT, COORDSYS_GLOBAL );
                _pDispNode->TransformPoint( &ptToOffset, COORDSYS_PARENT, COORDSYS_GLOBAL );

                _pDispNode->SetPosition( _ptTopLeft.AsSize() + (ptFromOffset - ptToOffset).AsPoint() );
            }

            else
            {
                CPoint  pt = _pDispNode->GetPosition();

                Assert( _fTopLeftValid );

                pt += *psize;

                _pDispNode->SetPosition( pt );
            }
        }

        //
        //  If the display node containing the element with focus is not correctly placed in the display
        //  tree, remove the focus display node as well to prevent artifacts
        //

        else
        {
            _pDispNode->ExtractFromTree();
        }
    }
}

//+====================================================================================
//
// Method: ShapeChanged
//
// Synopsis: Hit the Layout for the size you should be and ask your adorner for it to give
//           you your position based on this
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::ShapeChanged()
{
    Assert( _pView->IsInState(CView::VS_OPEN) );
    Assert( _pElement );

    delete _pShape;
    _pShape = NULL;

    _fTopLeftValid = FALSE;

    CDoc *      pDoc = _pView->Doc();
    CDocInfo    dci(pDoc->_dci);
    CShape *    pShape;

    if (    pDoc->_fTrustedDoc
        &&  (   (_pElement->_etag == ETAG_SPAN && DYNCAST(CSpanElement, _pElement)->GetAAnofocusrect())
             || (_pElement->_etag == ETAG_DIV && DYNCAST(CDivElement, _pElement)->GetAAnofocusrect())))
    {
        pShape = NULL;
    }
    else
    {
        _pElement->GetFocusShape( _iDivision, &dci, &pShape );
    }

    if ( pShape )
    {
        CRect       rc;

        pShape->GetBoundingRect( &rc );

        if ( rc.IsEmpty() )
        {
            delete pShape;
        }
        else
        {
            _pShape = pShape;
        }
    }
    
    EnsureDispNode();

    if ( _pDispNode )
    {
        CRect   rc;

        Assert( _pShape );

        _pShape->GetBoundingRect( &rc );
        _pDispNode->SetSize( rc.Size(), TRUE );
    }
}

//+====================================================================================
//
// Method: Draw
//
// Synopsis: Wraps call to IElementAdorner.Draw - to allow adorner
//           to draw.
//
//------------------------------------------------------------------------------------

void
CFocusAdorner::DrawClient(
            const RECT *   prcBounds,
            const RECT *   prcRedraw,
            CDispSurface * pDispSurface,
            CDispNode *    pDispNode,
            void *         cookie,
            void *         pClientData,
            DWORD          dwFlags)
{
    Assert( _pElement );
    if (    !_pElement->IsEditable( TRUE )
        &&  _pView->Doc()->HasFocus()
        &&  !(_pView->Doc()->_wUIState & UISF_HIDEFOCUS))
    {
        Assert(pClientData);

        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds( pDI, prcBounds, prcRedraw, pDispSurface );

        WHEN_DBG( ShowRectangle( pDI ) );

        _pShape->DrawShape( pDI );
    }
}

//+====================================================================================
//
// Method: GetZOrderForSelf
//
// Synopsis: IDispClient - get z-order.
//
//------------------------------------------------------------------------------------

LONG
CFocusAdorner::GetZOrderForSelf()
{
    Assert( _pElement );
    Assert( !_pElement->GetFirstBranch()->IsPositionStatic() );
    Assert( _dnl != DISPNODELAYER_FLOW );
    return _pElement->GetFirstBranch()->GetCascadedzIndex();
}
