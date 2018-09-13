//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       caret.cpp
//
//  Contents:   Implementation of the caret manipulation for the
//              CTextSelectionRecord class.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CARET_HXX_
#define X_CARET_HXX_
#include "caret.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__IME_H_
#define X__IME_H_
#include "_ime.h"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

MtDefine(CCaret, Edit, "CCaret");
MtDefine(CCaretUpdateScreenCaret_aryCaretBitMap_pv, Locals, "CCaret::UpdateScreenCaret aryCaretBitMap::_pv")
MtDefine(CCaretCreateCSCaret_aryCaretBitMap_pv, Locals, "CCaret::CreateCSCaret aryCaretBitMap::_pv")


#define WCH_NODE    WCHAR(0x0082)

const LONG CCaret::_xInvisiblePos = -32000;
const LONG CCaret::_yInvisiblePos = -32000;
const UINT CCaret::_HeightInvisible = 1;
const UINT CCaret::_xCSCaretShift = 2; // COMPLEXSCRIPT Number of pixels to shift bitmap caret to properly align

#if DBG ==1
static const LPCTSTR strCaretName = _T(" Physical Caret");
#endif

//////////////////////////////////////////////////////////////////////////
//
//  CCaret's Constructor, Destructor
//
//////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------
//    Method:       CCaret::CCaret
//
//    Parameters:   CDoc*           pDoc        Document that owns caret
//
//    Synopsis:     Constructs the CCaret object, sets the caret's associated
//                  document.
//+-----------------------------------------------------------------------

CCaret::CCaret( CDoc * pDoc )
    : _pDoc( pDoc )
{
    _cRef = 0;
    _fVisible = FALSE;
    _fNotAtBOL = FALSE;
    _fMoveForward = TRUE;
    _fPainting = 0;
    _Location.x = 0;
    _Location.y = 0;
    _width = 1;
    _height = 0;
    _dx = 0;
    _dy = 0;
    _dh = 0;
    _fCanPostDeferred = TRUE;
}


//+-----------------------------------------------------------------------
//    Method:        CCaret::~CCaret
//
//    Parameters:    None
//
//    Synopsis:     Destroys the CCaret object, sets the caret's associated
//                  document to null, releases the markup pointer.
//+-----------------------------------------------------------------------

CCaret::~CCaret()
{
    //
    // Remove any pending updates
    //
    _fCanPostDeferred = FALSE; // I'm dying here...
    GWKillMethodCall( this, ONCALL_METHOD( CCaret, DeferredUpdateCaret, deferredupdatecaret ), 0 );
    GWKillMethodCall( this, ONCALL_METHOD( CCaret, DeferredUpdateCaretScroll, deferredupdatecaretscroll ), 0 );
    _pMPCaret->Unposition();
    if ( _fVisible )
    {
        ::DestroyCaret();
    }        
    delete _pMPCaret;
}


//+-----------------------------------------------------------------------
//    Method:        CCaret::Init
//
//    Parameters:    None
//
//    Synopsis:     This method initalizes the caret object. It will create 
//                  the caret's mu pointer in the main tree of the document.
//                  Note that after initializing the caret, you must position
//                  and make the caret visible explicitly.
//+-----------------------------------------------------------------------

HRESULT
CCaret::Init()
{
    HRESULT hr = S_OK;

    _pMPCaret = new CMarkupPointer( _pDoc );
    if( _pMPCaret == NULL )
        goto Cleanup;
        
    hr = THR( _pMPCaret->SetGravity( POINTER_GRAVITY_Right ) );
    
#if DBG == 1
    _pMPCaret->SetDebugName(strCaretName);
#endif

Cleanup:

    RRETURN( hr );
}



//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CCaret::IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CCaret::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CCaret::Release( void )
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
CCaret::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    switch(iid.Data1)
    {
        QI_INHERITS( this , IUnknown )
        QI_INHERITS( this , IHTMLCaret )
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}



//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CCaret::IHTMLCaret's Implementation
//
//////////////////////////////////////////////////////////////////////////

//+-----------------------------------------------------------------------
//    Method:       CCaret::MoveCaretToPointer
//
//    Parameters:   
//          pPointer        -   Pointer to move caret to the left of
//          fAtEOL          -   Is the caret after the EOL character?
//
//    Synopsis:     Moves the caret to the specified location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MoveCaretToPointer( 
    IMarkupPointer *    pPointer,
    BOOL                fNotAtBOL,
    BOOL                fScrollIntoView,
    CARET_DIRECTION     eDir )
{
    HRESULT hr = S_OK ;

    Assert( _pMPCaret );

    switch (eDir)
    {
    case CARET_DIRECTION_INDETERMINATE:
    {
        HRESULT hr;
        BOOL fEqual;
        BOOL fLeft;

        hr = THR(_pMPCaret->IsEqualTo(pPointer, &fEqual));
        if (hr || fEqual)
            break;

        hr = THR(_pMPCaret->IsLeftOf(pPointer, &fLeft));
        if (SUCCEEDED(hr))
        {
            _fMoveForward = fLeft;
        }
        break;
    }
    case CARET_DIRECTION_SAME:
        break;
    case CARET_DIRECTION_BACKWARD:
    case CARET_DIRECTION_FORWARD:
        _fMoveForward = (eDir == CARET_DIRECTION_FORWARD);
        break;
#if DBG==1
    default:
        Assert(FALSE);
#endif
    }

    hr = THR( _pMPCaret->MoveToPointer( pPointer ));
    if( hr )
        goto Cleanup;

    _fNotAtBOL = fNotAtBOL;    
    DeferUpdateCaret( _fVisible && fScrollIntoView ); // Only scroll on move if visible

Cleanup:
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::MoveCaretToPointerEx
//
//    Parameters:   
//          pPointer        -   Pointer to move caret to the left of
//          fAtEOL          -   Is the caret after the EOL character?
//
//    Synopsis:     Moves the caret to the specified location and set
//                  some common attributes. This should cut down on the
//                  number of times we calc the caret's location
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MoveCaretToPointerEx( 
    IMarkupPointer *    pPointer,
    BOOL                fAtEOL,
    BOOL                fVisible,
    BOOL                fScrollIntoView,
    CARET_DIRECTION     eDir )
{
    _fVisible = fVisible;
    RRETURN ( THR( MoveCaretToPointer( pPointer, fAtEOL, fScrollIntoView, eDir )));
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::MovePointerToCaret
//
//    Parameters:   IMarkupPointer *    Pointer to move to the right of the 
//                                      caret
//
//    Synopsis:     Moves the pointer to the right of the caret's location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::MovePointerToCaret( 
    IMarkupPointer *    pPointer )
{
    HRESULT hr = S_OK ;
    Assert( _pMPCaret );
    hr = THR( pPointer->MoveToPointer( _pMPCaret ));
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::IsVisible
//
//    Parameters:   BOOL *  True if the caret is visible.
//
//    Synopsis:     The caret is visible if Show() has been called. NOTE that
//                  this does not tell you if the caret is on the screen.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::IsVisible(
    BOOL *              pIsVisible)
{
    
    *pIsVisible = _fVisible;
  
    return S_OK;
}

BOOL
CCaret::IsPositioned()
{
    BOOL fPositioned = FALSE;

    IGNORE_HR( _pMPCaret->IsPositioned( & fPositioned ));

    return fPositioned;
}

//+-----------------------------------------------------------------------
//    Method:       CCaret::Show
//
//    Parameters:   None
//
//    Synopsis:     Un-hides the caret. It will display wherever it is 
//                  currently located.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::Show(
    BOOL        fScrollIntoView )
{
    HRESULT hr = S_OK;
    _fVisible = TRUE;
    DeferUpdateCaret( fScrollIntoView );
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::Hide
//
//    Parameters:   None
//
//    Synopsis:     Makes the caret invisible.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::Hide()
{
    HRESULT hr = S_OK;
    _fVisible = FALSE;
    
    if( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd )
    {
        ::HideCaret( _pDoc->_pInPlace->_hwnd );
    }
    
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::InsertText
//
//    Parameters:   OLECHAR *           The text to insert
//
//    Synopsis:     Inserts text to the left of cursor's current location.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::InsertText( 
    OLECHAR *           pText,
    LONG                lLen )
{
    Assert( _pMPCaret );
    
    HRESULT hr = S_OK ;
    LONG lActualLen = lLen;
    ELEMENT_TAG eTag = ETAG_NULL;
    CTreeNode* pNode = NULL;
    
    if ( !_pMPCaret || !_pMPCaret->IsPositioned() )
    {
        hr= E_FAIL;
        goto Cleanup;
    }                

    //
    // marka - being EXTRA CAREFUL to fix a Stress failure.
    //
    pNode = _pMPCaret->CurrentScope(MPTR_SHOWSLAVE);
    if ( !pNode )
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    eTag = pNode->Element()->Tag();    
    
    if( eTag == ETAG_INPUT || eTag == ETAG_TXTSLAVE )
    {
        CFlowLayout * pLayout = GetFlowLayout();
        Assert( pLayout );
        if ( ! pLayout )
        {
            hr = E_FAIL;
            goto Cleanup;
        }
   
        if( lActualLen < 0 )
            lActualLen = pText ? _tcslen( pText ) : 0;
            
        LONG lMaxLen = pLayout->GetMaxLength();
        LONG lContentLen = pLayout->GetContentTextLength();
        LONG lCharsAllowed = lMaxLen - lContentLen;

        if( lActualLen > lCharsAllowed )
        {
            lActualLen = lCharsAllowed;
        }

        if( lActualLen <= 0 )
            goto Cleanup;
          
    }

    _fNotAtBOL = TRUE;
    _fTyping = TRUE;
    
    hr = _pDoc->InsertText( _pMPCaret, pText, lActualLen );
    if( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::InsertMarkkup
//
//    Parameters:   OLECHAR *           The markup to insert
//
//    Synopsis:     Inserts html to the left of cursor's current location.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::InsertMarkup( 
    OLECHAR *           pMarkup )
{
    return E_NOTIMPL;
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::ScrollIntoView
//
//    Parameters:   None
//
//    Synopsis:     Scrolls the current cursor location into view.
//+-----------------------------------------------------------------------

STDMETHODIMP 
CCaret::ScrollIntoView()
{
    DeferUpdateCaret( TRUE );
    return S_OK; // we can allways post a deferred update
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::GetElementContainer
//
//    Parameters:   IHTMLElement **     The containing element.
//
//    Synopsis:     Returns the parent element at the cursor's current 
//                  location.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::GetElementContainer( 
    IHTMLElement **     ppElement )
{
    HRESULT hr = S_OK ;
    hr = THR( _pMPCaret->CurrentScope( ppElement ));
    RRETURN( hr );
}


CTreeNode *
CCaret::GetNodeContainer(DWORD dwFlags)
{  
    return (_pMPCaret) ? _pMPCaret->CurrentScope(dwFlags) : NULL;
}


STDMETHODIMP
CCaret::GetLocation(
    POINT *             pPoint,
    BOOL                fTranslate )
{
    HRESULT hr = S_OK;
    
    if( fTranslate )
    {
        CFlowLayout * pLayout = GetFlowLayout();
        if( pLayout == NULL )
            RRETURN( E_FAIL );
            
        CPoint t( _Location );
        pLayout->TransformPoint(  &t, COORDSYS_CONTENT, COORDSYS_GLOBAL );
        pPoint->x = t.x;
        pPoint->y = t.y;
    }
    else
    {
        pPoint->x = _Location.x;
        pPoint->y = _Location.y;
    }

    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::UpdateCaret
//
//    Parameters:   none
//
//    Synopsis:     Allow the client of an IHTMLCaret to ask us to update
//                  our current location when we get around to it.
//+-----------------------------------------------------------------------

STDMETHODIMP
CCaret::UpdateCaret()
{
    DeferUpdateCaret( FALSE );
    return( S_OK );
}


//+-----------------------------------------------------------------------
//    Method:       CCaret::UpdateCaret
//
//    Parameters:   
//                  fScrollIntoView -   If TRUE, scroll caret into view if 
//                                      we have focus or if not and 
//                                      selection isn't hidden
//                  fForceScroll    -   Force the caret to scroll no
//                                      matter what
//                  pdci            -   CDocInfo. Not used and may be null
//
//    Synopsis:     Allow the internal client of an IHTMLCaret to ask us 
//                  to update our current location when we get around to it. 
//                  Also allows internal (mshtml) clients to control if we 
//                  scroll.
//
//+-----------------------------------------------------------------------

HRESULT
CCaret::UpdateCaret(
    BOOL        fScrollIntoView, 
    BOOL        fForceScroll,
    CDocInfo *  pdci )
{
    DeferUpdateCaret( ( fScrollIntoView || fForceScroll ));
    return( S_OK );
}


HRESULT
CCaret::BeginPaint()
{
    _fPainting ++ ;

    if( _fPainting == 1 )
    {
        _fUpdateEndPaint = FALSE;
        _fUpdateScrollEndPaint = FALSE;
    
        if( _fVisible )
        {
            Assert( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd );
            ::HideCaret( _pDoc->_pInPlace->_hwnd );
        }
    }
    
    return S_OK;
}


HRESULT
CCaret::EndPaint()
{
    HRESULT hr = S_OK;
    
    _fPainting -- ;

    //
    // if we recieved a deferred update of any flavor,
    // we want to post another update to compensate for
    // the loss.
    //

    if( _fPainting == 0 )
    {
        if( _fUpdateScrollEndPaint )
        {
            DeferUpdateCaret( TRUE );
        }
        else if( _fUpdateEndPaint )
        {
            DeferUpdateCaret( FALSE );
        }
        else if( _fVisible )
        {
            Assert( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd );
            ::ShowCaret( _pDoc->_pInPlace->_hwnd );
        }
        
        //
        // handled, even if there were nested paints...
        //

        _fUpdateEndPaint = FALSE;
        _fUpdateScrollEndPaint = FALSE;
    }

    return hr;
}


HRESULT
CCaret::LoseFocus()
{
//    ::DestroyCaret();
    if( _pDoc && _pDoc->_pInPlace && _pDoc->_pInPlace->_hwnd )
        ::HideCaret( _pDoc->_pInPlace->_hwnd );

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//  Private Utility functions
//
//////////////////////////////////////////////////////////////////////////

CFlowLayout *
CCaret::GetFlowLayout()
{
    CFlowLayout *   pFlowLayout;
    CTreeNode *     pNode;
    
    pNode = _pMPCaret->CurrentScope(MPTR_SHOWSLAVE);
    if( pNode == NULL )
        return NULL;
        
    pFlowLayout = _pDoc->GetFlowLayoutForSelection( pNode );

    
    return pFlowLayout;
}

//+-----------------------------------------------------------------------
//    CCaret::UpdateScreenCaret
//    PRIVATE
//
//    Parameters:   None
//
//    Synopsis:     Something moved us, so we have to update the position of 
//                  the screen caret.
//+-----------------------------------------------------------------------

HRESULT
CCaret::UpdateScreenCaret( BOOL fScrollIntoView, BOOL fIsIME )
{
    HRESULT             hr = S_OK;
    POINT               ptGlobal;
    POINT               ptClient;
    CFlowLayout *       pLayout;
    LCID                curKbd = LOWORD(GetKeyboardLayout(0));
    short               yDescent = 0;
    CTreeNode *         pNode = NULL;
    CCharFormat const * pCharFormat = NULL;
    LONG                cp;
    CCalcInfo           CI;
    CDisplay *          pdp = NULL;

    CLinePtr            rp( pdp );

    //
    // Prepare to update the caret
    //

    Assert( _pDoc != NULL );
    if( _pDoc == NULL )
    {
        return FALSE;
    }

    // If any of the following are true, we have no work to do
    if(     _pMPCaret == NULL || 
            _pDoc->_state < OS_INPLACE || 
            _pDoc->_pInPlace == NULL ||
            _pDoc->_pInPlace->_hwnd == NULL ||
            _pDoc->TestLock( SERVERLOCK_BLOCKPAINT ))
    {
        return FALSE;
    }

    if( ! _pMPCaret->IsPositioned() )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
        
    pNode = _pMPCaret->CurrentScope(MPTR_SHOWSLAVE);
    if( pNode == NULL )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    pLayout = _pDoc->GetFlowLayoutForSelection(pNode);
    if( pLayout == NULL )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
        
    pdp = pLayout->GetDisplay();

    if( pdp == NULL )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    cp = _pMPCaret->GetCp();
    pdp->WaitForRecalc( cp, -1 );

    // 
    // Calculate the updated caret location
    //
    
    pCharFormat = pNode->GetCharFormat();
    CI.Init(pLayout);

    if (-1 == pdp->PointFromTp( cp, NULL, _fNotAtBOL, _fMoveForward, ptClient, &rp, TA_BASELINE, &CI))
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }

    Assert( pNode->Element()->Tag() != ETAG_ROOT );

    //
    // Get the text height
    //

    if( ! _fVisible || _fPainting > 0 )
    {
        _height = 1;
    }
    else
    {
        // Start with springloader
        extern long GetSpringLoadedHeight(IMarkupPointer *, CFlowLayout *, short *);    
        _height = GetSpringLoadedHeight(NULL, pLayout, &yDescent);
        
        if( _height == -1 )
        {
            // if springloader fails, get the text height from the format cache
            CCcs     *pccs;
            CBaseCcs *pBaseCcs;
            
            pccs = fc().GetCcs( CI._hdc, &CI, pCharFormat );
            if(!pccs)
                goto Cleanup;

            pBaseCcs = pccs->GetBaseCcs();
            _height = pBaseCcs->_yHeight + pBaseCcs->_yOffset;
            yDescent = pBaseCcs->_yDescent;
            pccs->Release();
        }
    }

    if( ! pdp->IsRTL() )
    {
        _Location.x = ptClient.x - ( _hbmpCaret == NULL ? 0 : _xCSCaretShift) ;
    }
    else
    {
        // the -1 at the end is an off by one adjustment needed for RTL
        _Location.x = - ptClient.x - ( _hbmpCaret == NULL ? 0 : _xCSCaretShift) - 1 ;
    }

    _Location.y = ptClient.y - ( _height - yDescent ) ;

    //
    // Render the caret in the proper location
    //

    Assert( _fPainting == 0 );

    if( DocHasFocus() ||
      ( _pDoc->_pDragDropTargetInfo != NULL ) ) // We can update caret if we don't have focus during drag & drop
    {
        if( !fIsIME && g_fComplexScriptInput)
        {
            CreateCSCaret(curKbd);
        }
        else
        {
            if( _hbmpCaret != NULL )
            {
                DestroyCaret();
                DeleteObject((void*)_hbmpCaret);
                _hbmpCaret = NULL;
            }
        }

        
        if( FAILED( hr ) )
        {
            goto Cleanup;
        }
        
        if( _pDoc->_state > OS_INPLACE )
        {

            ::HideCaret( _pDoc->_pInPlace->_hwnd );

            // if we have been requested to scroll, scroll the caret location into view
            if( fScrollIntoView && _fVisible && _fPainting == 0 )
            {
                CRect rcLocal(  _Location.x + _dx , 
                                _Location.y + _dy , 
                                _Location.x + _dx + _width, 
                                _Location.y + _dy + _height + _dh );
                                
                pLayout->ScrollRectIntoView( rcLocal, SP_MINIMAL , SP_MINIMAL, TRUE );
            }
            
            long xAdjust = 0;
            if(_hbmpCaret)
            {
                if(pLayout->GetDisplay()->IsRTL())
                    xAdjust -= _xCSCaretShift;
                else
                    xAdjust += _xCSCaretShift;
            }

            // Translate our coordinates into window coordinates
            hr = THR( GetLocation( &ptGlobal , TRUE ) );
            CRect rcGlobal( ptGlobal.x + _dx + xAdjust, 
                            ptGlobal.y + _dy, 
                            ptGlobal.x + _dx + xAdjust + _width, 
                            ptGlobal.y + _dy + _height + _dh );

            // Calculate the visible caret after clipping to the layout in global
            // window coordinates
            CRect rcLayout;
            pLayout->GetClippedClientRect( &rcLayout, CLIENTRECT_CONTENT );
            pLayout->TransformRect( &rcLayout, COORDSYS_CONTENT, COORDSYS_GLOBAL );
            rcGlobal.IntersectRect( rcLayout );            
            
            INT iW = rcGlobal.right - rcGlobal.left;
            INT iH = rcGlobal.bottom - rcGlobal.top;

            ::CreateCaret(  _pDoc->_pInPlace->_hwnd,
                            _hbmpCaret,
                            iW,
                            iH );

            ::SetCaretPos( rcGlobal.left - xAdjust , rcGlobal.top );

            if( _fVisible && _fPainting == 0 && iW > 0  && iH > 0 )
            {
                ::ShowCaret( _pDoc->_pInPlace->_hwnd );                
            }
        }
    }

Cleanup:
    RRETURN( hr );
}



//+-----------------------------------------------------------------------
//  PRIVATE
//  CCaret::CreateCSCaret
//  COMPLEXSCRIPT
//
//  Parameters: LCID        curKbd              Current Keyboard script
//
//  Synopsis:   creates bitmap caret for complex scripts...like Arabic, 
//              Hebrew, Thai, etc.
//+-----------------------------------------------------------------------

void 
CCaret::CreateCSCaret(LCID curKbd)
{
    // array for creating the caret bitmap if needed
    CStackDataAry<WORD, 128> aryCaretBitMap( Mt( CCaretCreateCSCaret_aryCaretBitMap_pv ));

    // A new caret will be created. Destroy the caret bitmap if it existed
    if(_hbmpCaret != NULL)
    {
        DestroyCaret();
        DeleteObject((void*)_hbmpCaret);
        _hbmpCaret = NULL;
    }

    int i;
    Assert( _height >= 0 );

     // dynamically allocate more memory for the cursor so that
     // the bmp can be created to the correct size
     aryCaretBitMap.Grow( _height );

     // draw the vertical stem from top to bottom
     for(i = 0; i < _height; i++)
     {
         aryCaretBitMap[i] = 0x0020;
     }

    if(PRIMARYLANGID(LANGIDFROMLCID(curKbd)) == LANG_THAI)
    {
        // when Thai keyboard is active we need to create the cursor which
        // users are accustomed to

        // change the bottom row to the tail
        aryCaretBitMap[_height - 1] = 0x0038;

        // create the bitmap
         _hbmpCaret = (HBITMAP)CreateBitmap(5, _height, 1, 1, (void*) aryCaretBitMap );
    }
    else if(g_fBidiSupport)
    {

        // on a Bidi system we need to create the directional cursor which
        // users are accustomed to

        // get the current keyboard direction and draw the flag
        if(IsRtlLCID(curKbd))
        {
            aryCaretBitMap[0] = 0x00E0;
            aryCaretBitMap[1] = 0x0060;
        }
        else
        {
            aryCaretBitMap[0] = 0x0038;
            aryCaretBitMap[1] = 0x0030;
        }

    }

    // create the bitmap
    _hbmpCaret = (HBITMAP)CreateBitmap(5, _height, 1, 1, (void*) aryCaretBitMap);

}

//+-------------------------------------------------------------------------
//
//  Method:     DeferUpdateCaret
//
//  Synopsis:   Deferes the call to update caret
//
//  Returns:    None
//
//--------------------------------------------------------------------------
void
CCaret::DeferUpdateCaret( BOOL fScroll )
{
    if( _fCanPostDeferred )
    {
        // Kill pending calls if any
        GWKillMethodCall(this, ONCALL_METHOD(CCaret, DeferredUpdateCaret, deferredupdatecaret), 0);
        
        if( fScroll || _fTyping )
        {
            if( _fTyping )
            {
                _fTyping = FALSE;
            }
            
            // Kill pending scrolling calls if any
            GWKillMethodCall(this, ONCALL_METHOD(CCaret, DeferredUpdateCaretScroll, deferredupdatecaretscroll), 0);
            // Defer the update caret scroll call
            IGNORE_HR(GWPostMethodCall(this,
                                       ONCALL_METHOD(CCaret, DeferredUpdateCaretScroll, deferredupdatecaretscroll),
                                       (DWORD_PTR)_pDoc, FALSE, "CCaret::DeferredUpdateCaretScroll")); // There can be only one caret per cdoc
        }
        else
        {
            // Defer the update caret call
            IGNORE_HR(GWPostMethodCall(this,
                                       ONCALL_METHOD(CCaret, DeferredUpdateCaret, deferredupdatecaret),
                                       (DWORD_PTR)_pDoc, FALSE, "CCaret::DeferredUpdateCaret")); // There can be only one caret per cdoc
        }
    }
}


void
CCaret::DeferredUpdateCaret( DWORD_PTR dwContext )
{
    DWORD_PTR dwDoc = (DWORD_PTR)_pDoc;
    CDoc * pInDoc = (CDoc *) dwContext;

    if( _fPainting > 0 )
    {
        _fUpdateEndPaint = TRUE;
    }
    else if( dwDoc == dwContext && pInDoc->_state >= OS_INPLACE )
    {
        UpdateScreenCaret( FALSE , FALSE );
    }
}


void
CCaret::DeferredUpdateCaretScroll( DWORD_PTR dwContext )
{
    DWORD dwDoc = (DWORD_PTR)_pDoc;
    CDoc * pInDoc = (CDoc *) dwContext;

    if( _fPainting > 0 )
    {
        _fUpdateScrollEndPaint = TRUE;
    }
    else if( dwDoc == dwContext && pInDoc->_state >= OS_INPLACE )
    {
        UpdateScreenCaret( TRUE, FALSE );
    }
}


BOOL
CCaret::DocHasFocus()
{
    // BUGBUG (johnbed) potentially too simplistic - what if a control inside Trident has focus.
    // may be okay. Perhaps we should only show the caret if Trident itself has focus.
    
    BOOL fOut = FALSE;
    HWND hwHasFocus = GetFocus();
    fOut = ( _pDoc->_pInPlace != NULL                   && 
             _pDoc->_pInPlace->_hwnd != NULL            && 
             hwHasFocus != NULL && 
             _pDoc->_pInPlace->_hwnd == hwHasFocus );

    return fOut;
}


LONG
CCaret::GetCp()
{
    LONG cp = 0;

    if( _pMPCaret->IsPositioned() )
    {
        cp = _pMPCaret->GetCp();
    }

    return cp;
}
