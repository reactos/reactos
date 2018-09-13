#ifndef I_HTC_HXX_
#define I_HTC_HXX_
#pragma INCMSG("--- Beg 'htc.hxx'")

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

enum HTC_BEHAVIOR_TYPE
{
    HTC_BEHAVIOR_NONE       = 0x00,
    HTC_BEHAVIOR_DESC       = 0x01,
    HTC_BEHAVIOR_PROPERTY   = 0x02,
    HTC_BEHAVIOR_METHOD     = 0x04,
    HTC_BEHAVIOR_EVENT      = 0x08,
    HTC_BEHAVIOR_ATTACH     = 0x10,

    HTC_BEHAVIOR_PROPERTYORMETHOD        = HTC_BEHAVIOR_PROPERTY | HTC_BEHAVIOR_METHOD,
    HTC_BEHAVIOR_PROPERTYOREVENT         = HTC_BEHAVIOR_PROPERTY | HTC_BEHAVIOR_EVENT,
    HTC_BEHAVIOR_PROPERTYORMETHODOREVENT = HTC_BEHAVIOR_PROPERTY | HTC_BEHAVIOR_METHOD | HTC_BEHAVIOR_EVENT
};

extern CLSID CLSID_CHtmlComponentConstructor;
extern CLSID CLSID_CHtmlComponent;

MtExtern(CHtmlComponentConstructor)
MtExtern(CHtmlComponentBase)
MtExtern(CHtmlComponent)
MtExtern(CHtmlComponentDD)
MtExtern(CHtmlComponentProperty)
MtExtern(CHtmlComponentMethod)
MtExtern(CHtmlComponentEvent)
MtExtern(CHtmlComponentAttach)
MtExtern(CHtmlComponentDesc)

MtExtern(CProfferService_CItemsArray)


class CMarkupContext;

class CHtmlComponent;
class CHtmlComponentEvent;
class CHtmlComponentAttach;
class CProfferService;

#define _hxx_
#include "htc.hdl"

// Not defined in PDL as this is internal to HTCs - 5000 should give us enough room
#define BEHAVIOREVENT_INTERNAL_DETACH           5000


//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentConstructor
//
//-------------------------------------------------------------------------

class CHtmlComponentConstructor :
    public CStaticCF,
    public IElementBehaviorFactory
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentConstructor, CStaticCF)

    CHtmlComponentConstructor() : super(NULL) {};

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)()     { return super::AddRef();  };
    STDMETHOD_(ULONG, Release)()    { return super::Release(); };

    //
    // IElementBehaviorFactory
    //

    STDMETHOD(FindBehavior)
        (BSTR bstrName, BSTR bstrUrl, IElementBehaviorSite * pSite, IElementBehavior ** ppBehavior); 
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentBase
//
//-------------------------------------------------------------------------

class CHtmlComponentBase : public CBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentBase, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentBase))

    //
    // methods
    //

    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentBase)

    void Passivate();

    HRESULT InvokeEngines(
        LPTSTR          pchName,
        WORD            wFlags,
        DISPPARAMS *    pDispParams,
        VARIANT *       pvarRes,
        EXCEPINFO *     pExcepInfo,
        IServiceProvider * pSrvProvider);

           LPTSTR InternalName(BOOL * pfScriptsOnly = NULL, WORD * pwFlags = NULL, DISPPARAMS * pDispParams = NULL);
    inline LPTSTR ExternalName();
           LPTSTR ChildInternalName(LPTSTR pchChild);

    void EnsureComponent();
    virtual void OnComponentSet() { }
    
    //
    // IElementBehavior
    //

    DECLARE_TEAROFF_TABLE(IElementBehavior)

	DECLARE_TEAROFF_METHOD(Init, init, (IElementBehaviorSite * pSite));
    DECLARE_TEAROFF_METHOD(Notify, notify, (LONG lEvent, VARIANT * pVar));
    DECLARE_TEAROFF_METHOD(Detach, detach, ());

    //
    // wiring
    //

    enum { DISPID_COMPONENTBASE = 1 };

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // lock
    //

    class CLock
    {
    public:
        CLock (CHtmlComponentBase * pHtmlComponentBase)
        {
            _pHtmlComponentBase = pHtmlComponentBase;
            if (_pHtmlComponentBase)
            {
                _pHtmlComponentBase->PrivateAddRef();

                if (_pHtmlComponentBase->_pElement)
                {
                    _pHtmlComponentBase->_pElement->PrivateAddRef();
                }
            }
        };
        ~CLock ()
        {
            if (_pHtmlComponentBase)
            {
                if (_pHtmlComponentBase->_pElement)
                {
                    _pHtmlComponentBase->_pElement->PrivateRelease();
                }

                _pHtmlComponentBase->PrivateRelease();
            }
        };

        CHtmlComponentBase * _pHtmlComponentBase;
    };

    //
    // data
    //

    IElementBehaviorSite *  _pSite;
    CHtmlComponent *        _pComponent;            // weak ref
    CElement *              _pElement;              // weak ref, could be NULL if detached

    unsigned                _fComponentEnsured:1;
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentDD (a.k.a. CHtmlComponentDefaultDispatch)
//
//-------------------------------------------------------------------------

class CHtmlComponentDD : public CBase
{
public:
    DECLARE_CLASS_TYPES(CHtmlComponentDD, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentDD))

    STDMETHOD(PrivateQueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pSrvProvider));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown ** ppunk));

    //
    // wiring
    //

    #define _CHtmlComponentDD_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // methods
    //

    CHtmlComponent *    Component();

    //
    // data
    //

#if DBG == 1
    CHtmlComponent *    _pComponentDbg;
#endif
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponent
//
//-------------------------------------------------------------------------

class CHtmlComponent : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponent, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponent))

    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponent)

    CHtmlComponent();
    ~CHtmlComponent();

    //
    // IElementBehavior
    //

    STDMETHOD(Init)(IElementBehaviorSite * pSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);
    STDMETHOD(Detach)(void);

    //
    // IPersistMoniker
    //

    DECLARE_TEAROFF_TABLE(IPersistMoniker)

    //NV_DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID *pClassID)) { return E_NOTIMPL; };
    //NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, (void)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(Save, save, ( 
         IMoniker * pimkName,
         LPBC       pbc,
         BOOL       fRemember)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(SaveCompleted, savecompleted, ( 
         IMoniker * pimkName,
         LPBC       pibc)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(GetCurMoniker, getcurmoniker, (IMoniker **ppimkName)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(Load, load, (
         BOOL       fFullyAvailable,
         IMoniker * pimkName,
         LPBC       pibc,
         DWORD      grfMode));

    //
    // IDispatchEx
    //

    DECLARE_TEAROFF_TABLE(IDispatchEx)

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (
        BSTR        bstrName,
        DWORD       grfdex,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID      dispid,
        LCID        lcid,
        WORD        wFlags,
        DISPPARAMS *pdispparams,
        VARIANT *   pvarRes,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider));

    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
        DWORD       grfdex,
        DISPID      dispid,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (
        DISPID      dispid,
        BSTR *      pbstrName));


    //
    // IServiceProvider
    //

    DECLARE_TEAROFF_TABLE(IServiceProvider)

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject));

    //
    // IPersistPropertyBag2
    //

    DECLARE_TEAROFF_TABLE(IPersistPropertyBag2)

    NV_DECLARE_TEAROFF_METHOD(GetClassID,   getclassid, (CLSID * pClsid)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(InitNew,      initnew,    ()) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(Load,         load,       (IPropertyBag2 * pPropBag, IErrorLog * pErrLog)) { return E_NOTIMPL; };
    NV_DECLARE_TEAROFF_METHOD(Save,         save,       (IPropertyBag2 * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties));
    NV_DECLARE_TEAROFF_METHOD(IsDirty,      isdirty,    ()) { return E_NOTIMPL; };

    //
    // methods
    //

    void    Passivate();

    void    OnLoadStatus(LOADSTATUS LoadStatus);

    static  HRESULT FindBehavior(HTC_BEHAVIOR_TYPE type, IElementBehaviorSite * pSite, IElementBehavior ** ppPeer);

    HRESULT RegisterEvent        (CHtmlComponentEvent *        pEvent);

    HRESULT AttachNotification(DISPID dispid, IDispatch * pdispHandler);
    HRESULT FireNotification(LONG lEvent, VARIANT *pvar);

            HRESULT GetHtcElement(LONG * pIdxStart, HTC_BEHAVIOR_TYPE type, CElement ** ppElement);
    inline  HRESULT GetHtcElement(LONG    idxStart, HTC_BEHAVIOR_TYPE type, CElement ** ppElement)
    {
        RRETURN (GetHtcElement(&idxStart, type, ppElement));
    };

    BOOL    IsRecursiveUrl(LPTSTR pchUrl);

    HRESULT GetReadyState(READYSTATE * pReadyState);

    //
    // wiring
    //

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const { return  (CBase::CLASSDESC *)&s_classdesc; }

    static const CONNECTION_POINT_INFO s_acpi[];

    //
    // data
    //

    IElementBehaviorSiteOM *    _pSiteOM;
    IPropertyNotifySink *       _pPropertyNotifySink;
    CDoc *                      _pDoc;
    CMarkup *                   _pMarkup;
    CHtmlComponentDD            _DD;

    CProfferService *           _pProfferService;

    BOOL                        _fContentReadyPending  : 1;
    BOOL                        _fDocumentReadyPending : 1;
};

inline CHtmlComponent * CHtmlComponentDD::Component()
{
    Assert (_pComponentDbg == CONTAINING_RECORD(this, CHtmlComponent, _DD));

    return CONTAINING_RECORD(this, CHtmlComponent, _DD);
}


//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentProperty
//
//-------------------------------------------------------------------------

class CHtmlComponentProperty : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentProperty, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentProperty))

    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentProperty)

    //
    // methods
    //

    DECLARE_TEAROFF_METHOD(Notify, notify, (LONG lEvent, VARIANT * pVar));

    void    EnsureHtmlLoad(BOOL fScriptsOnly);
    HRESULT       HtmlLoad(BOOL fScriptsOnly);
    
    HRESULT InvokeItem (
        DISPID              dispid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pDispParams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pExcepInfo,
        IServiceProvider *  pServiceProvider,
        BOOL                fScriptsOnly);

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID              dispid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pDispParams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pExcepInfo,
        IServiceProvider *  pServiceProvider));

    //
    // wiring
    //

    #define _CHtmlComponentProperty_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    BOOL                _fHtmlLoadEnsured:1;
};


//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentMethod
//
//-------------------------------------------------------------------------

class CHtmlComponentMethod : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentMethod, CHtmlComponentBase)
    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentMethod)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentMethod))

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
        DISPID              dispid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider));

    //
    // wiring
    //

    #define _CHtmlComponentMethod_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentEvent
//
//-------------------------------------------------------------------------

class CHtmlComponentEvent : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentEvent, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentEvent))
    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentEvent)

    //
    // methods
    //

    virtual void    OnComponentSet();

    //
    // wiring
    //

    #define _CHtmlComponentEvent_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    LONG        _lCookie;
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentAttach
//
//-------------------------------------------------------------------------

class CHtmlComponentAttach : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentAttach, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentAttach))
    DECLARE_PRIVATE_QI_FUNCS(CHtmlComponentAttach)
    
    //
    // methods
    //

	DECLARE_TEAROFF_METHOD(Init, init, (IElementBehaviorSite * pSite));
    DECLARE_TEAROFF_METHOD(Notify, notify, (LONG lEvent, VARIANT * pVar));

    virtual void Passivate();

    inline LPTSTR GetEventName();
    CBase *       GetEventSource();
    IDispatch *   GetEventHandler();

    HRESULT Attach1();
    HRESULT Attach2();
    HRESULT SinkEvent(BOOL fAttach);

    HRESULT CreateEventObject(IDispatch * pdispArg, IHTMLEventObj ** ppEventObject);
    HRESULT FireHandler (IHTMLEventObj * pEventObject);
    HRESULT FireHandler2(IHTMLEventObj * pEventObject);

#if DBG == 1
    HRESULT TestProfferService();
#endif

    //
    // wiring
    //

    #define _CHtmlComponentAttach_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //

    IElementBehaviorSiteOM *    _pSiteOM;

    unsigned                    _fEvent:1;   // TRUE if this is really an event.
};

//+------------------------------------------------------------------------
//
//  Class:  CHtmlComponentDesc
//
//-------------------------------------------------------------------------

class CHtmlComponentDesc : public CHtmlComponentBase
{
public: 
    DECLARE_CLASS_TYPES(CHtmlComponentDesc, CHtmlComponentBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlComponentDesc))

    //
    // methods
    //

    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVar);

    //
    // wiring
    //

    #define _CHtmlComponentDesc_
    #include "htc.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // data
    //
};

//+------------------------------------------------------------------------
//
//  Class:  CProfferService
//
//-------------------------------------------------------------------------

class CProfferServiceItem
{
public:
    CProfferServiceItem (REFGUID refguid, IServiceProvider * pSP)
    {
        _guidService = refguid;
        _pSP = pSP;
        _pSP->AddRef();
    }

    ~CProfferServiceItem()
    {
        _pSP->Release();
    }

    GUID                _guidService;
    IServiceProvider *  _pSP;
};

class CProfferService : public IProfferService
{
public:

    //
    // methods and wiring
    //

    CProfferService();
    ~CProfferService();

    DECLARE_FORMS_STANDARD_IUNKNOWN(CProfferTestObj);

    STDMETHOD(ProfferService)(REFGUID rguidService, IServiceProvider * pSP, DWORD * pdwCookie);
    STDMETHOD(RevokeService) (DWORD dwCookie);

    HRESULT QueryService(REFGUID rguidService, REFIID riid, void ** ppv);

    //
    // data
    //

    DECLARE_CPtrAry(CItemsArray, CProfferServiceItem*, Mt(Mem), Mt(CProfferService_CItemsArray));

    CItemsArray     _aryItems;
};

#pragma INCMSG("--- End 'htc.hxx'")
#else
#pragma INCMSG("*** Dup 'htc.hxx'")
#endif
