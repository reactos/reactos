//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       peerfact.hxx
//
//  Contents:   peer factories
//
//----------------------------------------------------------------------------

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

MtExtern(CPeerFactoryUrl)
MtExtern(CPeerFactoryBinary)
#ifdef VSTUDIO7
MtExtern(CIdentityPeerFactoryUrl)
#endif //VSTUDIO7

MtExtern(CPeerFactoryUrl_aryDeferred_pv)

//+------------------------------------------------------------------------
//
//  Misc
//
//-------------------------------------------------------------------------

HRESULT FindPeer(
    IElementBehaviorFactory *   pFactory,
    LPTSTR                      pchName,
    LPTSTR                      pchUrl,
    IElementBehaviorSite *      pSite,
    IElementBehavior**          ppPeer);

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactory, abstract
//
//-------------------------------------------------------------------------

class CPeerFactory
{
public:
    virtual HRESULT FindBehavior(
        LPTSTR                  pchBehaviorName,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer) = 0;
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryUrl
//
//-------------------------------------------------------------------------

class CPeerFactoryUrl :
    public CDwnBindInfo,
    public IWindowForBindingUI,
    public CPeerFactory
{
public:
    DECLARE_CLASS_TYPES(CPeerFactoryUrl, CDwnBindInfo)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerFactoryUrl))

    //
    // consruction / destruction
    //

    CPeerFactoryUrl(CDoc *pDoc);
    ~CPeerFactoryUrl();

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) ()  { return super::AddRef();  };
    STDMETHOD_(ULONG, Release) () { return super::Release(); };

    
    //
    // subobject thunk
    //

    DECLARE_TEAROFF_TABLE(SubobjectThunk)

    NV_DECLARE_TEAROFF_METHOD(SubobjectThunkQueryInterface,     subobjectthunkqueryinterface, (REFIID, void **));
    NV_DECLARE_TEAROFF_METHOD_(ULONG, SubobjectThunkAddRef,     subobjectthunkaddref,         ()) { return SubAddRef(); }
    NV_DECLARE_TEAROFF_METHOD_(ULONG, SubobjectThunkSubRelease, subobjectthunkrelease,        ()) { return SubRelease();}

    //
    // CPeerFactory
    //

    virtual HRESULT FindBehavior(
        LPTSTR                  pchPeerName, 
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    //
    // CDwnBindInfo/IBindStatusCallback overrides
    //
    
    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding *pbinding);
    STDMETHOD(OnStopBinding)(HRESULT hrErr, LPCWSTR szErr);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);
    STDMETHOD(GetBindInfo)(DWORD * pdwBindf, BINDINFO * pbindinfo);
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);

    //
    // IWindowForBindingUI
    //

    DECLARE_TEAROFF_TABLE(IWindowForBindingUI)

    NV_DECLARE_TEAROFF_METHOD(GetWindow, getwindow, (REFGUID rguidReason, HWND *phwnd));

    //
    // methods
    //

    virtual void Passivate();

    static HRESULT Create(
                        LPTSTR                      pchUrl,
                        CDoc *                      pDoc,
                        CMarkup *                   pMarkup,
                        CPeerFactoryUrl **          ppFactory);

#ifdef VSTUDIO7
    virtual HRESULT Init (LPTSTR pchUrl);
#else
    HRESULT Init (LPTSTR pchUrl);
#endif //VSTUDIO7
    HRESULT Init (LPTSTR pchUrl, COleSite * pOleSite);
    HRESULT Clone(LPTSTR pchUrl, CPeerFactoryUrl ** ppPeerFactoryCloned);

    HRESULT AttachPeer    (CPeerHolder * pPeerHolder, BOOL fAfterDownload = FALSE);
    HRESULT AttachAllDeferred ();

    virtual HRESULT GetPeerName(CElement * pElement, LPTSTR pchName, LONG cchName);

    HRESULT LaunchUrlDownload(LPTSTR pchUrl);

    HRESULT OnStartBinding();
    HRESULT OnStopBinding();

    HRESULT OnOleObjectAvailable();

    void StopBinding();

    HRESULT PersistMonikerLoad(IUnknown * pUnk, BOOL fLoadOnce);

    inline BOOL HostOverrideBehaviorFactory()
        { return 0 != (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_OVERRIDEBEHAVIORFACTORY); }

    //
    // subclass CContext
    //

    class CContext :
        public IServiceProvider,
        public IScriptletSite
    {
    public:

        //
        // IUnknown
        //

        STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
        STDMETHOD_(ULONG, AddRef) ()  { return PFU()->SubAddRef();  };
        STDMETHOD_(ULONG, Release) () { return PFU()->SubRelease(); };

        //
        // IServiceProvider
        //

        STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppvObject);

        //
        // IScriptletSite
        //

        STDMETHOD(OnEvent)(DISPID dispid, int cArg, VARIANT *prgvarArg, VARIANT *pvarRes);
        STDMETHOD(GetProperty)(DISPID dispid, VARIANT *pvarRes);

        //
        // misc
        //

        inline CPeerFactoryUrl * PFU() { return CONTAINING_RECORD(this, CPeerFactoryUrl, _context); }
    };

    //
    // subclass COleReadyStateSink
    //

    class COleReadyStateSink : public IDispatch
    {
    public:

        //
        // IUnknown
        //

        DECLARE_TEAROFF_METHOD(QueryInterface,       queryinterface,         (REFIID riid, LPVOID * ppv));
        DECLARE_TEAROFF_METHOD_(ULONG, AddRef,       addref,                 ()) { return PFU()->SubAddRef();  };
        DECLARE_TEAROFF_METHOD_(ULONG, Release,      release,                ()) { return PFU()->SubRelease(); };

        //
        // IDispatch
        //

        STDMETHOD(GetTypeInfoCount) (UINT *pcTinfo) { RRETURN (E_NOTIMPL); };
        STDMETHOD(GetTypeInfo) (UINT itinfo, ULONG lcid, ITypeInfo ** ppTI) { RRETURN (E_NOTIMPL); };
        STDMETHOD(GetIDsOfNames) (REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid) { RRETURN (E_NOTIMPL); };
        STDMETHOD(Invoke) (
            DISPID          dispid,
            REFIID          riid,
            LCID            lcid,
            WORD            wFlags,
            DISPPARAMS *    pDispParams,
            VARIANT *       pvarResult,
            EXCEPINFO *     pExcepInfo,
            UINT *          puArgErr);

        //
        // misc
        //

        HRESULT SinkReadyState();

        inline CPeerFactoryUrl * PFU() { return CONTAINING_RECORD(this, CPeerFactoryUrl, _oleReadyStateSink); }
    };

    //
    // data
    //

    enum TYPE {
        TYPE_UNKNOWN,
        TYPE_NULL,
        TYPE_PEERFACTORY,
        TYPE_CLASSFACTORY,
        TYPE_CLASSFACTORYEX,
        TYPE_DEFAULT
    }                           _type;

    enum DOWNLOADSTATUS {
        DOWNLOADSTATUS_NOTSTARTED,
        DOWNLOADSTATUS_INPROGRESS,
        DOWNLOADSTATUS_DONE
    }                           _downloadStatus;

    CStr                        _cstrUrl;  // "http://... foo.htc"     TYPE_CLASSFACTORY,   
                                           //                          TYPE_CLASSFACTORYEX  htcs, scriptlets, etc.
                                           // "#bar#foo"               TYPE_PEERFACTORY    binary
                                           // "#default",
                                           // "#default#foo            TYPE_DEFAULT        default (iepeers), host, etc.

    DECLARE_CPtrAry(CAryDeferred, CPeerHolder*, Mt(Mem), Mt(CPeerFactoryUrl_aryDeferred_pv))
    CAryDeferred                _aryDeferred;       // array populated while download is in progress and
                                                    // used when download is complete

    IMoniker *                  _pMoniker;

    IUnknown *                  _pFactory;

    CDoc *                      _pDoc;

    // CONSIDER: (alexz) moving these in a union:
    // when _pBinding used, _pOleSite and _oleReadyStateSink are not, and vice versa
    IBinding *                  _pBinding;
    DWORD                       _dwBindingProgCookie;
    COleSite *                  _pOleSite;

    COleReadyStateSink          _oleReadyStateSink;
    CContext                    _context;
};

#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Class:     CIdentityPeerFactoryUrl
//
//-------------------------------------------------------------------------

class CIdentityPeerFactoryUrl : public CPeerFactoryUrl
{
public:
    
    //
    // consruction / destruction
    //
    CIdentityPeerFactoryUrl(CDoc * pDoc);
    
    virtual HRESULT Init (LPTSTR pchUrl);
    HRESULT Init(IIdentityBehaviorFactory *pFactory);

    //
    // CDwnBindInfo/IBindStatusCallback overrides
    //
      
    STDMETHOD(OnStopBinding)(HRESULT hrErr, LPCWSTR szErr);

    virtual HRESULT GetPeerName(CElement * pElement, LPTSTR pchName, LONG cchName);

    BOOL   _fContextSleeping;
    DWORD  _dwScriptCookie;
};
#endif //VSTUDIO7

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryBuiltin
//
//-------------------------------------------------------------------------

class CPeerFactoryBuiltin : public CPeerFactory
{
public:

    //
    // CPeerFactory virtuals
    //

    virtual HRESULT FindBehavior(
        LPTSTR                  pchBehaviorName,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    //
    // data
    //

    const CBuiltinGenericTagDesc * _pTagDesc;
};

//+------------------------------------------------------------------------
//
//  Class:     CPeerFactoryBuiltin
//
//-------------------------------------------------------------------------

class CPeerFactoryBinary : public CPeerFactory
{
public:
    //
    // methods
    //

    CPeerFactoryBinary();
    ~CPeerFactoryBinary();

    HRESULT AttachPeer (CPeerHolder * pPeerHolder, LPTSTR pchUrl);

    //
    // CPeerFactory virtuals
    //

    virtual HRESULT FindBehavior(
        LPTSTR                  pchBehaviorName,
        IElementBehaviorSite *  pSite,
        IElementBehavior **     ppPeer);

    //
    // data
    //

    IElementBehaviorFactory *   _pFactory;
    LPTSTR                      _pchUrl;
};
