//================================================================================
//              File:   FRAME.CPP
//              Date:   7/27/97
//              Desc:   Contains implementation of CFrameAO class.  CFrameAO 
//                              implements the accessible proxy for the Trident Table 
//                              object.
//================================================================================


//================================================================================
// Includes
//================================================================================

#include "stdafx.h"
#include "trid_ao.h"
#include "window.h"
#include "aommgr.h"
#include "document.h"
#include "prxymgr.h"
#include "frame.h"                              


//================================================================================
// global vars
//================================================================================
    
static const TCHAR szAccClassName[] = TEXT("Internet Explorer_Server");


//================================================================================
// defines and macros
//================================================================================

#define FAIL_IF_DETACHED if (!m_pAOContainer || \
                             m_pAOContainer->DoBlockForDetach())\
                                return E_FAIL;


//================================================================================
// CImplIUnknown class definition
//================================================================================

class CImplIUnknown : public IUnknown
{
public:

    //--------------------------------------------------
    // IUnknown
    //--------------------------------------------------
    
    virtual STDMETHODIMP        QueryInterface(REFIID riid, void** ppv);
    virtual STDMETHODIMP_(ULONG)    AddRef(void);
    virtual STDMETHODIMP_(ULONG)    Release(void);
    
    //--------------------------------------------------
    // CImplIUnknown creation/destruction/maintenance 
    //--------------------------------------------------
    
    CImplIUnknown(IUnknown * pIUnknown,CTridentAO * pAOContainer);
    ~CImplIUnknown();

    protected:

    //--------------------------------------------------
    // methods
    //--------------------------------------------------

    //------------------------------------------------
    // Members
    //------------------------------------------------

    IUnknown    * m_pIUnknown;
    CTridentAO  * m_pAOContainer;
};


//=======================================================================
// CImplIUnknown class implementation : public methods
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIUnknown::CImplIUnknown()
//
//  DESCRIPTION:
//
//      class constructor.
//
//  PARAMETERS:
//
//      pIUnknown           pointer to owner IUnknown.
//      pAOContainer        pointer to owner object
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CImplIUnknown::CImplIUnknown( IUnknown* pIUnknown, CTridentAO* pAOContainer )
{
    //------------------------------------------------
    // Assign controlling IUnknown.
    //------------------------------------------------
    
    m_pIUnknown     = pIUnknown;
    
    //------------------------------------------------
    // Assign AO container (parent) pointer.
    //------------------------------------------------

    m_pAOContainer      = pAOContainer;
}


//-----------------------------------------------------------------------
//  CImplIUnknown::~CImplIUnknown()
//
//  DESCRIPTION:
//
//      class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CImplIUnknown::~CImplIUnknown()
{

}


//=======================================================================
// CImplIUnknown : IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIUnknown::QueryInterface()
//
//  DESCRIPTION:
//
//  only implements IUnknown interface.
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

STDMETHODIMP CImplIUnknown::QueryInterface(REFIID riid, void** ppv)
{
    if ( !ppv )
        return E_INVALIDARG;


    *ppv = NULL;


    FAIL_IF_DETACHED;


    if ( riid == IID_IUnknown )
    {
        *ppv = (IUnknown*)this;
        AddRef();
        return NOERROR;
    }
    else
        return m_pIUnknown->QueryInterface(riid,ppv);
}


//-----------------------------------------------------------------------
//  CImplIUnknown::AddRef()
//
//  DESCRIPTION:
//
//  Handled in owner class
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//  reference count AFTER increment.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImplIUnknown::AddRef( void )
{
    //------------------------------------------------
    // delegate ref counting to owner
    //------------------------------------------------

    return m_pIUnknown->AddRef();
}

//-----------------------------------------------------------------------
//  CImplIUnknown::Release()
//
//  DESCRIPTION:
//
//  Handled in owner class.
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//  reference count AFTER decrement.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImplIUnknown::Release( void )
{
    //------------------------------------------------
    // delegate ref counting to owner
    //------------------------------------------------

    return m_pIUnknown->Release();
}


//================================================================================
// CFrameAO public methods
//================================================================================

//-----------------------------------------------------------------------
//      CFrameAO::CFrameAO()
//
//      DESCRIPTION:
//
//              Constructor
//
//      PARAMETERS:
//
//              pAOParent       [IN]    Pointer to the parent accessible object in 
//                                                      the AOM tree
//
//              nTOMIndex       [IN]    Index of the element from the TOM document.all 
//                                                      collection.
//              
//              hWnd            [IN]    Pointer to the window of the trident object that 
//                                                      this object corresponds to.
//      RETURNS:
//
//              None.
// ----------------------------------------------------------------------

CFrameAO::CFrameAO(CProxyManager * pProxyMgr,CTridentAO *pAOParent,UINT nTOMIndex,UINT nChildID)
: CWindowAO(pProxyMgr,pAOParent, nTOMIndex,nChildID, NULL)
{

    //------------------------------------------------
    // Assign the delegating IUnknown to CFrameAO :
    //  this member will be overridden in derived class
    //  constructors so that the delegating IUnknown 
    //  will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown = NULL;
    m_pImplIUnknown = NULL;


    //--------------------------------------------------
    // set the type to distinguish this object from 
    // CWindowAO objects.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_FRAME;


#ifdef _DEBUG

    lstrcpy(m_szAOMName,_T("FrameAO"));

#endif

}

//-----------------------------------------------------------------------
//      CFrameAO::Init()
//
//      DESCRIPTION:
//
//              Initialization : set values of data members
//
//      PARAMETERS:
//
//              pTOMObjIUnk             [IN]    Pointer to IUnknown of TOM object
//              pHTMLDocIUnk    [IN]    Pointer to IUnkown cast of IHTMLDocument2 
//                                                              pointer.
//
//      RETURNS:
//
//              S_OK | E_FAIL | E_NOINTERFACE
// ----------------------------------------------------------------------


HRESULT CFrameAO::Init(IUnknown * pTOMObjIUnk)
{
    HRESULT hr;


    //--------------------------------------------------
    // Validate input parameter.
    //--------------------------------------------------

    assert( pTOMObjIUnk );
    
    if ( !pTOMObjIUnk )
        return E_INVALIDARG;


    //--------------------------------------------------
    // at this point, a frame should 
    // (1) not have an HWND yet and 
    // (2) always have a parent.
    //--------------------------------------------------

    assert(!m_hWnd);

    if(m_hWnd)
        return(E_FAIL);

    assert(m_pParent);

    if(!m_pParent)
        return(E_FAIL);


    //--------------------------------------------------
    // set internal IUnknown pointer to passed in 
    // IUnknown pointer -- this enables the object
    // to be referenced as a FRAME element when we are
    // looking for a match.
    //--------------------------------------------------

    m_pTOMObjIUnk = pTOMObjIUnk;

    m_pTOMObjIUnk->AddRef();


    //--------------------------------------------------
    // Cache the FRAME's IHTMLElement*.  This will be
    //      Released by the CTridentAO destructor.
    //--------------------------------------------------

    if ( !m_pIHTMLElement )
        if ( hr = m_pTOMObjIUnk->QueryInterface( IID_IHTMLElement, (void**) &m_pIHTMLElement ) )
            return hr;

    assert( m_pIHTMLElement );


    //--------------------------------------------------
    // Map our internal IUnknown to the IUnknown
    // implementing object.  We need to do this because
    // our CTridentAO base class uses the internal
    // IUnknown pointer.
    //--------------------------------------------------

    if ( hr = createIUnknownImplementor() )
    return hr;

    m_pIUnknown = m_pImplIUnknown;

    //--------------------------------------------------
    // need to create interface implementor objects here.
    //--------------------------------------------------
    
    if(hr = createInterfaceImplementors(m_pImplIUnknown))
        return(hr);

    //--------------------------------------------------
    // need to create AOMMgr and Document here.
    //--------------------------------------------------
    
    if(hr = createMemberObjects())
        return(hr);

    //--------------------------------------------------
    // try to initialize the frame: if this fails, frame 
    // initialization will be re-attempted at every 
    // entry point.
    //--------------------------------------------------

    if(hr = initializeFrame())
    {
        if(hr != S_FALSE)
            return(hr);

        assert(!m_hWnd);
    }

    return(S_OK);
}



//-----------------------------------------------------------------------
//      CFrameAO::~CFrameAO()
//
//      DESCRIPTION:
//
//              CFrameAO class destructor.
//
//      PARAMETERS:
//
//      RETURNS:
// ----------------------------------------------------------------------

CFrameAO::~CFrameAO()
{
    //------------------------------------------------
    // Cleanup contained IUnknown implementor.
    // Set to NULL for safety.
    //------------------------------------------------

    if ( m_pImplIUnknown )
    {
    delete m_pImplIUnknown;
    m_pImplIUnknown = NULL;
    }

}               


//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//      CFrameAO::QueryInterface()
//
//      DESCRIPTION:
//
//              Standard QI implementation : the CFrameAO object only implements
//              IUnknown.
//
//      PARAMETERS:
//
//              riid    [IN]    REFIID of requested interface.
//              ppv             [OUT]   Pointer to requested interface pointer.
//
//      RETURNS:
//
//              E_NOINTERFACE | NOERROR.
// ----------------------------------------------------------------------

STDMETHODIMP CFrameAO::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;

    if(!ppv)
    return(E_INVALIDARG);

    *ppv = NULL;


    if (riid == IID_IUnknown)
    {
    assert( m_pImplIUnknown );

    *ppv = (LPUNKNOWN)m_pImplIUnknown;
    }
    else
    return(CWindowAO::QueryInterface(riid,ppv));
    
    ((LPUNKNOWN) *ppv)->AddRef();

    return(NOERROR);
}


//================================================================================
// CFrameAO Accessible interface methods
//================================================================================

//-----------------------------------------------------------------------
//      CFrameAO::GetAccName()
//
//      DESCRIPTION:
//
//              Returns name of object/element, which is synthesized.
//
//      PARAMETERS:
//
//              lChild          child ID / Self ID
//              pbstrName       returned name.
//              
//      RETURNS:
//
//              E_NOTIMPL : default element implementation doesn't support children.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccName(long lChild, BSTR * pbstrName)
{
    HRESULT hr = E_FAIL;

    //------------------------------------------------
    //      Validate the parameters.
    //------------------------------------------------

    assert(pbstrName);

    if(!pbstrName)
        return(E_INVALIDARG);

    *pbstrName = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if(hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccName(lChild, pbstrName));
}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccDescription()
//
//      DESCRIPTION:
//
//              Returns description string to client.
//
//              TODO: Does Window have a description???
//
//      PARAMETERS:
//
//              lChild                          Child/Self ID
//
//              pbstrDescription        Description string returned to client.
//      
//      RETURNS:
//
//              S_OK if success, else E_FAILED.
// ----------------------------------------------------------------------
    
HRESULT CFrameAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
    HRESULT hr = E_FAIL;

    //------------------------------------------------
    //      Validate the parameters.
    //------------------------------------------------

    assert(pbstrDescription);

    if(!pbstrDescription)
        return(E_INVALIDARG);

    *pbstrDescription = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.  If window initialization fails because
    // window is offscreen, we can still return a 
    // description for this window.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccDescription(lChild, pbstrDescription));
}

//----------------------------------------------------------------------- 
//      CFrameAO::GetAccFocus()
//
//      DESCRIPTION:
//
//              Get the object that has the focus, return the type of the focused 
//              object to the user. Since the main TOM document overlays the window,
//              return the document's IDispatch pointer if it or any of its children
//              are focused. The window itself never has the focus.
//
//              NOTE: is this ever called?  Need to verify, otherwise remove.
//
//      PARAMETERS:
//
//              ppIUnknown : pointer to IUnknown of returned object. This object
//                                       can be a CAccElement or a CAccObject.
//                                       QI for IAccessible to find out.
//
//      RETURNS:
//
//              HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccFocus(IUnknown **ppIUnknown)
{
    HRESULT hr = E_FAIL;

    //------------------------------------------------
    //      Validate the parameters.
    //------------------------------------------------

    assert( ppIUnknown );

    if ( !ppIUnknown )
        return E_INVALIDARG;

    *ppIUnknown = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------


    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccFocus(ppIUnknown));
}

//-----------------------------------------------------------------------
//      CFrameAO::AccLocation()
//
//      DESCRIPTION:
//              
//      returns location of frame element.
//
//      PARAMETERS:
//
//              pxLeft          left coord pointer.
//              pyTop           top coord pointer.
//              pcxWidth        width 
//              pcxHeight       height
//              lChild          child ID
//
//      RETURNS:
//
//              S_OK | E_NOINTERFACE | E_FAIL
// ----------------------------------------------------------------------

HRESULT CFrameAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
    
    HRESULT hr = E_FAIL;
    CDocumentAO * pOldDocAO = NULL;

    //------------------------------------------------
    //      Validate the parameters.
    //------------------------------------------------

    assert( pxLeft && pyTop && pcxWidth && pcyHeight );

    if ( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
        return(E_INVALIDARG);

    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // substitute m_pDocAO with parents m_pDocAO to 
    // get the location of the frame in the context
    // of its parent window.
    //--------------------------------------------------

    pOldDocAO = m_pDocAO;
    
    m_pDocAO = m_pParent->GetDocumentAO();

    hr = CTridentAO::AccLocation(pxLeft,pyTop,pcxWidth,pcyHeight,lChild);

    //--------------------------------------------------
    // restore original m_pDocAO pointer.
    //--------------------------------------------------

    m_pDocAO = pOldDocAO;

    return(hr);
}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccState()
//
//      DESCRIPTION:
//
//      returns state of frame to caller.
//
//      PARAMETERS:
//
//              lChild  child ID
//              plState pointer to store state var in.
//              
//      RETURNS:
//
//              S_OK | E_NOINTERFACE | E_FAIL
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccState(long lChild, long *plState)
{
    HRESULT hr = E_FAIL;

    //------------------------------------------------
    //      Validate the parameters.
    //------------------------------------------------

    assert(plState);

    if(!plState)
        return(E_INVALIDARG);

    //--------------------------------------------------
    // state is invisible until proven otherwise.
    //--------------------------------------------------

    *plState  = STATE_SYSTEM_INVISIBLE;     

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
        {

            //--------------------------------------------------
            // if initialization fails, return OK with INVISIBLE
            // bit set.  Otherwise notify user of more serious 
            // error.
            //--------------------------------------------------

            if(hr == S_FALSE)
                return(S_OK);
            else
                return(hr);
        }
            
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccState(lChild, plState));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccSelection()
//
//      DESCRIPTION:
//
//              tries to init frame if frame is not initialized, otherwise delegates
//              to base class.
//
//
//      PARAMETERS:
//              
//              ppIUnknown      pointer to an IUnknown*
//
//      RETURNS:
//
//              HRESULT         DISP_E_MEMBERNOTFOUND
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccSelection( IUnknown** ppIUnknown )
{
    HRESULT hr = E_FAIL;    

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    if(!ppIUnknown)
        return(E_INVALIDARG);

    *ppIUnknown = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccSelection( ppIUnknown ));

}


//-----------------------------------------------------------------------
//      CFrameAO::AccNavigate()
//
//      DESCRIPTION:
//      
//      handles navigation.  If frame has been fully initialized,
//  navigation is delegated to the base class.  If not, 
//  sibling navigation is the only type of navigation supported.
//
//  PARAMETERS:
//
//      see base class method.
//
//      RETURNS:
//
//      see base class method.
// ----------------------------------------------------------------------

HRESULT CFrameAO::AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown)
{
    HRESULT hr = E_FAIL;


    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!ppIUnknown)
        return(E_INVALIDARG);
    
    *ppIUnknown = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
        {
            if(hr != S_FALSE)
                return(hr);

            //------------------------------------------------
            // navigate in relation to the object 
            // itself or one of its specified children
            //------------------------------------------------

            if ( lStart == CHILDID_SELF )
            {
                //------------------------------------------------
                // Navigate w/ respect to the object, so only
                //      first/lastchild navigation is valid
                //------------------------------------------------

                switch( navDir )
                {

                    //--------------------------------------------------
                    // do not support navigation into children if
                    // the std accessible object proxy has not
                    // been created yet.
                    //--------------------------------------------------

                    case NAVDIR_FIRSTCHILD :
                    case NAVDIR_LASTCHILD :
                        return(S_FALSE);

                    case NAVDIR_NEXT :
                    case NAVDIR_PREVIOUS :
                        
                        //--------------------------------------------------
                        // sibling navigation: we need to delegate to parent
                        // passing this object's child ID to the parent.
                        //--------------------------------------------------

                        if(m_pParent)
                            return(m_pParent->AccNavigate(navDir,GetChildID(),ppIUnknown));
                        else
                            return(S_FALSE);
                        
                    default :

                        return( DISP_E_MEMBERNOTFOUND );
                }
            }
            else
            {
                //--------------------------------------------------
                // we dont support navigation into children objects.
                //--------------------------------------------------

                return(S_FALSE);
            }
        }

        assert(m_hWnd);

    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::AccNavigate(navDir,lStart,ppIUnknown));
}


//-----------------------------------------------------------------------
//      CFrameAO::AccHitTest()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::AccHitTest(long xLeft, long yTop,IUnknown **ppIUnknown)
{
    HRESULT hr = E_FAIL;
    
    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!ppIUnknown)
        return(E_INVALIDARG);

    *ppIUnknown = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::AccHitTest(xLeft,yTop,ppIUnknown));

}


//-----------------------------------------------------------------------
//      CFrameAO::GetAccParent()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccParent(IDispatch ** ppdispParent)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!ppdispParent)
        return(E_INVALIDARG);

    *ppdispParent = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
        {
            if(hr != S_FALSE)
                return(hr);
        }
        
    }
    
    //--------------------------------------------------
    // delegate to base class even if initializeFrame()
    // fails.
    //--------------------------------------------------

    return(CWindowAO::GetAccParent(ppdispParent));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccChildCount()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccChildCount(long* pChildCount)
{

    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!pChildCount)
        return(E_INVALIDARG);

    *pChildCount = 0;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccChildCount(pChildCount));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccChild()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccChild(long lChild, IDispatch ** ppdispChild)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!ppdispChild)
        return(E_INVALIDARG);

    *ppdispChild = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccChild(lChild,ppdispChild));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccValue()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccValue(long lChild, BSTR * pbstrValue)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!pbstrValue)
        return(E_INVALIDARG);

    *pbstrValue = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
        {
            return(hr);
        }

    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccValue(lChild,pbstrValue));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccHelp()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccHelp(long lChild, BSTR * pbstrHelp)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!pbstrHelp)
        return(E_INVALIDARG);

    *pbstrHelp = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccHelp(lChild,pbstrHelp));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccHelpTopic()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!pbstrHelpFile)
        return(E_INVALIDARG);

    if(!pidTopic)
        return(E_INVALIDARG);

    *pbstrHelpFile = NULL;
    *pidTopic = 0;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccHelpTopic(pbstrHelpFile,lChild,pidTopic));

}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccKeyboardShortcut()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!pbstrKeyboardShortcut)
        return(E_INVALIDARG);

    *pbstrKeyboardShortcut = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccKeyboardShortcut(lChild,pbstrKeyboardShortcut));


}

//-----------------------------------------------------------------------
//      CFrameAO::GetAccDefaultAction()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::GetAccDefaultAction(long lChild, BSTR * pbstrDefAction)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate parameters
    //--------------------------------------------------

    if(!pbstrDefAction)
        return(E_INVALIDARG);

    *pbstrDefAction = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::GetAccDefaultAction(lChild,pbstrDefAction));

}

//-----------------------------------------------------------------------
//      CFrameAO::AccSelect()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
//
//      NOTES:
//
//      If the Trident FRAME/IFRAME is still offscreen when this method is
//      called, we focus the FRAME/IFRAME object.  This will scroll it into
//      view.  Subsequent IAccessible requests to the CFrameAO will be able
//      to initialize the object because it will now have an HWND.
//      If this Trident FRAME/IFRAME is or has been onscreen when this
//      method is called, we will focus the Trident window object associated
//      with the FRAME/IFRAME by calling CWindowAO::AccSelect().
// ----------------------------------------------------------------------

HRESULT CFrameAO::AccSelect(long flagsSel, long lChild)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // only SELFLAG_TAKEFOCUS is supported.
    //--------------------------------------------------

    if ( !(flagsSel & SELFLAG_TAKEFOCUS) )
        return E_INVALIDARG;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
        {
            if(hr != S_FALSE)
                return(hr);
            
            //--------------------------------------------------
            // The Trident FRAME is offscreen, so focus the
            //      control element.
            //--------------------------------------------------

            CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement( m_pTOMObjIUnk );

            assert( pIHTMLControlElement );

            if ( !pIHTMLControlElement )
                return E_NOINTERFACE;

            return pIHTMLControlElement->focus();
        }
    }


    //--------------------------------------------------
    // delegate to base class
    //--------------------------------------------------

    return CWindowAO::AccSelect(flagsSel,lChild);
}

//-----------------------------------------------------------------------
//      CFrameAO::AccDoDefaultAction()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::AccDoDefaultAction(long lChild)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------
    
    return(CWindowAO::AccDoDefaultAction(lChild));

}

//-----------------------------------------------------------------------
//      CFrameAO::SetAccName()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::SetAccName(long lChild, BSTR bstrName)
{

    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    if(!bstrName)
        return(E_INVALIDARG);

    bstrName = NULL;

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::SetAccName(lChild,bstrName));
    
}


//-----------------------------------------------------------------------
//      CFrameAO::SetAccValue()
//
//      DESCRIPTION:
//      
//      override of base class  to handle uninit object scenario
//      
//  PARAMETERS:
//
//      see base class.
//
//      RETURNS:
//
//      see base class.
// ----------------------------------------------------------------------

HRESULT CFrameAO::SetAccValue(long lChild, BSTR bstrValue)
{
    HRESULT hr = E_FAIL;

    bstrValue = NULL;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    if(!bstrValue)
        return(E_INVALIDARG);

    //--------------------------------------------------
    // if no window, try to find the window proxied by 
    // this object, and complete initialization 
    // in order to get a fully functioning
    // object.
    //--------------------------------------------------

    if(!m_hWnd)
    {
        if( hr = initializeFrame())
            return(hr);
    }

    assert(m_hWnd);

    //--------------------------------------------------
    // delegate to base class.
    //--------------------------------------------------

    return(CWindowAO::SetAccName(lChild,bstrValue));

}


//-----------------------------------------------------------------------
//  CFrameAO::DoBlockForDetach ()
//
//  DESCRIPTION:
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

BOOL CFrameAO::DoBlockForDetach( void )
{
    if ( GetParent()->IsDetached() )
    {
        //--------------------------------------------------
        //  If the parent is detached, the frame should be
        //  detached, too.
        //--------------------------------------------------

        if ( !IsDetached() )
           Detach();

        return TRUE;
    }

    //--------------------------------------------------
    //  Assert that the frame was not detached if its parent
    //  was not.  This would happen if the frame was
    //  marked for detach and the subsequent reinit
    //  failed (see comment below).  Or, it could
    //  indicate a logic error; namely, that the ready
    //  tree detaching mechanism in the proxy manager
    //  isn't sufficently screening for frames.  Throw
    //  an assertion just in case.
    //--------------------------------------------------
    assert( !IsDetached() && "possible logic error in CFrameAO::DoBlockForDetach()" );


    //--------------------------------------------------
    //  Is the frame marked for detach, but its parent
    //  not detached?  (This happens in the frame
    //  navigation case.)  If so, gut the frame and
    //  re-initialize it.  If the reinit fails, detach
    //  the frame and return TRUE to block.
    //--------------------------------------------------

    if ( m_pDocAO->CanDetachTreeNow() )
    {
        DetachChildren();

        cleanupMemberData();

        if ( reInit() != S_OK )
        {
            Detach();
            return TRUE;
        }
    }

    return FALSE;
}


void CFrameAO::Zombify()
{
    std::list<CAccElement *>::iterator itCurPos;

    //--------------------------------------------------
    // while there is a child list, Zombify all my children.
    //--------------------------------------------------

    itCurPos = m_AEList.begin();

    while( itCurPos != m_AEList.end() )
    {
        (*itCurPos)->Zombify();

        itCurPos++;
    }

    //--------------------------------------------------
    // Only if our parent is zombified do we want to
    // be zombified.  Frame navigation is a case where
    // the parent won't be zombified at this point.
    // Since we want to preserve some of the frame AO's
    // state in frame-only navigation, we don't release
    // our Trident ifaces here.  That will be done
    // in DoBlockForDetach() or Detach().
    //--------------------------------------------------

    if ( m_pParent->IsZombified() )
    {
        m_bZombified = TRUE;

        ReleaseTridentInterfaces();
    }
}


//================================================================================
// protected methods
//================================================================================

//-----------------------------------------------------------------------
//  CFrameAO::createIUnknownImplementor()
//
//  DESCRIPTION:
//
//      Creates the member object that implements IUnknown.
//
//  PARAMETERS:
//
//      none.
//
//  RETURNS:
//
//      HRESULT     S_OK | E_OUTOFMEMORY
//
//  NOTES:
//
//      If a derived class does not call CFrameAO::Init(), it
//      must call this method to create the CImplIUnknown member.
//      (CFrameAO::Init() calls this method internally.)
//
//-----------------------------------------------------------------------

HRESULT CFrameAO::createIUnknownImplementor( void )
{
    HRESULT hr = S_OK;


    if ( !m_pImplIUnknown )
    {
    m_pImplIUnknown = new CImplIUnknown( this, this );

    if ( !m_pImplIUnknown )
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



//-----------------------------------------------------------------------
//      CFrameAO::initializeFrame()
//
//      DESCRIPTION:
//      attempts to initialize frame by finding a valid hwnd to proxy, then
//  finishing initialization of Frame.  
//
//  PARAMETERS:
//      none.
//
//      RETURNS:
//
//      S_OK if good init, else S_FALSE if init fails, else std COM error.
//
//  NOTE:
//      S_FALSE is a valid return code in the sense that it means initialization
//  failed in a non critical way.  Use S_FALSE to delay initialization : other
//  wise, propagate the more serious error return to the user.
// ----------------------------------------------------------------------

HRESULT CFrameAO::initializeFrame(void)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // the internal TOM IUnk ptr HAS to have been set
    // before this method is called.
    //--------------------------------------------------

    assert(m_pTOMObjIUnk);

    //--------------------------------------------------
    // m_hWnd should ALWAYS be NULL at this point
    //--------------------------------------------------

    assert(!m_hWnd);

    //--------------------------------------------------
    // get the new window handle that this CWindowAO
    // is proxying.
    //--------------------------------------------------

    if ( hr = findTridentWindow( m_pParent->GetWindowHandle(), &m_hWnd ) )
        return hr;

    //--------------------------------------------------
    // if a window handle was successfully found, continue
    // base class initialization.
    //--------------------------------------------------

    assert(m_hWnd);

    return(CWindowAO::Init(NULL));
}

//-----------------------------------------------------------------------
//      CWindowAO::findTridentWindow()
//
//      DESCRIPTION:
//      uses Win32 API to drill into child window encapsulated by the element.
//  uses center of window as drilling point if no POINT specified.
//
//      PARAMETERS:
//
//      hWnd                            pointer to owner window
//      phwndFrame                      pointer to return drilled window in.
//
//      RETURNS:
//
//      S_OK if window is found, S_FALSE if not, else standard COM error.
//
//  NOTE:
//      S_FALSE is a valid return code in the sense that it means initialization
//  failed in a non critical way.  Use S_FALSE to tell caller to recover from
//  this failure : more serious errors should be propagated to the user.
// ----------------------------------------------------------------------

HRESULT CFrameAO::findTridentWindow(    /* in */        HWND hWndParent,
                                        /* out */       HWND * phWndFound)
{
    TCHAR   tchClassName[MAX_PATH];
    POINT   ptDrill;
    HWND    hWndDrilledTo;
    HRESULT hr          = E_FAIL;
    long    xLeft       = 0;
    long    yTop        = 0;
    long    cxWidth     = 0;
    long    cyHeight    = 0;
    long    xClientLeft = 0;
    long    yClientTop  = 0;
    BOOL    bLookingForTridentClassWnd  = TRUE;
    HWND    hWndCurrentParent = hWndParent;
    DWORD   dwCorner        = 0;
    long xOffset            = 0;
    long yOffset            = 0;
    
    //--------------------------------------------------
    // validate input parameters & preconditions
    //--------------------------------------------------

    assert( hWndParent );
    assert( phWndFound );
    assert(!( m_hWnd ));
    
    assert(m_pParent);

    *phWndFound = NULL;

    assert(m_pTOMObjIUnk);

    //--------------------------------------------------
    // get a valid point on the window.
    //--------------------------------------------------

    if(hr = getVisibleCorner(&ptDrill,&dwCorner))
        goto Cleanup;

    //--------------------------------------------------
    // N.B. we add so that frames hit correctly. this is safe
    // since even if a frame is sized  0,0 it still has 
    // borders, and if the borders are 0, then we won't hit 
    // it anyhow.
    //
    // Use the returned dwCorner to determine which 
    // corner was returned, and base offset values from
    // that corner.
    //--------------------------------------------------
    
    switch(dwCorner & POINT_XMASK)
    {
    case POINT_XLEFT:
        xOffset = 2;
        break;
    case POINT_XRIGHT:
        xOffset = -2;
        break;
    case POINT_XMID:
        break;
    }

    switch(dwCorner & POINT_YMASK)
    {
    case POINT_YTOP:
        yOffset = 2;
        break;
    case POINT_YBOTTOM:
        yOffset = -2;
        break;
    case POINT_YMID:
        break;
    }
    
    ptDrill.x += xOffset;
    ptDrill.y += yOffset;

    while(bLookingForTridentClassWnd)
    {
    // prepare incase of errors
    hr = E_FAIL;

    if(!(ScreenToClient(hWndCurrentParent,&ptDrill) ))
        goto Cleanup;

        //--------------------------------------------------
        // get the first encapsulating child that matches
        // the Internet Explorer_Server class. If there
        // is no window from the point, that means that the
        // frame is completely offscreen.  Return S_FALSE
        // to delay initialization until the frame is 
        // onscreen.
        //
        // TODO: is this check necessary now that we
        // are checking for location above ?
        //--------------------------------------------------

    hWndDrilledTo = ChildWindowFromPoint(hWndCurrentParent,ptDrill);

        if(!hWndDrilledTo)
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    
        //--------------------------------------------------
        // if the window returned is == to the input window,
        // return S_FALSE to delay initialization (because 
        // initialization will fail at this point).
        //--------------------------------------------------

    if(hWndDrilledTo == hWndCurrentParent)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        //--------------------------------------------------
        // if we have a valid window that is not == to the 
        // parent window, we have successfully drilled down.
        // start evaluating class names.
        //--------------------------------------------------

    if(!(GetClassName( hWndDrilledTo,tchClassName,MAX_PATH) ))
        goto Cleanup;

        if(!(_tcscmp(tchClassName, szAccClassName) ))
            bLookingForTridentClassWnd  = FALSE;

        if(!(ClientToScreen(hWndCurrentParent,&ptDrill) ))
        goto Cleanup;

        //--------------------------------------------------
        // reset current parent
        //--------------------------------------------------

        hWndCurrentParent = hWndDrilledTo;
    }

    //--------------------------------------------------
    // Frame always proxies a window, so the window 
    // that is found should never == the parent window
    // passed in.
    //
    //  Getting to this point means we successfully found
    //   the hwnd
    //--------------------------------------------------

    assert(hWndDrilledTo != hWndParent);

    hr = S_OK;
    *phWndFound = hWndDrilledTo;
    
Cleanup:
    return( hr );
}

//-----------------------------------------------------------------------
//      CFrameAO::getVisibleCorner()
//
//      DESCRIPTION:
//      override of base class method : temporarily
//  replaces m_pDocAO with parent m_pDocAO to
//  get the visible corner in the context of 
//  the parent element.
//      
//      please see CTridentAO implementation of this method
//  for more details.
//
//  PARAMETERS:
//
//      pPt                     pointer to POINT structure to store point value in.
//      pdwCorner       pointer to DWORD to contain bitmask in.  Bitmask 
//                              corresponds to which corner of the window is present, 
//
//      RETURNS:
//
//      please see CTridentAO implementation for details. 
// ----------------------------------------------------------------------

HRESULT CFrameAO::getVisibleCorner(POINT * pPt,DWORD  *pdwCorner)
{
    
    HRESULT hr = E_FAIL;

    CDocumentAO * pOldDocAO = NULL;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------
    
    assert(pPt);
    assert(pdwCorner);

    //--------------------------------------------------
    // if this method is called w/o parent, just fail.
    //--------------------------------------------------

    assert(m_pParent);

    if(!m_pParent)
        return(E_FAIL);
        
    //--------------------------------------------------
    // set the internal document pointer to the parent
    // document pointer in order to get the visible corner
    // (if any) of this element in the context of its 
    // parent window. Then reset it to the original value
    //--------------------------------------------------
    
    pOldDocAO = m_pDocAO;

    m_pDocAO = m_pParent->GetDocumentAO();

    hr = CTridentAO::getVisibleCorner(pPt,pdwCorner);

    m_pDocAO = pOldDocAO;

    return(hr);

}


//-----------------------------------------------------------------------
//  CFrameAO::cleanupMemberData()
//
//  DESCRIPTION:
//
//  Deletes and/or resets various CAccElement, CAccObject, CTridentAO,
//  CWindowAO and CFrameAO data members.  The data members are those
//  that are specific to a particular instance of a Trident FRAME or
//  IFRAME.
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  none.
//-----------------------------------------------------------------------

void CFrameAO::cleanupMemberData( void )
{
    //--------------------------------------------------
    // Cleanup CAccElement specific data.
    //--------------------------------------------------

    m_hWnd = NULL;

    //--------------------------------------------------
    // Cleanup CAccObject specific data.
    //--------------------------------------------------

    //--------------------------------------------------
    // Cleanup CTridentAO specific data.
    //--------------------------------------------------

    if ( m_pDocAO )
        m_pDocAO->Release();

    //--------------------------------------------------
    // free all allocated strings.
    //--------------------------------------------------

    if(m_bstrName)
    {
        SysFreeString(m_bstrName);
        m_bstrName=NULL;
    }

    if(m_bstrValue)
    {
        SysFreeString(m_bstrValue);
        m_bstrValue = NULL;
    }

    if(m_bstrDescription)
    {
        SysFreeString(m_bstrDescription);
        m_bstrDescription = NULL;
    }

    if(m_bstrDefaultAction)
    {
        SysFreeString(m_bstrDefaultAction);
        m_bstrDefaultAction = NULL;
    }
    
    if(m_bstrKbdShortcut)
    {
        SysFreeString(m_bstrKbdShortcut);
        m_bstrKbdShortcut = NULL;
    }

    if(m_bstrStyle)
    {
        SysFreeString(m_bstrStyle);
        m_bstrStyle = NULL;
    }

    if(m_bstrDisplay)
    {
        SysFreeString(m_bstrDisplay);
        m_bstrDisplay = NULL;
    }

    //--------------------------------------------------
    // release allocated interface pointers
    //--------------------------------------------------

    if(m_pIHTMLStyle)
    {
        m_pIHTMLStyle->Release();
        m_pIHTMLStyle = NULL;       
    }

    //--------------------------------------------------
    // reset pertinent non-pointer data members
    //--------------------------------------------------

    m_bOffScreen                    = FALSE;
    m_bResolvedState                = FALSE;
    m_bDetached                     = FALSE;
    m_bNameAndDescriptionResolved   = FALSE;

    m_cache.CleanAll();


    //--------------------------------------------------
    // Cleanup CWindowAO specific data.
    //--------------------------------------------------

    if(m_pAOMMgr)
    {
        delete m_pAOMMgr;
        m_pAOMMgr = NULL;
    }

    if(m_pIHTMLWindow2)
    {
        m_pIHTMLWindow2->Release();
        m_pIHTMLWindow2 = NULL;
    }

    DESTROY_NOTIFY_EVENT_HANDLER(ImplIHTMLWindowEvents);

    //--------------------------------------------------
    // Cleanup CFrameAO specific data.
    //--------------------------------------------------

}


//-----------------------------------------------------------------------
//  CFrameAO::reInit()
//
//  DESCRIPTION:
//
//  Re-initializes the CFrameAO.
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  HRESULT     S_OK | standard COM error (e.g. E_OUTOFMEMORY).
//-----------------------------------------------------------------------

HRESULT CFrameAO::reInit( void )
{
    HRESULT hr;

    hr = createMemberObjects();

    if ( hr == S_OK )
    hr = initializeFrame();

    return hr;
}



//----  End of FRAME.CPP  ----
