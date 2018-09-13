//=======================================================================
//      File:   WINDOW.CPP
//      Date:   5/23/97
//      Desc:   contains implementation of CWindowAO class.  CWindowAO 
//              implements the root Window accessible object for the Trident 
//              MSAA Registered Handler.
//
//      
//      Notes:  The CWindowAO class implements the main proxy object for
//              MSAAHTML. As a main proxy object, it exposes and implements
//              The IAccessible interface. Several of the methods that
//              it derives from CTridentAO are overridden to provide quick
//              wrappers around the member CDocumentAO object, which implements
//              the AOM tree and handles all AOM heirarchy navigation and
//              information retrieval.
//=======================================================================

//=======================================================================
//  Includes
//=======================================================================

#include "stdafx.h"
#include "oleacapi.h"
#include "trid_ae.h"
#include "trid_ao.h"
#include "window.h"
#include "aommgr.h"
#include "document.h"
#include "prxymgr.h"
#include "exdisp.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif


//BUGBUG (carled) these are defined in a private copy of winuser.h from the oleacc team. 
// these should be removed once this is checked in.
#ifndef WMOBJ_ID
#define WMOBJ_ID 0x0000
#endif

#ifndef WMOBJ_SAMETHREAD
#define WMOBJ_SAMETHREAD 0x8000
#endif
                                            
//=======================================================================
//  Defines
//=======================================================================

#define MSGNAME_WM_HTML_GETOBJECT   _T("WM_HTML_GETOBJECT")

#define OBJID_ZOMBIFYTREE           (0x8F00000F)

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CWindowAO,ImplIHTMLWindowEvents,CEvent)

//  ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONLOAD,EVENT_OBJECT_CREATE)
    ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONUNLOAD,EVENT_OBJECT_DESTROY)                                                                         
//  ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONRESIZE,EVENT_OBJECT_STATECHANGE) 
//  ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONBEFOREUNLOAD,EVENT_OBJECT_STATECHANGE)   
//  ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
//  ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONSCROLL,EVENT_OBJECT_STATECHANGE)
//  ON_DISPID_FIRE_EVENT(DISPID_HTMLWINDOWEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)

         
END_EVENT_HANDLER_MAP()


//=======================================================================
// CWindowAO class implementation : public methods
//=======================================================================

//-----------------------------------------------------------------------
//  CWindowAO::CWindowAO()
//
//  DESCRIPTION:
//
//      Constructor
//
//  PARAMETERS:
//
//      pAOMParent          pointer to the parent accessible object in 
//                          the AOM tree
//
//      nTOMIndex           index of the element from the TOM document.all 
//                          collection.
//      
//      nUID                unique ID.
//
//      hWndTrident         pointer to the window of the trident object that 
//                          this object corresponds to.
//
//  RETURNS:
//
//      None.
// ----------------------------------------------------------------------

CWindowAO::CWindowAO(CProxyManager * pProxyMgr, CTridentAO * pAOMParent, long nTOMIndex, long nUID, HWND hTridentWnd)
: CTridentAO(pAOMParent,NULL,nTOMIndex,nUID,hTridentWnd)
{
    //--------------------------------------------------
    // the only reason that we needa a back pointer 
    // to the proxy manager is to notify it of an 
    // UNLOAD event.  w/o events, there is no 
    // need for a back pointer. Assert only with
    // events turned ON.
    //--------------------------------------------------


#ifdef _MSAA_EVENTS
    assert (pProxyMgr);
#endif

    NULL_NOTIFY_EVENT_HANDLER_PTR(ImplIHTMLWindowEvents);


    m_pProxyMgr = pProxyMgr;

    m_lRole     = ROLE_SYSTEM_CLIENT;

    //------------------------------------------------
    // Assign the delegating IUnknown to CTridentAE.
    //  This member will be overridden in derived class
    //  constructors so that the delegating IUnknown 
    //  will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown = (IUnknown *)this;

    //------------------------------------------------
    //  Initialize the message value for the
    //  WM_HTML_GETOBJECT message to zero to indicate
    //  that this private message has not yet been
    //  registered on the system.
    //------------------------------------------------

    m_nMsgHTMLGetObject = 0;

    //--------------------------------------------------
    // set the item type so that it can be accessed
    // via base class pointer.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_WINDOW;

    //--------------------------------------------------
    // These are set in the Init()
    //--------------------------------------------------

    m_pIHTMLWindow2     = NULL;
    m_pAOMMgr           = NULL;


#ifdef _DEBUG

    //--------------------------------------------------
    // set this string for debugging use
    //--------------------------------------------------
    _tcscpy(m_szAOMName,_T("WindowAO"));

#endif

}

//-----------------------------------------------------------------------
//  CWindowAO::~CWindowAO()
//
//  DESCRIPTION:
//
//      CWindowAO class destructor. The CTridentAO base class 
//      destructor destroys all of object's children, effectively
//      destroying the AOM tree attached to a CWindowAO object.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  NOTES:
//
//      The COM pointer m_pIHTMLWindow2 is released here if needed.
//
//      The CDocumentAO object created in Init() is destroyed
//      in CTridentAO::freeChildren() call.
// ----------------------------------------------------------------------

CWindowAO::~CWindowAO()
{
    ReleaseTridentInterfaces();

    if(m_pAOMMgr)
    {
        delete m_pAOMMgr;
        m_pAOMMgr = NULL;
    }
}


//-----------------------------------------------------------------------
//  CWindowAO::Detach()
//
//  DESCRIPTION: TridentAO virtual override, because windows need to
//                  remove themselves from the proxy manager's window list.
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

void 
CWindowAO::Detach()
{    
    m_bDetached = TRUE;

    // detach our own children
    DetachChildren();

    // release all cached Trident objects
    ReleaseTridentInterfaces();

    // since we are detached, remove us from the proxy managers list.
    m_pProxyMgr->RemoveAOMWindow(this);

    Release();
}
    

//-----------------------------------------------------------------------
//  CWindowAO::ReleaseTridentInterfaces()
//
//  DESCRIPTION: Calls release on all CWindowAO-specific cached Trident
//               object interface pointers.  Also calls the base class
//               ReleaseTridentInterfaces().
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

void CWindowAO::ReleaseTridentInterfaces ()
{
    if ( m_pAOMMgr )
        m_pAOMMgr->ReleaseTridentInterfaces();

    if ( m_pIHTMLWindow2 )
    {
        m_pIHTMLWindow2->Release();
        m_pIHTMLWindow2 = NULL;
    }

    if ( m_pTOMObjIUnk )
    {
        m_pTOMObjIUnk->Release();
        m_pTOMObjIUnk = NULL;
    }

    DESTROY_NOTIFY_EVENT_HANDLER(ImplIHTMLWindowEvents);

    CTridentAO::ReleaseTridentInterfaces();
}


//-----------------------------------------------------------------------
//  CWindowAO::Init()
//
//  DESCRIPTION:
//
//      Initializes the object to a clean state and sinks to the outgoing
//      Trident event interfaces to receive page reload notifications.
//
//
//  PARAMETERS:
//
//      pTOMObjIUnk     [optional] IUnknown pointer to TEO element, used
//                      if init is being called for a frame window.
//      
//  RETURNS:
//
//      HRESULT     Success or failure.
//
//  NOTES:
//
//  derived classes will want to call this initialization AFTER they have 
//  done their specific initialization. derived classes should always implement
//  their own interfaces before calling this method. 
// ----------------------------------------------------------------------

HRESULT CWindowAO::Init( IUnknown * pTOMObjIUnk)
{
    HRESULT hr =E_FAIL;
    long lDocSourceIndex = 0;

    
    //------------------------------------------------
    //  Register the private window message.  Since
    //  this message is the sole communication link
    //  with Trident initially available, return
    //  on message registration failure.
    //------------------------------------------------

    hr = registerHTMLGetObjectMsg();

    if ( hr != S_OK )
        return( hr );

    //--------------------------------------------------
    // get the pointer and AddRef() to lock it
    // down for the duration of this object.
    //--------------------------------------------------

    CComPtr<IHTMLWindow2> pIHTMLWindow2;
    CComPtr<IHTMLDocument2> pIHTMLDocument2;

    if ( !SUCCEEDED(hr = getIHTMLWindow2Ptr( &pIHTMLWindow2, &pIHTMLDocument2 )) )
        return( hr );

    m_pIHTMLWindow2 = pIHTMLWindow2;
    m_pIHTMLWindow2->AddRef();
        
    //--------------------------------------------------
    // CWindowAO specific initialization.
    //--------------------------------------------------

    if( m_lAOMType == AOMITEM_WINDOW)
    {
        //--------------------------------------------------
        // make sure that this is not a frame window.
        //--------------------------------------------------

        assert(!m_pParent);
        
        if(m_pParent) 
            return(E_FAIL);

        //--------------------------------------------------
        // set m_pTOMObjIUnk to the IUnknown of the IHTMLWindow2 
        // interface.
        //
        // AddRef() happens implicitly.
        //--------------------------------------------------

        if(hr = m_pIHTMLWindow2->QueryInterface(IID_IUnknown,(void **)&m_pTOMObjIUnk))
            return(hr);

        //--------------------------------------------------
        // Since this Init doesn't call CTridentAO::Init(),
        // it must call createInterfaceImplementors() to 
        // create the IAccessible and IOleWindow interfaces.
        //
        // Derived classes need to call this method 
        // prior to calling CWindowAO::Init()
        //--------------------------------------------------

        if (hr = createInterfaceImplementors())
            return hr;

        
        //--------------------------------------------------
        // derived classes need to create AOMMgr and 
        // CDocumentAO prior to calling CWindowAO::Init().
        // Both classes need to be present in order for 
        // IAccessible interface to work correctly (calls
        // are made from that interface to m_pDocAO)
        //--------------------------------------------------

        if(hr = createMemberObjects())
            return(hr);
    
    }
        
    //--------------------------------------------------
    // initialize AOMMgr with the document pointer --
    // that pointer is cached for better performance.
    //--------------------------------------------------

    if(hr = m_pAOMMgr->Init(pIHTMLDocument2))
        return(hr);

    CComPtr<IHTMLElement> pIHTMLElement;

    //--------------------------------------------------
    // get the correct source index for the document :
    // use it as the document's child ID.
    //--------------------------------------------------

    if(hr = pIHTMLDocument2->get_body(&pIHTMLElement) )
    {
        return(hr);
    }

    if(hr = pIHTMLElement->get_sourceIndex(&lDocSourceIndex))
    {
        return(hr);
    }


    CComPtr<IUnknown> pIUnknown(pIHTMLDocument2);

    if ( !pIUnknown )
    {
        m_pDocAO->Detach();
        m_pDocAO = NULL;
        return E_NOINTERFACE;
    }

    hr = m_pDocAO->Init( m_hWnd,lDocSourceIndex,pIUnknown );

    if ( hr != S_OK )
    {
        m_pDocAO->Detach();
        m_pDocAO = NULL;
        return hr;
    }

    //--------------------------------------------------
    // Create event handler and initialize to set up 
    //  the notification sink
    //
    // get an IUnknown ptr from the window interface 
    // pointer.
    //--------------------------------------------------

    IUnknown * pWindowObjIUnk;

    hr = m_pIHTMLWindow2->QueryInterface(IID_IUnknown,(void **)&pWindowObjIUnk);

    if(hr != S_OK)
        return(hr);

    //--------------------------------------------------
    // initialize event handling interface : establish
    // Advise.
    //--------------------------------------------------
    
    HRESULT hrEventInit = E_FAIL;

        
    hrEventInit = CREATE_NOTIFY_EVENT_HANDLER(ImplIHTMLWindowEvents);

    if ( hrEventInit == S_OK )
        hrEventInit = INIT_NOTIFY_EVENT_HANDLER(ImplIHTMLWindowEvents,m_pIUnknown,m_hWnd,m_nChildID,pWindowObjIUnk,this)

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
        OutputDebugString( _T("Event handler initialization in CWindowAO::Init() failed.\n") );

#endif

    if( hrEventInit != S_OK)
            hr = hrEventInit;

    //--------------------------------------------------
    // release allocated pointer.
    //--------------------------------------------------

    pWindowObjIUnk->Release();
    
    return( hr );
}

//=======================================================================
// CWindowAO IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CWindowAO::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CTridentAE object implements
//      IUnknown and IAccessible.
//
//
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
// ----------------------------------------------------------------------

STDMETHODIMP CWindowAO::QueryInterface(REFIID riid, void** ppv)
{
    if(!ppv)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
    }

#ifdef _MSAA_EVENTS 
    else if (riid == DIID_HTMLWindowEvents)
    {
        //--------------------------------------------------
        // this is the event interface for the window class.
        //--------------------------------------------------

        ASSIGN_TO_NOTIFY_EVENT_HANDLER(ImplIHTMLWindowEvents,ppv,HTMLWindowEvents)
    }
#endif

    else
    {
        return(CTridentAO::QueryInterface(riid,ppv));
    }

    ((LPUNKNOWN) *ppv)->AddRef();

    return(NOERROR);
}


//=======================================================================
// CWindowAO IAccessible interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CWindowAO::GetAccName()
//
//  DESCRIPTION:
//
//      Returns name of object/element, which is synthesized.
//
//  PARAMETERS:
//
//      lChild      child ID / Self ID
//      pbstrName   returned name.
//      
//  RETURNS:
//
//      E_NOTIMPL : default element implementation doesn't support children.
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetAccName(long lChild, BSTR * pbstrName)
{
    //------------------------------------------------
    //  Validate the parameters.
    //------------------------------------------------

    assert(pbstrName);

    if(!pbstrName)
        return(E_INVALIDARG);

    //--------------------------------------------------
    // return URL for IE3.0 compatibility : 
    // instead of caching this value, get it every time
    // in case url changes.
    //--------------------------------------------------

    return(m_pDocAO->GetURL(pbstrName));

}

//-----------------------------------------------------------------------
//  CWindowAO::GetAccDescription()
//
//  DESCRIPTION:
//
//      Returns description string to client.
//
//      TODO: Does Window have a description???
//
//  PARAMETERS:
//
//      lChild              Child/Self ID
//
//      pbstrDescription    Description string returned to client.
//  
//  RETURNS:
//
//      S_OK if success, else E_FAILED.
// ----------------------------------------------------------------------
    
HRESULT CWindowAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
    //------------------------------------------------
    //  Validate the parameters.
    //------------------------------------------------

    assert(pbstrDescription);

    if(!pbstrDescription)
        return(E_INVALIDARG);

    return GetResourceStringValue(IDS_WINDOW_DESCRIPTION, pbstrDescription);
}


//----------------------------------------------------------------------- 
//  CWindowAO::GetAccFocus()
//
//  DESCRIPTION:
//
//      Get the object that has the focus, return the type of the focused 
//      object to the user. Since the main TOM document overlays the window,
//      return the document's IDispatch pointer if it or any of its children
//      are focused. The window itself never has the focus.
//
//      NOTE: is this ever called?  Need to verify, otherwise remove.
//
//  PARAMETERS:
//
//      ppIUnknown : pointer to IUnknown of returned object. This object
//                   can be a CAccElement or a CAccObject.
//                   QI for IAccessible to find out.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetAccFocus(IUnknown **ppIUnknown)
{
    HRESULT hr;

    //------------------------------------------------
    //  Validate the parameters.
    //------------------------------------------------

    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;

    *ppIUnknown = NULL;

    //------------------------------------------------
    //  Handle NULL m_pDocAO.
    //------------------------------------------------

    assert( m_pDocAO );

    if ( !m_pDocAO )
        return DISP_E_MEMBERNOTFOUND;


    //------------------------------------------------
    //  If the window is focused, one of its
    //  descendants has actually has the focus.
    //  So, return the CDocumentAO child.
    //------------------------------------------------

    BOOL        bBrowserWindowHasFocus;
    BOOL        bThisWindowHasFocus;

    hr = IsFocused( &bBrowserWindowHasFocus, &bThisWindowHasFocus );
        
    if ( hr == S_OK && bThisWindowHasFocus )
    {
       *ppIUnknown = (IUnknown *)m_pDocAO;
    }

    return hr;
}

//-----------------------------------------------------------------------
//  CWindowAO::AccLocation()
//
//  DESCRIPTION:
//
//      Returns location of window
//      The window doesnt actually have a physical 
//      representation in the TOM world.  It shares the
//      same physical (size) attributes of its one 
//      and only child, the document.  
//      So we need to call down to  the document and 
//      return its dimensions to the client.
//      
//      **NOTE** call down to the document instead of
//      just getting the dimensions of m_hWnd because 
//      the method of sizing the document may change
//      and the implementation should be localized 
//      at one point.
//
//  PARAMETERS:
//
//      pxLeft      ptr to x coord of point
//      pyTop       ptr to y coord of point
//      pcxWidth    ptr to width of window
//      pcyHeight   ptr to height of window.
//
//  RETURNS:
//
//      E_NOTIMPL for base class version.
// ----------------------------------------------------------------------

HRESULT CWindowAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
    
    //------------------------------------------------
    //  Validate the parameters.
    //------------------------------------------------

    assert( pxLeft && pyTop && pcxWidth && pcyHeight );

    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
        return(E_INVALIDARG);

    //------------------------------------------------
    //  Handle NULL m_pDocAO.
    //------------------------------------------------

    assert( m_pDocAO );

    if ( !m_pDocAO )
        return(E_FAIL);


    //------------------------------------------------
    //  Delegate to document child.
    //------------------------------------------------

    
    return( m_pDocAO->AccLocation(pxLeft,pyTop,pcxWidth,pcyHeight,lChild) );
}

//-----------------------------------------------------------------------
//  CWindowAO::GetAccState()
//
//  DESCRIPTION:
//
//      Returns object state to client. If the window has the focus, its 
//      state is focused and focusable, otherwise its state is normal.
//
//  PARAMETERS:
//
//      lChild      CHILDID of child to get state of
//      plState     pointer to store returned state in.
//
//  RETURNS:
//
//      E_NOTIMPL for base class version.
//
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetAccState(long lChild, long *plState)
{
    LPUNKNOWN   lpUnk = NULL;
    HRESULT     hr =    E_FAIL;

    //------------------------------------------------
    //  Validate the parameters.
    //------------------------------------------------

    assert(plState);

    if(!plState)
        return(E_INVALIDARG);

    *plState = 0;


    BOOL        bBrowserWindowHasFocus;
    BOOL        bThisWindowHasFocus;

    hr = IsFocused( &bBrowserWindowHasFocus, &bThisWindowHasFocus );

    if ( hr == S_OK )
    {
        if ( bThisWindowHasFocus )
            *plState = STATE_SYSTEM_FOCUSED | STATE_SYSTEM_FOCUSABLE;
        else if ( bBrowserWindowHasFocus )
            *plState = STATE_SYSTEM_FOCUSABLE;
    }


    return hr;
}



//-----------------------------------------------------------------------
//  CWindowAO::GetAccSelection()
//
//  DESCRIPTION:
//
//      get_accSelection() not supported for a window.
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

HRESULT CWindowAO::GetAccSelection( IUnknown** ppIUnknown )
{
    return DISP_E_MEMBERNOTFOUND;
}


//-----------------------------------------------------------------------
//  CWindowAO::GetAccParent()
//
//  DESCRIPTION:
//
//  gets the parent of this object and returns its IDispatch pointer
//                               
//  PARAMETERS:
//
//  ppdispParent : pointer to parent interface to return to caller.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetAccParent(IDispatch ** ppdispParent)
{
    assert( ppdispParent );


    HRESULT hr;

    if ( m_pParent )
        hr = m_pParent->QueryInterface(IID_IDispatch,(void **)ppdispParent);
    else
        hr = CreateStdAccessibleObject( m_hWnd, OBJID_WINDOW, IID_IDispatch, (void **) ppdispParent );

    if ( hr != S_OK )
    {
        assert( *ppdispParent == NULL );

        hr = S_FALSE;
    }

    return hr;
}



//-----------------------------------------------------------------------
//  CWindowAO::AccSelect()
//
//  DESCRIPTION:
//  
//  explicit selection of the window.
//  
//  PARAMETERS:
//
//  see base class.
//
//  RETURNS:
//
//  see base class.
// ----------------------------------------------------------------------

HRESULT CWindowAO::AccSelect(long flagsSel, long lChild)
{
    HRESULT hr = E_FAIL;


    //--------------------------------------------------
    // if object fully initialized, take the focus
    //--------------------------------------------------

    assert(m_pIHTMLWindow2);

    return(m_pIHTMLWindow2->focus());


}

//-----------------------------------------------------------------------
//  CWindowAO::GetAOMMgr()
//
//  DESCRIPTION:
//  returns pointer to AOMMgr if it exists, else error
//
//  PARAMETERS:
//
//  ppAOMMgr        pointer to return AOMMgr in.
//
//  RETURNS:
//
//  S_OK | E_FAIL
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetAOMMgr(CAOMMgr ** ppAOMMgr)
{
    //------------------------------------------------
    //  Validate the parameters.
    //------------------------------------------------

    assert(ppAOMMgr);

    if(!ppAOMMgr)
        return(E_INVALIDARG);

    if(m_pAOMMgr)
    {
        *ppAOMMgr = m_pAOMMgr;
        return(S_OK);
    }
    else
    {
        return(E_FAIL);
    }

}

//-----------------------------------------------------------------------
//  CWindowAO::GetDocumentChild()
//
//  DESCRIPTION:
//  Returns     pointer to the window's document AO.
//
//  PARAMETERS:
//  ppDocAO     out pointer to window's document AO.        
//
//  RETURNS:
//
//  S_OK | E_FAIL | E_INVALIDARG
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetDocumentChild( CDocumentAO** ppDocAO )
{
    //------------------------------------------------
    //  Validate the parameter.
    //------------------------------------------------

    assert( ppDocAO );

    if ( !ppDocAO )
        return E_INVALIDARG;


    //------------------------------------------------
    //  Handle NULL m_pDocAO.
    //------------------------------------------------

    assert( m_pDocAO );

    if ( !m_pDocAO )
        return E_FAIL;


    //------------------------------------------------
    //  Return a pointer to the window's document.
    //------------------------------------------------

    *ppDocAO = m_pDocAO;


    return S_OK;
}


//-----------------------------------------------------------------------
//  CWindowAO::GetAccessibleObjectFromID()
//
//  DESCRIPTION:
//  returns the accessible object that corresponds to the 
//  input ID.  
//  
//
//  PARAMETERS:
//
//  lObjectID   - id to get object from.
//  ppAE
//
//  RETURNS:
//  
//  S_OK if element found, else S_FALSE if not, else standard COM
//  error.
//
// ----------------------------------------------------------------------

HRESULT CWindowAO::GetAccessibleObjectFromID(long lObjectID, CAccElement **ppAE)
{
    HRESULT hr = E_FAIL;
    
    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    if(!ppAE)
        return(E_INVALIDARG);

    *ppAE = NULL;

    if (DoBlockForDetach()) 
        return E_FAIL;

    //--------------------------------------------------
    // dont build the tree if we don't have to.
    //--------------------------------------------------

    if(lObjectID == OBJID_WINDOW || lObjectID == OBJID_CLIENT)
    {
        *ppAE = this;
        return(S_OK);
    }

    //--------------------------------------------------
    // look for the "magic" object ID that means we
    //  zombify and mark for detach the window and its
    //  child tree.
    //
    // BUGBUG: this is a grade A "hack"!  Trident sends
    //  MSAAHTML the ID when it's activation state changes
    //  from in-place to running.  This workarounds a
    //  Trident hosting bug in Outlook98.  For more
    //  info refer to IE5 bug 28935.
    //--------------------------------------------------

    if(lObjectID == OBJID_ZOMBIFYTREE)
    {
        m_pDocAO->SetReadyToDetach( TRUE );
        Zombify();

        return S_FALSE;
    }

    //--------------------------------------------------
    // m_pAOMMgr is set in the Init() call : Init()
    // MUST be called prior to this method.
    //--------------------------------------------------

    if(!(m_pAOMMgr))
    {
        return(E_FAIL);
    }

    
    //--------------------------------------------------
    // actual finding/building is done by AOMMgr
    //--------------------------------------------------

    hr = m_pAOMMgr->GetAccessibleObjectFromID(this,lObjectID,ppAE);
        
    return(hr);

}

//-----------------------------------------------------------------------
//  CWindowAO::Notify()
//
//  DESCRIPTION:
//  handler for event notifications.
//
//  PARAMETERS:
//  idEvent - ID of event just fired.
//
//  RETURNS:
//  none
//
// ----------------------------------------------------------------------

void CWindowAO::Notify(DISPID idEvent)
{

    switch(idEvent)
    {
    case DISPID_HTMLWINDOWEVENTS_ONUNLOAD:
        if (!IsDetached())
        {
            // in the frame case it is possible that the window will already be 
            // detached, by the time this arrives.
             m_pDocAO->SetReadyToDetach( TRUE );

             // this causes ReleaseTridentInteraces to be called 
             //   on this entire tree();
             Zombify();
        }
        break;

    default:
        return;
    }
}


//-----------------------------------------------------------------------
//  CWindowAO::IsFocused()
//
//  DESCRIPTION:
//  determine if this window has the focus.
//  
//  PARAMETERS:
//  pbIsBrowserWindowFocused
//  pbIsThisWindowFocused
//
//  RETURNS:
//  
//  S_OK if it has the focus, else S_FALSE if it doesnt, else 
//  standard COM error.
// ----------------------------------------------------------------------

HRESULT CWindowAO::IsFocused( LPBOOL pbIsBrowserWindowFocused, LPBOOL pbIsThisWindowFocused )
{
    HWND    hwndCurrent     = NULL;
    HWND    hwndTarget      = NULL;


    *pbIsBrowserWindowFocused = FALSE;
    *pbIsThisWindowFocused = FALSE;


    //--------------------------------------------------
    // walk up parent chain looking for system focused
    // foreground window.
    //--------------------------------------------------

    hwndCurrent = m_hWnd;
    
    hwndTarget = GetForegroundWindow();

    while(hwndCurrent)
    {
        if(hwndTarget == hwndCurrent)
            break;
    
        hwndCurrent = ::GetParent(hwndCurrent);
    }

    //--------------------------------------------------
    // no focused window means that neither this 
    // window or any of its parent chain has the focus.
    // **NOTE** this handles uninitialized frames also.
    //--------------------------------------------------

    if ( hwndCurrent )
    {
        *pbIsBrowserWindowFocused = TRUE;

        *pbIsThisWindowFocused = (GetFocus() == m_hWnd);
    }


    return S_OK;
}


//================================================================================
// protected methods
//================================================================================


//-----------------------------------------------------------------------
//  CWindowAO::getIHTMLWindow2Ptr()
//
//  DESCRIPTION:
//
//      Obtains the Trident window interface pointer.
//
//  PARAMETERS:
//
//      pTridentWnd         pointer to IHTMLWindow2
//
//  RETURNS:
//
//      E_NOTIMPL
//
//  NOTES:
//
//      This method was essentially copied from the OLEACC API
//      AccessibleObjectFromWindow() defined in API.CPP.
// ----------------------------------------------------------------------

HRESULT CWindowAO::getIHTMLWindow2Ptr( IHTMLWindow2** ppTridentWnd, IHTMLDocument2** ppTridentDoc )
{
    HRESULT hr = E_FAIL;

    LRESULT     lRetVal, ref = 0;
    WPARAM      wParam;


    //------------------------------------------------
    //  Initialize the IHTMLWindow2 pointer to NULL.
    //------------------------------------------------

    *ppTridentWnd = NULL;
    *ppTridentDoc = NULL;

    //------------------------------------------------
    //  Ensure that the window handle points to a
    //  valid window (e.g., the Trident window could
    //  be closed before this method is called).
    //------------------------------------------------

    if ( IsWindow( m_hWnd ) )
    {
        wParam = WMOBJ_ID;
        
        //------------------------------------------------
        //  If the window is on our thread, optimize the
        //  marshalling/unmarshalling.
        //------------------------------------------------

        if ( GetWindowThreadProcessId( m_hWnd, NULL ) == GetCurrentThreadId() )
            wParam |= WMOBJ_SAMETHREAD;

        //------------------------------------------------
        //  Use SendMessageTimeout() to send the
        //  WM_HTML_GETOBJECT message to the Trident
        //  window so that it will return immediately
        //  if Trident is in a well "hung" state.
        //------------------------------------------------

        lRetVal = SendMessageTimeout( m_hWnd, m_nMsgHTMLGetObject, wParam, 0L,
                                      SMTO_ABORTIFHUNG, 10000, (LPDWORD)&ref );

        //------------------------------------------------
        //  If SendMessageTimeout() returns failure,
        //  return the associated Win32 error code.
        //------------------------------------------------

        if ( lRetVal == FALSE )
        {
            DWORD   dw = GetLastError();

            //--------------------------------------------
            // BUGBUG: return last error???
            //--------------------------------------------

            return( E_FAIL );
        }
        
        //------------------------------------------------
        //  If SendMessageTimeout() doesn't return error
        //  but Trident returns a failure code while
        //  processing the message, return the specified
        //  error code.
        //
        //  BUGBUG: Is this safe to type cast LRESULT to 
        //   HRESULT?
        //------------------------------------------------

        else if ( FAILED( (HRESULT)ref ) )
            return (HRESULT)ref;
        
        //------------------------------------------------
        //  If SendMessageTimeout() doesn't return error
        //  and Trident returns a positive LRESULT, assume
        //  that the LRESULT maps to the IHTMLWindow2*
        //  that has been marshalled via
        //  LresultFromObject().  To get the interface
        //  pointer, the LRESULT must be unmarshalled by
        //  calling ObjectFromLresult().
        //------------------------------------------------

        else if ( ref )
        {
            //--------------------------------------------------
            // [arunj 8/9/97] the message now returns 
            // IID_IHTMLDocument2 interface (post 1106 builds)
            // Now we get the IID_IHTMLWindow2 pointer from 
            // the document interface.
            //--------------------------------------------------
            
            HRESULT     hr;

            IUnknown*   pUnk = (IUnknown*) ref;
            pUnk->AddRef();
    
            hr = ObjectFromLresult( ref, IID_IHTMLDocument2, wParam, (void**) ppTridentDoc );

            if ( hr == E_NOINTERFACE )
                hr = getDocumentFromTridentHost( pUnk, ppTridentDoc );

            pUnk->Release();

            if ( hr != S_OK )
                return hr;


            assert( *ppTridentDoc );


            hr = (*ppTridentDoc)->get_parentWindow( ppTridentWnd );


            //--------------------------------------------------
            // assign input parameter to NULL upon failure
            //--------------------------------------------------

            if(hr != S_OK)
            {
                *ppTridentWnd = NULL;
            }
            else
            {
                return(hr);
            }
        

        }
    }

    //------------------------------------------------
    //  E_FAIL will be returned in the following
    //  cases:
    //
    //  1) The window handle is invalid.
    //
    //  2) SendMessageTimeOut() doesn't fail but the
    //  LRESULT is set to zero.  The LRESULT will be
    //  set to zero if the window receiving the
    //  message does not process it.  This could
    //  happen if Trident disappeared between the
    //  time IsWindow() and SendMessageTimeout()
    //  are called.
    //------------------------------------------------

    return( E_FAIL );
}


//-----------------------------------------------------------------------
//  CWindowAO::getDocumentFromTridentHost()
//
//  DESCRIPTION:
//
//  PARAMETERS:
//
//      IUnknown*           [in]
//      IHTMLDocument2**    [out]
//
//  RETURNS:
//
//      HRESULT
//
//  NOTES:
//
// ----------------------------------------------------------------------

HRESULT
CWindowAO::getDocumentFromTridentHost( /* in */ IUnknown* pUnk, /* out */ IHTMLDocument2** ppTridentDoc )
{
    HRESULT hr = E_NOINTERFACE;


    CComQIPtr<IHTMLDocument,&IID_IHTMLDocument> pIHTMLDocument(pUnk);

    if ( pIHTMLDocument )
    {
        CComPtr<IDispatch>  pDisp;

        hr = pIHTMLDocument->get_Script( &pDisp );

        if ( hr == S_OK )
        {
            assert( pDisp );

            CComQIPtr<IHTMLWindow2,&IID_IHTMLWindow2> pWindow( pDisp );

            if ( !pWindow )
                hr = E_NOINTERFACE;
            else
                hr = pWindow->get_document( ppTridentDoc );
        }
    }

    return hr;
}



//-----------------------------------------------------------------------
//  CWindowAO::registerHTMLGetObjectMsg()
//
//  DESCRIPTION:
//
//      Registers the private window message WM_HTML_GETOBJECT.  This
//      window message is used to communicate with Trident and retrieve
//      an IHTMLWindow2 pointer to the Trident window.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      S_OK if the private message is successfully or already
//      registered; E_FAIL, otherwise.
//
//  NOTES:
//
//      This method calls the Win32 API RegisterWindowMessage().
// ----------------------------------------------------------------------

HRESULT CWindowAO::registerHTMLGetObjectMsg( void )
{
    if ( m_nMsgHTMLGetObject == 0 )
    {
        m_nMsgHTMLGetObject = RegisterWindowMessage( MSGNAME_WM_HTML_GETOBJECT );

        if ( m_nMsgHTMLGetObject == 0 )
            return( E_FAIL );
    }

    return( S_OK );
}


//-----------------------------------------------------------------------
//  CWindowAO::createMemberObjects()
//
//  DESCRIPTION:
//  
//  creates member objects that need to be created at initialization time.
//  All derived classes must call this method before the CWindowAO::Init()
//  is called.
// 
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  S_OK or standard COM error
// ----------------------------------------------------------------------

HRESULT CWindowAO::createMemberObjects(void)
{
    // AOMMgr sticks around for the lifetime of this object.
    //--------------------------------------------------

    if(! (m_pAOMMgr = new CAOMMgr(this)) )
        return(E_OUTOFMEMORY);

    if(!(m_pDocAO = new CDocumentAO( this, 
            DUMMY_SOURCEINDEX, 
            m_pAOMMgr->GetAOMID(), 
            m_hWnd )))
        return(E_OUTOFMEMORY);

    //--------------------------------------------------
    // increment for next item (each item must have 
    // a unique AOMID.
    //--------------------------------------------------

    m_pAOMMgr->IncrementAOMID();


    //--------------------------------------------------
    // add document to child list of window
    // the doc needs to be around as long as we are so,
    //   addref it twice (1-us, 1-CtridentAO)
    //--------------------------------------------------

    AddChild(m_pDocAO);
    m_pDocAO->AddRef();  

    return(S_OK);
}




//----  End of WINDOW.CPP  ----
