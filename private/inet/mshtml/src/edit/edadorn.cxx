#include "headers.hxx"

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

DeclareTag(tagAdornerHitTest, "Adorner", "Adorner Hit Test")
DeclareTag(tagAdornerShowResize, "Adorner", "Display Adorner Resize Info ")
DeclareTag(tagAdornerShowAdjustment, "Adorner", "Display Adorner Adjust Info ")
DeclareTag(tagAdornerResizeRed, "Adorner", "Draw Resize Feedback in Red")
using namespace EdUtil;

extern HINSTANCE           g_hInstance ;

template < class T > void swap ( T & a, T & b ) { T t = a; a = b; b = t; }

MtDefine( CGrabHandleAdorner, Utilities , "CGrabHandleAdorner" )
MtDefine( CActiveControlAdorner, Utilities , "CGrabHandleAdorner" )
MtDefine( CCursor, Utilities , "CCursor" )
//
// Constants
//

const int GRAB_SIZE = 7 ;
const int FEEDBACK_SIZE = 1;
    
static const ADORNER_HTI seHitHandles[] =
{
    ADORNER_HTI_TOPLEFTHANDLE ,
    ADORNER_HTI_TOPHANDLE ,
    ADORNER_HTI_TOPRIGHTHANDLE,    
    ADORNER_HTI_LEFTHANDLE,  
    ADORNER_HTI_RIGHTHANDLE,  
    ADORNER_HTI_BOTTOMLEFTHANDLE, 
    ADORNER_HTI_BOTTOMHANDLE,      
    ADORNER_HTI_BOTTOMRIGHTHANDLE 
};


static const USHORT sHandleAdjust[] =
{
    CT_ADJ_TOP | CT_ADJ_LEFT,       //  GRAB_TOPLEFTHANDLE
    CT_ADJ_TOP,                     //  GRAB_TOPHANDLE
    CT_ADJ_TOP | CT_ADJ_RIGHT,      //  GRAB_TOPRIGHTHANDLE    
    CT_ADJ_LEFT,                    //  GRAB_LEFTHANDLE
    CT_ADJ_RIGHT,                   //  GRAB_RIGHTHANDLE    
    CT_ADJ_BOTTOM | CT_ADJ_LEFT,    //  GRAB_BOTTOMLEFTHANDLE
    CT_ADJ_BOTTOM,                  //  GRAB_BOTTOMHANDLE
    CT_ADJ_BOTTOM | CT_ADJ_RIGHT   //  GRAB_BOTTOMRIGHTHANDLE
};

static const LPCTSTR sHandleCursor[] =
{
    IDC_SIZENWSE,
    IDC_SIZENS,
    IDC_SIZENESW,
    IDC_SIZEWE,
    IDC_SIZEWE,
    IDC_SIZENESW,
    IDC_SIZENS,
    IDC_SIZENWSE,
    IDC_ARROW,
    IDC_SIZEALL,
    IDC_CROSS
};

CEditAdorner::CEditAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc )
{
    _pIDoc = pIDoc;
    IGNORE_HR( pIElement->QueryInterface( IID_IHTMLElement, (void**) & _pIElement ));
    
    _adornerCookie = 0 ;
    _cRef = 0;
    _ctOnPositionSet = 0;
    Assert(! _fPositionSet);
    Assert(! _fScrollIntoViewOnPositionSet );
}

VOID 
CEditAdorner::SetManager( CSelectionManager * pManager )
{
    _pManager = pManager;
}

VOID
CEditAdorner::NotifyManager()
{
    if ( _fNotifyManagerOnPositionSet && _pManager )
    {
        _pManager->AdornerPositionSet(); // BUGBUG - do we want our own notification mechanism ?
        _fNotifyManagerOnPositionSet = FALSE; // only notify once
    }
}

CEditAdorner::~CEditAdorner()
{
    ReleaseInterface( _pIElement );
}

VOID
CGrabHandleAdorner::ShowCursor(ADORNER_HTI inAdorner )
{
    if ( inAdorner != _currentCursor )
    {
        if ( _pCursor )
        {
            delete _pCursor;
            _pCursor = NULL;
        }    
        if ( inAdorner != ADORNER_HTI_NONE )
            _pCursor = new CCursor( GetResizeHandleCursorId(inAdorner));
        

        _currentCursor = inAdorner;
    }
    else
    {   
        if ( _pCursor )
            _pCursor->Show();
    }
}


BOOL
CEditAdorner::IsAdornedElementPositioned()
{
    return ( IsElementPositioned( _pIElement ) );
}

// --------------------------------------------------
// IUnknown Interface
// --------------------------------------------------

STDMETHODIMP_(ULONG)
CEditAdorner::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CEditAdorner::Release( void )
{
    --_cRef;

    if( 0 == _cRef )
    {
        delete this;
        return 0;
    }

    return _cRef;
}


STDMETHODIMP
CEditAdorner::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    *ppv = ( IUnknown* ) this;
    
    switch(iid.Data1)
    {
        QI_INHERITS( this , IUnknown )
        QI_INHERITS( this , IElementAdorner )
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

//+====================================================================================
//
// Method: AttachToElement
//
// Synopsis: Add this Adorner inside of Trident.
//
//------------------------------------------------------------------------------------


HRESULT
CEditAdorner::CreateAdorner()
{
    HRESULT hr = S_OK;

    IElementAdorner * pElementAdorner = NULL;
    IHTMLViewServices* pVS = NULL;

    hr = THR( _pIDoc->QueryInterface( IID_IHTMLViewServices, (void**) & pVS ));
    if ( hr )  
        goto Cleanup;
    
    hr = THR( this->QueryInterface( IID_IElementAdorner, (void**) & pElementAdorner ));
    if ( hr )
        goto Cleanup;
        
    hr = THR( pVS->AddElementAdorner( _pIElement, pElementAdorner, &_adornerCookie ));
    if ( hr ) 
        goto Cleanup;

    SetBoundsInternal();
    
Cleanup:
    ReleaseInterface( pVS );
    ReleaseInterface( pElementAdorner );    
    RRETURN( hr );
}

//+====================================================================================
//
// Method:DestroyAdorner
//
// Synopsis: Destroy's the Adorner inside of trident that we are attached to.
//
//------------------------------------------------------------------------------------

HRESULT
CEditAdorner::DestroyAdorner()
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pVS = NULL;
    
    if ( _adornerCookie )
    {
        hr = THR( _pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pVS ));
        if ( hr )
            goto Cleanup;

        hr = THR( pVS->RemoveElementAdorner( _adornerCookie ));
        if ( hr ) 
            goto Cleanup;

        _adornerCookie = 0;
        
    }
    
Cleanup:
    ReleaseInterface( pVS);
    
    RRETURN ( hr );

}


//+====================================================================================
//
// Method:UpdateAdorner
//
// Synopsis: Call Update Adorner on the Adorner inside of Trident.
//
//------------------------------------------------------------------------------------

HRESULT
CEditAdorner::UpdateAdorner()
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pVS = NULL;
    
    if ( _adornerCookie )
    {
        hr = THR( _pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pVS ));
        if ( hr )
            goto Cleanup;

        hr = THR( pVS->UpdateAdorner(_adornerCookie ));        
        if ( hr ) 
            goto Cleanup;
        
    }
    
Cleanup:
    ReleaseInterface( pVS);
    
    RRETURN ( hr );

}

//+====================================================================================
//
// Method:ScrollIntoView
//
// Synopsis: Call ScrollIntoView on the Adorner inside of Trident.
//
//------------------------------------------------------------------------------------

HRESULT
CEditAdorner::ScrollIntoViewInternal()
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pVS = NULL;
    
    if ( _adornerCookie )
    {
        hr = THR( _pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pVS ));
        if ( hr )
            goto Cleanup;

        hr = THR( pVS->ScrollIntoView(_adornerCookie ));        

        
    }
    
Cleanup:
    ReleaseInterface( pVS);
    
    RRETURN1 ( hr, S_FALSE );

}

HRESULT
CEditAdorner::ScrollIntoView()
{
    if ( ! _fPositionSet )
    {
        _fScrollIntoViewOnPositionSet = TRUE;
        RRETURN( S_OK );
    }
    else
    {
        RRETURN ( ScrollIntoViewInternal() );
    }
    
}

//+====================================================================================
//
// Method:InvalidateAdorner
//
// Synopsis: Call Invalidate Adorner on the Adorner inside of Trident.
//
//------------------------------------------------------------------------------------

HRESULT
CEditAdorner::InvalidateAdorner()
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pVS = NULL;
    
    if ( _adornerCookie )
    {
        hr = THR( _pIDoc->QueryInterface( IID_IHTMLViewServices, ( void**) & pVS ));
        if ( hr )
            goto Cleanup;

        hr = THR( pVS->InvalidateAdorner(_adornerCookie ));        
        if ( hr ) 
            goto Cleanup;
        
    }
    
Cleanup:
    ReleaseInterface( pVS);
    
    RRETURN ( hr );

}
//+====================================================================================
//
// Method: SetBoundsInternal
//
// Synopsis: Maintain an internal rect of where we are for HitTesting
//
//------------------------------------------------------------------------------------


VOID
CEditAdorner::SetBoundsInternal()
{
    HRESULT hr = S_OK;
    IHTMLViewServices * pViewServices = NULL;

    hr = THR( _pIDoc->QueryInterface( IID_IHTMLViewServices, (void**) & pViewServices ));
    if ( hr )
        goto Cleanup;

    hr = THR( pViewServices->GetElementAdornerBounds( _adornerCookie , & _rcBounds ));
    
Cleanup:
    ReleaseInterface( pViewServices);
}


CBorderAdorner::CBorderAdorner( IHTMLElement* pIElement, IHTMLDocument2 * pIDoc )
    : CEditAdorner( pIElement, pIDoc )
{

}

CBorderAdorner::~CBorderAdorner()
{

}

HRESULT
CBorderAdorner::GetSize ( 
            SIZE* pElementSize, 
            SIZE* pAdornerSize )
{
    int myInset = GetInset();

    if ( pElementSize )
    {
        pAdornerSize->cx = pElementSize->cx + ( myInset * 2 ) ;
        pAdornerSize->cy = pElementSize->cy + ( myInset * 2 ) ;

        return ( S_OK );
    }
    else
        return( E_FAIL );
}
    
HRESULT
CBorderAdorner::GetPosition( 
            POINT* pElementPosition, 
            POINT* pAdornerPosition )
{
    int myInset = GetInset();
    
    if ( pElementPosition )
    {
        pAdornerPosition->x = pElementPosition->x - myInset ;
        pAdornerPosition->y = pElementPosition->y - myInset  ;
        
        return ( S_OK );
    }
    else
        return( E_FAIL );
}

//+====================================================================================
//
// Method: OnPositionSet
//
// Synopsis: Callback when the Adorner has updated its position. Used to call "NotifyManager"
//          which in turn processes any pending actions to be taken - when the Adorner's position
//          has been set. ( Currently this is used for UI-Active resizing ).
//
//------------------------------------------------------------------------------------


HRESULT
CBorderAdorner::OnPositionSet() 
{
    HRESULT hr = S_OK;

    SetBoundsInternal();

    if ( ( _rcBounds.left != 0 || 
           _rcBounds.top != 0 || 
           _rcBounds.bottom != 0 || 
           _rcBounds.right != 0 ) 
         && _ctOnPositionSet > 0 ) // BUGBUG - this is a MAJOR HACK ! Scrolling only works on the second OnPositionSet
    {
        NotifyManager();

        if ( _fScrollIntoViewOnPositionSet )
        {
            hr = ScrollIntoViewInternal() ;            
            _fScrollIntoViewOnPositionSet = FALSE;          
        }
    }
    
    _ctOnPositionSet++;
    _fPositionSet = TRUE;
    
    return S_OK;
}

HRESULT
CGrabHandleAdorner::HitTestPoint ( 
        POINT *phitPoint,
        RECT *pAdornerBounds,
        BOOL *pfHitTestResult,
        ADORNER_HTI* peAdornerHTI)
{

    RECT rc = *pAdornerBounds;
    Assert( peAdornerHTI );
    ADORNER_HTI eAdornerHTI = ADORNER_HTI_NONE;
    
    ::InflateRect( &rc, -GRAB_SIZE , -GRAB_SIZE);

    if ( ( PtInRect( pAdornerBounds, *phitPoint )) && 
         ( ! PtInRect( & rc, *phitPoint ) ) )
    {
        SetLocked();
        // Set the HTI
        if ( !_fLocked )
        {            
            if ( !IsInResizeHandle(*phitPoint, & eAdornerHTI , pAdornerBounds) )
                eAdornerHTI = ADORNER_HTI_TOPBORDER ;
        }        
        *pfHitTestResult = TRUE;
    }
    else
    {
        *pfHitTestResult = FALSE;
    }

    if ( peAdornerHTI )
        *peAdornerHTI = eAdornerHTI;
        
    return ( S_OK );
}


CGrabHandleAdorner::CGrabHandleAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc, BOOL fLocked )
    : CBorderAdorner( pIElement , pIDoc )
{
    _resizeAdorner = -1;
    _hbrFeedback = NULL;
    _hbrHatch = NULL;
    _pCursor = NULL;
    _currentCursor = ADORNER_HTI_NONE;
    _fIsPositioned = IsAdornedElementPositioned();
    _fLocked = fLocked;

    
}

CGrabHandleAdorner::~CGrabHandleAdorner()
{
    if ( _hbrFeedback )
        ::DeleteObject( _hbrFeedback );
    if ( _hbrHatch )
        ::DeleteObject( _hbrHatch );
    if ( _pCursor )
        delete _pCursor;        
}

int 
CGrabHandleAdorner::GetInset()
{
    return GRAB_SIZE ;
}

HRESULT 
CGrabHandleAdorner::Draw( 
        HDC hdc,
        RECT *pAdornerBounds)
{
    HRESULT hr = S_OK;

    SetLocked(); // VID Expects locked'ness to be refeshed on invalidate.
    
    DrawGrabBorders( hdc, pAdornerBounds, FALSE );
    DrawGrabHandles( hdc, pAdornerBounds );       

    RRETURN ( hr );
   
}

HRESULT
CGrabHandleAdorner::SetLocked()
{
    HRESULT hr = S_OK;
    BOOL fLocked = FALSE;
    
    hr = THR( _pManager->GetViewServices()->IsElementLocked( _pIElement, & fLocked ));

    _fLocked = fLocked;
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetGrabRect
//
//  Synopsis:   Compute grab rect for a given area.
//
//  Notes:      These diagrams show the output grab rect for handles and
//              borders.
//
//              -----   -----   -----               -------------
//              |   |   |   |   |   |               |           |
//              | TL|   | T |   |TR |               |     T     |
//              ----|-----------|----           ----|-----------|----
//                  |           |               |   |           |   |
//              ----| Input     |----           |   | Input     |   |
//              |   |           |   |           |   |           |   |
//              |  L|   RECT    |R  |           |  L|   RECT    |R  |
//              ----|           |----           |   |           |   |
//                  |           |               |   |           |   |
//              ----|-----------|----           ----|-----------|----
//              | BL|   | B |   |BR |               |     B     |
//              |   |   |   |   |   |               |           |
//              -----   -----   -----               -------------
//
//----------------------------------------------------------------------------

void
CGrabHandleAdorner::GetGrabRect(ADORNER_HTI htc, RECT * prcOut, RECT * prcIn)
{
    
    switch (htc)
    {
    case ADORNER_HTI_TOPLEFTHANDLE:
    case ADORNER_HTI_LEFTHANDLE:
    case ADORNER_HTI_BOTTOMLEFTHANDLE:

        prcOut->left = prcIn->left ;
        prcOut->right = prcIn->left + GRAB_SIZE ;
        break;

    case ADORNER_HTI_TOPHANDLE:
    case ADORNER_HTI_BOTTOMHANDLE:

        prcOut->left = ((prcIn->left + prcIn->right) - GRAB_SIZE) / 2;
        prcOut->right = prcOut->left + GRAB_SIZE;
        break;

    case ADORNER_HTI_TOPRIGHTHANDLE:
    case ADORNER_HTI_RIGHTHANDLE:
    case ADORNER_HTI_BOTTOMRIGHTHANDLE:

        prcOut->left = prcIn->right - GRAB_SIZE ;
        prcOut->right = prcIn->right ;
        break;


    default:
        Assert(FALSE && "Unsupported GRAB_ value in GetGrabRect");
        return;
    }

    switch (htc)
    {
    case ADORNER_HTI_TOPLEFTHANDLE:
    case ADORNER_HTI_TOPHANDLE:
    case ADORNER_HTI_TOPRIGHTHANDLE:

        prcOut->top = prcIn->top ;
        prcOut->bottom = prcIn->top + GRAB_SIZE  ;
        break;

    case ADORNER_HTI_LEFTHANDLE:
    case ADORNER_HTI_RIGHTHANDLE:

        prcOut->top = ((prcIn->top + prcIn->bottom) - GRAB_SIZE) / 2;
        prcOut->bottom = prcOut->top + GRAB_SIZE;
        break;

    case ADORNER_HTI_BOTTOMLEFTHANDLE:
    case ADORNER_HTI_BOTTOMHANDLE:
    case ADORNER_HTI_BOTTOMRIGHTHANDLE:

        prcOut->top = prcIn->bottom - GRAB_SIZE;
        prcOut->bottom = prcIn->bottom;
        break;

    default:
        Assert(FALSE && "Unsupported ADORNER_HTI_ value in GetHandleRegion");
        return;
    }

    if (prcOut->left > prcOut->right)
    {
        swap(prcOut->left, prcOut->right);
    }
    if (prcOut->top > prcOut->bottom)
    {
        swap(prcOut->top, prcOut->bottom);
    }
}

//+------------------------------------------------------------------------
//
//  Function:   PatBltRectH & PatBltRectV
//
//  Synopsis:   PatBlts the top/bottom and left/right.
//
//-------------------------------------------------------------------------
void
CGrabHandleAdorner::PatBltRectH(HDC hDC, RECT * prc, RECT* pExcludeRect, int cThick, DWORD dwRop)
{
    int trueLeft = 0 ;
    int trueWidth = 0 ;
    
    //
    // Don't do general clipping. Assume that Oneside of pExcludeRect is equivalent
    // This is true once you're resizing with a grab handle.
    //
    if ( pExcludeRect && 
         pExcludeRect->top == prc->top && 
         pExcludeRect->left == prc->left )
    {
        if ( pExcludeRect->right < prc->right )
        {
            trueLeft = pExcludeRect->right;
            trueWidth = abs( prc->right - trueLeft );
        }
        else
            trueWidth = 0; // Nothing to draw - we are completely contained
    }
    else if ( pExcludeRect &&
              pExcludeRect->top == prc->top  &&
              pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->left > prc->left )
        {    
            trueLeft = prc->left;
            trueWidth = abs( pExcludeRect->right - trueLeft );
        }
        else
            trueWidth = 0;
    }
    else 
    {
        trueLeft = prc->left;
        trueWidth = prc->right - prc->left;
    }

    if ( trueWidth > 0 )
    {
        PatBlt(
                hDC,
                trueLeft,
                prc->top,
                trueWidth ,
                cThick,
                dwRop);
    }

    if ( pExcludeRect && 
         pExcludeRect->bottom == prc->bottom && 
         pExcludeRect->left == prc->left )
    {
        if ( pExcludeRect->right < prc->right )
        {
            trueLeft = pExcludeRect->right;
            trueWidth = abs ( prc->right - trueLeft );
        }
        else
            trueWidth = 0;
    }
    else if ( 
        pExcludeRect &&
        pExcludeRect->bottom == prc->bottom &&
        pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->left > prc->left )
        {    
            trueLeft = prc->left;
            trueWidth = pExcludeRect->left - trueLeft;
        }
        else
            trueWidth = 0;
    }
    else 
    {
        trueLeft = prc->left;
        trueWidth = prc->right - prc->left;
    }

    if ( trueWidth > 0 )
    {
        PatBlt(
                hDC,
                trueLeft,
                prc->bottom - cThick,
                trueWidth ,
                cThick,
                dwRop);
    }            
}

void
CGrabHandleAdorner::PatBltRectV(HDC hDC, RECT * prc, RECT* pExcludeRect, int cThick, DWORD dwRop)
{
    int trueTop = 0 ;
    int trueHeight = 0 ;
    
    //
    // Don't do general clipping. Assume that Oneside of pExcludeRect is equivalent
    // This is true once you're resizing with a grab handle.
    //
    if ( pExcludeRect &&
         pExcludeRect->top == prc->top && 
         pExcludeRect->left == prc->left )
    {
        if ( pExcludeRect->bottom < prc->bottom )
        {
            trueTop = pExcludeRect->bottom;
            trueHeight = prc->bottom - trueTop ; 
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }
    }
    else if ( pExcludeRect && 
              pExcludeRect->bottom == prc->bottom && 
              pExcludeRect->left == prc->left )
    {        
        if ( pExcludeRect->top > prc->top )
        {
            trueTop = prc->top;
            trueHeight = pExcludeRect->top - prc->top ;
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }
    }
    else 
    {
        trueTop = prc->top;
        trueHeight = prc->bottom - prc->top;
    }
   
    if ( trueHeight > 0 )
    {
        PatBlt(
                hDC,
                prc->left,
                trueTop + cThick,
                cThick,
                trueHeight - (2 * cThick),
                dwRop);
    }

    if ( pExcludeRect && 
         pExcludeRect->top == prc->top && 
         pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->bottom < prc->bottom )
        {
            trueTop = pExcludeRect->bottom;
            trueHeight = prc->bottom - trueTop ; 
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }
    }
    else if ( pExcludeRect && 
              pExcludeRect->bottom == prc->bottom && 
              pExcludeRect->right == prc->right )
    {
        if ( pExcludeRect->top > prc->top )
        {
            trueTop = prc->top;
            trueHeight = pExcludeRect->top - trueTop ; 
        }
        else
        {
            trueHeight = 0; // We have nothing to draw on this side - as we're inside the rect on this edge
        }

    }
    else 
    {
        trueTop = prc->top;
        trueHeight = prc->bottom - prc->top;
    }

    if ( trueHeight > 0 )
    {
        PatBlt(
                hDC,
                prc->right - cThick,
                trueTop + cThick,
                cThick,
                trueHeight - (2 * cThick),
                dwRop);
    }            
}

void
CGrabHandleAdorner::PatBltRect(HDC hDC, RECT * prc, RECT* pExcludeRect, int cThick, DWORD dwRop)
{
    PatBltRectH(hDC, prc, pExcludeRect, cThick, dwRop);

    PatBltRectV(hDC, prc, pExcludeRect, cThick, dwRop);
}

LPCTSTR
CGrabHandleAdorner::GetResizeHandleCursorId(ADORNER_HTI inAdorner)
{
    return sHandleCursor[ inAdorner ];
}

BOOL 
CGrabHandleAdorner::IsInResizeHandle( POINT pt , ADORNER_HTI *pGrabAdorner , RECT* pRectBounds  )
{
    int     i;
    RECT    rc;
    RECT  * pUseRectBounds;

    SetLocked();
    
    if ( _fLocked )
        return FALSE;

    //
    // We set our bounding rect - based on whether it was given to us - or our coords were set.
    //
    if ( pRectBounds )
        pUseRectBounds = pRectBounds;
    else
    {
        SetBoundsInternal();
        pUseRectBounds = & _rcBounds ;            
    }
    
    rc.left = rc.top = rc.bottom = rc.right = 0;    // to appease LINT

    for (i = 0; i < ARRAY_SIZE(seHitHandles); ++i)
    {
        GetGrabRect( seHitHandles[i], &rc, pUseRectBounds );
        if (PtInRect(&rc, pt))
        {
            _resizeAdorner = (char)i;
            if ( pGrabAdorner )
                *pGrabAdorner = seHitHandles[i]; // make use of ordering of ENUM

            TraceTag(( tagAdornerHitTest , "IsInResizeHandle: Point:%d,%d Hit:%d rc: left:%d top:%d, bottom:%d, right:%d ",pt.x, pt.y, 
                    TRUE , rc.left, rc.top, rc.bottom, rc.right )); 
                    
            return TRUE;
        }            
    }
    TraceTag(( tagAdornerHitTest , "Did Not Hit Resizehandle: Point:%d,%d Hit:%d rc: left:%d top:%d, bottom:%d, right:%d ",pt.x, pt.y, 
            FALSE , rc.left, rc.top, rc.bottom, rc.right )); 
    return FALSE ;
}

BOOL 
CGrabHandleAdorner::IsInMoveArea( POINT pt )
{
    SetBoundsInternal();
    RECT rc = _rcBounds;
    BOOL fInBounds = FALSE;

    SetLocked();
    
    if ( _fLocked )
        return FALSE;
        
    ::InflateRect( &rc, -GRAB_SIZE , -GRAB_SIZE);

    if ( ( PtInRect(&_rcBounds, pt)) && 
         ( ! PtInRect( & rc, pt) ) )
    {
        fInBounds = TRUE;
    }
    else
        fInBounds = FALSE;

    TraceTag(( tagAdornerHitTest , "Point:%d,%d InMoveArea:%d _rcBounds: top:%d left:%d, bottom:%d, right:%d rc: top:%d left:%d, bottom:%d, right:%d ",pt.x, pt.y, 
            fInBounds, _rcBounds.top, _rcBounds.left, _rcBounds.bottom, _rcBounds.right, 
            rc.left, rc.top, rc.bottom, rc.right ));
            
    return fInBounds;        
}

BOOL 
CGrabHandleAdorner::IsInAdorner( POINT pt )
{
    return ( PtInRect(&_rcBounds, pt) );
}
#if 0
VOID 
CGrabHandleAdorner::BeginMove( POINT pt, HWND hwnd, RECT* pElementRect /* = NULL */)
{
    Assert(_hDC == NULL);
    Assert(_hwnd == NULL);
    _hDC = GetDCEx( hwnd, 0, DCX_PARENTCLIP | DCX_LOCKWINDOWUPDATE);
    _hwnd = hwnd;
    HBRUSH hbr = GetFeedbackBrush();
    SelectObject(_hDC, hbr);

    if ( pElementRect == NULL )
    {    
        int myInset = GetInset();
        
        _rcFirst.left = _rcBounds.left + myInset;
        _rcFirst.right = _rcBounds.right - myInset;
        _rcFirst.top = _rcBounds.top + myInset;
        _rcFirst.bottom = _rcBounds.bottom - myInset;
    }
    else
    {
        _rcFirst = *pElementRect;
    }

    _ptFirst.x = pt.x - _rcFirst.left;
    _ptFirst.y = pt.y - _rcFirst.top;
    
    _fFeedbackVis = FALSE;
    _fDrawNew = TRUE;
    WHEN_DBG( _fInMove = TRUE );
}

//+====================================================================================
//
// Method: CalcRectMove
//
// Synopsis: Calculate the bounds of the rectangle we're moving
//
//------------------------------------------------------------------------------------

void
CGrabHandleAdorner::CalcRectMove(RECT * prc, POINT pt)
{
    *prc = _rcFirst;
    //
    // Translate the rectange by the amount that we've moved
    // relative to the first point
    //
    prc->left = pt.x - _ptFirst.x;
    prc->top  = pt.y - _ptFirst.y;
    prc->right = prc->left + ( _rcFirst.right - _rcFirst.left );
    prc->bottom = prc->top + ( _rcFirst.bottom - _rcFirst.top );
}

VOID 
CGrabHandleAdorner::EndMove(
                            POINT pt, 
                            POINT * pNewLocation, 
                            BOOL fDrawFeedback /*=TRUE*/,
                            RECT* pNewRect )
{
    Assert( _fInMove);
    
    _fDrawNew = FALSE;
    if ( fDrawFeedback )
        DuringMove( pt, TRUE, pNewRect );
    
    if (_hDC)
    {
        ReleaseDC(_hwnd, _hDC);
        _hDC = NULL;
        _hwnd = NULL;
    }

    if ( pNewLocation )
    {
        pNewLocation->x = _rc.left ;
        pNewLocation->y = _rc.top;
    }
    WHEN_DBG( _fInMove = FALSE );
}

VOID 
CGrabHandleAdorner::DuringMove( 
                                POINT pt, 
                                BOOL fForceErase /*= FALSE*/, 
                                RECT* pRect /* = NULL */ )
{ 
    Assert( _fInMove);
    RECT newRC ;

    if ( ! pRect )
    {
        CalcRectMove(  & newRC, pt );
    }
    else
    {
        newRC = *pRect;
    }
    
    if  ( fForceErase ||
         newRC.left != _rc.left      ||
         newRC.top != _rc.top         ||
         newRC.right != _rc.right     ||
         newRC.bottom != _rc.bottom)
    {
        if (( _fFeedbackVis ) || ( fForceErase ) )
        {
            DrawFeedbackRect( & _rc );
        }
        if ( _fDrawNew )
        {
            DrawFeedbackRect( & newRC );
            _fFeedbackVis = TRUE;
        }            
        _rc = newRC;
    }
}
#endif

VOID 
CGrabHandleAdorner::BeginResize( POINT pt, HWND hwnd )
{
    //
    // Check to see if the point is within the last call to IsInResizeHandle
    // if it isn't update IsInResizeHandle.
    //
    if ( _resizeAdorner != - 1)
    {
        RECT    rc;
        GetGrabRect( seHitHandles[_resizeAdorner], &rc, & _rcBounds );
        if (! PtInRect(&rc, pt))
            IsInResizeHandle( pt, NULL  );
    }
    else
        IsInResizeHandle( pt, NULL  );       
    Assert( _resizeAdorner != - 1);

    //
    // Do the stuff to set up a drag.
    //
   

    _fFeedbackVis = FALSE;   
    _fDrawNew = TRUE;
#if DBG ==1
    _fInResize = TRUE;
#endif

    SetBoundsInternal();
    GetControlBounds( & _rcControl );
    
    _rcFirst = _rcBounds ;
    _ptFirst = pt;
    
    TraceTag((tagAdornerShowResize,"BeginResize. _rcFirst: left:%ld, top:%ld, right:%ld, bottom:%ld", 
                                    _rcFirst.left,
                                    _rcFirst.top,
                                    _rcFirst.right,
                                    _rcFirst.bottom ));     
    TraceTag((tagAdornerShowResize,"BeginResize. _ptFirst: x:%ld, y:%ld\n", 
                                    _ptFirst.x,
                                    _ptFirst.y ));                                     
                                    
    _adj = sHandleAdjust[ _resizeAdorner ];
}

VOID 
CGrabHandleAdorner::EndResize(POINT pt, RECT * pNewSize )
{
    Assert( _fInResize );
    
    _fDrawNew = FALSE;
    DuringResize( pt, TRUE );
       
    pNewSize->left = _rc.left; 
    pNewSize->top  = _rc.top ;
    pNewSize->right = _rc.right ;
    pNewSize->bottom = _rc.bottom ;
    
    WHEN_DBG( _fInResize = FALSE );
}

VOID 
CGrabHandleAdorner::DuringResize( POINT pt, BOOL fForceErase )
{
    RECT newRC;
    Assert( _fInResize);
    
    CalcRect( & newRC, pt );
     
    if  ( fForceErase ||
         newRC.left != _rc.left      ||
         newRC.top != _rc.top         ||
         newRC.right != _rc.right     ||
         newRC.bottom != _rc.bottom)
    {
        if ( _fFeedbackVis )
        {
            DrawFeedbackRect( & _rc );
        }
        if ( _fDrawNew )
        {
            DrawFeedbackRect( & newRC );
            _fFeedbackVis = TRUE;
        }            
        _rc = newRC;
    }
}

//+====================================================================================
//
// Method: GetControlBounds
//
// Synopsis: Get the bounds of the control you adorn.
//
//------------------------------------------------------------------------------------

VOID
CGrabHandleAdorner::GetControlBounds(RECT* pRect)
{
    RECT
    SetBoundsInternal();
    int myInset = GetInset();

    *pRect = _rcBounds;
    pRect->left += myInset;
    pRect->top += myInset;
    pRect->right -= myInset;
    pRect->bottom -= myInset;
}

VOID
CGrabHandleAdorner::DrawFeedbackRect( RECT* prc )
{
    //
    // Get and release the DC every time. The office assistants like to blow it away.
    //
    HWND hwnd ;
    HDC hdc;

    TraceTag((tagAdornerShowResize,"DrawFeedbackRect: left:%ld, top:%ld, right:%ld, bottom:%ld", 
                                prc->left,
                                prc->top,
                                prc->right,
                                prc->bottom ));
                                
    IGNORE_HR( _pManager->GetViewServices()->GetViewHWND( & hwnd ));
    hdc = GetDCEx( hwnd, 0 , DCX_PARENTCLIP | DCX_LOCKWINDOWUPDATE);
    
    HBRUSH hbr = GetFeedbackBrush();
    SelectObject( hdc, hbr);

#if DBG == 1
    if ( IsTagEnabled(tagAdornerResizeRed) )
    {
        PatBltRect( hdc, prc, & _rcControl, FEEDBACKRECTSIZE, PATCOPY );
    }
    else
#endif    
    PatBltRect( hdc, prc, & _rcControl, FEEDBACKRECTSIZE, PATINVERT);

    if (hdc)
    {
        ReleaseDC(hwnd, hdc );
    }

}



HBRUSH 
CGrabHandleAdorner::GetFeedbackBrush()
{
    HBITMAP hbmp;
    
    if ( ! _hbrFeedback )
    {
#if DBG == 1
        COLORREF cr;
        if ( IsTagEnabled(tagAdornerResizeRed ))
        {
            cr = RGB(0xFF,0x00,0x00);
            _hbrFeedback = ::CreateSolidBrush(cr );
        }
        else
        {
#endif    
            // Load the bitmap resouce.
            hbmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDR_FEEDBACKRECTBMP));
            if (!hbmp)
                goto Cleanup;

            // Turn the bitmap into a brush.
            _hbrFeedback = CreatePatternBrush(hbmp);

            DeleteObject(hbmp);
#if DBG ==1
        }
#endif
    }

Cleanup:
    return _hbrFeedback;
}

HBRUSH 
CGrabHandleAdorner::GetHatchBrush()
{
    HBITMAP hbmp;

    if ( ! _hbrHatch )
    {
        // Load the bitmap resouce.
        hbmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDR_HATCHBMP));
        if (!hbmp)
            goto Cleanup;

        // Turn the bitmap into a brush.
        _hbrHatch = CreatePatternBrush(hbmp);

        DeleteObject(hbmp);
    }
Cleanup:
    return _hbrHatch;
}



//+------------------------------------------------------------------------
//
//  Member:     DrawGrabHandles
//
//  Synopsis:   Draws grab handles around the givin rect.
//
//-------------------------------------------------------------------------

void
CGrabHandleAdorner::DrawGrabHandles(HDC hdc, RECT *prc )
{
    if ( ! _fLocked )
    {
        int         typeBrush = WHITE_BRUSH;
        int         typePen = BLACK_PEN;
        HBRUSH      hbr = (HBRUSH) GetStockObject(typeBrush);
        HPEN        hpen = (HPEN) GetStockObject(typePen);
        int         i;

        Assert(hbr && hpen);

        // Load the brush and pen into the DC.
        hbr = (HBRUSH) SelectObject(hdc, hbr);
        hpen = (HPEN) SelectObject(hdc, hpen);

        // Draw each grab handle.
        for (i = 0; i < ARRAY_SIZE(seHitHandles); ++i)
        {
            RECT    rc;

            // Get the grab rect for this handle.
            GetGrabRect( seHitHandles[i] , &rc, prc );

            // Draw it.
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }

        // Restore the old brush and pen.
        hbr = (HBRUSH) SelectObject(hdc, hbr);
        hpen = (HPEN) SelectObject(hdc, hpen);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CGrabHandleBorders::DrawGrabBorders
//
//  Synopsis:   Draws grab borders around the givin rect.
//
//
//-------------------------------------------------------------------------
void
CGrabHandleAdorner::DrawGrabBorders(HDC hdc, RECT *prc, BOOL fHatch)
{
    DWORD       dwRop;
    HBRUSH      hbrOld = NULL;
    HBRUSH      hbr;
    RECT        rc = *prc;
    POINT       pt ;
    VARIANT             var;

    //
    // Check the background color of the doc.
    //

    _pIDoc->get_bgColor( & var );                       
    if (V_VT(&var) == VT_BSTR
        && (V_BSTR(&var) != NULL)
        && (StrCmpW(_T("#FFFFFF"), V_BSTR(&var)) == 0))
    {
        dwRop = DST_PAT_NOT_OR;
    }
    else
    {
        dwRop = DST_PAT_AND;
    }
    VariantClear(&var);
    
    GetViewportOrgEx (hdc, &pt) ;


    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));

    //
    // In the code below, before each select of a brush into the dc, we do
    // an UnrealizeObject followed by a SetBrushOrgEx.  This assures that
    // the brush pattern is properly aligned in that face of scrolling,
    // resizing or dragging the form containing the "bordered" object.
    //
    if (  _fLocked )
    {
        // For a locked elemnet - we just draw a rectangle around it.
        HPEN hpenOld;

        //InflateRect(&rc, -GRAB_SIZE / 2, -GRAB_SIZE / 2);

        hbrOld = (HBRUSH) SelectObject(hdc, GetStockObject(WHITE_BRUSH));
        if (!hbrOld)
            goto Cleanup;

        PatBltRect(hdc, &rc, NULL, GRAB_SIZE / 2, PATCOPY);

        hpenOld = (HPEN) SelectObject(hdc, GetStockObject(BLACK_PEN));
        if (!hpenOld)
            goto Cleanup;

        MoveToEx(hdc, rc.left, rc.top, (GDIPOINT *)NULL);
        LineTo(hdc, rc.left, rc.bottom - 1);
        LineTo(hdc, rc.right - 1, rc.bottom - 1);
        LineTo(hdc, rc.right - 1, rc.top);
        LineTo(hdc, rc.left, rc.top);

        if (hpenOld)
            SelectObject(hdc, hpenOld);
    }
    if (fHatch)
    {
        hbr = GetHatchBrush();
        if (!hbr)
            goto Cleanup;

        // Brush alignment code.
#ifndef WINCE
        // not supported on WINCE
        UnrealizeObject(hbr);
#endif
        SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(rc.left,8),
                           POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(rc.top,8), NULL);

        hbrOld = (HBRUSH) SelectObject(hdc, hbr);
        if (!hbrOld)
            goto Cleanup;

        PatBltRect(hdc, &rc, NULL, GRAB_SIZE, dwRop);
    }

//
// marka - we used to draw a border around positioned elements, access and VID
// have now said they don't want this anymore.
//
#ifdef NEVER    
    else
    {
        if (  _fIsPositioned )
        {
            // For a site with position=absolute, draw an extra rectangle around it when selected
            HPEN hpenOld;

            //InflateRect(&rc, -GRAB_SIZE / 2, -GRAB_SIZE / 2);

            hbrOld = (HBRUSH) SelectObject(hdc, GetStockObject(WHITE_BRUSH));
            if (!hbrOld)
                goto Cleanup;

            PatBltRect(hdc, &rc, GRAB_SIZE / 2, PATCOPY);

            hpenOld = (HPEN) SelectObject(hdc, GetStockObject(BLACK_PEN));
            if (!hpenOld)
                goto Cleanup;

            MoveToEx(hdc, rc.left, rc.top, (GDIPOINT *)NULL);
            LineTo(hdc, rc.left, rc.bottom - 1);
            LineTo(hdc, rc.right - 1, rc.bottom - 1);
            LineTo(hdc, rc.right - 1, rc.top);
            LineTo(hdc, rc.left, rc.top);

            if (hpenOld)
                SelectObject(hdc, hpenOld);
         }
    }
#endif

Cleanup:
    if (hbrOld)
        SelectObject(hdc, hbrOld);
}

CActiveControlAdorner::CActiveControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc, BOOL fLocked )
    : CGrabHandleAdorner( pIElement , pIDoc, fLocked )
{

}

CActiveControlAdorner::~CActiveControlAdorner()
{

}

HRESULT
CActiveControlAdorner::Draw ( 
        HDC hdc,
        RECT *pAdornerBounds)
{

    HRESULT hr = S_OK;
    
    DrawGrabBorders( hdc, pAdornerBounds, TRUE );
    DrawGrabHandles( hdc, pAdornerBounds );       

    RRETURN ( hr );
   
}



//+---------------------------------------------------------------------------
//
//  Member:     CursorTracker::CalcRect
//
//  Synopsis:   Calculates the rect used for sizing or selection
//
//  Arguments:  [prc] -- Place to put new rect.
//              [pt]  -- Physical point which is the new second coordinate of
//                         the rectangle.
//
//----------------------------------------------------------------------------

void
CGrabHandleAdorner::CalcRect(RECT * prc, POINT pt)
{
    RectFromPoint(prc, pt);
    //
    // Adjust the rect to be the rect of the element being resized.
    //
    int myInset = GetInset();
    prc->left += myInset;
    prc->top += myInset;
    prc->right -= myInset;
    prc->bottom -= myInset;

    
#if 0    
    SnapRect(prc);
#endif    
}



//+---------------------------------------------------------------------------
//
//  Member:     CursorTracker::RectFromPoint
//
//  Synopsis:   Calculates the rect used for sizing or selection
//
//  Arguments:  [prc] -- Place to put new rect.
//              [pt]  -- Physical point which is the new second coordinate of
//                         the rectangle.
//
//----------------------------------------------------------------------------

void
CGrabHandleAdorner::RectFromPoint(RECT * prc, POINT pt)
{
    //
    // Calc how far we've moved from the point where the tracker started.
    //
    int     dx = pt.x - _ptFirst.x;
    int     dy = pt.y - _ptFirst.y;
    int     t;

    //
    // Return the rect we started with.
    //
    *prc = _rcFirst;

    //
    // Adjust the returned rect based on how far we've moved and which edge
    // is being adjusted.
    //
    if (_adj & CT_ADJ_LEFT)
    {   
        prc->left   += dx;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Left adjusted by:%ld. left:%ld", dx, prc->left));
#endif
    }        

    if (_adj & CT_ADJ_TOP)
    {
        prc->top    += dy;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Top adjusted by:%ld. top:%ld", dy, prc->top));
#endif        
    } 
    
    if (_adj & CT_ADJ_RIGHT)
    {
        prc->right  += dx;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Right adjusted by:%ld. right:%ld", dx, prc->right));
#endif 
    }        

    if (_adj & CT_ADJ_BOTTOM)       
    {
        prc->bottom += dy;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. Bottom adjusted by:%ld. bototm:%ld", dy, prc->bottom));
#endif        
    }        

    //
    // Fix the left and right edges if they have become swapped.  This
    // occurs if the left edge has been moved right of the right edge,
    // or vice versa.
    //
    if (prc->right < prc->left)
    {
        //
        // The edge NOT being adjusted replaces the edge that is.  In
        // other words, if the left edge is now right of the right edge
        // then the old right edge becomes the new left edge.
        //
        if (_adj & CT_ADJ_LEFT)
            _ptFirst.x = _rcFirst.left = _rcFirst.right;
        else
            _ptFirst.x = _rcFirst.right = _rcFirst.left;

        //
        // Now flip the edges of the rect being returned.
        //
        t = prc->right;
        prc->right = prc->left;
        prc->left = t;

        //
        // Finally, remember that a different edge is being adjusted.
        //
        _adj ^= CT_ADJ_LEFT | CT_ADJ_RIGHT;
#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. left and right flipped. Left:%ld Right:%ld", prc->left , prc->right ));
#endif        
        
    }

    //
    // Same as above but with the top and bottom.
    //
    if (prc->bottom < prc->top)
    {
        if (_adj & CT_ADJ_TOP)
            _ptFirst.y = _rcFirst.top = _rcFirst.bottom;
        else
            _ptFirst.y = _rcFirst.bottom = _rcFirst.top;

        t = prc->bottom;
        prc->bottom = prc->top;
        prc->top = t;

        _adj ^= CT_ADJ_TOP | CT_ADJ_BOTTOM;

#if DBG == 1
        TraceTag((tagAdornerShowAdjustment, "RectFromPoint. bottom and top flipped. Top:%ld Bottom:%ld", prc->top , prc->bottom ));
#endif        
        
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CursorTracker::SnapRect
//
//  Synopsis:   Calculates the rect used for sizing or selection
//
//  Arguments:  [prc] -- Place to put new rect.
//
//----------------------------------------------------------------------------
#ifdef NEVER
void
CGrabHandleAdorner::SnapRect(RECT * prc)
{
    //
    // marka - this is stubbed out for now. Make work later.
    //

    CSiteRange * pSR = GetSR();

    Assert( pSR );

    if (_adj & CT_ADJ_LEFT)
    {
        pSR->SnapCoordToGrid(0, &prc->left, TRUE);
    }
    if (_adj & CT_ADJ_TOP)
    {
        pSR->SnapCoordToGrid(1, &prc->top, TRUE);
    }
    if (_adj & CT_ADJ_RIGHT)
    {
        pSR->SnapCoordToGrid(0, &prc->right, TRUE);
    }
    if (_adj & CT_ADJ_BOTTOM)
    {
        pSR->SnapCoordToGrid(1, &prc->bottom, TRUE);
    }
    EnforceMinSize(prc);
}
#endif    


//+------------------------------------------------------------------------
//
//  Member:     CCurs::CCurs
//
//  Synopsis:   Constructor.  Loads the specified cursor and pushes it
//              on top of the cursor stack.  If the cursor id matches a
//              standard Windows cursor, then the cursor is loaded from
//              the system.  Otherwise, the cursor is loaded from this
//              instance.
//
//  Arguments:  idr - The resource id
//
//-------------------------------------------------------------------------
CCursor::CCursor(LPCTSTR idr)
{
    _hcrsOld = ::GetCursor();
    _hcrs    = ::LoadCursor( NULL, idr );

    ShowCursor(FALSE);
    SetCursor(_hcrs);
    ShowCursor(TRUE);
}



//+------------------------------------------------------------------------
//
//  Member:     CCurs::~CCurs
//
//  Synopsis:   Destructor.  Pops the cursor specified in the constructor
//              off the top of the cursor stack.  If the active cursor has
//              changed in the meantime, through some other mechanism, then
//              the old cursor is not restored.
//
//-------------------------------------------------------------------------
CCursor::~CCursor( )
{
    if (GetCursor() == _hcrs)
    {
        ShowCursor(FALSE);
        SetCursor(_hcrsOld);
        ShowCursor(TRUE);
    }
}

void
CCursor::Show()
{
    ShowCursor(FALSE);
    SetCursor(_hcrs) ;
    ShowCursor(TRUE);
}

BOOL
CGrabHandleAdorner::IsEditable()
{
    return TRUE;
}

BOOL
CSelectedControlAdorner::IsEditable()
{
    return FALSE;
}

CSelectedControlAdorner::CSelectedControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * pIDoc, BOOL fLocked )
    : CGrabHandleAdorner( pIElement , pIDoc, fLocked )
{

}

CSelectedControlAdorner::~CSelectedControlAdorner()
{

}

HRESULT
CSelectedControlAdorner::Draw ( 
        HDC hdc,
        RECT *pAdornerBounds)
{
    HRESULT hr = S_OK;

    DWORD       dwRop;
    HBRUSH      hbrOld = NULL;
    HBRUSH      hbr;
    RECT        rc = *pAdornerBounds;
    POINT       pt ;
    VARIANT             var;

    //
    // Check the background color of the doc.
    //

#ifdef NEVER
        // the trident way...
    if ((Doc()->_pOptionSettings->crBack() & RGB(0xff, 0xff, 0xff))
             == RGB(0x00, 0x00, 0x00))
#endif

    _pIDoc->get_bgColor( & var );                       
    if (V_VT(&var) == VT_BSTR
        && (V_BSTR(&var) != NULL)
        && (StrCmpW(_T("#FFFFFF"), V_BSTR(&var)) == 0))
    {
        dwRop = DST_PAT_NOT_OR;
    }
    else
    {
        dwRop = DST_PAT_AND;
    }
    VariantClear(&var);
    
    GetViewportOrgEx (hdc, &pt) ;


    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));

    //
    // In the code below, before each select of a brush into the dc, we do
    // an UnrealizeObject followed by a SetBrushOrgEx.  This assures that
    // the brush pattern is properly aligned in that face of scrolling,
    // resizing or dragging the form containing the "bordered" object.
    //

    hbr = GetHatchBrush();
    if (!hbr)
        goto Cleanup;

    // Brush alignment code.
#ifndef WINCE
    // not supported on WINCE
    UnrealizeObject(hbr);
#endif
    SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(rc.left,8),
                       POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(rc.top,8), NULL);

    hbrOld = (HBRUSH) SelectObject(hdc, hbr);
    if (!hbrOld)
        goto Cleanup;

    PatBltRect(hdc, &rc, NULL, 4, dwRop);
    


Cleanup:
    if (hbrOld)
        SelectObject(hdc, hbrOld);

    RRETURN ( hr );        
}


