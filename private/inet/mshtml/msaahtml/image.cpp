//================================================================================
//      File:   IMAGE.CPP
//      Date:   7/1/97
//      Desc:   contains implementation of CImageAO class.  CImageAO 
//              implements the accessible proxy for the Trident Image 
//              object.
//
//      Author: Arunj
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "image.h"
#include "anchor.h"


#ifdef _MSAA_EVENTS

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CImageAO,ImplIHTMLImgEvents,CEvent)

    ON_DISPID_FIRE_EVENT(DISPID_HTMLIMGBASEEVENTS_ONREADYSTATECHANGE,EVENT_OBJECT_STATECHANGE)
             
END_EVENT_HANDLER_MAP()

#endif


//================================================================================
// CImageAO : public methods
//================================================================================

//-----------------------------------------------------------------------
//  CImageAO::CImageAO()
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

CImageAO::CImageAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
    //------------------------------------------------
    // assign the delegating IUnknown to CImageAO :
    // this member will be overridden in derived class
    // constructors so that the delegating IUnknown 
    // will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown     = (IUnknown *)this;                                 


    //--------------------------------------------------
    // initialize map pointer : this will be set
    // if the image uses a map.
    //--------------------------------------------------

    m_pMapToUse = NULL;


    m_pIHTMLImgElement = NULL;


    //--------------------------------------------------
    // set role to zero
    //--------------------------------------------------

    m_lRole = 0;

    //--------------------------------------------------
    // set the item type so that it can be accessed
    // via base class pointer.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_IMAGE;


#ifdef _DEBUG

    lstrcpy(m_szAOMName,_T("ImageAO"));

#endif

}


//-----------------------------------------------------------------------
//  CImageAO::Init()
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

HRESULT CImageAO::Init(IUnknown * pTOMObjIUnk)
{
    HRESULT hr  = E_FAIL;


    assert( pTOMObjIUnk );
    
    //--------------------------------------------------
    // call down to base class to set unknown pointer.
    //--------------------------------------------------

    hr = CTridentAO::Init(pTOMObjIUnk);


    //--------------------------------------------------
    //  Cache the IMAGE's IHTMLImgElement pointer.
    //--------------------------------------------------

    if ( hr == S_OK )
    {
        hr = pTOMObjIUnk->QueryInterface( IID_IHTMLImgElement, (void**) &m_pIHTMLImgElement );

        if ( hr == S_OK )
        {
            assert( m_pIHTMLImgElement );

            if ( !m_pIHTMLImgElement )
                hr = E_NOINTERFACE;
        }
    }


#ifdef _MSAA_EVENTS

    if ( hr == S_OK )
    {
        HRESULT hrEventInit;

        //--------------------------------------------------
        // allocate event handling interface  and establish
        // Advise.
        //--------------------------------------------------
                
        hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLImgEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

        assert( hrEventInit );

#ifdef _DEBUG
        if ( hrEventInit != S_OK )
            OutputDebugString( "Event handler initialization in CImageAO::Init() failed.\n" );
#endif
    }

#endif  // _MSAA_EVENTS


    return hr;
}

//-----------------------------------------------------------------------
//  CImageAO::~CImageAO()
//
//  DESCRIPTION:
//
//      CImageAO class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.                      
//
// ----------------------------------------------------------------------

CImageAO::~CImageAO()
{

    if ( m_pIHTMLImgElement )
        m_pIHTMLImgElement->Release();

}       


//-----------------------------------------------------------------------
//  CImageAO::ReleaseTridentInterfaces()
//
//  DESCRIPTION: Calls release on all CImageAO-specific cached Trident
//               object interface pointers.  Also calls the base class
//               ReleaseTridentInterfaces().
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

void CImageAO::ReleaseTridentInterfaces ()
{
    if ( m_pIHTMLImgElement )
    {
        m_pIHTMLImgElement->Release();
        m_pIHTMLImgElement = NULL;
    }

    CTridentAO::ReleaseTridentInterfaces();
}


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//  CImageAO::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CImageAO object only implements
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

STDMETHODIMP CImageAO::QueryInterface(REFIID riid, void** ppv)
{
    if ( !ppv )
        return E_INVALIDARG;

    *ppv = NULL;

    if (riid == IID_IUnknown)  
    {
        *ppv = (IUnknown *)this;
    }

#ifdef _MSAA_EVENTS 
    
    else if (riid == DIID_HTMLImgEvents)
    {
        //--------------------------------------------------
        // this is the event interface for the image class.
        //--------------------------------------------------

        ASSIGN_TO_EVENT_HANDLER(ImplIHTMLImgEvents,ppv,HTMLImgEvents)
    } 
    
#endif

    else
    {
        //--------------------------------------------------
        // delegate to base class
        //--------------------------------------------------

        return(CTridentAO::QueryInterface(riid,ppv));
    }

    
    //--------------------------------------------------
    // ppv should be set by now.
    //--------------------------------------------------

    assert( *ppv );
    
    ((LPUNKNOWN) *ppv)->AddRef();

    return(NOERROR);
}

//================================================================================
// CImageAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//  CImageAO::GetAccName()
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

HRESULT CImageAO::GetAccName(long lChild, BSTR * pbstrName)
{
    HRESULT hr = S_OK;
    
    
    assert( pbstrName );


    if ( !m_bNameAndDescriptionResolved )
    {
        resolveNameAndDescription();
    }

    if ( m_bstrName )
        *pbstrName = SysAllocString( m_bstrName );
    else
        hr = DISP_E_MEMBERNOTFOUND;


    return hr;
}
        


//-----------------------------------------------------------------------
//  CImageAO::GetAccDescription()
//
//  DESCRIPTION:
//
//      returns description of image object.
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

HRESULT CImageAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
    HRESULT hr = S_OK;
    
    
    assert( pbstrDescription );


    if ( !m_bNameAndDescriptionResolved )
    {
        resolveNameAndDescription();
    }

    if ( m_bstrDescription )
        *pbstrDescription = SysAllocString( m_bstrDescription );
    else
        hr = DISP_E_MEMBERNOTFOUND;


    return hr;
}

    
//-----------------------------------------------------------------------
//  CImageAO::GetAccValue()
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

HRESULT CImageAO::GetAccValue(long lChild, BSTR * pbstrValue)
{
    HRESULT hr = S_OK;

    
    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pbstrValue );

    if(!pbstrValue)
        return(E_INVALIDARG);


    //--------------------------------------------------
    // if ancestor is an anchor, delegate to its value.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->GetAccValue( CHILDID_SELF, pbstrValue );
    else
        hr = S_OK;


    if ( m_bstrValue )
    {
        *pbstrValue = SysAllocString( m_bstrValue );
    }
    else
    {
        assert( m_pIHTMLImgElement );

        //--------------------------------------------------
        // query for the DYNSRC property
        //--------------------------------------------------

        hr = m_pIHTMLImgElement->get_dynsrc( pbstrValue );

        if ( hr == S_OK && *pbstrValue )
        {
            m_bstrValue = SysAllocString( *pbstrValue );
        }
        else
        {
            //--------------------------------------------------
            // since DYNSRC wasn't found, query for SRC
            //--------------------------------------------------

            hr = m_pIHTMLImgElement->get_src( pbstrValue );

            if ( hr == S_OK && *pbstrValue )
            {
                m_bstrValue = SysAllocString( *pbstrValue );
            }
            else
                hr = DISP_E_MEMBERNOTFOUND;
        }
    }


    return hr;
}


//-----------------------------------------------------------------------
//  CImageAO::GetAccState()
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
    
HRESULT CImageAO::GetAccState(long lChild, long *plState)
{
    HRESULT         hr          = E_FAIL;
    VARIANT_BOOL    bComplete   = 0;
    long            ltempState  = 0;



    assert( plState );


    *plState = 0;


    //--------------------------------------------------
    // Determine if the image has been fully downloaded.
    //--------------------------------------------------

    assert( m_pIHTMLImgElement );

    hr = m_pIHTMLImgElement->get_complete( &bComplete );

    if ( hr == S_OK )
    {
        if ( !bComplete )
        {
            *plState = STATE_SYSTEM_UNAVAILABLE;
            return hr;
        }
    }


    //--------------------------------------------------
    // If ancestor is an anchor, use its attributes.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        hr = pAnc->GetAccState( CHILDID_SELF, &ltempState );
    else
    {
        //--------------------------------------------------
        // Call down to the base class to determine
        // visibility.
        //--------------------------------------------------

        hr = CTridentAO::GetAccState( lChild, &ltempState );
    }


    if ( hr != S_OK )
    {
        //--------------------------------------------------
        // Update the location to determine offscreen status
        //--------------------------------------------------

        long lDummy;

        hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

        if (SUCCEEDED( hr ) && m_bOffScreen)
            ltempState |= STATE_SYSTEM_INVISIBLE;
    }


    *plState = ltempState;


    return S_OK;
}


//-----------------------------------------------------------------------
//  CImageAO::GetAccRole()
//
//  DESCRIPTION:
//
//      The role of an image is either ROLE_SYSTEM_GRAPHIC or
//      ROLE_SYSTEM_ANIMATION.
//
//  PARAMETERS:
//
//      lChild      Child ID
//      plRole      long to store returned role var in.
//
//  RETURNS:
//
//      HRESULT :   S_OK
//
//-----------------------------------------------------------------------

HRESULT CImageAO::GetAccRole(long lChild, long *plRole)
{
    HRESULT hr          = S_OK;
    BSTR    bstrTmp     = NULL;


    assert( m_pIHTMLImgElement );
    assert( plRole );


    if ( !m_lRole )
    {
        m_lRole = ROLE_SYSTEM_GRAPHIC;


        hr = m_pIHTMLImgElement->get_dynsrc( &bstrTmp );

        if ( hr == S_OK && bstrTmp )
        {
            m_lRole = ROLE_SYSTEM_ANIMATION;

            SysFreeString( bstrTmp );
        }
    }


    *plRole = m_lRole;

    
    return hr;
}

//-----------------------------------------------------------------------
//  CImageAO::AccDoDefaultAction()
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

HRESULT CImageAO::AccDoDefaultAction(long lChild)
{
    //--------------------------------------------------
    // if ancestor is an anchor, delegate to it.
    // Otherwise, use base class implementation.
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    HRESULT hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->AccDoDefaultAction( CHILDID_SELF );
    else
        return CTridentAO::AccDoDefaultAction( lChild );
}

//-----------------------------------------------------------------------
//  CImageAO::GetAccDefaultAction()
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

HRESULT CImageAO::GetAccDefaultAction(long lChild, BSTR * pbstrDefAction)
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
        return CTridentAO::GetAccDefaultAction( lChild, pbstrDefAction );
}


//-----------------------------------------------------------------------
//  CImageAO::GetAccSelection()
//
//  DESCRIPTION:
//
//      get_accSelection() not supported for a image.
//
//
//  PARAMETERS:
//      
//      ppIUnknown  pointer to an IUnknown*
//
//  RETURNS:
//
//      HRESULT     DISP_E_MEMBERNOTFOUND
// ----------------------------------------------------------------------

HRESULT CImageAO::GetAccSelection( IUnknown** ppIUnknown )
{
    return DISP_E_MEMBERNOTFOUND;
}


//-----------------------------------------------------------------------
//  CImageAO::AccSelect()
//
//  DESCRIPTION:
//      Selects specified object: selection based on flags.  
//
//      NOTE: only the SELFLAG_TAKEFOCUS flag is supported.
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

HRESULT CImageAO::AccSelect(long flagsSel, long lChild)
{
    HRESULT hr = S_OK;


    //--------------------------------------------------
    // only SELFLAG_TAKEFOCUS is supported.
    //--------------------------------------------------

    if ( !(flagsSel & SELFLAG_TAKEFOCUS) )
        return E_INVALIDARG;


    //--------------------------------------------------
    // set focus
    //--------------------------------------------------

    CAnchorAO*  pAnc = NULL;

    hr = m_pParent->IsAncestorAnchor( &pAnc );

    if ( hr == S_OK )
        return pAnc->AccSelect( flagsSel, CHILDID_SELF );
    else
        return CTridentAO::AccSelect( flagsSel, lChild );
}


//-----------------------------------------------------------------------
//  CImageAO::GetAccKeyboardShortcut()
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

HRESULT CImageAO::GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut )
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
// CImageAO : protected methods
//================================================================================

//-----------------------------------------------------------------------
//  CImageAO::getDescriptionString()
//
//  DESCRIPTION:
//
//      Obtains the text of the IMG's ALT property to be used as the
//      accDescription of the CImageAO.
//
//  PARAMETERS:
//
//      pbstrDescStr    [out]   pointer to the BSTR to hold the ALT text
//
//  RETURNS:
//
//      HRESULT
//
//-----------------------------------------------------------------------

HRESULT CImageAO::getDescriptionString( BSTR* pbstrDescStr )
{
    assert( m_pIHTMLImgElement );


    assert( pbstrDescStr );

    *pbstrDescStr = NULL;


    return m_pIHTMLImgElement->get_alt( pbstrDescStr );
}



//----  End of IMAGE.CPP  ----