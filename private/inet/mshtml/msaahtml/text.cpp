//================================================================================
//      File:   TEXT.CPP
//      Date:   7/28/97
//      Desc:   Contains implementation of CTextAE class.  CTextAE 
//              implements the accessible proxy for the Trident Table 
//              object.
//================================================================================

//================================================================================
//  Includes
//================================================================================

#include "stdafx.h"
#include <tchar.h>
#include "text.h"
#include "trid_ao.h"
#include "document.h"
#include "window.h"
#include "anchor.h"


//================================================================================
//  Defines
//
//  TODO:   Move all defines into a string table.  Load these strings from
//          the resource file, possibly as static CTextAE members.  Note that
//          the AOM Manager also uses, at least, the TEXTRANGE_* strings.
//================================================================================

#define HTMLTAG_BEGINDELIMITER              (_TCHAR('<'))
#define HTMLTAG_ENDDELIMITER                (_TCHAR('>'))
#define HTMLTAG_ENDTAGDELIMITER             (_TCHAR('/'))
#define HTMLTAG_SPAN                        (_T("SPAN"))

#define TEXTRANGE_MOVE_UNIT_CHARACTER       (_T("CHARACTER"))
#define TEXTRANGE_MOVE_UNIT_WORD            (_T("WORD"))
#define TEXTRANGE_MOVE_UNIT_SENTENCE        (_T("SENTENCE"))
#define TEXTRANGE_MOVE_UNIT_TEXTEDIT        (_T("TEXTEDIT"))

#define TEXTRANGE_SETENDPOINT_STARTTOSTART  (_T("STARTTOSTART"))
#define TEXTRANGE_SETENDPOINT_STARTTOEND    (_T("STARTTOEND"))
#define TEXTRANGE_SETENDPOINT_ENDTOSTART    (_T("ENDTOSTART"))
#define TEXTRANGE_SETENDPOINT_ENDTOEND      (_T("ENDTOEND"))


//================================================================================
//  CTextAE public methods
//================================================================================

//-----------------------------------------------------------------------
//  CTextAE::CTextAE()
//
//  DESCRIPTION:
//
//      Constructor
//
//  PARAMETERS:
//
//      pAOParent   [IN]    Pointer to the parent accessible object in 
//                          the AOM tree
//
//      pDocAO      [IN]    Pointer to closest document object.
//
//      nChildID    [IN]    AOM tree child ID.
//
//      hWnd        [IN]    Pointer to the window of the trident object
//                          that this object corresponds to.
//
//  RETURNS:
//
//      None.
//
//-----------------------------------------------------------------------


CTextAE::CTextAE( CTridentAO* pAOParent, UINT nChildID, HWND hWnd )
: CTridentAE( pAOParent, -1, nChildID, hWnd )
{
    //------------------------------------------------
    // Assign the delegating IUnknown to CTextAE :
    //  this member will be overridden in derived class
    //  constructors so that the delegating IUnknown 
    //  will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown = (LPUNKNOWN)this;                                  

    //--------------------------------------------------
    // Set the role parameter to be used
    //  in the default CAccElement implementation.
    //
    // TODO: set correctly.
    //--------------------------------------------------

    m_lRole = ROLE_SYSTEM_TEXT;

    //--------------------------------------------------
    // set the item type so that it can be accessed
    // via base class pointer.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_TEXT;


    //------------------------------------------------
    //  Initialize CTextAE member variables.
    //------------------------------------------------

    m_pIHTMLDocument2 = NULL;
    
    m_pIHTMLTxtRange = NULL;


#ifdef _DEBUG

    //--------------------------------------------------
    // Set symbolic name of object for easy identification
    //--------------------------------------------------

    lstrcpy(m_szAOMName,_T("Text"));

#endif

}

//-----------------------------------------------------------------------
//  CTextAE::~CTextAE()
//
//  DESCRIPTION:
//
//      CTextAE class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//-----------------------------------------------------------------------

CTextAE::~CTextAE()
{
    ReleaseTridentInterfaces();
}

void
CTextAE::ReleaseTridentInterfaces()
{
    if ( m_pIHTMLTxtRange )
    {
        m_pIHTMLTxtRange->Release();
        m_pIHTMLTxtRange = NULL;
    }

    if ( m_pIHTMLDocument2 )
    {
        m_pIHTMLDocument2->Release();
        m_pIHTMLDocument2 = NULL;
    }
}

//-----------------------------------------------------------------------
//  CTextAE::Init()
//
//  DESCRIPTION:
//
//      Initialization : set values of data members
//
//  PARAMETERS:
//
//      pTxtRngObjIUnk  [IN]    Pointer to IUnknown of TOM text range.
//
//      pTOMDoc         [IN]    Pointer to IUnknown of TOM document.
//
//  RETURNS:
//
//      S_OK | E_FAIL | E_NOINTERFACE
//
//-----------------------------------------------------------------------

HRESULT CTextAE::Init( IUnknown* pTxtRngObjIUnk, IUnknown* pTOMDocIUnk )
{

    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    assert(pTxtRngObjIUnk);
    assert(pTOMDocIUnk);

    if( (!pTxtRngObjIUnk) || (!pTOMDocIUnk)  )
        return(E_INVALIDARG);

    //--------------------------------------------------
    //  Obtain a referenced pointer to the TOM Document.
    //--------------------------------------------------

    hr = pTOMDocIUnk->QueryInterface( IID_IHTMLDocument2, (void**) &m_pIHTMLDocument2 );

    if ( hr != S_OK )
        return hr;

    if ( !m_pIHTMLDocument2 )
        return E_NOINTERFACE;



    //--------------------------------------------------
    // store the text range IUnknown in the m_pTOMObjIUnk
    // parameter so that QIs on TOM objects dont GPF :
    // (they will just fail)
    //--------------------------------------------------

    if(hr = pTxtRngObjIUnk->QueryInterface(IID_IUnknown,(void **)&m_pTOMObjIUnk))
        return(hr);


    //--------------------------------------------------
    //  Create a duplicate of the text range and hold
    //  onto a referenced pointer to it.  The dupe is
    //  created so that the AOM Manager doesn't change
    //  the text range out from under us.
    //--------------------------------------------------

    CComQIPtr<IHTMLTxtRange,&IID_IHTMLTxtRange> pIHTMLTxtRange( pTxtRngObjIUnk );

    if ( !pTxtRngObjIUnk )
        return E_NOINTERFACE;

    hr = pIHTMLTxtRange->duplicate( &m_pIHTMLTxtRange );

    if ( hr != S_OK )
        return hr;

    if ( !m_pIHTMLTxtRange )
        return E_NOINTERFACE;

#ifdef _DEBUG

    BSTR bstrText;

    if (hr = m_pIHTMLTxtRange->get_text(&bstrText))
        return(hr);

    SysFreeString(bstrText);

#endif
    
    return hr;
}



//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CTextAE::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CTextAE object only implements
//      IUnknown.
//
//  PARAMETERS:
//
//      riid    [IN]    REFIID of requested interface.
//      ppv     [OUT]   Pointer to requested interface pointer.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//-----------------------------------------------------------------------

STDMETHODIMP CTextAE::QueryInterface(REFIID riid, void** ppv)
{
    if(!ppv)
        return(E_INVALIDARG);

    *ppv = NULL;


    if (riid == IID_IUnknown)  
    {
        *ppv = (LPUNKNOWN)this;
        ((LPUNKNOWN) *ppv)->AddRef();
    }
    else
        return(E_NOINTERFACE);

    return(NOERROR);
}


//================================================================================
//  CTextAE Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//  CTextAE::GetAccName()
//
//  DESCRIPTION:
//
//  lChild      child ID
//  pbstrName       pointer to array to return child name in.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
//-----------------------------------------------------------------------

HRESULT CTextAE::GetAccName(long lChild, BSTR * pbstrName)
{
    HRESULT hr = S_OK;

    
    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pbstrName );

    if(!pbstrName)
        return(E_INVALIDARG);

    if ( !m_bstrName )
    {
        //--------------------------------------------------
        //  Get the accName of the object from the
        //  corresponding TOM text range.
        //--------------------------------------------------

        hr = m_pIHTMLTxtRange->get_text( &m_bstrName );

        if ( hr != S_OK )
            return hr;

        if ( !m_bstrName )
            return E_FAIL;
    }
    
    //--------------------------------------------------
    // set name to point to the returned name.
    //--------------------------------------------------

    *pbstrName = SysAllocString(m_bstrName);

    return hr;
}


//-----------------------------------------------------------------------
//  CTextAE::GetAccState()
//
//  DESCRIPTION:
//
//      state of text block is always read-only.
//
//  PARAMETERS:
//      
//      lChild      ChildID
//      plState     long to store returned state var in.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
//-----------------------------------------------------------------------
    
HRESULT CTextAE::GetAccState(long lChild, long *plState)
{
    HRESULT         hr              = NOERROR;
    CWindowAO *     pWindowAO       = NULL;
    IHTMLTxtRange * pIHTMLTxtRange  = NULL;
    BOOL            bSelected       = FALSE;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( plState );

    if(!plState)
        return(E_INVALIDARG);

    //--------------------------------------------------
    // text is always readonly.
    //--------------------------------------------------

    *plState = STATE_SYSTEM_READONLY;


    //--------------------------------------------------
    // Delegate to ancestor if it's an anchor. This will
    // update our state.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
    {
        long    lTmpState = 0;

        hr = pAnc->GetAccState( CHILDID_SELF, &lTmpState );

        if ( hr == S_OK )
            *plState |= lTmpState;
    }

    //--------------------------------------------------
    // Any errors associated with the above anchor
    // ancestor gyrations will be ignored.
    //--------------------------------------------------

    //--------------------------------------------------
    // Update the location to determine offscreen status
    //--------------------------------------------------

    long lDummy;

    hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

    if (SUCCEEDED( hr ) && m_bOffScreen)
        *plState |= STATE_SYSTEM_INVISIBLE;

    //--------------------------------------------------
    // Does the CWindowAO of the CDocumentAO that
    // ancestors the text has the focus.  If so,
    // then this text is selectable.
    //--------------------------------------------------

    BOOL        bBrowserWindowHasFocus;
    BOOL        bParentWindowHasFocus;


    pWindowAO = (CWindowAO *)(m_pParent->GetDocumentAO()->GetParent());

    hr = pWindowAO->IsFocused( &bBrowserWindowHasFocus, &bParentWindowHasFocus );

    if ( hr == S_OK && bBrowserWindowHasFocus )
    {
        *plState |= STATE_SYSTEM_SELECTABLE;

        if ( bParentWindowHasFocus )
        {
            //--------------------------------------------------
            // get the selection (if any) and compare it
            // to this text range.
            //--------------------------------------------------

            if(hr = m_pParent->GetDocumentAO()->IsTextRangeSelected(m_pIHTMLTxtRange,&bSelected))
                return(hr);
            else
            {
                if(bSelected)   
                    *plState |= STATE_SYSTEM_SELECTED;
            }
        }
    }

    return hr;
    
}

//-----------------------------------------------------------------------
//  CTextAE::AccDoDefaultAction()
//
//  DESCRIPTION:
//      executes default action of object.
//  
//  PARAMETERS:
//
//      lChild      child / self ID
//
//  RETURNS:
//
//      HRESULT :   DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT CTextAE::AccDoDefaultAction(long lChild)
{
    //--------------------------------------------------
    // If ancestor is an anchor, delegate to it.
    // Otherwise, use base class implementation.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    HRESULT hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->AccDoDefaultAction( CHILDID_SELF );
    else
        return CTridentAE::AccDoDefaultAction( lChild );
}

//-----------------------------------------------------------------------
//  CTextAE::GetAccDefaultAction()
//
//  DESCRIPTION:
//      returns description string for default action
//  
//  PARAMETERS:
//
//      lChild          child /self ID
//
//      pbstrDefAction  returned description string.
//
//  RETURNS:
//
//      HRESULT :   DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT CTextAE::GetAccDefaultAction(long lChild, BSTR * pbstrDefAction)
{
    //--------------------------------------------------
    // if ancestor is an anchor, delegate to it.
    // Otherwise, use base class implementation.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    HRESULT hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->GetAccDefaultAction( CHILDID_SELF, pbstrDefAction );
    else
        return CTridentAE::GetAccDefaultAction( lChild, pbstrDefAction );
}


//-----------------------------------------------------------------------
//  CTextAE::AccLocation()
//
//  DESCRIPTION:
//      returns location of the specified object
//  
//  PARAMETERS:
//
//      pxLeft      left screen coordinate
//      pyTop       top screen coordinate
//      pcxWidth    screen width of object
//      pcyHeight   screen height of object
//      lChild      child/self ID
//
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTextAE::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
    HRESULT hr;
    
    assert( pxLeft && pyTop && pcxWidth && pcyHeight );

    if( (!pxLeft) || (!pyTop) || (!pcxWidth) || (!pcyHeight) )
        return E_INVALIDARG;
    
    *pxLeft     = 0;
    *pyTop      = 0;
    *pcxWidth   = 0;
    *pcyHeight  = 0;

    //--------------------------------------------------
    // location is determined by getting the metrics from
    // the text range.
    //--------------------------------------------------

    RECT rc;

    hr = getBoundingRect( &rc, TRUE );
    if ( hr != S_OK )
        goto Cleanup;

    //--------------------------------------------------
    // for any text not parented directly by the BODY
    // (document), its location should be contained by
    // its parent AO.
    //--------------------------------------------------

    if ( m_pParent->GetAOMType() != AOMITEM_DOCUMENT )
    {
#ifdef _DEBUG
        BSTR bstrText;
        assert( m_pIHTMLTxtRange->get_text(&bstrText) == S_OK );
        SysFreeString(bstrText);
#endif

        long lLeft = 0;
        long lTop = 0;
        long lWidth = 0;
        long lHeight = 0;

        hr = m_pParent->AccLocation( &lLeft, &lTop, &lWidth, &lHeight, CHILDID_SELF );
        if ( hr == S_OK )
        {
            if ( rc.left < lLeft           || 
                 rc.top < lTop             ||
                 rc.right > lLeft + lWidth ||
                 rc.bottom > lTop + lHeight )
            {
                //--------------------------------------------------
                // set the text's location to the parent's.
                //--------------------------------------------------

                *pxLeft     = lLeft;
                *pyTop      = lTop;
                *pcxWidth   = lWidth;
                *pcyHeight  = lHeight;

                goto Cleanup;
            }
        }
        else
            // if we can't get out parent's location,
            // pretend we never tried
            hr = S_OK;
    }

    //--------------------------------------------------
    // set the outbound params to the adjusted values.
    //--------------------------------------------------

    *pxLeft     = rc.left;
    *pyTop      = rc.top;
    *pcxWidth   = rc.right - rc.left;
    *pcyHeight  = rc.bottom - rc.top;

Cleanup:
    return hr;
}   


//-----------------------------------------------------------------------
//  CTextAE::AccSelect()
//
//  DESCRIPTION:
//      Selects specified object: selection based on flags.  
//
//      NOTE: only the SELFLAG_TAKEFOCUS and SELFLAG_TAKESELECTION
//      are supported.
//  
//  PARAMETERS:
//
//      flagsSel    selection flags : 
//
//          SELFLAG_NONE            = 0,
//          SELFLAG_TAKEFOCUS       = 1,
//          SELFLAG_TAKESELECTION   = 2,
//          SELFLAG_EXTENDSELECTION = 4,
//          SELFLAG_ADDSELECTION    = 8,
//          SELFLAG_REMOVESELECTION = 16
//
//      lChild      child /self ID 
//
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTextAE::AccSelect(long flagsSel, long lChild)
{
    HRESULT hr = S_OK;
    long lState = 0;


    //--------------------------------------------------
    // only SELFLAG_TAKEFOCUS and SELFLAG_TAKESELECTION
    // are supported : screen out all other flags.
    //--------------------------------------------------

    if ((flagsSel & SELFLAG_NONE) || 
        (flagsSel & SELFLAG_EXTENDSELECTION) || 
        (flagsSel & SELFLAG_ADDSELECTION) || 
        (flagsSel & SELFLAG_REMOVESELECTION) )
    {
        return E_INVALIDARG;
    }

    //--------------------------------------------------
    // set focus
    //--------------------------------------------------

    if(flagsSel & SELFLAG_TAKEFOCUS)
    {
        //--------------------------------------------------
        // if ancestor is an anchor, delegate to it.
        //--------------------------------------------------

        CAnchorAO*  pAnc = NULL;

        hr = m_pParent->IsAncestorAnchor( &pAnc );

        if ( hr == S_OK )
            return pAnc->AccSelect( flagsSel, CHILDID_SELF );
        else
        {
            //--------------------------------------------------
            // scroll the text into view, even though we 
            // can't focus it.  This enables screen readers
            // to navigate to text objects that are currently
            // offscreen.  
            //
            // **NOTE** scrollIntoView() with arg of 1 scrolls 
            // text range to top of page. Set arg to 0 to 
            // scroll text range only to bottom of page.
            //--------------------------------------------------

            assert(m_pIHTMLTxtRange);

            if(hr = m_pIHTMLTxtRange->scrollIntoView(1))
                return(hr);
        }

    }

    //--------------------------------------------------
    // select the text range ONLY if the text is 
    // selectable (the document/textarea it is on
    // is currently focused).
    //--------------------------------------------------

    if(flagsSel & SELFLAG_TAKESELECTION)
    {

        if(hr = GetAccState(CHILDID_SELF,&lState))
            return(hr);

        if(lState & STATE_SYSTEM_SELECTABLE)
        {
            if(hr = m_pIHTMLTxtRange->select())
                return(hr);
        }
        else
            return(S_FALSE);
    }

    
    return S_OK;

}


//-----------------------------------------------------------------------
//  CTextAE::GetAccValue()
//
//  DESCRIPTION:
//
//      returns value string to client.
//
//  PARAMETERS:
//
//      lChild      ChildID/SelfID
//
//      pbstrValue  returned value string
//
//  RETURNS:
//
//      HRESULT :   DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT CTextAE::GetAccValue(long lChild, BSTR * pbstrValue)
{
    HRESULT hr;

    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    if(!pbstrValue)
        return(E_INVALIDARG);

    *pbstrValue = NULL;

    assert(m_pParent);

    //--------------------------------------------------
    // if ancestor is an anchor, delegate to it.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->GetAccValue( CHILDID_SELF, pbstrValue );

    //--------------------------------------------------
    // otherwise, we don't support a value.
    //--------------------------------------------------

    return DISP_E_MEMBERNOTFOUND;
}
    

//-----------------------------------------------------------------------
//  CTextAE::GetAccKeyboardShortcut()
//
//  DESCRIPTION:
//
//      Get shortcut string
//
//  PARAMETERS:
//
//      lChild                  child/self ID 
//
//      pbstrKeyboardShortcut       returned string containing kbd shortcut.
//
//  RETURNS:
//
//      HRESULT :   S_OK | DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT CTextAE::GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut )
{
    HRESULT hr;

    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    if ( !pbstrKeyboardShortcut )
        return E_INVALIDARG;

    *pbstrKeyboardShortcut = NULL;


    assert( m_pParent );

    //--------------------------------------------------
    // if ancestor is an anchor, delegate to it.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->GetAccKeyboardShortcut( CHILDID_SELF, pbstrKeyboardShortcut );
    
    //--------------------------------------------------
    // otherwise, we don't have a shortcut.
    //--------------------------------------------------

    return DISP_E_MEMBERNOTFOUND;
}


//================================================================================
// CTextAE public helper methods (non IAccessible)
//================================================================================

//-----------------------------------------------------------------------
//  CTextAE::ContainsPoint()
//
//  DESCRIPTION:
//  
//  determines whether the passed in point is in the text range.
//
//  PARAMETERS:
//
//  xLeft           x coord of point (client coords)
//  yTop            y coord of point (client coords)
//  pIHTMLTxtRange  pointer to text range that has been 
//                  moved to the point.
//
//  RETURNS:
//
//  S_OK if contains, S_FALSE if not, else standard COM error.
// ----------------------------------------------------------------------

HRESULT CTextAE::ContainsPoint(long xLeft,long yTop, IHTMLTxtRange * pIHTMLTxtRange)
{
    HRESULT hr = E_FAIL;
    
    RECT rcClient;
    POINT ptTest;


    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    if(!pIHTMLTxtRange)
    {
        return(E_INVALIDARG);
    }

    //--------------------------------------------------
    // is this point even in our text range ? if not,
    // OR if error, bail.
    //--------------------------------------------------

    if(hr = containsTextRange(pIHTMLTxtRange))
    {
        return(hr);
    }

    //--------------------------------------------------
    // is this point in our bounding rect ? 
    //--------------------------------------------------

    if(hr = getBoundingRect(&rcClient,FALSE))
    {
        return(hr);
    }

    ptTest.x = xLeft;
    ptTest.y = yTop;

    //--------------------------------------------------
    // convert point to screen coords because we are 
    // testing against screen coordinates of text 
    // range rectangle.
    //--------------------------------------------------

    ClientToScreen(m_hWnd,&ptTest);

    if(PtInRect(&rcClient,ptTest))
    {
        return(S_OK);
    }
    else
    {
        return(S_FALSE);
    }

}



//================================================================================
//  CTextAE protected methods implementation
//================================================================================



//-----------------------------------------------------------------------
//  CTextAE::isInClientWindow()
//
//  DESCRIPTION:
//
//  evaluates input unadjusted parmameters : determines whether they
//  are on screen or not.  An object is on screen if any part of it is on screen:
//  it is offscreen if and only if it is completely offscreen.
//
//  PARAMETERS:
//  
//  pxLeft      pointer to left coord IN CLIENT COORDINATES.
//  pyTop       pointer to top coord IN CLIENT COORDINATES.
//  cxWidth     input width.
//  cyHeight    input height.
//
//  RETURNS:
//
//  S_OK if point onscreen, S_FALSE if point offscreen, else std. COM error.
// ----------------------------------------------------------------------

HRESULT CTextAE::isInClientWindow(long xLeft,long yTop, long cxWidth, long cyHeight)
{
    assert(cxWidth > -1);
    assert(cyHeight > -1);


    //--------------------------------------------------
    // assume we are offscreen until proven onscreen.
    //--------------------------------------------------

    m_bOffScreen = TRUE;

    //--------------------------------------------------
    // detect if the point is to the left of/above the
    // client window.
    //--------------------------------------------------
    
    if(xLeft < 0)
    {
        if(xLeft + cxWidth < 0)
        {
            return(S_FALSE);
        }
    }
    
    if(yTop < 0)
    {
        if(yTop + cyHeight < 0)
        {
            return(S_FALSE);
        }
    }

    //--------------------------------------------------
    // detect if the point is to the right of/below
    // the client window.
    //--------------------------------------------------

    RECT rcClient;
    GetClientRect(m_hWnd,&rcClient);
    
    if(xLeft > rcClient.right)
    {
        return(S_FALSE);
    }

    if(yTop > rcClient.bottom)
    {
        return(S_FALSE);
    }

    m_bOffScreen = FALSE;
    return(S_OK);
}


//-----------------------------------------------------------------------
//  CTextAE::getBoundingRect()
//
//  DESCRIPTION:
//  
//  returns the bounding rect, in the client coordinates of the 
//  owner window.  
//
//
//  PARAMETERS:
//
//  pRect       : pointer to rect to fill out w/coordinates.
//  bOrigin     : TRUE gets the top and left coords of the first character in the 
//                range, FALSE gets the  top and left coords of the bounding
//                rect.
//
//  RETURNS:
//
//  S_OK if rect obtained | standard COM error.
// ----------------------------------------------------------------------

HRESULT CTextAE::getBoundingRect(RECT * pRect,BOOL bOrigin)
{
    HRESULT hr = E_FAIL;


    long xLeft      = 0;
    long yTop       = 0;
    long xRight     = 0;    
    long yBottom    = 0;
    
    long xOrigin    = 0;
    long yOrigin    = 0;
    long xBoundingLeft  =   0;
    long yBoundingTop   =   0;
    long cxBoundingWidth = 0;
    long cyBoundingHeight= 0;
    long cxWidth    = 0;
    long cyHeight   = 0;
    
    assert( pRect );

#ifdef _DEBUG

    BSTR bstrText;

    if(hr = m_pIHTMLTxtRange->get_text(&bstrText))
        return(hr);

    SysFreeString(bstrText);

#endif

    //--------------------------------------------------
    // initialize outbound parameter
    //--------------------------------------------------

    pRect->top = pRect->bottom = pRect->right = pRect->left = 0;

    //--------------------------------------------------
    // get the bounds of the text range encapsulated
    // by this CTextAE.
    //--------------------------------------------------

    CComQIPtr<IHTMLTextRangeMetrics,&IID_IHTMLTextRangeMetrics> pIHTMLTextRangeMetrics(m_pIHTMLTxtRange);

    if(!pIHTMLTextRangeMetrics)
        return(E_NOINTERFACE);

    //--------------------------------------------------
    // left x offset 
    //--------------------------------------------------

    if(hr = pIHTMLTextRangeMetrics->get_boundingLeft(&xBoundingLeft) )
        return(hr);

    //--------------------------------------------------
    // left y offset
    //--------------------------------------------------

    if(hr = pIHTMLTextRangeMetrics->get_boundingTop(&yBoundingTop) )
        return(hr);

    
    //--------------------------------------------------
    // get width 
    //--------------------------------------------------

    if(hr = pIHTMLTextRangeMetrics->get_boundingWidth(&cxBoundingWidth) )
        return(hr);

    //--------------------------------------------------
    // get height
    //--------------------------------------------------

    if(hr = pIHTMLTextRangeMetrics->get_boundingHeight(&cyBoundingHeight) )
        return(hr);
    

    //--------------------------------------------------
    // if the width or the height are zero, the text
    // range is not visible.
    //--------------------------------------------------

    if ( cxBoundingWidth == 0 || cyBoundingHeight == 0 )
    {
        m_bOffScreen = TRUE;
        return S_OK;
    }


    //--------------------------------------------------
    // if the origin flag was set, then get the left
    // and top offsets of the start of the text range,
    // and see if they are different than the 
    // bounding left and top offsets and adjust for
    // the difference.
    //--------------------------------------------------

    if(bOrigin)
    {
    
        if(hr = pIHTMLTextRangeMetrics->get_offsetLeft(&xOrigin))
            return(hr);

        //--------------------------------------------------
        // if left origin coord is > than left bounding
        // coord, then set bounding left to origin coord.
        // adjust width accordingly.
        //--------------------------------------------------
        
        if(xOrigin > xBoundingLeft)
        {
            xLeft = xOrigin;
            cxWidth = cxBoundingWidth - (xOrigin - xBoundingLeft);
        }
        else
        {
            xLeft = xBoundingLeft;
            cxWidth = cxBoundingWidth;
        }

    
        if(hr = pIHTMLTextRangeMetrics->get_offsetTop(&yOrigin))
            return(hr);

        //--------------------------------------------------
        // if top origin coord is > than top bounding coord,
        // then set bounding top to origin coord.
        // adjust height accordingly.
        //--------------------------------------------------

        if(yOrigin > yBoundingTop)
        {
            yTop = yOrigin;
            cyHeight = cyBoundingHeight - (yOrigin - yBoundingTop);
        }
        else
        {
            yTop = yBoundingTop;
            cyHeight = cyBoundingHeight;
        }

    }
    else
    {

        //--------------------------------------------------
        // set the xLeft,yTop,cxWidth, and cyHeight to 
        // the bounding coordinates 
        //--------------------------------------------------

        xLeft   = xBoundingLeft;
        yTop    = yBoundingTop;
        cxWidth = cxBoundingWidth;
        cyHeight= cyBoundingHeight;

    }

    if(hr = isInClientWindow(xLeft,yTop,cxWidth,cyHeight))
    {

        //--------------------------------------------------
        // return code of S_FALSE means that the text 
        // is NOT onscreen. 
        //--------------------------------------------------

        if(hr == S_FALSE)
        {

            //--------------------------------------------------
            // if the origin flag has been 
            // set, try to see if the bounding xTop and yLeft
            // are still offscreen.
            //--------------------------------------------------

            if(bOrigin)
            {

                //--------------------------------------------------
                // if the bounding rect is in the window, continue 
                // on as before.
                //--------------------------------------------------

                if(hr = isInClientWindow(xBoundingLeft,yBoundingTop,cxBoundingWidth,cyBoundingHeight))
                {
                    if(hr != S_FALSE)
                        return(hr);
                }
            }
            else
            {
                return(S_OK);
            }
        }
        else
        {
            return(hr);
        }
    }

    //--------------------------------------------------
    // isInClientWindow() set the offscreen state.
    // Now convert the xLeft and yTop to screen
    // coordinates and re-evaluate width and height.
    //--------------------------------------------------

    POINT ptScreen;

    ptScreen.x = xLeft;
    ptScreen.y = yTop;

    ClientToScreen(m_hWnd,&ptScreen);

    xLeft = ptScreen.x;
    yTop = ptScreen.y;

    //--------------------------------------------------
    // assign rect members.
    //--------------------------------------------------

    pRect->left     = xLeft;
    pRect->top      = yTop;
    pRect->right    = xLeft + cxWidth;
    pRect->bottom   = yTop + cyHeight;

    return(S_OK);
}


//-----------------------------------------------------------------------
//  CTextAE::containsTextRange()
//
//  DESCRIPTION:
//
//  checks to see if the text range associated with this textAO contains
//  the input text range..
//
//  PARAMETERS:
//
//  pDocTxtRange    input text range to check against.
//
//  RETURNS:
//
//  S_OK if the obj. contains the text range, S_FALS if not, else
//  standard COM error.
// ----------------------------------------------------------------------


HRESULT CTextAE::containsTextRange(IHTMLTxtRange *pDocTxtRange)
{
    HRESULT hr = E_FAIL;
    short sInRange = 0;

    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    assert( pDocTxtRange );
        
    assert(m_pIHTMLTxtRange);


#ifdef _DEBUG

    BSTR bstrText;

    if(hr = m_pIHTMLTxtRange->get_text(&bstrText))
        return(hr);

    SysFreeString(bstrText);

#endif


    //--------------------------------------------------
    // Is the input range in the internal text range ?
    //--------------------------------------------------

    if(hr = m_pIHTMLTxtRange->inRange( pDocTxtRange,&sInRange ))
        return(hr);

    if(sInRange)
    {

        //--------------------------------------------------
        // yes it is
        //--------------------------------------------------

        return(S_OK);
    }
    else
    {

        //--------------------------------------------------
        // no its not.
        //--------------------------------------------------

        return(S_FALSE);
    }


}


//----  End of TEXT.CPP  ----
