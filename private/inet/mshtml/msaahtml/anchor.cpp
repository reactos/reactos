//================================================================================
//      File:   ANCHOR.CPP
//      Date:   7/1/97
//      Desc:   contains implementation of CAnchorAO class.  CAnchorAO 
//              implements the accessible proxy for the Trident Anchor 
//              object.
//
//      Author: Arunj
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "anchor.h"
#include "document.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif

#ifdef _MSAA_EVENTS

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CAnchorAO,ImplIHTMLAnchorEvents,CEvent)

    ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
    ON_DISPID_FIRE_EVENT(DISPID_HTMLANCHOREVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
    ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
    ON_DISPID_FIRE_EVENT(DISPID_HTMLANCHOREVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
         
END_EVENT_HANDLER_MAP()

#endif



//================================================================================
// CAnchorAO : public methods
//================================================================================

//-----------------------------------------------------------------------
//  CAnchorAO::CAnchorAO()
//
//  DESCRIPTION:
//
//      constructor
//
//  PARAMETERS:
//
//      pAOParent           pointer to the parent accessible object in 
//                          the AOM tree
//
//      nTOMIndex           index of the element from the TOM document.all 
//                          collection.
//      
//      hWnd                pointer to the window of the trident object that 
//                          this object corresponds to.
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CAnchorAO::CAnchorAO(CTridentAO * pAOParent,CDocumentAO * pAODoc,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,pAODoc,nTOMIndex,nChildID,hWnd)
{
    //------------------------------------------------
    // assign the delegating IUnknown to CAnchorAO :
    // this member will be overridden in derived class
    // constructors so that the delegating IUnknown 
    // will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown     = (IUnknown *)this;                                 

    //--------------------------------------------------
    // set the role parameter to be used
    // in the default CAccElement implementation.
    //--------------------------------------------------

    m_lRole = ROLE_SYSTEM_LINK;
    

    //--------------------------------------------------
    // set the item type so that it can be accessed
    // via base class pointer.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_ANCHOR;


#ifdef _DEBUG

    //--------------------------------------------------
    // set this string for debugging use
    //--------------------------------------------------
    lstrcpy(m_szAOMName,_T("AnchorAO"));

#endif

    
}


//-----------------------------------------------------------------------
//  CAnchorAO::~CAnchorAO()
//
//  DESCRIPTION:
//
//      CAnchorAO class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.                      
//
// ----------------------------------------------------------------------

CAnchorAO::~CAnchorAO()
{

}       


//-----------------------------------------------------------------------
//  CAnchorAO::Init()
//
//  DESCRIPTION:
//
//      Initialization : set values of data members
//
//  PARAMETERS:
//
//      pTOMObjIUnk     pointer to IUnknown of TOM object.
//
//  RETURNS:
//
//      S_OK | E_FAIL | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CAnchorAO::Init(IUnknown * pTOMObjIUnk)
{
    HRESULT hr  = E_FAIL;

    assert( pTOMObjIUnk );

    //--------------------------------------------------
    // call down to base class to set unknown pointer.
    //--------------------------------------------------

    hr = CTridentAO::Init(pTOMObjIUnk);


#ifdef _MSAA_EVENTS

    if ( hr == S_OK )
    {
        HRESULT hrEventInit;

        //--------------------------------------------------
        // initialize event handling interface : establish
        // Advise.
        //--------------------------------------------------
                
        hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLAnchorEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

#ifdef _DEBUG
        //--------------------------------------------------
        //  If we are in debug mode, assert any
        //  event handler initialization errors.
        //
        //  (In release mode, just ignore.  This will allow
        //  the object to be created, it just won't support
        //  events.)
        //--------------------------------------------------

        assert( hrEventInit == S_OK );

        if ( hrEventInit != S_OK )
            OutputDebugString( "Event handler initialization in CAnchorAO::Init() failed.\n" );
#endif

    }

#endif  // _MSAA_EVENTS


    return hr;
}


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//  CAnchorAO::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CAnchorAO object only implements
//      IUnknown.
//
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------

STDMETHODIMP CAnchorAO::QueryInterface( REFIID riid, void** ppv )
{
    if ( !ppv )
        return E_INVALIDARG;


    *ppv = NULL;

    if (riid == IID_IUnknown)  
    {
        *ppv = (IUnknown *)this;
    }

#ifdef _MSAA_EVENTS 
    
    else if (riid == DIID_HTMLAnchorEvents)
    {
        //--------------------------------------------------
        // this is the event interface for the button class.
        //--------------------------------------------------

        ASSIGN_TO_EVENT_HANDLER(ImplIHTMLAnchorEvents,ppv,HTMLAnchorEvents)
    } 
    
#endif

    else
    {
        //--------------------------------------------------
        // delegate to base class
        //--------------------------------------------------

        return CTridentAO::QueryInterface( riid, ppv );
    }
    

    //--------------------------------------------------
    // *ppv should be set.
    //--------------------------------------------------

    if ( !*ppv )
        return E_NOINTERFACE;


    ((LPUNKNOWN) *ppv)->AddRef();

    return NOERROR;
}


//================================================================================
// CAnchorAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//  CAnchorAO::GetAccName()
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
// ----------------------------------------------------------------------

HRESULT CAnchorAO::GetAccName(long lChild, BSTR * pbstrName)
{
    HRESULT hr = S_OK;
    
    
    assert( pbstrName );


    if ( !m_bNameAndDescriptionResolved )
    {
        hr = resolveNameAndDescription();

        if ( hr != S_OK )
            return hr;
    }

    if ( m_bstrName )
        *pbstrName = SysAllocString( m_bstrName );
    else
        hr = DISP_E_MEMBERNOTFOUND;


    return hr;
}



//-----------------------------------------------------------------------
//  CAnchorAO::GetAccDescription()
//
//  DESCRIPTION:
//
//      returns description of the anchor object.
//
//  PARAMETERS:
//      
//      lChild              ChildID
//      pbstrDescription    string to store value in
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CAnchorAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
    HRESULT hr = S_OK;
    
    
    assert( pbstrDescription );


    if ( !m_bNameAndDescriptionResolved )
    {
        hr = resolveNameAndDescription();

        if ( hr != S_OK )
            return hr;
    }

    if ( m_bstrDescription )
        *pbstrDescription = SysAllocString( m_bstrDescription );
    else
        hr = DISP_E_MEMBERNOTFOUND;


    return hr;
}


//-----------------------------------------------------------------------
//  CAnchorAO::GetAccValue()
//
//  DESCRIPTION:
//
//      returns value of image object.
//
//  PARAMETERS:
//      
//      lChild      ChildID
//      pbstrValue  string to store value in
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CAnchorAO::GetAccValue( long lChild, BSTR* pbstrValue )
{
    //--------------------------------------------------
    //  Validate the parameters
    //--------------------------------------------------

    assert( pbstrValue );

    if ( !pbstrValue )
        return E_INVALIDARG;
        
        
    //------------------------------------------------
    //  The A's HREF will be used as the
    //  CAnchorAO's accValue.
    //
    //  If the href has already been retrieved, use
    //  cached value; otherwise, get the A's href.
    //------------------------------------------------

    if ( m_bstrValue )
    {
        *pbstrValue = SysAllocString( m_bstrValue );
        goto success;
    }
    else
    {
        CComQIPtr<IHTMLAnchorElement,&IID_IHTMLAnchorElement> pIHTMLAnchorElement(m_pTOMObjIUnk);

        if ( pIHTMLAnchorElement )
        {
            HRESULT  hr;

            hr = pIHTMLAnchorElement->get_href( pbstrValue );

            if ( hr == S_OK )
            {
                if ( *pbstrValue )
                {
                    m_bstrValue = SysAllocString( *pbstrValue );
                    goto success;
                }
            }
        }
    }


    return DISP_E_MEMBERNOTFOUND;

success:
    return S_OK;
}


//-----------------------------------------------------------------------
//  CAnchorAO::GetAccState()
//
//  DESCRIPTION:
//
//      state of image depends on whether it is completely downloaded or 
//      not.
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
// ----------------------------------------------------------------------
    
HRESULT CAnchorAO::GetAccState(long lChild, long *plState)
{
    HRESULT hr      = E_FAIL;
    long ltempState = 0;


    assert( plState );

    //--------------------------------------------------
    // Call down to the base class to determine
    // visibility.
    //--------------------------------------------------

    hr = CTridentAO::GetAccState( lChild, &ltempState );


    //--------------------------------------------------
    // anchor state always (at a minimum)
    // STATE_SYSTEM_LINKED
    //--------------------------------------------------

    ltempState |= STATE_SYSTEM_LINKED;


    //--------------------------------------------------
    // Update the location to determine offscreen status
    //--------------------------------------------------

    long lDummy;

    hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

    if (SUCCEEDED( hr ) && m_bOffScreen)
        ltempState |= STATE_SYSTEM_INVISIBLE;


    //--------------------------------------------------
    //  The CAnchorAO is focusable only if the Trident
    //  document or one of its children has the focus.
    //--------------------------------------------------

    UINT uTOMID = 0;

    hr = GetFocusedTOMElementIndex( &uTOMID );
    
    if ( hr == S_OK  &&  uTOMID > 0 )
    {
        ltempState |= STATE_SYSTEM_FOCUSABLE;

        //--------------------------------------------------
        //  If IDs match, then this anchor has the focus.
        //--------------------------------------------------

        if ( uTOMID == m_nTOMIndex )
            ltempState |= STATE_SYSTEM_FOCUSED;
    }
    else
    {
        //--------------------------------------------------
        //  GetFocusedTOMElementIndex() returned something
        //  other than S_OK meaning that the source index
        //  of the focused TEO could not be obtained.
        //  So, just assume that there is no active element,
        //  which means that our CAnchorAO can neither be
        //  FOCUSABLE nor FOCUSED.
        //--------------------------------------------------

        hr = S_OK;
    }


    *plState = ltempState;


    return hr;
}



//-----------------------------------------------------------------------
//  CAnchorAO::GetAccKeyboardShortcut()
//
//  DESCRIPTION:
//
//      return specified keyboard accelerator
//
//  PARAMETERS:
//      
//      lChild                  ChildID
//      pbstrKeyboardShortcut   BSTR to return shortcut in
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CAnchorAO::GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut )
{
    //--------------------------------------------------
    //  Validate the parameters
    //--------------------------------------------------

    assert( pbstrKeyboardShortcut );

    if ( !pbstrKeyboardShortcut )
        return E_INVALIDARG;
        
        
    //------------------------------------------------
    //  The A's ACCESSKEY will be used as the
    //  CAnchorAO's accKeyboardShortcut.
    //
    //  If the accesskey has already been retrieved,
    //  use cached value; otherwise, get it.
    //------------------------------------------------

    if ( m_bstrKbdShortcut )
    {
        *pbstrKeyboardShortcut = SysAllocString( m_bstrKbdShortcut );
        goto success;
    }
    else
    {
        CComQIPtr<IHTMLAnchorElement,&IID_IHTMLAnchorElement> pIHTMLAnchorElement(m_pTOMObjIUnk);

        if ( pIHTMLAnchorElement )
        {
            HRESULT hr;

            hr = pIHTMLAnchorElement->get_accessKey( pbstrKeyboardShortcut );

            if ( hr == S_OK )
            {
                if ( *pbstrKeyboardShortcut )
                {
                    m_bstrKbdShortcut = SysAllocString( *pbstrKeyboardShortcut );
                    goto success;
                }
            }
        }
    }

    return DISP_E_MEMBERNOTFOUND;

success:
    return S_OK;
}


//-----------------------------------------------------------------------
//  CAnchorAO::GetAccDefaultAction()
//
//  DESCRIPTION:
//
//      return hardcoded string 'Jump'
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
// ----------------------------------------------------------------------
    
HRESULT CAnchorAO::GetAccDefaultAction( long lChild, BSTR* pbstrDefAction )
{
    //--------------------------------------------------
    //  Validate the parameters
    //--------------------------------------------------

    assert( pbstrDefAction );

    if ( !pbstrDefAction )
        return E_INVALIDARG;
    else
        return GetResourceStringValue(IDS_JUMP_ACTION, pbstrDefAction);
}

//-----------------------------------------------------------------------
//  CAnchorAO::AccSelect()
//
//  DESCRIPTION:
//
//  scroll into view and focus anchor
//
//  PARAMETERS:
//      
//      flagsSel    selection flags to apply
//      lChild      ChildID
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CAnchorAO::AccSelect( long flagsSel, long lChild )
{
    HRESULT hr = S_OK;


    //--------------------------------------------------
    // only SELFLAG_TAKEFOCUS is supported.
    //--------------------------------------------------

    if ( !(flagsSel & SELFLAG_TAKEFOCUS) )
        return E_INVALIDARG;

    //--------------------------------------------------
    // set focus to owner window first to ensure that
    // keyboard focus is set to this anchor.
    //--------------------------------------------------

    assert(m_pDocAO);

    if(hr = m_pDocAO->GetParent()->AccSelect(SELFLAG_TAKEFOCUS,0))
        return(hr);

    //--------------------------------------------------
    // set focus
    //--------------------------------------------------

    CComQIPtr<IHTMLAnchorElement,&IID_IHTMLAnchorElement> pIHTMLAnchorElement(m_pTOMObjIUnk);

    if ( !pIHTMLAnchorElement )
        hr = E_NOINTERFACE;
    else
        hr = pIHTMLAnchorElement->focus();

    return hr;
}



//-----------------------------------------------------------------------
//  CAnchorAO::AccDoDefaultAction()
//
//  DESCRIPTION:
//
//  scroll into view, focus, click
//
//  PARAMETERS:
//      
//      lChild      ChildID
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CAnchorAO::AccDoDefaultAction( long lChild )
{
    HRESULT hr = E_FAIL;


    //--------------------------------------------------
    // select the anchor before acting on it.
    //--------------------------------------------------

    hr = AccSelect( SELFLAG_TAKEFOCUS, lChild );

    if ( hr == S_OK )
    {
        //--------------------------------------------------
        // click the anchor to invoke its default action.
        //
        // TODO: evaluate whether the clicking should
        // be moved into a base method also.  Right now
        // AnchorAO is the only AO that gets 'clicked'.
        // that may change in the future.
        //--------------------------------------------------

        CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

        if ( !pIHTMLElement )
            hr = E_NOINTERFACE;
        else
            hr = pIHTMLElement->click();
    }

    return hr;
}



//================================================================================
//  CAnchorAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//  CAnchorAO::resolveNameAndDescription()
//
//  DESCRIPTION:
//
//      Determines the accName and accDescription properties.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      HRESULT
//
//  NOTES:
//
//      This method overrides the method defined in the base class,
//      CTridentAO.
//
//      For the anchor, <A>.INNERTEXT is the accName, <A>.TITLE is
//      the accDescription.
//
//  BUGBUG:
//
//      This method follows the prevalent MSAAHTML object method
//      theme: "if error, fail."  Maybe we should be a bit more
//      friendly and try "if error, bail not fail."
//-----------------------------------------------------------------------

HRESULT CAnchorAO::resolveNameAndDescription( void )
{
    HRESULT hr;


    //------------------------------------------------
    //  Initialize affected member variables.
    //------------------------------------------------

    m_bNameAndDescriptionResolved = FALSE;

    if ( m_bstrName )
    {
        SysFreeString( m_bstrName );
        m_bstrName = NULL;
    }

    if ( m_bstrDescription )
    {
        SysFreeString( m_bstrDescription );
        m_bstrDescription = NULL;
    }

    //------------------------------------------------
    //  Set the CAnchorAO's accName by calling
    //  CTridentAO::getDescriptionString() to get
    //  the <A>'s INNERTEXT.
    //------------------------------------------------

    hr = getDescriptionString( &m_bstrName );

    if ( hr == S_OK )
    {
        //------------------------------------------------
        //  If the CAnchorAO now has a non-empty accName,
        //  set its accDescription using the <A>'s TITLE.
        //  Otherwise, use the <A>'s TITLE as the accName
        //  and leave the accDescription empty.
        //------------------------------------------------

        if ( m_bstrName )
            hr = getTitleFromIHTMLElement( &m_bstrDescription );
        else
            hr = getTitleFromIHTMLElement( &m_bstrName );

        if ( hr == S_OK )
            m_bNameAndDescriptionResolved = TRUE;
    }

    //------------------------------------------------
    //  Cleanup on error.
    //------------------------------------------------

    if ( hr != S_OK )
    {
        if ( m_bstrName )
        {
            SysFreeString( m_bstrName );
            m_bstrName = NULL;
        }

        if ( m_bstrDescription )
        {
            SysFreeString( m_bstrDescription );
            m_bstrDescription = NULL;
        }
    }

    return hr;
}


//----  End of ANCHOR.CPP  ----