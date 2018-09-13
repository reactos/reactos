#ifndef _EVENT_H
#define _EVENT_H

//=======================================================================
//      File:   EVENT.H
//      Date:   7/24/97
//      Desc:   Contains definition of CEvent class, an abstrast base
//              base defining the general interface for derived event
//              handling classes for specific AOs and AEs.
//=======================================================================

//=======================================================================
//  Includes                  
//=======================================================================

//=======================================================================
//  Defines
//=======================================================================

//================================================================================
// DISPID MAP structures  and macros
//================================================================================


//--------------------------------------------------
// used to signal loops that they are at the 
// last map entry
//--------------------------------------------------

#define _EVENT_MAP_END_ -1


//--------------------------------------------------
// map entry structure :
// contains validity flag, dispid, and event 
// corresponding to that dispid
//--------------------------------------------------

struct _EVENT_MAP_ENTRY_
{
    long IsValidMapEntry;
    DISPID dispid;
    DWORD  dwEvent;
};


//--------------------------------------------------
// map structure : points to base map (to enable
// default implementation in base classes) and
// to the current class map entry array
//--------------------------------------------------

struct _EVENT_MAP_
{
    const _EVENT_MAP_ * pBaseMap;
    const _EVENT_MAP_ENTRY_ * pMapEntries;
};



//--------------------------------------------------
// call DECLARE_EVENT_MAP() in the declaration
// of the event class.  the variables and method
// declared here are implemented in BEGIN_DISPID_MAP()
//--------------------------------------------------

#define  DECLARE_EVENT_MAP() \
private: \
    static const _EVENT_MAP_ENTRY_ m_event_mapEntries[]; \
protected: \
    static const _EVENT_MAP_ m_event_map; \
    virtual const _EVENT_MAP_ * getEventMap() const; 



//--------------------------------------------------
// call BEGIN_EVENT_MAP() in the definition
// of the event class.  
//--------------------------------------------------

#define BEGIN_EVENT_HANDLER_MAP(ownerClass,thisClass,baseClass) \
const _EVENT_MAP_* ownerClass::C##thisClass::getEventMap() const \
{ return(&ownerClass::C##thisClass::m_event_map); } \
const _EVENT_MAP_ ownerClass::C##thisClass::m_event_map = \
{ &baseClass::m_event_map,  &ownerClass::C##thisClass::m_event_mapEntries[0] }; \
const _EVENT_MAP_ENTRY_ ownerClass::C##thisClass::m_event_mapEntries[] = { 
  
//--------------------------------------------------
// END_EVENT_MAP() must be called whenever 
// BEGIN_EVENT_MAP() is called or you will get
// compile time errors.
//--------------------------------------------------

#define END_EVENT_HANDLER_MAP() \
{ _EVENT_MAP_END_,0,0 } \
}; 


//--------------------------------------------------
// ON_DISPID_FIRE_EVENT() macro is used to create
// map entries.  
//--------------------------------------------------

#define ON_DISPID_FIRE_EVENT(dispid,dwEvent) \
{0,dispid,dwEvent},


//--------------------------------------------------
// DECLARE_EVENT_HANDLER() is used to declare 
// a derived class and a member of that class in
// the owner class declaration.
//--------------------------------------------------

#define DECLARE_EVENT_HANDLER(theClass,baseClass,EventIID) \
class C##theClass : public baseClass \
{ \
public: \
HRESULT Init(LPUNKNOWN pIUnknown,HWND hWnd,UINT nChildID, LPUNKNOWN pTOMObjIUnk) \
{ \
    return(baseClass::Init(pIUnknown,EventIID,hWnd,nChildID, pTOMObjIUnk)); \
} \
DECLARE_EVENT_MAP() \
}; \
C##theClass m_##theClass;
  


//--------------------------------------------------
// DECLARE_NOTIFY_OWNER_EVENT_HANDLER() macro is
// used when the owner class needs to be notified 
// of all events.  The owner class needs to 
// implement the Notify() method, or you will get
// an unresolved external at link time.
//--------------------------------------------------


#define  DECLARE_NOTIFY_OWNER_EVENT_HANDLER(ownerClass,theClass,baseClass,EventIID) \
public: \
void Notify(DISPID idEvent);    \
protected:  \
class C##theClass : public baseClass \
{ \
public: \
C##theClass():baseClass() { m_pOwnerClass = NULL; } \
HRESULT Init(LPUNKNOWN pIUnknown,HWND hWnd,UINT nChildID, LPUNKNOWN pTOMObjIUnk,ownerClass* pOwnerClass) \
{ \
    m_pOwnerClass = pOwnerClass; \
    return(baseClass::Init(pIUnknown,EventIID,hWnd,nChildID, pTOMObjIUnk));     \
} \
virtual STDMETHODIMP    Invoke(DISPID dispidMember, REFIID riid, LCID lcid,     \
        WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, \
        UINT* puArgErr) \
{   \
    HRESULT hr;     \
    hr = baseClass::Invoke(dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr); \
    if(hr != S_OK)  \
        return(hr); \
    ((ownerClass *)m_pOwnerClass)->Notify(dispidMember);    \
    return(hr); \
}   \
DECLARE_EVENT_MAP() \
protected: \
    ownerClass* m_pOwnerClass; \
}; \
C##theClass* m_p##theClass; 

    

//--------------------------------------------------
// NULL_NOTIFY_EVENT_HANDLER_PTR is used to init
// the event handler object pointer to NULL
//-------------------------------------------------- 

#define NULL_NOTIFY_EVENT_HANDLER_PTR(theClass) \
m_p##theClass = NULL;


//--------------------------------------------------
// CREATE_NOTIFY_EVENT_HANDLER is used to create
// the event handler
//-------------------------------------------------- 

#define CREATE_NOTIFY_EVENT_HANDLER(theClass) \
((m_p##theClass = (C##theClass*) new C##theClass()) ? S_OK : E_OUTOFMEMORY)


//--------------------------------------------------
// DESTROY_NOTIFY_EVENT_HANDLER is used to destroy
// the event handler
//-------------------------------------------------- 

#define DESTROY_NOTIFY_EVENT_HANDLER(theClass) \
if ( m_p##theClass ) \
{ m_p##theClass->Detach(); m_p##theClass->Release(); m_p##theClass = NULL; }


//--------------------------------------------------
// INIT_NOTIFY_EVENT_HANDLER is used to initialize
// the event handler
//-------------------------------------------------- 

#define INIT_NOTIFY_EVENT_HANDLER(theClass,pIUnknown,hWnd,nChildID,pTOMObjIUnk,pOwnerClass) \
m_p##theClass->Init(pIUnknown,hWnd,nChildID,pTOMObjIUnk,pOwnerClass);


//--------------------------------------------------
// INIT_EVENT_HANDLER is used to initialize the 
// event handler
//-------------------------------------------------- 

#define INIT_EVENT_HANDLER(theClass,pIUnknown,hWnd,nChildID,pTOMObjIUnk) \
m_##theClass.Init(pIUnknown,hWnd,nChildID,pTOMObjIUnk);


//--------------------------------------------------
// ASSIGN_TO_NOTIFY_EVENT_HANDLER() is used to
// delegate QIs to the generated class.
//--------------------------------------------------

#define ASSIGN_TO_NOTIFY_EVENT_HANDLER(theClass,ppv,pCastToPtr) \
*ppv = (pCastToPtr *)m_p##theClass;


//--------------------------------------------------
// ASSIGN_TO_EVENT_HANDLER() is used to delegate QIs
// to the generated class.
//--------------------------------------------------

#define ASSIGN_TO_EVENT_HANDLER(theClass,ppv,pCastToPtr) \
*ppv = (pCastToPtr *)&m_##theClass;



//--------------------------------------------------
// FIRE_EVENT is used by the parent class to 
// fire events directly (w/o TOM initiation)
//--------------------------------------------------

#define FIRE_EVENT(theClass,dwEvent) \
m_##theClass.FireMSAAEvent(dwEvent);





//--------------------------------------------------
// These debug macros are used to map DISPIDs and Event
// DWORDs to a more readable string output.
// TODO : is there a way to paste the ID into a string ?
// right now, I cant figure out how to do it.  
//--------------------------------------------------

#ifdef _DEBUG


#define START_IDTOSTRING_MAP(theID) \
    switch(theID)  \
    {

#define IDTOSTRING(id,string) \
    case id: \
        lstrcpy(string,"##id"); \
        break;

#define END_IDTOSTRING_MAP()  \
    }


#endif

//================================================================================
//  CEvent class definition
//================================================================================

class CEvent : public IDispatch
{

public:

    //--------------------------------------------------
    // IUnknown 
    //--------------------------------------------------
    
virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);

    
    //--------------------------------------------------
    // this object isn't linked to its controlling 
    // object anymore since it handles QIs for IUnknown.
    // Since it doesnt delegate IUnknown QIs to the 
    // controlling object, it shouldnt delegate AddRef()
    // or Release() calls either.
    //--------------------------------------------------

virtual STDMETHODIMP_(ULONG)    AddRef(void) 
    { 
        return(m_cRef++); 
    }
    
virtual STDMETHODIMP_(ULONG)    Release(void) 
    { 
        if ( m_cRef > 0 )
            m_cRef--;
        else
        {
            assert(FALSE);
            return 0;
        }

        // if cRef ==0 we are outa here
        if (!m_cRef)
        {
            delete this;
            return 0;
        }

        return m_cRef;
    }
    
    //--------------------------------------------------
    // IDispatch
    //--------------------------------------------------

    virtual STDMETHODIMP    GetTypeInfoCount(UINT* pctinfo) { return(S_OK); }
    virtual STDMETHODIMP    GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) 
    { 
        return(S_OK); 
    }

    virtual STDMETHODIMP    GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
        UINT cNames, LCID lcid, DISPID* rgdispid) 
    { 
        return(S_OK); 
    }
        
    virtual STDMETHODIMP    Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, 
        UINT* puArgErr);

    //--------------------------------------------------
    // Ctor/dtor
    //--------------------------------------------------

    CEvent();
    ~CEvent();
    
    virtual HRESULT Init(LPUNKNOWN pIUnknown, REFIID riid, HWND hwnd, UINT nContainerChildID,LPUNKNOWN pTOMObjUnk);     
    virtual void Detach();
    
#ifdef _MSAA_EVENTS

    void FireMSAAEvent( DWORD dwEvent )
    {
        if(m_bAccessibleObject)
            NotifyWinEvent(dwEvent,m_hWnd,m_nParentAOID,CHILDID_SELF);
        else
            NotifyWinEvent(dwEvent,m_hWnd,m_nParentAOID,m_nContainerChildID);
    }
#endif // _MSAA_EVENTS

protected:

#ifdef _DEBUG

    void getDispID(long ldispID,TCHAR * pchDispID);
    void getEventID(DWORD dwEventID,TCHAR * pchEventID);

#endif

    //--------------------------------------------------
    // Pointer to containing IUnknown and containing
    //  object (use the IUnknown for QI, Addref, Release.
    //  use the containing object to access non IUnknown
    //  functionality)
    //--------------------------------------------------

    GUID                 m_eventIID;        // must set in derived class!
    IUnknown            *m_pIUnknown;
    HWND                 m_hWnd;
    UINT                 m_nContainerChildID;
    UINT                 m_nParentAOID;
    IConnectionPoint    *m_pIConnectionPoint;
    DWORD                m_dwAdviseCookie;
    BOOL                 m_bAccessibleObject;
    UINT                 m_cRef;


    //--------------------------------------------------
    // event map :
    // the CEvent class doesn't implement an event map :
    // the event map variables here are stubs so 
    // that the event invocation process knows where
    // to stop.
    // 
    // Initialization of these members is done at the
    // beginning of the source file.
    //--------------------------------------------------

private: 
    
    static const _EVENT_MAP_ENTRY_ m_event_mapEntries[]; 
    
protected: 

    static const _EVENT_MAP_ m_event_map; 
    virtual const _EVENT_MAP_ * getEventMap() const
    {
        return &m_event_map;
    }; 
};


#endif  // _EVENT_H
