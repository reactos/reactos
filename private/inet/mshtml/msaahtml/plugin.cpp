//================================================================================
//      File:   PLUGIN.CPP
//      Date:   7/9/97
//      Desc:   contains implementation of CPluginAO class.  CPluginAO implements
//              the Accessible Object proxy for an HTML Plugin element.
//      
//      Notes:  this is a unique AO implementation : a CPluginAO can wrap an 
//      embedded object which implements IAccessible itself.  In this case, 
//      we wrap all of the embedded object's accessible methods with CPluginAO methods
//      and delegate to the embedded object. We override those methods that enable
//      us to 'tie in' the embedded object to our AOM hierarchy.
//              
//
//      Author: Arunj
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "plugin.h"
#include "oleacapi.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif

//================================================================================
// globals
//================================================================================

enum _readystate
{
    IUNINIT = 1,
    ILOADING,
    IINTERACTIVE,
    ICOMPLETE
};



//================================================================================
// CPluginAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//  CPluginAO::CPluginAO()
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
//      nChildID            unique Child ID
//
//      hWnd                pointer to the window of the trident object that 
//                          this object corresponds to.
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CPluginAO::CPluginAO( CTridentAO* pAOMParent,
                      CDocumentAO* pDocAO,
                      UINT nTOMIndex,
                      UINT nChildID,
                      HWND hWnd )
: CTridentAO(pAOMParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
    //------------------------------------------------
    // assign the delegating IUnknown to CPluginAO :
    // this member will be overridden in derived class
    // constructors so that the delegating IUnknown 
    // will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown     = (IUnknown *)this;
    
    m_iPluginType = 0;

    m_pIAccessible = NULL;

    m_bCheckedForHWND = FALSE;

    m_lRole = ROLE_SYSTEM_CLIENT;

    //--------------------------------------------------
    // set the item type so that it can be accessed
    // via base class pointer.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_PLUGIN;


#ifdef _DEBUG

    lstrcpy( m_szAOMName, _T("PluginAO") );

#endif
}



//-----------------------------------------------------------------------
//  CPluginAO::~CPluginAO()
//
//  DESCRIPTION:
//
//      CPluginAO class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CPluginAO::~CPluginAO()
{
    //--------------------------------------------------
    // release IAccessible pointer of embedded object 
    // if it was allocated.
    //--------------------------------------------------

    if(m_pIAccessible)
        m_pIAccessible->Release();
}


//-----------------------------------------------------------------------
//  CPluginAO::Init()
//
//  DESCRIPTION:
//
//      Initialization : set values of data members
//
//  PARAMETERS:
//
//      pTOMObjUnk      pointer to IUnknown of TOM object.
//
//  RETURNS:
//
//      S_OK | E_FAIL
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::Init( IUnknown* pTOMObjIUnk )
{
    HRESULT hr;
    BSTR    bstrTag;


    assert( pTOMObjIUnk );

    if ( !pTOMObjIUnk )
        return E_INVALIDARG;

    //--------------------------------------------------
    // call down to base class to set unknown pointer.
    //--------------------------------------------------

    hr = CTridentAO::Init( pTOMObjIUnk );
    
    if ( hr != S_OK )
        return hr;

    //--------------------------------------------------
    // determine what type of plugin is being created.
    // this information will be used in several post 
    // init methods if the embedded object itself neither
    // supports IAccessible nor hosts an HWND.
    //--------------------------------------------------

    CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLEl(m_pTOMObjIUnk);

    if ( !pIHTMLEl )
        return E_NOINTERFACE;

    hr = pIHTMLEl->get_tagName( &bstrTag );

    if ( hr == S_OK )
    {
        if ( !_wcsicmp( bstrTag, L"EMBED") )
            m_iPluginType = IEMBED;
        else if ( !_wcsicmp( bstrTag, L"APPLET" ) )
            m_iPluginType = IAPPLET;
        else if ( !_wcsicmp( bstrTag, L"OBJECT" ) )
            m_iPluginType = IOBJECT;
        else
        {
            assert( 0 && "Unexpected plugin type!" );

            hr = E_UNEXPECTED;
        }
    }

    SysFreeString( bstrTag );

    if ( hr )
        return hr;

    //--------------------------------------------------
    //  Does this plugin wrap an embedded object that
    //  supports IAccessible?  If so, we are done.
    //--------------------------------------------------

    hr = doesPluginSupportIAccessible();


    if ( hr != S_OK )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );
        if ( hr == S_FALSE )
            hr = S_OK;
    }
    

    return hr;
}


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//  CPluginAO::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CPluginAO object delegates to
//      its internal IAccessible.
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

STDMETHODIMP CPluginAO::QueryInterface( REFIID riid, void** ppv )
{
    HRESULT hr;

    if ( !ppv )
        return E_INVALIDARG;


    *ppv = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
            {
                hr = E_NOINTERFACE;
                goto Cleanup;
            }
        }
    }


    //--------------------------------------------------
    // delegate the handling of IEnumVARIANT to the
    // inner object
    //--------------------------------------------------

    if ( riid == IID_IEnumVARIANT )
    {
        if ( m_pIAccessible )
            hr = m_pIAccessible->QueryInterface( riid, ppv );
        else
            hr = E_NOINTERFACE;
    }

    //--------------------------------------------------
    // let the base AO class handle the rest 
    //--------------------------------------------------

    else
    {
        hr = CTridentAO::QueryInterface( riid, ppv );
    }


/***
    // if we actually have an inner object, delegate to it
    if ( m_pIAccessible )
    {
        hr = m_pIAccessible->QueryInterface( riid, ppv );
    }
    else if ( riid == IID_IEnumVARIANT && !scrolledIntoViewAlready() )
    {
        // we haven't been scrolled into view yet which
        // means we don't know if we've got an HWND so
        // don't allow acces to this interface..
        //--------------------------------------------------
        hr = E_NOINTERFACE;
    }
    else
    {
        // let the base AO class try the rest 
        //--------------------------------------------------
        hr = CTridentAO::QueryInterface( riid, ppv );
    }
***/


Cleanup:
    return hr;
}



//================================================================================
// CPluginAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//  CPluginAO::GetAccName()
//
//  DESCRIPTION:
//  
//      returns accessible name to client.  
///     This method either wraps embedded AO call, or returns plugin specific 
//      information.
//
//  PARAMETERS:
//
//      lChild          child ID
//      pbstrName       pointer to array to return child name in.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccName( long lChild, BSTR* pbstrName )
{
    HRESULT hr = S_OK;

    
    assert( pbstrName );

    if ( !pbstrName )
        return E_INVALIDARG;

    *pbstrName = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );
        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate if we have an inner object.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );
            
        if ( hr != S_OK )
            goto Cleanup;

        hr = m_pIAccessible->get_accName( varID, pbstrName );
    }
    else
    {
        //------------------------------------------------
        //  The TEO's title will be used as the
        //  CPluginAO's accName.
        //
        //  If the title has already been retrieved
        //  from the TEO, use cached value; otherwise,
        //  get the TEO's title.
        //------------------------------------------------

        if ( m_bstrName )
            *pbstrName = SysAllocString( m_bstrName );
        else
        {
            hr = getTitleFromIHTMLElement( pbstrName );

            if ( hr == S_OK )
            {
                if ( !*pbstrName )
                    hr = DISP_E_MEMBERNOTFOUND;
                else
                    m_bstrName = SysAllocString( *pbstrName );
            }
        }
    }


Cleanup:            
    return hr;
}



//-----------------------------------------------------------------------
//  CPluginAO::GetAccValue(long lChild, BSTR * pbstrValue)
//
//  DESCRIPTION:
//
//      returns value string to client.
//      This method either wraps embedded AO call, or returns plugin specific 
//      information.
//
//  PARAMETERS:
//
//      lChild      ChildID/SelfID
//
//      pbstrValue  returned value string
//
//  RETURNS:
//
//      HRESULT :   S_OK if success, S_FALSE if fail, DISP_E_MEMBERNOTFOUND
//                  for no implement.
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccValue( long lChild, BSTR* pbstrValue )
{
    HRESULT hr = S_OK;


    assert( pbstrValue );
    
    if ( !pbstrValue )
        return E_INVALIDARG;

    *pbstrValue = NULL;
        
    
    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );
        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );
            
        if ( hr != S_OK ) 
            goto Cleanup;

        hr = m_pIAccessible->get_accValue( varID, pbstrValue );
    }
    else
    {
        //--------------------------------------------------
        //  accValue not supported for APPLETs or OBJECTs
        //--------------------------------------------------

        if ( m_iPluginType != IEMBED )
            hr = DISP_E_MEMBERNOTFOUND;
        else
        {
            //------------------------------------------------
            //  The EMBED's src will be used as the
            //  CPluginAO's accValue.
            //
            //  If the src has already been retrieved from
            //  the EMBED, use the cached value; otherwise,
            //  get the EMBED's src.
            //------------------------------------------------

            if ( m_bstrValue )
                *pbstrValue = SysAllocString( m_bstrValue );
            else
            {
                CComQIPtr<IHTMLEmbedElement,&IID_IHTMLEmbedElement> pIHTMLEmbedEl(m_pTOMObjIUnk);

                if( !pIHTMLEmbedEl )
                {
                    hr = E_NOINTERFACE;
                    goto Cleanup;
                }

                hr = pIHTMLEmbedEl->get_src( pbstrValue );

                if ( hr == S_OK )
                {
                    if ( !*pbstrValue )
                        hr = DISP_E_MEMBERNOTFOUND;
                    else
                        m_bstrValue = SysAllocString( *pbstrValue );
                }
            }
        }
    }

Cleanup:
    return hr;
}



//-----------------------------------------------------------------------
//  CPluginAO::GetAccDescription()
//
//  DESCRIPTION:
//
//      returns description string to client.
//      This method either wraps embedded AO call, or
//      returns plugin specific information.
//
//  PARAMETERS:
//
//      lChild              Child/Self ID
//
//      pbstrDescription        Description string returned to client.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccDescription( long lChild, BSTR* pbstrDescription )
{
    HRESULT hr = S_OK;

    
    assert( pbstrDescription );
    
    if ( !pbstrDescription )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );
        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );
            
        if ( hr != S_OK )
            goto Cleanup;

        hr = m_pIAccessible->get_accDescription( varID, pbstrDescription );
    }
    else
    {
        switch ( m_iPluginType )
        {
            case IEMBED:
                hr = GetResourceStringValue(IDS_EMBED_DESCRIPTION, pbstrDescription);
                break;
            case IAPPLET:
                hr = GetResourceStringValue(IDS_APPLET_DESCRIPTION, pbstrDescription);
                break;
            case IOBJECT:
                hr = GetResourceStringValue(IDS_OBJECT_DESCRIPTION, pbstrDescription);
                break;
        }
    }

Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccState()
//
//  DESCRIPTION:
//
//      returns state of plugin which can include FOCUSABLE and FOCUSED.
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
    
HRESULT CPluginAO::GetAccState( long lChild, long* plState )
{
    HRESULT hr = S_OK;

    
    assert( plState );

    if ( !plState )
        return E_INVALIDARG;

    
    *plState = STATE_SYSTEM_UNAVAILABLE;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );
        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;
        VARIANT varState;

        hr = loadVariant( &varID, lChild );
        
        if ( hr != S_OK ) 
            goto Cleanup;

        hr = m_pIAccessible->get_accState( varID, &varState );

        if ( hr == S_OK )
            hr = unpackVariant( varState, plState );
    }

    else
    {
        //--------------------------------------------------
        //  If html object is still out of view, set the
        //  state to unavailable and invisible.
        //--------------------------------------------------

        if ( !scrolledIntoViewAlready() )
        {
            *plState |= STATE_SYSTEM_INVISIBLE;
            goto Cleanup;
        }


        //--------------------------------------------------
        //  If the associated TEO is an APPLET or OBJECT,
        //  make sure that it is ready (i.e., fully
        //  downloaded and initialized).
        //--------------------------------------------------

        if ( m_iPluginType == IAPPLET  ||  m_iPluginType == IOBJECT )
        {
            long lReadyState = 0;

            CComQIPtr<IHTMLObjectElement,&IID_IHTMLObjectElement> pIHTMLObjectEl(m_pTOMObjIUnk);

            if ( !pIHTMLObjectEl )
            {
                hr = E_NOINTERFACE;
                goto Cleanup;
            }

            hr = pIHTMLObjectEl->get_readyState( &lReadyState );

            if ( hr != S_OK ||
                 lReadyState == IUNINIT ||
                 lReadyState == ILOADING )
                goto Cleanup;
        }


        //--------------------------------------------------
        // Call down to the base class to determine
        // visibility.
        //--------------------------------------------------

        long ltempState = 0;

        hr = CTridentAO::GetAccState( lChild, &ltempState );


        //--------------------------------------------
        // Update the location to determine offscreen status
        //--------------------------------------------------

        long lDummy;

        hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

        if (SUCCEEDED( hr ) && m_bOffScreen)
            ltempState |= STATE_SYSTEM_INVISIBLE;


        //--------------------------------------------------
        //  If there is a focused element on the page,
        //  that means that this element is focusable
        //--------------------------------------------------

        UINT uTOMID = 0;

        hr = GetFocusedTOMElementIndex( &uTOMID );
    
        //--------------------------------------------------
        //  The CPluginAO is focusable only if the Trident
        //  document or one of its children has the focus.
        //--------------------------------------------------

        if ( hr == S_OK  &&  uTOMID > 0 )
        {
            ltempState |= STATE_SYSTEM_FOCUSABLE;

            //--------------------------------------------------
            //  If IDs match, then this element has the focus.
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
            //  which means that our CPluginAO can neither be
            //  FOCUSABLE nor FOCUSED.
            //--------------------------------------------------

            hr = S_OK;
        }
    
        *plState = ltempState;
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccDefaultAction()
//
//  DESCRIPTION:
//      returns description string for default action
//      This method either wraps embedded AO call, or returns plugin specific 
//      information.
//  
//  PARAMETERS:
//
//      lChild          child /self ID
//
//      pbstrDefAction  returned description string.
//
//  RETURNS:
//
//      HRESULT :   S_OK if success, E_FAIL if fail, DISP_E_MEMBERNOTFOUND
//                  for no implement.
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccDefaultAction( long lChild, BSTR* pbstrDefAction )
{
    HRESULT hr = S_OK;


    assert( pbstrDefAction );

    if ( !pbstrDefAction )
        return E_INVALIDARG;

    
    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );
        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );
            
        if ( hr != S_OK )
            goto Cleanup;

        hr = m_pIAccessible->get_accDefaultAction( varID, pbstrDefAction );
    }
    else
    {
        hr = GetResourceStringValue(IDS_SELECT_ACTION, pbstrDefAction);
    }

Cleanup:
    return hr;
}



//-----------------------------------------------------------------------
//  CPluginAO::AccDoDefaultAction()
//
//  DESCRIPTION:
//      selects plugin.
//      This method either wraps embedded AO call, or returns plugin specific 
//      information.
//      
//  PARAMETERS:
//
//      lChild      child / self ID
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::AccDoDefaultAction( long lChild )
{
    HRESULT hr;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );
        
        if ( hr == S_OK )
            hr = m_pIAccessible->accDoDefaultAction( varID );
    }
    else
    {
        //--------------------------------------------------
        //  Delegate to internal method that scrolls control
        //  into view and places focus on control.
        //--------------------------------------------------

        hr = AccSelect( SELFLAG_TAKEFOCUS, lChild );
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccChildCount()
//
//  DESCRIPTION:
//      returns child count to client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      pChildCount pointer to long var to fill out w/child count.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccChildCount( long* pChildCount )
{
    HRESULT hr = S_OK;


    assert( pChildCount );

    if ( !pChildCount )
        return E_INVALIDARG;

    *pChildCount = 0;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
        hr = m_pIAccessible->get_accChildCount( pChildCount );

    // else just return

Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccChild()
//
//  DESCRIPTION:
//      returns IDispatch * of requested child to client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      ppdispChild IDispatch pointer to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccChild( long lChild, IDispatch** ppdispChild )
{
    HRESULT hr = S_OK;


    assert( ppdispChild );

    if ( !ppdispChild )
        return E_INVALIDARG;

    *ppdispChild = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );

        if( hr == S_OK )
            hr = m_pIAccessible->get_accChild( varID, ppdispChild );
    }

    //--------------------------------------------------
    //  Otherwise, no children.
    //--------------------------------------------------

    else
        hr = S_FALSE;


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccHelp()
//
//  DESCRIPTION:
//      returns help string to client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      lChild      child ID
//      pbstrHelp   help string to return to caller..
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccHelp(long lChild, BSTR * pbstrHelp)
{
    HRESULT hr = S_OK;


    assert( pbstrHelp );

    if ( !pbstrHelp )
        return E_INVALIDARG;

    *pbstrHelp = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;
        
        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->get_accHelp( varID, pbstrHelp );
    }

    //--------------------------------------------------
    //  Otherwise, just return.
    //--------------------------------------------------


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccHelpTopic()
//
//  DESCRIPTION:
//      returns help file name and topic to client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      pbstrHelpFile   help string to return to caller..
//      lChild          child ID
//      pidTopic        long var to return to caller.
//      
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic)
{
    HRESULT hr = S_OK;


    assert ( pbstrHelpFile );
    assert ( pidTopic );

    if ( !pbstrHelpFile  ||  !pidTopic )
        return E_INVALIDARG;

    *pbstrHelpFile = NULL;
    *pidTopic = 0;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->get_accHelpTopic( pbstrHelpFile, varID, pidTopic );
    }

    //--------------------------------------------------
    //  Otherwise, just return.
    //--------------------------------------------------


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::AccLocation()
//
//  DESCRIPTION:
//      returns location of app to client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      pxLeft
//      pyTop
//      pcxWidth
//      pcyHeight       pointers to return coords in.
//      
//      lChild          child ID    
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
    HRESULT hr = S_OK;


    assert( pxLeft );
    assert( pyTop );
    assert( pcxWidth );
    assert( pcyHeight );

    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;
        
        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->accLocation( pxLeft, pyTop, pcxWidth, pcyHeight, varID );
    }

    //--------------------------------------------------
    //  Otherwise, delegate to base class.
    //--------------------------------------------------

    else
    {
        if ( lChild != CHILDID_SELF )
            hr = E_INVALIDARG;
        else
            hr = CTridentAO::AccLocation( pxLeft, pyTop, pcxWidth, pcyHeight, lChild );
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccRole()
//
//  DESCRIPTION:
//      returns role to client.
//      if embedded object implements IAccessible, forward call to it.
//      else set role to ROLE_SYSTEM_CLIENT.
//      
//  PARAMETERS:
//
//      lChild          child ID    
//      plRole          pointer to role var
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccRole( long lChild, long* plRole )
{
    HRESULT hr = S_OK;


    assert( plRole );

    if ( !plRole )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;
        VARIANT varRole;

        hr = loadVariant( &varID, lChild );

        if ( hr != S_OK )
            return hr;


        hr = m_pIAccessible->get_accRole( varID, &varRole );

        if ( hr == S_OK )
            hr = unpackVariant( varRole, plRole );
    }
    else
    {
        *plRole = m_lRole;
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccFocus()
//
//  DESCRIPTION:
//      returns IUnknown * of object that has focus to client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccFocus( IUnknown** ppIUnknown )
{
    HRESULT hr = S_OK;


    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;

    *ppIUnknown = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varIUnk;

        hr = m_pIAccessible->get_accFocus( &varIUnk );

        if ( hr == S_OK )
            hr = unpackVariant( varIUnk, ppIUnknown );
    }

    //--------------------------------------------------
    //  Otherwise, delegate to base class.
    //--------------------------------------------------

    else
    {
        hr = CTridentAO::GetAccFocus( ppIUnknown );
    }


Cleanup:
    return hr;
}


    
//-----------------------------------------------------------------------
//  CPluginAO::GetAccSelection()
//
//  DESCRIPTION:
//      returns IUnknown * of selected object to client, or IUnknown
//      of IEnumVariant interface that contains selection of objects.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccSelection( IUnknown** ppIUnknown )
{
    HRESULT hr = S_OK;


    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;

    *ppIUnknown = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varIUnk;

        hr = m_pIAccessible->get_accSelection( &varIUnk );

        if ( hr == S_OK )
            hr = unpackVariant( varIUnk, ppIUnknown );
    }

    //--------------------------------------------------
    //  Otherwise, delegate to base class.
    //--------------------------------------------------

    else
    {
        hr = CTridentAO::GetAccSelection( ppIUnknown );
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::AccNavigate()
//
//  DESCRIPTION:
//      navigates from one object to next in specified direction.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      navDir      constant : direction to navigate in
//      lStart      id of object to start navigation from.
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::AccNavigate( long navDir, long lStart, IUnknown** ppIUnknown )
{
    HRESULT hr = S_OK;


    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;

    *ppIUnknown = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varStart;
        VARIANT varIUnk;

        hr = loadVariant( &varStart, lStart );

        if ( hr == S_OK )
        {
            hr = m_pIAccessible->accNavigate( navDir, varStart, &varIUnk );

            if ( hr == S_OK )
                hr = unpackVariant( varIUnk, ppIUnknown );
        }
    }

    //--------------------------------------------------
    //  Otherwise, delegate to base class.
    //--------------------------------------------------

    else
    {
        hr = CTridentAO::AccNavigate( navDir, lStart, ppIUnknown );
    }

Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::AccHitTest()
//
//  DESCRIPTION:
//      returns IUnknown * of object at tested coordinates.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      xLeft       x coord.
//      yTop        y coord.
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//  BUGBUG : our current architecture makes no provisions
//  for Accessible element children of an embedded
//  HWND object. In the case of select lists, this is 
//  fine.  However, other objects that implement 
//  accessible elements will not be able to resolve
//  those children.  This should be KB'd for the
//  4.01 release and fixed in IE5.
// ----------------------------------------------------------------------

HRESULT CPluginAO::AccHitTest( long xLeft, long yTop, IUnknown** ppIUnknown )
{
    HRESULT hr = S_OK;


    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;

    *ppIUnknown = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varIUnk;

        if ( hr = m_pIAccessible->accHitTest( xLeft, yTop, &varIUnk ) )
            goto Cleanup;

        if(varIUnk.vt == VT_I4)
        {
            //--------------------------------------------------
            // for now, all element children are mapped to the 
            // parent object.
            //--------------------------------------------------
            
            *ppIUnknown = m_pIAccessible;
        }
        else
            hr = unpackVariant( varIUnk, (IDispatch **)ppIUnknown );
    }

    //--------------------------------------------------
    //  Otherwise, delegate to base class.
    //--------------------------------------------------

    else
    {
        hr = CTridentAO::AccHitTest( xLeft, yTop, ppIUnknown );
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::AccSelect()
//
//  DESCRIPTION:
//      selects object.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      lflagsSel   selection flags
//      lChild      child to select.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::AccSelect( long flagsSel, long lChild )
{
    HRESULT hr;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->accSelect( flagsSel, varID );
    }

    //--------------------------------------------------
    //  Delegate to internal method that scrolls control
    //  into view and then focuses the control.
    //--------------------------------------------------
        
    else
    {
        //--------------------------------------------------
        // only SELFLAG_TAKEFOCUS is supported.
        //--------------------------------------------------

        if ( !(flagsSel & SELFLAG_TAKEFOCUS) || lChild != CHILDID_SELF )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        //--------------------------------------------------
        // set focus for all types of plugins
        //--------------------------------------------------

        CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement(m_pTOMObjIUnk);

        if ( !pIHTMLControlElement )
            hr = E_NOINTERFACE;
        else
            hr = pIHTMLControlElement->focus();
    }

Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::GetAccKeyboardShortcut()
//
//  DESCRIPTION:
//      returns keystroke associated with object.
//      if embedded object implements IAccessible, forward call to it.
//      else return not implemented
//      
//  PARAMETERS:
//
//      lChild
//      pbstrKeyboardShortcut
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_INVALIDARG | E_DISP_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut )
{
    HRESULT hr;


    assert( pbstrKeyboardShortcut );

    if ( !pbstrKeyboardShortcut )
        return E_INVALIDARG;

    *pbstrKeyboardShortcut = NULL;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    //--------------------------------------------------
    //  Delegate to embedded object if it implements
    //  IAccessible.
    //--------------------------------------------------

    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->get_accKeyboardShortcut( varID, pbstrKeyboardShortcut );
    }
    else
        hr = DISP_E_MEMBERNOTFOUND;


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::SetAccName()
//
//  DESCRIPTION:
//      sends name string to object from client.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      lChild      Child ID
//      szName      BSTR to submit to object.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CPluginAO::SetAccName( long lChild, BSTR bstrName )
{
    HRESULT hr;


    assert( bstrName );

    if ( !bstrName )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->put_accName( varID, bstrName );
    }
    else
        hr = S_FALSE;


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CPluginAO::SetAccValue()
//
//  DESCRIPTION:
//      sends value string from client to object.
//      if embedded object implements IAccessible, forward call to it.
//      else call CTridentAO implementation.
//      
//  PARAMETERS:
//
//      lChild      Child ID
//      szValue     BSTR to submit to object.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------


HRESULT CPluginAO::SetAccValue( long lChild, BSTR bstrValue )
{
    HRESULT hr;


    assert ( bstrValue );

    if ( !bstrValue )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  If the internal IAccessible* isn't set and
    //  html object was previously out of view, try
    //  to create a standard accessible object.
    //--------------------------------------------------

    if ( !m_pIAccessible && !scrolledIntoViewAlready() )
    {
        hr = createStdAccObjIfVisibleAndHasHWND( &m_pIAccessible );

        if ( hr != S_OK )
        {
            if ( hr == S_FALSE )
                hr = S_OK;
            else
                goto Cleanup;
        }
    }


    if ( m_pIAccessible )
    {
        VARIANT varID;

        hr = loadVariant( &varID, lChild );

        if ( hr == S_OK )
            hr = m_pIAccessible->put_accValue( varID, bstrValue );
    }
    else
        hr = S_FALSE;


Cleanup:
    return hr;
}



//================================================================================
// CPluginAO class implementation : protected methods
//================================================================================


//--------------------------------------------------------------------------------
//  CPluginAO::doesPluginSupportIAccessible()
//
//  DESCRIPTION:
//
//      checks to see if the plugin implements the IAccessible interface : 
//      if it does, we set the internal IAccessible* interface and 
//      do nothing else.
//
//  PARAMETERS:
//
//      none.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | E_NOINTERFACE
//
//--------------------------------------------------------------------------------

HRESULT CPluginAO::doesPluginSupportIAccessible()
{
    HRESULT hr;


    m_pIAccessible = NULL;


    //--------------------------------------------------
    //  There is no way to get a hold of an EMBED tag's
    //  IAccessible* (if it even supports IAccessible),
    //  through TOM, so just return here.
    //--------------------------------------------------

    if ( m_iPluginType == IEMBED )
        return E_NOINTERFACE;



    CComPtr<IDispatch> pIDispatch;

    CComQIPtr<IHTMLObjectElement,&IID_IHTMLObjectElement> pIHTMLObjectEl(m_pTOMObjIUnk);

    if ( !pIHTMLObjectEl )
        return E_FAIL;


    hr = pIHTMLObjectEl->get_object( &pIDispatch );

    if ( hr != S_OK )
        return hr;


    //--------------------------------------------------
    //  Ensure that the IDispatch* is valid.  If not,
    //  return E_NOINTERFACE.
    //--------------------------------------------------

    if ( !pIDispatch )
        return E_NOINTERFACE;


    //--------------------------------------------------
    //  QI the returned IDispatch for IAccessible
    //  interface.
    //--------------------------------------------------
    
    hr = pIDispatch->QueryInterface( IID_IAccessible, (void**) &m_pIAccessible );

    if ( hr == S_OK )
    {
        assert( m_pIAccessible );

        m_bCheckedForHWND = TRUE;
    }
    else
        assert( !m_pIAccessible );


    return hr;
}




//--------------------------------------------------------------------------------
//  CPluginAO::createStdAccObjIfVisibleAndHasHWND()
//
//  DESCRIPTION:
//  creates std accessible object if plugin is visible and hosts a windowed object.
//
//  PARAMETERS:
//  ppIAccessible   pointer to return IAccessible * in.
//
//  RETURNS:
//  S_OK if IAccessible interface created, 
//  S_FALSE if object not visible,
//  else std COM error.
//
//  NOTES:
//  this method sets the internal boolean m_bCheckedForHWND to TRUE if the
//  plugin is visible.
//--------------------------------------------------------------------------------

HRESULT
CPluginAO::createStdAccObjIfVisibleAndHasHWND( IAccessible** ppIAccessible )
{
    HRESULT hr;
    long xLeft      = 0;
    long yTop       = 0;
    long cxWidth    = 0;
    long cyHeight   = 0;

    CComQIPtr<IOleWindow,&IID_IOleWindow> pIOleWindow(m_pTOMObjIUnk);


    assert( ppIAccessible );

    *ppIAccessible = NULL;

    //--------------------------------------------------
    // is the html plugin visible? if not, stop here.
    //--------------------------------------------------

    hr = CTridentAO::AccLocation(&xLeft,&yTop,&cxWidth,&cyHeight,0);

    if ( m_bOffScreen )
        hr = S_FALSE;

    if ( hr != S_OK )
        goto Cleanup;
    else
        m_bCheckedForHWND = TRUE;


    //--------------------------------------------------
    // Check if the plugin is a windowed container.
    //  If it is, use the HWND to create a standard
    //  accessible object.
    //--------------------------------------------------

    if ( pIOleWindow )
    {
        HWND    hWndObj = NULL;

        hr = pIOleWindow->GetWindow( &hWndObj );

        if ( hr == S_OK )
        {
            assert( hWndObj );

            hr = CreateStdAccessibleObject( hWndObj, OBJID_WINDOW, IID_IAccessible, (void **)&m_pIAccessible );

            if ( hr != S_OK )
                m_pIAccessible = NULL;
        }
        else
            hr = S_FALSE;
    }


Cleanup:
    return hr;
}



//--------------------------------------------------------------------------------
//  CPluginAO::loadVariant()
//
//  DESCRIPTION:
//
//  loads variant with specified long value.
//
//  PARAMETERS:
//
//      pVar    pointer to variant.
//      lvar    long to store in variant.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CPluginAO::loadVariant( VARIANT* pVar, long lVar )
{
    assert( pVar );

    if ( !pVar )
        return E_INVALIDARG;


    VariantInit( pVar );

    pVar->vt = VT_I4;
    pVar->lVal = lVar;


    return S_OK;
}


//--------------------------------------------------------------------------------
//  CPluginAO::unpackVariant()
//
//  DESCRIPTION:
//
//  unpacks contents of variant, verifies that it contains a 
//  pointer to IUnknown, and sets the passed in parameter to that pointer value.
//
//  PARAMETERS:
//
//      pVarToUnpack    pointer to variant to unpack.
//      ppIUnknown      pointer to store IUnknown * in.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CPluginAO::unpackVariant( VARIANT varToUnpack, IUnknown** ppIUnknown )
{
    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  This method only handles IUnknown pointers.
    //--------------------------------------------------

    if( varToUnpack.vt != VT_UNKNOWN )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  "Unpacking" means that we are assigning the 
    //  contents of the VARIANT to the passed in pointer
    //  (this will be returned to calling method).
    //--------------------------------------------------

    *ppIUnknown = varToUnpack.punkVal;

    return S_OK;
}


//--------------------------------------------------------------------------------
//  CPluginAO::unpackVariant()
//
//  DESCRIPTION:
//
//  unpacks contents of variant, verifies that it contains a 
//  pointer to IDispatch, and sets the passed in parameter to that pointer value.
//
//  PARAMETERS:
//
//      pVarToUnpack    pointer to variant to unpack.
//      ppIDispatch     pointer to store IDispatch * in.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CPluginAO::unpackVariant(VARIANT varToUnpack,IDispatch ** ppIDispatch)
{

    assert( ppIDispatch );


    //--------------------------------------------------
    // this method only handles IDispatch pointers
    //--------------------------------------------------

    if(varToUnpack.vt != VT_DISPATCH)
        return(E_FAIL);

    //--------------------------------------------------
    // 'unpacking' means that we are assigning contents 
    // of variant to passed in pointer
    // (this will be returned to calling method)
    //--------------------------------------------------

    *ppIDispatch = varToUnpack.pdispVal;

    return(S_OK);
}


//--------------------------------------------------------------------------------
//  CPluginAO::unpackVariant()
//
//  DESCRIPTION:
//
//  unpacks contents of variant, verifies that it contains a 
//  pointer to IUnknown, and sets the passed in parameter to that pointer value.
//
//  PARAMETERS:
//
//      pVarToUnpack    pointer to variant to unpack.
//      plong           pointer to store long in.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CPluginAO::unpackVariant( VARIANT varToUnpack, long* plong )
{
    assert( plong );

    if ( !plong )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  This method only handles longs.
    //--------------------------------------------------

    if ( varToUnpack.vt != VT_I4 )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  "Unpacking" means that we are assigning the 
    //  contents of the VARIANT to the passed in pointer
    //  (this will be returned to calling method).
    //--------------------------------------------------

    *plong = varToUnpack.lVal;

    return S_OK;
}





//----  End of PLUGIN.CPP  ----
