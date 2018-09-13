//=======================================================================
//      File:   EVENT.CPP
//      Date:   7/24/97
//      Desc:   Contains implementation of CEvent class, an abstrast base
//              base defining the general interface for derived event
//              handling classes for specific AOs and AEs.
//
//      Author: Jay Clark
//=======================================================================

//=======================================================================
//  Includes
//=======================================================================

#include "stdafx.h"
#include "trid_ae.h"
#include "trid_ao.h"
#include "event.h"

//=======================================================================
//  Defines
//=======================================================================

//================================================================================
// const value initialization
//================================================================================

const _EVENT_MAP_ENTRY_ CEvent::m_event_mapEntries[] = 
{   
    { _EVENT_MAP_END_,0,0 } 
};

const _EVENT_MAP_ CEvent::m_event_map = 
{ 
    NULL,&CEvent::m_event_mapEntries[0] 
};


//================================================================================
// CEvent implementation : public methods
//================================================================================

//--------------------------------------------------------------------------------
// CEvent::CEvent()
//
//  DESCRIPTION:
//
//      constructor
//
//  PARAMETERS:
//
//      pIUnknown       pointer to controlling IUnknown interface.
//      
//      riid            IID to establish connection point with
//
//      nContainerChildID   AOM ID of containing object.
//
//  RETURNS:
//
//      None.
//--------------------------------------------------------------------------------

CEvent::CEvent()
{
    //assert( pIUnknown );

    m_pIUnknown         = NULL; 


    //--------------------------------------------------
    // init the event IID to IUnknown : this will cause
    // a FAIL in the Init of the calling class unless
    // the event class is initialized with the correct
    // IID.
    //--------------------------------------------------

    m_eventIID          = IID_IUnknown;
    m_hWnd              = NULL;
    m_nContainerChildID = 0;    
    m_nParentAOID       = 0;

                            
    m_pIConnectionPoint = NULL;
    m_dwAdviseCookie    = 0;

    m_bAccessibleObject = FALSE;

    m_cRef = 1;
}

//--------------------------------------------------------------------------------
// CEvent::~CEvent()
//
//  DESCRIPTION:
//
//      destructor
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//--------------------------------------------------------------------------------

CEvent::~CEvent()
{
    if (m_pIConnectionPoint)
        Detach();
}
    
//+-------------------------------------------------
// 
//  Member : Detach (); 
//
//  Synopsis : when the event sink is detached from the 
//      AO/AE then it should do the unadvise to terminate 
//      the advise at the other end 
//
//--------------------------------------------------
void
CEvent::Detach()
{
    if (m_pIConnectionPoint)
    {
#ifdef _MSAA_EVENTS
        //--------------------------------------------------
        // notify MSAA that this object is being destroyed
        //--------------------------------------------------

        FireMSAAEvent(EVENT_OBJECT_DESTROY);
#endif
        //--------------------------------------------------
        // Unadvise() only if the cookie has 
        // been set in an earlier call to Advise()
        //--------------------------------------------------

        if(m_dwAdviseCookie)
            m_pIConnectionPoint->Unadvise(m_dwAdviseCookie);

        m_pIConnectionPoint->Release();
        m_pIConnectionPoint = NULL;
    }
}

//--------------------------------------------------------------------------------
// CEvent::Init()
//
//  DESCRIPTION:
//
//      This initialization method establishes an Advise session with the
//      TOM window object so that this object can receive event notifications. 
//      **NOTE** we store the IConnectionPoint interface as a standard COM ptr
//      because we need it for the lifetime of the object and it is a lot more
//      explicit to release it during object destruction.
//
//  PARAMETERS:
//
//      pTOMObjUnk          pointer to TOM object unknown to query for
//                          connection point.
//  RETURNS:
//
//      S_OK if success else COM error.
//--------------------------------------------------------------------------------  

HRESULT CEvent::Init(IUnknown *pIUnknown, REFIID riid, HWND hwnd,UINT nContainerChildID, LPUNKNOWN pTOMObjIUnk)
{
    HRESULT hr;


    assert( pIUnknown );
    assert( pTOMObjIUnk );

    if ( !pTOMObjIUnk || !pTOMObjIUnk )
        return(E_INVALIDARG);


    m_pIUnknown         = pIUnknown; 
    m_eventIID          = riid;
    m_hWnd              = hwnd;
    m_nContainerChildID = nContainerChildID;


    //--------------------------------------------------
    // is our containing object an AO or an AE ?
    //--------------------------------------------------

    CComQIPtr<IAccessible,&IID_IAccessible> pIAccessible(pIUnknown);

    if(!(pIAccessible))
        m_bAccessibleObject = FALSE;
    else
        m_bAccessibleObject = TRUE;



    //--------------------------------------------------
    // if the containing object is an AO, then set 
    // m_nParentAOID to its ID.  Otherwise, find the
    // ParentAO and get its ID.
    //--------------------------------------------------

    if(m_bAccessibleObject)
    {
        m_nParentAOID = m_nContainerChildID;
    }
    else
    {
        CTridentAE * pAE = (CTridentAE *)pIUnknown;

        CTridentAO * pParentAO = pAE->GetParent();

        m_nParentAOID = pParentAO->GetChildID();
    }


    //--------------------------------------------------
    // Establish an advise with the TOM window object 
    //--------------------------------------------------

    CComQIPtr<IConnectionPointContainer,&IID_IConnectionPointContainer> pIConnectionPointContainer(pTOMObjIUnk);

    if (!pIConnectionPointContainer)
        return(E_NOINTERFACE);

    if ( !SUCCEEDED(hr = pIConnectionPointContainer->FindConnectionPoint(m_eventIID, &m_pIConnectionPoint)) )
        return( hr );

    if (!m_pIConnectionPoint)
        return(E_NOINTERFACE);

    //--------------------------------------------------
    // Enlist for event notifications from the TOM
    //  button object.
    //
    // BUGBUG: do we need to cast the this pointer to 
    //  the appropriate derived class before casting to
    //  LPUNKNOWN??
    //--------------------------------------------------
    
    if ( !SUCCEEDED(hr = m_pIConnectionPoint->Advise((IDispatch *)this, &m_dwAdviseCookie)) )
        return( hr );

    if (!m_dwAdviseCookie)
        return(E_FAIL);

    //--------------------------------------------------
    // Notify MSAA that this object has been created.
    //--------------------------------------------------
#ifdef _MSAA_EVENTS

    FireMSAAEvent( EVENT_OBJECT_CREATE );

#endif

    return(S_OK);
}


//--------------------------------------------------------------------------------
// CEvent::QueryInterface()
//
//  DESCRIPTION:
//
//      QI for the HTMLWindowEvents interface.
//
//  PARAMETERS:
//
//      riid        queried ID.
//      ppv         outbound pointer of interface to return to caller.
//
//  RETURNS:
//
//      HRESULT S_OK | E_NOINTERFACE.
//
//--------------------------------------------------------------------------------


STDMETHODIMP CEvent::QueryInterface(REFIID riid, void** ppv)
{
    assert( ppv );

    if ( !ppv )
        return(E_INVALIDARG);

    //--------------------------------------------------
    // make sure that this is never called before the
    // Init() method.
    //--------------------------------------------------

    assert(m_pIUnknown);


    *ppv = NULL;
      

    //--------------------------------------------------
    // [arunj 8/25/97]
    // Trident uses all three IIDs to establish a 
    // successful Advise session with us.  This violates
    // the reflexive property of this interface (QI on 
    // IUnknown should delegate to controlling IUnknown,
    // but we need to do this in order for events to work.
    //--------------------------------------------------

    if((riid == m_eventIID) || (riid == IID_IDispatch) || (riid == IID_IUnknown) )
    {
        *ppv = (IDispatch *)this;
        AddRef();
        return(S_OK);
    }
    else
    {
        //--------------------------------------------------
        // Delegate to controlling unknown for other 
        //  interfaces.
        //--------------------------------------------------

        return(m_pIUnknown->QueryInterface(riid,ppv));
    }
}


//--------------------------------------------------------------------------------
// CEvent::Invoke()
//
//  DESCRIPTION:
//
//      Overridden Invoke handles TOM Window object events.  
//      **NOTE** this implementation of Invoke() is pretty simple. 
//      basically we only care about the dispidMember parameter, which
//      we switch on and notify the client that an event of that type
//      has occurred.
//
//  PARAMETERS:
//
//      dispidMember    DISPID of fired event.
//      
//      riid            reserved for future use : must be NULL
//
//      lcid            locale ID : ignored for this version of MSAAHTML
//
//      wFlags          flags describing context in which call to Invoke()
//                      was made.
//  
//      pDispParams     arguments passed in.
//
//      pVarResult      pointer to location to store result in : set to NULL
//                      if user doesnt want returned value. 
//
//      pexcepinfo      pointer to exception information if applicable.
//
//      puArgErr        pointer to error code if applicable.
//

//  RETURNS:
//
//      HRESULT S_OK | DISP_E_MEMBERNOTFOUND .
//
//--------------------------------------------------------------------------------

STDMETHODIMP    CEvent::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, 
        UINT* puArgErr)
{

    
    BOOL bdispidFound = FALSE;


    //--------------------------------------------------
    // get the most derived classes' message map.
    //--------------------------------------------------

    const _EVENT_MAP_ * pMap = getEventMap();

    while(pMap && !bdispidFound)
    {

        //--------------------------------------------------
        // search current map for dispid match
        //--------------------------------------------------

        const _EVENT_MAP_ENTRY_ * pCurrentMap = pMap->pMapEntries;


        //--------------------------------------------------
        // it is possible for a map to have no entries :
        //--------------------------------------------------

        if(!pCurrentMap)
            break;


#ifdef _DEBUG

        TCHAR chDispID[256];
        TCHAR chEventID[256];

#endif

        //--------------------------------------------------
        // fire the matching event.
        //--------------------------------------------------

        for(; pCurrentMap->IsValidMapEntry != _EVENT_MAP_END_; pCurrentMap++)
        {
            if ( pCurrentMap->dispid == dispidMember )
            {

#ifdef _DEBUG

                getDispID(pCurrentMap->dispid,chDispID);
                getEventID(pCurrentMap->dwEvent,chEventID);

#endif

#ifdef _MSAA_EVENTS

                FireMSAAEvent( pCurrentMap->dwEvent );

#endif   // _MSAA_EVENTS

                bdispidFound = TRUE;
                break;
            }
        }

        //--------------------------------------------------
        // now move to base class map
        //--------------------------------------------------

        pMap = pMap->pBaseMap;
    }


    if ( !bdispidFound )
        return( DISP_E_MEMBERNOTFOUND );
    else
        return( S_OK );

}


#ifdef _DEBUG


//-----------------------------------------------------------------------
//  CEvent::getDispID()
//
//  DESCRIPTION:
//  returns a string corresponding to the input dispatch ID
//
//  PARAMETERS:
//  
//  ldispID     DISPID to switch on.
//  pchDispID   string to return
//
//  RETURNS:
//  none.
//
// ----------------------------------------------------------------------

void CEvent::getDispID(long ldispID,TCHAR * pchDispID)
{

    switch(ldispID)
    {

    case DISPID_HTMLELEMENTEVENTS_ONFOCUS:
        _tcscpy(pchDispID,_T("DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS\0"));
        break;
    
    case DISPID_HTMLELEMENTEVENTS_ONBLUR:
        _tcscpy(pchDispID,_T("DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR\0"));
        break;

    case DISPID_HTMLELEMENTEVENTS_ONCLICK:
        _tcscpy(pchDispID,_T("DISPID_HTMLELEMENTEVENTS_ONCLICK\0"));
        break;

    case DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE:
        _tcscpy(pchDispID,_T("DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE\0"));
        break;

    case DISPID_HTMLELEMENTEVENTS_ONKEYPRESS:
        _tcscpy(pchDispID,_T("DISPID_HTMLELEMENTEVENTS_ONKEYPRESS\0"));
        break;

    case DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONSELECT:
        _tcscpy(pchDispID,_T("DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONSELECT\0"));
        break;

    case DISPID_HTMLMARQUEEELEMENTEVENTS_ONBOUNCE:
        _tcscpy(pchDispID,_T("DISPID_HTMLMARQUEEELEMENTEVENTS_ONBOUNCE\0"));
        break;

    case DISPID_HTMLMARQUEEELEMENTEVENTS_ONFINISH:
        _tcscpy(pchDispID,_T("DISPID_HTMLMARQUEEELEMENTEVENTS_ONFINISH\0"));
        break;
    
    case DISPID_HTMLMARQUEEELEMENTEVENTS_ONSTART:
        _tcscpy(pchDispID,_T("DISPID_HTMLMARQUEEELEMENTEVENTS_ONSTART\0"));
        break;

    case DISPID_HTMLWINDOWEVENTS_ONLOAD:
        _tcscpy(pchDispID,_T("DISPID_HTMLWINDOWEVENTS_ONLOAD\0"));
        break;

    case DISPID_HTMLWINDOWEVENTS_ONUNLOAD:
        _tcscpy(pchDispID,_T("DISPID_HTMLWINDOWEVENTS_ONUNLOAD\0"));
        break;

    case DISPID_HTMLWINDOWEVENTS_ONRESIZE:
        _tcscpy(pchDispID,_T("DISPID_HTMLWINDOWEVENTS_ONRESIZE\0"));
        break;
    
    case DISPID_HTMLWINDOWEVENTS_ONBEFOREUNLOAD:
        _tcscpy(pchDispID,_T("DISPID_HTMLWINDOWEVENTS_ONBEFOREUNLOAD\0"));
        break;

    case DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE:
        _tcscpy(pchDispID,_T("DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE\0"));
        break;

    case DISPID_HTMLDOCUMENTEVENTS_ONSELECTSTART:
        _tcscpy(pchDispID,_T("DISPID_HTMLDOCUMENTEVENTS_ONSELECTSTART\0"));
        break;

    case DISPID_IHTMLBODYELEMENT_ONLOAD:
        _tcscpy(pchDispID,_T("DISPID_IHTMLBODYELEMENT_ONLOAD\0"));
        break;

    case DISPID_IHTMLBODYELEMENT_ONUNLOAD:
        _tcscpy(pchDispID,_T("DISPID_IHTMLBODYELEMENT_ONUNLOAD\0"));
        break;
    
    case DISPID_IHTMLCONTROLELEMENT_ONRESIZE:
        _tcscpy(pchDispID,_T("DISPID_IHTMLCONTROLELEMENT_ONRESIZE\0"));
        break;

    case DISPID_HTMLWINDOWEVENTS_ONSCROLL:
        _tcscpy(pchDispID,_T("DISPID_HTMLWINDOWEVENTS_ONSCROLL\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONAFTERUPDATE:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONAFTERUPDATE\0"));
        break;
                                                                            

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONBEFOREUPDATE:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONBEFOREUPDATE\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONERRORUPDATE:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONERRORUPDATE\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONROWEXIT:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONROWEXIT\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONROWENTER:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCHANGED\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCHANGED:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCHANGED\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONDATAAVAILABLE:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONDATAAVAILABLE\0"));
        break;

    case DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCOMPLETE:
        _tcscpy(pchDispID,_T("DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCOMPLETE\0"));
        break;


    default:
        _tcscpy(pchDispID,_T("event not mapped\0"));
        break;

    }
    
}


//-----------------------------------------------------------------------
//  CEvent::getEventID()
//
//  DESCRIPTION:
//  returns a string corresponding to the input Event ID
//
//  PARAMETERS:
//  
//  dwEventID       DISPID to switch on.
//  pchDispID   string to return
//
//  RETURNS:
//  none.
//
// ----------------------------------------------------------------------

void CEvent::getEventID(DWORD dwEventID,TCHAR * pchEventID)
{

    switch(dwEventID)
    {
    case EVENT_OBJECT_FOCUS:
        _tcscpy(pchEventID,_T("EVENT_OBJECT_FOCUS\0"));
        break;
    
    case EVENT_OBJECT_STATECHANGE:
        _tcscpy(pchEventID,_T("EVENT_OBJECT_STATECHANGE\0"));
        break;
    
    case EVENT_OBJECT_HELPCHANGE:
        _tcscpy(pchEventID,_T("EVENT_OBJECT_HELPCHANGE\0"));
        break;

    case EVENT_OBJECT_SELECTION:
        _tcscpy(pchEventID,_T("EVENT_OBJECT_SELECTION\0"));
        break;

    case EVENT_OBJECT_CREATE:
        _tcscpy(pchEventID,_T("EVENT_OBJECT_CREATE\0"));
        break;
        
    case EVENT_OBJECT_DESTROY:
        _tcscpy(pchEventID,_T("EVENT_OBJECT_DESTROY\0"));
        break;
        
    case EVENT_SYSTEM_ALERT:
        _tcscpy(pchEventID,_T("EVENT_SYSTEM_ALERT\0"));
        break;
        
    default:
        _tcscpy(pchEventID,_T("Event not mapped\0"));
    }
}

#endif  // _DEBUG

// end of EVENT.CPP
