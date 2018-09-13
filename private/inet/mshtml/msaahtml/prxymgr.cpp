//=======================================================================
//      File:   PRXYMGR.CPP
//      Date:   8/7/97
//      Desc:   Contains the implementation of CProxyManager class. CProxyManager
//              implements the Trident proxy manager object for the Trident 
//              MSAA Registered Handler. It is responsible for managing
//              multiple CWindowAOs (which own an entire AOM tree) --
//              one for each instance of Trident
//              running in a separate window.
//
//              [v-jaycl] CProxyManager now implements MSAA event
//              hooks for FOCUS and STATECHANGE events to track the
//              currently focused Trident object.
//      
//      Notes:  The CProxyManager class implements the IAccessibleHandler 
//              interface to handle all client requests for accessible
//              objects via a window handle/OBJID combination. 
//
//      Author: Jay Clark
//=======================================================================

//=======================================================================
//  Includes
//=======================================================================

#include "stdafx.h"
#include "focusdef.h"
#include "prxymgr.h"
#include "document.h"
#include "window.h"
#include "frame.h"
#include "userapi.h"

//================================================================================
//  globals variables
//================================================================================

static const TCHAR szUserDLL[]  = TEXT("USER32.DLL");

    //-----------------------------------------------
    // Synchronization objects.
    //-----------------------------------------------

static  CRITICAL_SECTION        s_CritSectWindowList;


//--------------------------------------------------
// initializes userdll api storage struct.
//--------------------------------------------------

INIT_USERAPI_STRUCTURE()


//================================================================================
//  External variables
//================================================================================

extern  CProxyManager   *g_pProxyMgr;

extern LONG g_cLocks; // global dll lock, for memory management and dll unloading

//================================================================================
//  Global Data
//================================================================================

#ifdef _DEBUG
ULONG g_uObjCount = 0;  // Global object count for all derived objects
#endif


//================================================================================
//  Static functions
//================================================================================

//-----------------------------------------------------------------------
//  EventNofityProc()
//
//  DESCRIPTION:
//      This is our MSAA event callback used to trap FOCUS and
//      STATECHANGE events fired from Trident upon focus and blur (unfocus)
//      events, respectively. This is necessary to track the Trident
//      object that has the current focus.  Upon trapping one of these
//      MSAA events, the appropriate CDocumentAO object is updated with
//      the ID of the focused Trident object.  This focus state is then
//      used in the implementation of CTridentAO::GetAccState() to 
//      accurately set the focus state for an AO or AE.
//
//      NOTE: CProxyManager established two (2) event hooks that both
//      use this event notify procedures:  one for FOCUS events and
//      one for STATECHANGE events.  These two event hooks are shared
//      by *all* Trident windows.
//
//      CAVEAT: Due to the synchronous nature of MSAA events, MSAA clients 
//      must register their FOCUS/STATECHANGE event hooks *before* 
//      the proxy does (when ProxyManager is created) to ensure that
//      MSAAHTML receives these events and saves the current focused
//      object before the client requests the focus state of that object.  
//
//  PARAMETERS:
//      
//      See MSAA 1.0 API reference for details.
//
//  RETURNS:
// ----------------------------------------------------------------------

void __stdcall EventNotifyProc( HWINEVENTHOOK hEvent, 
                                 DWORD event, 
                                 HWND hwnd, 
                                 LONG idObject, 
                                 LONG idChild,
                                 DWORD idProcess, 
                                 DWORD idThread)
{
    HRESULT         hr = E_FAIL;
    CDocumentAO*    pDocAO = NULL;


    //----------------------------------------------------
    // If the event was fired from within one of the 
    // windows we are currently proxying, then find that 
    // window and update it's CDocumentAO with the ID of 
    // the currently focused object
    //----------------------------------------------------

    if ( g_pProxyMgr )
    {
        hr = g_pProxyMgr->GetDocumentChildOfWindow( hwnd, &pDocAO );
    
        if ( hr == S_OK && pDocAO )
        {
#ifdef _DEBUG
            TCHAR   str[MAX_PATH];
#endif

            //---------------------------------------------
            // Ignore all MSAA system object events 
            //---------------------------------------------

            if ( idObject <= 0 )
                return;

            //---------------------------------------------
            // For FOCUS events, save the focused object's ID.
            // For STATECHANGE (unfocus) events, indicate
            //  that no object is currently focused.
            //---------------------------------------------

            switch (event)
            {
                //---------------------------------------------
                // WARNING: Possible concurrency conflict. Since
                //  this event proc can be called by numerous
                //  out of process instances of Trident, calls
                //  into this proc are not guaranteed to be 
                //  synchronized w/ interprocess IAccessible calls 
                //  from an MSAA client.
                //  In such cases, we might be calling 
                //  CDocumentAO:SetFocusedItem to set a member
                //  data item at the same time CTridentAO:GetAccState
                //  might be accessing the focused object member 
                //  data item.  While not a catastrophic 
                //  consequence, it could result in GetAccState()
                //  reporting incorrect focus state.
                //---------------------------------------------

                case EVENT_OBJECT_FOCUS :
#ifdef _DEBUG
                    wsprintf( str, _T("EventNotifyProc: EVENT_OBJECT_FOCUS caught with idObject = %d, idChild = %d.\n"), idObject, idChild );
                    OutputDebugString( str );
#endif
                    pDocAO->SetFocusedItem( idObject );
                    break;

                case EVENT_OBJECT_STATECHANGE : 
#ifdef _DEBUG
                    wsprintf( str, _T("EventNotifyProc: EVENT_OBJECT_STATECHANGE caught with idObject = %d, idChild = %d.\n"), idObject, idChild );
                    OutputDebugString( str );
#endif
                    pDocAO->SetFocusedItem( NO_TRIDENT_FOCUSED );
                    break;
            }
        }
    }


    return;
}


//================================================================================
// CProxyManager public methods : IAccessibleHandler implementation
//================================================================================

//-----------------------------------------------------------------------
//  CProxyManager::AccessibleObjectFromID()
//
//  DESCRIPTION:
//      This method returns a pointer to an accessible object associated with 
//      a specified window handle and identified with a specific OBJID.
//
//  PARAMETERS:
//
//      hWnd            handle of window in which the object resides.
//      lObjectID       OBJID of the accessible object to be retrieved.
//                      This can be a standard or custom OBJID.
//      ppvObject       Pointer to specified accessible object if found.
//
//  RETURNS:
//
//      NOERROR is OK, else a COM error.
//
//  NOTES:
//
//      The proxy manager's window list will contain only one CWindowAO
//      per proxied HWND.  This window and its tree will either be
//      attached or attached but detachable ("ready to detach").
//      Detached windows are removed from the window list: CWindowAOs
//      via CProxyManager::DetachReadyTrees(), CFrameAOs via
//      CWindowAO::Detach().
//
// ----------------------------------------------------------------------

STDMETHODIMP CProxyManager::AccessibleObjectFromID( /* in */    long hWnd, 
                                                    /* in */    long lObjectID, 
                                                    /* out */   LPACCESSIBLE *ppIAccessible )
{
    HRESULT         hr          = S_OK;
    CAccElement     *pAE        = NULL;
    CWindowAO       *pWindowAO  = NULL;
    BOOL            fWeCreated  = FALSE;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert(hWnd && ppIAccessible);

    if(!hWnd)
        return(E_INVALIDARG);

    if(!ppIAccessible)
        return(E_INVALIDARG);

    if ( !IsWindow( (HWND)hWnd ) )
        return( E_FAIL );

    *ppIAccessible = NULL;

    //------------------------------------------------
    // Find correct CWindowAO based on hWnd, then
    //  see if it contains the requested accessible
    //  object.
    //------------------------------------------------

    std::list<CWindowAO *>::iterator    itCurPos;


    EnterCriticalSection( &s_CritSectWindowList );

    DetachReadyTrees();

    itCurPos = m_windowList.begin();

    while ( itCurPos != m_windowList.end() )
    {
        assert( *itCurPos );

        if ((*itCurPos)->GetWindowHandle() == (HWND)hWnd )
        {
            if ( (*itCurPos)->DoBlockForDetach() )
            {
                //------------------------------------------------
                //  If DoBlockForDetach() returns TRUE, it means
                //  that this window has been detached and has
                //  been removed from the proxy manager's window
                //  list.  Drop out of the loop and try to create
                //  a new window for the requested HWND.
                //------------------------------------------------

                break;
            }
            else
            {
                CDocumentAO*    pDoc = NULL;

                if ( hr = (*itCurPos)->GetDocumentChild( &pDoc ) )
                    break;

                //------------------------------------------------
                //  If the TOM document.readyState is not set to
                //  complete, detach the tree rooted by this
                //  CWindowAO and create another CWindowAO.
                //------------------------------------------------

                if ( pDoc->IsTOMDocumentReady() )
                    pWindowAO = (CWindowAO *)*itCurPos;
                else
                {
                    pDoc->SetReadyToDetach( TRUE );
                    ((CWindowAO *)*itCurPos)->Zombify();
                }

                hr = S_OK;
                break;
            }
        }

        itCurPos++;
    }


    LeaveCriticalSection( &s_CritSectWindowList );


    if ( hr == S_OK && !pWindowAO )
    {
        //------------------------------------------------
        // If not found, then consider creating a new one
        //------------------------------------------------

        hr = CreateAOMWindow( (HWND)hWnd, NULL, NULL, 0, 0, &pWindowAO );
        fWeCreated = TRUE;
    }

    if ( hr != S_OK )
        return hr;

    assert( pWindowAO );

    //------------------------------------------------
    // Now find the accessible object. The lObjectID
    // will be non zero if this method was called 
    // as the result of an AccessibleObjectFromEvent()
    // call.
    //------------------------------------------------

    hr = pWindowAO->GetAccessibleObjectFromID(lObjectID, &pAE);

    if(hr != S_OK)
        goto ErrorCase;

    //--------------------------------------------------
    // if the pAE corresponding to the ID doesn't 
    // implement IAccessible, then it must be an
    // Accessible Element.  Return its parent's IAccessible
    // interface or ERROR if the call fails.
    //--------------------------------------------------
    hr = pAE->QueryInterface(IID_IAccessible, (LPVOID *)ppIAccessible);

    if(hr != S_OK)
    {
        if(hr == E_NOINTERFACE)
            hr = pAE->GetAccParent((IDispatch **)ppIAccessible);

        if (hr)
            goto ErrorCase;
    }

    return( hr );

ErrorCase:
    // (Carled) there are some timing issues between incontext event 
    // handling aides and the events fired from trident. Specifically,
    // trident fires the CP events first (e.g. onunload which zombifies 
    // the tree) and then the accEvent (which an aide may turn around with
    // and ask for accessibleObjectFromEvent).  In cases like this, the tree
    // is detached at the top of this function, and then potentially accessed
    // in the middle (e.g. refresh in which the HWND is constant) and then
    // if the new WindowAO fails to initialize properly all later access is blocked
    // because of the partially built tree. 
    //      what we want to do is, in the case of a late failure in this fx,
    // we want to remove the window we just created.
    // a failure asking for one of the predeifined objects shouldn't cause a cleanup
    if (FAILED(hr) && 
        !pWindowAO->IsDetached() &&
        fWeCreated)
    {
        // in the frame case it is possible that the window will already be 
        // detached, by the time this arrives.
        CDocumentAO*    pDoc = NULL;

        if ( (S_OK == pWindowAO->GetDocumentChild( &pDoc ))
            && pDoc )
            pDoc->SetReadyToDetach( TRUE );

         // this causes ReleaseTridentInteraces to be called 
         //   on this entire tree();
         pWindowAO->Zombify();
    }

    return hr;
}

//-----------------------------------------------------------------------
//  CProxyManager::QueryInterface()
//
//  DESCRIPTION:
//
//      QI implementation for embedded object : return this pointer for 
//      IAccessibleHandler, else delegate to parent.
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

STDMETHODIMP CProxyManager::QueryInterface(REFIID riid, void** ppv)
{

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    if(!ppv)
        return(E_INVALIDARG);

    if (riid == IID_IUnknown)
        *ppv = (LPACCESSIBLEHANDLER)this;

    else if (riid == IID_IAccessibleHandler) 
        *ppv = (LPACCESSIBLEHANDLER)this;

    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return(NOERROR);
}

//-----------------------------------------------------------------------
//  CProxyManager::AddRef()
//
//  DESCRIPTION:
//
//      See online help
//
//  PARAMETERS:
//
//  RETURNS:
//
//      Reference count AFTER increment.
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CProxyManager::AddRef(void)
{
    InterlockedIncrement( &g_cLocks );

    return( ++m_cRef );
}

//-----------------------------------------------------------------------
//  CProxyManager::Release()
//
//  DESCRIPTION:
//
//      See online help.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      Reference count AFTER decrement.
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CProxyManager::Release(void)
{ 
    if ( m_cRef == 0 )
    {
        delete this;
        return 0;
    }
    else if (m_cRef > 0)
    {
        InterlockedDecrement( &g_cLocks );
        m_cRef--;
    }
    else
    {
        assert(FALSE);
    }

    return m_cRef;
}


//================================================================================
// CProxyManager public methods : non OLE interfaces
//================================================================================

//-----------------------------------------------------------------------
//  CProxyManager::CProxyManager()
//
//  DESCRIPTION:
//      Constructor. Also registers two (2) MSAA event hooks to track
//      the currently focused Trident object.  These event hooks need
//      to exist for the lifetime of MSAAHTML.DLL, thus they are managed
//      by the CProxyManager.
//
//      NOTE: If the registration of either MSAA event hook fails, then
//      anchor accessible objects will never report that their state is 
//      focused.
//
//  PARAMETERS:
//  none.
//
//  RETURNS:
//
//  n/a
// ----------------------------------------------------------------------

CProxyManager::CProxyManager(void)
{
    m_cRef                  = 0;
    m_hUserMod              = NULL;
    m_hFocusEventHook       = NULL;
    m_hStateChangeEventHook = NULL;


    InitializeCriticalSection( &s_CritSectWindowList );

    setMSAAEventSinks();
}


//-----------------------------------------------------------------------
//  CProxyManager::~CProxyManager()
//
//  DESCRIPTION:
//      Destructor.  Frees all sub objects and unhooks from MSAA
//      WinEvent notifications for focus and statechange events.
//
//  PARAMETERS:
//  none.
//
//  RETURNS:
//
//  n/a
// ----------------------------------------------------------------------

CProxyManager::~CProxyManager()
{
    g_pProxyMgr = NULL;


    removeMSAAEventSinks();


    //------------------------------------------------------------------
    //  ARCHITECTURAL QUAGMIRE (CARLED):
    //
    //  freeAllMemory is commented out to prevent a crash. why?
    //  1. Because detaching the tree causes ->Release()'s to be called
    //      on pointers to trident interfaces, and these calls are NOT 
    //      valid while in DLL_PROCESS_DETACH hadling.
    //  2. We are DTOR'ing in DLL_PROCESS_DETACH handling because our 
    //      refcount is initialized to 1 (should be 0) so that the lifetime
    //      of the proxymgr and thus its tree, is the same as the dll. This
    //      is necessary since the client is accessing and immediately 
    //      freeing the pointers/objects.  If we where hard-ref driven teh 
    //      proxymgr/tree/objects would be created and released with every
    //      access.  This is bad, so we keep the tree around until shut
    //      down. and let the memeory and ref-counts to trident be cleaned
    //      up by windows. 
    // This is bad, and is an architectural flaw.  The solution is:
    // 1. the ProxyMgr's lifetime, and thus its tree, should be ref-count 
    //      based. This means that the client would access the window, and
    //      hang onto it, until the query sequence was completed, and then
    //      all the pointers would be freed. this would release all the 
    //      objects.  The problem with this is that:
    //              A. It requirs client to change the way they use AA stuff
    //              B. It is performance intensive to be building and releaseing
    // 2. So, a better solution is to have the ProxyMgr and its trees as internal
    //      data only, The implementation of IAccessibleHandler should be a 
    //      different object, AND a new method needs to be added to tell MSAA
    //      that the client is done, and all internal memory can be cleaned up.
    //      (think OleInitialize/OleUnInitialize style sequences).  This way
    //      Clients can continue with their "weak-ref" style accessing (bkwrd
    //      compatibility).  All they would need to do is before shuting down, 
    //      call AAUnititialize(), or AADisconnect(), or whatever you want to 
    //      name it.  This would give MSAA an opportunity to clean up its memory 
    //      and release the trident pointers and do whatever else it needs to.
    //              A. to do this, uncomment freeAllMemory and set the m_cRefs 
    //                  to 0 in the CTOR and add teh logic for AAUninitialize().
    //--------------------------------------------------------------------
    //
    // freeAllMemory();


    DeleteCriticalSection( &s_CritSectWindowList );
}

//-----------------------------------------------------------------------
//  CProxyMgr::CreateAOMWindow()
//
//  DESCRIPTION:
//  creates new CWindowAO, which is the root of an AOM tree.
//
//  PARAMETERS:
//
//  hwndToProxy     hwnd that the window will proxy.
//  pParent         pointer to parent node.
//  pIHTMLElement   pointer to IHTMLElement interface of frame.
//  ppAccElem       pointer to return to calling object.
//
//  RETURNS:
//
//  S_OK | standard COM error code.
// ----------------------------------------------------------------------

HRESULT CProxyManager::CreateAOMWindow(     /* in */    HWND            hwndToProxy,
                                            /* in */    CTridentAO      * pParent,
                                            /* in */    IHTMLElement    * pIHTMLElement,
                                            /* in */    long            lTOMIndex,
                                            /* in */    long            lAOMID,
                                            /* out */   CWindowAO       **ppWindowAO)
{
    HRESULT hr = E_FAIL;
        
    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------
    
    assert(ppWindowAO);

    if(!ppWindowAO)
        return E_INVALIDARG;

    *ppWindowAO = NULL;


    if(!hwndToProxy)
    {
        //--------------------------------------------------
        // no window means that we must be creating a 
        // frame element.
        //--------------------------------------------------

        if(!(*ppWindowAO = new CFrameAO(this,pParent,lTOMIndex,lAOMID) ))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        CComPtr<IUnknown> pIUnknown;

        assert(pIHTMLElement);

        if(hr = pIHTMLElement->QueryInterface(IID_IUnknown,(void **)&pIUnknown) )
            goto Cleanup;

        //--------------------------------------------------
        // initalize frames internal data members.
        //--------------------------------------------------

        if(hr = ((CFrameAO *)(*ppWindowAO))->Init(pIUnknown) )
            goto Cleanup;
    }
    else
    {
        //--------------------------------------------------
        // supplied HWND means that we must be proxying
        // a window.
        //--------------------------------------------------

        if(!(*ppWindowAO = new CWindowAO(this,pParent,lTOMIndex,lAOMID,hwndToProxy) ))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        //--------------------------------------------------
        // initalize windows internal data members. 
        //--------------------------------------------------

        if (hr = (*ppWindowAO)->Init(NULL))
            goto Cleanup;
    }

    //--------------------------------------------
    // Add window/frame to list of proxied Trident windows
    //--------------------------------------------

    EnterCriticalSection( &s_CritSectWindowList );

    m_windowList.push_back( *ppWindowAO );
    
    LeaveCriticalSection( &s_CritSectWindowList );


Cleanup:
    if ( hr != S_OK && *ppWindowAO )
    {
        (*ppWindowAO)->Detach();
        *ppWindowAO = NULL;
    }


    return hr;
}
                           
//-----------------------------------------------------------------------
//  CProxyManager::DestroyAOMWindow()
//
//  DESCRIPTION:
//      This method removes the CWindowAO from the window list and deletes 
//      it.  it is central to the window unload logic.
//                                                                        
//  PARAMETERS:
//  pWindowAOProxyToFree    CWindowAO object to release and destroy.
//
//  RETURNS:
//
//  S_OK | E_FAIL
// ----------------------------------------------------------------------

HRESULT CProxyManager::DestroyAOMWindow(    /* in */    CWindowAO * pWindowAOToFree)
{
    HRESULT     hr = E_FAIL;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pWindowAOToFree );

    if(!pWindowAOToFree)
        return(E_INVALIDARG);

    std::list<CWindowAO *>::iterator    itCurPos;


    EnterCriticalSection( &s_CritSectWindowList );

    itCurPos = m_windowList.begin();

    //--------------------------------------------------
    // since there is one unique CWindowAO for each
    // proxy, deleting the first found CWindowAO and
    // then leaving is valid.
    //--------------------------------------------------

    while(itCurPos != m_windowList.end())
    {
        assert( *itCurPos );

        if (*itCurPos == pWindowAOToFree)
        {

            m_windowList.erase(itCurPos);

            //--------------------------------------------------
            // free the window (this is not done by the erase()
            //--------------------------------------------------
            if (! pWindowAOToFree->IsDetached())
                pWindowAOToFree->Detach();

            hr = S_OK;
            break;
        }

        itCurPos++;
    }

    //--------------------------------------------------
    // should never get here (the only callers of this
    // method should be valid CWindowAO pointers)
    //--------------------------------------------------

    if ( hr != S_OK )
        assert(*itCurPos == pWindowAOToFree);

    LeaveCriticalSection( &s_CritSectWindowList );


    return hr;
}


//-----------------------------------------------------------------------
//  CProxyManager::DetachreadyTrees
//
//  DESCRIPTION: This function deals with multiple instances of Trident 
//      and possible refresh/navigation scenarios in which a tree is built, 
//      detached, but never accessed again to delete it.  If in our gatekeeper
//      tests we find any tree that is ready to be detached, then we claim
//      that the whole m_windowList should be walked and cleared out of any
//      other trees that have been marked readToDetached but have not been.
//
//  PARAMETERS: none
//
//  RETURNS: none
//
// ----------------------------------------------------------------------

void 
CProxyManager::DetachReadyTrees()
{

    //--------------------------------------------------
    // iterate through list and detach any vestigial trees.
    //--------------------------------------------------

    std::list<CWindowAO *>::iterator    itCurPos;

    //--------------------------------------------------
    // the way windows cleanup after themselves, it is likely
    // that we will loose list members as this is processed.
    //--------------------------------------------------

    EnterCriticalSection( &s_CritSectWindowList );

    while (m_windowList.size())
    {
        // start at the beginning
        itCurPos = m_windowList.begin();

        //----------------------------------------------------
        // only top level windows get detached and removed
        // from the window list here.  if the window has no
        // parent, it's a top level window.  (otherwise, the
        // window is a frame; it will be detached and removed
        // from the window list if its parent calls DetachChildren()
        // or it will be gutted and reinited when its own
        // DoBlockForDetach() is called.)
        //----------------------------------------------------

        // skip through all the active, non-frame rooted trees.
        while ( itCurPos != m_windowList.end() &&
                ( (!(*itCurPos)->GetDocumentAO()->CanDetachTreeNow()) || 
                  (*itCurPos)->GetParent()) )
        {
            itCurPos++;
        }

        //----------------------------------------------------
        // if we are at the end of the list, we're done
        // it this element is Detachable, we don't have to 
        // do it right now. since it will get picked up in 
        // a later pass or on shut down.
        //----------------------------------------------------
        if (itCurPos == m_windowList.end())
            break;

        //----------------------------------------------------
        // must be at a top level window that is ready
        // to be detached.
        //
        // the window list will be cleared out, leaving
        // this loop in a very different state. in which 
        // our itCurPos is potentially invalid.  We need to start
        // back at the beginning and look for others (though generally
        // there shouldn't be, and this list should be SMALL (<10))
        //----------------------------------------------------
        assert((*itCurPos)->GetDocumentAO()->CanDetachTreeNow());
        assert(!(*itCurPos)->GetParent());


        CWindowAO * pWindowAOToFree = (*itCurPos);

        m_windowList.erase(itCurPos);

        //--------------------------------------------------
        // free the window (this is not done by the erase()
        //--------------------------------------------------
        if (! pWindowAOToFree->IsDetached())
            pWindowAOToFree->Detach();
    
    }

    LeaveCriticalSection( &s_CritSectWindowList );

}


//-----------------------------------------------------------------------
//  CProxyManager::GetDocumentChildOfWindow
//
//  DESCRIPTION: 
//      Returns the document AO of a window AO associated with a given hwnd.
//
//  PARAMETERS: 
//      hwnd        handle of the window specifying the windowAO to obtain 
//                  the documentAO.
//      ppDocAO     out pointer containing the retrieved documentAO pointer
//
//  RETURNS: S_OK | S_FALSE
//
// ----------------------------------------------------------------------

HRESULT CProxyManager::GetDocumentChildOfWindow( /* in */  HWND hwnd,
                                                 /* out */ CDocumentAO** ppDocAO )
{
    HRESULT hr = S_FALSE;

    assert( ppDocAO );

    *ppDocAO = NULL;


    //--------------------------------------------------
    //  We will walk the window list looking for a
    //  match.  If one is found, we will return a
    //  pointer to that window's CDocumentAO child.
    //--------------------------------------------------

    std::list<CWindowAO *>::iterator    itCurPos;

    EnterCriticalSection( &s_CritSectWindowList );

    itCurPos = m_windowList.begin();

    while ( itCurPos != m_windowList.end() )
    {
        assert( *itCurPos );

        if ( ((CWindowAO *) *itCurPos)->GetWindowHandle() == (HWND)hwnd )
        {
            hr = ((CWindowAO *) *itCurPos)->GetDocumentChild( ppDocAO );
            break;
        }

        itCurPos++;
    }

    LeaveCriticalSection( &s_CritSectWindowList );


    return hr;
}


//================================================================================
// CProxyManager protected methods
//================================================================================

//-----------------------------------------------------------------------
//  CProxyManager::freeAllMemory
//
//  DESCRIPTION:
//
//      Frees all internal data members, including the list of CWindowAOs.
//
//  PARAMETERS:
//
//  RETURNS:
//
// ----------------------------------------------------------------------

void 
CProxyManager::freeAllMemory( void )
{
    std::list<CWindowAO *>::iterator    itCurPos;

    //--------------------------------------------------
    // iterate through list and clean up all memory.
    //--------------------------------------------------

    EnterCriticalSection( &s_CritSectWindowList );

    //--------------------------------------------------
    // the way windows cleanup after themselves, it is likely
    // that we will loose list members as this is processed.
    // so what we do here is:
    //   1. as long as there are windows to detach, go to the
    //         head of the list.
    //   2. search for the next undetached window (this should
    //          ALWAYS be the first one (Head), but I put the
    //          loop in to break potential infinite loops
    //   3. Detach that window. This causes all other sibling
    //          and children windows to be removed from the
    //          proxymanager's list (via RemoveWindow() ). 
    //   4. loop.
    //--------------------------------------------------

    while(m_windowList.size())
    {
        itCurPos = m_windowList.begin();

        // paranoia loop. should not enter
        if ((*itCurPos)->IsDetached())
        {
            assert(FALSE && "logic error: detached window in prxymgr");
            m_windowList.erase(itCurPos);
        }
        else
        {
            //--------------------------------------------------
            // Detach each window, and its associated subtrees
            // the PrxyManager. 
            //--------------------------------------------------

            (*itCurPos)->Detach();  
        }
    }

    LeaveCriticalSection( &s_CritSectWindowList );

#ifdef _DEBUG
    if(g_uObjCount != 0)
    {

#ifndef NDEBUG
        TCHAR szBuf[140];
        wsprintf(szBuf, _T("Object count is %lu. ONLY if mshtml has already been shut down are these leaks. Do you want to break?"), g_uObjCount);
        int nRet = MessageBox(NULL, szBuf, _T("MEMORY LEAKS"), MB_YESNO | MB_ICONSTOP);
        if(nRet == IDYES)
        {
            #ifdef _M_IX86
                __asm int 3;
            #else
                DebugBreak();
            #endif // _M_IX86
        }
#endif // NDEBUG
    }
#endif
}
    
//-----------------------------------------------------------------------
//  CProxyManager::loadUserDLLs()
//
//  DESCRIPTION:
//  loads the user APIs that we call. 
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  BOOL TRUE if good load.
// ----------------------------------------------------------------------


BOOL CProxyManager::loadUserDLLs(void)
{
    BOOL    bLoadSuccess = TRUE;


    m_hUserMod = LoadLibrary( szUserDLL );


    if ( m_hUserMod )
    {
        for ( int i = 0; i < SIZE_USERAPI; i++ )
        {
            USERAPI_LPFN(i) = GetProcAddress( m_hUserMod, USERAPI_NAME(i) );

            if ( USERAPI_LPFN(i) == NULL )
            {
                bLoadSuccess = FALSE;
                break;
            }
        }
    }
    else
        bLoadSuccess = FALSE;


    if ( !bLoadSuccess && m_hUserMod )
    {
        FreeLibrary( m_hUserMod );
        m_hUserMod = NULL;
    }

    return( bLoadSuccess );
}

//-----------------------------------------------------------------------
//  CProxyManager::unloadUserDLLs()
//
//  DESCRIPTION:
//
//  unloads user32.lib
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  none.
// ----------------------------------------------------------------------

void CProxyManager::unloadUserDLLs(void)
{
    if ( m_hUserMod)
        FreeLibrary( m_hUserMod);

}


//-----------------------------------------------------------------------
//  CProxyManager::setMSAAEventSinks()
//
//  DESCRIPTION:
//
//      Establishes the MSAA event sinks for EVENT_OBJECT_FOCUS and
//      EVENT_OBJECT_STATECHANGE.
//
//      In IE4.01, these events as fired by Trident must be handled so
//      that the proxy can accurately reflect the focused object.
//      (In Trident of IE4.01 there is a bug such that a focused link
//      will not show up as the document.activeElement.)
//
//      Sinking MSAA events is not necessary for IE5 and better.
//
//  PARAMETERS:
//
//      none.
//
//  RETURNS:
//
//      none.
//
// ----------------------------------------------------------------------

inline void CProxyManager::setMSAAEventSinks( void )
{
    //--------------------------------------------------
    // load User APIs for event hooking : if it fails,
    // dont load hooks
    //--------------------------------------------------

    if(!loadUserDLLs())
        return;

    //--------------------------------------------------
    //  Set event hook for FOCUS events.
    //
    //  We don't want WINEVENT_SKIPOWNPROCESS because
    //  it's most likely that MSAAHTML will be loaded
    //  into the same process as Trident and those are
    //  the very events we need to catch.
    //--------------------------------------------------

    m_hFocusEventHook =  SetWinEventHook( EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, EventNotifyProc,
                                            0, 0, WINEVENT_OUTOFCONTEXT );

    if ( !m_hFocusEventHook )
        return;

    //--------------------------------------------------
    //  Set event hook for STATE CHANGE events
    //--------------------------------------------------

    m_hStateChangeEventHook =  SetWinEventHook( EVENT_OBJECT_STATECHANGE, EVENT_OBJECT_STATECHANGE, NULL, EventNotifyProc,
                                            0, 0, WINEVENT_OUTOFCONTEXT );

    if ( !m_hStateChangeEventHook )
    {
        UnhookWinEvent( m_hFocusEventHook );
        m_hFocusEventHook = NULL;
    }
}


//-----------------------------------------------------------------------
//  CProxyManager::removeMSAAEventSinks()
//
//  DESCRIPTION:
//
//      Unhooks the MSAA event sinks for EVENT_OBJECT_FOCUS and
//      EVENT_OBJECT_STATECHANGE.
//
//  PARAMETERS:
//
//      none.
//
//  RETURNS:
//
//      none.
// ----------------------------------------------------------------------

void CProxyManager::removeMSAAEventSinks( void )
{
    //--------------------------------------------------
    // Unhook the focus and statechange event hooks
    //--------------------------------------------------

    if ( m_hFocusEventHook )
        UnhookWinEvent( m_hFocusEventHook );

    if ( m_hStateChangeEventHook )
        UnhookWinEvent( m_hStateChangeEventHook );

    unloadUserDLLs();
}



//----  End of PRXYMGR.CPP  ----
